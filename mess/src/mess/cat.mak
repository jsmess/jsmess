###########################################################################
#
#   cat.mak
#
#   Canon Cat-specific Makefile
#
###########################################################################

# disable messui for tiny build
MESSUI = 0

# include MESS core defines
include $(SRC)/mess/messcore.mak
include $(SRC)/mess/osd/$(OSD)/$(OSD).mak
include $(SRC)/mess/emudummy.mak

#-------------------------------------------------
# Specify all the CPU cores necessary for the
# drivers referenced in cat.lst
#-------------------------------------------------

CPUS += M680X0

DRVLIBS += $(MESS_DRIVERS)/cat.o
DRVLIBS += $(EMU_MACHINE)/68681.o

#-------------------------------------------------
# MESS special OSD rules
#-------------------------------------------------

include $(SRC)/mess/osd/$(OSD)/$(OSD).mak

