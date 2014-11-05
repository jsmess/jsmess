#!/bin/bash
#
# Take a MESS driver (system) name as the sole argument
#
if [ "$#" -lt 1 ]
then
	echo "Generates MAME tiny makefiles given a system name."
	echo "For names, see \"name\", \"parent\" and \"sourcefile\" columns here:"
	echo "http://www.progettoemma.net/mess/sysset.php/"
	echo ""
	echo "e.g. \"$0 a1200xl\" for the Atari 1200XL plus ~12 related models"
	exit 1
fi


# Path to the correct, identical version of MESS as JSMESS uses
# No trailing slash!
MESS64PATH=.

# Name of that same MESS executable (it's probably MESS64, but JIC)
MESS64NAME=mame64

# If this isn't in jsmess/helpers, where are
# the jsmess/mess/src/m.../drivers directories?
MESSDRV=../third_party/mame/src/mess/drivers
MAMEDRV=../third_party/mame/src/mame/drivers

# If this isn't in jsmess/helpers, where are the two makefile
# paths, jsmess/make/systems and jsmess/mess/src/mess ?
JSMESSMAKE=../systems
MESSMAKE=../third_party/mame/src/mess

if [ ! -f $MESS64PATH/$MESS64NAME ]
   then
   echo ""
   echo "Please edit this script to point to the proper, full MESS executable."
   echo "See \"Building old MESS\" on the JSMESS wiki for instructions."
   echo "https://github.com/jsmess/jsmess/wiki/Building-old-MESS"
   echo ""
   exit 1
fi

DRIVER=$1

if [ "$2" == "-d" ]
   then
   rm -f $JSMESSMAKE/$DRIVER.mak
fi

if [ -f $JSMESSMAKE/$DRIVER.mak ]
   then
   echo ""
   echo "Looks like files for $DRIVER already exist."
   echo "You may be able to build this system already,"
   echo "or run \"$0 $1 -d\" to overwrite them." 
   echo ""
   exit 1
fi

FULLNAME=`$MESS64PATH/$MESS64NAME -listxml $DRIVER | grep "<description>" | head -1 | sed 's/.*<description>//g' | sed 's/<\/description>.*//g'`
if [ "$FULLNAME" == "" ]
   then
   echo ""
   echo "Problem.  Couldn't find a system with that name."
   echo "MAME may have provided some suggestions, try those?"
   echo ""
   exit 1
fi

#resolution
width=`$MESS64PATH/$MESS64NAME -listxml $DRIVER | grep "<display.*\"screen\"" | sed 's/.*width=\"//g' | cut -f1 -d'"'`
height=`$MESS64PATH/$MESS64NAME -listxml $DRIVER | grep "<display.*\"screen\"" | sed 's/.*height=\"//g' | cut -f1 -d'"'`
if [ "$width" == "" ]
   then
   width=`$MESS64PATH/$MESS64NAME -listxml $DRIVER | grep "<display.*\"raster\"" | sed 's/.*width=\"//g' | cut -f1 -d'"'`
   height=`$MESS64PATH/$MESS64NAME -listxml $DRIVER | grep "<display.*\"raster\"" | sed 's/.*height=\"//g' | cut -f1 -d'"'`
fi
RESOLUTION="${width}x${height}"

#files
DEVICE=`$MESS64PATH/$MESS64NAME -listxml $DRIVER | grep briefname= | cut -f4 -d'"' | head -1`

#parent
SOURCEFILE=`$MESS64PATH/$MESS64NAME -listxml $DRIVER | grep "\"$DRIVER\".*sourcefile" | sed "s/.*sourcefile=\"\(.*\)\.c.*/\1/"`
echo "$DRIVER is part of $SOURCEFILE, along with:"

#children
if [ ! -f $MESSDRV/$SOURCEFILE.c ]
   then
   CHILDREN=`grep "^COMP\|^CONS\|^SYST\|^GAME" $MAMEDRV/$SOURCEFILE.c | cut -d "," -f 2`
   else
   CHILDREN=`grep "^COMP\|^CONS\|^SYST\|^GAME" $MESSDRV/$SOURCEFILE.c | cut -d "," -f 2`
