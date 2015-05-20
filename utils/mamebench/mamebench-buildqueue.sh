#!/bin/bash

ROMDIR=$1
LOGFILE=$2
BENCHTIME=30
THREADS=4
PATTERN="*"
EXECUTABLE=mame

shift 2
while getopts "t:p:x:" opt; do
	case "$opt" in
	t)
		BENCHTIME=$OPTARG
		;;
	p)
		PATTERN=$OPTARG
		;;
	x)
		EXECUTABLE=$OPTARG
		;;
	esac
done

for I in "$ROMDIR"/$PATTERN.zip; do 
	GAME=$(basename "${I/\.zip/}")
	echo "./mamebench-game.sh \"${ROMDIR}\" \"${LOGFILE}\" $GAME -t $BENCHTIME -x \"${EXECUTABLE}\""
done
