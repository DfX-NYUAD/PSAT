#include "sim.h"
#include "util.h"
#include <unordered_map>
#include <map>

// JOHANN
#include <chrono>

namespace ckt_n {

    eval_t::eval_t(ckt_t& c) 
        : ckt(c)
    {
        ckt.init_solver(S, mappings);

	// JOHANN
	//
	for (auto* gate : ckt.gates) {

		// sanity check for inputs; boolean evaluation in eval() implemented only for 2 inputs
		assert(gate->inputs.size() <= 2);

		// assign more efficient unsigned enum to gates; avoids repetitive calls to str::compare in eval()
		if (gate->func == "and") {
			gate->func_enum = FUNC::AND;
		}
		else if (gate->func == "nand") {
			gate->func_enum = FUNC::NAND;
		}
		else if (gate->func == "or") {
			gate->func_enum = FUNC::OR;
		}
		else if (gate->func == "nor") {
			gate->func_enum = FUNC::NOR;
		}
		else if (gate->func == "xor") {
			gate->func_enum = FUNC::XOR;
		}
		else if (gate->func == "xnor") {
			gate->func_enum = FUNC::XNOR;
		}
		else if (gate->func == "not") {
			gate->func_enum = FUNC::INV;
		}
		else if (gate->func == "buf") {
			gate->func_enum = FUNC::BUF;
		}
		else {
			gate->func_enum = FUNC::UNDEF;
		}
	}

	// init srand with long and high-resolution timing seed
	//
	auto now = std::chrono::high_resolution_clock::now();
	auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
	srand(nanos);
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

	if (ckt_n::DBG_VERBOSE) {
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

		if (gate->func_enum == FUNC::AND) {
			gate->output_bit = gate->inputs[0]->output_bit && gate->inputs[1]->output_bit;
		}
		else if (gate->func_enum == FUNC::NAND) {
			gate->output_bit = !(gate->inputs[0]->output_bit && gate->inputs[1]->output_bit);
		}
		else if (gate->func_enum == FUNC::OR) {
			gate->output_bit = gate->inputs[0]->output_bit || gate->inputs[1]->output_bit;
		}
		else if (gate->func_enum == FUNC::NOR) {
			gate->output_bit = !(gate->inputs[0]->output_bit || gate->inputs[1]->output_bit);
		}
		else if (gate->func_enum == FUNC::XOR) {
			gate->output_bit = gate->inputs[0]->output_bit ^ gate->inputs[1]->output_bit;
		}
		else if (gate->func_enum == FUNC::XNOR) {
			gate->output_bit = !(gate->inputs[0]->output_bit ^ gate->inputs[1]->output_bit);
		}
		else if (gate->func_enum == FUNC::INV) {
			gate->output_bit = !gate->inputs[0]->output_bit;
		}
		else if (gate->func_enum == FUNC::BUF) {
			gate->output_bit = gate->inputs[0]->output_bit;
		}
		else {
			std::cout << "ERROR: unsupported function for gate " << gate->name << ": \"" << gate->func << "\"" << std::endl;
		}

		// post-process stochastic gates: they may experience flipping of their outputs depending on their error rate
		//
		if (gate->error_rate > 0.0) {

			if (
					// random double between 0.0 and 100.0, excluding 100.0 itself
					(100.0 * (static_cast<double>(rand()) / RAND_MAX))
					// chances that the random value falls below error_rate are approx error_rate %
					< gate->error_rate
			   ) {
				gate->output_bit = !gate->output_bit;
			}
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

	if (ckt_n::DBG_VERBOSE) {
		std::cout << "Outputs (graph evaluation): " << outputs << std::endl;
	}

	// original function; run for DBG mode only
	//
	if (ckt_n::DBG_SAT) {

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
		if (ckt_n::DBG_VERBOSE) {
		    std::cout << "inputs: ";
		}
            for(unsigned i=0; i != input_nodes.size(); i++) {
                std::cout << input_nodes[i]->name << " ";
            }
		if (ckt_n::DBG_VERBOSE) {
		    std::cout << std::endl;
		}
        }
        assert(true == result);

        outputs_SAT.resize(ckt.num_outputs());
        for(unsigned i=0; i != outputs_SAT.size(); i++) {
            int idx = ckt.outputs[i]->get_index();
            lbool vi = S.modelValue(var(mappings[idx]));
            assert(vi.isDef());
            outputs_SAT[i] = (vi.getBool());
        }

	if (ckt_n::DBG_VERBOSE) {
		std::cout << "Outputs (SAT evaluation): " << outputs_SAT << std::endl;
	}
	assert(outputs == outputs_SAT);
	}
    }

    void ckt_eval_t::eval(
        const std::vector<bool>& input_values,
        std::vector<bool>& output_values
    )
    {
	    // JOHANN
	    //
	    // sample the output for the input several times, and pick only the most common observation as ground truth to be used for further SAT solving
	    //
	    if (sim.ckt.IO_sampling_flag) {

		    std::unordered_map<std::vector<bool>, unsigned> output_samples_counts;
		    std::multimap<unsigned, std::vector<bool>, std::greater<unsigned>> output_samples_sorted;

		    // sample outputs N times (for the same input), track the counts of the different observed output patterns, select the most promising one as ground truth for
		    // this input pattern
		    //
		    for (unsigned i = 0; i < sim.ckt.IO_sampling_iter; i++) {

			    sim.eval(inputs, input_values, output_values);

			    // first occurrence of this pattern, init map for this key/pattern
			    if (output_samples_counts.find(output_values) == output_samples_counts.end()) {
				    output_samples_counts[output_values] = 1;
			    }
			    else {
				    output_samples_counts[output_values]++;
			    }

			    if (ckt_n::DBG_VERBOSE) {
				    std::cout << "Output: " << output_values << std::endl;
				    std::cout << " Samples count: " << output_samples_counts[output_values] << std::endl;
			    }
		    }

		    // convert sample counts into sorted multimap, with highest counts coming first
		    //
		    for (auto const& count : output_samples_counts) {

			    output_samples_sorted.emplace(std::make_pair(
						    count.second,
						    count.first
					    ));
		    }

		    if (ckt_n::DBG) {
			    for (auto const& sample : output_samples_sorted) {
				    std::cout << "Output: " << sample.second << std::endl;
				    std::cout << " Samples count: " << sample.first << std::endl;
			    }
		    }

		    // in case the most common pattern is dominant, i.e., occurs more frequently than the next two patterns taken together, consider it directly as ground truth
		    auto iter = output_samples_sorted.begin();
		    unsigned first = (*iter).first;
		    ++iter;
		    unsigned second_third = (*iter).first;
		    ++iter;
		    second_third += (*iter).first;

		    if (first > second_third) {
			output_values = (*output_samples_sorted.begin()).second;
		    }
		    // otherwise, even the most common pattern is not dominant
		    // then, randomly select a pattern based on its occurrence
		    // (the higher its count, the more likely a pattern will be chosen)
		    //
		    else {
			    // the random value is between [0, N]; the pattern which falls within that range will be picked
			    unsigned r = rand() % (sim.ckt.IO_sampling_iter + 1);
			    if (ckt_n::DBG) {
				    std::cout << "r: " << r << std::endl;
			    }

			    unsigned count = 0;
			    for (auto const& sample : output_samples_sorted) {

				    count +=  sample.first;

				    // the first pattern where the cumulative count goes just at or beyond the random value should be the one to pick; this is to mimic a random
				    // selection weighted by the counts
				    if (count >= r) {
					    output_values = sample.second;
					    break;
				    }
			    }
		    }
		    
		    if (ckt_n::DBG) {
			    std::cout << "Consider output: " << output_values << std::endl;
			    std::cout << std::endl;
		    }
	    }

	    // JOHANN
	    // original code, calls evaluation only once
	    //
	    else {
		    sim.eval(inputs, input_values, output_values);
	    }
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
