#include <iterator>
#include "tvsolver.h"
#include "util.h"
#include "CEGSolver.h"

void tv_solver_t::_freeze_nodes()
{
    using namespace ckt_n;
    using namespace sat_n;

    for(unsigned i=0; i != dbl.dbl->num_inputs(); i++) {
        node_t* ni = dbl.dbl->inputs[i];
        Lit l0 = dbl.dbl->getTLit0(lit_map, ni);
        Lit l1 = dbl.dbl->getTLit1(lit_map, ni);
        S.freeze(l0);
        S.freeze(l1);
    }

    for(unsigned i=0; i != ckt.num_outputs(); i++) {
        node_t* niA = dbl.getA(ckt.outputs[i]);
        node_t* niB = dbl.getB(ckt.outputs[i]);

        // if the two nodes are the same (happens when they are wired directly
        // to an input) we just ignore this output and move on.
        if(niA == niB) {
            Lit l0 = dbl.dbl->getTLit0(lit_map, niA);
            Lit l1 = dbl.dbl->getTLit1(lit_map, niA);
            S.freeze(l0);
            S.freeze(l1);
        } else {
            Lit lA0 = dbl.dbl->getTLit0(lit_map, niA);
            Lit lA1 = dbl.dbl->getTLit1(lit_map, niA);

            Lit lB0 = dbl.dbl->getTLit0(lit_map, niB);
            Lit lB1 = dbl.dbl->getTLit1(lit_map, niB);

            S.freeze(lA0); S.freeze(lA1);
            S.freeze(lB0); S.freeze(lB1);
        }
    }

    assert(dbl.dbl->num_outputs() == 1);
    Lit l0 = dbl.dbl->getTLit0(lit_map, dbl.dbl->outputs[0]);
    Lit l1 = dbl.dbl->getTLit1(lit_map, dbl.dbl->outputs[0]);
    S.freeze(l0);
    S.freeze(l1);
}

void tv_solver_t::_set_node_value(ckt_n::dblckt_t& dbl_in, ckt_n::index2lit_map_t& lit_map_in, ckt_n::node_t* n, int val, sat_n::vec_lit_t& assumps)
{
    using namespace sat_n;
    Lit l0 = dbl_in.dbl->getTLit0(lit_map_in, n);
    Lit l1 = dbl_in.dbl->getTLit1(lit_map_in, n);
    if(val == 0) {
        assumps.push(~l0);
        assumps.push(~l1);
    } else if(val == 1) {
        assumps.push(~l0);
        assumps.push(l1);
    } else {
        assert(val >= 2 && val <= 3);
        assumps.push(l0);
    }
}

void tv_solver_t::_init_assumps( sat_n::vec_lit_t& assumps, unsigned keyIndex )
{
    using namespace sat_n;
    using namespace ckt_n;

    for(unsigned i=0; i != ckt.num_key_inputs(); i++) {
        node_t* ni = ckt.key_inputs[i];
        node_t* niA = dbl.getA(ni);
        node_t* niB = dbl.getB(ni);

        if(i == keyIndex) {
            _set_node_value(dbl, lit_map, niA, 0, assumps);
            _set_node_value(dbl, lit_map, niB, 1, assumps);
        } else {
            _set_node_value(dbl, lit_map, niA, 2, assumps);
            _set_node_value(dbl, lit_map, niB, 2, assumps);
        }
    }
}

void tv_solver_t::_init_assumps( sat_n::vec_lit_t& assumps, 
    const std::vector<unsigned>& ks, int vi, int vj)
{
    using namespace sat_n;
    using namespace ckt_n;

    for(unsigned i=0; i != ckt.num_key_inputs(); i++) {
        node_t* ni = ckt.key_inputs[i];
        node_t* niA = dbl.getA(ni);
        node_t* niB = dbl.getB(ni);

        bool found = false;
        for(unsigned j=0; j != ks.size(); j++) {
            if(i == ks[j]) {
                _set_node_value(dbl, lit_map, niA, ((vi>>j)&1), assumps);
                _set_node_value(dbl, lit_map, niB, ((vj>>j)&1), assumps);
                found = true;
            }
        }
        if(!found) {
            _set_node_value(dbl, lit_map, niA, 2, assumps);
            _set_node_value(dbl, lit_map, niB, 2, assumps);
        }
    }
}

