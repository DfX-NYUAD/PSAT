#ifndef _SAT_H_DEFINED_
#define _SAT_H_DEFINED_

#include "SATInterface.h"
#include "allsatckt.h"
#include <string>
#include <vector>
#include <map>
#include "ISolver.h"

namespace ckt_n {
    struct node_t;
    typedef std::vector<node_t*> nodelist_t;
    typedef std::map<node_t*, sat_n::Var> node2var_map_t;
    typedef std::vector<sat_n::Lit> index2lit_map_t;

    template<class Solver>
    struct clause_provider_i {
        virtual void add_clauses(
            Solver& S, 
            node2var_map_t& map, 
            nodelist_t& inputs, 
            node_t* output
        ) const = 0;
    };

    // provider factory.
    template<class Solver>
    const clause_provider_i<Solver>* get_provider(
        const std::string& func, 
        int num_inputs
    );

    template<class Solver>
    struct and_provider_t : public clause_provider_i<Solver> {
        virtual void add_clauses(
            Solver& S, 
            node2var_map_t& map, 
            nodelist_t& inputs, 
            node_t* output
        ) const
        {
            using namespace sat_n;

            assert(map.find(output) != map.end());
            Lit y = mkLit(map[output]);

            vec_lit_t xs;
            xs.growTo(inputs.size() + 1);
            for(unsigned i=0; i != inputs.size(); i++) {
                assert(map.find(inputs[i]) != map.end());
                xs[i] = mkLit(map[inputs[i]]);

                // ~xs[i] => ~y
                // <==> xs[i] + ~y 
                S.addClause(xs[i], ~y);
                // invert for the ~xs[0] + ~xs[1] + ... + ~xs[n] + y clause.
                xs[i] = ~xs[i];
            }
            int last = inputs.size();
            xs[last] = y;
            S.addClause(xs);
        }
    };

    template<class Solver>
    struct or_provider_t : public clause_provider_i<Solver> {
        virtual void add_clauses(
            Solver& S, 
            node2var_map_t& map, 
            nodelist_t& inputs, 
            node_t* output
        ) const
        {
            using namespace sat_n;

            assert(map.find(output) != map.end());
            Lit y = mkLit(map[output]);

            vec_lit_t xs;
            xs.growTo(inputs.size() + 1);
            for(unsigned i=0; i != inputs.size(); i++) {
                assert(map.find(inputs[i]) != map.end());
                xs[i] = mkLit(map[inputs[i]]);

                // xs[i] => y
                // <==> ~xs[i] + y 
                S.addClause(~xs[i], y);
            }
            // xs[0] + xs[1] + ... + xs[n] + ~y clause.
            int last = inputs.size();
            xs[last] = ~y;
            S.addClause(xs);
        } 
    };

    template<class Solver>
    struct nand_provider_t : public clause_provider_i<Solver> {
        virtual void add_clauses(
            Solver& S, 
            node2var_map_t& map, 
            nodelist_t& inputs, 
            node_t* output
        ) const
        {
            using namespace sat_n;

            assert(map.find(output) != map.end());
            Lit y = mkLit(map[output]);

            vec_lit_t xs;
            xs.growTo(inputs.size() + 1);
            for(unsigned i=0; i != inputs.size(); i++) {
                assert(map.find(inputs[i]) != map.end());
                xs[i] = mkLit(map[inputs[i]]);

                // ~xs[i] => y
                // <==> xs[i] + y 
                S.addClause(xs[i], y);
                // invert for the ~xs[0] + ~xs[1] + ... + ~xs[n] + ~y clause.
                xs[i] = ~xs[i];
            }
            int last = inputs.size();
            xs[last] = ~y;
            S.addClause(xs);
        }
    };

    template<class Solver>
    struct nor_provider_t : public clause_provider_i<Solver> {
        virtual void add_clauses(
            Solver& S, 
            node2var_map_t& map, 
            nodelist_t& inputs, 
            node_t* output
        ) const
        {
            using namespace sat_n;

            assert(map.find(output) != map.end());
            Lit y = mkLit(map[output]);

            vec_lit_t xs;
            xs.growTo(inputs.size() + 1);
            for(unsigned i=0; i != inputs.size(); i++) {
                assert(map.find(inputs[i]) != map.end());
                xs[i] = mkLit(map[inputs[i]]);

                // xs[i] => ~y
                // <==> ~xs[i] + ~y 
                S.addClause(~xs[i], ~y);
            }
            // xs[0] + xs[1] + ... + xs[n] + y clause.
            int last = inputs.size();
            xs[last] = y;
            S.addClause(xs);
        }
    };

    template<class Solver>
    struct xor_provider_t : public clause_provider_i<Solver> {
        virtual void add_clauses(
            Solver& S, 
            node2var_map_t& map, 
            nodelist_t& inputs, 
            node_t* output
        ) const
        {
            using namespace sat_n;

            assert(inputs.size() == 2);
            assert(map.find(inputs[0]) != map.end());
            assert(map.find(inputs[1]) != map.end());
            Lit a = mkLit(map[inputs[0]]);
            Lit b = mkLit(map[inputs[1]]);

            assert(map.find(output) != map.end());
            Lit y = mkLit(map[output]);

            // ~a ~b => ~y <==> a + b + ~y
            S.addClause(a, b, ~y);
            // a b => ~y <==> ~a + ~b + ~y
            S.addClause(~a, ~b, ~y);
            // ~a b => y <==> a + ~b + y
            S.addClause(a, ~b, y);
            // a ~b => y <==> ~a + b + y
            S.addClause(~a, b, y);
        }
    };

