#!/bin/sh
# 
# INSANE GREPPING TO THE RESCUE: Generate the list of CPU options for the supermake file.
# --- Jason Scott

# Instructions - Give the exact location of jsmess/mess/src/emu/cpu/cpu.mak
#                And we'll output some useful CPU reference files for you for the makefile.

if [ ! -f "$1" ]
   then
   echo "To generate this output requires the cpu.mak file in the MESS source."
   echo "Please run this like $0 jsmess/mess/src/emu/cpu/cpu.mak."
   exit 1
fi

CPUFILE=$1

echo "Generating a list of support CPUs."

cat $1 | grep filter | cut -c 17- | cut -f1 -d',' | sort -u > rawcpulist.txt

echo "rawcpulist.txt generated."
echo "Currently, CPU.MAK supports `wc -l rawcpulist.txt | cut -f1 -d" "` CPUs."

echo "Generating descriptions from the CPU.MAK file. (This is not perfect.)"

cat $1 | grep -B 4 filter | grep -v -- --- | sed 's/ifneq ($(filter //g' | sed 's/,$(CPUS)),)//g' > cpudescs.txt

echo "cpudescs.txt generated."

# And now, black magic.

echo "#--------------------------------------------------" > cpulist.txt
echo "# Automatically Generated list of CPUs in MESS"      >> cpulist.txt
echo "# Uncomment modules in use with the current item."   >> cpulist.txt
echo "#--------------------------------------------------" >> cpulist.txt
echo ""                                                    >> cpulist.txt

for cpu in `cat rawcpulist.txt`
    do
    DESC=`grep -B 4 "^${cpu}$" cpudescs.txt | grep \# `
    CPULINE="# CPUS +="
    printf "%-1s%-13s%-20s\n" "$CPULINE" $cpu "$DESC" >> cpulist.txt
    done
    

