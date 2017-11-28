#include "sat.h"

namespace ckt_n {
    and_provider_t<sat_n::Solver>  and_provider;
    or_provider_t<sat_n::Solver>   or_provider;
    nand_provider_t<sat_n::Solver> nand_provider;
    nor_provider_t<sat_n::Solver>  nor_provider;
    xor_provider_t<sat_n::Solver>  xor_provider;
    xnor_provider_t<sat_n::Solver>  xnor_provider;
    mux_provider_t<sat_n::Solver>  mux_provider;

    template<>
    const clause_provider_i<sat_n::Solver>* get_provider<sat_n::Solver>(
        const std::string& func, 
        int num_inputs
    )
    {
        if(func == "and") {
            assert(num_inputs >= 2);
            return &and_provider;
        } else if(func == "or") {
            assert(num_inputs >= 1 /* we allow or gates to have only one input. */);
            return &or_provider;
        } else if(func == "nand") {
            assert(num_inputs >= 2);
            return &nand_provider;
        } else if(func == "nor") {
            assert(num_inputs >= 2);
            return &nor_provider;
        } else if(func == "xor") {
            assert(num_inputs == 2);
            return &xor_provider;
        } else if(func == "xnor") {
            assert(num_inputs == 2);
            return &xnor_provider;
        } else if(func == "not") {
            assert(num_inputs == 1);
            return &nand_provider;
        } else if(func == "buf") {
            assert(num_inputs == 1);
            return &and_provider;
        } else if(func == "mux") {
            assert(num_inputs == 3);
            return &mux_provider;
        } else {
            return NULL;
        }
    }

    and_provider_t<AllSAT::AllSATCkt>  and_provider3;
    or_provider_t<AllSAT::AllSATCkt>   or_provider3;
    nand_provider_t<AllSAT::AllSATCkt> nand_provider3;
    nor_provider_t<AllSAT::AllSATCkt>  nor_provider3;
    xor_provider_t<AllSAT::AllSATCkt>  xor_provider3;
    xnor_provider_t<AllSAT::AllSATCkt>  xnor_provider3;
    mux_provider_t<AllSAT::AllSATCkt>  mux_provider3;

    template<>
    const clause_provider_i<AllSAT::AllSATCkt>* get_provider<AllSAT::AllSATCkt>(
        const std::string& func, 
        int num_inputs
    )
    {
        if(func == "and") {
            assert(num_inputs >= 2);
            return &and_provider3;
        } else if(func == "or") {
            assert(num_inputs >= 1 /* we allow or gates to have only one input. */);
            return &or_provider3;
        } else if(func == "nand") {
            assert(num_inputs >= 2);
            return &nand_provider3;
        } else if(func == "nor") {
            assert(num_inputs >= 2);
            return &nor_provider3;
        } else if(func == "xor") {
            assert(num_inputs == 2);
            return &xor_provider3;
        } else if(func == "xnor") {
            assert(num_inputs == 2);
            return &xnor_provider3;
        } else if(func == "not") {
            assert(num_inputs == 1);
            return &nand_provider3;
        } else if(func == "buf") {
            assert(num_inputs == 1);
            return &and_provider3;
        } else if(func == "mux") {
            assert(num_inputs == 3);
            return &mux_provider3;
        } else {
            return NULL;
        }
    }

    and_provider_t<AllSAT::ClauseList>  and_provider4;
    or_provider_t<AllSAT::ClauseList>   or_provider4;
    nand_provider_t<AllSAT::ClauseList> nand_provider4;
    nor_provider_t<AllSAT::ClauseList>  nor_provider4;
    xor_provider_t<AllSAT::ClauseList>  xor_provider4;
    xnor_provider_t<AllSAT::ClauseList>  xnor_provider4;
    mux_provider_t<AllSAT::ClauseList>  mux_provider4;

    template<>
    const clause_provider_i<AllSAT::ClauseList>* get_provider<AllSAT::ClauseList>(
        const std::string& func, 
        int num_inputs
    )
    {
        if(func == "and") {
            assert(num_inputs >= 2);
            return &and_provider4;
        } else if(func == "or") {
            assert(num_inputs >= 1 /* we allow or gates to have only one input. */);
            return &or_provider4;
        } else if(func == "nand") {
            assert(num_inputs >= 2);
            return &nand_provider4;
        } else if(func == "nor") {
            assert(num_inputs >= 2);
            return &nor_provider4;
        } else if(func == "xor") {
            assert(num_inputs == 2);
            return &xor_provider4;
        } else if(func == "xnor") {
            assert(num_inputs == 2);
            return &xnor_provider4;
        } else if(func == "not") {
            assert(num_inputs == 1);
            return &nand_provider4;
        } else if(func == "buf") {
            assert(num_inputs == 1);
            return &and_provider4;
        } else if(func == "mux") {
            assert(num_inputs == 3);
            return &mux_provider4;
        } else {
            return NULL;
        }
    }

    and_provider_t<b2qbf::ISolver> and_provider5;
    or_provider_t<b2qbf::ISolver>   or_provider5;
    nand_provider_t<b2qbf::ISolver> nand_provider5;
    nor_provider_t<b2qbf::ISolver>  nor_provider5;
    xor_provider_t<b2qbf::ISolver>  xor_provider5;
    xnor_provider_t<b2qbf::ISolver>  xnor_provider5;
    mux_provider_t<b2qbf::ISolver>  mux_provider5;

    template<>
    const clause_provider_i<b2qbf::ISolver>* get_provider<b2qbf::ISolver>(
        const std::string& func, 
        int num_inputs
    ) 
    {
        if(func == "and") {
            assert(num_inputs >= 2);
            return &and_provider5;
        } else if(func == "or") {
            assert(num_inputs >= 1 /* we allow or gates to have only one input. */);
            return &or_provider5;
        } else if(func == "nand") {
            assert(num_inputs >= 2);
            return &nand_provider5;
        } else if(func == "nor") {
            assert(num_inputs >= 2);
            return &nor_provider5;
        } else if(func == "xor") {
            assert(num_inputs == 2);
            return &xor_provider5;
        } else if(func == "xnor") {
            assert(num_inputs == 2);
            return &xnor_provider5;
        } else if(func == "not") {
            assert(num_inputs == 1);
            return &nand_provider5;
        } else if(func == "buf") {
            assert(num_inputs == 1);
            return &and_provider5;
        } else if(func == "mux") {
            assert(num_inputs == 3);
            return &mux_provider5;
        } else {
            return NULL;
        }
    }
}
