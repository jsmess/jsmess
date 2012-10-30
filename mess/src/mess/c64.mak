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
SOUNDS += MOS656X
SOUNDS += SID6581
SOUNDS += DAC



#-------------------------------------------------
# This is the list of files that are necessary
# for building all of the drivers referenced
# in colecovision.lst
#-------------------------------------------------

DRVLIBS = \
	$(EMUOBJ)/drivers/emudummy.o \
  $(EMUOBJ)/machine/6526cia.o \
  $(EMUOBJ)/machine/6522via.o \
  $(OBJ)/lib/formats/d64_dsk.o \
  $(OBJ)/lib/formats/g64_dsk.o \
  $(OBJ)/lib/formats/fdi_dsk.o \
  $(OBJ)/lib/formats/td0_dsk.o \
  $(OBJ)/lib/formats/imd_dsk.o \
  $(OBJ)/lib/formats/cqm_dsk.o \
  $(OBJ)/lib/formats/dsk_dsk.o \
  $(OBJ)/lib/formats/d88_dsk.o \
  $(OBJ)/lib/formats/flopimg.o \
  $(OBJ)/lib/formats/cbm_tap.o \
  $(MESS_DEVICES)/appldriv.o  \
  $(MESSOBJ)/formats/cbm_snqk.o \
  $(MESS_MACHINE)/cbm.o \
  $(MESS_VIDEO)/vic6567.o \
  $(MESS_MACHINE)/cbmipt.o \
  $(MESS_MACHINE)/iecstub.o \
  $(MESS_MACHINE)/64h156.o \
  $(MESS_MACHINE)/cbmiec.o \
  $(MESS_MACHINE)/c1541.o \
	$(MESS_DRIVERS)/c64.o    \
  $(MESS_MACHINE)/c64.o     \


#-------------------------------------------------
# MESS special OSD rules
#-------------------------------------------------

include $(SRC)/mess/osd/$(OSD)/$(OSD).mak
