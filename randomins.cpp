#include "randomins.h"
#include <boost/lexical_cast.hpp>

namespace ckt_n {
    void random_insert(ckt_t& ckt, int numKeys)
    {
        nodeset_t selNodes;

        std::vector< std::vector<bool> > output_maps;
        ckt.init_outputmap(output_maps);

        nodelist_t node_copy(ckt.nodes);
        std::random_shuffle(node_copy.begin(), node_copy.end());

        for(unsigned i=0; i != node_copy.size(); i++) {
            // select a node randomly.
            node_t* n = node_copy[i];

            // if already selected, move on.
            assert(selNodes.find(n) == selNodes.end());

            // if input is selected, move on.
            for(unsigned j=0; j != n->num_inputs(); j++) {
                node_t* ni = n->inputs[j];
                if(selNodes.find(ni) != selNodes.end()) {
                    continue;
                }
            }
            // if fanout is selected, move on.
            for(unsigned j=0; j != n->num_fanouts(); j++) {
                node_t* nj = n->fanouts[j];
                if(selNodes.find(nj) != selNodes.end()) {
                    continue;
                }
            }
            // if not observable, move on.
            if(!ckt.is_observable(n, output_maps[n->get_index()])) {
                continue;
            }

            assert(n != NULL);

            selNodes.insert(n);
            if((int)selNodes.size() >= numKeys) break;
        }

        std::cout << "# key=";
        for(auto it=selNodes.begin(); it != selNodes.end(); it++) {
            node_t* n = *it;
            node_t* ki = ckt.create_key_input();
            int val = rand() % 2;
            ckt.insert_key_gate(n, ki, val);
            std::cout << val;
        }
        std::cout << std::endl;
        ckt.init_indices();
        ckt.init_fanouts();
        ckt.topo_sort();
    }
}
