#include "optenc.h"
#include <ilcplex/ilocplex.h>
#include <boost/lexical_cast.hpp>
#include <iterator>
#include <limits.h>

ILOSTLBEGIN

namespace ckt_n {

    ilp_encoder_t::ilp_encoder_t(ast_n::statements_t& stms, double fraction)
        : ckt(stms)
    {
        target_keys = int(ckt.num_gates() * fraction + 0.5);
    }

    ilp_encoder_t::~ilp_encoder_t()
    {
    }

    int ilp_encoder_t::get_min_output_level(node_t* n, std::vector<bool>& output_map)
    {
        int min_output_level = INT_MAX;
        assert(output_map.size() == ckt.num_outputs());
        for(unsigned i=0; i != output_map.size(); i++) {
            if(output_map[i]) {
                if(ckt.outputs[i]->level < min_output_level) {
                    min_output_level = ckt.outputs[i]->level;
                }
            }
        }
        assert(min_output_level != INT_MAX);
        return min_output_level;
    }

    int ilp_encoder_t::should_use_min_cnst(node_t* n, std::vector<bool>& output_map)
    {
        return 0;
        //return (1.2*n->level) > get_min_output_level(n, output_map);
    }

    void ilp_encoder_t::encode3()
    {
        std::vector< std::vector<bool> >output_maps;
        ckt.init_outputmap(output_maps);

        IloEnv env;
        IloNumVarArray vars(env);

        int numNodes = ckt.num_nodes();
        // first create the variables which count the number of keys
        // in the fanin cone. Let's call this n_i
        for(int i=0; i != numNodes; i++) {
            std::string name = "n" + boost::lexical_cast<std::string>(i);
            vars.add(IloNumVar(env, 0, IloInfinity, name.c_str()));
        }
        // now a boolean variable that counts whether a key was inserted
        // at the output of this node. Let's call these b_i.
        for(int i=0; i != numNodes; i++) {
            std::string name = "b" + boost::lexical_cast<std::string>(i);
            vars.add(IloNumVar(env, 0, 1, name.c_str()));
        }
        int maxVar = 2*numNodes;
        int totalVars = maxVar + 1;
        std::string name = "maxKeys";
        vars.add(IloNumVar(env, 0, IloInfinity, name.c_str()));

        int cnstPos = 0;
        // Add the constraints that minimize over the keys in the fanin.
        // The constraint basically says: n_i <= min(n_i1+b_i, n_i2+b_i, ... )
        // This translates to n_i <= n_i1 + b_i <=> n_i - n_i1 - b_i <= 0
        IloRangeArray cnst(env);
        for(int i=0; i != numNodes; i++) {
            node_t* ni = ckt.nodes[i];
            assert(ni->get_index() == i);

            // i - j1 - j2 ... -jN - bi = 0
            if(ni->num_inputs() == 0) {
                std::string name = "c_inp_" + ni->name;
                cnst.add(IloRange(env, 0, 0, name.c_str()));
                cnst[cnstPos].setLinearCoef(vars[i], 1);    // n_i
                cnst[cnstPos].setLinearCoef(vars[i+numNodes], -1); // b_i
                cnstPos += 1;
            } else {
                if(should_use_min_cnst(ni, output_maps[ni->get_index()])) {
                    for(unsigned j=0; j != ni->num_inputs(); j++) {
                        node_t* nj = ni->inputs[j];
                        int jj = nj->get_index();

                        // create constraint.
                        std::string name = "c_gate_min_" + ni->name + "_" + nj->name;
                        cnst.add(IloRange(env, -IloInfinity, 0, name.c_str()));
                        cnst[cnstPos].setLinearCoef(vars[i], 1);    // n_i
                        cnst[cnstPos].setLinearCoef(vars[jj], -1);  // n_i1
                        cnst[cnstPos].setLinearCoef(vars[i+numNodes], -1); // b_i
                        cnstPos += 1;
                    }
                } else {
                    std::string name = "c_gate_add_" + ni->name;
                    cnst.add(IloRange(env, 0, 0, name.c_str()));
                    // i - j1 - j2 ... -jN - bi = 0
                    for(unsigned j=0; j != ni->num_inputs(); j++) {
                        node_t* nj = ni->inputs[j];
                        int jj = nj->get_index();
                        // ji
                        cnst[cnstPos].setLinearCoef(vars[jj], -1);
                    }
                    // i
                    cnst[cnstPos].setLinearCoef(vars[i], 1);
                    // bi
                    cnst[cnstPos].setLinearCoef(vars[i+numNodes], -1);
                    cnstPos += 1;
                }
            }
        }

        // b1 + b2 + ... + bi <= target_keys.
        cnst.add(IloRange(env, 0, target_keys, "cKeyTarget"));
        for(int i=0; i != numNodes; i++) {
            int vi = i+numNodes;
            cnst[cnstPos].setLinearCoef(vars[vi], 1);
        }
        cnstPos += 1;

        // n_i >= maxVar
        for(unsigned i=0; i != ckt.num_outputs(); i++) {
            node_t* oi = ckt.outputs[i];
            int idx = oi->get_index();
            std::string name = "c_out_" + oi->name;
            // n_i >= maxVar
            // i.e., n_i - maxVar >= 0
            cnst.add(IloRange(env, 0, IloInfinity, name.c_str()));
            cnst[cnstPos].setLinearCoef(vars[idx], 1);
            cnst[cnstPos].setLinearCoef(vars[maxVar], -1);
            cnstPos += 1;
        }

        // objective maximize maxVars;
        IloObjective obj = IloMaximize(env);
        IloNumArray coeffs(env, totalVars);
        coeffs[maxVar] = 1;
        obj.setLinearCoefs(vars, coeffs);

        IloModel model(env);
        model.add(obj);
        model.add(cnst);
        IloCplex cplex(model);
        //cplex.setOut(env.getNullStream());
        //cplex.setParam(IloCplex::TiLim, 1);
        //cplex.setParam(IloCplex::EpGap, 0.25);
        
        cplex.exportModel("encmodel.lp");
        if(cplex.solve()) {
            std::cout << "Status    : " << cplex.getStatus() << std::endl;
            std::cout << "Value     : " << cplex.getObjValue() << std::endl;

            IloNumArray vals(env);
            cplex.getValues(vals, vars);

            // extract solution.
            nodeset_t insertion_nodes;
            for(int i=0; i != numNodes; i++) {
                int val_i = (int) vals[i+numNodes];
                if(val_i) {
                    insertion_nodes.insert(ckt.nodes[i]);
                }
            }

            // add nodes.
            for(auto it=insertion_nodes.begin(); it != insertion_nodes.end(); it++) {
                node_t* ni = *it;
                node_t* ki = ckt.create_key_input();
                int val = rand() % 2;
                key_values.push_back(val);
                ckt.insert_key_gate(ni, ki, val);
            }
        } else {
            assert(false);
        }

    }

