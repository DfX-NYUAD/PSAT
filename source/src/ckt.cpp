#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <math.h>
#include "ckt.h"
#include "dbl.h"
#include "ternarysat.h"
#include "kcut.h"
// JOHANN
#include <fstream>

namespace ckt_n {
    ckt_t::ckt_t(const ckt_t& ckt, node_t* n_cut,
                 const std::vector<bool>& fanin, 
                 node_map_t& nm_fwd, node_map_t& nm_rev)
    {
        assert(nm_fwd.size() == 0);
        assert(nm_rev.size() == 0);
        for(unsigned i=0; i != fanin.size(); i++) {
            if(fanin[i]) {
                node_t* n = ckt.nodes[i];
                node_t* n_c = NULL;
                if(n->is_input()) {
                    n_c = node_t::create_input(n->name);
                    if(n->is_keyinput()) {
                        add_key_input(n_c);
                    } else {
                        add_ckt_input(n_c);
                    }
                } else if(n == n_cut) {
                    assert(n->is_gate());
                    n_c = node_t::create_input(n->name);
                    add_ckt_input(n_c);
                } else {
                    assert(n->is_gate());
                    n_c = node_t::create_gate(n->name, n->func);
                    add_gate(n_c);
                }
                assert(n_c != NULL);
                nm_fwd[n] = n_c;
                nm_rev[n_c] = n;
                n_c->output = n->output;
                if(n_c->output) {
                    outputs.push_back(n_c);
                }
            }
        }
        
        for(unsigned i=0; i != num_gates(); i++) {
            node_t* n_c = gates[i];
            auto p1 = nm_rev.find(n_c);
            assert(p1 != nm_rev.end());
            node_t* n = p1->second;

            for(unsigned j=0; j != n->num_inputs(); j++) {
                node_t* n_i = n->inputs[j];
                auto p2 = nm_fwd.find(n_i);
                assert(p2 != nm_fwd.end());
                node_t* n_i_c = p2->second;
                n_c->add_input(n_i_c);
            }
        }
        _init_fanouts();
        _init_indices();
        topo_sort();
    }

    struct index_comparer_t {
        std::map<node_t*, int>& index_map;
        index_comparer_t(std::map<node_t*, int>& m) : index_map(m) {}

        bool operator()(node_t* n1, node_t* n2) const {
            auto p1 = index_map.find(n1);
            auto p2 = index_map.find(n2);
            if(p1 == index_map.end()) {
                std::cout << "can't find node: " << n1->name << std::endl;
            }
            if(p2 == index_map.end()) {
                std::cout << "can't find node: " << n2->name << std::endl;
            }
            assert(p1 != index_map.end());
            assert(p2 != index_map.end());
            return p1->second < p2->second;
        }
    };

    // create a slice of the circuit that contains
    // only these specified outputs.
    ckt_t::ckt_t(const ckt_t& ckt, nodelist_t& outputList,
                 node_map_t& nm_fwd, node_map_t& nm_rev) 
    {
        std::vector<bool> fanin_cone(ckt.num_nodes(), false);
        for(unsigned i=0; i != outputList.size(); i++) {
            node_t* n = outputList[i];
            ckt.compute_transitive_fanin(n, fanin_cone, false);
        }

        assert(nm_fwd.size() == 0);
        assert(nm_rev.size() == 0);
        for(unsigned i=0; i != fanin_cone.size(); i++) {
            if(fanin_cone[i]) {
                node_t* n = ckt.nodes[i];
                node_t* n_c = NULL;
                if(n->is_input()) {
                    n_c = node_t::create_input(n->name);
                    if(n->is_keyinput()) {
                        add_key_input(n_c);
                    } else {
                        add_ckt_input(n_c);
                    }
                } else {
                    assert(n->is_gate());
                    n_c = node_t::create_gate(n->name, n->func);
                    add_gate(n_c);
                }
                assert(n_c != NULL);
                nm_fwd[n] = n_c;
                nm_rev[n_c] = n;
                n_c->output = n->output;
                if(n_c->output) {
                    outputs.push_back(n_c);
                }
            }
        }
        for(unsigned i=0; i != num_gates(); i++) {
            node_t* n_c = gates[i];
            auto p1 = nm_rev.find(n_c);
            assert(p1 != nm_rev.end());
            node_t* n = p1->second;

            for(unsigned j=0; j != n->num_inputs(); j++) {
                node_t* n_i = n->inputs[j];
                auto p2 = nm_fwd.find(n_i);
                assert(p2 != nm_fwd.end());
                node_t* n_i_c = p2->second;
                n_c->add_input(n_i_c);
            }
        }
        _init_fanouts();
        _init_indices();
        topo_sort();

        // we have to fix up the input and output orders to match the original circuit

        // ckt inputs
        std::map<node_t*, int> ckt_input_order;
        for(unsigned i=0; i != ckt.num_ckt_inputs(); i++) {
            node_t* inp_i = ckt.ckt_inputs[i];
            auto pos = nm_fwd.find(inp_i);
            if(pos != nm_fwd.end()) {
                node_t* inp_new_i = pos->second;
                ckt_input_order[inp_new_i] = i;
            }
        }
        index_comparer_t cicmp(ckt_input_order);
        std::sort(ckt_inputs.begin(), ckt_inputs.end(), cicmp);

        // key inputs
        std::map<node_t*, int> key_input_order;
        for(unsigned i=0; i != ckt.num_key_inputs(); i++) {
            node_t* inp_i = ckt.key_inputs[i];
            auto pos = nm_fwd.find(inp_i);
            if(pos != nm_fwd.end()) {
                node_t* inp_new_i = pos->second;
                key_input_order[inp_new_i] = i;
            }
        }
        index_comparer_t kicmp(key_input_order);
        std::sort(key_inputs.begin(), key_inputs.end(), kicmp);

        // outputs
        std::map<node_t*, int> output_order;
        for(unsigned i=0; i != ckt.num_outputs(); i++) {
            node_t* out_i = ckt.outputs[i];
            auto pos = nm_fwd.find(out_i);
            // std::cout << "out: " << out_i->name << std::endl;
            if(pos != nm_fwd.end()) {
                node_t* out_new_i = pos->second;
                output_order[out_new_i] = i;
                // std::cout << "newout: " << out_new_i->name << std::endl;
            }
        }
        index_comparer_t ocmp(output_order);
        std::sort(outputs.begin(), outputs.end(), ocmp);
    }

    ckt_t::ckt_t(const ckt_t& c1, const ckt_t& c2)
    {
        if(c1.num_ckt_inputs() != c2.num_ckt_inputs()) {
            std::cerr << "Number of ckt inputs must be the same." << std::endl;
            exit(1);
        }
        if(c1.num_outputs() != c2.num_outputs()) {
            std::cerr << "Number of outputs must be the same." << std::endl;
            exit(1);
        }

        for(unsigned i=0; i != c1.num_ckt_inputs(); i++) {
            if(c1.ckt_inputs[i]->name != c2.ckt_inputs[i]->name) {
                std::cerr << "Input " << i << " named differently: "
                          << c1.ckt_inputs[i]->name << "/"
                          << c2.ckt_inputs[i]->name << "." << std::endl;
                exit(1);
            }
        }

        for(unsigned i=0; i != c1.num_outputs(); i++) {
            const std::string& n1 = c1.outputs[i]->name;
            const std::string& n2 = c2.outputs[i]->name;

            if((n1 != n2) && 
               (n1 + "$enc" != n2) &&
               (n1 != n2 + "$enc")) 
            {
                std::cout << "Output " << i << " differing names: "
                          << n1 << "/" << n2 << "." << std::endl;
            }
        }
        
        node_map_t nm;
        _create_ckt_inputs(nm, c1.ckt_inputs, c2.ckt_inputs);
        _create_key_inputs(nm, c1.key_inputs, "_A");
        _create_key_inputs(nm, c2.key_inputs, "_B");
        _create_gates(nm, c1.gates, "_A");
        _create_gates(nm, c2.gates, "_B");
        _wire_gate_inputs(nm, c1.gates);
        _wire_gate_inputs(nm, c2.gates);
        _compare_outputs(nm, c1.outputs, c2.outputs);
        _init_indices();
        topo_sort();
    }

    void ckt_t::_create_ckt_inputs(node_map_t& node_map, const nodelist_t& i1, const nodelist_t& i2)
    {
        assert(i1.size() == i2.size());
        for(unsigned i=0; i != i1.size(); i++) {
            node_t* n1 = i1[i];
            node_t* n2 = i2[i];
            assert(n1->name == n2->name);
            node_t* n = node_t::create_input(n1->name);
            node_map[n1] = n;
            node_map[n2] = n;
            add_ckt_input(n);
        }
    }

    void ckt_t::_create_key_inputs(node_map_t& node_map, const nodelist_t& i1, const char* suffix)
    {
        for(unsigned i=0; i != i1.size(); i++) {
            node_t* n_in = i1[i];
            node_t* n_out = node_t::create_input(n_in->name + suffix);
            node_map[n_in] = n_out;
            add_key_input(n_out);
        }
    }

