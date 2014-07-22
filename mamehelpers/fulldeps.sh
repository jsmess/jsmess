#!/bin/bash

DEPCOUNT=1
NEWDEPCOUNT=0
CPUMAK=../third_party/mame/src/emu/cpu/cpu.mak
SOUNDMAK=../third_party/mame/src/emu/sound/sound.mak
VIDEOMAK=../third_party/mame/src/emu/video/video.mak
MACHINEMAK=../third_party/mame/src/emu/machine/machine.mak
BUSESMAK=../third_party/mame/src/emu/bus/bus.mak
MESSMAK=../third_party/mame/src/mess/mess.mak
MAMEMAK=../third_party/mame/src/mame/mame.mak

## Give it a system name as the only parameter
if [ "$#" -ne 1 ]
then
	echo "Resolves MESS tiny makefile dependencies given a DRIVER name."
	echo "(This may be different than a system or platform name.)"
	echo "For driver names, see \"sourcefile\" column here:"
	echo "http://www.progettoemma.net/mess/sysset.php/"
	echo ""
	echo "e.g. \"$0 atari400\" for ~12 different Atari models"
	exit 1
fi

grep "\./m.../drivers/$1.o" mangled-all-unresolved.txt | awk '{print $1}' | uniq > $1.newdeps.tmp
cp $1.newdeps.tmp $1.deps.tmp
DEPCOUNT=`wc -l $1.deps.tmp | awk '{print $1}'`
echo $DEPCOUNT dependencies... 1>&2

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
	echo Resolving `wc -l $1.unr.tmp | awk '{print $1}'` references... 1>&2
	sort -u $1.unr.tmp | while read LINE
	do
		grep "\.o $LINE$" mangled-all-resolved.txt | awk '{print $1}' >> $1.newdeps.tmp
	done
	rm $1.unr.tmp

	## this list of things to pull out should grow as we cull additional stock files, like Z80 and MCS48
	BADPATHS='^./lib/formats\|^./emu/imagedev\|^./emu/debug\|^./lib/libflac\|^./lib/expat\|^./lib/softfloat\|^./lib/util\|^./lib/zlib\|^./osd\|^./emu/ui'

	sort -u $1.newdeps.tmp | grep -v $BADPATHS | awk -F '/' 'NF > 3' > $1.newerdeps.tmp
	mv $1.newerdeps.tmp $1.newdeps.tmp
	cat $1.deps.tmp $1.newdeps.tmp | sort -u | grep -v $BADPATHS | awk -F '/' 'NF > 3' > $1.alldeps.tmp
	mv $1.alldeps.tmp $1.deps.tmp
	NEWDEPCOUNT=`wc -l $1.deps.tmp | awk '{print $1}'`
	echo $NEWDEPCOUNT dependencies... 1>&2
done

cat $1.deps.tmp | grep "^./emu/cpu/" > $1.newdeps.tmp
NUMCPUS=`wc -l $1.newdeps.tmp | awk '{print $1}'`

if [ "$NUMCPUS" -gt 0 ]
then
	cut -d '/' -f 2-4 $1.newdeps.tmp | sort -u | while read LINE
	do
		grep "#@src/$LINE/" $CPUMAK | cut -d ',' -f 2 >> $1.cpus.tmp
	done
	sort -u $1.cpus.tmp
	echo ""
	rm $1.cpus.tmp
fi

cat $1.deps.tmp | grep "^./emu/sound/" > $1.newdeps.tmp
NUMSOUNDS=`wc -l $1.newdeps.tmp | awk '{print $1}'`

if [ "$NUMSOUNDS" -gt 0 ]
then
	cut -d '.' -f 2 $1.newdeps.tmp | sort -u | while read LINE
	do
		grep "#@src$LINE" $SOUNDMAK | cut -d ',' -f 2 >> $1.sounds.tmp
	done
	sort -u $1.sounds.tmp
	echo ""
	rm $1.sounds.tmp
fi

cat $1.deps.tmp | grep "^./emu/video/" > $1.newdeps.tmp
NUMVIDEO=`wc -l $1.newdeps.tmp | awk '{print $1}'`

if [ "$NUMVIDEO" -gt 0 ]
then
	cut -d '.' -f 2 $1.newdeps.tmp | sort -u | while read LINE
	do
		grep "#@src$LINE" $VIDEOMAK | cut -d ',' -f 2 >> $1.video.tmp
	done
	sort -u $1.video.tmp
	echo ""
	rm $1.video.tmp
fi

cat $1.deps.tmp | grep "^./emu/machine/" > $1.newdeps.tmp
NUMMACHINE=`wc -l $1.newdeps.tmp | awk '{print $1}'`

