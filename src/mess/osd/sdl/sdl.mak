###########################################################################
#
#   sdl.mak
#
#   SDLMESS-specific makefile
#
###########################################################################

MESS_SDLSRC = src/mess/osd/sdl
MESS_SDLOBJ = $(OBJ)/mess/osd/sdl

OBJDIRS += $(MESS_SDLOBJ)

#OSDOBJS += \

$(LIBOSD): $(OSDOBJS)

ifeq ($(TARGETOS),win32)
OSDCOREOBJS += \
	$(MESS_SDLOBJ)/winutils.o \
	$(MESS_SDLOBJ)/w32util.o
endif

$(LIBOCORE): $(OSDCOREOBJS)

$(LIBOCORE_NOMAIN): $(OSDCOREOBJS:$(WINOBJ)/main.o=)

