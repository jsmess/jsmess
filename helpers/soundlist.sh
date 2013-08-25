#!/bin/sh
# 
# INSANE GREPPING TO THE RESCUE: Generate the list of SOUND options for the supermake file.
# --- Jason Scott

# Instructions - Give the exact location of jsmess/mess/src/emu/sound/sound.mak
#                And we'll output some useful sound reference files for you for the makefile.

if [ ! -f "$1" ]
   then
   echo "To generate this output requires the sound.mak file in the MESS source."
   echo "Please run this like $0 jsmess/mess/src/emu/sound/sound.mak."
   exit 1
fi

CPUFILE=$1

echo "Generating a list of Sound chips."

cat $1 | grep filter | cut -c 17- | cut -f1 -d',' | sort -u > rawsoundlist.txt

echo "rawsoundlist.txt generated."
echo "Currently, SOUND.MAK supports `wc -l rawsoundlist.txt | cut -f1 -d" "` soundchips."

echo "Generating descriptions from the SOUND.MAK file. (This is not perfect.)"

cat $1 | grep -B 4 filter | grep -v -- --- | sed 's/ifneq ($(filter //g' | sed 's/,$(SOUNDS)),)//g' > sounddescs.txt

echo "sounddescs.txt generated."

# And now, black magic.

echo "#---------------------------------------------------" > soundlist.txt
echo "# Automatically Generated list of Soundchips in MESS"      >> soundlist.txt
echo "# Uncomment modules in use with the current item."   >> soundlist.txt
echo "#---------------------------------------------------" >> soundlist.txt
echo ""                                                    >> soundlist.txt

for chip in `cat rawsoundlist.txt`
    do
    DESC=`grep -B 4 "^${chip}$" sounddescs.txt | grep \# `
    SOUNDLINE="# SOUNDS +="
    printf "%-1s%-13s%-20s\n" "$SOUNDLINE" $chip "$DESC" >> soundlist.txt
    done
    

