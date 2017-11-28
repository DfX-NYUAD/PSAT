#ifndef _SATINTERFACE_H_DEFINED_
#define _SATINTERFACE_H_DEFINED_

#include <stdint.h>
#include <lglib.h>
#include <cmsat/Solver.h>

namespace sat_n
{
//#define CMSAT
#ifdef CMSAT
    typedef CMSat::Var Var;
    typedef CMSat::Lit Lit;
    typedef CMSat::lbool lbool;
    typedef CMSat::vec<Lit> vec_lit_t;
    const CMSat::lbool l_True = CMSat::l_True;
    const CMSat::lbool l_False = CMSat::l_False;
    const CMSat::lbool l_Undef = CMSat::l_Undef;

    inline Lit mkLit(Var vi) { 
        return CMSat::Lit(vi, false);
    }

    inline Var var(Lit l) {
        return l.var();
    }

    inline bool sign(Lit l) {
        return l.sign();
    }

    inline uint32_t toInt(Lit l) {
        return l.toInt();
    }

    //stupid sentinel function, I added to check if lingeling was being compiled.
    //inline void dostuff() {
    //    LGL* lgl = lglinit();
    //    lglrelease(lgl);
    //}

    class Solver {
        CMSat::Solver S;

    public:
        Solver() {}
        ~Solver() {}

        Var newVar() { return S.newVar(); }
        int nVars() const { return S.nVars(); }
        int nClauses() const { return S.nClauses(); }

        // freeze a single literal.
        void freeze(Lit l) { }
        // freeze a vector of literals.
        void freeze(const std::vector<Lit>& ys) { }
        
        bool addClause(const vec_lit_t& ps) {
            vec_lit_t ps_copy(ps);
            return S.addClause(ps_copy);
        }
        bool addClause(Lit x) { 
            vec_lit_t vs;
            vs.push(x);
            return S.addClause(vs); 
        }
        bool addClause(Lit x, Lit y) { 
            vec_lit_t vs;
            vs.push(x);
            vs.push(y);
            return S.addClause(vs);
        }
        bool addClause(Lit x, Lit y, Lit z) {
            vec_lit_t vs;
            vs.push(x);
            vs.push(y);
            vs.push(z);
            return S.addClause(vs);
        }

        lbool modelValue(Lit x) const {
            return S.modelValue(x);
        }
        lbool modelValue(Var x) const {
            return S.modelValue(mkLit(x));
        }
        bool solve() { 
            lbool result = S.solve(); 
            if(result.isDef()) {
                return result.getBool();
            } else {
                assert(false);
                return false;
            }
        }
        bool solve(const vec_lit_t& assump) { 
            lbool result = S.solve(assump); 
            if(result.isDef()) {
                return result.getBool();
            } else {
                assert(false);
                return false;
            }
        }
        bool solve(Lit x) { 
            vec_lit_t assump;
            assump.push(x);
            lbool result = S.solve(assump); 
            if(result.isDef()) {
                return result.getBool();
            } else {
                assert(false);
                return false;
            }
        }
        void writeCNF(const std::string& filename) {
            S.dumpOrigClauses(filename);
        }
        int64_t getNumDecisions() const {
            return S.getNumDecisions();
        }
    };
#else

    typedef CMSat::Var Var;
    typedef CMSat::Lit Lit;
    typedef CMSat::vec<Lit> vec_lit_t;

    // homemade lbool.
    struct lbool {
        enum x_t { p_True = 1, p_Undef = 0, p_False = -1 } x;
        lbool(int val) : x((x_t)val) {
            assert(x == p_True || x == p_Undef || x == p_False);
        }

        inline bool isDef() { 
            return x != p_Undef;
        }
        inline bool isUndef() {
            return x == p_Undef;
        }

        inline bool getBool() {
            assert(isDef());
            return (x == p_True);
        }

        bool operator==(const lbool& other) const {
            return x == other.x;
        }
        bool operator!=(const lbool& other) const {
            return x != other.x;
        }
    };

    extern const lbool l_True;
    extern const lbool l_False;
    extern const lbool l_Undef;

    inline Lit mkLit(Var vi) { 
        return CMSat::Lit(vi, false);
    }

    inline Var var(Lit l) {
        return l.var();
    }

    inline bool sign(Lit l) {
        return l.sign();
    }

    inline uint32_t toInt(Lit l) {
        return l.toInt();
    }

    class Solver {
        LGL* solver;
        uint32_t numVars;

        static int translate(Lit l) {
            int v = (var(l) + 1);
            return sign(l) ? -v : v;
        }
        static int translate(Var v) {
            assert(v >= 0);
            return v+1;
        }
        bool _solve() { 
            lglsetopt(solver, "dlim", -1);
            int result = lglsat(solver);
            if(result == LGL_SATISFIABLE) {
                return true;
            } else if(result == LGL_UNSATISFIABLE) {
                return false;
            } else {
                assert(false);
                return false;
            }
        }

    public:
        // Constructor.
        Solver() { 
            solver = lglinit(); 
            numVars = 0; 
        }
        // Destrucutor.
        ~Solver() { 
            lglrelease(solver); 
        }

        // Create a new variable.
        Var newVar() { 
            // No need to create vars in lgl, just do our own bookkeeping.
            Var v = numVars;
            numVars += 1;
            return v;
        }
        // Number of variables created so far.
        int nVars() const { return numVars; }
        // Number of clauses added so far.
        int nClauses() const { return lglnclauses(solver); }

        // freeze a single literal.
        void freeze(Lit l) {
            lglfreeze(solver, translate(l));
        }

        // freeze a vector of literals.
        void freeze(const std::vector<Lit>& ys) {
            for(unsigned i=0; i != ys.size(); i++) {
                lglfreeze(solver, translate(ys[i]));
            }
        }

        // Add a vector of literals as a clause.
        bool addClause(const vec_lit_t& ps) {
            for(unsigned i=0; i != ps.size(); i++) {
                lgladd(solver, translate(ps[i]));
            }
            lgladd(solver, 0);
            return true;
        }

        // Add a single literal as a clause.
        bool addClause(Lit x) { 
            lgladd(solver, translate(x));
            lgladd(solver, 0);
            return true;
        }

        // Add a clause with two literals.
        bool addClause(Lit x, Lit y) { 
            lgladd(solver, translate(x));
            lgladd(solver, translate(y));
            lgladd(solver, 0);
            return true;
        }

        // Add a clause with three literals.
        bool addClause(Lit x, Lit y, Lit z) {
            lgladd(solver, translate(x));
            lgladd(solver, translate(y));
            lgladd(solver, translate(z));
            lgladd(solver, 0);
            return true;
        }

        // Return the value of a literal.
        lbool modelValue(Lit x) const {
            int v = lglderef(solver, translate(x));
            if(v == 1) return l_True;
            else if(v == -1) return l_False;
            else return l_Undef;
        }

        // Return the value of a variable.
        lbool modelValue(Var x) const {
            int v = lglderef(solver, translate(x));
            if(v == 1) return l_True;
            else if(v == -1) return l_False;
            else return l_Undef;
        }

        // Solve without any assumptions.
        bool solve() { 
            return _solve();
        }

        bool solve(const vec_lit_t& assump) { 
            for(unsigned i=0; i != assump.size(); i++) {
                lglassume(solver, translate(assump[i]));
            }
            return _solve();
        }
        bool solve(Lit x) { 
            lglassume(solver, translate(x));
            return _solve();
        }

        void writeCNF(const std::string& filename) {
            // FIXME.
        }
        int64_t getNumDecisions() const {
            return lglgetdecs(solver);
        }
    };
#endif
}

#endif
