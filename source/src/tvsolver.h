#ifndef _TVSOLVER_H_DEFINED_
#define _TVSOLVER_H_DEFINED_

#include "ckt.h"
#include "dbl.h"
#include "sat.h"
#include "sim.h"
#include "solver.h"
#include <vector>

class tv_solver_t {
private:
    ckt_n::ckt_t&               ckt;
    ckt_n::simulator_t&         sim;
    ckt_n::dblckt_t             dbl;
    sat_n::Solver               S;
    ckt_n::index2lit_map_t      lit_map;

    void _init_assumps( sat_n::vec_lit_t& assumps, unsigned keyIndex );
    void _init_assumps( sat_n::vec_lit_t& assumps, const std::vector<unsigned>& ks, int vi, int vj); 
    void _extract_inputs(std::vector<bool>& input_values);
    std::pair<int,int> _compare_outputs(std::vector<bool>& output_values);
    static void _set_node_value(ckt_n::dblckt_t& dbl_in, ckt_n::index2lit_map_t& lit_map, ckt_n::node_t* n, int val, sat_n::vec_lit_t& assumps);
    int _isolateKeys(const std::vector<unsigned>& keys, int vi, int vj, keyset_list_t& keysets);
    void _freeze_nodes();
public:
    tv_solver_t(ckt_n::ckt_t& c, ckt_n::simulator_t& s)
        : ckt(c)
        , sim(s)
        , dbl(c, ckt_n::dup_allkeys, true)
    {
        dbl.dbl->split_gates();
        dbl.dbl->init_ternary_solver(S, lit_map);
        _freeze_nodes();
    }
    ~tv_solver_t() {}

    void solveSingleKeys(std::map<std::string, int>& values);
    bool canSolveSingleKeys();
    int countSolveableSingleKeys();
    void qbfSolveSingleKeys(std::vector<std::pair<int, int> >& values);
    void solveDoubleKeys(keyset_list_t& keysets);
    int countNonMutablePairs(std::ostream& out);
    bool isNonMutable(int i, int j);
};
#endif
