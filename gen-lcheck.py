#! /usr/bin/python2.7
import sys
import os
import stat
from string import Template

def find_files(d):
    files = os.listdir(d)
    return [os.path.join(d, f) for f in files if f.endswith('.bench')]


script = Template("""#PBS -l walltime=30:00:00
cd $curdir
./lcheck -o $graphoutfile $infile >& $logfile
./max-clique.py $graphoutfile >& $cliqueoutfile""")

def create_script(infile, outdir):
    basename = os.path.basename(infile)
    script_name = os.path.join(outdir, basename[:basename.find('.bench')] + '_lcheck.sh')

    graphoutfile = os.path.join(outdir, basename.replace('.bench', '.keygraph'))
    cliqueoutfile = os.path.join(outdir, basename.replace('.bench', '.clique'))
    logfile = script_name.replace('.sh', '.out')
    with open(script_name, 'wt') as fileobj:
        print >> fileobj, script.substitute(curdir=os.getcwd(), graphoutfile=graphoutfile, infile=infile, logfile=logfile, cliqueoutfile=cliqueoutfile)

    os.chmod(script_name, stat.S_IREAD + stat.S_IWRITE + stat.S_IEXEC)
    

# argument 1 - the location of the encrypted benchmarks.

fs = find_files(sys.argv[1])
for f in fs:
    create_script(f, sys.argv[1])

