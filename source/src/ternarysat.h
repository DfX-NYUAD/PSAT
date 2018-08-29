#ifndef _TERNARYSAT_H_DEFINED_
#define _TERNARYSAT_H_DEFINED_

#include "SATInterface.h"
#include "sat.h"

namespace ckt_n {
    template<class Solver>
    const clause_provider_i<Solver>* get_ternary_provider(
        const std::string& func, 
        int num_inputs
    );

    template<class Solver>
    void add_ternary_triop_clauses(
        Solver& S,
        node2var_map_t& map,
        nodelist_t& inputs,
        node_t* output,
        void (*add_clauses)(Solver& S, sat_n::vec_lit_t& xs, sat_n::vec_lit_t& ys)
    )
    {
        using namespace sat_n;
        assert(inputs.size() == 3);
        assert(map.find(inputs[0]) != map.end());
        assert(map.find(inputs[1]) != map.end());
        assert(map.find(inputs[2]) != map.end());
        Lit a0 = mkLit(map[inputs[0]]);
        Lit a1 = mkLit(map[inputs[0]] + 1);
        Lit b0 = mkLit(map[inputs[1]]);
        Lit b1 = mkLit(map[inputs[1]] + 1);
        Lit c0 = mkLit(map[inputs[2]]);
        Lit c1 = mkLit(map[inputs[2]] + 1);

        assert(map.find(output) != map.end());
        Lit y0 = mkLit(map[output]);
        Lit y1 = mkLit(map[output] + 1);

        vec_lit_t xs;
        xs.push(a0); xs.push(a1);
        xs.push(b0); xs.push(b1);
        xs.push(c0); xs.push(c1);

        vec_lit_t ys;
        ys.push(y0); ys.push(y1);

        add_clauses(S, xs, ys);
    }

    template<class Solver>
    void add_ternary_binop_clauses(
        Solver& S,
        node2var_map_t& map,
        nodelist_t& inputs,
        node_t* output,
        void (*add_clauses)(Solver& S, sat_n::vec_lit_t& xs, sat_n::vec_lit_t& ys)
    )
    {
        using namespace sat_n;
        assert(inputs.size() == 2);
        assert(map.find(inputs[0]) != map.end());
        assert(map.find(inputs[1]) != map.end());
        Lit a0 = mkLit(map[inputs[0]]);
        Lit a1 = mkLit(map[inputs[0]] + 1);
        Lit b0 = mkLit(map[inputs[1]]);
        Lit b1 = mkLit(map[inputs[1]] + 1);

        assert(map.find(output) != map.end());
        Lit y0 = mkLit(map[output]);
        Lit y1 = mkLit(map[output] + 1);

        vec_lit_t xs;
        xs.push(a0); xs.push(a1);
        xs.push(b0); xs.push(b1);

        vec_lit_t ys;
        ys.push(y0); ys.push(y1);

        add_clauses(S, xs, ys);
    }

    template<class Solver>
    void add_ternary_unop_clauses(
        Solver& S,
        node2var_map_t& map,
        nodelist_t& inputs,
        node_t* output,
        void (*add_clauses)(Solver& S, sat_n::vec_lit_t& xs, sat_n::vec_lit_t& ys)
    )
    {
        using namespace sat_n;
        assert(inputs.size() == 1);
        assert(map.find(inputs[0]) != map.end());
        Lit a0 = mkLit(map[inputs[0]]);
        Lit a1 = mkLit(map[inputs[0]] + 1);

        assert(map.find(output) != map.end());
        Lit y0 = mkLit(map[output]);
        Lit y1 = mkLit(map[output] + 1);

        vec_lit_t xs;
        xs.push(a0); xs.push(a1);

        vec_lit_t ys;
        ys.push(y0); ys.push(y1);

        add_clauses(S, xs, ys);
    }

    template<class Solver>
    class ternary_triop_provider_t : public clause_provider_i<Solver> {
    protected:
        typedef void (*add_clauses_t)(Solver& S, sat_n::vec_lit_t& xs, sat_n::vec_lit_t& ys);
        add_clauses_t add_clauses_func;
    public:
        ternary_triop_provider_t(add_clauses_t f) : add_clauses_func(f) {}