void tv_solver_t::_extract_inputs(std::vector<bool>& input_values)
{
    using namespace sat_n;
    using namespace ckt_n;

    for(unsigned i=0; i != ckt.num_ckt_inputs(); i++) {
        node_t* ni_ckt = ckt.ckt_inputs[i];
        node_t* ni_dbl = dbl.getA(ni_ckt);
        assert(ni_dbl == dbl.getB(ni_ckt));

        Lit l0 = dbl.dbl->getTLit0(lit_map, ni_dbl);
        Lit l1 = dbl.dbl->getTLit1(lit_map, ni_dbl);

        lbool v0 = S.modelValue(l0);
        lbool v1 = S.modelValue(l1);

        assert(v0.isDef());
        assert(v1.isDef());
        input_values[i] = (v1.getBool());
    }
}

std::pair<int,int> tv_solver_t::_compare_outputs(std::vector<bool>& output_values)
{
    using namespace sat_n;
    using namespace ckt_n;

    int a_correct = 1, b_correct = 1;

    if(verbose) std::cout << "solver outputs = ";
    for(unsigned i=0; i != ckt.num_outputs(); i++) {
        node_t* niA = dbl.getA(ckt.outputs[i]);
        node_t* niB = dbl.getB(ckt.outputs[i]);
        // if the two nodes are the same (happens when they are wired directly
        // to an input) we just ignore this output and move on.
        if(niA == niB) {
            continue;
        }

        Lit lA0 = dbl.dbl->getTLit0(lit_map, niA);
        Lit lA1 = dbl.dbl->getTLit1(lit_map, niA);

        Lit lB0 = dbl.dbl->getTLit0(lit_map, niB);
        Lit lB1 = dbl.dbl->getTLit1(lit_map, niB);

        lbool vA0 = S.modelValue(lA0);
        lbool vA1 = S.modelValue(lA1);

        lbool vB0 = S.modelValue(lB0);
        lbool vB1 = S.modelValue(lB1);

        assert(vA0.isDef());
        assert(vA1.isDef());
        assert(vB0.isDef());
        assert(vB1.isDef());

        if(verbose) {
            if(vA0.getBool()) std::cout << "X";
            else std::cout << (int) (vA1.getBool());

            if(vB0.getBool()) std::cout << "X";
            else std::cout << (int) (vB1.getBool());

            std::cout << " ";
        }

        if(!vA0.getBool()) {
            bool oa = (vA1.getBool());
            if(oa != output_values[i]) a_correct = 0;
        }
        if(!vB0.getBool()) {
            bool ob = (vB1.getBool());
            if(ob != output_values[i]) b_correct = 0;
        }
    }
    if(verbose) std::cout << std::endl;
    if(verbose) {
        std::cout << "a = " << a_correct << std::endl;
        std::cout << "b = " << b_correct << std::endl;
    }
    return std::pair<int,int>(a_correct, b_correct);
}

void tv_solver_t::solveSingleKeys(std::map<std::string, int>& values)
{
    using namespace sat_n;
    using namespace ckt_n;

    assert(dbl.dbl->num_outputs() == 1);
    node_t* nout = dbl.dbl->outputs[0];
    Lit lout0 = dbl.dbl->getTLit0(lit_map, nout);
    Lit lout1 = dbl.dbl->getTLit1(lit_map, nout);

    vec_lit_t assumps;
    int cnt = 0;
    std::vector<bool> input_values(ckt.num_ckt_inputs());
    std::vector<bool> output_values(ckt.num_outputs());

    for(unsigned i=0; i != ckt.num_key_inputs(); i++) {
        assumps.clear();
        assumps.push(~lout0);
        assumps.push(lout1);
            
        _init_assumps(assumps, i);
        if(S.solve(assumps) == true) {
            using namespace ckt_n;

            if(verbose) {
                std::cout << "isolated key input: " << i << std::endl;
            }
            cnt += 1;

            _extract_inputs(input_values);
            sim.eval(input_values, output_values);
            if(verbose) {
                std::cout << "input values  = " << input_values << std::endl;
                std::cout << "output values = " << output_values << std::endl;
            }
            std::pair<int,int> p = _compare_outputs(output_values);
            assert(p.first ^ p.second);
            if(p.first == 1 && p.second == 0) {
                values[ckt.key_inputs[i]->name] = 0;
            } else if(p.first == 0 && p.second == 1) {
                values[ckt.key_inputs[i]->name] = 1;
            }
        }
    }
    if(verbose) {
        std::cout << "isolated_cnt = " << cnt << std::endl;
    }
}

