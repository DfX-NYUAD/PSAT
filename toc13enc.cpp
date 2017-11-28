#include "util.h"
#include "toc13enc.h"
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <iomanip>

namespace ckt_n
{
    void toc13enc_t::readFaultImpact(const std::string& fault_impact_file)
    {
        std::ifstream fin(fault_impact_file.c_str());

        std::string node_name;
        double fault_impact_metric;
        std::map<std::string, double> fault_impact_map;
        while((fin >> node_name >> fault_impact_metric)) {
            fault_impact_map[node_name] = fault_impact_metric;
        }

        if(ckt.num_nodes() != fault_impact_map.size()) {
            std::cerr << "Error: the fault impact file should have "
                      << ckt.num_nodes() << " values but it has "
                      << fault_impact_map.size() << " values instead."
                      << std::endl;
            exit(1);
        }
        faultMetrics.resize(ckt.num_nodes());
        for(unsigned i=0; i != ckt.num_nodes(); i++) {
            std::string name = ckt.nodes[i]->name;
            auto pos = fault_impact_map.find(name);
            if(pos == fault_impact_map.end()) {
                std::cerr << "Error: Unable to find node " << name 
                          << " in map. " << std::endl;
                exit(1);
            }
            faultMetrics[i] = pos->second;
        }
    }

    void toc13enc_t::evaluateFaultImpact(int nSims)
    {
        _evaluateRandomVectors(nSims);
        faultMetrics.resize(ckt.num_nodes());
        for(unsigned i=0; i != ckt.num_nodes(); i++) {
            faultMetrics[i] = _evaluateFaultImpact(ckt.nodes[i]);
        }
    }

    void toc13enc_t::_convert_node_prob(std::vector<double>& ps, std::map<std::string, double>& pmap)
    {
        for(unsigned i=0; i != ckt.num_nodes(); i++) {
            assert(ckt.nodes[i]->get_index() == (int) i);
            const std::string& name = ckt.nodes[i]->name;
            assert(pmap.find(name) == pmap.end());
            pmap[name] = ps[i];
        }
    }

    void toc13enc_t::encodeIOLTS14()
    {
        int target_keys = (int) (fraction*ckt.num_nodes() + 0.5);
        std::vector<double> ps;
        ckt.compute_node_prob(ps);

        // convert to a map.
        std::map<std::string, double> pmap;
        _convert_node_prob(ps, pmap);

        // sort the probabilities according to the scheme in the paper.
        assert(ckt.num_nodes() == ps.size());
        typedef std::pair<double, node_t*> key_t;
        std::vector<key_t> candidates;
        for(unsigned i=0; i != ckt.num_nodes(); i++) {
            double pi = ps[i];
            assert(pi >= 0 && pi < (1 + 1e-10));
            if(pi > 1) pi = 1;
            double fi = 1-pi;
            double mi = std::min(pi, fi);
            key_t k(mi, ckt.nodes[i]);
            candidates.push_back(k);
        }

        std::sort(candidates.begin(), candidates.end());
        assert(target_keys < (int) candidates.size());
        for(int i=0; i != target_keys; i++) {
            node_t* n = candidates[i].second;
            double p = _get_prob(pmap, n);

            // decide whether to insert and or or.
            std::string func;
            int key;

            if(p > 0.5) {
                func = "and";
                key = 1;
            } else {
                func = "or";
                key = 0;
            }

            node_t* ki = ckt.create_key_input();
            key_values.push_back(key);
            ckt.insert_2inp_key_gate(ki, n, func);
        }
    }

