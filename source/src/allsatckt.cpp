#include "allsatckt.h"
#include "util.h"
#include <iostream>
#include <iterator>
#include <algorithm>
#include <queue>

namespace AllSAT { 

    AllSATCkt::AllSATCkt()
    {
        MAX_CUBES_PER_LITERAL = 10;
    }

    AllSATCkt::~AllSATCkt()
    {
    }

    void AllSATCkt::addBlockingClause(const sat_n::vec_lit_t& ps)
    {
        S_solve.addClause(ps);
        clist.addClause(ps);
    }

    void AllSATCkt::addClause(const sat_n::vec_lit_t& ps)
    {
        S_solve.addClause(ps);
        S_simp.addClause(ps);
        clist.addClause(ps);
    }

    void AllSATCkt::addClause(sat_n::Lit x)
    {
        S_solve.addClause(x);
        S_simp.addClause(x);
        clist.addClause(x);
    }

    void AllSATCkt::addClause(sat_n::Lit x, sat_n::Lit y)
    {
        S_solve.addClause(x, y);
        S_simp.addClause(x, y);
        clist.addClause(x, y);
    }

    void AllSATCkt::addClause(sat_n::Lit x, sat_n::Lit y, sat_n::Lit z)
    {
        S_solve.addClause(x, y, z);
        S_simp.addClause(x, y, z);
        clist.addClause(x, y, z);
    }

    void AllSATCkt::_resize(sat_n::vec_lit_t& vs, unsigned sz)
    {
        assert(sz >= 0);
        if(vs.size() < sz) {
            vs.growTo(sz);
        } else if(vs.size() > sz) {
            vs.shrink(vs.size() - sz);
        }
    }

    bool AllSATCkt::solveOnce(sat_n::Lit assump)
    {
        return S_solve.solve(assump);
    }

    bool AllSATCkt::solveOnce(sat_n::vec_lit_t& assumps)
    {
        return S_solve.solve(assumps);
    }

    bool AllSATCkt::solve(
        const sat_n::vec_lit_t& assumps_in,
        const sat_n::vec_lit_t& inputs,
        sat_n::Lit output
    )
    {
        _resize(solve_assump, assumps_in.size() + 1);
        for(unsigned i=0; i != assumps_in.size(); i++) {
            solve_assump[i+1] = assumps_in[i];
        }
        solve_assump[0] = output;
        if((verbose&ALLSATCKT_VERBOSE) != 0) {
            ckt_n::dump_clause(std::cout << "solve_assump: ", solve_assump) << std::endl;
        }

        bool result = S_solve.solve(solve_assump);
        if(!result) return result;
        else {
            _init_inputs(inputs, input_values);
            if((verbose&ALLSATCKT_VERBOSE) != 0) {
                ckt_n::dump_clause(std::cout << "minterm: ", input_values) << std::endl;
            }
            _simplify(assumps_in, output);
            return result;
        }
    }

    void AllSATCkt::init_inputs(
        std::vector<sat_n::Lit>& inps1, 
        std::vector<sat_n::Lit>& inps2)
    {
        _resize(input_values, inps1.size() + inps2.size());
        int pos=0;
        for(unsigned i=0; i != inps1.size(); i++) {
            using namespace sat_n;
            lbool ri = S_solve.modelValue(inps1[i]);
            assert(ri.isDef());
            if(ri.getBool()) {
                input_values[pos++] = inps1[i];
            } else {
                input_values[pos++] = ~inps1[i];
            }
        }

        for(unsigned i=0; i != inps2.size(); i++) {
            using namespace sat_n;
            lbool ri = S_solve.modelValue(inps2[i]);
            assert(ri.isDef());
            if(ri.getBool()) {
                input_values[pos++] = inps2[i];
            } else {
                input_values[pos++] = ~inps2[i];
            }
        }
        if((verbose&ALLSATCKT_VERBOSE) != 0) {
            using namespace ckt_n;
            dump_clause(std::cout << "input_values: ", input_values) << std::endl;
        }
        _initCubeAndClause();
    }