        virtual void add_clauses(
            Solver& S, 
            node2var_map_t& map, 
            nodelist_t& inputs, 
            node_t* output
        ) const
        {
            add_ternary_triop_clauses(S, map, inputs, output, add_clauses_func);
        }
    };

    template<class Solver>
    class ternary_binop_provider_t : public clause_provider_i<Solver> {
    protected:
        typedef void (*add_clauses_t)(Solver& S, sat_n::vec_lit_t& xs, sat_n::vec_lit_t& ys);
        add_clauses_t add_clauses_func;
    public:
        ternary_binop_provider_t(add_clauses_t f) : add_clauses_func(f) {}

        virtual void add_clauses(
            Solver& S, 
            node2var_map_t& map, 
            nodelist_t& inputs, 
            node_t* output
        ) const
        {
            add_ternary_binop_clauses(S, map, inputs, output, add_clauses_func);
        }
    };

    template<class Solver>
    class ternary_unop_provider_t : public clause_provider_i<Solver> {
    protected:
        typedef void (*add_clauses_t)(Solver& S, sat_n::vec_lit_t& xs, sat_n::vec_lit_t& ys);
        add_clauses_t add_clauses_func;
    public:
        ternary_unop_provider_t(add_clauses_t f) : add_clauses_func(f) {}

        virtual void add_clauses(
            Solver& S, 
            node2var_map_t& map, 
            nodelist_t& inputs, 
            node_t* output
        ) const
        {
            add_ternary_unop_clauses(S, map, inputs, output, add_clauses_func);
        }
    };


    extern ternary_triop_provider_t<sat_n::Solver>    ternary_mux_provider;

    extern ternary_binop_provider_t<sat_n::Solver>    ternary_and_provider;
    extern ternary_binop_provider_t<sat_n::Solver>    ternary_or_provider;
    extern ternary_binop_provider_t<sat_n::Solver>    ternary_nand_provider;
    extern ternary_binop_provider_t<sat_n::Solver>    ternary_nor_provider;
    extern ternary_binop_provider_t<sat_n::Solver>    ternary_xor_provider;
    extern ternary_binop_provider_t<sat_n::Solver>    ternary_xnor_provider;

    extern ternary_unop_provider_t<sat_n::Solver>     ternary_not_provider;
    extern ternary_unop_provider_t<sat_n::Solver>     ternary_buf_provider;

    template<>
    const clause_provider_i<sat_n::Solver>* get_ternary_provider<sat_n::Solver>(
        const std::string& func, 
        int num_inputs
    );

    // the following code is generated using a python script.

    // --BEGIN AUTOGENERATED--

    // Ternary represenation comments.
    // a[0] is the X bit.
    // a[1] is the actual value.

    template<class Solver>
    void add_buf_ternary(
        Solver& S,
        sat_n::vec_lit_t& xs, 
        sat_n::vec_lit_t& ys
    )
    {
        using namespace sat_n;
        // xs[0] => ys[0]
        S.addClause(~xs[0], ys[0]);
        // ~xs[0] => ~ys[0]
        S.addClause(xs[0], ~ys[0]);
        // xs[1] => ys[1]
        S.addClause(~xs[1], ys[1]);
        // ~xs[1] => ~ys[1]
        S.addClause(xs[1], ~ys[1]);
    }

    template<class Solver>
    void add_not_ternary(
        Solver& S,
        sat_n::vec_lit_t& xs, 
        sat_n::vec_lit_t& ys
    )
    {
        using namespace sat_n;
        // xs[0] => ys[0]
        S.addClause(~xs[0], ys[0]);
        // ~xs[0] => ~ys[0]
        S.addClause(xs[0], ~ys[0]);
        // xs[1] => ~ys[1]
        S.addClause(~xs[1], ~ys[1]);
        // ~xs[1] => ys[1]
        S.addClause(xs[1], ys[1]);
    }

