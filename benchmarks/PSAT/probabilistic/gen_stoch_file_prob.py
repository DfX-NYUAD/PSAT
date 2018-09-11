import time
import sys
import os
import re


def parse(input_file, gates_count, error_rate):
    with open(input_file) as f:
        lines = f.readlines()
    inputs = list()
    outputs = list()
    input_pattern = re.compile('INPUT\s*\((\w+)\s*\)')
    op_pattern = re.compile('OUTPUT\s*\((\w+)\s*\)') # OUTPUT(po2)
    gates = list()
    for line in lines:
        grp1 = input_pattern.search(line)
        grp2 = op_pattern.search(line)
        if grp1:
            inputs.append(grp1.group(1))
        elif grp2:
            outputs.append(grp2.group(1))
        elif outputs:
            gate = line.split('=')[0].strip()
            if gate not in outputs and gate not in inputs:
                gates.append(gate)

    print '--' * 100
    print ' Total number of gates {}'.format(len(gates))
    print '--' * 100
    import random

    text = """# OUTPUT_SAMPLING_ON OUTPUT_SAMPLING_ITERATIONS OUTPUT_SAMPLING_FOR_TEST_ON TEST_PATTERNS\ntrue 1000 false 10000\n# GATE_NAME ERROR_RATE (%)\n"""
    next_gate = "NEXT_GATE\n"
    output_file = 'out.txt'
    f = open(output_file, 'w')
    f.write(text)
    for i in range(gates_count):
        f.write(next_gate)
        random_gate = random.choice(gates)
        gates.remove(random_gate)
        f.write('{} {}\n'.format(random_gate, error_rate))
    f.close()


def main():
    start_time = time.time()
    if len(sys.argv) < 4:
        print "Usage python script.py 'input/file/path' 'gates_count' 'error_rate'"
        exit(0)

    input_file = sys.argv[1]
    if not os.path.exists(input_file):
        exit(0)
    gates_count = int(sys.argv[2])
    # error_rate = int(sys.argv[3])
    # read_data_and_sort(input_file, gates_count)
    parse(input_file, gates_count, sys.argv[3])
    print "Total Time taken", time.time() - start_time
    return 0


if __name__ == "__main__":
    sys.exit(main())
