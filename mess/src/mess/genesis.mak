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
CPUS += M680X0
CPUS += SH2
CPUS += SSP1601


#-------------------------------------------------
# Specify all the sound cores necessary for the
# drivers referenced in colecovision.lst
#-------------------------------------------------

SOUNDS += SN76496
SOUNDS += YM2612
SOUNDS += RF5C68
SOUNDS += CDDA
SOUNDS += DAC


#-------------------------------------------------
# This is the list of files that are necessary
# for building all of the drivers referenced
# in colecovision.lst
#-------------------------------------------------

DRVLIBS = \
	$(EMUOBJ)/drivers/emudummy.o \
  $(MAME_MACHINE)/megadriv.o  \
  $(MAME_MACHINE)/md_cart.o \
  $(MESS_DRIVERS)/megadriv.o  \
  $(EMU_MACHINE)/nvram.o  \


#-------------------------------------------------
# MESS special OSD rules
#-------------------------------------------------

include $(SRC)/mess/osd/$(OSD)/$(OSD).mak
