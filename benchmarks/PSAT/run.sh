#!/bin/bash

if [ $# -lt 3 ]; then
	echo "Please provide the following parameters:"
	echo "	1) benchfile"
	echo "	2) iterations/runs"
	echo "	3) log file to write result into"
	echo ""
	exit
fi

bench_file=$1
runs=$2
log_file=$3

bench_file_no_path=${bench_file##*/}

echo "Backup $log_file to $log_file"_backup_at_"`date +%s` .."
echo ""
mv $log_file $log_file"_backup_at_"`date +%s`

echo "Note that only successful runs will provide some output"
echo ""

successful_runs=0
for (( r = 1; r <= $runs; r++ ))
do
	## check lines of logfile before and after, to infer whether run was successful
	lines_before=`wc -l $log_file | awk '{print $1}'`

	## run attack, grep only those lines with successfully generated keys
	echo "Run $r on bench $bench_file ..."
	../../bin/sld $bench_file ../ORIGINAL/$bench_file_no_path .stoch | grep -B 4 -E "key=0|key=1" | tee -a $log_file
	echo "Done"

	lines_after=`wc -l $log_file | awk '{print $1}'`

	if ((lines_after > lines_before)); then
		((successful_runs++))
		echo "" >> $log_file
	fi
done

echo "Successful runs: $successful_runs" | tee -a $log_file
echo "Runs in total: $runs" | tee -a $log_file
