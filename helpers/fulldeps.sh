#!/bin/bash

DEPCOUNT=1
NEWDEPCOUNT=0

## Give it a system name as the only parameter

grep "\./m.../drivers/$1.o" mangled-all-unresolved.txt | awk '{print $1}' | uniq > $1.newdeps.tmp
cp $1.newdeps.tmp $1.deps.tmp
DEPCOUNT=`wc -l $1.deps.tmp | awk '{print $1}'`
echo $DEPCOUNT dependencies...

while [ "$DEPCOUNT" -ne "$NEWDEPCOUNT" ]
do

	DEPCOUNT=$NEWDEPCOUNT
	
	sort -u $1.newdeps.tmp | while read LINE
	do
		SEDLINE=`echo $LINE | sed 's/[\/&]/\\\&/g'`
		grep "^$LINE " mangled-all-unresolved.txt | sed "s/$SEDLINE\ //g" >> $1.unr.tmp
	done
	rm $1.newdeps.tmp

	## find the other .o files that say they have those things
	echo Resolving `wc -l $1.unr.tmp | awk '{print $1}'` references...
	sort -u $1.unr.tmp | while read LINE
	do
		grep "\.o $LINE$" mangled-all-resolved.txt | awk '{print $1}' >> $1.newdeps.tmp
	done
	rm $1.unr.tmp
	
	## this list of things to pull out should grow as we cull additional stock files, like Z80 and MCS48
	BADPATHS='^./emu/debug\|^./lib/expat\|^./lib/softfloat\|^./lib/util\|^./lib/zlib\|^./osd'
	
	sort -u $1.newdeps.tmp | grep -v $BADPATHS | awk -F '/' 'NF > 3' > $1.newerdeps.tmp
	mv $1.newerdeps.tmp $1.newdeps.tmp
	cat $1.deps.tmp $1.newdeps.tmp | sort -u | grep -v $BADPATHS | awk -F '/' 'NF > 3' > $1.alldeps.tmp
	mv $1.alldeps.tmp $1.deps.tmp
	NEWDEPCOUNT=`wc -l $1.deps.tmp | awk '{print $1}'`
	echo $NEWDEPCOUNT dependencies...
done

## make the output makefile-ready
cat $1.deps.tmp | grep -v '^./emu/imagedev' | sed 's/\.\/mess\/drivers/$(MESS_DRIVERS)/g' |sed 's/\.\/lib\/formats/$(OBJ)\/lib\/formats/g' | sed 's/\.\/emu\/machine/$(EMUOBJ)\/machine/g' | sed 's/\.\/emu\/sound/$(EMUOBJ)\/sound/g' | sed 's/\.\/mess\/video/$(MESS_VIDEO)/g' | sed 's/\.\/mess\/devices/$(MESS_DEVICES)/g' | sed 's/\.\/emu\/cpu/$(EMUOBJ)\/cpu/g' | sed 's/\.\/mess\/machine/$(MESS_MACHINE)/g' | sed 's/\.\/mess\/formats/$(MESS_FORMATS)/g'

rm $1.newdeps.tmp
rm $1.deps.tmp
