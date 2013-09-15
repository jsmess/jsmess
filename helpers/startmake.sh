#!/bin/bash
#
# Take a MESS driver (system) name as the sole argument
#

# Path to the correct, identical version of MESS as JSMESS uses
# No trailing slash!
MESS64PATH=../../mess

# Name of that same MESS executable (it's probably MESS64, but JIC)
MESS64NAME=mess64

# If this isn't in jsmess/helpers, where are
# the jsmess/mess/src/m.../drivers directories?
MESSDRV=../mess/src/mess/drivers
MAMEDRV=../mess/src/mame/drivers

# If this isn't in jsmess/helpers, where are the two makefile
# paths, jsmess/make/systems and jsmess/mess/src/mess ?
JSMESSMAKE=../make/systems
MESSMAKE=../mess/src/mess

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

if [ -f $JSMESSMAKE/$DRIVER.mak ]
   then
   echo ""
   echo "Problem.  Looks like files for $DRIVER already exist."
   echo "Please make sure work isn't already done on these, or clear them out."
   echo ""
   exit 1
fi

FULLNAME=`$MESS64PATH/$MESS64NAME -listxml $DRIVER | grep "<description>" | head -1 | sed 's/.*<description>//g' | sed 's/<\/description>.*//g'`
if [ "$FULLNAME" == "" ]
   then
   echo ""
   echo "Problem.  Couldn't find a system with that name."
   echo "MESS may have provided some suggestions, try those?"
   echo ""
   exit 1
fi

echo "Creating the $DRIVER makefiles."

width=`$MESS64PATH/$MESS64NAME -listxml $DRIVER | grep "<display " | sed 's/.*width=\"//g' | cut -f1 -d'"'`
height=`$MESS64PATH/$MESS64NAME -listxml $DRIVER | grep "<display " | sed 's/.*height=\"//g' | cut -f1 -d'"'`
RESOLUTION="${width}x${height}"

#echo "Run this to figure out a potential format for files (it can change later, as needed)"
#echo "src/mame/mess64 -listxml $DRIVER | grep briefname"
#echo "[?] Please enter what sort of device name for software (cart1, cart, flop, dump)"
DEVICE=`$MESS64PATH/$MESS64NAME -listxml $DRIVER | grep briefname= | cut -f4 -d'"' | head -1`

SOURCEFILE=`$MESS64PATH/$MESS64NAME -listxml $1 | grep "sourcefile=" | sed "s/.*sourcefile=\"\(.*\)\.c.*/\1/"`
echo "$DRIVER is part of $SOURCEFILE, along with:"

if [ ! -f $MESSDRV/$SOURCEFILE.c ]
   then
   CHILDREN=`grep "^COMP\|^CONS\|^SYST\|^GAME" $MAMEDRV/$SOURCEFILE.c | cut -d "," -f 2`
   else
   CHILDREN=`grep "^COMP\|^CONS\|^SYST\|^GAME" $MESSDRV/$SOURCEFILE.c | cut -d "," -f 2`
fi
echo "$CHILDREN"
echo "(building any of those should be very fast now)"

echo "Blowing out $JSMESSMAKE/$DRIVER.mak..."

O="$JSMESSMAKE/$DRIVER.mak"
echo "##################################################################" >$O
echo "#                                                                " >>$O
echo "#    JSMESS-specific makefile for ${FULLNAME}." >>$O
echo "#                                                                " >>$O
echo "##################################################################" >>$O
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

if [ -f $MESSMAKE/$SOURCEFILE.mak ]
   then
   echo ""
   echo "$MESSMAKE/$SOURCEFILE.mak already exists."
   echo "You may already be able to build $DRIVER."
   echo "Please make sure work isn't already done on it, or clear it out."
   echo ""
   exit 1
fi

echo "Creating the ${MESSMAKE}/${SOURCEFILE}.mak file."

O=${MESSMAKE}/${SOURCEFILE}.mak

echo "## " >$O
echo "## $SOURCEFILE.mak" >>$O
echo "## " >>$O
echo "" >>$O
echo "MESSUI = 0" >>$O
echo "include \$(SRC)/mess/messcore.mak" >>$O
echo "include \$(SRC)/mess/osd/\$(OSD)/\$(OSD).mak" >>$O
echo "include \$(SRC)/mess/emudummy.mak" >>$O
echo "" >>$O
echo "# CPUS +=" >>$O
echo "" >>$O
echo "# SOUNDS +=" >>$O
echo "" >>$O
echo "# DRVLIBS +=" >>$O
./fulldeps.sh $SOURCEFILE >> $O
echo "" >>$O
echo "include \$(SRC)/mess/osd/\$(OSD)/\$(OSD).mak" >>$O

## It's safe and easy to regenerate the .lst file

echo "Creating the $MESSMAKE/${SOURCEFILE}.lst file."

O=${MESSMAKE}/${SOURCEFILE}.lst

echo "/******************************************************************************" >$O
echo "" >> $O
echo "     List of drivers $SOURCEFILE supports. This file is parsed by" >>$O
echo "     makelist.exe, sorted, and output as C code describing the drivers." >>$O
echo "" >> $O
echo "******************************************************************************/" >>$O
echo "" >> $O

for AAA in $CHILDREN
   do
   BBB=`$MESS64PATH/$MESS64NAME -listxml $AAA 2>/dev/null | grep "<description>" | head -1 | sed 's/.*<description>//g' | sed 's/<\/description>.*//g'`
   if [ "$BBB" != "" ]
      then
      echo "${AAA} // ${BBB}" >>$O
   fi
done

echo ""
echo "You may need to edit ${MESSMAKE}/${SOURCEFILE}.mak"
echo "to add 'CPUS +=' lines, or to set up layout files."
echo "See the JSMESS wiki for more instructions."
echo ""

