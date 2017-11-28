#ifndef _CKT_H_DEFINED_
#define _CKT_H_DEFINED_

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "node.h"
#include "ast.h"
#include "sat.h"
#include "allsatckt.h"
#include "SATInterface.h"

namespace ckt_n {
    typedef std::pair<node_t*, node_t*> nodepair_t;
    typedef std::vector< nodepair_t > nodepair_list_t;

    struct ckt_t
    {
        typedef std::map<std::string, node_t*> map_t;
        typedef std::map<node_t*, node_t*> node_map_t;

        nodelist_t inputs;
        nodelist_t ckt_inputs;
        nodelist_t key_inputs;

        nodelist_t outputs;
        nodelist_t gates;
        nodelist_t nodes;
        nodelist_t gates_sorted;
        nodelist_t nodes_sorted;

        ckt_t(ast_n::statements_t& stms);           // constructor from bench file.
        ckt_t(nodepair_list_t& pair_map);           // circuit doubling constructor.
        ckt_t(const ckt_t& c1, const ckt_t& c2);    // circuit comparison constructor.
        ckt_t(const ckt_t& c1, node_map_t& nm);     // copy constructor.

        // Create a circuit from a slice of this circuit.
        ckt_t(const ckt_t& ckt, nodelist_t& outputs,
              node_map_t& nm_fwd,
              node_map_t& nm_rev);                  

        // also from a slice but a slice defined by the fan-in flags.
        ckt_t(const ckt_t& ckt, 
              node_t* n_cut,
              const std::vector<bool>& fanin, 
              node_map_t& nm_fwd,
              node_map_t& nm_rev);
        ~ckt_t();

        sat_n::Lit getLit(index2lit_map_t& mappings, node_t* n);
        sat_n::Lit getTLit0(index2lit_map_t& mappings, node_t* n);
        sat_n::Lit getTLit1(index2lit_map_t& mappings, node_t* n);

        void init_solver(sat_n::Solver& S, index2lit_map_t& mappings);
        void init_solver_ignoring_nodes(sat_n::Solver& S, index2lit_map_t& mappings, nodeset_t& nodes2ignore);

        void init_ternary_solver(sat_n::Solver& S, index2lit_map_t& mappings);

        void init_solver(AllSAT::AllSATCkt& S, index2lit_map_t& mappings);
        void init_solver(AllSAT::ClauseList& S, index2lit_map_t& mappings);
        void init_solver(AllSAT::AllSATCkt& S1, sat_n::Solver& S2, 
                            index2lit_map_t& mappings);
        void init_solver(sat_n::Solver& S1, AllSAT::ClauseList& S2, 
                            index2lit_map_t& mappings, bool skipNewlyAdded);

        // sanity check, make sure all node inputs are present, indices match etc.
        void check_sanity();

        void rewrite_buffers(void);
        // set the values of the keys to these specified constants.
        void rewrite_keys(std::vector<sat_n::lbool>& keys);
        void rewrite_keys(std::map<std::string, int>& keys);
        void rewrite_keys(std::vector< std::pair<int, int> >& keys);
        // remove keys which don't have any fanouts.
        bool remove_dead_keys(void);
        // remove gates which don't have fanouts. (rewires outputs)
        bool remove_dead_gates(void);

        void init_solver(
            b2qbf::ISolver& S, 
            const nodeset_t& eq,
            const nodeset_t& uq,
            node_t* output,
            index2lit_map_t& mappings
        );
        sat_n::Lit create_slice_diff(
            sat_n::Solver& S, 
            node_t* out,
            node_t* diff,
            const nodeset_t& internals,
            const nodeset_t& inputs,
            node2var_map_t& n2v1,
            node2var_map_t& n2v2
        );

        void init_input_map(index2lit_map_t& mappings, std::vector<bool>& inputs) const;
        void init_keyinput_map(index2lit_map_t& mappings, std::vector<bool>& inputs) const;

        bool check_equiv(const char* keys);
        bool has_cycle(std::vector<int>& marks) const;
        bool _cycle_check(unsigned i, std::vector<int>& marks) const;

        unsigned num_inputs() const { return inputs.size(); }
        unsigned num_ckt_inputs() const { return ckt_inputs.size(); }
        unsigned num_key_inputs() const { return key_inputs.size(); }
        unsigned num_outputs() const { return outputs.size(); }
        unsigned num_gates() const { return gates.size(); }
        unsigned num_nodes() const { return nodes.size(); }

        void topo_sort();
        void init_keymap(std::vector< std::vector<bool> >& keymap);
        void init_outputmap(std::vector< std::vector<bool> >& outmap);