    void ckt_t::_create_gates(node_map_t& node_map, const nodelist_t& gs, const char* suffix)
    {
        for(unsigned i=0; i != gs.size(); i++) {
            node_t* n_in = gs[i];
            node_t* n_out = node_t::create_gate(n_in->name + suffix, n_in->func);
            node_map[n_in] = n_out;
            add_gate(n_out);
        }
    }

    void ckt_t::_wire_gate_inputs(const node_map_t& nm, const nodelist_t& gs)
    {
        for(unsigned i=0; i != gs.size(); i++) {
            node_t* n_in = gs[i];
            node_map_t::const_iterator p_g = nm.find(n_in);
            assert(p_g != nm.end());
            node_t* n_out = p_g->second;

            for(unsigned j=0; j != n_in->num_inputs(); j++) {
                node_t* inp_j = n_in->inputs[j];
                node_map_t::const_iterator p_in = nm.find(inp_j);
                assert(p_in != nm.end());
                node_t* inp_rewritten = p_in->second;
                n_out->add_input(inp_rewritten);
            }
        }
    }

    void ckt_t::_compare_outputs(node_map_t& nm, const nodelist_t& o1, const nodelist_t& o2)
    {
        assert(o1.size() == o2.size());

        std::string name("__final_cmp_out__");
        std::string func("or");
        node_t* g_out = node_t::create_gate(name, func);
        add_gate(g_out);

        for(unsigned i=0; i != o1.size(); i++) {
            node_t* nin_1 = o1[i];
            node_t* nin_2 = o2[i];

            assert(nm.find(nin_1) != nm.end());
            assert(nm.find(nin_2) != nm.end());

            node_t* n1 = nm[nin_1];
            node_t* n2 = nm[nin_2];

            std::string name = n1->name + "__cmp__" + n2->name;
            std::string func("xor");
            node_t* g_xor = node_t::create_gate(name, func);
            g_xor->add_input(n1);
            g_xor->add_input(n2);
            add_gate(g_xor);
            g_out->add_input(g_xor);
        }
        outputs.push_back(g_out);
    }

    bool ckt_t::check_equiv(const char* keys)
    {
        using namespace sat_n;

        assert(num_outputs() == 1);
        assert(outputs[0]->name == "__final_cmp_out__");

        sat_n::Solver S;
        index2lit_map_t map;

        init_solver(S, map);
        assert(strlen(keys) == num_key_inputs());

        for(unsigned i=0; i != num_key_inputs(); i++) {
            Lit li = getLit(map, key_inputs[i]);
            const char* nptr = key_inputs[i]->name.c_str();
            const char* eptr = nptr + strlen(nptr) - 2;
            bool akey = strcmp(eptr, "_A") == 0;
            bool bkey = strcmp(eptr, "_B") == 0;
            assert(!akey && bkey);
            if(bkey) {
                assert(i >= 0 && i < num_key_inputs());
                if(keys[i] == '0') {
                    S.addClause(~li);
                } else if(keys[i] == '1') {
                    S.addClause(li);
                } else {
                    std::cout << "not a valid key bit: '" << keys[i] << "'" << std::endl;
                    exit(1);
                }
            }
        }
        Lit lout = getLit(map, outputs[0]);
        S.addClause(lout);

        bool eq = !S.solve();
        return eq;
    }

    ckt_t::ckt_t(const ckt_t& c1, node_map_t& nm)
    {
        for(unsigned i=0; i != c1.num_inputs(); i++) {
            node_t* ni = c1.inputs[i];
            node_t* new_inp = node_t::create_input(ni->name);
            nm[ni] = new_inp;
            inputs.push_back(new_inp);
        }
        for(unsigned i=0; i != c1.num_gates(); i++) {
            node_t* gi = c1.gates[i];
            node_t* new_gate = node_t::create_gate(gi->name, gi->func);
            nm[gi] = new_gate;
            gates.push_back(new_gate);
        }
        _copy_array(nm, c1.ckt_inputs, ckt_inputs);
        _copy_array(nm, c1.key_inputs, key_inputs);
        _copy_array(nm, c1.outputs, outputs);
        _copy_array(nm, c1.nodes, nodes);
    }

    void ckt_t::_copy_array(const node_map_t& nm, const nodelist_t& in_array, nodelist_t& out_array)
    {
        for(unsigned i=0; i != in_array.size(); i++) {
            node_t* n_in = in_array[i];

            node_map_t::const_iterator pos = nm.find(n_in);
            assert(pos != nm.end());

            node_t* n_out = pos->second;
            out_array.push_back(n_out);
        }
    }

    ckt_t::ckt_t(ast_n::statements_t& stms)
    {
        map_t map;
        _init_map(map, stms);
        _init_inputs(map, stms);
        _init_outputs(map, stms);
        _init_fanouts();
        _init_indices();
        topo_sort();

        assert(inputs.size() + gates.size() == nodes.size());
        assert(outputs.size() <= nodes.size());
    }

    ckt_t::ckt_t(nodepair_list_t& pair_map)
    {
        for(unsigned i=0; i != pair_map.size(); i++) {
            node_t* nA = pair_map[i].first;
            node_t* nB = pair_map[i].second;
            if(nA == nB) {
                assert(node_t::INPUT == nA->type);
                inputs.push_back(nA);
                nodes.push_back(nA);
                ckt_inputs.push_back(nA);
            } else {
                assert(nA->type == nB->type);
                if(node_t::INPUT == nA->type) {
                    inputs.push_back(nA);
                    inputs.push_back(nB);

                    assert(nA->is_keyinput());
                    assert(nB->is_keyinput());
                    key_inputs.push_back(nA);
                    key_inputs.push_back(nB);
                } else {
                    assert(node_t::GATE == nA->type);
                    gates.push_back(nA);
                    gates.push_back(nB);
                }
                nodes.push_back(nA);
                nodes.push_back(nB);
            }
        }
    }

    ckt_t::~ckt_t()
    {
        for(unsigned i=0; i != nodes.size(); i++) {
            delete nodes[i];
        }
        nodes.clear();
        inputs.clear();
        gates.clear();
        outputs.clear();
    }

    void ckt_t::init_input_map(index2lit_map_t& mappings, std::vector<bool>& inputs) const
    {
        std::fill(inputs.begin(), inputs.end(), false);
        for(nodelist_t::const_iterator it = nodes.begin(); it != nodes.end(); it++) {
            using namespace sat_n;

            node_t* n = *it;
            int index = n->get_index();
            Lit l_i = mappings[index];
            Var v_i = var(l_i);
            assert((int)v_i < (int) inputs.size());
            inputs[v_i] = n->is_input();
        }
    }

    void ckt_t::init_keyinput_map(index2lit_map_t& mappings, std::vector<bool>& inputs) const
    {
        std::fill(inputs.begin(), inputs.end(), false);
        for(nodelist_t::const_iterator it = nodes.begin(); it != nodes.end(); it++) {
            using namespace sat_n;

            node_t* n = *it;
            int index = n->get_index();
            Lit l_i = mappings[index];
            Var v_i = var(l_i);
            assert((int)v_i < (int) inputs.size());
            inputs[v_i] = n->is_keyinput();
        }
    }

    void ckt_t::init_node_map(map_t& map)
    {
        for(unsigned i=0; i != nodes.size(); i++) {
            node_t* ni = nodes[i];
            assert(map.find(ni->name) == map.end());
            map[ni->name] = ni;
        }
    }

    void ckt_t::_init_map(map_t& map, ast_n::statements_t& stms)
    {
        using namespace ast_n;

        for(unsigned i=0; i != stms.size(); i++) {
            statement_t& stm = stms[i];
            if(stm.type == statement_t::INPUT) {
                node_t* inp = node_t::create_input(stm.input->name);
                map[inp->name] = inp;
                inputs.push_back(inp);
                if(inp->is_keyinput()) {
                    key_inputs.push_back(inp);
                } else {
                    ckt_inputs.push_back(inp);
                }
                nodes.push_back(inp);
            } else if(stm.type == statement_t::GATE) {
                node_t* g = node_t::create_gate(
                    stm.gate->output, 
                    stm.gate->function
                );
                map[g->name] = g;
                gates.push_back(g);
                nodes.push_back(g);
            }
        }
    }

    void ckt_t::_init_inputs(map_t& map, ast_n::statements_t& stms)
    {
        using namespace ast_n;

        for(unsigned i=0; i != stms.size(); i++) {
            statement_t& stm = stms[i];
            if(stm.type == statement_t::GATE) {
                gate_decl_t* g = stms[i].gate;
                map_t::iterator pos = map.find(g->output);
                assert(map.end() != pos);
                node_t* node = pos->second;

                for(unsigned i=0; i != g->inputs.size(); i++) {
                    std::string& name = g->inputs[i];
                    map_t::iterator pos = map.find(name);
                    if(map.end() == pos) {
                        std::cerr << "Unknown input \'" << name 
                                  << "\' in the following declaration: "
                                  << std::endl 
                                  << stm << std::endl; 
                        exit(1);
                    }
                    node_t* inp = pos->second;
                    node->add_input(inp);
                }
            }
        }
    }

