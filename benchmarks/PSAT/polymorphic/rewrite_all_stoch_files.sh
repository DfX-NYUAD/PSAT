#!/bin/bash

if [ $# -lt 1 ]; then
	echo "Please provide the following parameters:"
	echo "	1) probability for original functionality to occur for each polymorphic gate [%]"
	echo ""
	exit
fi

for file in `ls */*/*.stoch`
do
	bench_file=${file%.*}
	bench_file_no_path=${bench_file##*/}

	echo "Working on $bench_file ..."

	./rewrite_stoch_files.sh $file ../../ORIGINAL/$bench_file_no_path $1

	echo "Done"
	echo ""
done
