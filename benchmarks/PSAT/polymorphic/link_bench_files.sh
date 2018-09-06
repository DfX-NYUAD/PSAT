#!/bin/bash

echo "This script is to link the bench file into the subfolders for camouflaging range and correctness..."

if [ $# -lt 2 ]; then
	echo "Please provide the following parameters:"
	echo "  1) name of benchmark"
	echo "  2) name of bench file, including the locking scheme (e.g., c432_RTL_rnd32.bench)"
	exit
fi

cd $1

for perc in `ls *% -d`
do
	cd $perc

	ln -s ../$2 .

	cd ../
done

cd ../
