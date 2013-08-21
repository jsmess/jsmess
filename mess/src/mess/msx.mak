###########################################################################
#
#   msx.mak
#
# MSX-specific Makefile
#
###########################################################################

# disable messui for tiny build
MESSUI = 0

# include MESS core defines
include $(SRC)/mess/messcore.mak
include $(SRC)/mess/osd/$(OSD)/$(OSD).mak


#-------------------------------------------------
# Specify all the CPU cores necessary for the
# drivers referenced in msx.lst
#-------------------------------------------------

CPUS += Z80
CPUS += MCS48

#-------------------------------------------------
# Specify all the sound cores necessary for the
# drivers referenced in msx.lst
#-------------------------------------------------

SOUNDS += DAC
SOUNDS += WAVE
SOUNDS += AY8910
SOUNDS += K051649
SOUNDS += YM2413

#-------------------------------------------------
# This is the list of files that are necessary
# for building all of the drivers referenced
# in msx.lst
#-------------------------------------------------

DRVLIBS = \
	$(EMUOBJ)/drivers/emudummy.o \
	$(MESS_DRIVERS)/msx.o		\
	$(MESS_MACHINE)/msx.o		\
	$(MESS_MACHINE)/msx_slot.o	\
	$(EMUVIDEO)/v9938.o			\
	$(EMUMACHINE)/ctronics.o	\
	$(EMUMACHINE)/i8255.o		\
	$(EMUMACHINE)/rp5c01.o		\
	$(EMUMACHINE)/wd17xx.o		\
	$(LIBOBJ)/formats/fmsx_cas.o \
	$(LIBOBJ)/formats/msx_dsk.o	\
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

#-------------------------------------------------
# MESS special OSD rules
#-------------------------------------------------

include $(SRC)/mess/osd/$(OSD)/$(OSD).mak