    void ckt_t::_init_outputs(map_t& map, ast_n::statements_t& stms)
    {
        using namespace ast_n;

        for(unsigned i=0; i != stms.size(); i++) {
            statement_t& stm = stms[i];
            if(stm.type == statement_t::OUTPUT) {
                output_decl_t* o = stms[i].output;
                map_t::iterator pos = map.find(o->name);
                if(map.end() == pos) {
                    std::cerr << "Unknown output: " << o->name << std::endl;
                    exit(1);
                }
                pos->second->output = true;
                outputs.push_back(pos->second);
            }
        }
    }

    void ckt_t::_init_fanouts()
    {
        for(unsigned i=0; i != nodes.size(); i++) {
            nodes[i]->fanouts.clear();
        }

        for(unsigned i=0; i != gates.size(); i++) {
            for(unsigned j=0; j != gates[i]->inputs.size(); j++) {
                node_t* inp_j = gates[i]->inputs[j];
                inp_j->add_fanout(gates[i]);
            }
        }
    }
    
    void ckt_t::_init_indices()
    {
        for(unsigned i=0; i != nodes.size(); i++) {
            nodes[i]->index = i;
        }
    }

    sat_n::Lit ckt_t::create_slice_diff(
        sat_n::Solver& S, 
        node_t* out,
        node_t* diff,
        const nodeset_t& internals,
        const nodeset_t& inputs,
        node2var_map_t& n2v1,
        node2var_map_t& n2v2
    )
    {
        using namespace sat_n;
        for(auto it=internals.begin(); it != internals.end(); it++) {
            node_t* ni = *it;
            assert(inputs.find(ni) == inputs.end());
            n2v1[ni] = S.newVar();
            n2v2[ni] = S.newVar();
        }
        for(auto it = inputs.begin(); it != inputs.end(); it++) {
            node_t* ni = *it;
            assert(internals.find(ni) == internals.end());
            if(ni != diff) {
                n2v1[ni] = n2v2[ni] = S.newVar();
            } else {
                n2v1[ni] = S.newVar();
                n2v2[ni] = S.newVar();
            }
        }

        assert(n2v1.find(out) != n2v1.end());
        assert(n2v2.find(out) != n2v2.end());

        Lit o1 = mkLit(n2v1[out]);
        Lit o2 = mkLit(n2v2[out]);

        Lit y = mkLit(S.newVar());

        S.addClause(o1, o2, ~y);
        S.addClause(~o1, ~o2, ~y);
        S.addClause(o1, ~o2, y);
        S.addClause(~o1, o2, y);

        return y;
    }

    void ckt_t::init_solver_ignoring_nodes(
        sat_n::Solver& S, index2lit_map_t& mappings, nodeset_t& nodes2ignore)
    {
        using namespace sat_n;

        node2var_map_t node2vars;
        for(unsigned i=0; i != nodes.size(); i++) {
            node2vars[nodes[i]] = Var(i+1);
        }
        while(S.nVars() <= (int) (nodes.size()+1)) {
            S.newVar();
        }

        for(unsigned i=0; i != gates.size(); i++) {
            node_t* n = gates[i];
            if(nodes2ignore.find(n) == nodes2ignore.end()) {
                const clause_provider_i<sat_n::Solver>* prov = 
                    n->get_solver_provider();
                prov->add_clauses(S, node2vars, n->inputs, n);
            }
        }

        mappings.resize(nodes.size());
        for(unsigned i=0; i != nodes.size(); i++) {
            node_t* n = nodes[i];
            int idx = n->get_index();
            assert(idx >= 0 && idx < (int) nodes.size());
            mappings[idx] = mkLit(node2vars[n]);
        }
    }

    void ckt_t::init_solver(
        sat_n::Solver& S, index2lit_map_t& mappings)
    {
        using namespace sat_n;

        node2var_map_t node2vars;
        for(unsigned i=0; i != nodes.size(); i++) {
            node2vars[nodes[i]] = Var(i+1);
        }
        while(S.nVars() <= (int) (nodes.size()+1)) {
            S.newVar();
        }

        for(unsigned i=0; i != gates.size(); i++) {
            node_t* n = gates[i];
            const clause_provider_i<sat_n::Solver>* prov = 
                n->get_solver_provider();
            prov->add_clauses(S, node2vars, n->inputs, n);
        }

        mappings.resize(nodes.size());
        for(unsigned i=0; i != nodes.size(); i++) {
            node_t* n = nodes[i];
            int idx = n->get_index();
            assert(idx >= 0 && idx < (int) nodes.size());
            mappings[idx] = mkLit(node2vars[n]);
        }
    }

    void ckt_t::init_solver(
        b2qbf::ISolver& S, 
        const nodeset_t& eq,
        const nodeset_t& uq,
        node_t* output,
        index2lit_map_t& mappings
    )
    {
        using namespace sat_n;

        int nVars = nodes.size()+1;
        node2var_map_t node2vars;
        for(unsigned i=0; i != nodes.size(); i++) {
            node2vars[nodes[i]] = Var(i+1);
        }

        std::vector<Var> eqVars, uqVars, eq2Vars;
        for(nodeset_t::const_iterator it = eq.begin();
                                      it != eq.end();
                                      it++)
        {
            node_t* n = *it;
            assert(n->is_input());
            eqVars.push_back(node2vars[n]);
        }
        for(nodeset_t::const_iterator it = uq.begin();
                                      it != uq.end();
                                      it++)
        {
            node_t* n = *it;
            assert(n->is_input());
            uqVars.push_back(node2vars[n]);
        }

        for(unsigned i=0; i != nodes.size(); i++) {
            node_t* n = nodes[i];
            if(eq.find(n) == eq.end() &&
               uq.find(n) == uq.end())
            {
                eq2Vars.push_back(node2vars[n]);
            }
        }
        assert(eqVars.size() == eq.size());
        assert(uqVars.size() == uq.size());
        assert((eqVars.size() + uqVars.size() + eq2Vars.size()) == nodes.size());
        S.initialize(nVars, eqVars, uqVars, eq2Vars, mkLit(node2vars[output]));

        for(unsigned i=0; i != gates.size(); i++) {
            node_t* n = gates[i];
            const clause_provider_i<b2qbf::ISolver>* prov = 
                n->get_qbf_provider();
            prov->add_clauses(S, node2vars, n->inputs, n);
        }

        mappings.resize(nodes.size());
        for(unsigned i=0; i != nodes.size(); i++) {
            node_t* n = nodes[i];
            int idx = n->get_index();
            assert(idx >= 0 && idx < (int) nodes.size());
            mappings[idx] = mkLit(node2vars[n]);
        }
    }

    sat_n::Lit ckt_t::getLit(index2lit_map_t& mappings, node_t* n)
    {
        assert(n->get_index() >= 0);
        assert(n->get_index() < (int) mappings.size());
        return mappings[n->get_index()];
    }

    void ckt_t::init_ternary_solver(
        sat_n::Solver& S, index2lit_map_t& mappings)
    {
        using namespace sat_n;

        node2var_map_t node2vars;
        for(unsigned i=0; i != nodes.size(); i++) {
            node2vars[nodes[i]] = Var(2*i+1);
        }
        while(S.nVars() <= (int) (2*nodes.size()+1)) {
            S.newVar();
        }

        for(unsigned i=0; i != gates.size(); i++) {
            node_t* n = gates[i];
            const clause_provider_i<sat_n::Solver>* prov = 
                n->get_ternary_solver_provider();
            prov->add_clauses(S, node2vars, n->inputs, n);
        }

        mappings.resize(nodes.size());
        for(unsigned i=0; i != nodes.size(); i++) {
            node_t* n = nodes[i];
            int idx = n->get_index();
            assert(idx >= 0 && idx < (int) nodes.size());
            mappings[idx] = mkLit(node2vars[n]);
        }
    }

    sat_n::Lit ckt_t::getTLit0(index2lit_map_t& mappings, node_t* n)
    {
        assert(n->get_index() >= 0);
        assert(n->get_index() < (int) mappings.size());
        return mappings[n->get_index()];
    }

    sat_n::Lit ckt_t::getTLit1(index2lit_map_t& mappings, node_t* n)
    {
        using namespace sat_n;
        assert(n->get_index() >= 0);
        assert(n->get_index() < (int) mappings.size());
        return mkLit(var(mappings[n->get_index()]) + 1);
    }

    void ckt_t::init_solver(AllSAT::AllSATCkt& S, index2lit_map_t& mappings)
    {
        using namespace sat_n;

        node2var_map_t node2vars;
        for(unsigned i=0; i != nodes.size(); i++) {
            node2vars[nodes[i]] = Var(i+1);
        }
        while(S.nVars() <= (int) (nodes.size()+1)) {
            S.newVar();
        }

        for(unsigned i=0; i != gates.size(); i++) {
            node_t* n = gates[i];
            const clause_provider_i<AllSAT::AllSATCkt>* prov = 
                n->get_allsatckt_provider();
            prov->add_clauses(S, node2vars, n->inputs, n);
        }

        mappings.resize(nodes.size());
        for(unsigned i=0; i != nodes.size(); i++) {
            node_t* n = nodes[i];
            int idx = n->get_index();
            assert(idx >= 0 && idx < (int) nodes.size());
            mappings[idx] = mkLit(node2vars[n]);
        }
    }

