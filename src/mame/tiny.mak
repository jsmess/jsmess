###########################################################################
#
#   tiny.mak
#
#   Small driver-specific example makefile
#	Use make SUBTARGET=tiny to build
#
#   Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
#   Visit http://mamedev.org for licensing and usage restrictions.
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
# You need to define two strings:
#
#	TINY_NAME is a comma-separated list of driver
#	names that will be referenced.
#
#	TINY_DRIVER should be the same list but with
#	an & in front of each name.
#-------------------------------------------------

COREDEFS += -DTINY_NAME="driver_robby,driver_gridlee,driver_polyplay,driver_alienar"
COREDEFS += -DTINY_POINTER="&driver_robby,&driver_gridlee,&driver_polyplay,&driver_alienar"



#-------------------------------------------------
# Specify all the CPU cores necessary for these
# drivers.
#-------------------------------------------------

CPUS += Z80
CPUS += M6809



#-------------------------------------------------
# Specify all the sound cores necessary for these
# drivers.
#-------------------------------------------------

SOUNDS += CUSTOM
SOUNDS += SAMPLES
SOUNDS += SN76496
SOUNDS += ASTROCADE
SOUNDS += DAC
SOUNDS += HC55516
SOUNDS += YM2151
SOUNDS += OKIM6295



#-------------------------------------------------
# This is the list of files that are necessary
# for building all of the drivers referenced
# above.
#-------------------------------------------------

DRVLIBS = \
	$(MAMEOBJ)/tiny.o \
	$(MACHINE)/6821pia.o \
	$(MACHINE)/ticket.o \
	$(VIDEO)/res_net.o \
	$(DRIVERS)/astrocde.o $(MACHINE)/astrocde.o $(VIDEO)/astrocde.o \
	$(AUDIO)/gorf.o $(AUDIO)/wow.o \
	$(DRIVERS)/gridlee.o $(AUDIO)/gridlee.o $(VIDEO)/gridlee.o \
	$(DRIVERS)/polyplay.o $(AUDIO)/polyplay.o $(VIDEO)/polyplay.o \
	$(DRIVERS)/williams.o $(MACHINE)/williams.o $(AUDIO)/williams.o $(VIDEO)/williams.o \
