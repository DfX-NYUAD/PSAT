#! /usr/bin/python2.7
import sys
import os
import stat
from string import Template

def find_files(d):
    files = os.listdir(d)
    return [os.path.join(d, f) for f in files if f.endswith('.bench')]


script = Template("""#PBS -l walltime=36:00:00
#PBS -o $stdout
#PBS -e $stderr
cd $outdir
./sle -i -f 0.05 $infile -o ${base}_enc05.bench
./sle -i -f 0.10 $infile -o ${base}_enc10.bench
./sle -i -f 0.25 $infile -o ${base}_enc25.bench
./sle -i -f 0.50 $infile -o ${base}_enc50.bench
""")

# argument 1 bench dir
# argument 2 output dir

def create_script(infile, outdir):
    basename = os.path.basename(infile)
    script_name = os.path.join(outdir, basename[:basename.find('.bench')] + '.sh')

    base = os.path.join(outdir, basename.replace('.bench', ''))
    stdout = os.path.join(outdir, basename.replace('.bench', '.out'))
    stderr = os.path.join(outdir, basename.replace('.bench', '.out'))
    with open(script_name, 'wt') as fileobj:
        print >> fileobj, script.substitute(
            stdout=stdout, 
            stderr=stderr,
            outdir=outdir,
            infile=infile,
            base=base)

    os.chmod(script_name, stat.S_IREAD + stat.S_IWRITE + stat.S_IEXEC)

fs = find_files(sys.argv[1])
if not os.path.exists(sys.argv[2]): os.mkdir(sys.argv[2])
for f in fs:
    create_script(f, sys.argv[2])
