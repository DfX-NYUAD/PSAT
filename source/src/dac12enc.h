#ifndef _DAC12ENC_H_DEFINED_
#define _DAC12ENC_H_DEFINED_

#include "ckt.h"
#include "ast.h"
#include <iostream>
#include <list>
#include <set>

namespace ckt_n {

    class dac12enc_t
    {
        typedef std::list<node_t*> clique_list_t;
        typedef std::set<nodepair_t> nodepair_set_t;

        ckt_t               ckt;              
        ckt_t::map_t        nmap;
        clique_list_t       clique;
        nodepair_set_t      edges;
        nodeset_t           targets;
        int                 target_keys;
        std::vector<int>    key_values;

        void _read_graph(const std::string& graphFile);
        void _read_clique(const std::string& cliqueFile);
        void _add_clique();
        void _add_greedy();
        int _get_wt(node_t* n1, node_t* n2);
        int _get_wt(node_t* n1, const nodeset_t& ns);
    public:
        dac12enc_t(ast_n::statements_t& stms, const std::string& graphFile, const std::string& cliqueFile, double fraction);
        virtual ~dac12enc_t();

        void write(std::ostream& out) {
            assert(key_values.size() == ckt.num_key_inputs());
            out << "# key=";
            for(unsigned i=0; i != key_values.size(); i++) {
                out << key_values[i];
            }
            out << std::endl;

            out << ckt;
        }
    };
}

#endif
