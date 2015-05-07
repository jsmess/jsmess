#!/bin/bash
#
# Regenerate the JSMESS helper data files
#
# Requires `nm` in your path
#
# Assumes a 64-bit native build!
#

# Path to the correct, identical version of MESS as JSMESS uses
# No trailing slash!
MESS64PATH=../third_party/mame

if [ ! -f "$MESS64PATH/makefile" ]
   then
   echo ""
   echo "Please edit this script to point to the proper path to MAME."
   echo ""
   exit 1
fi

hash nm 2>/dev/null || { echo >&2 "'nm' required, not found."; exit 1; }

echo ""
echo "(1/5) Generating dependencies..."
echo ""
cd "$MESS64PATH"
make TARGET=mame depend

echo ""
echo "(2/5) Rebuilding MAME with symbols..."
echo ""
make TARGET=mame SYMBOLS=1 NOWERROR=1 -j4
if [ $? -ne 0 ]
   then
   echo ""
   echo "MAME/MESS compilation failed"
   echo
   exit 1 
fi
mv mame64 ../../mamehelpers/

cd obj/sdl64

rm -f mangled-all-resolved.txt
rm -f mangled-all-unresolved.txt
rm -f all-resolved.txt

echo ""
echo "(3/5) Finding all the functions..."
echo ""
for a in `find . -name '*.o'`
	do nm --defined-only $a | cut -d ' ' -f 3- > $a.tmp
	cat $a.tmp | while read LINE
		do echo $a $LINE >> mangled-all-resolved.txt
	done
	rm $a.tmp
done

echo ""
echo "(4/5) Finding all the missing functions..."
echo ""
for a in `find . -name '*.o'`
	do nm -u $a | cut -c 20- > $a.tmp
	cat $a.tmp | while read LINE
		do echo $a $LINE >> mangled-all-unresolved.txt
	done
	rm $a.tmp
done

echo ""
echo "(5/5) Finding all the readable names..."
echo ""
for a in `find . -name '*.o'`
	do nm -C --defined-only $a | cut -d ' ' -f 3- > $a.tmp
	cat $a.tmp | while read LINE
		do echo $a $LINE >> all-resolved.txt
	done
	rm $a.tmp
done

mv mangled-all-resolved.txt ../../../../mamehelpers/
mv mangled-all-unresolved.txt ../../../../mamehelpers/
mv all-resolved.txt ../../../../mamehelpers/

cd ../..
#make TARGET=mame clean

cd ../../mamehelpers

echo ""
echo "Ready for startmake.sh"
echo ""
