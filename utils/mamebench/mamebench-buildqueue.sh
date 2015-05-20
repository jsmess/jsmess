#!/bin/bash

ROMDIR=$1
LOGFILE=$2
BENCHTIME=30
THREADS=4
PATTERN="*"

shift 2
while getopts "t:p:" opt; do
	case "$opt" in
	t)
		BENCHTIME=$OPTARG
		;;
	p)
		PATTERN=$OPTARG
		;;
	esac
done

for I in "$ROMDIR"/$PATTERN.zip; do 
	GAME=$(basename "${I/\.zip/}")
	echo "./mamebench-game.sh \"${ROMDIR}\" \"${LOGFILE}\" $GAME -t $BENCHTIME"
done