    void ckt_t::init_solver(AllSAT::ClauseList& S, index2lit_map_t& mappings)
    {
        using namespace sat_n;

        node2var_map_t node2vars;
        for(unsigned i=0; i != nodes.size(); i++) {
            node2vars[nodes[i]] = Var(i+1);
        }
        while(S.nVars() <= (int) (nodes.size()+1)) {
            S.newVar();
        }

        for(unsigned i=0; i != gates.size(); i++) {
            node_t* n = gates[i];
            const clause_provider_i<AllSAT::ClauseList>* prov = 
                n->get_clauselist_provider();
            prov->add_clauses(S, node2vars, n->inputs, n);
        }

        mappings.resize(nodes.size());
        for(unsigned i=0; i != nodes.size(); i++) {
            node_t* n = nodes[i];
            int idx = n->get_index();
            assert(idx >= 0 && idx < (int) nodes.size());
            mappings[idx] = mkLit(node2vars[n]);
        }
    }

    void ckt_t::init_solver(AllSAT::AllSATCkt& S1, 
        sat_n::Solver& S2, index2lit_map_t& mappings)
    {
        using namespace sat_n;

        node2var_map_t node2vars;
        for(unsigned i=0; i != nodes.size(); i++) {
            node2vars[nodes[i]] = Var(i+1);
        }
        while(S1.nVars() <= (int) (nodes.size()+1)) { S1.newVar(); }
        while(S2.nVars() <= (int) (nodes.size()+1)) { S2.newVar(); }

        for(unsigned i=0; i != gates.size(); i++) {
            node_t* n = gates[i];

            // add clauses to the all sat solver.
            const clause_provider_i<AllSAT::AllSATCkt>* prov1 = 
                n->get_allsatckt_provider();
            prov1->add_clauses(S1, node2vars, n->inputs, n);

            // add clauses to the minisat solver.
            const clause_provider_i<sat_n::Solver>* prov2 = 
                n->get_solver_provider();
            prov2->add_clauses(S2, node2vars, n->inputs, n);
        }

        mappings.resize(nodes.size());
        for(unsigned i=0; i != nodes.size(); i++) {
            node_t* n = nodes[i];
            int idx = n->get_index();
            assert(idx >= 0 && idx < (int) nodes.size());
            mappings[idx] = mkLit(node2vars[n]);
        }
    }

    void ckt_t::init_solver(sat_n::Solver& S1, 
        AllSAT::ClauseList& S2,
        index2lit_map_t& mappings,
        bool skipNewlyAdded)
    {
        using namespace sat_n;

        node2var_map_t node2vars;
        for(unsigned i=0; i != nodes.size(); i++) {
            node2vars[nodes[i]] = Var(i+1);
        }
        while(S1.nVars() <= (int) (nodes.size()+1)) { S1.newVar(); }
        while(S2.nVars() <= (int) (nodes.size()+1)) { S2.newVar(); }

        for(unsigned i=0; i != gates.size(); i++) {
            node_t* n = gates[i];

            // add clauses to the all sat solver.
            const clause_provider_i<sat_n::Solver>* prov1 = 
                n->get_solver_provider();
            prov1->add_clauses(S1, node2vars, n->inputs, n);

            if(!n->newly_added || !skipNewlyAdded) {
                // add clauses to the clause list.
                const clause_provider_i<AllSAT::ClauseList>* prov2 = 
                    n->get_clauselist_provider();
                prov2->add_clauses(S2, node2vars, n->inputs, n);
            }
        }

        mappings.resize(nodes.size());
        for(unsigned i=0; i != nodes.size(); i++) {
            node_t* n = nodes[i];
            int idx = n->get_index();
            assert(idx >= 0 && idx < (int) nodes.size());
            mappings[idx] = mkLit(node2vars[n]);
        }
    }

    std::ostream& operator<<(std::ostream& out, const ckt_t& ckt)
    {
        for(unsigned i=0; i != ckt.inputs.size(); i++) {
            out << *ckt.inputs[i] << std::endl;
        }
        for(unsigned i=0; i != ckt.outputs.size(); i++) {
            out << "OUTPUT(" << ckt.outputs[i]->name << ")" << std::endl;
        }
        for(unsigned i=0; i != ckt.gates.size(); i++) {
            out << *ckt.gates[i] << std::endl;
        }
        return out;
    }

    bool node_topo_cmp(const node_t* n_lt, const node_t* n_rt) 
    {
        return (n_lt->level < n_rt->level);
    }

    void ckt_t::topo_sort()
    {
        for(unsigned i=0; i != num_inputs(); i++) {
            inputs[i]->level = 0;
        }

        bool changed = false;
        do { 
            changed = false;
            for(unsigned i=0; i != num_gates(); i++) {
                node_t* g = gates[i];
                for(unsigned j=0; j != g->num_inputs(); j++) {
                    node_t* gi = g->inputs[j];
                    if(g->level < (1 + gi->level)) {
                        g->level = 1+gi->level;
                        changed = true;
                    }
                }
            }
        } while(changed);

        nodes_sorted.resize(nodes.size());
        gates_sorted.resize(gates.size());
        std::copy(gates.begin(), gates.end(), gates_sorted.begin());
        std::copy(nodes.begin(), nodes.end(), nodes_sorted.begin());

        std::sort(gates_sorted.begin(), gates_sorted.end(), node_topo_cmp);
        std::sort(nodes_sorted.begin(), nodes_sorted.end(), node_topo_cmp);
    }

    void ckt_t::init_keymap(std::vector< std::vector<bool> >& keymap)
    {
        keymap.resize(num_nodes());
        for(unsigned i=0; i != nodes_sorted.size(); i++) {
            keymap[i].resize(num_key_inputs(), false);
            std::fill(keymap[i].begin(), keymap[i].end(), false);
        }

        for(unsigned i=0; i != num_key_inputs(); i++) {
            node_t* ki = key_inputs[i];
            keymap[ki->get_index()][i] = true;
        }

        for(unsigned i=0; i != nodes_sorted.size(); i++) {
            node_t* n = nodes_sorted[i];
            int index = n->get_index();

            if(n->is_gate()) {
                for(unsigned j=0; j != n->num_inputs(); j++) {
                    or_bitmap(keymap[index], keymap[n->inputs[j]->get_index()]);
                }
            }
        }
    }

    void ckt_t::init_outputmap(std::vector< std::vector<bool> >& outmap)
    {
        outmap.resize(num_nodes());
        for(unsigned i=0; i != nodes_sorted.size(); i++) {
            outmap[i].resize(num_outputs(), false);
            std::fill(outmap[i].begin(), outmap[i].end(), false);
        }
        for(unsigned i=0; i != num_outputs(); i++) {
            node_t* oi = outputs[i];
            outmap[oi->get_index()][i] = true;
        }

        for(int i=nodes_sorted.size()-1; i >= 0; i--) {
            node_t* n = nodes_sorted[i];
            int index = n->get_index();
            for(unsigned j=0; j != n->num_fanouts(); j++) {
                or_bitmap(outmap[index], outmap[n->fanouts[j]->get_index()]);
            }
        }
    }

    void or_bitmap(std::vector<bool>& v1, std::vector<bool>& v2)
    {
        assert(v1.size() == v2.size());
        for(unsigned i=0; i != v2.size(); i++) {
            if(v2[i]) v1[i] = true;
        }
    }

    void ckt_t::get_indices_in_bitmap(
        const std::vector< std::vector<bool> >& bitmap, 
        node_t* n, 
        std::vector<int>& indices)
    {
        assert(bitmap.size() == num_nodes());
        int i = n->get_index();
        assert(i >= 0 && i < (int) num_nodes());
        const std::vector<bool>& bm = bitmap[i];

        for(unsigned i=0; i != bm.size(); i++) {
            if(bm[i]) {
                indices.push_back(i);
            }
        }
    }

    unsigned ckt_t::count_bitmap(const std::vector< std::vector<bool> >& keymap, node_t* n)
    {
        assert(keymap.size() == num_nodes());
        int i = n->get_index();
        assert(i >= 0 && i < (int) num_nodes());
        const std::vector<bool>& km = keymap[i];
        unsigned cnt=0;
        for(unsigned i=0; i != km.size(); i++) {
            if(km[i]) cnt += 1;
        }
        return cnt;
    }

