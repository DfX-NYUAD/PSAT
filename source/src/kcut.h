#ifndef _KCUT_H_DEFINED_
#define _KCUT_H_DEFINED_

#include "node.h"
#include "ckt.h"
#include <iostream>

namespace ckt_n
{
    struct kcut_t;
    typedef std::vector<kcut_t*> kcutset_t;

    struct kcut_t {
        node_t* root;
        nodeset_t nodes;
        nodeset_t inputs;

        kcut_t(node_t* n) : root(n) {
            inputs.insert(n);
        }

        bool has_node(node_t* n) const {
            return nodes.find(n) != nodes.end();
        }
        bool has_input(node_t* n) const {
            return inputs.find(n) != inputs.end();
        }
        bool is_self_contained() const;
        bool has_multiple_keyinputs() const;

        static kcut_t* merge_cuts(node_t* n, kcut_t& k1, kcut_t& k2, int limit, int ht);

    private:
        kcut_t() : root(NULL) {}
    };

    kcutset_t* get_cuts(node_t* n, std::vector<kcutset_t*>& allcuts, int limit, int ht);
    void compute_cuts(std::vector<kcutset_t*>& cuts, ckt_t& ckt, int limit, int ht);

    std::ostream& operator<<(std::ostream& out, const kcut_t& kc);
}


#endif
