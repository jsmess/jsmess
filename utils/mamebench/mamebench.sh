#!/bin/sh

if [ $# -lt 2 ]; then 
	echo "Usage: $0 <romdir> <logfile> [-t <benchtime>] [-j <processes>] [-x <executable>]"
	exit 1
fi

ROMDIR=$1
LOGFILE=$2
BENCHTIME=30
PROCESSES=4
PATTERN="*"
EXECUTABLE=mame

shift 2
while getopts "t:j:p:x:" opt; do
	case "$opt" in
	t)
		BENCHTIME=$OPTARG
		;;
	j)
		PROCESSES=$OPTARG
		;;
	p)
		PATTERN=$OPTARG
		;;
	x)
		EXECUTABLE=$OPTARG
		;;
	esac
done

if [ ! -d "$ROMDIR" ]; then
	echo "Could not find rom directory: $ROMDIR"
	exit 1
fi

NUMROMS=$(ls "${ROMDIR}"/${PATTERN} |wc -l)

echo "Benchmarking $NUMROMS roms in $ROMDIR for $BENCHTIME seconds ($PROCESSES processes)"

./mamebench-buildqueue.sh "$ROMDIR" "$LOGFILE" -t $BENCHTIME -p "$PATTERN" -x "$EXECUTABLE" | ./procspawn.sh $PROCESSES