    void ckt_t::identify_keypairs(std::set<std::pair<int, int> >& keypairs)
    {
        std::vector< std::vector<bool> > keymap;
        init_keymap(keymap);
        for(unsigned i=0; i != gates.size(); i++) {
            int cnt = count_bitmap(keymap, gates[i]);
            if(cnt == 2) {
                std::vector<int> keys;
                get_indices_in_bitmap(keymap, gates[i], keys);
                assert(keys.size() == 2);
                std::sort(keys.begin(), keys.end());
                std::pair<int, int> p(keys[0], keys[1]);
                keypairs.insert(p);
            } else if(cnt == 3) {
                std::vector<int> keys;
                get_indices_in_bitmap(keymap, gates[i], keys);
                assert(keys.size() == 3);
                std::sort(keys.begin(), keys.end());

                std::pair<int, int> p1(keys[0], keys[1]);
                keypairs.insert(p1);

                std::pair<int, int> p2(keys[0], keys[2]);
                keypairs.insert(p2);

                std::pair<int, int> p3(keys[1], keys[2]);
                keypairs.insert(p3);
            } else if(cnt == 4) {
                std::vector<int> keys;
                get_indices_in_bitmap(keymap, gates[i], keys);
                assert(keys.size() == 4);
                std::sort(keys.begin(), keys.end());

                std::pair<int, int> p1(keys[0], keys[1]);
                keypairs.insert(p1);

                std::pair<int, int> p2(keys[0], keys[2]);
                keypairs.insert(p2);

                std::pair<int, int> p3(keys[0], keys[3]);
                keypairs.insert(p3);

                std::pair<int, int> p4(keys[1], keys[2]);
                keypairs.insert(p4);

                std::pair<int, int> p5(keys[1], keys[3]);
                keypairs.insert(p5);

                std::pair<int, int> p6(keys[2], keys[3]);
                keypairs.insert(p6);
            }
        }
        keymap.clear();
    }

    void ckt_t::key_map_analysis()
    {

        std::vector< std::vector<bool> > keymap;
        init_keymap(keymap);
        for(unsigned i=0; i != gates.size(); i++) {
            int cnt = count_bitmap(keymap, gates[i]);
            if(cnt >= 2 && cnt <= 5) {
                std::cout << std::setw(10) << gates[i]->name << " --> " << 
                          std::setw(10) << cnt << std::endl;
            }
        }
    }

    void ckt_t::dump(std::ostream& out)
    {
        for(unsigned i=0; i != inputs.size(); i++) {
            out << "INPUT(" << inputs[i]->name << ")" << std::endl;
        }
        for(unsigned i=0; i != outputs.size(); i++) {
            out << "OUTPUT(" << outputs[i]->name << ")" << std::endl;
        }
        for(unsigned i=0; i != gates.size(); i++) {
            out << gates[i]->name << " = ";
            std::string f(gates[i]->func);
            std::transform(f.begin(), f.end(), f.begin(), ::toupper);
            out << f << "(";
            for(unsigned j=0; j != gates[i]->num_inputs(); j++) {
                bool last = (j+1 == gates[i]->num_inputs());
                out << gates[i]->inputs[j]->name;
                if(!last) out << ", ";
            }
            out << ")" << std::endl;
        }
    }

    void ckt_t::split_gates()
    {
        for(unsigned i=0; i != num_gates(); i++) {
            _split_gate(gates[i]);
        }
        _init_fanouts();
        _init_indices();
        topo_sort();
    }

    void ckt_t::_split_gate(node_t* g)
    {
        assert(node_t::GATE == g->type);
        if(g->num_inputs() > 2) {
            if(g->func == "xnor"  || g->func == "nand" || g->func == "nor") {
                std::string f;
                if(g->func == "xnor") { f = "xor"; } 
                else if(g->func == "nand") { f = "and"; } 
                else if(g->func == "nor") { f = "or"; }

                std::string n = g->name + "$inv";
                node_t* inp = _create_gate(g->inputs, f, n);

                g->func = "not";
                g->inputs.resize(1);
                g->inputs[0] = inp;
                g = inp;
            }

            assert(g->func == "or"  ||
                   g->func == "and" ||
                   g->func == "xor");

            unsigned h = g->num_inputs() / 2;
            nodelist_t i1, i2;
            // create the two groups of inputs.
            for(unsigned i=0; i < h; i++) { i1.push_back(g->inputs[i]); }
            for(unsigned i=h; i != g->num_inputs(); i++) { i2.push_back(g->inputs[i]); }

            assert(i1.size() >= 1);
            assert(i2.size() >= 1);

            node_t *g1 = NULL, *g2 = NULL;

            std::string n1 = g->name + "$1";
            std::string n2 = g->name + "$2";

            if(i1.size() == 1) { g1 = i1[0]; } 
            else { g1 = _create_gate(i1, g->func, n1); }

            if(i2.size() == 1) { g2 = i2[0]; }
            else { g2 = _create_gate(i2, g->func, n2); }

            g->inputs.resize(2);
            g->inputs[0] = g1;
            g->inputs[1] = g2;
        } else if(g->num_inputs() == 1) {
            if(g->func == "or") {
                g->func = "buf";
            }
        }
    }

    node_t* ckt_t::_create_gate(nodelist_t& inps, const std::string& func, const std::string& name)
    {
        node_t* g = node_t::create_gate(name, func);
        for(unsigned i=0; i != inps.size(); i++) {
            g->inputs.push_back(inps[i]);
        }
        gates.push_back(g);
        nodes.push_back(g);
        return g;
    }

    void ckt_t::add_ckt_input(node_t* ci)
    {
        assert(!ci->is_keyinput());
        nodes.push_back(ci);
        inputs.push_back(ci);
        ckt_inputs.push_back(ci);
    }

    node_t* ckt_t::create_key_input()
    {
        std::string name = "keyinput" + boost::lexical_cast<std::string>(num_key_inputs());
        node_t* ki = node_t::create_input(name);
        add_key_input(ki);
        return ki;
    }

    void ckt_t::add_key_input(node_t* ki)
    {
        assert(ki->is_keyinput());
        nodes.push_back(ki);
        inputs.push_back(ki);
        key_inputs.push_back(ki);
    }

    void ckt_t::insert_2inp_key_gate(node_t* ki, node_t* n, const std::string& func)
    {
        std::string name = n->name + "$enc";
        node_t* kg = node_t::create_gate(name, func);
        kg->add_input(n);
        kg->add_input(ki);
        add_gate(kg);
        if(n->output) {
            kg->output = true;
            replace_output(n, kg);
        }

        n->rewrite_fanouts_with(kg);
        assert(n->num_fanouts() == 0);

        ki->add_fanout(kg);
        n->add_fanout(kg);
    }

    node_t* ckt_t::insert_mux_key_gate(node_t* ki, node_t* n)
    {
        std::string func = "mux";
        std::string name = n->name + "$enc";
        node_t* kg = node_t::create_gate(name, func);
        kg->add_input(ki);
        kg->add_input(n);
        kg->add_input(NULL);
        add_gate(kg);

        if(n->output) {
            kg->output = true;
            replace_output(n, kg);
        }

        n->rewrite_fanouts_with(kg);
        assert(n->num_fanouts() == 0);

        ki->add_fanout(kg);
        n->add_fanout(kg);
        return kg;
    }

    void ckt_t::insert_key_gate(node_t* n, node_t* ki, int val)
    {
        // type = 0 means XOR and type = 1 means XNOR
        int type = rand() % 2;
        static const int GATE_XOR = 0;
        static const int GATE_XNOR = 1; (void) GATE_XNOR;

        std::string name = n->name + "$enc";
        std::string func = (type == GATE_XOR) ? "xor" : "xnor";
        node_t* kg = node_t::create_gate(name, func);
        kg->add_input(ki);
        ki->add_fanout(kg);
        add_gate(kg);

        if(n->output) {
            kg->output = true;
            replace_output(n, kg);
        }

        if(n->is_input()) {
            if((val == 0 && type == GATE_XOR) || (val == 1 && type == GATE_XNOR)) {
                n->rewrite_fanouts_with(kg);
                assert(n->num_fanouts() == 0);

                kg->add_input(n);
                n->add_fanout(kg);
            } else if((val == 1 && type == GATE_XOR) || (val == 0 && type == GATE_XNOR)) {
                n->rewrite_fanouts_with(kg);
                assert(n->num_fanouts() == 0);

                // need to invert in this case.
                std::string inv_name = n->name + "$inv";
                node_t* inv = node_t::create_gate(inv_name, "not");
                inv->add_input(n);
                n->add_fanout(inv);
                add_gate(inv);

                kg->add_input(inv);
                inv->add_fanout(kg);
            }
        } else {
            n->rewrite_fanouts_with(kg);
            assert(n->num_fanouts() == 0);

            kg->add_input(n);
            n->add_fanout(kg);

            if((val == 1 && type == GATE_XOR) || (val == 0 && type == GATE_XNOR)) {
                n->func = get_inv_func(n->func);
            }
        }
    }

    void ckt_t::add_gate(node_t* g)
    {
        assert(g->is_gate());
        nodes.push_back(g);
        gates.push_back(g);
    }

    const std::string& ckt_t::get_inv_func(const std::string& f)
    {
        static const std::string map[][2] = {
            { std::string("and"), std::string("nand") },
            { std::string("or"), std::string("nor") },
            { std::string("xor"), std::string("xnor") },
            { std::string("not"), std::string("buf") }
        };
        static const int MAP_SIZE = sizeof(map) / sizeof(map[0]);
        for(int i=0; i != MAP_SIZE; i++) {
            if(map[i][0] == f) return map[i][1];
            if(map[i][1] == f) return map[i][0];
        }
        assert(false);
        static const std::string& fail("fail");
        return fail;
    }

