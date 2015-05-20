#!/bin/sh

if [ $# -lt 2 ]; then 
	echo "Usage: $0 <romdir> <logfile> [-t <benchtime>] [-j <threads>]"
	exit 1
fi

ROMDIR=$1
LOGFILE=$2
BENCHTIME=30
THREADS=4
PATTERN="*"

shift 2
while getopts "t:j:p:" opt; do
	case "$opt" in
	t)
		BENCHTIME=$OPTARG
		;;
	j)
		THREADS=$OPTARG
		;;
	p)
		PATTERN=$OPTARG
		;;
	esac
done

if [ ! -d "$ROMDIR" ]; then
	echo "Could not find rom directory: $ROMDIR"
	exit 1
fi

NUMROMS=$(ls "${ROMDIR}"/${PATTERN} |wc -l)

echo "Benchmarking $NUMROMS roms in $ROMDIR for $BENCHTIME seconds ($THREADS threads)"

./mamebench-buildqueue.sh "$ROMDIR" "$LOGFILE" -t $BENCHTIME -p "$PATTERN" | ./procspawn.sh $THREADS