    void ilp_encoder_t::encode2()
    {
        IloEnv env;
        IloNumVarArray vars(env);

        int numNodes = ckt.num_nodes();
        int totalVars = 2*numNodes;
        // first create the variables which count the number of keys
        // in the fanin cone.
        for(int i=0; i != numNodes; i++) {
            std::string name = "n" + boost::lexical_cast<std::string>(i);
            vars.add(IloNumVar(env, 0, IloInfinity, name.c_str()));
        }
        // now a boolean variable that counts whether a key was inserted
        // at the output of this node.
        for(int i=0; i != numNodes; i++) {
            std::string name = "b" + boost::lexical_cast<std::string>(i);
            vars.add(IloNumVar(env, 0, 1, name.c_str()));
        }

        int cnstPos = 0;

        // add the constraints that count the number of keys in the fanin.
        IloRangeArray cnst(env);
        for(int i=0; i != numNodes; i++) {
            node_t* ni = ckt.nodes[i];
            assert(ni->get_index() == i);

            std::string name = "c" + boost::lexical_cast<std::string>(i);
            cnst.add(IloRange(env, 0, 0, name.c_str()));
            // i - j1 - j2 ... -jN - bi = 0
            for(unsigned j=0; j != ni->num_inputs(); j++) {
                node_t* nj = ni->inputs[j];
                int jj = nj->get_index();
                // ji
                cnst[cnstPos].setLinearCoef(vars[jj], -1);
            }
            // i
            cnst[cnstPos].setLinearCoef(vars[i], 1);
            // bi
            cnst[cnstPos].setLinearCoef(vars[i+numNodes], -1);
            cnstPos += 1;
        }

        // b1 + b2 + ... + bi <= target_keys.
        cnst.add(IloRange(env, 0, target_keys, "cKeyTarget"));
        for(int i=0; i != numNodes; i++) {
            int vi = i+numNodes;
            cnst[cnstPos].setLinearCoef(vars[vi], 1);
        }
        cnstPos += 1;

        // find the "biggest" output.
        std::vector<bool> fanin_flags(ckt.num_nodes());
        int max_sz = 0;
        int max_index = -1;
        for(unsigned i=0; i != ckt.num_outputs(); i++) {
            node_t* oi = ckt.outputs[i];
            ckt.compute_transitive_fanin(oi, fanin_flags, true);
            int sz=0;
            for(auto it=fanin_flags.begin(); it != fanin_flags.end(); it++) {
                if(*it) sz += 1;
            }
            if(sz > max_sz) {
                max_sz = sz;
                max_index = i;
            }
        }
        assert(max_index != -1);
        node_t* omax = ckt.outputs[max_index];
        int max_node_index = omax->get_index();

        std::cout << "biggest output : " << omax->name << std::endl;
        std::cout << "output size    : " << max_sz << std::endl;

        // maximize the keys which affect this output.
        IloObjective obj = IloMaximize(env);
        IloNumArray coeffs(env, totalVars);
        coeffs[max_node_index] = 1;
        for(unsigned i=0; i != ckt.num_outputs(); i++) {
            if((int)i != max_index) {
                node_t* oi = ckt.outputs[i];
                int idx = oi->get_index();
                coeffs[idx] = -1;
            }
        }
        obj.setLinearCoefs(vars, coeffs);

        IloModel model(env);
        model.add(obj);
        model.add(cnst);
        IloCplex cplex(model);
        //cplex.setOut(env.getNullStream());
        //cplex.setParam(IloCplex::TiLim, 1);
        //cplex.setParam(IloCplex::EpGap, 0.25);
        
        cplex.exportModel("encmodel.lp");
        if(cplex.solve()) {
            std::cout << "Status    : " << cplex.getStatus() << std::endl;
            std::cout << "Value     : " << cplex.getObjValue() << std::endl;

            IloNumArray vals(env);
            cplex.getValues(vals, vars);

            // extract solution.
            nodeset_t insertion_nodes;
            for(int i=0; i != numNodes; i++) {
                int val_i = (int) vals[i+numNodes];
                if(val_i) {
                    insertion_nodes.insert(ckt.nodes[i]);
                }
            }

            // add nodes.
            for(auto it=insertion_nodes.begin(); it != insertion_nodes.end(); it++) {
                node_t* ni = *it;
                node_t* ki = ckt.create_key_input();
                int val = rand() % 2;
                key_values.push_back(val);
                ckt.insert_key_gate(ni, ki, val);
            }
        } else {
            assert(false);
        }

    }

