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

###########################################################################
#################   BEGIN USER-CONFIGURABLE OPTIONS   #####################
###########################################################################


#-------------------------------------------------
# specify build options; see each option below
# for details
#-------------------------------------------------

# uncomment next line to build without OpenGL support
# NO_OPENGL = 1


###########################################################################
##################   END USER-CONFIGURABLE OPTIONS   ######################
###########################################################################

# bring in external flags for RPM build
CFLAGS += $(OPT_FLAGS)

#-------------------------------------------------
# sanity check the configuration
#-------------------------------------------------

# disable DRC cores for PowerPC builds
ifdef G4
PPC = 1
endif

ifdef G5
PPC = 1
endif

ifdef CELL
PPC = 1
endif

ifdef PPC
X86_MIPS3_DRC =
X86_PPC_DRC =
endif

# OS/2 can't have OpenGL (aww)
ifeq ($(TARGETOS),os2)
NO_OPENGL = 1
endif

#-------------------------------------------------
# compile and linking flags
#-------------------------------------------------

ifdef SYMBOLS
ifdef PPC
ifeq ($(TARGETOS),macosx)
CFLAGS += -mlong-branch
endif	# macosx
endif	# PPC
endif	# SYMBOLS



# add SDLMAME TARGETOS definitions
ifeq ($(TARGETOS),linux)
TARGETOS = unix
endif

ifeq ($(TARGETOS),freebsd)
TARGETOS = unix
endif

ifeq ($(TARGETOS),unix)
DEFS += -DSDLMAME_UNIX -DSDLMAME_X11
endif

ifeq ($(TARGETOS),macosx)
DEFS += -DSDLMAME_UNIX -DSDLMAME_MACOSX
MAINLDFLAGS = -Xlinker -all_load
ifdef PPC
ifdef PTR64
CFLAGS += -arch ppc64
LDFLAGS += -arch ppc64
else
CFLAGS += -arch ppc
LDFLAGS += -arch ppc
endif
else
ifdef PTR64
CFLAGS += -arch x86_64
LDFLAGS += -arch x86_64
else
CFLAGS += -arch i386
LDFLAGS += -arch i386
endif
endif
endif

ifeq ($(TARGETOS),win32)
DEFS += -DSDLMAME_WIN32
endif

ifeq ($(TARGETOS),os2)
DEFS += -DSDLMAME_OS2
endif

#-------------------------------------------------
# object and source roots
#-------------------------------------------------

SDLSRC = $(SRC)/osd/$(OSD)
SDLOBJ = $(OBJ)/osd/$(OSD)

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

OSDOBJS =  $(SDLOBJ)/sdlmain.o \
	$(SDLOBJ)/input.o \
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
OSDOBJS += $(SDLOBJ)/gl_shader_tool.o $(SDLOBJ)/gl_shader_mgr.o
DEFS += -DUSE_OPENGL=1
LIBGL=-lGL
endif

# Unix: add the necessary libraries
ifeq ($(TARGETOS),unix)
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
ifeq ($(TARGETOS),win32)
OSDCOREOBJS += $(SDLOBJ)/main.o
SDLMAIN = $(SDLOBJ)/main.o

# at least compile some stubs to link it
ifdef DEBUG
OSDOBJS += $(SDLOBJ)/debugwin.o
endif

LIBS += -lmingw32 -lSDL -lopengl32
endif	# Win32

# Mac OS X: add the necessary libraries
ifeq ($(TARGETOS),macosx)
OSDCOREOBJS += $(SDLOBJ)/osxutils.o
OSDOBJS += $(SDLOBJ)/SDLMain_tmpl.o
LIBS += -framework SDL -framework Cocoa -framework OpenGL -lpthread 

SDLMAIN = $(SDLOBJ)/SDLMain_tmpl.o

# the new debugger relies on GTK+ in addition to the base SDLMAME needs
ifdef DEBUG
OSDOBJS += $(SDLOBJ)/debugosx.o
LIBS += -framework Carbon
endif	# DEBUG
endif	# Mac OS X

# OS2: add the necessary libraries
ifeq ($(TARGETOS),os2)
CFLAGS += `sdl-config --cflags`
LIBS += `sdl-config --libs`
endif # OS2

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
	$(CC)  $(CFLAGS) $(DEFS) -c $< -o $@
	
TESTKEYSOBJS = \
	$(SDLOBJ)/testkeys.o \

testkeys$(EXE): $(TESTKEYSOBJS) $(LIBUTIL)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(SDLMAIN) $(SDLOBJ)/strconv.o $(LIBS) -o $@
	
