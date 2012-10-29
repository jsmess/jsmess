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

CPUS += TMS9900
CPUS += Z80
CPUS += MCS48



#-------------------------------------------------
# Specify all the sound cores necessary for the
# drivers referenced in colecovision.lst
#-------------------------------------------------

SOUNDS += SN76496
SOUNDS += WAVE
SOUNDS += TMS5220
SOUNDS += DAC



#-------------------------------------------------
# This is the list of files that are necessary
# for building all of the drivers referenced
# in colecovision.lst
#-------------------------------------------------

DRVLIBS = \
	$(EMUOBJ)/drivers/emudummy.o         \
  $(MESS_DEVICES)/appldriv.o           \
  $(MESS_MACHINE)/at29040a.o           \
  $(MESS_MACHINE)/smc92x4.o            \
  $(MESS_MACHINE)/mm58274c.o           \
  $(MESS_MACHINE)/smartmed.o           \
  $(MESS_MACHINE)/strata.o             \
  $(EMUOBJ)/machine/rtc65271.o         \
  $(EMUOBJ)/machine/idectrl.o          \
  $(EMUOBJ)/machine/wd17xx.o           \
  $(MESS_MACHINE)/ti99/hsgpl.o         \
  $(MESS_MACHINE)/ti99/p_code.o        \
  $(MESS_MACHINE)/ti99/crubus.o        \
  $(MESS_MACHINE)/ti99/datamux.o       \
  $(MESS_MACHINE)/ti99/peribox.o       \
  $(MESS_MACHINE)/ti99/grom.o          \
  $(MESS_MACHINE)/ti99/gromport.o      \
  $(MESS_MACHINE)/ti99/ti32kmem.o      \
  $(MESS_MACHINE)/ti99/samsmem.o       \
  $(MESS_MACHINE)/ti99/memex.o         \
  $(MESS_MACHINE)/ti99/myarcmem.o      \
  $(MESS_MACHINE)/ti99/ti_fdc.o        \
  $(MESS_MACHINE)/ti99/hfdc.o          \
  $(MESS_MACHINE)/ti99/bwg.o           \
  $(MESS_MACHINE)/ti99/ti_rs232.o      \
  $(MESS_MACHINE)/ti99/tn_usbsm.o      \
  $(MESS_MACHINE)/ti99/tn_ide.o        \
  $(MESS_MACHINE)/ti99/spchsyn.o       \
  $(MESS_MACHINE)/ti99/evpc.o          \
  $(MESS_MACHINE)/ti99/videowrp.o      \
  $(MESS_MACHINE)/ti99/mapper8.o       \
  $(MESS_MACHINE)/ti99/speech8.o       \
  $(MESS_MACHINE)/ti99/mecmouse.o      \
  $(MESS_MACHINE)/ti99/handset.o       \
  $(MESS_MACHINE)/ti99/sgcpu.o         \
  $(MESS_MACHINE)/ti99/genboard.o      \
  $(MESS_MACHINE)/ti99/tiboard.o       \
  $(MESS_MACHINE)/ti99/ti99_hd.o       \
  $(MESS_MACHINE)/tms9901.o            \
  $(MESS_MACHINE)/tms9902.o            \
  $(MESS_DRIVERS)/ti99_4x.o            \
  $(MESS_DRIVERS)/ti99_4p.o            \
  $(OBJ)/lib/formats/flopimg.o         \
  $(OBJ)/lib/formats/fdi_dsk.o         \
  $(OBJ)/lib/formats/td0_dsk.o         \
  $(OBJ)/lib/formats/imd_dsk.o         \
  $(OBJ)/lib/formats/cqm_dsk.o         \
  $(OBJ)/lib/formats/dsk_dsk.o         \
  $(OBJ)/lib/formats/d88_dsk.o         \
  $(OBJ)/lib/formats/ti99_dsk.o        \
  $(EMUOBJ)/video/v9938.o              \


#-------------------------------------------------
# MESS special OSD rules
#-------------------------------------------------

include $(SRC)/mess/osd/$(OSD)/$(OSD).mak