int tv_solver_t::countSolveableSingleKeys()
{
    using namespace sat_n;
    using namespace ckt_n;

    assert(dbl.dbl->num_outputs() == 1);
    node_t* nout = dbl.dbl->outputs[0];
    Lit lout0 = dbl.dbl->getTLit0(lit_map, nout);
    Lit lout1 = dbl.dbl->getTLit1(lit_map, nout);

    vec_lit_t assumps;
    int cnt = 0;
    for(unsigned i=0; i != ckt.num_key_inputs(); i++) {
        assumps.clear();
        assumps.push(~lout0);
        assumps.push(lout1);
            
        _init_assumps(assumps, i);
        if(S.solve(assumps) == true) {
            cnt += 1;
        }
    }
    return cnt;
}

bool tv_solver_t::canSolveSingleKeys()
{
    using namespace sat_n;
    using namespace ckt_n;

    assert(dbl.dbl->num_outputs() == 1);
    node_t* nout = dbl.dbl->outputs[0];
    Lit lout0 = dbl.dbl->getTLit0(lit_map, nout);
    Lit lout1 = dbl.dbl->getTLit1(lit_map, nout);

    vec_lit_t assumps;
    for(unsigned i=0; i != ckt.num_key_inputs(); i++) {
        assumps.clear();
        assumps.push(~lout0);
        assumps.push(lout1);
            
        _init_assumps(assumps, i);
        if(S.solve(assumps) == true) {
            return true;
        }
    }
    return false;
}

bool tv_solver_t::isNonMutable(int i, int j)
{
    using namespace ckt_n;
    using namespace sat_n;

    node_t* ki = ckt.key_inputs[i];
    node_t* kj = ckt.key_inputs[j];

    dup_twoKeys_t dup(ki, kj);
    dblckt_t dbl(ckt, dup, true);

    // if(verbose) { std::cout << *dbl.dbl << std::endl; }

    node_t* kiA = dbl.getA(ki);
    node_t* kiB = dbl.getB(ki);
    node_t* kjA = dbl.getA(kj);
    node_t* kjB = dbl.getB(kj);

    index2lit_map_t lit_map;
    sat_n::Solver S;
    dbl.dbl->split_gates();
    dbl.dbl->init_ternary_solver(S, lit_map);

    node_t* nout = dbl.dbl->outputs[0];
    Lit lout0 = dbl.dbl->getTLit0(lit_map, nout);
    Lit lout1 = dbl.dbl->getTLit1(lit_map, nout);

    vec_lit_t assumps;

    assumps.clear();
    assumps.push(~lout0);
    assumps.push(lout1);
    _set_node_value(dbl, lit_map, kiA, 0, assumps);
    _set_node_value(dbl, lit_map, kiB, 1, assumps);
    _set_node_value(dbl, lit_map, kjA, 2, assumps);
    _set_node_value(dbl, lit_map, kjB, 2, assumps);
    if(S.solve(assumps) == true) {
        return false;
    }

    assumps.clear();
    assumps.push(~lout0);
    assumps.push(lout1);
    _set_node_value(dbl, lit_map, kiA, 2, assumps);
    _set_node_value(dbl, lit_map, kiB, 2, assumps);
    _set_node_value(dbl, lit_map, kjA, 0, assumps);
    _set_node_value(dbl, lit_map, kjB, 1, assumps);
    if(S.solve(assumps) == true) {
        return false;
    }

    return true;
}

int tv_solver_t::countNonMutablePairs(std::ostream& out)
{
    int cnt = 0;
    out << "keys " << ckt.num_key_inputs() << std::endl;
    for(unsigned i=0; i != ckt.num_key_inputs(); i++) {
        int this_cnt = 0;
        for(unsigned j=i+1; j != ckt.num_key_inputs(); j++) {
            if(isNonMutable(i, j)) {
                cnt += 1;
                this_cnt += 1;
                out << i << " " << j << std::endl;
            }
        }
    }
    return cnt;
}

