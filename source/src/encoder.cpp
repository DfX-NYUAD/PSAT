#include "encoder.h"
#include "sim.h"
#include "tvsolver.h"

#include <boost/lexical_cast.hpp>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <limits.h>

namespace ckt_n {

    encoder_t::encoder_t(ast_n::statements_t& stms, int seed)
        : ckt(stms)
        , ckt_o(ckt, nm)
        , max_keys(INT_MIN)
    {
    }

    encoder_t::~encoder_t()
    {
    }

    key_insertion_record_t* encoder_t::_add_key(ckt_t& ckt_in, node_t* g, bool polarity)
    {
        std::string name = "keyinput" + boost::lexical_cast<std::string>(ckt_in.num_key_inputs());
        node_t* key_inp = node_t::create_input(name);
        ckt_in.add_key_input(key_inp);

        name = g->name + "$enc";
        std::string func = polarity ? "xnor" : "xor";
        node_t* enc_g = node_t::create_gate(name, func);
        enc_g->keygate = true;
        enc_g->add_input(key_inp);
        enc_g->add_input(g);
        ckt_in.add_gate(enc_g);

        if(polarity) {
            g->func = node_t::invert(g->func);
        }

        key_insertion_record_t* krec = new key_insertion_record_t(polarity, key_inp, g, enc_g);
        for(unsigned i=0; i != g->num_fanouts(); i++) {
            node_t* fi = g->fanouts[i];
            fi->rewrite_input(g, enc_g);
            krec->rewritten_gates.push_back(fi);
        }
        std::copy(g->fanouts.begin(), g->fanouts.end(), std::back_inserter(enc_g->fanouts));
        g->fanouts.clear();
        g->add_fanout(enc_g);
        key_inp->add_fanout(enc_g);

        if(g->output) {
            enc_g->output = true;
            g->output = false;
            ckt_in.replace_output(g, enc_g);
        }

        return krec;
    }

    void encoder_t::_remove_key(ckt_t& ckt_in, key_insertion_record_t* krec)
    {
        if(krec->inv_polarity) {
            krec->orig_gate->func = node_t::invert(krec->orig_gate->func);
        }
        if(krec->new_gate->output) {
            krec->orig_gate->output = true;
            ckt_in.replace_output(krec->new_gate, krec->orig_gate);
        }

        krec->orig_gate->fanouts.clear();
        for(unsigned i=0; i != krec->rewritten_gates.size(); i++) {
            krec->rewritten_gates[i]->rewrite_input(krec->new_gate, krec->orig_gate);
            krec->orig_gate->fanouts.push_back(krec->rewritten_gates[i]);
        }

        ckt_in.remove_key_input(krec->key_inp);
        delete krec->key_inp;

        for(unsigned i=0; i != krec->new_gates.size(); i++) {
            ckt_in.remove_gate(krec->new_gates[i]);
            delete krec->new_gates[i];
        }
        delete krec;
    }

    bool encoder_t::_bad_insertion() 
    {
        for(unsigned i=0; i != ckt.num_outputs(); i++) {
            if(ckt.count_bitmap(keymap, ckt.outputs[i]) == 1) {
                return true;
            }
        }
        return false;
    }

    int encoder_t::randomInsertion(int keys)
    {
        int inserted = 0;
        int init_pos = rand() % ckt.num_gates();
        for(int cur_pos = 0; cur_pos < (int)ckt.num_nodes() && inserted < keys; cur_pos ++) {
            int pos = (cur_pos + init_pos) % ckt.num_nodes();
            node_t* n = ckt.nodes[pos];

            if(n->is_keygate()) {
                continue;
            } else if(n->is_fanout_keygate()) {
                continue;
            } else {
                bool polarity = n->is_gate()? (!!(rand() & 1)) : false;
                key_insertion_record_t* kr = _add_key(ckt, n, polarity);
                delete kr;
                inserted += 1;
            }
        }
        return inserted;
    }

