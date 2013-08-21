###########################################################################
#
#   spectrum.mak
#
# ZX Spectrum-specific Makefile
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

SOUNDS += SPEAKER
SOUNDS += WAVE
SOUNDS += AY8910

#-------------------------------------------------
# This is the list of files that are necessary
# for building all of the drivers referenced
# in colecovision.lst
#-------------------------------------------------

DRVLIBS = \
	$(EMUOBJ)/drivers/emudummy.o \
	$(MESS_VIDEO)/spectrum.o	\
	$(MESS_VIDEO)/timex.o		\
	$(MESS_DRIVERS)/spectrum.o	\
	$(MESS_DRIVERS)/spec128.o	\
	$(MESS_DRIVERS)/timex.o		\
	$(MESS_DRIVERS)/specpls3.o	\
	$(MESS_DRIVERS)/scorpion.o	\
	$(MESS_DRIVERS)/atm.o		\
	$(MESS_DRIVERS)/pentagon.o	\
	$(MESS_MACHINE)/beta.o		\
	$(MESS_FORMATS)/spec_snqk.o	\
	$(MESS_FORMATS)/timex_dck.o	\
	$(OBJ)/lib/formats/tzx_cas.o \
	$(OBJ)/lib/formats/trd_dsk.o \
	$(MESS_MACHINE)/upd765.o	\
	$(EMUMACHINE)/wd17xx.o		\
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
