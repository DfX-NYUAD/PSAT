#ifndef _LUT_ENCODER_H_DEFINED_
#define _LUT_ENCODER_H_DEFINED_

#include "ckt.h"
#include "ast.h"

namespace ckt_n {
    struct lut_encoder_t
    {
        ckt_n::ckt_t ckt;
        lut_encoder_t(ast_n::statements_t& stms, int seed);
        int encode(int keys, const std::string& outputfile, bool ex);
        ~lut_encoder_t();
    };
}
#endif
