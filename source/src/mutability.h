#ifndef _MUTABILITY_H_DEFINED_
#define _MUTABILITY_H_DEFINED_

#include "ckt.h"
#include <set>
#include <pthread.h>

// TODO:
// 1. build observability cache.
// 2. think about the n1,n2 not present issues.

namespace ckt_n {

    class mutability_analysis_t
    {
    public:
        typedef std::pair<node_t*, node_t*> node_pair_t;
    protected:
        ckt_t& ckt;
        std::set<node_pair_t> immutables;
        std::vector<bool> observable_value;
        std::vector<bool> observable_computed;
        pthread_mutex_t outmutex;
        pthread_mutex_t obsmutex;
        pthread_mutex_t* obsmutexes;
        std::ostream& out;
    public:
        mutability_analysis_t(ckt_t& c, std::ostream& sout) 
            : ckt(c)
            , out(sout)
        {
            assert(ckt.num_key_inputs() == 0);
            observable_value.resize(ckt.num_nodes(), false);
            observable_computed.resize(ckt.num_nodes(), false);
            pthread_mutex_init(&outmutex, NULL);
            pthread_mutex_init(&obsmutex, NULL);

            obsmutexes = new pthread_mutex_t[ckt.num_nodes()];
            for(unsigned i=0; i != ckt.num_nodes(); i++) {
                pthread_mutex_init(&obsmutexes[i], NULL);
            }
        }

        ~mutability_analysis_t() {
            pthread_mutex_destroy(&outmutex);
            pthread_mutex_destroy(&obsmutex);
            for(unsigned i=0; i != ckt.num_nodes(); i++) {
                pthread_mutex_destroy(&obsmutexes[i]);
            }
        }

        void add_immutable(node_t* n1, node_t* n2);
        void analyze();

        bool is_observable(node_t* n, std::vector<bool>& output_map);
        void analyze(node_t *n, std::vector< std::vector<bool> >& output_map);
        bool is_immutable( node_t* n1, node_t* n2, 
            std::vector< std::vector<bool> >& output_map);
    };

}

#endif
