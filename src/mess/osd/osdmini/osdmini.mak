###########################################################################
#
#   sdl.mak
#
#   SDLMESS-specific makefile
#
###########################################################################

MESS_SDLSRC = src/mess/osd/osdmini
MESS_SDLOBJ = $(OBJ)/mess/osd/osdmini

OBJDIRS += $(MESS_SDLOBJ)

#OSDOBJS += \

$(LIBOSD): $(OSDOBJS)

$(LIBOCORE): $(OSDCOREOBJS)

$(LIBOCORE_NOMAIN): $(OSDCOREOBJS:$(WINOBJ)/main.o=)