    void AllSATCkt::init_inputs( std::vector<sat_n::Lit>& inps1 )
    {
        _resize(input_values, inps1.size());
        int pos=0;
        for(unsigned i=0; i != inps1.size(); i++) {
            using namespace sat_n;
            lbool ri = S_solve.modelValue(inps1[i]);
            assert(ri.isDef());
            if(ri.getBool()) {
                input_values[pos++] = inps1[i];
            } else {
                input_values[pos++] = ~inps1[i];
            }
        }
        assert(pos == (int) inps1.size());
        _initCubeAndClause();
    }

    void AllSATCkt::init_inputs(
        const sat_n::vec_lit_t& inps, 
        const std::vector<bool>& flags)
    {
        unsigned pos=0;
        for(unsigned int i=0; i != inps.size(); i++) {
            using namespace sat_n;
            Var vi = sat_n::var(inps[i]);
            if(flags[vi]) {
                lbool bi = S_solve.modelValue(vi);
                assert(bi.isDef());

                Lit li = bi.getBool() ? mkLit(vi) : ~mkLit(vi);
                if(pos >= input_values.size()) { input_values.push(li); } 
                else { input_values[pos] = li; }
                pos += 1;
            }
        }
        _resize(input_values, pos);
        if((verbose&ALLSATCKT_VERBOSE) != 0) {
            ckt_n::dump_clause(std::cout << "new input_values: ", input_values) << std::endl;
        }
        _initCubeAndClause();
    }

    void AllSATCkt::_init_inputs(
        const sat_n::vec_lit_t& inputs,
        sat_n::vec_lit_t& input_values
    )
    {
        _resize(input_values, inputs.size());
        for(unsigned i=0; i != inputs.size(); i++) {
            using namespace sat_n;

            lbool ri = S_solve.modelValue(inputs[i]);
            assert(ri.isDef());
            if(ri.getBool()) {
                input_values[i] = inputs[i];
            } else {
                input_values[i] = ~inputs[i];
            }
        }
        _initCubeAndClause();
    }

    void AllSATCkt::_simplify(
            const sat_n::vec_lit_t& assump, 
            sat_n::Lit output
    )
    {
        _verify_unsat(assump, output);
        _resize(simp_assump, assump.size() + input_values.size());
        if((verbose&ALLSATCKT_VERBOSE) != 0) {
            ckt_n::dump_clause(std::cout << "assump: ", assump) << std::endl;
            ckt_n::dump_clause(std::cout << "input_values: ", input_values) << std::endl;
            std::cout << "simp_assump.size() = " << simp_assump.size() << std::endl;
        }

        for(unsigned skip=0; skip != input_values.size(); skip++) {
            unsigned pos = 0;
            for(unsigned i=0; i != assump.size(); i++) {
                simp_assump[pos++] = assump[i];
            }
            for(unsigned i=0; i != input_values.size(); i++) {
                if(i != skip) {
                    simp_assump[pos++] = input_values[i];
                }
            }
            simp_assump[pos++] = ~output;
            assert(pos == assump.size() + input_values.size());
            
            if((verbose&ALLSATCKT_VERBOSE) != 0) {
                ckt_n::dump_clause(std::cout << "simp_assump: ", 
                                   simp_assump) << std::endl;
            }

            bool r = S_simp.solve(simp_assump);
            if(r == false) {
                if((verbose&ALLSATCKT_VERBOSE) != 0) std::cout << "unsat" << std::endl;
                using namespace sat_n;
                input_values[skip] = input_values[input_values.size()-1];
                _resize(input_values, input_values.size()-1);
                _initCubeAndClause();
                _simplify(assump, output);
                return;
            } else {
                if((verbose&ALLSATCKT_VERBOSE) != 0) std::cout << "sat" << std::endl;
            }
        }
    }

    void AllSATCkt::_initCubeAndClause()
    {
        _resize(cube, input_values.size());
        _resize(clause, input_values.size());
        for(unsigned i=0; i != cube.size(); i++) {
            cube[i] = input_values[i];
            clause[i] = ~cube[i];
        }
    }

