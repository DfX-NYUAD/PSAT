#! /usr/bin/python2.7

import os
import sys

from os.path import basename, dirname

def get_files(benchdir):
    files = [os.path.join(benchdir, f) for f in os.listdir(benchdir) if f.endswith('.bench')]
    return files


def get_out_file(f, outdir, fr):
    newname = basename(f).replace('.bench', '_enc%02d.bench' % int(fr*100))
    return os.path.join(outdir, newname)

# Usage: ./run-ilp1-encryption.py <orig-bench-dir> <mutability-dir> <output-dir>
print '#! /bin/bash'

for f in get_files(sys.argv[1]):
    assert os.path.exists(f)
    for ff in [0.1, 0.25, 0.50]:
        o = get_out_file(f, sys.argv[2], ff)
        print 'echo %s %f' % (f, ff)
        print './sle -I -o %s -f %f %s' % (o, ff, f)
