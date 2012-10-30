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

CPUS += M6502
CPUS += Z80
CPUS += MCS48



#-------------------------------------------------
# Specify all the sound cores necessary for the
# drivers referenced in colecovision.lst
#-------------------------------------------------

#SOUNDS += SN76496
SOUNDS += POKEY
SOUNDS += DAC



#-------------------------------------------------
# This is the list of files that are necessary
# for building all of the drivers referenced
# in colecovision.lst
#-------------------------------------------------

DRVLIBS = \
	$(EMUOBJ)/drivers/emudummy.o \
  $(EMUOBJ)/machine/6821pia.o \
  $(OBJ)/mame/video/atari.o \
  $(OBJ)/mame/video/antic.o \
  $(OBJ)/mame/video/gtia.o \
  $(OBJ)/mame/machine/atari.o \
  $(MESS_DEVICES)/appldriv.o           \
  $(OBJ)/lib/formats/flopimg.o         \
  $(OBJ)/lib/formats/fdi_dsk.o         \
  $(OBJ)/lib/formats/td0_dsk.o         \
  $(OBJ)/lib/formats/imd_dsk.o         \
  $(OBJ)/lib/formats/cqm_dsk.o         \
  $(OBJ)/lib/formats/dsk_dsk.o         \
  $(OBJ)/lib/formats/d88_dsk.o         \
  $(OBJ)/lib/formats/atari_dsk.o \
  $(MESS_MACHINE)/ataricrt.o \
  $(MESS_MACHINE)/atarifdc.o \
  $(MESS_DRIVERS)/atari400.o  \


#-------------------------------------------------
# MESS special OSD rules
#-------------------------------------------------

include $(SRC)/mess/osd/$(OSD)/$(OSD).mak