    void AllSATCkt::_verify_unsat(
            const sat_n::vec_lit_t& assump, 
            sat_n::Lit output
    )
    {
#ifdef VERIFY_UNSAT
        _resize(simp_assump, assump.size() + input_values.size() + 1);
        int pos=0;
        for(int i=0; i != assump.size(); i++) {
            simp_assump[pos++] = assump[i];
        }
        for(int i=0; i != input_values.size(); i++) {
            simp_assump[pos++] = input_values[i];
        }
        simp_assump[pos++] = ~output;
        bool s = S_simp.solve(simp_assump);
        if((verbose&ALLSATCKT_VERBOSE) != 0) {
            using namespace ckt_n;
            dump_clause(std::cout << "simp_assump: ", simp_assump) << std::endl;
        }
        if(s) {
            std::cout << "simp_state: "; dump_simp_state(std::cout);
            std::cout << "Error: should have been unsatisfiable." << std::endl;
            exit(1);
        }
#endif
    }

    bool AllSATCkt::solve(
        const sat_n::vec_lit_t& assumps,
        const sat_n::vec_lit_t& inputs,
        const sat_n::vec_lit_t& outputs
    )
    {
        _resize(io_clause, assumps.size() + outputs.size());
        int pos=0;
        for(unsigned i=0; i != assumps.size(); i++) io_clause[pos++] = ~assumps[i];
        for(unsigned i=0; i != outputs.size(); i++) io_clause[pos++] = ~outputs[i];

        if((verbose&ALLSATCKT_VERBOSE) != 0) {
            ckt_n::dump_clause(std::cout << "io_clause: ", io_clause) << std::endl;
        }
        S_solve.addClause(io_clause);

        //S_solve.conflicts = S_solve.propagations = 0;
        bool result = S_solve.solve(assumps);
        //solve_conf_hist.incr(S_solve.conflicts);
        //solve_prop_hist.incr(S_solve.propagations);

        if(!result) return result;
        else {
            _init_inputs(inputs, input_values);
            if((verbose&ALLSATCKT_VERBOSE) != 0) {
                ckt_n::dump_clause(std::cout << "minterm: ", input_values) << std::endl;
            }
            _simplify(assumps, outputs);
            return result;
        }
    }

    void AllSATCkt::_simplify(
            const sat_n::vec_lit_t& assumps, 
            const sat_n::vec_lit_t& outputs
    )
    {
        _verify_unsat(assumps, outputs);
        _resize(
            simp_assump, 
            assumps.size() + input_values.size() + outputs.size() - 1);

        if((verbose&ALLSATCKT_VERBOSE) != 0) {
            ckt_n::dump_clause(std::cout << "assumps: ", assumps) << std::endl;
            ckt_n::dump_clause(std::cout << "input_values: ", input_values) << std::endl;
        }

        for(unsigned skip=0; skip != input_values.size(); skip++) {
            unsigned pos = 0;
            for(unsigned i=0; i != assumps.size(); i++) {
                simp_assump[pos++] = assumps[i];
            }
            for(unsigned i=0; i != input_values.size(); i++) {
                if(i != skip) {
                    simp_assump[pos++] = input_values[i];
                }
            }
            for(unsigned i=0; i != outputs.size(); i++) {
                simp_assump[pos++] = outputs[i];
            }
            assert(pos == 
                assumps.size() + input_values.size() + outputs.size() - 1);
            if((verbose&ALLSATCKT_VERBOSE) != 0) {
                ckt_n::dump_clause(std::cout << "simp_assump: ", 
                                   simp_assump) << std::endl;
            }

            //S_simp.conflicts = S_simp.propagations = 0;
            bool r = S_simp.solve(simp_assump);
            //simp_conf_hist.incr(S_simp.conflicts);
            //simp_prop_hist.incr(S_simp.propagations);

            if(r == false) {
                if((verbose&ALLSATCKT_VERBOSE) != 0) std::cout << "unsat" << std::endl;
                using namespace sat_n;
                input_values[skip] = input_values[input_values.size()-1];
                _resize(input_values, input_values.size()-1);
                _initCubeAndClause();
                _simplify(assumps, outputs);
                return;
            } else {
                if((verbose&ALLSATCKT_VERBOSE) != 0) std::cout << "sat" << std::endl;
            }
        }
    }
    void AllSATCkt::_verify_unsat(
            const sat_n::vec_lit_t& assumps, 
            const sat_n::vec_lit_t& outputs
    )
    {
#ifdef VERIFY_UNSAT
        _resize(simp_assump, 
            assumps.size() + input_values.size() + outputs.size());

        int pos=0;
        for(int i=0; i != assumps.size(); i++) {
            simp_assump[pos++] = assumps[i];
        }
        for(int i=0; i != input_values.size(); i++) {
            simp_assump[pos++] = input_values[i];
        }
        for(int i=0; i != outputs.size(); i++) {
            simp_assump[pos++] = outputs[i];
        }
        assert(pos == assumps.size() + input_values.size() + outputs.size());
        bool s = S_simp.solve(simp_assump);
        if(s) {
            std::cout << "Error: should have been unsatisfiable." << std::endl;
            exit(1);
        }
#endif
    }

