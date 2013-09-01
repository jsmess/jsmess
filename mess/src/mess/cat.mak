## 
## cat.mak
## 

MESSUI = 0
include $(SRC)/mess/messcore.mak
include $(SRC)/mess/osd/$(OSD)/$(OSD).mak
include $(SRC)/mess/emudummy.mak

# CPUS +=
CPUS +=M680X0       # Motorola 68000 series

# SOUNDS +=

# DRVLIBS +=
DRVLIBS += $(EMUOBJ)/cpu/m68000/m68kcpu.o
DRVLIBS += $(EMUOBJ)/cpu/m68000/m68kdasm.o
DRVLIBS += $(EMUOBJ)/cpu/m68000/m68kops.o
DRVLIBS += $(EMUOBJ)/machine/68681.o
DRVLIBS += $(EMU_VIDEO)/generic.o
DRVLIBS += $(MESS_DRIVERS)/cat.o

include $(SRC)/mess/osd/$(OSD)/$(OSD).mak