    void toc13enc_t::encodeMuxes()
    {
        int target_keys = (int) (fraction*ckt.num_nodes() + 0.5);

        std::vector<double> ps;
        std::map<std::string, double> pmap;
        ckt.compute_node_prob(ps);
        _convert_node_prob(ps, pmap);

        typedef std::pair<double, int> double_int_pair_t;
        std::set<double_int_pair_t> metric_set;
        for(unsigned i=0; i != ckt.num_nodes(); i++) {
            double_int_pair_t p(faultMetrics[i], i);
            metric_set.insert(p);
        }
        key_values.clear();

        assert(target_keys < (int) ckt.num_nodes());
        int cnt;
        nodeset_t nodes_to_insert;
        for(cnt=0; cnt < target_keys; cnt++) {
            // select the node.
            assert(metric_set.size() > 0);
            auto last = metric_set.rbegin();
            int index = last->second;
            assert(index >= 0 && index < (int) ckt.num_nodes());
            node_t* n = ckt.nodes[index];

            // get rid of this node now.
            double_int_pair_t p(*last);
            metric_set.erase(p);
            nodes_to_insert.insert(n);
        }
        for(auto it=nodes_to_insert.begin(); it != nodes_to_insert.end(); it++) {
            node_t* n = *it;

            // add the key now.
            node_t* ki = ckt.create_key_input();
            int val = rand() % 2;
            key_values.push_back(val ? true : false);
            node_t* kg = ckt.insert_mux_key_gate(ki, n);
            node_t* other = _get_best_other(n, kg, pmap);
            if(val == 1) {
                // mux input B is the node n.
                kg->inputs[1] = other;
                kg->inputs[2] = n;
            } else {
                // mux input A is the node n.
                kg->inputs[2] = other;
            }
            other->add_fanout(kg);
        }
        ckt.init_indices();
        ckt.topo_sort();
        ckt.cleanup();
    }

    double toc13enc_t::_get_prob(std::map<std::string, double>& pmap, node_t* n)
    {
        auto pos = pmap.find(n->name);
        if(pos != pmap.end()) {
            return pos->second;
        } else {
            return -1;
        }
    }

    node_t* toc13enc_t::_get_best_other(node_t* n1, node_t* kg, std::map<std::string, double>& pmap)
    {
        assert(kg->num_inputs() == 3);
        assert(kg->inputs[1] == n1);
        assert(kg->inputs[2] == NULL);

        double best_pr = 0;
        node_t* best_node = NULL;

        double p0 = _get_prob(pmap, n1);
        ckt.init_indices();

        for(unsigned i=0; i != ckt.num_nodes(); i++) {
            node_t* ni = ckt.nodes[i];
            if(ni->is_keyinput()) continue;

            double pi = _get_prob(pmap, ni);
            if(pi == -1) continue;

            kg->inputs[2] = ni;
            if(ckt.has_cycle(marks)) continue;

            double pr_metric = p0*(1-pi) + pi*(1-p0);
            if(pr_metric > best_pr) {
                best_node = ni;
                best_pr = pr_metric;
            }
        }
        assert(best_node != NULL);
        return best_node;
    }

    void toc13enc_t::encodeXORs()
    {
        int target_keys = (int) (fraction*ckt.num_nodes() + 0.5);

        typedef std::pair<double, int> double_int_pair_t;
        std::set<double_int_pair_t> metric_set;
        for(unsigned i=0; i != ckt.num_nodes(); i++) {
            double_int_pair_t p(faultMetrics[i], i);
            metric_set.insert(p);
        }

        key_values.clear();

        assert(target_keys < (int) ckt.num_nodes());
        int cnt;
        nodeset_t nodes_to_insert;
        for(cnt=0; cnt < target_keys; cnt++) {
            // select the node.
            assert(metric_set.size() > 0);
            auto last = metric_set.rbegin();
            int index = last->second;
            assert(index >= 0 && index < (int) ckt.num_nodes());
            node_t* n = ckt.nodes[index];

            // get rid of this node now.
            double_int_pair_t p(*last);
            metric_set.erase(p);
            // add to the set.
            nodes_to_insert.insert(n);
        }
        for(auto it=nodes_to_insert.begin(); it != nodes_to_insert.end(); it++) {
            node_t* n = *it;
            // add the key now.
            node_t* ki = ckt.create_key_input();
            int val = rand() % 2;
            key_values.push_back(val ? true : false);
            ckt.insert_key_gate(n, ki, val);
        }
        ckt.topo_sort();
        ckt.cleanup();
    }