    void ilp_encoder_t::encode1()
    {
        IloEnv env;
        IloNumVarArray vars(env);

        int numNodes = ckt.num_nodes();
        // first create the variables which count the number of keys
        // in the fanin cone.
        for(int i=0; i != numNodes; i++) {
            std::string name = "n" + boost::lexical_cast<std::string>(i);
            vars.add(IloNumVar(env, 0, IloInfinity, name.c_str()));
        }
        // now a boolean variable that counts whether a key was inverted
        // at the output of this node.
        for(int i=0; i != numNodes; i++) {
            std::string name = "b" + boost::lexical_cast<std::string>(i);
            vars.add(IloNumVar(env, 0, 1, name.c_str()));
        }
        int maxVar = 2*numNodes;
        int totalVars = maxVar + 1;
        std::string name = "maxKeys";
        vars.add(IloNumVar(env, 0, IloInfinity, name.c_str()));

        int cnstPos = 0;

        // add the constraints that count the number of keys in the fanin.
        IloRangeArray cnst(env);
        for(int i=0; i != numNodes; i++) {
            node_t* ni = ckt.nodes[i];
            assert(ni->get_index() == i);

            std::string name = "c" + boost::lexical_cast<std::string>(i);
            cnst.add(IloRange(env, 0, 0, name.c_str()));
            // i - j1 - j2 ... -jN - bi = 0
            for(unsigned j=0; j != ni->num_inputs(); j++) {
                node_t* nj = ni->inputs[j];
                int jj = nj->get_index();
                // ji
                cnst[cnstPos].setLinearCoef(vars[jj], -1);
            }
            // i
            cnst[cnstPos].setLinearCoef(vars[i], 1);
            // bi
            cnst[cnstPos].setLinearCoef(vars[i+numNodes], -1);
            cnstPos += 1;
        }

        for(unsigned i=0; i != ckt.num_outputs(); i++) {
            node_t* oi = ckt.outputs[i];
            int idx = oi->get_index();
            std::string name = "co" + boost::lexical_cast<std::string>(idx);
            // i >= maxVar
            // i.e., i - maxVar >= 0
            cnst.add(IloRange(env, 0, IloInfinity, name.c_str()));
            cnst[cnstPos].setLinearCoef(vars[idx], 1);
            cnst[cnstPos].setLinearCoef(vars[maxVar], -1);
            cnstPos += 1;
        }

        // b1 + b2 + ... + bi <= target_keys.
        cnst.add(IloRange(env, 0, target_keys, "cKeyTarget"));
        for(int i=0; i != numNodes; i++) {
            int vi = i+numNodes;
            cnst[cnstPos].setLinearCoef(vars[vi], 1);
        }
        cnstPos += 1;


        // objective maximize maxVars;
        IloObjective obj = IloMaximize(env);
        IloNumArray coeffs(env, totalVars);
        coeffs[maxVar] = 1;
        obj.setLinearCoefs(vars, coeffs);

        IloModel model(env);
        model.add(obj);
        model.add(cnst);
        IloCplex cplex(model);
        //cplex.setOut(env.getNullStream());
        //cplex.setParam(IloCplex::TiLim, 1);
        //cplex.setParam(IloCplex::EpGap, 0.25);
        
        cplex.exportModel("encmodel.lp");
        if(cplex.solve()) {
            IloNumArray vals(env);
            cplex.getValues(vals, vars);

            // extract solution.
            int maxKeyVal = (int) vals[maxVar];
            std::cout << "max keys: " << maxKeyVal << std::endl;
            nodeset_t insertion_nodes;
            for(int i=0; i != numNodes; i++) {
                int val_i = (int) vals[i+numNodes];
                if(val_i) {
                    insertion_nodes.insert(ckt.nodes[i]);
                }
            }

            // add nodes.
            for(auto it=insertion_nodes.begin(); it != insertion_nodes.end(); it++) {
                node_t* ni = *it;
                node_t* ki = ckt.create_key_input();
                int val = rand() % 2;
                key_values.push_back(val);
                ckt.insert_key_gate(ni, ki, val);
            }
        }

    }

    void ilp_encoder_t::write(std::ostream& out)
    {
        out << "# key=";
        std::copy(key_values.begin(), key_values.end(), std::ostream_iterator<int>(out, ""));
        out << std::endl << ckt << std::endl;
    }
}
