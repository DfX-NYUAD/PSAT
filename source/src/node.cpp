#include <algorithm>
#include <ctype.h>
#include <assert.h>
#include "node.h"
#include <boost/algorithm/string/predicate.hpp>
#include "ternarysat.h"

namespace ckt_n {
    node_t::node_t() 
    {
        output = false; 
        prov = NULL;
        prov3 = NULL;
        prov4 = NULL;
        prov5 = NULL;
        tprov = NULL;

        index = -1;
        level = -1;
        keyinput = false;
        keygate = false;
        newly_added = false;
    }

    node_t* node_t::create_input(const std::string name) 
    {
        node_t* n = new node_t();
        n->_init_input(name);
        return n;
    }

    node_t* node_t::create_gate( const std::string& name, const std::string& func_in )
    {
        node_t* n = new node_t();
        std::string func(func_in);
        std::transform(func.begin(), func.end(), func.begin(), ::tolower);
        if(func == "buff" || func == "buf")  {
            std::string f = "buf";
            n->_init_gate(name, f);
        } else {
            n->_init_gate(name, func);
        }
        return n;
    }

    void node_t::add_input(node_t* input)
    {
        inputs.push_back(input);
    }

    void node_t::rewrite_fanouts_with(node_t* repl)
    {
        for(unsigned i=0; i != fanouts.size(); i++) {
            node_t* fout = fanouts[i];
            fout->rewrite_input(this, repl);
            repl->add_fanout(fout);
        }
        fanouts.clear();
    }

    void node_t::rewrite_input(node_t* oldinp, node_t* newinp)
    {
        bool found = false;
        for(unsigned i=0; i != num_inputs(); i++) {
            if(inputs[i] == oldinp) {
                inputs[i] = newinp;
                found = true;
            }
        }
        assert(found);
    }

    void node_t::add_fanout(node_t* fout)
    {
        if(fanouts.end() == std::find(fanouts.begin(), fanouts.end(), fout)) {
            fanouts.push_back(fout);
        }
    }

    void node_t::_init_input(const std::string& n) 
    {
        type = INPUT; 
        name = n;
        func.clear();
        inputs.clear();
        fanouts.clear();
        output = false;
        keyinput = boost::starts_with(n, "keyinput");
    }

    void node_t::_init_gate( const std::string& o, const std::string& f ) 
    {
        type = GATE;
        name = o;
        func = f;
        std::transform(func.begin(), func.end(), func.begin(), ::tolower);
        inputs.clear();
        fanouts.clear();
        output = false;
    }

    const clause_provider_i<sat_n::Solver>* node_t::get_solver_provider()
    {
        assert(GATE == type);
        if(prov == NULL) {
            prov = ckt_n::get_provider<sat_n::Solver>(func, num_inputs());
        }
        return prov;
    }

    const clause_provider_i<sat_n::Solver>* node_t::get_ternary_solver_provider()
    {
        assert(GATE == type);
        if(tprov == NULL) {
            tprov = ckt_n::get_ternary_provider<sat_n::Solver>(func, num_inputs());
        }
        return tprov;
    }

    const clause_provider_i<AllSAT::AllSATCkt>* node_t::get_allsatckt_provider()
    {
        assert(GATE == type);
        if(prov3 == NULL) {
            prov3 = ckt_n::get_provider<AllSAT::AllSATCkt>(func, num_inputs());
        }
        return prov3;
    }

    const clause_provider_i<AllSAT::ClauseList>* node_t::get_clauselist_provider()
    {
        assert(GATE == type);
        if(prov4 == NULL) {
            prov4 = ckt_n::get_provider<AllSAT::ClauseList>(func, num_inputs());
        }
        return prov4;
    }

    const clause_provider_i<b2qbf::ISolver>* node_t::get_qbf_provider()
    {
        assert(GATE == type);
        if(prov5 == NULL) {
            prov5 = ckt_n::get_provider<b2qbf::ISolver>(func, num_inputs());
        }
        return prov5;
    }

    std::ostream& operator<<(std::ostream& out, const node_t& n)
    {
        if(node_t::INPUT == n.type) {
            out << "INPUT(" << n.name << ")";
        } else if(node_t::GATE == n.type) {
            out << n.name << " = " << n.func << "("
                << n.inputs << ")";
        } else {
            assert(false);
        }
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const nodeset_t& ns)
    {
        for(auto it = ns.begin(); it != ns.end(); it++) {
            out << (*it)->name << " ";
        }
        return out;
    }

    bool node_t::is_fanout_keygate() const
    {
        for(unsigned i=0; i != num_fanouts(); i++) {
            node_t* fo = fanouts[i];
            if(fo->is_keygate()) return true;
        }
        return false;
    }

    bool node_t::is_input_keyinput() const
    {
        for(unsigned i=0; i != num_inputs(); i++) {
            node_t* ni = inputs[i];
            if(ni->is_keyinput()) return true;
        }
        return false;
    }

    std::ostream& operator<<(std::ostream& out, const nodelist_t& nl)
    {
        if(nl.size() == 0) return out;
        else if(nl.size() == 1) {
            out << nl[0]->name;
        } else {
            for(unsigned i=0; i != nl.size(); i++) {
                out << nl[i]->name;
                if(i+1 != nl.size()) {
                    out << ", ";
                }
            }
        }
        return out;
    }

