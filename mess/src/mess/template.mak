###########################################################################
#
#   template.mak
#
#   Nothing-specific Makefile
#
###########################################################################

MESSUI = 0
include $(SRC)/mess/messcore.mak
include $(SRC)/mess/osd/$(OSD)/$(OSD).mak
include $(SRC)/mess/emudummy.mak

# CPUS +=

# SOUNDS +=

# DRVLIBS +=

include $(SRC)/mess/osd/$(OSD)/$(OSD).mak
