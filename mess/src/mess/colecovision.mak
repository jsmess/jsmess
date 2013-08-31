###########################################################################
#
#   colecovision.mak
#
# ColecoVision-specific Makefile
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
# drivers referenced in colecovision.lst
#-------------------------------------------------



#-------------------------------------------------
# Specify all the sound cores necessary for the
# drivers referenced in colecovision.lst
#-------------------------------------------------

SOUNDS += SN76496


#-------------------------------------------------
# This is the list of files that are necessary
# for building all of the drivers referenced
# in colecovision.lst
#-------------------------------------------------

DRVLIBS += $(MESS_DRIVERS)/coleco.o
DRVLIBS += $(MESS_MACHINE)/coleco.o


#-------------------------------------------------
# MESS special OSD rules
#-------------------------------------------------

include $(SRC)/mess/osd/$(OSD)/$(OSD).mak