    void ckt_t::invert_gate(node_t* n)
    {
        static const std::string map[][2] = {
            { std::string("and"), std::string("nand") },
            { std::string("or"), std::string("nor") },
            { std::string("xor"), std::string("xnor") },
            { std::string("not"), std::string("buf") }
        };
        static const int MAP_SIZE = sizeof(map) / sizeof(map[0]);
        if(n->is_gate()) {
            bool inverted = false;
            for(int i=0; i != MAP_SIZE; i++) {
                if(n->func == map[i][0]) { 
                    inverted = true;
                    n->func = map[i][1];
                    break;
                } else if(n->func == map[i][1]) {
                    n->func = map[i][0];
                    inverted = true;
                    break;
                }
            }
            assert(inverted);
        } else {
            assert(n->is_input());

            node_t* n_inv = node_t::create_gate(n->name + "$inv", "not");
            n_inv->add_input(n);

            nodelist_t fanouts_A;
            for(nodelist_t::iterator it = n->fanouts.begin(); it != n->fanouts.end(); it++) {
                node_t* fout = *it;
                const std::string& name = fout->name;
                //std::cout << "fanout name: " << name << std::endl;
                if(boost::algorithm::ends_with(name, "_A")) {
                    //std::cout << "ends with A it seems " << name << std::endl;
                    fanouts_A.push_back(fout);
                } else {
                    assert(boost::algorithm::ends_with(name, "_B"));
                    //std::cout << "ends with B it seems " << name << std::endl;
                    fout->rewrite_input(n, n_inv);
                    n_inv->fanouts.push_back(fout);
                }
            }

            n->fanouts.clear();
            std::copy(fanouts_A.begin(), fanouts_A.end(), std::back_inserter(n->fanouts));
            add_gate(n_inv);
        }
    }

    int ckt_t::_find_in_array(const nodelist_t& list, node_t* n)
    {
        for(unsigned i=0; i != list.size(); i++) {
            if(list[i] == n) { return i; }
        }
        return -1;
    }

    void ckt_t::replace_output(node_t* old_out, node_t* new_out)
    {
        int pos = _find_in_array(outputs, old_out);
        assert(pos != -1);
        outputs[pos] = new_out;
    }

    void ckt_t::get_nodesets(const std::vector<bool>& fanins, node_t* inp, nodeset_t& internals, nodeset_t& inputs)
    {
        for(unsigned i=0; i != fanins.size(); i++) {
            if(fanins[i]) {
                node_t* n = nodes[i];
                assert(n->get_index() == (int)i);

                if(n == inp || n->is_input()) {
                    inputs.insert(n);
                } else {
                    internals.insert(n);
                }
            }
        }
    }


    void ckt_t::get_observable_nodes(node_t* out, nodeset_t& obs)
    {
        std::vector<bool> fanin_full(num_nodes(), false);
        std::vector<bool> fanin(num_nodes(), false);
        compute_transitive_fanin(out, fanin_full);

        for(unsigned i=0; i != fanin_full.size(); i++) {
            node_t* n = nodes[i];
            assert(n->get_index() == (int) i);
            if(fanin_full[i] && n != out) {
                compute_transitive_fanin(out, n, fanin);
                nodeset_t internals, inputs;
                get_nodesets(fanin, n, internals, inputs);
                
#ifdef DEBUG_OBSERVABLES
                std::cout << "tfi:: " << out->name << "/" << n->name << std::endl;
                std::cout << "   internals: " << internals << std::endl;
                std::cout << "   inputs: " << inputs << std::endl;
#endif

                if(is_observable(out, n, internals, inputs)) {
                    obs.insert(n);
                }
            }
        }
        obs.insert(out);
    }

    bool ckt_t::is_observable(node_t* n, std::vector<bool>& output_map)
    {
        if(n->is_input() && n->output) {
            return true;
        } else {
            nodelist_t sel_outputs;
            for(unsigned i=0; i != num_outputs(); i++) {
                if(output_map[i]) {
                    node_t* n_i = outputs[i];
                    sel_outputs.push_back(n_i);
                }
            }
            // so we are trying to create a slice of the circuit 
            // which contains only the outputs affected by this node.
            ckt_t::node_map_t nm_fwd, nm_rev;
            ckt_t cs(*this, sel_outputs, nm_fwd, nm_rev);

            // n_pr (i.e., n-prime) is the node we are interested in the sliced ckt.
            ckt_t::node_map_t::iterator pos_n = nm_fwd.find(n);
            assert(pos_n != nm_fwd.end());
            node_t* n_pr  = pos_n->second;

            // now we want to double this circuit.
            dblckt_t dbl(cs, dup_allkeys, true);
            // n_p2 is the node we are going to invert in the doubled ckt.
            node_t* n_p2 = dbl.getB(n_pr);

            //std::cout << "testing observability of: " << n->name << std::endl;
            //std::cout << "ORIGINAL CIRCUIT" << std::endl;
            //std::cout << *(dbl.dbl) << std::endl;
            //std::cout << "INVERTED CIRCUIT" << std::endl;
            dbl.dbl->invert_gate(n_p2);
            dbl.dbl->topo_sort();
            dbl.dbl->init_indices();
            //std::cout << *(dbl.dbl) << std::endl;

            // now we create the solver.
            sat_n::Solver S;
            index2lit_map_t  lmap;
            dbl.dbl->init_solver(S, lmap);

            // now we solve for the output = 1, which means the two circuits output different values.
            assert(dbl.dbl->num_outputs() == 1);
            node_t* out = dbl.dbl->outputs[0];
            sat_n::Lit l = dbl.dbl->getLit(lmap, out);

            return S.solve(l);
        }
    }

    bool ckt_t::is_observable(node_t* out, node_t* n, const nodeset_t& internals, const nodeset_t& inputs)
    {
        assert(inputs.find(n) != inputs.end());
        assert(internals.find(out) != internals.end());
        assert(inputs.find(out) == inputs.end());
        assert(internals.find(n) == internals.end());

        using namespace sat_n;
        Solver S;
        node2var_map_t nm1, nm2;
        Lit y = create_slice_diff(S, out, n, internals, inputs, nm1, nm2);

        assert(nm1.find(n) != nm1.end());
        assert(nm2.find(n) != nm2.end());
        Lit l1 = mkLit(nm1[n]);
        Lit l2 = mkLit(nm2[n]);

        vec_lit_t assumps;
        assumps.push(y);
        assumps.push(~l1);
        assumps.push(l2);
        return S.solve(assumps);
    }

    void ckt_t::compute_transitive_fanin(node_t* out, std::vector<bool>& fanin_flags, bool reset) const
    {
        if(reset) { std::fill(fanin_flags.begin(), fanin_flags.end(), false); }
        _compute_fanin_recursive(out, fanin_flags);
    }

    void ckt_t::compute_transitive_fanin(node_t* out, node_t* stop, std::vector<bool>& fanin_flags) const
    {
        fanin_flags.resize(num_nodes());
        std::fill(fanin_flags.begin(), fanin_flags.end(), false);
        fanin_flags[stop->get_index()] = true;
        _compute_fanin_recursive(out, fanin_flags);
    }

    void ckt_t::_compute_fanin_recursive(node_t* out, std::vector<bool>& fanin_flags) const
    {
        int index = out->get_index();
        assert(nodes[index] == out);
        if(fanin_flags[index]) return;
        else {
            fanin_flags[index] = true;
            for(unsigned i=0; i != out->num_inputs(); i++) {
                node_t* n = out->inputs[i];
                _compute_fanin_recursive(n, fanin_flags);
            }
        }
    }

    void ckt_t::_remove_from_array(nodelist_t& list, node_t* n)
    {
        assert(list.size() > 0);

        int pos = _find_in_array(list, n);
        assert(pos != -1);

        int last = list.size() - 1;
        list[pos] = list[last];

        list.resize(list.size() - 1);
    }

    void ckt_t::remove_key_input(node_t* ki)
    {
        _remove_from_array(key_inputs, ki);
        _remove_from_array(inputs, ki);
        _remove_from_array(nodes, ki);
    }

    void ckt_t::remove_gate(node_t* g)
    {
        _remove_from_array(nodes, g);
        _remove_from_array(gates, g);
    }