    template<class Solver>
    struct xnor_provider_t : public clause_provider_i<Solver> {
        virtual void add_clauses(
            Solver& S, 
            node2var_map_t& map, 
            nodelist_t& inputs, 
            node_t* output
        ) const
        {
            using namespace sat_n;

            assert(inputs.size() == 2);
            assert(map.find(inputs[0]) != map.end());
            assert(map.find(inputs[1]) != map.end());
            Lit a = mkLit(map[inputs[0]]);
            Lit b = mkLit(map[inputs[1]]);

            assert(map.find(output) != map.end());
            Lit y = mkLit(map[output]);

            // ~a ~b => y <==> a + b + y
            S.addClause(a, b, y);
            // a b => y <==> ~a + ~b + y
            S.addClause(~a, ~b, y);
            // ~a b => ~y <==> a + ~b + ~y
            S.addClause(a, ~b, ~y);
            // a ~b => ~y <==> ~a + b + ~y
            S.addClause(~a, b, ~y);
        }
    };

    template<class Solver>
    struct mux_provider_t : public clause_provider_i<Solver> {
        virtual void add_clauses(
            Solver& S, 
            node2var_map_t& map, 
            nodelist_t& inputs, 
            node_t* output
        ) const
        {
            using namespace sat_n;

            // The mux is defined as:
            // y    = ~s*a   +  s*b
            // ~y   =  ~s*~a +  s*~b
            // This translates to the following clauses:
            //   (s + ~a + y) (~s + ~b + y) (s + a + ~y) (~s + b + ~y)

            assert(inputs.size() == 3);
            assert(map.find(inputs[0]) != map.end());
            assert(map.find(inputs[1]) != map.end());
            assert(map.find(inputs[2]) != map.end());
            Lit s = mkLit(map[inputs[0]]);
            Lit a = mkLit(map[inputs[1]]);
            Lit b = mkLit(map[inputs[2]]);

            assert(map.find(output) != map.end());
            Lit y = mkLit(map[output]);

            //   (s + ~a + y) 
            S.addClause(s, ~a, y);
            //   (~s + ~b + y)
            S.addClause(~s, ~b, y);
            //   (s + a + ~y)
            S.addClause(s, a, ~y);
            //   (~s + b + ~y)
            S.addClause(~s, b, ~y);
        }
    };

    extern and_provider_t<sat_n::Solver>  and_provider;
    extern or_provider_t<sat_n::Solver>   or_provider;
    extern nand_provider_t<sat_n::Solver> nand_provider;
    extern nor_provider_t<sat_n::Solver>  nor_provider;
    extern xor_provider_t<sat_n::Solver>  xor_provider;
    extern xnor_provider_t<sat_n::Solver>  xnor_provider;
    extern mux_provider_t<sat_n::Solver>  mux_provider;

    template<>
    const clause_provider_i<sat_n::Solver>* get_provider<sat_n::Solver>(
        const std::string& func, 
        int num_inputs
    );

    extern and_provider_t<AllSAT::AllSATCkt>  and_provider3;
    extern or_provider_t<AllSAT::AllSATCkt>   or_provider3;
    extern nand_provider_t<AllSAT::AllSATCkt> nand_provider3;
    extern nor_provider_t<AllSAT::AllSATCkt>  nor_provider3;
    extern xor_provider_t<AllSAT::AllSATCkt>  xor_provider3;
    extern xnor_provider_t<AllSAT::AllSATCkt>  xnor_provider3;
    extern mux_provider_t<AllSAT::AllSATCkt>  mux_provider3;

    template<>
    const clause_provider_i<AllSAT::AllSATCkt>* get_provider<AllSAT::AllSATCkt>(
        const std::string& func, 
        int num_inputs
    );

    extern and_provider_t<AllSAT::ClauseList>  and_provider4;
    extern or_provider_t<AllSAT::ClauseList>   or_provider4;
    extern nand_provider_t<AllSAT::ClauseList> nand_provider4;
    extern nor_provider_t<AllSAT::ClauseList>  nor_provider4;
    extern xor_provider_t<AllSAT::ClauseList>  xor_provider4;
    extern xnor_provider_t<AllSAT::ClauseList>  xnor_provider4;
    extern mux_provider_t<AllSAT::ClauseList>  mux_provider4;

    template<>
    const clause_provider_i<AllSAT::ClauseList>* get_provider<AllSAT::ClauseList>(
        const std::string& func, 
        int num_inputs
    );

    extern and_provider_t<b2qbf::ISolver> and_provider5;
    extern or_provider_t<b2qbf::ISolver>   or_provider5;
    extern nand_provider_t<b2qbf::ISolver> nand_provider5;
    extern nor_provider_t<b2qbf::ISolver>  nor_provider5;
    extern xor_provider_t<b2qbf::ISolver>  xor_provider5;
    extern xnor_provider_t<b2qbf::ISolver>  xnor_provider5;
    extern mux_provider_t<b2qbf::ISolver>  mux_provider5;

    template<>
    const clause_provider_i<b2qbf::ISolver>* get_provider<b2qbf::ISolver>(
        const std::string& func, 
        int num_inputs
    );
}
#endif
