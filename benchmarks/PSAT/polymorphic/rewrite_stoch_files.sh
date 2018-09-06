#!/bin/bash

if [ $# -lt 3 ]; then
	echo "Please provide the following parameters:"
	echo "	1) stoch file to rewrite"
	echo "	2) original bench file"
	echo "	3) probability for original functionality to occur for each polymorphic gate [%]"
	echo ""
	exit
fi

## parameters
STOCH_FILE=$1
ORIG_BENCH=$2
PROB=$3

## pre-calculate probabilities for remaining functions
PROB_5_REM=`bc <<< "scale = 2; ((100 - $PROB) / 5)"`
PROB_1_REM=`bc <<< "scale = 2; (100 - $PROB)"`

## backup stoch file
cp $STOCH_FILE $STOCH_FILE"_backup_at_"`date +%s`

echo "Backup $STOCH_FILE to $STOCH_FILE"_backup_at_"`date +%s` .."
echo ""

## for all polymorphic gates in the stoch file ...
for poly_gate in `cat $STOCH_FILE | grep -v "GATE_NAME" | grep "POLYMORPHIC_GATE" | cut -f 1 -d " "`
do
	## parse the related gate's line in the original bench file
	orig_gate=`awk '{if($1=="'$poly_gate'") print $0;}' $ORIG_BENCH`

	## also extract the error rate for the polymorphic gate (from the stoch file)
	poly_gate_error=`awk '{if($1=="'$poly_gate'") print $2;}' $STOCH_FILE`

	echo "Polymorphic gate name: $poly_gate"
	#echo "	Gate error rate: $poly_gate_error"
	echo "	Related gate definition in $ORIG_BENCH: $orig_gate"
	echo "	Old line/definition in $STOCH_FILE: `cat $STOCH_FILE | grep -w $poly_gate`"

	## then, depending on the original gate, rewrite the related polymorphic gate
	##
	### *STRING* matches substrings -- to avoid mistaking AND for NAND, e.g., those longer and more precise substrings have to be
	### checked for first
	if [[ ($orig_gate = *NAND*) || ($orig_gate = *nand*) ]]; then

		sed -i "s/^\b$poly_gate\b.*/$poly_gate $poly_gate_error POLYMORPHIC_GATE NAND $PROB AND $PROB_5_REM NOR $PROB_5_REM OR $PROB_5_REM XOR $PROB_5_REM XNOR $PROB_5_REM/g" $STOCH_FILE

	elif [[ ($orig_gate = *AND*) || ($orig_gate = *and*) ]]; then

		sed -i "s/^\b$poly_gate\b.*/$poly_gate $poly_gate_error POLYMORPHIC_GATE NAND $PROB_5_REM AND $PROB NOR $PROB_5_REM OR $PROB_5_REM XOR $PROB_5_REM XNOR $PROB_5_REM/g" $STOCH_FILE

	elif [[ ($orig_gate = *XNOR*) || ($orig_gate = *xnor*) ]]; then

		sed -i "s/^\b$poly_gate\b.*/$poly_gate $poly_gate_error POLYMORPHIC_GATE NAND $PROB_5_REM AND $PROB_5_REM NOR $PROB_5_REM OR $PROB_5_REM XOR $PROB_5_REM XNOR $PROB/g" $STOCH_FILE

	elif [[ ($orig_gate = *NOR*) || ($orig_gate = *nor*) ]]; then

		sed -i "s/^\b$poly_gate\b.*/$poly_gate $poly_gate_error POLYMORPHIC_GATE NAND $PROB_5_REM AND $PROB_5_REM NOR $PROB OR $PROB_5_REM XOR $PROB_5_REM XNOR $PROB_5_REM/g" $STOCH_FILE

	elif [[ ($orig_gate = *XOR*) || ($orig_gate = *xor*) ]]; then

		sed -i "s/^\b$poly_gate\b.*/$poly_gate $poly_gate_error POLYMORPHIC_GATE NAND $PROB_5_REM AND $PROB_5_REM NOR $PROB_5_REM OR $PROB_5_REM XOR $PROB XNOR $PROB_5_REM/g" $STOCH_FILE

	elif [[ ($orig_gate = *OR*) || ($orig_gate = *or*) ]]; then

		sed -i "s/^\b$poly_gate\b.*/$poly_gate $poly_gate_error POLYMORPHIC_GATE NAND $PROB_5_REM AND $PROB_5_REM NOR $PROB_5_REM OR $PROB XOR $PROB_5_REM XNOR $PROB_5_REM/g" $STOCH_FILE

	elif [[ ($orig_gate = *NOT*) || ($orig_gate = *not*) ]]; then

		sed -i "s/^\b$poly_gate\b.*/$poly_gate $poly_gate_error POLYMORPHIC_GATE INV $PROB BUF $PROB_1_REM/g" $STOCH_FILE

	elif [[ ($orig_gate = *BUF*) || ($orig_gate = *buf*) ]]; then

		sed -i "s/^\b$poly_gate\b.*/$poly_gate $poly_gate_error POLYMORPHIC_GATE INV $PROB_1_REM BUF $PROB/g" $STOCH_FILE
	else
		echo ""
		echo "Error -- function for $poly_gate cannot be parsed from $orig_gate !"
		exit
	fi

	echo "	New line/definition in $STOCH_FILE: `cat $STOCH_FILE | grep -w $poly_gate`"
done
