###########################################################################
#
#   atari2600.mak
#
# Atari 2600-specific Makefile
#include "machine/6532riot.h"

#include "sound/wave.h"
#include "sound/tiaintf.h"
#include "imagedev/cartslot.h"
#include "imagedev/cassette.h"
#include "formats/a26_cas.h"
#include "video/tia.h"
#include "hashfile.h"
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
CPUS += M6809
CPUS += Z80
CPUS += MCS48

#-------------------------------------------------
# Specify all the sound cores necessary for the
# drivers referenced in colecovision.lst
#-------------------------------------------------

#SOUNDS += CUSTOM
#SOUNDS += WAVE
#SOUNDS += TIA

#-------------------------------------------------
# This is the list of files that are necessary
# for building all of the drivers referenced
# in colecovision.lst
#-------------------------------------------------

DRVLIBS = \
  $(EMUOBJ)/drivers/emudummy.o \
  $(MESS_MACHINE)/ieee488.o \
  $(MESS_MACHINE)/ieeestub.o  \
  $(EMUOBJ)/video/mc6845.o \
  $(EMUOBJ)/machine/6821pia.o \
  $(EMUOBJ)/machine/6522via.o \
  $(EMUOBJ)/machine/6532riot.o \
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
  $(MESS_MACHINE)/c2040.o \
  $(MESS_MACHINE)/mos6530.o \
  $(MESS_VIDEO)/pet.o     \
  $(MESS_DRIVERS)/pet.o   \
  $(MESS_MACHINE)/pet.o   \
  $(MESS_FORMATS)/cbm_snqk.o  \
  $(MESS_MACHINE)/cbm.o   \
  $(MESS_MACHINE)/cbmipt.o  \

#-------------------------------------------------
# MESS special OSD rules
#-------------------------------------------------

include $(SRC)/mess/osd/$(OSD)/$(OSD).mak
