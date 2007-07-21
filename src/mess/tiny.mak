###########################################################################
#
#   tiny.mak
#
#   Small driver-specific example makefile
#	Use make SUBTARGET=tiny to build
#
#   As an example this makefile builds MESS with the three Colecovision
#   drivers enabled only.
#
#   Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
#   Visit  http://mamedev.org for licensing and usage restrictions.
#
###########################################################################

# root object directories
MAMEOBJ = $(OBJ)/mame
MESSOBJ = $(OBJ)/mess

# MAME directories
AUDIO = $(MAMEOBJ)/audio
DRIVERS = $(MAMEOBJ)/drivers
LAYOUT = $(MAMEOBJ)/layout
MACHINE = $(MAMEOBJ)/machine
VIDEO = $(MAMEOBJ)/video

# MESS directories
MESS_AUDIO = $(MESSOBJ)/audio
MESS_DEVICES = $(MESSOBJ)/devices
MESS_DRIVERS = $(MESSOBJ)/drivers
MESS_FORMATS = $(MESSOBJ)/formats
MESS_LAYOUT = $(MESSOBJ)/layout
MESS_MACHINE = $(MESSOBJ)/machine
MESS_VIDEO = $(MESSOBJ)/video

OBJDIRS += \
	$(AUDIO) \
	$(DRIVERS) \
	$(LAYOUT) \
	$(MACHINE) \
	$(VIDEO) \
	$(MESS_AUDIO) \
	$(MESS_DEVICES) \
	$(MESS_DRIVERS) \
	$(MESS_FORMATS) \
	$(MESS_LAYOUT) \
	$(MESS_MACHINE) \
	$(MESS_VIDEO) \



#-------------------------------------------------
# Specify all the CPU cores necessary for the
# drivers referenced in tiny.c.
#-------------------------------------------------

CPUS += Z80



#-------------------------------------------------
# Specify all the sound cores necessary for the
# drivers referenced in tiny.c.
#-------------------------------------------------

SOUNDS += SN76496



#-------------------------------------------------
# This is the list of files that are necessary
# for building all of the drivers referenced
# in tiny.c
#-------------------------------------------------

DRVLIBS = \
	$(MESSOBJ)/tiny.o \
	$(MESS_MACHINE)/coleco.o \
	$(MESS_DRIVERS)/coleco.o \
	$(VIDEO)/tms9928a.o \
	$(MESS_DEVICES)/cartslot.o \



#-------------------------------------------------
# layout dependencies
#-------------------------------------------------



