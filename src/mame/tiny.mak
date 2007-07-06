###########################################################################
#
#   tiny.mak
#
#   Small driver-specific example makefile
#	Use make SUBTARGET=tiny to build
#
#   Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
#   Visit  http://mamedev.org for licensing and usage restrictions.
#
###########################################################################


MAMESRC = $(SRC)/mame
MAMEOBJ = $(OBJ)/mame

AUDIO = $(MAMEOBJ)/audio
DRIVERS = $(MAMEOBJ)/drivers
LAYOUT = $(MAMEOBJ)/layout
MACHINE = $(MAMEOBJ)/machine
VIDEO = $(MAMEOBJ)/video

OBJDIRS += \
	$(AUDIO) \
	$(DRIVERS) \
	$(LAYOUT) \
	$(MACHINE) \
	$(VIDEO) \



#-------------------------------------------------
# Specify all the CPU cores necessary for these
# drivers.
#-------------------------------------------------

CPUS += Z80
CPUS += M6808
CPUS += M6809



#-------------------------------------------------
# Specify all the sound cores necessary for these
# drivers.
#-------------------------------------------------

SOUNDS += CUSTOM
SOUNDS += SAMPLES
SOUNDS += DAC
SOUNDS += AY8910
SOUNDS += YM2151
SOUNDS += SN76496
SOUNDS += ASTROCADE
SOUNDS += OKIM6295
SOUNDS += HC55516



#-------------------------------------------------
# This is the list of files that are necessary
# for building all of the drivers referenced
# above.
#-------------------------------------------------

DRVLIBS = \
	$(MAMEOBJ)/tiny.o \
	$(MACHINE)/6821pia.o \
	$(MACHINE)/ticket.o \
	$(MACHINE)/z80ctc.o \
	$(DRIVERS)/astrocde.o $(VIDEO)/astrocde.o \
	$(DRIVERS)/gridlee.o $(AUDIO)/gridlee.o $(VIDEO)/gridlee.o \
	$(DRIVERS)/williams.o $(MACHINE)/williams.o $(AUDIO)/williams.o $(VIDEO)/williams.o \
	$(AUDIO)/gorf.o \
	$(AUDIO)/wow.o \
	$(DRIVERS)/polyplay.o $(AUDIO)/polyplay.o $(VIDEO)/polyplay.o \



#-------------------------------------------------
# layout dependencies
#-------------------------------------------------

$(DRIVERS)/astrocde.o: $(LAYOUT)/tenpindx.lh