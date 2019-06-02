#include "lutencoder.h"
#include <stdlib.h>

namespace ckt_n
{
    lut_encoder_t::lut_encoder_t(ast_n::statements_t& stms, int seed)
        : ckt(stms)
    {
        srand(seed);
    }

    lut_encoder_t::~lut_encoder_t()
    {
    }

    int lut_encoder_t::encode(int keys, const std::string& outputfile, bool ex)
    {
        for(unsigned i=0; i != ckt.num_outputs(); i++) {
            using namespace ckt_n;

            nodeset_t obs;
            ckt.init_indices();
            ckt.get_observable_nodes(ckt.outputs[i], obs);
            std::cout << "observable[" << ckt.outputs[i]->name << "]: " << obs << std::endl;
        }
        return 0;
    }
}