fi
echo "$CHILDREN"
echo "(building any of those should be very fast now)"

echo "(1/3) Creating $JSMESSMAKE/$DRIVER.mak"

O="$JSMESSMAKE/$DRIVER.mak"
echo "## " >$O
echo "## JSMESS-specific makefile for ${FULLNAME}" >>$O
echo "## " >>$O
echo "" >>$O
echo "# ${FULLNAME} BIOS File Location" >> $O
echo "#         " >>$O
echo "# The filename of the .zip file containing the machine's ROM files." >>$O
echo "# Most computer and console systems will require this collection." >> $O
echo "#" >> $O
echo "BIOS := ${DRIVER}.zip" >> $O
echo "" >>$O
echo "# SUBTARGET Location" >> $O
echo "# " >>$O
echo "# The MESS source of the driver (may be completely different)." >>$O
echo "#" >>$O
echo "SUBTARGET := ${SOURCEFILE}" >>$O
echo "" >>$O
echo "# MESS Arguments for ${FULLNAME}" >>$O
echo "#         " >>$O
echo "# Arguments that will be passed to the JSMESS routine to properly ">>$O
echo "# emulate the system and provide the type of files to read and screen" >>$O
echo "# settings. Can be modified later." >>$O
echo "#" >>$O
echo -n "MESS_ARGS := [\"${DRIVER}\",\"-verbose\",\"-rompath\",\".\"," >>$O
if [ "$DEVICE" != "" ]
   then
   echo -n "\"-${DEVICE}\",gamename," >>$O
fi
echo "\"-window\",\"-resolution\",\"${RESOLUTION}\",\"-nokeepaspect\"]" >> $O
echo "" >>$O
echo "# MESS Compilation Flags" >>$O
echo "# " >>$O
echo "# Some systems need additional compilation flags to work properly, especially" >>$O
echo "# with regards to memory usage." >>$O
echo "#" >> $O
echo "# MESS_FLAGS +=" >>$O
echo "# EMCC_FLAGS +=" >>$O
echo "" >>$O

if [ "$2" == "-d" ]
   then
   rm -f $MESSMAKE/$SOURCEFILE.mak
   rm -f ${MESSMAKE}/${SOURCEFILE}.lst
fi

if [ -f $MESSMAKE/$SOURCEFILE.mak ]
   then
   echo ""
   echo "$MESSMAKE/$SOURCEFILE.mak already exists."
   echo "You may already be able to build $DRIVER."
   echo "or run \"$0 $1 -d\" to overwrite them." 
   echo ""
   exit 1
fi

echo "(2/3) Creating ${MESSMAKE}/${SOURCEFILE}.mak"

O=${MESSMAKE}/${SOURCEFILE}.mak

echo "## " >$O
echo "## $SOURCEFILE.mak" >>$O
echo "## " >>$O
echo "" >>$O
echo "include \$(SRC)/mess/messcore.mak" >>$O
echo "" >>$O
./fulldeps.sh $SOURCEFILE >> $O
echo "" >>$O

## It's safe and easy to regenerate the .lst file

echo "(3/3) Creating $MESSMAKE/${SOURCEFILE}.lst"

O=${MESSMAKE}/${SOURCEFILE}.lst

echo "// " >$O
echo "// List of drivers $SOURCEFILE.mak supports" >>$O
echo "// " >>$O
echo "" >> $O

for AAA in $CHILDREN
   do
   BBB=`$MESS64PATH/$MESS64NAME -listxml $AAA 2>/dev/null | grep "<description>" | head -1 | sed 's/.*<description>//g' | sed 's/<\/description>.*//g'`
   if [ "$BBB" != "" ]
      then
      echo "${AAA} // ${BBB}" >>$O
   fi
done

