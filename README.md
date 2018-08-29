# Probabilistic SAT (PSAT) attack

This is a probabilistic SAT attack tool developed on top of the SAT attack code originally developed by 
Pramod Subramanyani (the latter is available at https://bitbucket.org/spramod/host15-logic-encryption).

We provide the binary of the attack located in */bin* and associated benchmarks in */benchmarks*. The complete attack code shall
be made available as open source to the research community in the near future. This binary has been tested on 64-bit Ubuntu 16.04 LTS. To run the (dynamically linked) binary, default C++ libraries are required as well as OpenMP. Also run
	*ldd bin/sld*
for more details.

To invoke the attack use the *sld* command in the following manner:

	cd bin
	./sld  ../benchmarks/PSAT/probabilistic/c432_RTL_rnd32.bench ../benchmarks/ORIGINAL/c432_RTL.bench PASSWORD

The password is currently only known to the reviewers.

The output when this command is run is something like this:

	Stochastic gates count: 20
	Sampling of output observations for stochastic circuits: on
	Sampling iterations (for each pattern): 1000
	inputs=36 keys=32 outputs=7 gates=238

	<many lines snipped>

	Verifying key for 10000 test patterns ...
	5 % done ...
	10 % done ...
	15 % done ...
	20 % done ...

	<many lines snipped>

	Average Hamming distance: 6.24429 %
	 These metrics DON'T cover the sampling of I/O patterns to mitigate the stochastic behaviour of the circuit, i.e., testing considers any output pattern as is, even if they are erroneous.
	key=01001110000000000101110111100000

	iteration=13; backbones_count=0; cube_count=16638; cpu_time=1.432; maxrss=8.5

The penultimate output line is the inferred key value.


We enable the probabilistic computation internally, so the user has to provide a corresponding file (*.stoch*, residing in the same folder as the locked/camouflaged netlist). Examples can be found in the directory
	*benchmarks/stoch*.

A short example is also given below:

	# OUTPUT_SAMPLING_ON OUTPUT_SAMPLING_ITERATIONS OUTPUT_SAMPLING_FOR_TEST_ON TEST_PATTERNS
	true 1000 false 10000
	# GATE_NAME ERROR_RATE(%) [POLYMORPHIC_GATE FCT_1 PROBABILITY_FOR_FCT_1(%) ... [FCT_n PROBABILITY_FOR_FCT_n(%)]]
	# FCT can be any of AND, NAND, OR, NOR, XOR, XNOR, INV, BUF
	NEXT_GATE
	n_72 5
	NEXT_GATE
	n_116 5 POLYMORPHIC_GATE NAND 50 NOR 50
	NEXT_GATE
	n_118 0 POLYMORPHIC_GATE NAND 50 NOR 50

This means the gates *n\_72*, *n\_116* each have a probability of correctness (or error) of 95% (or 5%). Furthermore, the gates *n\_116* and *n\_118* are polymorphic and can act as NAND 50% of the time and as NOR 50% of the time.

Note that definitions for probabilistic and polymorphic behaviour can be set up independently, but their effects will combine. In other words, here *n\_116* behaves as NAND/NOR/AND/OR with 47.5/47.5/2.5/2.5% chances, respectively. Also note that polymorphic behavior, for the SAT solver, imposes just another form of errors.


To execute the conventional/original SAT attack, simply change the first flag related to *OUTPUT_SAMPLING* from *true* to *false* in the header of the *.stoch* file

Depending on the error rates, the attack may fail:

	Stochastic gates count: 20
	Sampling of output observations for stochastic circuits: off
	inputs=36 keys=32 outputs=7 gates=238

	<many lines snipped>

	Verifying key for 10000 test patterns ...
	UNSAT model!
	key=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

	iteration=8; backbones_count=0; cube_count=10700; cpu_time=0.024; maxrss=7.5625

In those cases, the attack can be tried several times, to gauge the randomized error and attack success for multiple runs.
