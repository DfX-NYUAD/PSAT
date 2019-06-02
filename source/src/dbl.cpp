#include "dbl.h"
#include <boost/algorithm/string/predicate.hpp>

namespace ckt_n {

    dup_allkeys_t dup_allkeys;

    bool dup_allkeys_t::shouldDup(node_t* inp) {
        return boost::starts_with(inp->name, "keyinput");
    }

    bool dup_oneKey_t::shouldDup(node_t* n) {
        return n == key;
    }

    bool dup_twoKeys_t::shouldDup(node_t* n) {
        return (n == k1) || (n == k2);
    }


    dblckt_t::dblckt_t(ckt_t& c, dup_interface_t& interface, bool compare_outputs)
        : ckt(c)
    {
        pair_map.resize(c.nodes.size());
        // deal with the inputs first.
        for(unsigned i=0; i != c.num_inputs(); i++) {
            node_t* inp = c.inputs[i];
            int index = inp->get_index();
            assert(index >= 0 && index < (int) pair_map.size());

            if(!interface.shouldDup(inp)) {
                pair_map[index].first = node_t::create_input(inp->name);
                pair_map[index].second = pair_map[index].first;
            } else {
                // a keyinput, so duplicate it.
                pair_map[index].first = 
                    node_t::create_input(inp->name + "_A");
                pair_map[index].second = 
                    node_t::create_input(inp->name + "_B");
            }
        }
        // now deal with the other nodes. 
        for(unsigned i=0; i != c.num_gates(); i++) {
            node_t* g = c.gates[i];
            int index = g->get_index();
            assert(index >= 0 && index < (int) pair_map.size());

            pair_map[index].first = 
                node_t::create_gate(g->name + "_A", g->func);
            pair_map[index].second = 
                node_t::create_gate(g->name + "_B", g->func);
        }
        // now create the inputs for the gates.
        for(unsigned i=0; i != c.num_gates(); i++) {
            node_t* g = c.gates[i];
            _create_inputs(g);
        }
        dbl = new ckt_t(pair_map);

        if(compare_outputs) {
            // now add comparators for the outputs.
            std::string out_name("final_cmp_out");
            std::string out_func("or");
            node_t* final_or = node_t::create_gate(out_name, out_func);
            final_or->output = true;
            final_or->newly_added = true;
            dbl->outputs.push_back(final_or);
            dbl->gates.push_back(final_or);
            dbl->nodes.push_back(final_or);

            for(unsigned i=0; i != c.num_outputs(); i++) {
                node_t* o = c.outputs[i];
                int oidx = o->get_index();
                assert(oidx >= 0 && oidx < (int) pair_map.size());

                node_t* oA = pair_map[oidx].first;
                node_t* oB = pair_map[oidx].second;

                std::string name = oA->name + "_" + oB->name + "_cmp";
                std::string func = "xor";
                node_t* g = node_t::create_gate(name, func);
                g->add_input(oA);
                g->add_input(oB);
                g->newly_added = true;

                dbl->gates.push_back(g);
                dbl->nodes.push_back(g);
                final_or->add_input(g);
            }
        } else {
            for(unsigned i=0; i != c.num_outputs(); i++) {
                node_t* o = c.outputs[i];
                int oidx = o->get_index();
                assert(oidx >= 0 && oidx < (int) pair_map.size());

                node_t* oA = pair_map[oidx].first;
                node_t* oB = pair_map[oidx].second;

                std::string name = oA->name + "$inv";
                std::string func = "not";
                node_t* oA_inv = node_t::create_gate(name, func);
                oA_inv->add_input(oA);
                dbl->gates.push_back(oA_inv);
                dbl->nodes.push_back(oA_inv);

                name = oB->name + "$inv";
                func = "not";
                node_t* oB_inv = node_t::create_gate(name, func);
                oB_inv->add_input(oB);
                dbl->gates.push_back(oB_inv);
                dbl->nodes.push_back(oB_inv);

                name = o->name + "$cmp1";
                func = "and";
                node_t* cmp1 = node_t::create_gate(name, func);
                cmp1->add_input(oA_inv);
                cmp1->add_input(oB);
                dbl->gates.push_back(cmp1);
                dbl->nodes.push_back(cmp1);

                name = o->name + "$cmp2";
                func = "and";
                node_t* cmp2 = node_t::create_gate(name, func);
                cmp2->add_input(oA);
                cmp2->add_input(oB_inv);
                dbl->gates.push_back(cmp2);
                dbl->nodes.push_back(cmp2);

                cmp1->output = true;
                dbl->outputs.push_back(cmp1);
                cmp2->output = true;
                dbl->outputs.push_back(cmp2);
            }
        }

        dbl->init_fanouts();
        dbl->_init_indices();
        dbl->topo_sort();
    }

    void dblckt_t::_create_inputs(node_t* g) 
    {
        int index = g->get_index();
        assert(index >= 0 && index < (int) pair_map.size());

        node_t* nA = pair_map[index].first;
        node_t* nB = pair_map[index].second;

        for(unsigned i = 0; i != g->num_inputs(); i++) {
            int idx = g->inputs[i]->get_index();
            node_t* inpA = pair_map[idx].first;
            node_t* inpB = pair_map[idx].second;
            nA->add_input(inpA);
            nB->add_input(inpB);
        }
    }

    void dblckt_t::dump_solver_state(
        std::ostream& out,
        sat_n::Solver& S, 
        index2lit_map_t& lmap) const
    {
        using namespace sat_n;

        for(unsigned i=0; i != ckt.num_inputs(); i++) {
            int idx = ckt.inputs[i]->get_index();
            out << ckt.inputs[i]->name << ":";

            int i1 = pair_map[idx].first->get_index();
            int i2 = pair_map[idx].second->get_index();

            Lit l1 = lmap[i1];
            lbool v1 = S.modelValue(l1);

            Lit l2 = lmap[i2];
            lbool v2 = S.modelValue(l2);

            assert(v1.isDef());
            if(!v1.getBool()) out << "0";
            else out << "1";

            assert(v2.isDef());
            if(!v2.getBool()) out << "0";
            else out << "1";

            out << " ";
        }

        out << " ==> ";
        for(unsigned i=0; i != ckt.num_outputs(); i++) {
            int idx = ckt.outputs[i]->get_index();
            out << ckt.outputs[i]->name << ":";

            int i1 = pair_map[idx].first->get_index();
            int i2 = pair_map[idx].second->get_index();

            Lit l1 = lmap[i1];
            lbool v1 = S.modelValue(l1);

            Lit l2 = lmap[i2];

            lbool v2 = S.modelValue(l2);

            assert(v1.isDef());
            if(v1.getBool()) out << "0";
            else out << "1";

            assert(v2.isDef());
            if(!v2.getBool()) out << "0";
            else out << "1";

            out << " ";
        }
    }

    dblckt_t::~dblckt_t()
    {
        delete dbl;
    }
}