    template<class Solver>
    void add_and_ternary(
        Solver& S,
        sat_n::vec_lit_t& xs, 
        sat_n::vec_lit_t& ys
    )
    {
        using namespace sat_n;
        vec_lit_t cl;


        // 1--1 1
        cl.push(~xs[0]);
        cl.push(~xs[3]);
        cl.push(ys[0]);
        S.addClause(cl);
        cl.clear();

        // -11- 1
        cl.push(~xs[1]);
        cl.push(~xs[2]);
        cl.push(ys[0]);
        S.addClause(cl);
        cl.clear();

        // 1-1- 1
        cl.push(~xs[0]);
        cl.push(~xs[2]);
        cl.push(ys[0]);
        S.addClause(cl);
        cl.clear();

        // -1-1 1
        cl.push(~xs[1]);
        cl.push(~xs[3]);
        cl.push(ys[1]);
        S.addClause(cl);
        cl.clear();

        // 0-0- 1
        cl.push(xs[0]);
        cl.push(xs[2]);
        cl.push(~ys[0]);
        S.addClause(cl);
        cl.clear();

        // --00 1
        cl.push(xs[2]);
        cl.push(xs[3]);
        cl.push(~ys[0]);
        S.addClause(cl);
        cl.clear();

        // 00-- 1
        cl.push(xs[0]);
        cl.push(xs[1]);
        cl.push(~ys[0]);
        S.addClause(cl);
        cl.clear();

        // ---0 1
        cl.push(xs[3]);
        cl.push(~ys[1]);
        S.addClause(cl);
        cl.clear();

        // -0-- 1
        cl.push(xs[1]);
        cl.push(~ys[1]);
        S.addClause(cl);
        cl.clear();


    }

    template<class Solver>
    void add_nand_ternary(
        Solver& S,
        sat_n::vec_lit_t& xs, 
        sat_n::vec_lit_t& ys
    )
    {
        using namespace sat_n;
        vec_lit_t cl;


        // 1--1 1
        cl.push(~xs[0]);
        cl.push(~xs[3]);
        cl.push(ys[0]);
        S.addClause(cl);
        cl.clear();

        // -11- 1
        cl.push(~xs[1]);
        cl.push(~xs[2]);
        cl.push(ys[0]);
        S.addClause(cl);
        cl.clear();

        // 1-1- 1
        cl.push(~xs[0]);
        cl.push(~xs[2]);
        cl.push(ys[0]);
        S.addClause(cl);
        cl.clear();

        // ---0 1
        cl.push(xs[3]);
        cl.push(ys[1]);
        S.addClause(cl);
        cl.clear();

        // -0-- 1
        cl.push(xs[1]);
        cl.push(ys[1]);
        S.addClause(cl);
        cl.clear();

        // 0-0- 1
        cl.push(xs[0]);
        cl.push(xs[2]);
        cl.push(~ys[0]);
        S.addClause(cl);
        cl.clear();

        // --00 1
        cl.push(xs[2]);
        cl.push(xs[3]);
        cl.push(~ys[0]);
        S.addClause(cl);
        cl.clear();

        // 00-- 1
        cl.push(xs[0]);
        cl.push(xs[1]);
        cl.push(~ys[0]);
        S.addClause(cl);
        cl.clear();

        // -1-1 1
        cl.push(~xs[1]);
        cl.push(~xs[3]);
        cl.push(~ys[1]);
        S.addClause(cl);
        cl.clear();


    }

    template<class Solver>
    void add_or_ternary(
        Solver& S,
        sat_n::vec_lit_t& xs, 
        sat_n::vec_lit_t& ys
    )
    {
        using namespace sat_n;
        vec_lit_t cl;


        // 1--0 1
        cl.push(~xs[0]);
        cl.push(xs[3]);
        cl.push(ys[0]);
        S.addClause(cl);
        cl.clear();

        // -01- 1
        cl.push(xs[1]);
        cl.push(~xs[2]);
        cl.push(ys[0]);
        S.addClause(cl);
        cl.clear();

        // 1-1- 1
        cl.push(~xs[0]);
        cl.push(~xs[2]);
        cl.push(ys[0]);
        S.addClause(cl);
        cl.clear();

        // -1-- 1
        cl.push(~xs[1]);
        cl.push(ys[1]);
        S.addClause(cl);
        cl.clear();

        // ---1 1
        cl.push(~xs[3]);
        cl.push(ys[1]);
        S.addClause(cl);
        cl.clear();

        // 0-0- 1
        cl.push(xs[0]);
        cl.push(xs[2]);
        cl.push(~ys[0]);
        S.addClause(cl);
        cl.clear();

        // --01 1
        cl.push(xs[2]);
        cl.push(~xs[3]);
        cl.push(~ys[0]);
        S.addClause(cl);
        cl.clear();

        // 01-- 1
        cl.push(xs[0]);
        cl.push(~xs[1]);
        cl.push(~ys[0]);
        S.addClause(cl);
        cl.clear();

        // -0-0 1
        cl.push(xs[1]);
        cl.push(xs[3]);
        cl.push(~ys[1]);
        S.addClause(cl);
        cl.clear();


    }

