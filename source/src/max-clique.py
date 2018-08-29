#! /usr/bin/python2.7

import sys
import networkx as nx
import networkx.algorithms.approximation as nx_alg

def main(argv):
    if len(argv) != 2:
        print 'Syntax error.'
        print '   Usage: %s <graph>' % (argv[0])
    else:
        process(argv[1])

def process(graph_file):
    with open(graph_file, 'rt') as fileobj:
        g = nx.Graph()
        for line, text in enumerate(fileobj):
            words = text.split()
            assert len(words) == 2
            if line == 0:
                assert words[0] == 'keys'
                num_keys = int(words[1])
                vertices = range(num_keys)
                g.add_nodes_from(vertices)
            else:
                v1 = int(words[0])
                v2 = int(words[1])
                assert v1 >= 0 and v1 < num_keys
                assert v2 >= 0 and v2 < num_keys
                g.add_edge(v1, v2)

        print len(nx_alg.max_clique(g)), num_keys



if __name__ == '__main__':
    main(sys.argv)
