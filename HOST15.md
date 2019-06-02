Information
===========

The following is some information about the logic decryption tool described in
our paper: Evaluating the Security of Logic Encryption Algorithms.

The paper itself is available at: http://dl.dropbox.com/u/28673593/web/papers/host15.pdf.

Download
========

To clone this repository, use the following command.

    $ hg clone https://host15author@bitbucket.org/host15author/host15-logic-decryption

This assumes you have the tool mercurial installed. If you do not have
mercurial, you can also download a zip file by clicking the download button to
the left.  The icon for the download button is a cloud with an arrow pointing
downwards.

Directory Structure
===================

We have provided pre-built binaries for Ubuntu 64-bit Linux. It is suggested
that you use these binaries. You can also attempt to build the source in the
source/ directory; detailed instructions for this are in COMPILING.md.

The binaries of the decryption and verification tool are stored in the bin/
subdirectory.

The benchmarks are stored in the benchmarks/ subdirectory.  There are seven
subdirectories in this folder. These are:

1. iolts14
2. toc13mux
3. toc13xor
4. dtc10lut
5. rnd
6. dac12
7. original

The first six subdirectories correspond to the logic encryption schemes
evaluated in our paper. For all the schemes except DTC'10/LUT, we 
provide four encrypted versions for each benchmark. The naming convention
for these benchmarks is: <circuit>\_enc<PCT>.bench. PCT is a 2 digit
number which can be one of 05, 10, 25 and 50 and denotes the
percentage area overhead of encryption for this benchmark. 
For DTC'10/LUT there is only one file for each benchmark and this
file is named <circuit>\_enc.bench.

The original subdirectory contains the original, unencrypted benchmark
circuits.

Running The Decryption Tool
===========================

The decryption tool expects two arguments. The first argument is the
encrypted benchmark. The second argument is the original benchmark,
the latter is only used to evaluate the outputs for distinguishing
input patterns.

An example:

    $ cd bin
    $ ./sld ../benchmarks/rnd/c880_enc50.bench ../benchmarks/original/c880.bench

The output when this command is run is:

    inputs=60 keys=192 outputs=26 gates=590
    iteration: 1; vars: 3693; clauses: 4545; decisions: 4318
    iteration: 2; vars: 4749; clauses: 7151; decisions: 7160

    <many lines snipped>

    iteration: 47; vars: 52269; clauses: 2120; decisions: 128158
    finished solver loop.
    key=000000110011111011100110110100001001000100000001000010111010001100111111100100001101000010111000010000010011110000111111011001010000001110110011101011111010010100010101110000010110000110000101
    iteration=47; backbones_count=0; cube_count=156540; cpu_time=4.42028; maxrss=9.80859

The penultimate output line is the correct key value.  The last line is some
information about runtime, number of iterations etc.


Verifying Key Values
====================

We have also provided a utility to verify the key value. The program uses a SAT
formulation to verify circuit equality. The first argument to this utility is
the original unencrypted circuit, the second argument is the encrypted
circuit. The third argument must be a string of the form 'key=<keyvalue>'.

An example of its invocation:

    $ ./lcmp ../benchmarks/original/c880.bench ../benchmarks/rnd/c880_enc50.bench key=000000110011111011100110110100001001000100000001000010111010001100111111100100001101000010111000010000010011110000111111011001010000001110110011101011111010010100010101110000010110000110000101

The output will be the string 'equivalent' if the correct key is provided. If
not, the utility will output the string 'different'. The above command should
produce the output 'equivalent'. If any of the key bits are changed, it is very
likely that the utility will output 'different'.

Running the Partial-Break Algorithm
===================================

In order to run the partial-break algorithm use the -sT flag. To run the fault-analysis
attack described in [Rajendran et al., DAC'12], use the -tT flag. To run both
the partial-break and fault-analysis attack use the -stT flag. An example:


    $ ./sld -stT ../benchmarks/rnd/c880_enc50.bench ../benchmarks/original/c880.bench