    template<class Solver>
    void add_nor_ternary(
        Solver& S,
        sat_n::vec_lit_t& xs, 
        sat_n::vec_lit_t& ys
    )
    {
        using namespace sat_n;
        vec_lit_t cl;


        // 1--0 1
        cl.push(~xs[0]);
        cl.push(xs[3]);
        cl.push(ys[0]);
        S.addClause(cl);
        cl.clear();

        // -01- 1
        cl.push(xs[1]);
        cl.push(~xs[2]);
        cl.push(ys[0]);
        S.addClause(cl);
        cl.clear();

        // 1-1- 1
        cl.push(~xs[0]);
        cl.push(~xs[2]);
        cl.push(ys[0]);
        S.addClause(cl);
        cl.clear();

        // -0-0 1
        cl.push(xs[1]);
        cl.push(xs[3]);
        cl.push(ys[1]);
        S.addClause(cl);
        cl.clear();

        // 0-0- 1
        cl.push(xs[0]);
        cl.push(xs[2]);
        cl.push(~ys[0]);
        S.addClause(cl);
        cl.clear();

        // --01 1
        cl.push(xs[2]);
        cl.push(~xs[3]);
        cl.push(~ys[0]);
        S.addClause(cl);
        cl.clear();

        // 01-- 1
        cl.push(xs[0]);
        cl.push(~xs[1]);
        cl.push(~ys[0]);
        S.addClause(cl);
        cl.clear();

        // -1-- 1
        cl.push(~xs[1]);
        cl.push(~ys[1]);
        S.addClause(cl);
        cl.clear();

        // ---1 1
        cl.push(~xs[3]);
        cl.push(~ys[1]);
        S.addClause(cl);
        cl.clear();


    }

    template<class Solver>
    void add_xor_ternary(
        Solver& S,
        sat_n::vec_lit_t& xs, 
        sat_n::vec_lit_t& ys
    )
    {
        using namespace sat_n;
        vec_lit_t cl;


        // 1--- 1
        cl.push(~xs[0]);
        cl.push(ys[0]);
        S.addClause(cl);
        cl.clear();

        // --1- 1
        cl.push(~xs[2]);
        cl.push(ys[0]);
        S.addClause(cl);
        cl.clear();

        // -1-0 1
        cl.push(~xs[1]);
        cl.push(xs[3]);
        cl.push(ys[1]);
        S.addClause(cl);
        cl.clear();

        // -0-1 1
        cl.push(xs[1]);
        cl.push(~xs[3]);
        cl.push(ys[1]);
        S.addClause(cl);
        cl.clear();

        // 0-0- 1
        cl.push(xs[0]);
        cl.push(xs[2]);
        cl.push(~ys[0]);
        S.addClause(cl);
        cl.clear();

        // -0-0 1
        cl.push(xs[1]);
        cl.push(xs[3]);
        cl.push(~ys[1]);
        S.addClause(cl);
        cl.clear();

        // -1-1 1
        cl.push(~xs[1]);
        cl.push(~xs[3]);
        cl.push(~ys[1]);
        S.addClause(cl);
        cl.clear();


    }

