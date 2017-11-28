#ifndef _OPT_ENC_H_DEFINED_H_
#define _OPT_ENC_H_DEFINED_H_

#include <iostream>
#include "ast.h"
#include "ckt.h"
#include "SATInterface.h"

namespace ckt_n {
    class ilp_encoder_t
    {
        ckt_t ckt;
        int target_keys;
        std::vector<int> key_values;

        int get_min_output_level(node_t* n, std::vector<bool>& output_map);
        int should_use_min_cnst(node_t* n, std::vector<bool>& output_map);
    public:
        ilp_encoder_t(ast_n::statements_t& stms, double fraction);
        virtual ~ilp_encoder_t();
        
        void encode1();
        void encode2();
        void encode3();
        void write(std::ostream& out);
    };
}
#endif
