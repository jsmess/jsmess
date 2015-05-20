#!/bin/sh

ROMDIR=$1
LOGFILE=$2
GAME=$3

BENCHTIME=30
EXECUTABLE=mame

shift 3
while getopts "t:x:" opt; do
	case "$opt" in
	t)
		BENCHTIME=$OPTARG
		;;
	x)
		EXECUTABLE=$OPTARG
		;;
	esac
done

MAMEOUT=$(SDLMAME_DESKTOPDIM=800x600 SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software "${EXECUTABLE}" -rompath "$ROMDIR" -bench $BENCHTIME $GAME | tr -d '\n' | sed 's/.*Average speed: //' | sed 's/\% .*$/%/' )
FULLNAME=$("${EXECUTABLE}" -listfull $GAME |tail -1 |sed -r 's/^.*"(.*)"$/\1/g')
echo "$GAME\t$FULLNAME\t$MAMEOUT" >> "${LOGFILE}"