    bool ckt_t::compareIOs(ckt_t& ckt, int verbose)
    {
        if(ckt.num_outputs() != num_outputs()) {
            if(verbose) std::cerr << "Error: number of outputs don't match." << std::endl;
            return false;
        }
        if(ckt.num_ckt_inputs() != num_ckt_inputs()) {
            if(verbose) std::cerr << "Error: number of inputs don't match." << std::endl;
            return false;
        }

        for(unsigned i=0; i != ckt.num_outputs(); i++) {
            if(ckt.outputs[i]->name != outputs[i]->name && 
               ckt.outputs[i]->name + "$enc" != outputs[i]->name &&
               ckt.outputs[i]->name != outputs[i]->name + "$enc") 
            {
                if(verbose) std::cerr << "Error: names don't match: " 
                                      << ckt.outputs[i]->name << " and "
                                      << outputs[i]->name << "." << std::endl;
                return false;
            }
        }
        for(unsigned i=0; i != ckt.num_ckt_inputs(); i++) {
            if(ckt.ckt_inputs[i]->name != ckt_inputs[i]->name) {
                if(verbose) std::cerr << "Error: names don't match: " 
                                      << ckt.ckt_inputs[i]->name << " and "
                                      << ckt_inputs[i]->name << "." << std::endl;
                return false;
            }
        }
        return true;
    }
    int ckt_t::get_key_input_index(node_t* ki)
    {
        for(unsigned i=0; i != num_key_inputs(); i++) {
            if(ki == key_inputs[i]) {
                return i;
            }
        }
        return -1;
    }
    int ckt_t::get_ckt_input_index(node_t* ki)
    {
        for(unsigned i=0; i != num_ckt_inputs(); i++) {
            if(ki == ckt_inputs[i]) {
                return i;
            }
        }
        return -1;
    }

    void ckt_t::key_simplify(std::vector<sat_n::lbool>& keys)
    {
    }

    void ckt_t::rewrite_buffers(void)
    {
        for(unsigned i=0; i != gates.size(); i++) {
            bool rewrite = false;
            node_t* inp = NULL;
            // we look at the gate and set rewrite
            // and inp if the gate can be rewritten.
            if(gates[i]->func == "buf") {
                // case 1: gate is just a buffer.
                assert(gates[i]->num_inputs() == 1);
                rewrite = true;
                inp = gates[i]->inputs[0];
            } else if(gates[i]->func == "not") {
                // case 2: gate is an inverter and its input
                // is also an inverter.
                assert(gates[i]->num_inputs() == 1);
                if(gates[i]->inputs[0]->is_gate() &&
                   gates[i]->inputs[0]->func == "not")
                {
                    assert(gates[i]->inputs[0]->num_inputs() == 1);
                    rewrite = true;
                    inp = gates[i]->inputs[0]->inputs[0];
                }
            }

            // now do the actual rewriting.
            if(rewrite) {
                assert(inp != NULL);
                for(auto it = gates[i]->fanouts.begin(); 
                         it != gates[i]->fanouts.end();
                         it++)
                {
                    node_t* fout = *it;
                    fout->rewrite_input(gates[i], inp);
                    inp->fanouts.push_back(fout);
                }
                gates[i]->fanouts.clear();
            }
        }
    }

    void ckt_t::rewrite_keys(std::vector<sat_n::lbool>& keys)
    {
        assert(keys.size() == num_key_inputs());
        for(unsigned i=0; i != keys.size(); i++) {
            if(keys[i] != sat_n::l_Undef) {
                bool value = (keys[i] == sat_n::l_True);
                key_inputs[i]->const_propagate(NULL, value);
            }
        }
        _cleanup();
    }

    void ckt_t::rewrite_keys(std::map<std::string, int>& keys)
    {
        std::map<std::string, int> keynames;
        for(unsigned i=0; i != key_inputs.size(); i++) {
            keynames[key_inputs[i]->name] = i;
        }
        for(auto it = keys.begin(); it != keys.end(); it++) {
            auto pos = keynames.find(it->first);
            if(pos == keynames.end()) {
                std::cout << "Attempting to rewrite non-existent key: " << it->first 
                          << std::endl;
                exit(1);
            }
            int index = pos->second;
            int value = it->second;
            assert(index >= 0 && index < (int) num_key_inputs());
            assert(value == 0 || value == 1);
            key_inputs[index]->const_propagate(NULL, value);
        }
        _cleanup();
    }

    void ckt_t::rewrite_keys(std::vector< std::pair<int, int> >& keys)
    {
        for(unsigned i=0; i != keys.size(); i++) {
            int index = keys[i].first;
            int value = keys[i].second;
            assert(index >= 0 && index < (int) num_key_inputs());
            assert(value == 0 || value == 1);
            key_inputs[index]->const_propagate(NULL, value);
        }
        _cleanup();
    }

    void ckt_t::_cleanup()
    {
        bool changed;
        do {
            changed = false;
            changed = remove_dead_keys() || changed;
            changed = remove_dead_gates() || changed;
            rewrite_buffers();
            changed = remove_dead_keys() || changed;
            changed = remove_dead_gates() || changed;
        } while(changed);
        _init_fanouts();
        _init_indices();
        topo_sort();
        check_sanity();
    }

    void ckt_t::check_sanity()
    {
        assert(inputs.size() + gates.size() == nodes.size());

        nodeset_t input_set;
        nodeset_t gate_set;
        nodeset_t node_set;

        std::copy(inputs.begin(), inputs.end(), std::inserter(input_set, input_set.begin()));
        std::copy(gates.begin(), gates.end(), std::inserter(gate_set, gate_set.begin()));
        std::copy(nodes.begin(), nodes.end(), std::inserter(node_set, node_set.begin()));


        assert(inputs.size() == input_set.size());
        assert(gates.size() == gate_set.size());
        assert(nodes.size() == node_set.size());

        for(unsigned i=0; i != nodes.size(); i++) {
            node_t* ni = nodes[i];
            if(ni->is_input()) {
                assert(input_set.find(ni) != input_set.end());
                assert(gate_set.find(ni) == gate_set.end());
            } else {
                assert(ni->is_gate());
                assert(input_set.find(ni) == input_set.end());
                assert(gate_set.find(ni) != gate_set.end());
            }
            assert(nodes[ni->get_index()] == ni);
            for(unsigned j=0; j != ni->num_inputs(); j++) {
                node_t* nj = ni->inputs[j];
                assert(node_set.find(nj) != node_set.end());
                assert(nodes[nj->get_index()] == nj);
            }
        }
    }

    bool ckt_t::remove_dead_keys(void)
    {
        nodeset_t to_del_keyinputs;
        for(unsigned i=0; i != num_key_inputs(); i++) {
            node_t* ki = key_inputs[i];
            if(ki->num_fanouts() == 0) {
                assert(!ki->output);
                to_del_keyinputs.insert(ki);
            }
        }
        _delete_from_list(key_inputs, to_del_keyinputs);
        _delete_from_list(inputs, to_del_keyinputs);
        _delete_from_list(nodes, to_del_keyinputs);
        return to_del_keyinputs.size() > 0;
    }

    bool ckt_t::remove_dead_gates(void)
    {
        nodeset_t to_del_gates;
        for(unsigned i=0; i != num_gates(); i++) {
            node_t* gi = gates[i];
            if(gi->num_fanouts() == 0 && !gi->output) {
                to_del_gates.insert(gi);
            }
        }
        _delete_from_list(gates, to_del_gates);
        _delete_from_list(nodes, to_del_gates);
        return to_del_gates.size() > 0;
    }

    void ckt_t::_delete_from_list(nodelist_t& list, nodeset_t& delset) 
    {
        nodelist_t newlist;
        for(auto it=list.begin(); it != list.end(); it++) {
            node_t* ni = *it;
            if(delset.find(ni) == delset.end()) {
                newlist.push_back(ni);
            }
        }
        list.resize(newlist.size());
        std::copy(newlist.begin(), newlist.end(), list.begin());
    }

    void ckt_t::dump_cuts(int limit, int ht)
    {
        std::vector<kcutset_t*> allcuts;
        compute_cuts(allcuts, *this, limit, ht);
        for(unsigned i=0; i != num_nodes(); i++) {
            int idx = nodes[i]->get_index();
            kcutset_t* cuts = allcuts[idx];
            std::cout << "NODE: " << nodes[i]->name << std::endl;
            for(auto jt=cuts->begin(); jt != cuts->end(); jt++) {
                kcut_t* cut = *jt;
                std::cout << "  " << *cut; 
                if(cut->has_multiple_keyinputs() && cut->is_self_contained()) {
                    std::cout << "[yes]";
                }
                std::cout << std::endl;
            }
        }
    }

    void ckt_t::_compute_gate_prob( node_t* ni, std::vector<double>& ps )
    {
        assert(ni->is_gate());
        int idx = ni->get_index();

        std::vector<double> pins;
        for(unsigned j=0; j != ni->num_inputs(); j++) {
            node_t* nj = ni->inputs[j];
            int jdx = nj->get_index();
            assert(ps[jdx] != -1);
            pins.push_back(ps[jdx]);
        }
        ps[idx] = ni->calc_out_prob(pins);
    }

    void ckt_t::_compute_node_prob( std::vector<double>& ps ) 
    {
        ps.resize(num_nodes());
        std::fill(ps.begin(), ps.end(), -1);

        for(unsigned i=0; i != num_nodes(); i++) {
            node_t* ni = nodes_sorted[i];
            int idx = ni->get_index();
            assert(ps[idx] == -1);
            if(ni->is_input()) {
                ps[idx] = 0.5;
            } else {
                _compute_gate_prob(ni, ps);
            }
        }
    }

