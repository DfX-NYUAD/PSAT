#include "sim.h"
#include "util.h"

namespace ckt_n {

    eval_t::eval_t(ckt_t& c) 
        : ckt(c)
    {
        ckt.init_solver(S, mappings);
    }

    void eval_t::set_cnst(node_t* n, int val)
    {
        using namespace sat_n;

        assert(n->is_keyinput());

        assert(val == 0 || val == 1);

        int idx = n->get_index();
        assert(idx < (int) mappings.size());

        Lit v = (val == 0) ? ~mappings[idx] : mappings[idx];
        S.addClause(v);
    }
    
    void eval_t::eval(nodelist_t& input_nodes, const bool_vec_t& input_values, bool_vec_t& outputs)
    {
        using namespace sat_n;

        assert(input_nodes.size() == input_values.size());

        vec_lit_t assump;
        for(unsigned i=0; i != input_values.size(); i++) {
            Lit l = input_values[i] ? 
                mappings[input_nodes[i]->get_index()] : 
                ~mappings[input_nodes[i]->get_index()];
            assump.push(l);
        }

        bool result = S.solve(assump);
        if(!result) {
            std::cout << "inputs: ";
            for(unsigned i=0; i != input_nodes.size(); i++) {
                std::cout << input_nodes[i]->name << " ";
            }
            std::cout << std::endl;
        }
        assert(true == result);

        outputs.resize(ckt.num_outputs());
        for(unsigned i=0; i != outputs.size(); i++) {
            int idx = ckt.outputs[i]->get_index();
            lbool vi = S.modelValue(var(mappings[idx]));
            assert(vi.isDef());
            outputs[i] = (vi.getBool());
        }
    }

    void ckt_eval_t::eval(
        const std::vector<bool>& input_values,
        std::vector<bool>& output_values
    )
    {
        sim.eval(inputs, input_values, output_values);
    }

    void convert(uint64_t v, bool_vec_t& result)
    {
        for(unsigned i = 0; i < result.size(); i++) {
            uint64_t mask = (uint64_t)((uint64_t)1 << i);
            if((v & mask) != 0) {
                result[i] = true;
            } else {
                result[i] = false;
            }
        }
    }

    uint64_t convert(const bool_vec_t& v) {
        uint64_t r = 0;
        for(unsigned i=0; i < v.size(); i++) {
            uint64_t mask = (uint64_t)((uint64_t)1 << i);
            if(v[i]) r |= mask;
        }
        return r;
    }

    void dump_mappings(std::ostream& out, ckt_t& ckt, index2lit_map_t& mappings)
    {
        for(unsigned i=0; i != ckt.num_inputs(); i++) {
            std::cout << "input " << ckt.inputs[i]->name << " --> " 
                      << mappings[ckt.inputs[i]->get_index()] << std::endl;
        }
        for(unsigned i=0; i != ckt.num_outputs(); i++) {
            std::cout << "output " << ckt.outputs[i]->name << " --> " 
                      << mappings[ckt.outputs[i]->get_index()] << std::endl;
        }
        for(unsigned i=0; i != ckt.num_gates(); i++) {
            std::cout << "gate " << ckt.gates[i]->name << " --> " 
                      << mappings[ckt.gates[i]->get_index()] << std::endl;
        }
    }
}