if [ "$NUMMACHINE" -gt 0 ]
then
	cut -d '.' -f 2 $1.newdeps.tmp | sort -u | while read LINE
	do
		grep "#@src$LINE" $MACHINEMAK | cut -d ',' -f 2 >> $1.machine.tmp
	done
	sort -u $1.machine.tmp
	echo ""
	rm $1.machine.tmp
fi

cat $1.deps.tmp | grep "^./emu/bus/" > $1.newdeps.tmp
NUMBUSES=`wc -l $1.newdeps.tmp | awk '{print $1}'`

if [ "$NUMBUSES" -gt 0 ]
then
	cut -d '/' -f 2-4 $1.newdeps.tmp | sort -u | while read LINE
	do
		grep "#@src/$LINE/" $BUSESMAK | cut -d ',' -f 2 >> $1.buses.tmp
	done
	sort -u $1.buses.tmp
	echo ""
	rm $1.buses.tmp
fi

cat $1.deps.tmp | grep -v '^./emu/imagedev' | sed 's/\.\/mess\/drivers/$(MESS_DRIVERS)/g' | sed 's/\.\/lib\/formats/$(OBJ)\/lib\/formats/g' | sed 's/\.\/emu\/machine/$(EMUOBJ)\/machine/g' | sed 's/\.\/emu\/sound/$(EMUOBJ)\/sound/g' | sed 's/\.\/mess\/video/$(MESS_VIDEO)/g' | sed 's/\.\/mess\/devices/$(MESS_DEVICES)/g' | sed 's/\.\/emu\/cpu/$(EMUOBJ)\/cpu/g' | sed 's/\.\/mess\/machine/$(MESS_MACHINE)/g' | sed 's/\.\/mess\/formats/$(MESS_FORMATS)/g' | sed 's/\.\/emu\/video/$(EMU_VIDEO)/g' | sed 's/\.\/mame\/machine/$(MAME_MACHINE)/g' | sed 's/\.\/mame\/video/$(MAME_VIDEO)/g' | sed 's/\.\/mess\/audio/$(MESS_AUDIO)/g' | sed 's/\.\/mame\/audio/$(MAME_AUDIO)/g' | sed 's/\.\/emu\/audio/$(EMU_AUDIO)/g' | sed 's/\.\/mame\/drivers/$(DRIVERS)/g' | while read LINE
do
	SEDLINE=`echo $LINE | sed 's/[$)(\/]/\\\&/g'`
	grep "\.lh" $MAMEMAK | awk '{ while( /\\$/ ) { getline n; $0 = $0 n; }} '"/$SEDLINE/" | sed 's/$(DRIVERS)/$(MAME_DRIVERS)/g' | sed 's/$(LAYOUT)/$(MAME_LAYOUT)/g'
done

## make the output makefile-ready
cat $1.deps.tmp | grep -v '^./emu/imagedev\|^./emu/cpu/\|^./emu/sound/\|^./emu/video/\|^./emu/machine/\|^./emu/bus/' | sed 's/\.\/mess\/drivers/$(MESS_DRIVERS)/g' | sed 's/\.\/lib\/formats/$(OBJ)\/lib\/formats/g' | sed 's/\.\/emu\/machine/$(EMUOBJ)\/machine/g' | sed 's/\.\/emu\/sound/$(EMUOBJ)\/sound/g' | sed 's/\.\/mess\/video/$(MESS_VIDEO)/g' | sed 's/\.\/mess\/devices/$(MESS_DEVICES)/g' | sed 's/\.\/emu\/cpu/$(EMUOBJ)\/cpu/g' | sed 's/\.\/mess\/machine/$(MESS_MACHINE)/g' | sed 's/\.\/mess\/formats/$(MESS_FORMATS)/g' | sed 's/\.\/emu\/video/$(EMU_VIDEO)/g' | sed 's/\.\/mame\/machine/$(MAME_MACHINE)/g' | sed 's/\.\/mame\/video/$(MAME_VIDEO)/g' | sed 's/\.\/mess\/audio/$(MESS_AUDIO)/g' | sed 's/\.\/mame\/audio/$(MAME_AUDIO)/g' | sed 's/\.\/emu\/audio/$(EMU_AUDIO)/g' | sed 's/\.\/mame\/drivers/$(MAME_DRIVERS)/g' > $1.newdeps.tmp

cat $1.newdeps.tmp | while read LINE
do
	echo "DRVLIBS += $LINE"
done

rm $1.newdeps.tmp
rm $1.deps.tmp