    std::string node_t::invert(const std::string& func)
    {
        static const std::string map[][2] = {
            { std::string("and"), std::string("nand") },
            { std::string("or"), std::string("nor") },
            { std::string("xor"), std::string("xnor") },
            { std::string("not"), std::string("buf") }
        };
        static const int MAP_SIZE = sizeof(map) / sizeof(map[0]);
        for(int i=0; i != MAP_SIZE; i++) {
            if(func == map[i][0]) return map[i][1];
            else if(func == map[i][1]) return map[i][0];
        }
        std::cout << "func=" << func << std::endl;
        assert(false);
        return "invalid";
    }
    void node_t::remove_input(node_t* inp)
    {
        auto pos = std::find(inputs.begin(), inputs.end(), inp);
        assert(pos != inputs.end());
        inputs.erase(pos);
    }

    void node_t::remove_fanout(node_t* fout)
    {
        auto pos = std::find(fanouts.begin(), fanouts.end(), fout);
        assert(pos != fanouts.end());
        fanouts.erase(pos);
    }

    void node_t::const_propagate(node_t* input, bool value)
    {
        if(is_input()) {
            assert(input == NULL);
            for(auto it = fanouts.begin(); it != fanouts.end(); it++) {
                node_t* fout = *it;
                fout->const_propagate(this, value);
            }
            fanouts.clear();
        } else {
            assert(is_gate());
            remove_input(input);
            if(func == "and" || func == "nand") {
                // value = 0
                if(!value) {
                    // rewrite fanout because this gate output is constant.
                    for(auto it = fanouts.begin(); it != fanouts.end(); it++) {
                        node_t *fout = *it;
                        if(func == "and") {
                            // and means output is zero.
                            fout->const_propagate(this, false);
                        } else {
                            // nand means output is one.
                            fout->const_propagate(this, true);
                        }
                    }
                    fanouts.clear();
                }
            } else if(func == "or" || func == "nor") {
                if(value) {
                    // rewrite fanout because this gate output is constant.
                    for(auto it = fanouts.begin(); it != fanouts.end(); it++) {
                        node_t* fout = *it;
                        if(func == "or") {
                            // or means output is one.
                            fout->const_propagate(this, true);
                        } else {
                            // nor means output is zero.
                            fout->const_propagate(this, false);
                        }
                    }
                    fanouts.clear();
                }
            } else if(func == "not" || func == "buf") {
                // have to rewrite fanouts.
                for(auto it = fanouts.begin(); it != fanouts.end(); it++) {
                    node_t* fout = *it;
                    bool new_value = (func == "not") ? !value : value;
                    fout->const_propagate(this, new_value);
                }
                fanouts.clear();
            } else if(func == "xor" || func == "xnor") {
                // one input has already been removed!
                assert(num_inputs() == 1);
                if((func == "xor"  && value == 1) ||
                   (func == "xnor" && value == 0)) 
                {
                    func = "not";
                } else {
                    assert((func == "xor"  && value == 0) ||
                           (func == "xnor" && value == 1));
                    func = "buf";
                }
            }
            if(num_inputs() == 1) {
                if(func == "and" || func == "or") {
                    func = "buf";
                } else if(func == "nand" || func == "nor") {
                    func = "not";
                }
            }
        }
    }

    bool node_t::is_reachable(node_t* n, int depth) const
    {
        if(depth < 0) return false;
        if(n == this) return true;
        for(unsigned i=0; i != num_inputs(); i++) {
            if(inputs[i]->is_reachable(n, depth-1)) return true;
        }
        return false;
    }

    double node_t::calc_out_prob(std::vector<double>& p_in) const
    {
        assert(p_in.size() == inputs.size());
        if(func == "and") {
            double p = 1;
            for(unsigned i=0; i != p_in.size(); i++) {
                p *= p_in[i];
            }
            return p;
        } else if(func == "nand") {
            double p = 1;
            for(unsigned i=0; i != p_in.size(); i++) {
                p *= p_in[i];
            }
            return 1-p;
        } else if(func == "or") {
            double p = 1;
            for(unsigned i=0; i != p_in.size(); i++) {
                p *= (1-p_in[i]);
            }
            return 1-p;
        } else if(func == "nor") {
            double p = 1;
            for(unsigned i=0; i != p_in.size(); i++) {
                p *= (1-p_in[i]);
            }
            return p;
        } else if(func == "xor") {
            assert(p_in.size() == 2);
            double p0 = p_in[0];
            double p1 = p_in[1];
            return p0*(1-p1) + (1-p0)*p1;
        } else if(func == "xnor") {
            assert(p_in.size() == 2);
            double p0 = p_in[0];
            double p1 = p_in[1];
            return p0*p1 + (1-p0)*(1-p1);
        } else if(func == "not") {
            assert(p_in.size() == 1);
            return 1-p_in[0];
        } else if(func == "buf") {
            assert(p_in.size() == 1);
            return p_in[0];
        } else if(func == "mux") {
            assert(p_in.size() == 3);
            double ps = p_in[0];
            double pa = p_in[1];
            double pb = p_in[2];
            return ps*pb + (1-ps)*pa;
        } else {
            assert(false);
            return -1;
        }
    }
}
