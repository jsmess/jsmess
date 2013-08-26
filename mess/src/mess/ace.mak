###########################################################################
#
#   ace.mak
#
# Jupiter Ace-specific Makefile
#
###########################################################################

# disable messui for tiny build
MESSUI = 0

# include MESS core defines
include $(SRC)/mess/messcore.mak
include $(SRC)/mess/osd/$(OSD)/$(OSD).mak
include $(SRC)/mess/emudummy.mak

#-------------------------------------------------
# Specify all the CPU cores necessary for the
# drivers referenced in ace.lst
#-------------------------------------------------

#CPUS += 

#-------------------------------------------------
# Specify all the sound cores necessary for the
# drivers referenced in ace.lst
#-------------------------------------------------

SOUNDS += SPEAKER
SOUNDS += WAVE
SOUNDS += AY8910
SOUNDS += SP0256

#-------------------------------------------------
# This is the list of files that are necessary
# for building all of the drivers referenced
# in ace.lst
#-------------------------------------------------

DRVLIBS += $(MESS_DRIVERS)/ace.o
DRVLIBS += $(EMU_MACHINE)/ctronics.o
DRVLIBS += $(EMU_MACHINE)/i8255.o
DRVLIBS += $(EMU_MACHINE)/z80pio.o
DRVLIBS += $(LIBOBJ)/formats/ace_tap.o
DRVLIBS += $(MESS_FORMATS)/ace_ace.o

#-------------------------------------------------
# MESS special OSD rules
#-------------------------------------------------

include $(SRC)/mess/osd/$(OSD)/$(OSD).mak