    void ckt_t::compute_node_prob( std::vector<double>& ps )
    {
        std::vector<double> ps_init;
        _compute_node_prob(ps_init);

        std::vector< std::vector<double> > ps_array;
        std::vector< double > ps0, ps1, ps_avg;
        ps_avg.resize(num_nodes());
        for(unsigned i=0; i != num_inputs(); i++) {
            _compute_node_prob(ps0, inputs[i], 0);
            _compute_node_prob(ps1, inputs[i], 1);
            for(unsigned j=0; j != num_nodes(); j++) {
                ps_avg[j] = (ps0[j] + ps1[j]) / 2.0;
            }
            ps_array.push_back(ps_avg);
        }
        assert(ps_array.size() == num_inputs());

        ps.resize(num_nodes());
        for(unsigned i=0; i != num_nodes(); i++) {
            ps[i] = _compute_wt_avg(ps_init, ps_array, i);
            assert(ps[i] >= 0 && ps[i] <= (1+1e-9));
        }
    }

    double ckt_t::_compute_wt_avg(
        std::vector<double>& ps_init, std::vector<std::vector<double> >& ps_array, unsigned j)
    {
        assert(ps_init.size() == num_nodes());
        assert(j < num_nodes());

        double p0 = ps_init[j];
        assert(nodes[j]->get_index() == (int) j);
        if(nodes[j]->is_input()) return p0;

        double sumDiff = 0;
        for(unsigned i=0; i != ps_array.size(); i++) {
            assert(ps_array[i].size() == num_nodes());
            double pj = ps_array[i][j];
            sumDiff += fabs(pj - p0);
        }

        if(sumDiff == 0) {
            return p0;
        }

        double wtavg = 0;
        for(unsigned i=0; i != ps_array.size(); i++) {
            assert(ps_array[i].size() == num_nodes());
            double pj = ps_array[i][j];
            double wt = fabs(pj - p0) / sumDiff;
            wtavg += wt * pj;
        }
        return wtavg;
    }


    void ckt_t::_compute_node_prob( std::vector<double>& ps, node_t* inp, int val ) 
    {
        ps.resize(num_nodes());
        std::fill(ps.begin(), ps.end(), -1);
        assert(inp->is_input());
        assert(val == 0 || val == 1);

        for(unsigned i=0; i != num_nodes(); i++) {
            node_t* ni = nodes_sorted[i];
            int idx = ni->get_index();
            assert(ps[idx] == -1);
            if(ni->is_input()) {
                if(ni == inp) {
                    ps[idx] = val;
                } else {
                    ps[idx] = 0.5;
                }
            } else {
                _compute_gate_prob(ni, ps);
            }
        }
    }

    bool ckt_t::has_cycle(std::vector<int>& marks) const
    {
        static const int NO_MARK = 0;
        if(marks.size() != num_nodes()) marks.resize(num_nodes());

        std::fill(marks.begin(), marks.end(), NO_MARK);

        for(unsigned i=0; i != num_nodes(); i++) {
            if(marks[i] != NO_MARK) continue;
            else {
                bool cycle_found = _cycle_check(i, marks);
                if (cycle_found) return true;
            }
        }
        return false;
    }

    bool ckt_t::_cycle_check(unsigned i, std::vector<int>& marks) const
    {
        static const int NO_MARK = 0, TEMP_MARK = 1, PERM_MARK = 2;
        if(marks[i] == TEMP_MARK) return true;
        if(marks[i] == NO_MARK) {
            marks[i] = TEMP_MARK;
            node_t* ni = nodes[i];
            assert(ni->get_index() == (int)i);
            for(unsigned j=0; j != ni->num_inputs(); j++) {
                node_t* nj = ni->inputs[j];
                unsigned jdx = nj->get_index();
                if(_cycle_check(jdx, marks)) return true;
            }
            marks[i] = PERM_MARK;
        }
        return false;
    } 

    // JOHANN
    void ckt_t::readStochFile(std::string file) {
	std::ifstream in;
	node_t* gate;
	std::string drop;
	std::string sampling_flag;
	std::string gate_name;
	std::string error_rate;
	std::string polymorphic_gate__function;
	std::string polymorphic_gate__probability;
	unsigned stochastic_gates = 0;

	in.open(file.c_str());

	if (!in.good()) {
		std::cout << std::endl;
		std::cout << "No such stochastic definition file: " << file << std::endl;
		std::cout << "Circuit will behave fully deterministic" << std::endl;
		std::cout << std::endl;
		return;
	}

	// drop header; five words
	// # OUTPUT_SAMPLING_ON OUTPUT_SAMPLING_ITERATIONS OUTPUT_SAMPLING_FOR_TEST_ON TEST_PATTERNS
	in >> drop;
	in >> drop;
	in >> drop;
	in >> drop;
	in >> drop;

	// parse sampling flag; if true sample multiple output observations for one particular input pattern, and subsequently pick the most common pattern as ground truth
	in >> sampling_flag;

	if (sampling_flag == "1" || sampling_flag == "true") {
		IO_sampling_flag = true;
	}
	else {
		IO_sampling_flag = false;
	}

	// parse sampling iterations
	in >> IO_sampling_iter;

	// parse sampling flag for test phase; if true also sample output observations for testing 
	in >> sampling_flag;

	if (sampling_flag == "1" || sampling_flag == "true") {
		IO_sampling_for_test_flag = true;
	}
	else {
		IO_sampling_for_test_flag = false;
	}

	// parse test patterns 
	in >> test_patterns;

	// drop header; until keyword NEXT_GATE
	do {
		in >> drop;
	} while (drop != "NEXT_GATE");

	// parse stochastic gates
	//
	while (!in.eof()) {
		in >> gate_name;
		in >> error_rate;

		if (in.eof()) {
			break;
		}

		gate = nullptr;
		for (auto* g : gates) {
			if (g->name == gate_name) {
				gate = g;
				stochastic_gates++;
				break;
			}
		}

		if (gate == nullptr) {
			std::cout << "Parsing of " << file << "; no such gate exists in the circuit: " << gate_name << std::endl;

			// drop list of polymorphic functions, if there is one, for gate not found
			do {
				in >> drop;

				if (in.eof()) {
					break;
				}
			} while (drop != "NEXT_GATE");
		}
		else {
			gate->error_rate = atof(error_rate.c_str());

			if (ckt_t::DBG) {
				std::cout << "Parsing of " << file << "; gate " << gate_name << " annotated with error rate of " << gate->error_rate << "%" << std::endl;
			}

			// next word is keyword, either NEXT_GATE or POLYMORPHIC_GATE
			in >> drop;

			// don't check for POLYMORPHIC_GATE, but rather check only that it's not NEXT_GATE; this way, any typo in POLYMORPHIC_GATE is not an issue
			if (drop != "NEXT_GATE") {

				// parse list of polymorphic functions
				do {
					// polymorphic functions are always in pairs; if NEXT_GATE occurs, the list of functions ended already
					//
					in >> polymorphic_gate__function;
					if (in.eof() || polymorphic_gate__function == "NEXT_GATE") {
						break;
					}
					in >> polymorphic_gate__probability;
					if (in.eof()) {
						break;
					}

					poly_fct p_fct;

					p_fct.probability = atof(polymorphic_gate__probability.c_str());
					p_fct.name = polymorphic_gate__function;

					if (p_fct.name == "AND") {
						p_fct.function = fct::AND;
					}
					else if (p_fct.name == "NAND") {
						p_fct.function = fct::NAND;
					}
					else if (p_fct.name == "OR") {
						p_fct.function = fct::OR;
					}
					else if (p_fct.name == "NOR") {
						p_fct.function = fct::NOR;
					}
					else if (p_fct.name == "XOR") {
						p_fct.function = fct::XOR;
					}
					else if (p_fct.name == "XNOR") {
						p_fct.function = fct::XNOR;
					}
					else if (p_fct.name == "INV") {
						p_fct.function = fct::INV;
					}
					else if (p_fct.name == "BUF") {
						p_fct.function = fct::BUF;
					}
					else {
						p_fct.function = fct::UNDEF;
					}

					gate->polymorphic_fcts.push_back(p_fct);

					if (ckt_t::DBG) {
						std::cout << "Parsing of " << file << "; gate " << gate_name << " has polymorphic function \"" << p_fct.name << "\"; probability for that function is " << p_fct.probability << "%" << std::endl;
					}

				} while (true);

				if (ckt_t::DBG) {
					std::cout << "Parsing of " << file << "; gate " << gate_name << " has in total " << gate->polymorphic_fcts.size() << " polymorphic functions" << std::endl;
				}
			}
		}
	}
	std::cout << "Stochastic gates count: " << stochastic_gates << std::endl;

	// in case no stochastic gates was defined, sampling of output patterns is superfluous
	//
	if (stochastic_gates == 0) {
		IO_sampling_flag = false;
	}

	std::cout << "Sampling of output observations for stochastic circuits: ";
	if (IO_sampling_flag) {
		std::cout << "on" << std::endl;
		std::cout << " Sampling iterations (for each pattern): " << IO_sampling_iter << std::endl;
	}
	else {
		std::cout << "off" << std::endl;
	}

	in.close();
    }
}
