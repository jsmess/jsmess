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

CPUS += M680X0
CPUS += 68681
CPUS += Z80
CPUS += MCS48

DRVLIBS = \
	$(EMUOBJ)/drivers/emudummy.o \
	$(EMUOBJ)/machine/68681.o \
	$(MESS_DRIVERS)/cat.o \


#-------------------------------------------------
# MESS special OSD rules
#-------------------------------------------------

include $(SRC)/mess/osd/$(OSD)/$(OSD).mak

