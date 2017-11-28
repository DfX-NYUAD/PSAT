#ifndef _ENCODER_H_DEFINED_
#define _ENCODER_H_DEFINED_

#include "ckt.h"
#include "ast.h"
#include <list>

namespace ckt_n
{
    struct key_insertion_record_t
    {
        bool inv_polarity;
        node_t* key_inp;
        node_t* orig_gate;
        node_t* new_gate;

        nodelist_t new_gates;
        nodelist_t rewritten_gates;

        key_insertion_record_t(bool ip, node_t* ki, node_t* og, node_t* ng)
            : inv_polarity(ip)
            , key_inp(ki)
            , orig_gate(og)
            , new_gate(ng)
        {
            new_gates.push_back(ng);
        }
    };

    typedef std::list<key_insertion_record_t*> key_insertion_records_t;
    void delete_records(key_insertion_records_t& r);

    struct encoder_t
    {
        ckt_t ckt;              // the copy of the circuit we insert keys into.
        ckt_t::node_map_t nm;   // the mapping between the modified and the original circuit.
        ckt_t ckt_o;            // the original circuit.
        int max_keys;
        std::vector< std::vector<bool> > keymap;

        static key_insertion_record_t* _add_key(ckt_t& ckt_in, node_t* g, bool polarity);
        static void _remove_key(ckt_t& ckt_in, key_insertion_record_t* krec);
        bool _bad_insertion();
        void _dump_to_file(const std::string& outfile, bool force_dump) const;

        encoder_t(ast_n::statements_t& stms, int seed);
        ~encoder_t();

        int encode(int keys, const std::string& outputfile, bool ex);
        bool recursiveEncode(int keys, int inserted, key_insertion_records_t& records, const std::string& outfile);
        bool recursiveEncodeEx(int keys, int inserted, key_insertion_records_t& records, const std::string& outfile);
        int randomInsertion(int keys);

        typedef bool (encoder_t::*encode_fn_t)(int keys, int inserted, key_insertion_records_t& records, const std::string& outfile);
    };
}

#endif
