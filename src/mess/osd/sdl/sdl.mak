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

OSDCOREOBJS += \
	$(MESS_SDLOBJ)/configms.o	\
	$(MESS_SDLOBJ)/sdlmess.o	\
	$(MESS_SDLOBJ)/parallel.o	

$(LIBOSD): $(OSDOBJS)

ifeq ($(SUBARCH),win32)
OSDCOREOBJS += \
	$(MESS_SDLOBJ)/glob.o	\
	$(MESS_SDLOBJ)/w32util.o
endif

$(LIBOCORE): $(OSDCOREOBJS)

$(LIBOCORE_NOMAIN): $(OSDCOREOBJS:$(WINOBJ)/main.o=)