    void AllSATCkt::_dump_state(sat_n::Solver& S, std::ostream& out)
    {
        for(int i=0; i != nVars(); i++) {
            using namespace sat_n;

            lbool vi = S.modelValue(i);
            assert(vi.isDef());
            if(!vi.getBool()) { std::cout << "-"; }
            else { std::cout << "+"; }
            out << i << " ";
        }
        out << std::endl;
    }

    void AllSATCkt::_deleteMemo(cube_memo_t& memo)
    {
        for(cube_memo_t::iterator it = memo.begin(); it != memo.end(); it++) {
            bddset_t* ptr = it->second;
            assert(ptr != NULL);
            delete ptr;
        }
        memo.clear();
    }

    AllSATCkt::bddset_t* AllSATCkt::_findCube(
        impl_graph_t& g,
        cube_memo_t& memo,
        const std::vector<bool> inputs,
        sat_n::Lit l, 
        std::set<int> visited)
    {
        assert(visited.find(sat_n::var(l)) == visited.end());

        if((verbose&FINDCUBE_VERBOSE) != 0) {
            using namespace ckt_n;
            std::cout << "_findCube called for: " << l << std::endl;
        }

        // see if the result is already in the memo.
        cube_memo_t::iterator pos = memo.find(l);
        if(pos != memo.end()) {
            bddset_t* cubes = pos->second;
            // FIXME: might need debug output here.
            assert(cubes != NULL);
            return cubes;
        }

        using namespace sat_n;
        assert(solverModelValue(l).isDef() && solverModelValue(l).getBool());

        // if it's an input just return it.
        int v_l = var(l);
        if(inputs[v_l]) {
            BDD b_l = mgr.bddVar(v_l);
            if(sign(l)) {
                b_l = !b_l;
            }
            bddset_t* set = new bddset_t();
            set->insert(b_l);

            if((verbose&FINDCUBE_VERBOSE) != 0) {
                using namespace ckt_n;
                std::cout << "_findCube base-case result [" << l << "]: ";
                dump_clause(std::cout, b_l) << std::endl;
            }
            memo[l] = set;
            return set;
        }

        // now we need to trawl through the implication graph.
        visited.insert(var(l));

        assert((int)g.size() == nVars());
        assert(g[var(l)].size() > 0);

        bddset_t* set = new bddset_t();
        for(unsigned i=0; i != g[var(l)].size(); i++) {
            int clauseIndex = g[var(l)][i];
            int clauseSize = clist.clauseLen(clauseIndex);
            assert(clauseSize > 0);

            bool goodClause = true;
            for(int j=0; j != clauseSize; j++) {
                Lit x = clist.clauseLit(clauseIndex, j);
                if(x != l) {
                    assert(solverModelValue(x).isDef() && !solverModelValue(x).getBool());
                    if(visited.find(var(x)) != visited.end()) {
                        goodClause = false;
                        break;
                    }
                }
            }
            if(goodClause) {
                bddset_t temp_set;
                temp_set.insert(mgr.bddOne());
                for(int j=0; j != clauseSize; j++) {
                    Lit x = clist.clauseLit(clauseIndex, j);
                    if(x != l) {
                        assert(solverModelValue(x).isDef() && !solverModelValue(x).getBool());
                        bddset_t* resultSet = _findCube(g, memo, inputs, ~x, visited);
                        if(resultSet == NULL) {
                            goodClause = false;
                            break;
                        } else {
                            _cartesianProduct(temp_set, *resultSet);
                        }
                    }
                }
                if(!goodClause) {
                    continue;
                } else {
                    std::copy(temp_set.begin(), temp_set.end(), 
                              std::inserter(*set, set->begin()));
                    if(set->size() >= MAX_CUBES_PER_LITERAL) {
                        break;
                    }
                }
            }
        }
        visited.erase(var(l));
        if(set->size() == 0) {
            delete set;
            return NULL;
        } else {
            memo[l] = set;
            return set;
        }
    }