        unsigned count_bitmap(const std::vector< std::vector<bool> >& keymap, node_t* n);
        void get_indices_in_bitmap(
            const std::vector< std::vector<bool> >& bitmap, 
            node_t* n, std::vector<int>& indices);
        void key_map_analysis();
        void identify_keypairs(std::set<std::pair<int, int> >& keypairs);

        void split_gates();
        void dump(std::ostream& out);
        void init_fanouts() { _init_fanouts(); }
        void init_indices() { _init_indices(); }

        void add_ckt_input(node_t* ci);
        void add_key_input(node_t* ki);
        void add_gate(node_t* g);
        void invert_gate(node_t* n);

        node_t* create_key_input();
        // Insert a key gate at the output of this node.
        // This key gate is controlled by the key input 'ki'.
        // val is the correct value of the key input.
        void insert_key_gate(node_t* n, node_t* ki, int val);
        node_t* insert_mux_key_gate(node_t* ki, node_t* n);
        void insert_2inp_key_gate(node_t* ki, node_t* n, const std::string& func);

        static const std::string& get_inv_func(const std::string& name);

        void remove_key_input(node_t* ki);
        void remove_gate(node_t* g);
        void replace_output(node_t* old_out, node_t* new_out);

        // compute the transitive fanin for node out. if reset=true, then fanin_flags are all set to false.
        void compute_transitive_fanin(node_t* out, std::vector<bool>& fanin_flag, bool reset=true) const;
        // compute the transitive fanin for node out but stop dfs at the node
        // stop (i.e., treat it like an input). if reset=true, then fanin_flags
        // are all set to false.
        void compute_transitive_fanin(node_t* out, node_t* stop, std::vector<bool>& fanin_flags) const;
        void get_observable_nodes(node_t* out, nodeset_t& obs);
        void get_nodesets(const std::vector<bool>& fanins, node_t* inp, nodeset_t& internals, nodeset_t& inputs);
        bool is_observable(node_t* out, node_t* n, const nodeset_t& internals, const nodeset_t& inputs);

        // return true if n node is observable for the outputs s.t. output_map[o] = true.
        bool is_observable(node_t* n, std::vector<bool>& output_map);

        void init_node_map(map_t& map);
        bool compareIOs(ckt_t& ckt, int verbose);
        int get_key_input_index(node_t* ki);
        int get_ckt_input_index(node_t* ki);

        void key_simplify(std::vector<sat_n::lbool>& keys);

        static void _delete_from_list(nodelist_t& list, nodeset_t& delset);
        void dump_cuts(int limit, int ht);
        void cleanup() { _cleanup(); }
        void compute_node_prob( std::vector<double>& ps );
    private:
        void _init_map(map_t& map, ast_n::statements_t& stms);
        void _init_inputs(map_t& map, ast_n::statements_t& stms);
        void _init_outputs(map_t& map, ast_n::statements_t& stms);
        void _init_fanouts();
        void _init_indices();
        void _copy_array(const node_map_t& nm, const nodelist_t& in_array, nodelist_t& out_array);
        void _cleanup();

        void _create_ckt_inputs(node_map_t& n, const nodelist_t& i1, const nodelist_t& i2);
        void _create_key_inputs(node_map_t& n, const nodelist_t& i1, const char* suffix);
        void _create_gates(node_map_t& node_map, const nodelist_t& gs, const char* suffix);
        void _wire_gate_inputs(const node_map_t& nm, const nodelist_t& gs);
        void _compare_outputs(node_map_t& nm, const nodelist_t& o1, const nodelist_t& o2);
        void _compute_gate_prob( node_t* ni, std::vector<double>& ps );

        void _split_gate(node_t* g);
        node_t* _create_gate(nodelist_t& inputs, const std::string& type, const std::string& name);

        static int _find_in_array(const nodelist_t& list, node_t* n);
        static void _remove_from_array(nodelist_t& list, node_t* n);

        void _compute_fanin_recursive(node_t* out, std::vector<bool>& fanin_flag) const;

        void _compute_node_prob(std::vector<double>& ps);
        void _compute_node_prob( std::vector<double>& ps, node_t* inp, int val ) ;
        double _compute_wt_avg(
            std::vector<double>& ps_init, std::vector<std::vector<double> >& ps_array, unsigned j);

        friend class dblckt_t;

    };


    std::ostream& operator<<(std::ostream& out, const ckt_t& ckt);
    bool node_topo_cmp(const node_t* n_lt, const node_t* n_rt);
    void or_bitmap(std::vector<bool>& v1, std::vector<bool>& v2);
}

#endif
