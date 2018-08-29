#! /usr/bin/python2.7

import sys
import os
from subprocess import check_output

def get_node(nodes, n):
    if n in nodes:
        return nodes[n]
    else:
        index = len(nodes)
        nodes[n] = index
        return index

def reverse_map(node_map):
    rev_map = {}
    for n in node_map:
        v = node_map[n]
        assert v not in rev_map
        rev_map[v] = n
    return rev_map

def read_graph(filename):
    with open(filename, 'rt') as fileobj:
        nodes = {}
        edges = set()
        for line in fileobj:
            if not len(line.strip()):
                continue
            else:
                words = line.split()
                assert len(words) == 3
                wt = int(words[2])

                if wt == 3:
                    n1 = get_node(nodes, words[0])
                    n2 = get_node(nodes, words[1])

                    e = (n1, n2)
                    edges.add(e)
    return nodes, edges

def print_cnf(filename, nodes, edges):
    with open(filename, 'wt') as f:
        print >> f, 'p edge %d' % len(nodes)
        for n1, n2 in edges:
            print >> f, 'e %d %d' % (n1, n2)

def main(filename):
    n, e = read_graph(filename)
    base = os.path.basename(filename)
    base = base[:base.rfind('.')]
    cnf = base + '.clq'
    print_cnf(cnf, n, e)
    print base, len(n), len(e)

    clique = check_output(['./mcqd', cnf])
    clique_lines = [l.strip() for l in clique.split('\n') if len(l.strip()) > 0]
    assert len(clique_lines[0].split()) == 1
    num_clique_nodes = int(clique_lines[0].split()[0])
    clique_nodes = [int(x) for x in clique_lines[1].split()]
    assert len(clique_nodes) == num_clique_nodes

    rev_node_map = reverse_map(n)
    clique_node_names = [rev_node_map[c] for c in clique_nodes]
    clique_node_names.sort()

    clique_file = os.path.join(os.path.dirname(filename), base + '.clique')
    with open(clique_file, 'wt') as f:
        print >> f, ' '.join(clique_node_names)

if __name__ == '__main__':
    main(sys.argv[1])

