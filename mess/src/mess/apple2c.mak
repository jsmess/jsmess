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
CPUS += COP400
CPUS += Z80
CPUS += MCS48



#-------------------------------------------------
# Specify all the sound cores necessary for the
# drivers referenced in colecovision.lst
#-------------------------------------------------

#SOUNDS += SN76496
#SOUNDS += SPEAKER
#SOUNDS += SP0256


#-------------------------------------------------
# This is the list of files that are necessary
# for building all of the drivers referenced
# in colecovision.lst
#-------------------------------------------------

DRVLIBS = \
	$(EMUOBJ)/drivers/emudummy.o \
  $(MESS_MACHINE)/a2cffa.o  \
  $(MESS_MACHINE)/ap2_slot.o \
  $(MESS_MACHINE)/applefdc.o \
  $(OBJ)/lib/formats/d64_dsk.o \
  $(OBJ)/lib/formats/g64_dsk.o \
  $(OBJ)/lib/formats/fdi_dsk.o \
  $(OBJ)/lib/formats/td0_dsk.o \
  $(OBJ)/lib/formats/imd_dsk.o \
  $(OBJ)/lib/formats/cqm_dsk.o \
  $(OBJ)/lib/formats/dsk_dsk.o \
  $(OBJ)/lib/formats/d88_dsk.o \
  $(OBJ)/lib/formats/flopimg.o \
  $(OBJ)/lib/formats/ap_dsk35.o \
  $(OBJ)/lib/formats/ap2_dsk.o \
  $(OBJ)/lib/formats/basicdsk.o \
  $(MESS_DEVICES)/appldriv.o \
  $(MESS_DEVICES)/sonydriv.o \
  $(MESS_MACHINE)/ay3600.o \
  $(EMUOBJ)/sound/ay8910.o \
  $(EMUOBJ)/machine/idectrl.o \
  $(EMUOBJ)/sound/speaker.o \
  $(EMUOBJ)/machine/6522via.o \
  $(MESS_MACHINE)/ap2_lang.o \
  $(MESS_MACHINE)/mockngbd.o \
	$(MESS_VIDEO)/apple2.o    \
  $(MESS_MACHINE)/apple2.o  \
  $(MESS_DRIVERS)/apple2.o  \


#-------------------------------------------------
# MESS special OSD rules
#-------------------------------------------------

include $(SRC)/mess/osd/$(OSD)/$(OSD).mak