    void toc13enc_t::write(std::ostream& out)
    {
        out << "# key=" << key_values << std::endl;
        out << ckt << std::endl;
    }

    void toc13enc_t::_evaluateRandomVectors(int nSims)
    {
        ckt_eval_t sim(ckt, ckt.ckt_inputs);

        assert(ckt.num_key_inputs() == 0);
        bool_vec_t inputs(ckt.num_ckt_inputs());
        bool_vec_t outputs(ckt.num_outputs());

        for(int i=0; i < nSims; i++) {
            for(unsigned j=0; j != inputs.size(); j++) {
                inputs[j] = rand()%2;
            }
            sim.eval(inputs, outputs);
            input_sims.push_back(inputs);
            output_sims.push_back(outputs);
        }
    }

    double toc13enc_t::_evaluateFaultImpact(node_t* n)
    {
        assert(input_sims.size() == output_sims.size());

        using namespace sat_n;

        Solver S;

        nodeset_t n2ignore;
        n2ignore.insert(n);
        index2lit_map_t lmap;
        ckt.init_solver_ignoring_nodes(S, lmap, n2ignore);

        vec_lit_t assumps(ckt.num_inputs() + 1);
        bool_vec_t err_outputs(ckt.num_outputs());

        double metric = 0;
        for(unsigned int i=0; i < input_sims.size(); i++) {
            bool result;

            _set_assumps(assumps, lmap, input_sims[i], n, 0);
            result = S.solve(assumps);
            assert(result);
            _extract_outputs(S, lmap, err_outputs);
            metric += _count_differing_outputs(output_sims[i], err_outputs);

            _set_assumps(assumps, lmap, input_sims[i], n, 1);
            result = S.solve(assumps);
            assert(result);
            _extract_outputs(S, lmap, err_outputs);
            metric += _count_differing_outputs(output_sims[i], err_outputs);
        }
        return metric / input_sims.size();
    }

    void toc13enc_t::_set_assumps(sat_n::vec_lit_t& assumps,
        ckt_n::index2lit_map_t& lmap,
        const bool_vec_t& inputs,
        node_t* n,
        int fault)
    {
        using namespace sat_n;
        assert(assumps.size() == inputs.size()+1);
        assert(inputs.size() == ckt.num_ckt_inputs());
        for(unsigned i=0; i != ckt.num_ckt_inputs(); i++) {
            Lit li = ckt.getLit(lmap, ckt.ckt_inputs[i]);
            if(ckt.ckt_inputs[i] != n) {
                assumps[i] = inputs[i] ? li : ~li;
            } else {
                assumps[i] = fault ? li : ~li;
            }
        }
        int last = ckt.num_ckt_inputs();
        Lit l_node = ckt.getLit(lmap, n);
        assumps[last] = fault ? l_node : ~l_node;
    }

    void toc13enc_t::_extract_outputs(
        sat_n::Solver& S,
        ckt_n::index2lit_map_t& lmap,
        bool_vec_t& outputs
    )
    {
        using namespace sat_n;

        assert(outputs.size() == ckt.num_outputs());
        for(unsigned i=0; i != ckt.num_outputs(); i++) {
            lbool l_out_i = S.modelValue(ckt.getLit(lmap, ckt.outputs[i]));
            assert(l_out_i.isDef());
            outputs[i] = l_out_i.getBool();
        }
    }
    
    int toc13enc_t::_count_differing_outputs(
        const bool_vec_t& sim_out, const bool_vec_t& err_out)
    {
        int cnt = 0;
        assert(sim_out.size() == err_out.size());
        for(unsigned i=0; i != sim_out.size(); i++) {
            if(sim_out[i] != err_out[i]) cnt += 1;
        }
        return cnt;
    }
}



// end of the file.
// 
//
//
