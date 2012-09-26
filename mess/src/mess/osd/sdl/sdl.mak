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

$(LIBOCORE): $(OSDCOREOBJS)

$(LIBOCORE_NOMAIN): $(OSDCOREOBJS:$(WINOBJ)/main.o=)