    bool encoder_t::recursiveEncodeEx(int keys, int inserted, key_insertion_records_t& records, const std::string& outfile)
    {
        int init_pos = rand() % ckt.num_gates();
        for(int cur_pos = 0; cur_pos < (int)ckt.num_nodes(); cur_pos ++) {
            int pos = (cur_pos + init_pos) % ckt.num_nodes();
            node_t* n = ckt.nodes[pos];

            if(n->is_keygate()) {
                continue;
            } else if(n->is_fanout_keygate()) {
                continue;
            } else {
                bool polarity = n->is_gate()? (!!(rand() & 1)) : false;
                key_insertion_record_t* kr = _add_key(ckt, n, polarity);
                int nonMutableCnt = 0;
                if(ckt.num_key_inputs() > 1) {
                    ckt.init_indices();
                    ckt.topo_sort();
                    ckt.init_keymap(keymap);

                    if(_bad_insertion()) {
                        _remove_key(ckt, kr);
                        continue;
                    }

                    ckt_eval_t sim(ckt, ckt.ckt_inputs);
                    tv_solver_t tvs(ckt, sim);
                    if(tvs.canSolveSingleKeys()) {
                        _remove_key(ckt, kr);
                        continue;
                    }

                    if(ckt.num_key_inputs() < 61) {
                        unsigned j = ckt.num_key_inputs() - 1; 
                        assert(j > 0); // j is the last key inserted.
                        for(unsigned i=0; i < j; i++) {
                            if(tvs.isNonMutable(i, j)) nonMutableCnt += 1;
                        }
                        if((nonMutableCnt + 5) < (int) ckt.num_key_inputs()) {
                            _remove_key(ckt, kr);
                            continue;
                        }
                    }
                }

                // update the inserted count.
                if((inserted+1) > max_keys) { 
                    max_keys = inserted + 1; 
                    std::cout << "max_keys=" << max_keys << std::endl;
                    _dump_to_file(outfile, false);
                }

                // recurse.
                if(recursiveEncodeEx(keys-1, inserted+1, records, outfile)) {
                    records.push_back(kr);
                    return true;
                } else {
                    _remove_key(ckt, kr);
                    continue;
                }
            }
        }

        return false;
    }

    bool encoder_t::recursiveEncode(int keys, int inserted, key_insertion_records_t& records, const std::string& outfile )
    {
        if(keys == 0) return true;

        int init_pos = rand() % ckt.num_gates();
        for(int cur_pos = 0; cur_pos < (int)ckt.num_nodes(); cur_pos ++) {
            int pos = (cur_pos + init_pos) % ckt.num_nodes();
            node_t* n = ckt.nodes[pos];

            if(n->is_keygate()) {
                continue;
            } else if(n->is_fanout_keygate()) {
                continue;
            } else {
                bool polarity = n->is_gate()? (!!(rand() & 1)) : false;
                key_insertion_record_t* kr = _add_key(ckt, n, polarity);
                if(ckt.num_key_inputs() > 1) {
                    ckt.init_indices();
                    ckt.topo_sort();
                    ckt.init_keymap(keymap);

                    if(_bad_insertion()) {
                        _remove_key(ckt, kr);
                        continue;
                    }

                    ckt_eval_t sim(ckt, ckt.ckt_inputs);
                    tv_solver_t tvs(ckt, sim);
                    std::vector<std::pair<int, int> > keysFound;
                    if(tvs.canSolveSingleKeys()) {
                        _remove_key(ckt, kr);
                        continue;
                    }
                }
                if((inserted+1) > max_keys) { 
                    max_keys = inserted + 1; 
                    std::cout << "max_keys=" << max_keys << std::endl;
                    _dump_to_file(outfile, false);
                }
                if(recursiveEncode(keys-1, inserted+1, records, outfile)) {
                    records.push_back(kr);
                    return true;
                } else {
                    _remove_key(ckt, kr);
                    continue;
                }
            }
        }
        return false;
    }

    void encoder_t::_dump_to_file(const std::string& outfile, bool force_dump) const
    {
        if(outfile.size() == 0) {
            if(force_dump) {
                std::cout << ckt << std::endl;
            }
        } else {
            std::ofstream fout(outfile.c_str());
            fout << ckt << std::endl;
            fout.close();
        }
    }

    int encoder_t::encode(int keys, const std::string& outfile, bool ex)
    {
        key_insertion_records_t records;
        int k1 = (int) ceil(keys * 0.75);
        int k2 = keys - k1;

        encode_fn_t fn = ex ? &encoder_t::recursiveEncodeEx : &encoder_t::recursiveEncode;
        int hard_insertions = -1;
        if((this->*fn)(k1, 0, records, outfile)) {
            randomInsertion(k2);
            hard_insertions = k1;
        }  else {
            randomInsertion(keys);
            hard_insertions = 0;
        }

        ckt.init_indices();
        ckt.topo_sort();
        _dump_to_file(outfile, true);
        delete_records(records);
        return hard_insertions;
    }

    void delete_records(key_insertion_records_t& r)
    {
        for(key_insertion_records_t::iterator it = r.begin(); it != r.end(); it++) {
            delete *it;
        }
        r.clear();
    }
}