    void AllSATCkt::_buildImplGraph( 
        impl_graph_t& g, 
        const std::vector<bool> inputs,
        sat_n::Lit output )
    {
        assert((int)g.size() == nVars());
        assert((int)inputs.size() == nVars());

        std::vector<bool> visited(nVars(), false);
        std::queue< sat_n::Lit > bfsq;
        bfsq.push(output);

        while(!bfsq.empty()) {
            using namespace sat_n;

            Lit y = bfsq.front();
            bfsq.pop();

            assert(solverModelValue(y).isDef() && solverModelValue(y).getBool());
            if(visited[var(y)]) continue;
            if(inputs[var(y)]) continue;

            visited[var(y)] = true;
            int nClausesWithLit = clist.numClausesWithLit(y);
            for(int j=0; j != nClausesWithLit; j++) {
                int clauseIndex = clist.clauseWithLit(y, j);
                int clauseSize = clist.clauseLen(clauseIndex);
                bool implied = true;
                for(int i=0; i != clauseSize; i++) {
                    Lit x = clist.clauseLit(clauseIndex, i);
                    if( x != y && solverModelValue(x).getBool() )
                    {
                        implied = false;
                        break;
                    }
                }

                // so now we have an implied clause.
                if(clauseSize > 1 && implied) {
                    g[var(y)].push_back(clauseIndex);
                    for(int i=0; i != clauseSize; i++) {
                        Lit x = clist.clauseLit(clauseIndex, i);
                        if( x != y ) {
                            assert(solverModelValue(x).isDef() && !solverModelValue(x).getBool());
                            bfsq.push(~x);
                        }
                    }
                }
            }
        }
    }

    void AllSATCkt::_cartesianProduct(bddset_t& a, bddset_t& b)
    {
        bddset_t result;
        for(bddset_t::iterator it = a.begin(); it != a.end(); it++) {
            BDD a_i = *it;
            for(bddset_t::iterator jt = b.begin(); jt != b.end(); jt++) {
                BDD b_j = *jt;
                BDD ab_ij = a_i & b_j;
                result.insert(ab_ij);
            }
        }
        a.clear();
        std::copy(result.begin(), result.end(), std::inserter(a, a.begin()));
    }

    int AllSATCkt::findBackbones(
        std::vector<bool>& fixedvars,
        const std::vector<sat_n::Lit>& vars,
        const std::vector<sat_n::Lit>& assumps,
        // output is assumed to be the correct output (i.e. not the cex).
        sat_n::Lit output)
    {
        int cnt=0;

        using namespace sat_n;
        vec_lit_t bb_assumps(assumps.size() + 2);
        for(unsigned i=0; i != assumps.size(); i++) {
            bb_assumps[i+2] = assumps[i];
        }

        assert(fixedvars.size() == vars.size());
        for(unsigned i=0; i != vars.size(); i++) {
            if(fixedvars[i]) continue;

            Lit x_i = vars[i];
            // case 1;
            bb_assumps[0] = x_i;
            bb_assumps[1] = output;
            if(S_solve.solve(bb_assumps) == false) {
                // this means F.x_i => ~output
                // so block x_i
                S_solve.addClause(x_i);
                fixedvars[i] = true;
                cnt += 1;
                continue;
            }
            // case 2;
            bb_assumps[0] = ~x_i;
            bb_assumps[1] = output;
            if(S_solve.solve(bb_assumps) == false) {
                // this means F.~x_i => ~output
                // so block ~x_i
                S_solve.addClause(~x_i);
                fixedvars[i] = true;
                cnt += 1;
                continue;
            }
        }
        return cnt;
    }
    void AllSATCkt::initVector(
        const std::vector<sat_n::Lit>& literals,
        std::vector<sat_n::Lit>& values
    )
    {
        using namespace sat_n;

        for(unsigned i=0; i != literals.size(); i++) {
            lbool v_i = solverModelValue(literals[i]);
            assert(v_i.isDef());
            values[i] = v_i.getBool()  ?  literals[i] : ~literals[i];
        }
    }
}
