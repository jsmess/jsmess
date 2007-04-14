###########################################################################
#
#   sdl.mak
#
#   SDL-specific makefile
#
#   Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
#   Visit http://mamedev.org for licensing and usage restrictions.
#
#   SDLMAME by Olivier Galibert and R. Belmont 
#
###########################################################################

#-------------------------------------------------
# object and source roots
#-------------------------------------------------

SDLSRC = $(SRC)/osd/$(MAMEOS)
SDLOBJ = $(OBJ)/osd/$(MAMEOS)

OBJDIRS += $(SDLOBJ)

SDLMAIN =

#-------------------------------------------------
# OSD core library
#-------------------------------------------------

OSDCOREOBJS = \
	$(SDLOBJ)/strconv.o	\
	$(SDLOBJ)/sdldir.o	\
	$(SDLOBJ)/sdlfile.o  	\
	$(SDLOBJ)/sdlmisc.o     \
	$(SDLOBJ)/sdlsync.o     \
	$(SDLOBJ)/sdltime.o	\
	$(SDLOBJ)/sdlwork.o	

OSDOBJS =  $(SDLOBJ)/config.o $(SDLOBJ)/sdlmain.o \
	$(SDLOBJ)/fronthlp.o $(SDLOBJ)/input.o \
	$(SDLOBJ)/sound.o  $(SDLOBJ)/video.o \
	$(SDLOBJ)/drawsdl.o $(SDLOBJ)/window.o $(SDLOBJ)/keybled.o \
	$(SDLOBJ)/scale2x.o

# add the debugger includes
CFLAGS += -Isrc/debug

# add the prefix file
CFLAGS += -include $(SDLSRC)/sdlprefix.h

ifdef NO_OPENGL
DEFS += -DUSE_OPENGL=0
LIBGL=
else
DEFS += -DUSE_OPENGL=1
LIBGL=-lGL
endif

# Unix: add the necessary libraries
ifeq ($(SUBARCH),unix)
CFLAGS += `sdl-config --cflags`
LIBS += -lm `sdl-config --libs` $(LIBGL) -lX11 -lXinerama

# the new debugger relies on GTK+ in addition to the base SDLMAME needs
ifdef DEBUG
OSDOBJS += $(SDLOBJ)/debugwin.o $(SDLOBJ)/dview.o $(SDLOBJ)/debug-sup.o $(SDLOBJ)/debug-intf.o
CFLAGS += `pkg-config --cflags gtk+-2.0` `pkg-config --cflags gconf-2.0` 
LIBS += `pkg-config --libs gtk+-2.0` `pkg-config --libs gconf-2.0`
endif # DEBUG

# make sure we can find X headers
CFLAGS += -I/usr/X11/include -I/usr/X11R6/include -I/usr/openwin/include
# some systems still put important things in a different prefix
LIBS += -L/usr/X11/lib -L/usr/X11R6/lib -L/usr/openwin/lib
endif # Unix

# Win32: add the necessary libraries
ifeq ($(SUBARCH),win32)
OSDCOREOBJS += $(SDLOBJ)/main.o
SDLMAIN = $(SDLOBJ)/main.o

LIBS += -lmingw32 -lSDL -lopengl32
endif	# Win32

# Mac OS X: add the necessary libraries
ifeq ($(SUBARCH),macosx)
OSDOBJS += $(SDLOBJ)/SDLMain_tmpl.o
LIBS += -framework SDL -framework Cocoa -framework OpenGL -lpthread 

SDLMAIN = $(SDLOBJ)/SDLMain_tmpl.o

# the new debugger relies on GTK+ in addition to the base SDLMAME needs
ifdef DEBUG
OSDOBJS += $(SDLOBJ)/debugosx.o
LIBS += -framework Carbon
endif	# DEBUG
endif	# Mac OS X

TOOLS += \
	testkeys$(EXE)

# drawSDL depends on the core software renderer, so make sure it exists
$(SDLOBJ)/drawsdl.o : $(SRC)/emu/rendersw.c $(SRC)/osd/sdl/yuv_blit.c

# due to quirks of using /bin/sh, we need to explicitly specify the current path
CURPATH = ./

$(LIBOCORE): $(OSDCOREOBJS)
$(LIBOSD): $(OSDOBJS)

$(SDLOBJ)/scale2x.o: $(SDLSRC)/scale2x.c $(SDLSRC)/effect_func.h $(SDLSRC)/scale2x_core.c $(SDLSRC)/texsrc.h

#-------------------------------------------------
# testkeys
#-------------------------------------------------

$(SDLOBJ)/testkeys.o: $(SDLSRC)/testkeys.c 
	@echo Compiling $<...
	$(CC)  $(CFLAGS) -c $< -o $@
	
TESTKEYSOBJS = \
	$(SDLOBJ)/testkeys.o \

testkeys$(EXE): $(TESTKEYSOBJS) 
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(SDLMAIN) $(SDLOBJ)/strconv.o $(LIBS) -o $@
	
