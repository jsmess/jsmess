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


#-------------------------------------------------
# Specify all the CPU cores necessary for the
# drivers referenced in colecovision.lst
#-------------------------------------------------

CPUS += Z80
CPUS += MCS48



#-------------------------------------------------
# Specify all the sound cores necessary for the
# drivers referenced in colecovision.lst
#-------------------------------------------------

#SOUNDS += SN76496
SOUNDS += SPEAKER
SOUNDS += SP0256


#-------------------------------------------------
# This is the list of files that are necessary
# for building all of the drivers referenced
# in colecovision.lst
#-------------------------------------------------

DRVLIBS = \
	$(EMUOBJ)/drivers/emudummy.o \
	$(MESS_MACHINE)/odyssey2.o \
  $(MESS_VIDEO)/odyssey2.o  \
  $(MESS_DRIVERS)/odyssey2.o  \


#-------------------------------------------------
# MESS special OSD rules
#-------------------------------------------------

include $(SRC)/mess/osd/$(OSD)/$(OSD).mak
