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
SOUNDS += WAVE

#-------------------------------------------------
# This is the list of files that are necessary
# for building all of the drivers referenced
# in colecovision.lst
#-------------------------------------------------

DRVLIBS = \
	$(EMUOBJ)/drivers/emudummy.o \
  $(EMUOBJ)/machine/ctronics.o \
  $(EMUOBJ)/machine/wd17xx.o \
  $(OBJ)/lib/formats/d64_dsk.o \
  $(OBJ)/lib/formats/g64_dsk.o \
  $(OBJ)/lib/formats/fdi_dsk.o \
  $(OBJ)/lib/formats/td0_dsk.o \
  $(OBJ)/lib/formats/imd_dsk.o \
  $(OBJ)/lib/formats/cqm_dsk.o \
  $(OBJ)/lib/formats/dsk_dsk.o \
  $(OBJ)/lib/formats/d88_dsk.o \
  $(OBJ)/lib/formats/flopimg.o \
  $(OBJ)/lib/formats/basicdsk.o \
  $(OBJ)/lib/formats/coco_dsk.o \
  $(MESS_DEVICES)/appldriv.o           \
  $(MESS_MACHINE)/ay31015.o \
  $(MESS_MACHINE)/trs80.o   \
  $(MESS_VIDEO)/trs80.o   \
  $(MESS_FORMATS)/trs_cmd.o \
  $(OBJ)/lib/formats/trs_cas.o \
  $(OBJ)/lib/formats/trs_dsk.o \
  $(MESS_DRIVERS)/trs80.o   \


#-------------------------------------------------
# MESS special OSD rules
#-------------------------------------------------

include $(SRC)/mess/osd/$(OSD)/$(OSD).mak
