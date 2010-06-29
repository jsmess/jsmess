###########################################################################
#
#   tools.mak
#
#   MESS tools makefile
#
###########################################################################


# add include path to tools directory
CFLAGS += -I$(MESSSRC)/tools

# tools object directory
MESS_TOOLS = $(MESSOBJ)/tools


ifdef BUILD_IMGTOOL
include $(MESSSRC)/tools/imgtool/imgtool.mak
TOOLS += $(IMGTOOL)
endif

ifdef BUILD_CASTOOL
include $(MESSSRC)/tools/castool/castool.mak
TOOLS += $(CASTOOL)
endif

#ifdef BUILD_MESSTEST
#include $(MESSSRC)/tools/messtest/messtest.mak
#TOOLS += $(MESSTEST)
#endif

ifdef BUILD_DAT2HTML
include $(MESSSRC)/tools/dat2html/dat2html.mak
TOOLS += $(DAT2HTML)
endif

# include OS-specific MESS stuff
ifeq ($(OSD),windows)
include $(MESSSRC)/tools/messdocs/messdocs.mak

ifdef BUILD_WIMGTOOL
include $(MESSSRC)/tools/imgtool/windows/wimgtool.mak
TOOLS += $(WIMGTOOL)
endif
endif 