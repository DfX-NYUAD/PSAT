#include "sim.h"
#include "util.h"

namespace ckt_n {

    eval_t::eval_t(ckt_t& c) 
        : ckt(c)
    {
        ckt.init_solver(S, mappings);

	// JOHANN
	//
	for (const auto* gate : ckt.gates_sorted) {

		// boolean evaluation in eval() only for 2 inputs as of now
		assert(gate->inputs.size() <= 2);
	}
    }

    void eval_t::set_cnst(node_t* n, int val)
    {
        using namespace sat_n;

        assert(n->is_keyinput());

        assert(val == 0 || val == 1);

        int idx = n->get_index();
        assert(idx < (int) mappings.size());

        Lit v = (val == 0) ? ~mappings[idx] : mappings[idx];
        S.addClause(v);
    }
    
    void eval_t::eval(nodelist_t& input_nodes, const bool_vec_t& input_values, bool_vec_t& outputs)
    {
	// JOHANN
	//

	if (ckt_n::DBG) {
		std::cout << "Inputs: " << input_values << std::endl;
	}

	// assign input values to input nodes
	//
	for (std::size_t i = 0; i < ckt.ckt_inputs.size(); i++) {
		ckt.ckt_inputs[i]->output_bit = input_values[i];
	}

	// now, evaluate all gates' outputs, by traversing the sorted netlist graph
	//
	for (auto* gate : ckt.gates_sorted) {

		if (gate->func == "and") {
			gate->output_bit = gate->inputs[0]->output_bit && gate->inputs[1]->output_bit;
		}
		else if (gate->func == "nand") {
			gate->output_bit = !(gate->inputs[0]->output_bit && gate->inputs[1]->output_bit);
		}
		else if (gate->func == "or") {
			gate->output_bit = gate->inputs[0]->output_bit || gate->inputs[1]->output_bit;
		}
		else if (gate->func == "nor") {
			gate->output_bit = !(gate->inputs[0]->output_bit || gate->inputs[1]->output_bit);
		}
		else if (gate->func == "xor") {
			gate->output_bit = gate->inputs[0]->output_bit ^ gate->inputs[1]->output_bit;
		}
		else if (gate->func == "xnor") {
			gate->output_bit = !(gate->inputs[0]->output_bit ^ gate->inputs[1]->output_bit);
		}
		else if (gate->func == "not") {
			gate->output_bit = !gate->inputs[0]->output_bit;
		}
		else if (gate->func == "buf") {
			gate->output_bit = gate->inputs[0]->output_bit;
		}
		else {
			std::cout << "ERROR: unsupported function for gate " << gate->name << ": \"" << gate->func << "\"" << std::endl;
		}

		if (ckt_n::DBG_VERBOSE) {
			std::cout << "Gate " << gate->name << std::endl;
			std::cout << " func: " << gate->func << std::endl;
			std::cout << " inputs[0]: " << gate->inputs[0]->output_bit << std::endl;
			if (gate->inputs.size() > 1) {
				std::cout << " inputs[1]: " << gate->inputs[1]->output_bit << std::endl;
			}
			std::cout << " output_bit: " << gate->output_bit << std::endl;
		}
	}

	// memorize output values in vector
        outputs.resize(ckt.num_outputs());
	for (std::size_t i = 0; i < ckt.outputs.size(); i++) {
		outputs[i] = ckt.outputs[i]->output_bit;
	}

	if (ckt_n::DBG) {
		std::cout << "Outputs (graph evaluation): " << outputs << std::endl;
	}

	// original function; run for DBG mode only
	//
	if (ckt_n::DBG) {

	bool_vec_t outputs_SAT;

        using namespace sat_n;

        assert(input_nodes.size() == input_values.size());

        vec_lit_t assump;
        for(unsigned i=0; i != input_values.size(); i++) {
            Lit l = input_values[i] ? 
                mappings[input_nodes[i]->get_index()] : 
                ~mappings[input_nodes[i]->get_index()];
            assump.push(l);
        }

        bool result = S.solve(assump);
        if(!result) {
            std::cout << "inputs: ";
            for(unsigned i=0; i != input_nodes.size(); i++) {
                std::cout << input_nodes[i]->name << " ";
            }
            std::cout << std::endl;
        }
        assert(true == result);

        outputs_SAT.resize(ckt.num_outputs());
        for(unsigned i=0; i != outputs_SAT.size(); i++) {
            int idx = ckt.outputs[i]->get_index();
            lbool vi = S.modelValue(var(mappings[idx]));
            assert(vi.isDef());
            outputs_SAT[i] = (vi.getBool());
        }

	std::cout << "Outputs (SAT evaluation): " << outputs_SAT << std::endl;
	assert(outputs == outputs_SAT);
	}
    }

    void ckt_eval_t::eval(
        const std::vector<bool>& input_values,
        std::vector<bool>& output_values
    )
    {
        sim.eval(inputs, input_values, output_values);
    }

    void convert(uint64_t v, bool_vec_t& result)
    {
        for(unsigned i = 0; i < result.size(); i++) {
            uint64_t mask = (uint64_t)((uint64_t)1 << i);
            if((v & mask) != 0) {
                result[i] = true;
            } else {
                result[i] = false;
            }
        }
    }

    uint64_t convert(const bool_vec_t& v) {
        uint64_t r = 0;
        for(unsigned i=0; i < v.size(); i++) {
            uint64_t mask = (uint64_t)((uint64_t)1 << i);
            if(v[i]) r |= mask;
        }
        return r;
    }

    void dump_mappings(std::ostream& out, ckt_t& ckt, index2lit_map_t& mappings)
    {
        for(unsigned i=0; i != ckt.num_inputs(); i++) {
            std::cout << "input " << ckt.inputs[i]->name << " --> " 
                      << mappings[ckt.inputs[i]->get_index()] << std::endl;
        }
        for(unsigned i=0; i != ckt.num_outputs(); i++) {
            std::cout << "output " << ckt.outputs[i]->name << " --> " 
                      << mappings[ckt.outputs[i]->get_index()] << std::endl;
        }
        for(unsigned i=0; i != ckt.num_gates(); i++) {
            std::cout << "gate " << ckt.gates[i]->name << " --> " 
                      << mappings[ckt.gates[i]->get_index()] << std::endl;
        }
    }
}
