## 
## ibmpc.mak
## 

MESSUI = 0
include $(SRC)/mess/messcore.mak
include $(SRC)/mess/osd/$(OSD)/$(OSD).mak
include $(SRC)/mess/emudummy.mak

# CPUS +=
CPUS +=I386                             
CPUS +=I86          # Intel 80x86 series
CPUS +=MCS51        # Intel 8051 and derivatives

# SOUNDS +=

# DRVLIBS +=
DRVLIBS += $(EMUOBJ)/cpu/i386/i386dasm.o
DRVLIBS += $(EMUOBJ)/cpu/i86/i86.o
DRVLIBS += $(EMUOBJ)/cpu/mcs51/mcs51dasm.o
DRVLIBS += $(EMUOBJ)/cpu/mcs51/mcs51.o
DRVLIBS += $(EMUOBJ)/machine/8237dma.o
DRVLIBS += $(EMUOBJ)/machine/i8255.o
DRVLIBS += $(EMUOBJ)/machine/ins8250.o
DRVLIBS += $(EMUOBJ)/machine/pic8259.o
DRVLIBS += $(EMUOBJ)/machine/pit8253.o
DRVLIBS += $(EMUOBJ)/machine/ram.o
DRVLIBS += $(EMUOBJ)/sound/3812intf.o
DRVLIBS += $(EMUOBJ)/sound/fmopl.o
DRVLIBS += $(EMUOBJ)/sound/saa1099.o
DRVLIBS += $(EMUOBJ)/sound/speaker.o
DRVLIBS += $(EMUOBJ)/sound/ymdeltat.o
DRVLIBS += $(EMU_VIDEO)/mc6845.o
DRVLIBS += $(OBJ)/lib/formats/ap_dsk35.o
DRVLIBS += $(OBJ)/lib/formats/basicdsk.o
DRVLIBS += $(OBJ)/lib/formats/cassimg.o
DRVLIBS += $(OBJ)/lib/formats/cqm_dsk.o
DRVLIBS += $(OBJ)/lib/formats/d88_dsk.o
DRVLIBS += $(OBJ)/lib/formats/dsk_dsk.o
DRVLIBS += $(OBJ)/lib/formats/fdi_dsk.o
DRVLIBS += $(OBJ)/lib/formats/flopimg.o
DRVLIBS += $(OBJ)/lib/formats/imageutl.o
DRVLIBS += $(OBJ)/lib/formats/imd_dsk.o
DRVLIBS += $(OBJ)/lib/formats/ioprocs.o
DRVLIBS += $(OBJ)/lib/formats/pc_dsk.o
DRVLIBS += $(OBJ)/lib/formats/td0_dsk.o
DRVLIBS += $(OBJ)/lib/formats/wavfile.o
DRVLIBS += $(MESS_DEVICES)/appldriv.o
DRVLIBS += $(MESS_DEVICES)/sonydriv.o
DRVLIBS += $(MESS_DRIVERS)/ibmpc.o
DRVLIBS += $(MESS_MACHINE)/genpc.o
DRVLIBS += $(MESS_MACHINE)/isa_adlib.o
DRVLIBS += $(MESS_MACHINE)/isa_com.o
DRVLIBS += $(MESS_MACHINE)/isa_fdc.o
DRVLIBS += $(MESS_MACHINE)/isa_gblaster.o
DRVLIBS += $(MESS_MACHINE)/isa_hdc.o
DRVLIBS += $(MESS_MACHINE)/isa.o
DRVLIBS += $(MESS_MACHINE)/isa_sblaster.o
DRVLIBS += $(MESS_MACHINE)/kb_keytro.o
DRVLIBS += $(MESS_MACHINE)/upd765.o
DRVLIBS += $(MESS_VIDEO)/cgapal.o
DRVLIBS += $(MESS_VIDEO)/pc_cga.o

include $(SRC)/mess/osd/$(OSD)/$(OSD).mak
