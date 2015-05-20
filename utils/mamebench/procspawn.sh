#!/bin/bash

if [ "$1" = "" ]; then
  echo "Usage: $0 <NUMPROCS>"
  exit
fi
NUMPROCS=$1
CURRPROCS=0

ALLPROCS=

trap 'echo -n "Killing processes:"; for I in $ALLPROCS; do echo -n " $I"; kill $I; done; echo " done"; exit 1' SIGINT
trap 'NUMPROCS=$(( $NUMPROCS + 1 )); echo "Increased NUMPROCS to ${NUMPROCS}"' SIGUSR1
trap 'NUMPROCS=$(( $NUMPROCS - 1 )); echo "Decreased NUMPROCS to ${NUMPROCS}"' SIGUSR2

while read -r CMD; do
  while [ $CURRPROCS -ge $NUMPROCS ]; do
    STILLRUNNING=
    STILLRUNNINGCNT=0
    for PID in $ALLPROCS; do
      if [ -e /proc/$PID ]; then
        STILLRUNNING="$STILLRUNNING $PID"
        STILLRUNNINGCNT=$(( $STILLRUNNINGCNT + 1 ))
      fi
    done
    if [ "$ALLPROCS" = "$STILLRUNNING" ]; then
      sleep 1
    else
      ALLPROCS=$STILLRUNNING
      CURRPROCS=$STILLRUNNINGCNT
    fi
  done

  sh -c "$CMD" &
  NEWPID=$!
  echo "Spawned '$CMD' ($NEWPID)"
  ALLPROCS="$ALLPROCS $NEWPID"
  CURRPROCS=$(( $CURRPROCS + 1 ))
done
wait

echo COMPLETE
