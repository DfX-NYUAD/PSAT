#ifndef _TOC_13_ENC_H_DEFINED_
#define _TOC_13_ENC_H_DEFINED_

#include <vector>
#include "ckt.h"
#include "ast.h"
#include "node.h"
#include "sim.h"

namespace ckt_n
{
    class toc13enc_t
    {
    public:
    protected:
        ckt_t ckt;
        double fraction;

        std::vector<bool_vec_t> input_sims;
        std::vector<bool_vec_t> output_sims;
        std::vector<int> onesCount;
        std::vector<double> faultMetrics;
        std::vector<bool> key_values;

        std::vector<int> marks;
    public:
        toc13enc_t(ast_n::statements_t& stms, double fr)
            : ckt(stms)
            , fraction(fr)
        {
        }

        void evaluateFaultImpact(int nSims);
        void readFaultImpact(const std::string& fault_impact_file);
        void encodeXORs();
        void encodeMuxes();
        void encodeIOLTS14();
        void write(std::ostream& out);
    private:
        // reformat the vector of probabilities into a map (indices will change during encryption.)
        void _convert_node_prob(std::vector<double>& ps, std::map<std::string, double>& pmap);
        double _get_prob(std::map<std::string, double>& pmap, node_t* n);

        void _evaluateRandomVectors(int nSims);
        double _evaluateFaultImpact(node_t* n);
        void _set_assumps(sat_n::vec_lit_t& assumps,
            ckt_n::index2lit_map_t& lmap,
            const bool_vec_t& inputs,
            node_t* n,
            int fault);
        void _extract_outputs(
            sat_n::Solver& S,
            ckt_n::index2lit_map_t& lmap,
            bool_vec_t& outputs
        );
        node_t* _get_best_other(node_t* n1, node_t* kg, std::map<std::string, double>& ps);
        int _count_differing_outputs(
            const bool_vec_t& sim_out, const bool_vec_t& err_out);
    };
}

#endif


// end of the file.
// 
//
//
