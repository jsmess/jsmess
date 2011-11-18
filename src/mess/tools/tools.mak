###########################################################################
#
#   tools.mak
#
#   MESS tools makefile
#
###########################################################################


# add include path to tools directory
INCPATH += -I$(MESSSRC)/tools

# tools object directory
MESS_TOOLS = $(MESSOBJ)/tools

include $(MESSSRC)/tools/imgtool/imgtool.mak
TOOLS += $(IMGTOOL)

include $(MESSSRC)/tools/castool/castool.mak
TOOLS += $(CASTOOL)

include $(MESSSRC)/tools/floptool/floptool.mak
TOOLS += $(FLOPTOOL)

# include OS-specific MESS stuff
# the following expression is true if OSD is windows or winui
ifeq ($(OSD),$(filter $(OSD),windows winui))
include $(MESSSRC)/tools/imgtool/windows/wimgtool.mak
TOOLS += $(WIMGTOOL)
endif 