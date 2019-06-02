#include "mutability.h"
#include "dbl.h"
#include "forker.h"
#include <sys/types.h>
#include <sys/wait.h>

namespace ckt_n {

    void mutability_analysis_t::add_immutable(node_t* n1, node_t* n2)
    {
        assert(n1 < n2);
        node_pair_t p(n1, n2);
        immutables.insert(p);
    }

    void mutability_analysis_t::analyze()
    {
        // the output map keeps track of the number of outputs affected by each node.
        std::vector< std::vector<bool> > output_maps;
        ckt.init_outputmap(output_maps);
        unsigned num_nodes = ckt.num_nodes();
        static const unsigned STEP_SIZE = 8;
        std::cout << "num_nodes = " << num_nodes << std::endl;

        uint8_t *computed_flags = new uint8_t[num_nodes];
        uint8_t *observable_flags = new uint8_t[num_nodes];

        for(unsigned gstart=0; gstart < num_nodes; gstart += STEP_SIZE) {

            forker_t f;
            pid_t childpid = f.fork();
            if(childpid == -1) {
                std::cout << "Error forking!" << std::endl;
                exit(1);
            } else if(childpid == 0) {
                // child.
                unsigned istart = gstart;
                unsigned icount = std::min(STEP_SIZE, num_nodes-istart);

                std::cout << "range: " << istart << ":" << istart+icount 
                          << std::endl;
                #pragma omp parallel for
                for(unsigned i=0; i < icount; i++) {
                    unsigned iadjusted = i + istart; 
                    node_t* ni = ckt.nodes[iadjusted];
                    if(is_observable(ni, output_maps[ni->get_index()])) {
                        analyze(ni, output_maps);
                    }
                }

                // send updated observability cache to parent.
                assert(observable_computed.size() == num_nodes);
                assert(observable_value.size() == num_nodes);
                for(unsigned i=0; i != num_nodes; i++) {
                    computed_flags[i] = (uint8_t) observable_computed[i];
                    observable_flags[i] = (uint8_t) observable_value[i];
                }
                f.write(computed_flags, num_nodes);
                f.write(observable_flags, num_nodes);
                f.close();
                exit(0);
            } else {
                // parent.
                int status;
                waitpid(childpid, &status, 0);
                if(status != 0) {
                    std::cout << "Child process error." << std::endl;
                    exit(1);
                }

                // update the observability cache.
                f.read(computed_flags, num_nodes);
                f.read(observable_flags, num_nodes);

                assert(observable_computed.size() == num_nodes);
                assert(observable_value.size() == num_nodes);
                for(unsigned i=0; i != num_nodes; i++) {
                    observable_computed[i] = computed_flags[i];
                    observable_value[i] = observable_flags[i];
                }
            }
        }
        delete [] computed_flags;
        delete [] observable_flags;
    }

    bool mutability_analysis_t::is_observable(node_t* n, std::vector<bool>& output_map) 
    {
        pthread_mutex_lock(&obsmutex);
        if(observable_computed[n->get_index()]) {
            bool result =  observable_value[n->get_index()];
            pthread_mutex_unlock(&obsmutex);
            return result;
        }
        pthread_mutex_unlock(&obsmutex);

        assert(ckt.num_key_inputs() == 0);
        bool result;

        // TODO: call ckt_t::is_observable instead of this code.
        if(n->is_input() && n->output) {
            result = true;
        } else {
            int index = n->get_index();
            pthread_mutex_lock(&obsmutexes[index]);
            nodelist_t outputs;
            for(unsigned i=0; i != ckt.num_outputs(); i++) {
                if(output_map[i]) {
                    node_t* n_i = ckt.outputs[i];
                    outputs.push_back(n_i);
                }
            }
            if(outputs.size() == 0) {
                // dead node.
                result = false;
            } else {
                // so we are trying to create a slice of the circuit 
                // which contains only the outputs affected by this node.
                ckt_t::node_map_t nm_fwd, nm_rev;
                ckt_t cs(ckt, outputs, nm_fwd, nm_rev);

                // n_pr (i.e., n-prime) is the node we are interested in the sliced ckt.
                ckt_t::node_map_t::iterator pos_n = nm_fwd.find(n);
                assert(pos_n != nm_fwd.end());
                node_t* n_pr  = pos_n->second;

                // now we want to double this circuit.
                dblckt_t dbl(cs, dup_allkeys, true);
                // n_p2 is the node we are going to invert in the doubled ckt.
                node_t* n_p2 = dbl.getB(n_pr);

                //std::cout << "testing observability of: " << n->name << std::endl;
                //std::cout << "ORIGINAL CIRCUIT" << std::endl;
                //std::cout << *(dbl.dbl) << std::endl;
                //std::cout << "INVERTED CIRCUIT" << std::endl;
                dbl.dbl->invert_gate(n_p2);
                dbl.dbl->topo_sort();
                dbl.dbl->init_indices();
                //std::cout << *(dbl.dbl) << std::endl;

                // now we create the solver.
                sat_n::Solver S;
                index2lit_map_t  lmap;
                dbl.dbl->init_solver(S, lmap);

                // now we solve for the output = 1, which means the two circuits output different values.
                assert(dbl.dbl->num_outputs() == 1);
                node_t* out = dbl.dbl->outputs[0];
                sat_n::Lit l = dbl.dbl->getLit(lmap, out);

                result = S.solve(l);
            }
            pthread_mutex_unlock(&obsmutexes[index]);
        }

        pthread_mutex_lock(&obsmutex);
        observable_computed[n->get_index()] = true;
        observable_value[n->get_index()] = result;
        pthread_mutex_unlock(&obsmutex);

        return result;
    }

