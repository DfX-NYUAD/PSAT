#include <fstream>
#include "dac12enc.h"
#include <boost/lexical_cast.hpp>

namespace ckt_n
{
    dac12enc_t::dac12enc_t(
        ast_n::statements_t& stms, 
        const std::string& graphFile, 
        const std::string& cliqueFile, 
        double key_fraction
    )
        : ckt(stms)
    {
        target_keys = int((ckt.num_gates() * key_fraction) + 0.5);

        ckt.init_node_map(nmap);
        _read_clique(cliqueFile);
        _add_clique();
        _read_graph(graphFile);
        _add_greedy();
        std::cout << "encoding circuit with graph: " << graphFile << "; clique: " << cliqueFile << std::endl;
        std::cout << "clique size: " << clique.size() << std::endl;
        ckt.topo_sort();
    }

    void dac12enc_t::_read_graph(const std::string& graphFile)
    {
        std::ifstream fin(graphFile.c_str());
        std::string name1, name2;
        int mut;
        while(fin >> name1 >> name2 >> mut) {
            ckt_t::map_t::iterator pos;
            if((pos = nmap.find(name1)) == nmap.end()) {
                std::cout << "unknown node: " << name1 << std::endl;
                exit(1);
            }
            node_t* n1 = pos->second;
            if((pos = nmap.find(name2)) == nmap.end()) {
                std::cout << "unknown node: " << name2 << std::endl;
                exit(1);
            }
            node_t* n2 = pos->second;
            if(mut < 1 || mut > 3) {
                std::cout << "unknown mutability value: " << mut << std::endl;
                exit(1);
            }
            if((mut&1)) {
                nodepair_t p1(n1,n2);
                edges.insert(p1);
            }
            if((mut&2)) {
                nodepair_t p2(n2,n1);
                edges.insert(p2);
            }
        }
    }

    int dac12enc_t::_get_wt(node_t* n1, node_t* n2) 
    {
        nodepair_t p1(n1,n2);
        nodepair_t p2(n2,n1);

        return ((edges.find(p1) != edges.end()) ? 1 : 0) + 
               ((edges.find(p2) != edges.end()) ? 1 : 0);
    }

    int dac12enc_t::_get_wt(node_t* n1, const nodeset_t& ns) {
        int s = 0;
        for(nodeset_t::const_iterator it = ns.begin(); it != ns.end(); it++) {
            s += _get_wt(n1, *it);
        }
        return s;
    }

    void dac12enc_t::_read_clique(const std::string& cliqueFile)
    {
        std::ifstream fin(cliqueFile.c_str());
        std::string node_name;
        while(fin >> node_name) {
            ckt_t::map_t::iterator pos;
            if((pos = nmap.find(node_name)) == nmap.end()) {
                std::cout << "unknown node: " << node_name << std::endl;
                exit(1);
            }
            node_t* n = pos->second;
            clique.push_back(n);
        }
    }

    void dac12enc_t::_add_clique()
    {
        for(clique_list_t::iterator it = clique.begin(); it != clique.end(); it++) {
            node_t* n = *it;
            node_t* ki = ckt.create_key_input();
            int val = rand() % 2;
            key_values.push_back(val);
            ckt.insert_key_gate(n, ki, val);
            targets.insert(n);
            if((int)key_values.size() >= target_keys) break;
        }
    }

    void dac12enc_t::_add_greedy()
    {
        int num_nodes = ckt.nodes.size();
        int max_greedy_keys = target_keys - ckt.num_key_inputs();

        for(int cnt=0; cnt < max_greedy_keys; cnt++) {
            unsigned best_index = 0; 
            int best_wt = 0;
            for(int i=0; i != num_nodes; i++) {
                node_t* n = ckt.nodes[i];
                if(n->is_keyinput()) continue;
                if(targets.find(n) != targets.end()) continue;
                int s = _get_wt(n, targets);
                if(s > best_wt) {
                    best_wt = s;
                    best_index = i;
                }
            }
            if(best_wt == 0) break;
            node_t* n = ckt.nodes[best_index];
            targets.insert(n);
            // add the key now.
            node_t* ki = ckt.create_key_input();
            int val = rand() % 2;
            key_values.push_back(val);
            ckt.insert_key_gate(n, ki, val);
        }
    }


    dac12enc_t::~dac12enc_t()
    {
    }
}
