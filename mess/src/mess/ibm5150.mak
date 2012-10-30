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

CPUS += I86
CPUS += NEC
CPUS += Z80
CPUS += MCS48
CPUS += MCS51



#-------------------------------------------------
# Specify all the sound cores necessary for the
# drivers referenced in colecovision.lst
#-------------------------------------------------

#SOUNDS += SN76496
SOUNDS += SPEAKER
SOUNDS += YM3812
SOUNDS += SAA1099


#-------------------------------------------------
# This is the list of files that are necessary
# for building all of the drivers referenced
# in colecovision.lst
#-------------------------------------------------

DRVLIBS = \
	$(EMUOBJ)/drivers/emudummy.o \
  $(MESS_DRIVERS)/genpc.o   \
  $(MESS_MACHINE)/genpc.o   \
  $(MESS_MACHINE)/isa.o \
  $(MESS_MACHINE)/kb_keytro.o \
  $(MESS_VIDEO)/ibm_vga.o \
  $(MESS_VIDEO)/isa_mda.o \
  $(MESS_MACHINE)/isa_com.o \
  $(MESS_MACHINE)/isa_fdc.o \
  $(MESS_MACHINE)/isa_hdc.o \
  $(MESS_MACHINE)/isa_adlib.o \
  $(MESS_MACHINE)/isa_gblaster.o \
  $(MESS_MACHINE)/isa_sblaster.o \
  $(EMUOBJ)/video/mc6845.o \
  $(MESS_MACHINE)/pc_lpt.o \
  $(MESS_MACHINE)/upd765.o \
  $(MESS_VIDEO)/pc_cga.o \
  $(MESS_VIDEO)/pc_ega.o \
  $(MESS_VIDEO)/cgapal.o \
  $(MESS_VIDEO)/crtc_ega.o \
  $(EMUOBJ)/machine/ins8250.o \
  $(EMUOBJ)/machine/pit8253.o \
  $(EMUOBJ)/machine/8237dma.o \
  $(EMUOBJ)/machine/pic8259.o \
  $(EMUOBJ)/machine/i8255.o \
  $(EMUOBJ)/machine/ctronics.o \
  $(OBJ)/lib/formats/flopimg.o         \
  $(OBJ)/lib/formats/fdi_dsk.o         \
  $(OBJ)/lib/formats/td0_dsk.o         \
  $(OBJ)/lib/formats/imd_dsk.o         \
  $(OBJ)/lib/formats/cqm_dsk.o         \
  $(OBJ)/lib/formats/dsk_dsk.o         \
  $(OBJ)/lib/formats/d88_dsk.o         \
  $(OBJ)/lib/formats/pc_dsk.o         \
  $(OBJ)/lib/formats/basicdsk.o \
  $(MESS_DEVICES)/appldriv.o           \
  $(MESS_DRIVERS)/ibmpc.o   \


#-------------------------------------------------
# MESS special OSD rules
#-------------------------------------------------

include $(SRC)/mess/osd/$(OSD)/$(OSD).mak
