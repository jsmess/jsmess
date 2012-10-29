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

CPUS += F8
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
  $(MESS_VIDEO)/channelf.o  \
  $(MESS_AUDIO)/channelf.o  \
  $(MESS_DRIVERS)/channelf.o  \

#-------------------------------------------------
# MESS special OSD rules
#-------------------------------------------------

include $(SRC)/mess/osd/$(OSD)/$(OSD).mak
