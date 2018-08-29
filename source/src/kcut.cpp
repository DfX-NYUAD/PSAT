#include "kcut.h"

#include <algorithm>
#include <iterator>

namespace ckt_n {
    kcut_t* kcut_t::merge_cuts(node_t* n, kcut_t& k1, kcut_t& k2, int limit, int ht)
    {
        kcut_t* cut = new kcut_t();
        cut->root = n;
        for(auto it=k1.inputs.begin(); it != k1.inputs.end(); it++) {
            node_t* ni = *it;
            if(!k2.has_node(ni)) {
                cut->inputs.insert(ni);
                assert(n->level > ni->level);
                if((int)cut->inputs.size() > limit ||
                   !n->is_reachable(ni, ht))
                {
                    delete cut;
                    return NULL;
                }
            }
        }
        for(auto it=k2.inputs.begin(); it != k2.inputs.end(); it++) {
            node_t* ni = *it;
            if(!k1.has_node(ni)) {
                cut->inputs.insert(ni);
                assert(n->level > ni->level);
                if((int)cut->inputs.size() > limit || 
                   !n->is_reachable(ni, ht))
                {
                    delete cut;
                    return NULL;
                }
            }
        }

        std::copy(k1.nodes.begin(), k1.nodes.end(), 
                  std::inserter(cut->nodes, cut->nodes.begin()));
        std::copy(k2.nodes.begin(), k2.nodes.end(), 
                  std::inserter(cut->nodes, cut->nodes.begin()));
        return cut;
    }

    bool kcut_t::is_self_contained() const
    {
        for(auto it = nodes.begin(); it != nodes.end(); it++) {
            node_t* ni = *it;
            for(unsigned j=0; j != ni->num_fanouts(); j++) {
                node_t* nj = ni->fanouts[j];
                if(nj != root && nodes.find(nj) == nodes.end()) return false;
            }
        }
        return true;
    }

    bool kcut_t::has_multiple_keyinputs() const
    {
        int cnt=0;
        for(auto it=inputs.begin(); it != inputs.end(); it++) {
            node_t* ni = *it;
            if(ni->is_keyinput()) cnt+= 1;
        }
        return cnt>=2;
    }

    kcutset_t* get_cuts(node_t* n, std::vector<kcutset_t*>& allcuts, int limit, int ht)
    {
        if(allcuts[n->get_index()]) {
            return allcuts[n->get_index()];
        }

        if(n->is_input()) {
            kcutset_t* cuts = new kcutset_t();
            cuts->push_back(new kcut_t(n));
            allcuts[n->get_index()] = cuts;
        } else {
            assert(n->is_gate());
            kcutset_t* cuts = new kcutset_t();
            for(unsigned i=0; i != n->num_inputs(); i++) {
                node_t* inp = n->inputs[i];
                kcutset_t* inp_cuts = allcuts[inp->get_index()];
                assert(inp_cuts != NULL);

                if(i==0) {
                    for(auto it = inp_cuts->begin(); it != inp_cuts->end(); it++) {
                        kcut_t* kc = *it;
                        cuts->push_back(new kcut_t(*kc));
                    }
                } else {
                    kcutset_t* new_cuts = new kcutset_t();
                    for(auto it = inp_cuts->begin(); it != inp_cuts->end(); it++) {
                        kcut_t* ki = *it;
                        for(auto jt = cuts->begin(); jt != cuts->end(); jt++) {
                            kcut_t* kj = *jt;
                            kcut_t* merged = kcut_t::merge_cuts(n, *ki, *kj, limit, ht);
                            if(merged) {
                                merged->nodes.insert(n);
                                new_cuts->push_back(merged);
                            }
                        }
                    }
                    for(auto it=cuts->begin(); it != cuts->end(); it++) {
                        delete *it;
                    }
                    cuts = new_cuts;
                }
            }
            cuts->push_back(new kcut_t(n));
            allcuts[n->get_index()] = cuts;
        }
        assert(allcuts[n->get_index()] != NULL);
        return allcuts[n->get_index()];
    }

    void compute_cuts(std::vector<kcutset_t*>& cuts, ckt_t& ckt, int limit, int ht)
    {
        cuts.resize(ckt.num_nodes(), NULL);
        for(unsigned i=0; i != ckt.num_nodes(); i++) {
            node_t* n = ckt.nodes_sorted[i];

            printf("Progress: %5d/%5d [%20s]\r", (int)i, (int)ckt.num_nodes(), n->name.c_str());
            fflush(stdout);

            get_cuts(n, cuts, limit, ht);
        }
        printf("\n");
    }

    std::ostream& operator<<(std::ostream& out, const kcut_t& kc)
    {
        out << "inputs[ ";
        for(auto it=kc.inputs.begin(); it != kc.inputs.end(); it++) {
            out << (*it)->name << " ";
        }
        out << "]; internals[ ";
        for(auto it=kc.nodes.begin(); it != kc.nodes.end(); it++) {
            out << (*it)->name << " ";
        }
        out << "]";
        return out;
    }

}
