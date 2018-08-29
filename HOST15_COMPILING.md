Requirements
============

These instructions have been tested with 

1. Ubuntu 14.04.05 (Trust Tahir)
2. flex 2.5.35
3. bison 3.0.2
4. g++ 4.8.4
5. CPLEX 12.5

There may be other dependencies. Let me (pramod.subramanyan@gmail.com) know
if you find other dependencies that you would like listed here.

Compilation Instructions
========================

First build cudd-2.5.0:

    $ cd source/cudd-2.5.0/
    $ make
    $ cd obj
    $ make
    $ cd ../../..

Next build minisat:

    $ cd source/minisat
    $ export MROOT=$PWD
    $ cd core
    $ make libr
    $ cd ../../..

Next build CryptoMinisat:

    $ cd source/cmsat-2.9.9/
    $ mkdir build
    $ cd build
    $ ../configure
    $ make
    $ cd ../../..

Next we build lingeling:

    $ cd source/lingeling/
    $ ./configure.sh
    $ make
    $ cd ../..


Finally we can build our tool:

    $ cd source/src
    $ make

Check if the tool works with the following command:

    $ ./sld ../../benchmarks/rnd/c880_enc50.bench ../../benchmarks/original/c880.bench
