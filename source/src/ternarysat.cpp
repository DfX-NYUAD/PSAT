#include "ternarysat.h"

namespace ckt_n
{
    ternary_triop_provider_t<sat_n::Solver>   ternary_mux_provider    (add_mux_ternary);

    ternary_binop_provider_t<sat_n::Solver>   ternary_and_provider    (add_and_ternary);
    ternary_binop_provider_t<sat_n::Solver>   ternary_or_provider     (add_or_ternary);
    ternary_binop_provider_t<sat_n::Solver>   ternary_nand_provider   (add_nand_ternary);
    ternary_binop_provider_t<sat_n::Solver>   ternary_nor_provider    (add_nor_ternary);
    ternary_binop_provider_t<sat_n::Solver>   ternary_xor_provider    (add_xor_ternary);
    ternary_binop_provider_t<sat_n::Solver>   ternary_xnor_provider   (add_xnor_ternary);

    ternary_unop_provider_t<sat_n::Solver>    ternary_not_provider    (add_not_ternary);
    ternary_unop_provider_t<sat_n::Solver>    ternary_buf_provider    (add_buf_ternary);

    template<>
    const clause_provider_i<sat_n::Solver>* get_ternary_provider<sat_n::Solver>(
        const std::string& func, 
        int num_inputs
    )
    {
        if(func == "and") {
            assert(num_inputs == 2);
            return &ternary_and_provider;
        } else if(func == "or") {
            assert(num_inputs >= 1 && num_inputs <= 2);

            if(num_inputs == 2) {
                return &ternary_or_provider;
            } else {
                return &ternary_buf_provider;
            }

        } else if(func == "nand") {
            assert(num_inputs >= 2);
            return &ternary_nand_provider;
        } else if(func == "nor") {
            assert(num_inputs >= 2);
            return &ternary_nor_provider;
        } else if(func == "xor") {
            assert(num_inputs == 2);
            return &ternary_xor_provider;
        } else if(func == "xnor") {
            assert(num_inputs == 2);
            return &ternary_xnor_provider;
        } else if(func == "not") {
            assert(num_inputs == 1);
            return &ternary_not_provider;
        } else if(func == "buf") {
            assert(num_inputs == 1);
            return &ternary_buf_provider;
        } else if(func == "mux") {
            assert(num_inputs == 3);
            return &ternary_mux_provider;
        } else {
            return NULL;
        }
    }

}
