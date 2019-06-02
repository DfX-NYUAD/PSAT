#include <assert.h>
#include "ast.h"

namespace ast_n
{
    statements_t* statements;

    std::ostream& operator<<(std::ostream& out, const input_decl_t& i)
    {
        out << "INPUT(" << i.name << ")";
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const output_decl_t& i)
    {
        out << "OUTPUT(" << i.name << ")";
        return out;
    }
    std::ostream& operator<<(std::ostream& out, const gate_inputs_t& inps)
    {
        if(inps.size() == 0) { return out; }
        else if(inps.size() == 1) { 
            out << inps[0];
            return out;
        } else {
            for(unsigned i = 0; i != inps.size(); i++) {
                out << inps[i];
                if(i+1 != inps.size()) {
                    out << ", ";
                }
            }
            return out;
        }
    }
    std::ostream& operator<<(std::ostream& out, const gate_decl_t& g)
    {
        out << g.output << " = " << g.function
            << "(" << g.inputs << ")";
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const statement_t& s)
    {
        if(statement_t::INPUT == s.type) {
            out << *s.input;
        } else if(statement_t::OUTPUT == s.type) {
            out << *s.output;
        } else if(statement_t::GATE == s.type) {
            out << *s.gate;
        } else {
            assert(false);
        }
        return out;
    }
}