    void mutability_analysis_t::analyze(node_t* n, std::vector< std::vector<bool> >& output_map)
    {
        const std::vector<bool>& om = output_map[n->get_index()];
        assert(om.size() == ckt.num_outputs());

        std::vector<bool> output_fanins(ckt.num_nodes(), false);
        for(unsigned i=0; i != om.size(); i++) {
            if(om[i]) {
                ckt.compute_transitive_fanin(ckt.outputs[i], output_fanins, false /* no reset. */);
            }
        }
        for(int i=0; i != (int)output_fanins.size(); i++) {
            if(output_fanins[i]) {
                node_t* ni = ckt.nodes[i];
                if(n < ni) {
                    if(is_observable(ni, output_map[ni->get_index()])) {
                        assert(ni->get_index() == i);
                                  // is there a test vector with ni=X that distinguishes n=0|1
                        int cnt = is_immutable(n, ni, output_map)*2 +
                                  // is there a test vector with n=X that distinguishes ni=0|1
                                  is_immutable(ni, n, output_map);
                        if(cnt) {
                            pthread_mutex_lock(&outmutex);
                            out << n->name << " " << ni->name << " " 
                                << cnt << std::endl;
                            pthread_mutex_unlock(&outmutex);
                        }
                    }
                }
            }
        }
    }

    bool mutability_analysis_t::is_immutable(
        node_t* n1, node_t* n2, 
        std::vector< std::vector<bool> >& output_map
    )
    {
        // get the outputs affected by these two nodes.
        std::vector<bool>& om1 = output_map[n1->get_index()];
        std::vector<bool>& om2 = output_map[n2->get_index()];

        // the fanin_flags encode the nodes in the circuit which are necessary
        // to compute the outputs required by both n1 and n2.
        std::vector<bool> fanin_flags(ckt.num_nodes(), false);
        fanin_flags[n2->get_index()] = true;

        // walk through the outputs and do the recursive traversal.
        std::vector<bool> om(om1.size());
        assert(om.size() == om2.size());
        assert(om2.size() == ckt.num_outputs());
        for(unsigned i=0; i != om.size(); i++) {
            om[i] = om1[i] || om2[i];
            if(om[i]) {
                ckt.compute_transitive_fanin(ckt.outputs[i], fanin_flags, false/*noreset*/);
            }
        }

        // now create a slice of the circuit using these fanin flags.
        ckt_t::node_map_t nm_fwd, nm_rev;
        ckt_t cs(ckt, n2, fanin_flags, nm_fwd, nm_rev);
        //std::cout << "n1: " << n1->name << "; n2: " << n2->name << std::endl;
        //std::cout << cs << std::endl;

        // FIXME: what if n1 is not in the circuit?

        // this means thta n2 has cut n1 out of the circuit.
        // this means that if n2 is set to X then n1 can't be observed.
        // this in turn means that n1,n2 is immutable iff n2 is observable.

        // n_pr1 (i.e., n-prime) is the node we are interested in the sliced ckt.
        ckt_t::node_map_t::iterator pos_n1 = nm_fwd.find(n1);
        if(pos_n1 == nm_fwd.end()) {
            return false;
        }
        assert(pos_n1 != nm_fwd.end());
        node_t* n_pr1  = pos_n1->second;

        // FIXME: what is n2 is not in the circuit?
        // this means that n1 has cut n2 out of the circuit.
        // this means that if n1 is set to X then n2 can't be observed.
        // ???

        // n_pr2 is equivalent for n2
        ckt_t::node_map_t::iterator pos_n2 = nm_fwd.find(n2);
        if(pos_n2 == nm_fwd.end()) {
            return false;
        }
        assert(pos_n2 != nm_fwd.end());
        node_t* n_pr2  = pos_n2->second;

        // now we again double this circuit.
        dblckt_t dbl(cs, dup_allkeys, true);

        // n_p1 is the node we are going to invert in the doubled ckt.
        node_t* n_p1 = dbl.getB(n_pr1);

        // n_p2 is the node we are going to set to X.
        node_t* n_p2A  = dbl.getA(n_pr2);
        node_t* n_p2B  = dbl.getB(n_pr2);

        dbl.dbl->invert_gate(n_p1);
        dbl.dbl->split_gates();

        // don't need topo sort or init_indices because split gates will do
        // that for us.

        //dbl.dbl->topo_sort();
        //dbl.dbl->init_indices();

        //std::cout << *dbl.dbl << std::endl;


        using namespace sat_n;

        Solver S;
        ckt_n::index2lit_map_t lit_map;
        dbl.dbl->init_ternary_solver(S, lit_map);

        vec_lit_t assumps;
        // assert the output = 1.
        // this means l_out0 is = 0 (i.e. not X) and l_out1 = 1 (corresp. 1)
        Lit l_out0 = dbl.dbl->getTLit0(lit_map, dbl.dbl->outputs[0]);
        Lit l_out1 = dbl.dbl->getTLit1(lit_map, dbl.dbl->outputs[0]);
        assumps.push(~l_out0);
        assumps.push(l_out1);

        // n_2 must be X.
        Lit ln2A0 = dbl.dbl->getTLit0(lit_map, n_p2A);
        assumps.push(ln2A0);
        Lit ln2B0 = dbl.dbl->getTLit0(lit_map, n_p2B);
        assumps.push(ln2B0);

        return (!S.solve(assumps));
    }
}
