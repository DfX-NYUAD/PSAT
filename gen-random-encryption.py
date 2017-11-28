#! /usr/bin/python2.7

import sys
import os
import stat

from string import Template

def find_files(d):
    files = os.listdir(d)
    return [os.path.join(d, f) for f in files if f.endswith('.bench')]


#script = Template("""#! /bin/bash
#PBS -l walltime=36:00:00
#cd $curdir
#./sle -r1 -f0.10 $infile >& ${outfile}10.bench
#./sle -r1 -f0.25 $infile >& ${outfile}25.bench
#./sle -r1 -f0.50 $infile >& ${outfile}50.bench
#./sle -r1 -f0.75 $infile >& ${outfile}75.bench""")

script = Template("""#PBS -l walltime=36:00:00
cd $curdir
./sle -r1 -f0.05 $infile >& ${outfile}05.bench""")

def create_script(infile, outdir):
    basename = os.path.basename(infile)
    script_name = os.path.join(outdir, basename[:basename.find('.bench')] + '.sh')

    outfile = script_name.replace('.sh', '_enc')
    with open(script_name, 'wt') as fileobj:
        print >> fileobj, script.substitute(curdir=os.getcwd(), outfile=outfile, infile=infile)

    os.chmod(script_name, stat.S_IREAD + stat.S_IWRITE + stat.S_IEXEC)
    

# argument 1 - the location of the original benchmarks.
# argument 2 - the location of the encrypted benchmarks.

fs = find_files(sys.argv[1])
if not os.path.exists(sys.argv[2]): os.mkdir(sys.argv[2])
for f in fs:
    create_script(f, sys.argv[2])
