#ifndef _SOLVER_H_DEFINED_
#define _SOLVER_H_DEFINED_

#include <vector>
#include "ckt.h"
#include "sim.h"
#include "dbl.h"
#include "sat.h"
#include "sim.h"
#include <sys/time.h>
#include <sys/resource.h>
#include <list>

struct keyset_value_t
{
    std::vector<unsigned> keys;
    std::vector<int> values;

    keyset_value_t(const std::vector<unsigned>& ks, int vs)
        : keys(ks)
    {
        assert(keys.size() < 32);
        for(unsigned i=0; i != keys.size(); i++) {
            int v = (vs>>i)&1;
            values.push_back(v);
        }
    }
};

typedef std::list<keyset_value_t> keyset_list_t;

class solver_t {
public:
    // types
    enum solver_version_t { SOLVER_V0=0, SOLVER_V1=1, SOLVER_V2=2, SOLVER_V4=4 /* v2 without cube finding. */ };
    static bool is_valid_version(int ver) {
        return SOLVER_V0 == ver; 
    }
    typedef std::map<std::string, int> rmap_t;

    struct slice_t {
        ckt_n::ckt_t& ckt;
        ckt_n::ckt_t& sim;

        std::vector<int> outputs;
        std::vector<int> keys;

        AllSAT::ClauseList cl;
        ckt_n::ckt_t* cktslice;
        ckt_n::ckt_t* simslice;
        ckt_n::ckt_t::node_map_t ckt_nmfwd, ckt_nmrev;
        ckt_n::ckt_t::node_map_t sim_nmfwd, sim_nmrev;

        std::map<int,int> cktm_kfwd, cktm_krev;
        std::map<int,int> cktm_cfwd, cktm_crev;

        slice_t(ckt_n::ckt_t& cktin, ckt_n::ckt_t& simin) 
            : ckt(cktin), sim(simin) 
            , cktslice(NULL), simslice(NULL)
        {}
        ~slice_t() {
            if(cktslice) delete cktslice;
            if(simslice) delete simslice;
        }

        void createCkts();

        ckt_n::node_t* getSliceKey(int i) { 
            assert(i >= 0 && i < (int) cktslice->num_key_inputs());
            return cktslice->key_inputs[i];
        }
        ckt_n::node_t* getSliceInp(int i) {
            assert(i >= 0 && i < (int) cktslice->num_ckt_inputs());
            return cktslice->ckt_inputs[i];
        }

        int mapKeyIndexFwd(int i) {
            assert(i >= 0 && i < (int) ckt.num_key_inputs());
            auto pos = cktm_kfwd.find(i);
            if(pos != cktm_kfwd.end()) { return pos->second; }
            else { return -1; }
        }

        int mapCktIndexFwd(int i) {
            assert(i >= 0 && i < (int) cktslice->num_ckt_inputs());
            auto pos = cktm_cfwd.find(i);
            if(pos != cktm_cfwd.end()) { return pos->second; }
            else { return -1; }
        }

        int mapKeyIndexRev(int i) {
            assert(i >= 0 && i < (int) cktslice->num_key_inputs());
            auto pos = cktm_krev.find(i);
            if(pos != cktm_krev.end()) { return pos->second; }
            else { assert(false); return -1; }
        }

        int mapCktIndexRev(int i) {
            assert(i >= 0 && i < (int) cktslice->num_ckt_inputs());
            auto pos = cktm_crev.find(i);
            if(pos != cktm_crev.end()) { return pos->second; }
            else { assert(false); return -1; }
        }
    };

    struct iovalue_t {
        std::vector<bool> inputs;
        std::vector<bool> outputs;
    };
    typedef std::vector<iovalue_t> iovalue_vector_t;

private:
    ckt_n::ckt_t& ckt;
    ckt_n::ckt_t& simckt;
    ckt_n::ckt_eval_t sim;
    ckt_n::dblckt_t dbl;

    int MAX_VERIF_ITER;
    sat_n::Solver S;              // "doubled-ckt" solver.
    AllSAT::ClauseList cl;        // solver with the initial list of clauses.

    ckt_n::index2lit_map_t lmap;  // map for S

    // dbl_keyinput_flags[var(l)] tells us whether l is a key input
    std::vector<bool>           dbl_keyinput_flags;

    std::map<sat_n::Var, std::pair<sat_n::Lit, sat_n::Lit> > keyinput_vars;

    sat_n::Lit l_out; // output literal of dbl.
    std::vector<sat_n::Lit> keyinput_literals_A, keyinput_literals_B;
    std::vector<sat_n::Lit> cktinput_literals;
    std::vector<sat_n::Lit> output_literals_A, output_literals_B;

    std::vector<bool> input_values;
    std::vector<bool> output_values;
    std::vector<bool> fixed_keys;
    iovalue_vector_t iovectors;

    // methods.
    void _sanity_check_model();
    bool _verify_solution_sim(std::map< std::string, int >& keysFound);
    bool _verify_solution_sat();

    // Evaluates the output for the values stored in input_values and then
    // records this in the solver.
    void _record_input_values();
    void _record_sim(
        const std::vector<bool>& input_values, 
        const std::vector<bool>& output_values, 
        std::vector<sat_n::lbool>& values
    );

    bool _solve_v0(rmap_t& keysFound, bool quiet, int dlimFactor);
    void _testBackbones(
        const std::vector<bool>& inputs, 
        sat_n::Solver& S, ckt_n::index2lit_map_t& lmap,
        std::map<int, int>& backbones);
    void _extractSolution(rmap_t& keysFound);
public:
    // flags and limits.
    int verbose;
    double time_limit;
    struct rusage ru_start;
    // counters.
    volatile int iter;
    volatile int backbones_count;
    volatile int cube_count;


    solver_t(ckt_n::ckt_t& ckt, ckt_n::ckt_t& sim, int verbose);
    ~solver_t();

    // find slices to cut the circuit into.
    static void sliceAndDice(
        ckt_n::ckt_t& ckt, 
        ckt_n::ckt_t& sim, 
        std::vector<slice_t*>& slices
    );

    void addKnownKeys(std::vector<std::pair<int, int> >& values);
    bool solve(solver_version_t ver, rmap_t& keysFoundMap, bool quiet);
    void blockKey(rmap_t& keysFoundMap);
    bool getNewKey(rmap_t& keysFoundMap);
    void findFixedKeys(std::map<int, int>& backbones);

    static void solveSlice(
        slice_t& slice, 
        std::map<int, int>& fixedKeys,
        rmap_t& allKeys 
    );
    int sliceAndSolve( rmap_t& keysFoundMap, int maxKeys, int maxNodes );
};

#endif
