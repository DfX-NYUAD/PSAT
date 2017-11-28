#ifndef _NODE_H_DEFINED_
#define _NODE_H_DEFINED_

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

#include <set>
#include <queue>

#include "sat.h"
#include "allsatckt.h"
#include "ISolver.h"

namespace ckt_n {
    struct node_t;
    typedef std::vector<node_t*> nodelist_t;
    typedef std::set<node_t*> nodeset_t;

    struct node_t {

        enum { INPUT, GATE }                            type;
        std::string                                     name;
        std::string                                     func;
        nodelist_t                                      inputs;
        nodelist_t                                      fanouts;
        bool                                            output;
        bool                                            keyinput;
        // keygate is only set in the encoder.
        bool                                            keygate;
        bool                                            newly_added;
        const clause_provider_i<sat_n::Solver>          *prov;
        const clause_provider_i<AllSAT::AllSATCkt>      *prov3;
        const clause_provider_i<AllSAT::ClauseList>     *prov4;
        const clause_provider_i<sat_n::Solver>          *tprov;
        const clause_provider_i<b2qbf::ISolver>         *prov5;
        int                                             index;
        int                                             level;

        unsigned num_inputs() const { return inputs.size(); }
        unsigned num_fanouts() const { return fanouts.size(); }

        static node_t* create_input(const std::string name);
        static node_t* create_gate( const std::string& name, const std::string& func );
        void add_input(node_t* input);
        void add_fanout(node_t* fout);
        void rewrite_input(node_t* oldinp, node_t* newinp);
        void rewrite_fanouts_with(node_t* repl);

        void remove_input(node_t* inp);
        void remove_fanout(node_t* fout);
        void const_propagate(node_t* input, bool value);
       
        bool is_reachable(node_t* n, int depth) const;

        const clause_provider_i<sat_n::Solver>* get_solver_provider();
        const clause_provider_i<AllSAT::AllSATCkt>* get_allsatckt_provider();
        const clause_provider_i<AllSAT::ClauseList>* get_clauselist_provider();
        const clause_provider_i<b2qbf::ISolver>* get_qbf_provider();

        const clause_provider_i<sat_n::Solver>* get_ternary_solver_provider();

        int get_index() const { assert(index != -1); return index; };
        bool is_keyinput() const { return (keyinput && INPUT == type); }
        bool is_keygate() const { return keygate && GATE == type; }
        bool is_input() const { return type == INPUT; }
        bool is_gate() const { return type == GATE; }
        bool is_fanout_keygate() const;
        bool is_input_keyinput() const;
        double calc_out_prob(std::vector<double>& p_in) const;

        template<class T> void forEachNeighbour(int depth, T& func);

        static std::string invert(const std::string& func);

    private:
        node_t();
        void _init_input(const std::string& n);
        void _init_gate( const std::string& o, const std::string& f );
    };

    template<class T> void node_t::forEachNeighbour(int depth, T& func)
    {
        typedef std::pair<node_t*, int> qentry_t;
        std::set<node_t*> visited;
        std::queue<qentry_t> q;
        q.push(qentry_t(this, 0));
        while(!q.empty()) {
            qentry_t qe = q.front();
            node_t* nxt = qe.first;

            int this_depth = qe.second;
            if(this_depth == depth) continue;
            if(visited.find(nxt) != visited.end()) continue;

            for(unsigned i=0; i != nxt->num_inputs(); i++) {
                node_t* inp_i = nxt->inputs[i];
                qentry_t qe(inp_i, this_depth+1);
                q.push(qe);
            }
            for(unsigned i=0; i != nxt->num_fanouts(); i++) {
                node_t* out_i = nxt->fanouts[i];
                qentry_t qe(out_i, this_depth+1);
                q.push(qe);
            }

            func(nxt);
        }
    }

    std::ostream& operator<<(std::ostream& out, const node_t& n);
    std::ostream& operator<<(std::ostream& out, const nodelist_t& nl);
    std::ostream& operator<<(std::ostream& out, const nodeset_t& nout);
}
#endif
