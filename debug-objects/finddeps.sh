#!/bin/bash

## Give it a .o file as the only parameter

## `nm` extracts the declarations and functions that .o file needs
nm -Cu $1 | cut -c 20- > $1.tmp

cat $1.tmp | while read LINE
## find the other .o files that say they have those things
	do grep ".o $LINE$" all-objects.txt | awk '{print $1}' >> $1.tmp.tmp
done

## pull out the unique ones, remove paths we know for sure we don't need, and only tell me about things more than one directory deep
## this list of things to pull out should grow as we cull additional stock files, like Z80 and MCS48
sort -u $1.tmp.tmp | grep -v '^./lib/expat\|^./lib/softfloat\|^./lib/util\|^./lib/zlib\|^./osd' | awk -F '/' 'NF > 3'

rm $1.tmp

rm $1.tmp.tmp