void tv_solver_t::qbfSolveSingleKeys(std::vector<std::pair<int, int> >& values)
{
    using namespace ckt_n;
    using namespace sat_n;
    using namespace b2qbf;

    for(unsigned i=0; i != ckt.num_key_inputs(); i++) {
        dup_oneKey_t dup(ckt.key_inputs[i]);
        dblckt_t dbl(ckt, dup, false);

        nodeset_t eq, uq;
        for(unsigned j=0; j != ckt.num_key_inputs(); j++) {
            if(i != j) {
                uq.insert(dbl.getA(ckt.key_inputs[j]));
                uq.insert(dbl.getB(ckt.key_inputs[j]));
            }
        }
        for(unsigned j=0; j != ckt.num_ckt_inputs(); j++) {
            eq.insert(dbl.getA(ckt.ckt_inputs[j]));
            eq.insert(dbl.getB(ckt.ckt_inputs[j]));
        }
        eq.insert(dbl.getA(ckt.key_inputs[i]));
        eq.insert(dbl.getB(ckt.key_inputs[i]));

        assert(dbl.dbl->num_outputs() == ckt.num_outputs()*2);
        for(unsigned j=0; j != ckt.num_outputs(); j++) {
            node_t* cmp1 = dbl.dbl->outputs[2*j];
            node_t* cmp2 = dbl.dbl->outputs[2*j+1];

            CEGSolver S1;
            index2lit_map_t m1;
            dbl.dbl->init_solver(S1, eq, uq, cmp1, m1);
            S1.addClause(~dbl.getLitA(m1, ckt.key_inputs[i]));
            S1.addClause(dbl.getLitB(m1, ckt.key_inputs[i]));
            if(S1.solve() == true) {
                std::cout << "isolated " << ckt.key_inputs[i]->name << std::endl;
                break;
            }

            CEGSolver S2;
            index2lit_map_t m2;
            dbl.dbl->init_solver(S2, eq, uq, cmp2, m2);
            S2.addClause(~dbl.getLitA(m2, ckt.key_inputs[i]));
            S2.addClause(dbl.getLitB(m2, ckt.key_inputs[i]));
            if(S2.solve() == true) {
                std::cout << "isolated " << ckt.key_inputs[i]->name << std::endl;
                break;
            }
        }

    }
}

int tv_solver_t::_isolateKeys(const std::vector<unsigned>& keys, int vi, int vj, keyset_list_t& keysets)
{
    using namespace sat_n;
    using namespace ckt_n;

    assert(dbl.dbl->num_outputs() == 1);
    node_t* nout = dbl.dbl->outputs[0];
    Lit lout0 = dbl.dbl->getTLit0(lit_map, nout);
    Lit lout1 = dbl.dbl->getTLit1(lit_map, nout);

    vec_lit_t assumps;
    int cnt = 0;
    std::vector<bool> input_values(ckt.num_ckt_inputs());
    std::vector<bool> output_values(ckt.num_outputs());

    assumps.clear();
    assumps.push(~lout0);
    assumps.push(lout1);

    _init_assumps(assumps, keys, vi, vj);
    if(S.solve(assumps) == true) {
        using namespace ckt_n;

        if(verbose) {
            std::cout << "isolated key set: ";
            std::ostream_iterator<unsigned> out_it(std::cout, ", ");
            std::copy(keys.begin(), keys.end(), out_it);
            std::cout << "; values=" << vi << ", " << vj << std::endl;
        }

        _extract_inputs(input_values);
        sim.eval(input_values, output_values);
        std::pair<int,int> p = _compare_outputs(output_values);
        if(verbose) {
            std::cout << "input values  = " << input_values << std::endl;
            std::cout << "output values = " << output_values << std::endl;
            std::cout << "status=" << p.first << p.second << std::endl;
        }

        assert(p.first == 0 || p.second == 0);
        cnt += (p.first == 0) + (p.second == 0);
        if(p.first == 0) {
            keyset_value_t v(keys, vi);
            keysets.push_back(v);
        }
        if(p.second == 0) {
            keyset_value_t v(keys, vj);
            keysets.push_back(v);
        }
    }
    return cnt;
}

void tv_solver_t::solveDoubleKeys(keyset_list_t& keysets)
{
    std::set< std::pair<int,int> > keypairs;
    dbl.ckt.identify_keypairs(keypairs);
    if(verbose) std::cout << "number of keypairs = " << keypairs.size() << std::endl;
    int cnt=0;
    std::vector<unsigned> keys(2);
    for(std::set< std::pair<int,int> >::iterator it = keypairs.begin(); it != keypairs.end(); it++) {
        int ki = it->first, kj = it->second;
        keys[0] = ki; keys[1] = kj;
        for(unsigned vi=0; vi <= 3; vi++) {
            for(unsigned vj=0; vj < vi; vj++) {
                cnt += _isolateKeys(keys, vi, vj, keysets);
            }
        }
    }
    if(verbose) std::cout << "isolated pair count = " << cnt << std::endl;
}

