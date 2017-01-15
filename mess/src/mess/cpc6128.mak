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

SOUNDS += SPEAKER
SOUNDS += WAVE
SOUNDS += AY8910



#-------------------------------------------------
# This is the list of files that are necessary
# for building all of the drivers referenced
# in colecovision.lst
#-------------------------------------------------

DRVLIBS = \
	$(EMUOBJ)/drivers/emudummy.o
DRVLIBS += $(EMUOBJ)/cpu/z80/z80daisy.o
DRVLIBS += $(EMUOBJ)/cpu/z80/z80dasm.o
DRVLIBS += $(EMUOBJ)/cpu/z80/z80.o
DRVLIBS += $(EMUOBJ)/machine/ctronics.o
DRVLIBS += $(EMUOBJ)/machine/i8255.o
DRVLIBS += $(EMUOBJ)/machine/mc146818.o
DRVLIBS += $(EMUOBJ)/machine/ram.o
DRVLIBS += $(EMUOBJ)/sound/ay8910.o
DRVLIBS += $(EMUOBJ)/sound/wave.o
DRVLIBS += $(EMU_VIDEO)/mc6845.o
DRVLIBS += $(OBJ)/lib/formats/ap_dsk35.o
DRVLIBS += $(OBJ)/lib/formats/basicdsk.o
DRVLIBS += $(OBJ)/lib/formats/cassimg.o
DRVLIBS += $(OBJ)/lib/formats/cqm_dsk.o
DRVLIBS += $(OBJ)/lib/formats/d88_dsk.o
DRVLIBS += $(OBJ)/lib/formats/dsk_dsk.o
DRVLIBS += $(OBJ)/lib/formats/fdi_dsk.o
DRVLIBS += $(OBJ)/lib/formats/flopimg.o
DRVLIBS += $(OBJ)/lib/formats/imageutl.o
DRVLIBS += $(OBJ)/lib/formats/imd_dsk.o
DRVLIBS += $(OBJ)/lib/formats/ioprocs.o
DRVLIBS += $(OBJ)/lib/formats/msx_dsk.o
DRVLIBS += $(OBJ)/lib/formats/td0_dsk.o
DRVLIBS += $(OBJ)/lib/formats/tzx_cas.o
DRVLIBS += $(OBJ)/lib/formats/wavfile.o
DRVLIBS += $(MESS_DEVICES)/appldriv.o
DRVLIBS += $(MESS_DEVICES)/sonydriv.o
DRVLIBS += $(MESS_DRIVERS)/amstrad.o
DRVLIBS += $(MESS_MACHINE)/amstrad.o
DRVLIBS += $(MESS_MACHINE)/upd765.o


#-------------------------------------------------
# MESS special OSD rules
#-------------------------------------------------

include $(SRC)/mess/osd/$(OSD)/$(OSD).mak
