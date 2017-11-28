#ifndef _SIM_H_DEFINED_
#define _SIM_H_DEFINED_

#include <vector>
#include "ckt.h"
#include "util.h"
#include "SATInterface.h"

namespace ckt_n {
    typedef std::vector<bool> bool_vec_t;

    struct simulator_t {
        virtual void eval(
            const std::vector<bool>& input_values,
            std::vector<bool>& output_values
        ) = 0;
    };

    struct eval_t {
        sat_n::Solver S;
        index2lit_map_t mappings;
        ckt_t& ckt;

        eval_t(ckt_t& c);
        ~eval_t() {}

        void set_cnst(node_t* n, int val);
        void eval(nodelist_t& input_nodes, const bool_vec_t& input_values, bool_vec_t& outputs);
    };

    struct ckt_eval_t : public simulator_t {
        eval_t sim;
        nodelist_t& inputs;

        ckt_eval_t(ckt_t& c, nodelist_t& inps) 
            : sim(c)
            , inputs(inps)
        {
            for(unsigned i=0; i != c.num_key_inputs(); i++) {
                set_cnst(c.key_inputs[i], 0);
            }
        }

        void set_cnst(node_t* n, int val) { sim.set_cnst(n, val); }
        virtual void eval(
            const std::vector<bool>& inputs,
            std::vector<bool>& outputs
        );
    };

    void convert(uint64_t v, bool_vec_t& result);
    uint64_t convert(const bool_vec_t& v);

    void dump_mappings(
        std::ostream& out, ckt_t& ckt, index2lit_map_t& mappings);
}

#endif
