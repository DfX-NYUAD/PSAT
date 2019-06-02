# Logic Locking

The sle binary can be used for logic encryption/logic locking. It supports the following locking methods:

1. Random Insertion (Called "RND" in the paper.)
2. MUX insertion based on Fault Impact analysis (Called "ToC'13/MUX in the paper.)
3. XOR insertion based on Fault Impact analysis (Called "Toc'13/XOR in the paper.)
4. AND/OR insertion based on IOLTS'14. (Called IOLTS'14 in the paper.)
5. Interference clique based encryption. (Called DAC'12 in the paper.)

Most of these are pretty straightforward to invoke, run `sle -h` for details.

## DAC'12 Locking

For the DAC12 clique-based encryption, you will need to first generate the "graph" file using a command somewhat like this:

    $ ./sle -M c432.mut ../../benchmarks/original/c432.bench 

Then generate clique information, using the clique analysis tool:

    $ cd clique-analysis
    $ ./mut2cnf.py ../c432.mut 

Note you may have to build the clique-analysis tool using the makefile first.

Then run the sle command with the graph and clique files.

    ./sle -f 0.2 -d -g c432.mut -C c432.clique -o c432_dac12enc.bench ../../benchmarks/original/c432.bench 


## ToC'13 : Fault Impact Based Encoding

The XOR encoding can be done as follows.

    $ ./sle -t -f 0.2 -o c432_toc13xor_enc20.bench ../../benchmarks/original/c432.bench 

The MUX encoding can be done as follows.

    $ ./sle -t -s -f 0.2 -o c432_toc13mux_enc20.bench ../../benchmarks/original/c432.bench 

