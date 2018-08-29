#ifndef _AST_H_DEFINED_
#define _AST_H_DEFINED_

#include <string>
#include <vector>
#include <iostream>
#include <assert.h>

namespace ast_n {
    struct input_decl_t {
        std::string name;
        input_decl_t(std::string& n) : name(n) {}
    };
    std::ostream& operator<<(std::ostream& out, const input_decl_t& i);

    struct output_decl_t {
        std::string name;
        output_decl_t(std::string& n) : name(n) {}
    };
    std::ostream& operator<<(std::ostream& out, const output_decl_t& i);

    typedef std::vector< std::string > gate_inputs_t;
    struct gate_decl_t {
        std::string     output;
        std::string     function;
        gate_inputs_t   inputs;

        gate_decl_t(const std::string& o, const std::string& f,
                    const std::vector<std::string>& i)
            : output(o)
            , function(f)
            , inputs(i)
        {
        }
    };
    std::ostream& operator<<(std::ostream& out, const gate_inputs_t& i);
    std::ostream& operator<<(std::ostream& out, const gate_decl_t& i);

    struct statement_t {
        enum { INPUT, OUTPUT, GATE } type;
        union {
            input_decl_t        *input;
            output_decl_t       *output;
            gate_decl_t         *gate;
        };

        statement_t(input_decl_t* i) : type(INPUT), input(i) {}
        statement_t(output_decl_t* o) : type(OUTPUT), output(o) {}
        statement_t(gate_decl_t* g) : type(GATE), gate(g) {}
        statement_t(const statement_t& other) 
            : type(other.type)
        {
            if(INPUT == type) {
                input = new input_decl_t(*other.input);
            } else if(OUTPUT == type) {
                output = new output_decl_t(*other.output);
            } else if(GATE == type) {
                gate = new gate_decl_t(*other.gate);
            } else {
                assert(false);
            }
        }
        ~statement_t() {
            if(INPUT == type) {
                if(input) delete input;
            } else if(OUTPUT == type) {
                if(output) delete output;
            } else if(GATE == type) { 
                if(gate) delete gate;
            } else assert(false);
        }
    };
    std::ostream& operator<<(std::ostream& out, const statement_t& i);

    typedef std::vector< statement_t > statements_t;
    extern statements_t* statements;
}
#endif // _AST_H_DEFINED_