    template<class Solver>
    void add_xnor_ternary(
        Solver& S,
        sat_n::vec_lit_t& xs, 
        sat_n::vec_lit_t& ys
    )
    {
        using namespace sat_n;
        vec_lit_t cl;


        // 1--- 1
        cl.push(~xs[0]);
        cl.push(ys[0]);
        S.addClause(cl);
        cl.clear();

        // --1- 1
        cl.push(~xs[2]);
        cl.push(ys[0]);
        S.addClause(cl);
        cl.clear();

        // -0-0 1
        cl.push(xs[1]);
        cl.push(xs[3]);
        cl.push(ys[1]);
        S.addClause(cl);
        cl.clear();

        // -1-1 1
        cl.push(~xs[1]);
        cl.push(~xs[3]);
        cl.push(ys[1]);
        S.addClause(cl);
        cl.clear();

        // 0-0- 1
        cl.push(xs[0]);
        cl.push(xs[2]);
        cl.push(~ys[0]);
        S.addClause(cl);
        cl.clear();

        // -1-0 1
        cl.push(~xs[1]);
        cl.push(xs[3]);
        cl.push(~ys[1]);
        S.addClause(cl);
        cl.clear();

        // -0-1 1
        cl.push(xs[1]);
        cl.push(~xs[3]);
        cl.push(~ys[1]);
        S.addClause(cl);
        cl.clear();


    }

    template<class Solver>
    void add_mux_ternary(
        Solver& S,
        sat_n::vec_lit_t& xs, 
        sat_n::vec_lit_t& ys
    )
    {
        using namespace sat_n;
        vec_lit_t cl;

        assert(xs.size() == 6);

        // 1--1-0 1
        cl.push(~xs[0]);
        cl.push(~xs[3]);
        cl.push(xs[5]);
        cl.push(ys[0]);
        S.addClause(cl);
        cl.clear();

        // -01--- 1
        cl.push(xs[1]);
        cl.push(~xs[2]);
        cl.push(ys[0]);
        S.addClause(cl);
        cl.clear();

        // 1--0-1 1
        cl.push(~xs[0]);
        cl.push(xs[3]);
        cl.push(~xs[5]);
        cl.push(ys[0]);
        S.addClause(cl);
        cl.clear();

        // -1--1- 1
        cl.push(~xs[1]);
        cl.push(~xs[4]);
        cl.push(ys[0]);
        S.addClause(cl);
        cl.clear();

        // 1-1--- 1
        cl.push(~xs[0]);
        cl.push(~xs[2]);
        cl.push(ys[0]);
        S.addClause(cl);
        cl.clear();

        // 1---1- 1
        cl.push(~xs[0]);
        cl.push(~xs[4]);
        cl.push(ys[0]);
        S.addClause(cl);
        cl.clear();

        // -0-1-- 1
        cl.push(xs[1]);
        cl.push(~xs[3]);
        cl.push(ys[1]);
        S.addClause(cl);
        cl.clear();

        // -1---1 1
        cl.push(~xs[1]);
        cl.push(~xs[5]);
        cl.push(ys[1]);
        S.addClause(cl);
        cl.clear();

        // --0000 1
        cl.push(xs[2]);
        cl.push(xs[3]);
        cl.push(xs[4]);
        cl.push(xs[5]);
        cl.push(~ys[0]);
        S.addClause(cl);
        cl.clear();

        // 01--0- 1
        cl.push(xs[0]);
        cl.push(~xs[1]);
        cl.push(xs[4]);
        cl.push(~ys[0]);
        S.addClause(cl);
        cl.clear();

        // --0101 1
        cl.push(xs[2]);
        cl.push(~xs[3]);
        cl.push(xs[4]);
        cl.push(~xs[5]);
        cl.push(~ys[0]);
        S.addClause(cl);
        cl.clear();

        // 000--- 1
        cl.push(xs[0]);
        cl.push(xs[1]);
        cl.push(xs[2]);
        cl.push(~ys[0]);
        S.addClause(cl);
        cl.clear();

        // -1---0 1
        cl.push(~xs[1]);
        cl.push(xs[5]);
        cl.push(~ys[1]);
        S.addClause(cl);
        cl.clear();

        // -0-0-- 1
        cl.push(xs[1]);
        cl.push(xs[3]);
        cl.push(~ys[1]);
        S.addClause(cl);
        cl.clear();
    }
    // -- END AUTOGENERATED --
}


#endif
