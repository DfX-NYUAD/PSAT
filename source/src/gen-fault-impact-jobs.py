#! /usr/bin/python2.7
import sys
import os
import stat
from string import Template

def find_files(d):
    files = os.listdir(d)
    return [os.path.join(d, f) for f in files if f.endswith('.bench')]


script = Template("""#PBS -l walltime=36:00:00
cd $outdir
./sle -t -f 0.1 $infile >& $outfile""")

# argument 1 bench dir
# argument 2 output dir

def create_script(infile, outdir):
    basename = os.path.basename(infile)
    script_name = os.path.join(outdir, basename[:basename.find('.bench')] + '.sh')

    outfile = os.path.join(outdir, basename.replace('.bench', '.faultimpact'))
    with open(script_name, 'wt') as fileobj:
        print >> fileobj, script.substitute(outdir=outdir, outfile=outfile, infile=infile)

    os.chmod(script_name, stat.S_IREAD + stat.S_IWRITE + stat.S_IEXEC)

fs = find_files(sys.argv[1])
if not os.path.exists(sys.argv[2]): os.mkdir(sys.argv[2])
for f in fs:
    create_script(f, sys.argv[2])
