###########################################################################
#
#   sdl.mak
#
#   SDL-specific makefile
#
#   Copyright (c) 1996-2009, Nicola Salmoria and the MAME Team.
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


# uncomment and edit next line to specify a distribution
# supported debian-stable, ubuntu-intrepid

# DISTRO = debian-stable
# DISTRO = ubuntu-intrepid

# uncomment next line to build without OpenGL support

# NO_OPENGL = 1

# uncomment next line to build without X11 support

# NO_X11 = 1

# uncomment and adapt next line to link against specific GL-Library
# this will also add a rpath to the executable
# MESA_INSTALL_ROOT = /usr/local/dfb_GL

# uncomment the next line to build a binary using 
# GL-dispatching. 
# This option takes precedence over MESA_INSTALL_ROOT

USE_DISPATCH_GL = 1

# uncomment and change the next line to compile and link to specific
# SDL library. This is currently only supported for unix!
# There is no need to play with this option unless you are doing
# active development on sdlmame or SDL.

#SDL_INSTALL_ROOT = /usr/local/sdl13w32
#SDL_INSTALL_ROOT = /usr/local/sdl13
#SDL_INSTALL_ROOT = /usr/local/test

###########################################################################
##################   END USER-CONFIGURABLE OPTIONS   ######################
###########################################################################

ifdef IA64
DEFS += -DSDLMAME_NOASM
endif

# bring in external flags for RPM build
CFLAGS += $(OPT_FLAGS)

#-------------------------------------------------
# distribution may change things
#-------------------------------------------------
	
ifeq ($(DISTRO),)
DISTRO = generic
else 
ifeq ($(DISTRO),debian-stable)
DEFS += -DNO_AFFINITY_NP
else 
ifeq ($(DISTRO),ubuntu-intrepid)
# Force gcc-4.2 on ubuntu-intrepid
CC = @gcc -V 4.2
LD = @gcc -V 4.2
else
$(error DISTRO $(DISTRO) unknown)
endif
endif
endif

DEFS += -DDISTRO=$(DISTRO)

#-------------------------------------------------
# sanity check the configuration
#-------------------------------------------------

ifdef BIGENDIAN
X86_MIPS3_DRC =
X86_PPC_DRC =
FORCE_DRC_C_BACKEND = 1
endif

ifdef IA64
X86_MIPS3_DRC =
X86_PPC_DRC =
FORCE_DRC_C_BACKEND = 1
endif

# OS/2 can't have OpenGL (aww)
ifeq ($(TARGETOS),os2)
NO_OPENGL = 1
DEFS += -DNO_THREAD_COOPERATIVE -DNO_DEBUGGER
endif

# additional definitions when we are on Solaris
ifeq ($(TARGETOS),solaris)
DEFS += -DNO_AFFINITY_NP -DNO_DEBUGGER -DSDLMAME_X11 -DSDLMAME_UNIX -DSDLMAME_SOLARIS
endif

#-------------------------------------------------
# compile and linking flags
#-------------------------------------------------

ifdef SYMBOLS
ifdef BIGENDIAN
ifeq ($(TARGETOS),macosx)
CFLAGS += -mlong-branch
endif	# macosx
endif	# PPC
endif	# SYMBOLS

# add an ARCH define
DEFS += "-DSDLMAME_ARCH=$(ARCHOPTS)"

# add SDLMAME TARGETOS definitions
ifeq ($(TARGETOS),linux)
TARGETOS = unix
endif

ifeq ($(TARGETOS),freebsd)
TARGETOS = unix
DEFS += -DNO_THREAD_COOPERATIVE
endif

ifeq ($(TARGETOS),openbsd)
TARGETOS = unix
DEFS += -DNO_THREAD_COOPERATIVE
endif

ifeq ($(TARGETOS),unix)
DEFS += -DSDLMAME_UNIX
ifndef NO_X11
DEFS += -DSDLMAME_X11
else
DEFS += -DSDLMAME_NO_X11 -DNO_DEBUGGER
endif
endif

ifeq ($(TARGETOS),macosx)
DEFS += -DNO_THREAD_COOPERATIVE -DSDLMAME_UNIX -DSDLMAME_MACOSX
MAINLDFLAGS = -Xlinker -all_load
ifdef BIGENDIAN
PPC=1
endif
ifdef PPC
ifdef PTR64
CCOMFLAGS += -arch ppc64
LDFLAGS += -arch ppc64
else
CCOMFLAGS += -arch ppc
LDFLAGS += -arch ppc
endif
else
ifdef PTR64
CCOMFLAGS += -arch x86_64
LDFLAGS += -arch x86_64
else
CCOMFLAGS += -arch i386
LDFLAGS += -arch i386
endif
endif
endif

ifeq ($(TARGETOS),win32)
DEFS += -DSDLMAME_WIN32 -DNO_DEBUGGER -DX64_WINDOWS_ABI
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
	$(SDLOBJ)/drawsdl.o $(SDLOBJ)/window.o $(SDLOBJ)/output.o 


ifndef NO_OPENGL
OSDOBJS += $(SDLOBJ)/drawogl.o
endif

ifdef SDL_INSTALL_ROOT
OSDOBJS += $(SDLOBJ)/draw13.o
endif

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
ifeq ($(TARGETOS),win32)
LIBGL=-lGL
else
ifdef USE_DISPATCH_GL
DEFS += -DUSE_DISPATCH_GL=1
else
LIBGL=-lGL
endif
endif
endif

#-------------------------------------------------
# specific configurations
#-------------------------------------------------

# Unix: add the necessary libraries
ifeq ($(TARGETOS),unix)

ifndef USE_DISPATCH_GL
ifdef MESA_INSTALL_ROOT
LIBS += -L$(MESA_INSTALL_ROOT)/lib
LDFLAGS += -Wl,-rpath=$(MESA_INSTALL_ROOT)/lib
CFLAGS += -I$(MESA_INSTALL_ROOT)/include
endif
endif

ifndef SDL_INSTALL_ROOT
CFLAGS += `sdl-config --cflags`
LIBS += -lm `sdl-config --libs` $(LIBGL)
else
CFLAGS += -I$(SDL_INSTALL_ROOT)/include
#LIBS += -L/opt/intel/cce/9.1.051/lib  -limf -L$(SDL_INSTALL_ROOT)/lib -Wl,-rpath,$(SDL_INSTALL_ROOT)/lib -lSDL $(LIBGL)
LIBS += -lm -L$(SDL_INSTALL_ROOT)/lib -Wl,-rpath,$(SDL_INSTALL_ROOT)/lib -lSDL $(LIBGL)
endif

ifndef NO_X11
LIBS += -lX11 -lXinerama
endif

# the new debugger relies on GTK+ in addition to the base SDLMAME needs
# Non-X11 builds can not use the debugger
ifndef NO_X11
OSDCOREOBJS += $(SDLOBJ)/debugwin.o $(SDLOBJ)/dview.o $(SDLOBJ)/debug-sup.o $(SDLOBJ)/debug-intf.o
CFLAGS += `pkg-config --cflags gtk+-2.0` `pkg-config --cflags gconf-2.0` 
LIBS += `pkg-config --libs gtk+-2.0` `pkg-config --libs gconf-2.0`
CFLAGS += -DGTK_DISABLE_DEPRECATED
else
OSDCOREOBJS += $(SDLOBJ)/debugwin.o
endif # NO_X11

# make sure we can find X headers
CFLAGS += -I/usr/X11/include -I/usr/X11R6/include -I/usr/openwin/include
# some systems still put important things in a different prefix
ifndef NO_X11
LIBS += -L/usr/X11/lib -L/usr/X11R6/lib -L/usr/openwin/lib
endif
endif # Unix

# Solaris: add the necessary object
ifeq ($(TARGETOS),solaris)
OSDCOREOBJS += $(SDLOBJ)/debugwin.o

# explicitly add some libs on Solaris
LIBS += -lSDL -lX11 -lXinerama -lm
endif # Solaris

# Win32: add the necessary libraries
ifeq ($(TARGETOS),win32)

ifdef SDL_INSTALL_ROOT
CFLAGS += -I$(SDL_INSTALL_ROOT)/include
LIBS += -L$(SDL_INSTALL_ROOT)/lib 
# -Wl,-rpath,$(SDL_INSTALL_ROOT)/lib -lSDL $(LIBGL)
endif

OSDCOREOBJS += $(SDLOBJ)/main.o
SDLMAIN = $(SDLOBJ)/main.o

# at least compile some stubs to link it
OSDCOREOBJS += $(SDLOBJ)/debugwin.o

LIBS += -lmingw32 -lSDL -lopengl32
endif	# Win32

# Mac OS X: add the necessary libraries
ifeq ($(TARGETOS),macosx)
OSDCOREOBJS += $(SDLOBJ)/osxutils.o
OSDOBJS += $(SDLOBJ)/SDLMain_tmpl.o

ifndef MACOSX_USE_LIBSDL
# Compile using framework (compile using libSDL is the exception)
LIBS += -framework SDL -framework Cocoa -framework OpenGL -lpthread 
else
# Compile using installed libSDL (Fink or MacPorts):
#
# Remove the "/SDL" component from the include path so that we can compile
# files (header files are #include "SDL/something.h", so the extra "/SDL"
# causes a significant problem)
CFLAGS += `sdl-config --cflags | sed 's:/SDL::'` -DNO_SDL_GLEXT
# Remove libSDLmain, as its symbols conflict with SDLMain_tmpl.m
LIBS += `sdl-config --libs | sed 's/-lSDLmain//'` -lpthread
endif

SDLMAIN = $(SDLOBJ)/SDLMain_tmpl.o

# the new debugger relies on GTK+ in addition to the base SDLMAME needs
OSDOBJS += $(SDLOBJ)/debugosx.o
LIBS += -framework Carbon
endif	# Mac OS X

# OS2: add the necessary libraries
ifeq ($(TARGETOS),os2)
OSDCOREOBJS += $(SDLOBJ)/debugwin.o

CFLAGS += `sdl-config --cflags`
LIBS += `sdl-config --libs`

# to avoid name clash of '_brk'
$(OBJ)/emu/cpu/h6280/6280dasm.o : CDEFS += -D__STRICT_ANSI__
endif # OS2

TOOLS += \
	testkeys$(EXE)

# drawSDL depends on the core software renderer, so make sure it exists
$(SDLOBJ)/drawsdl.o : $(SRC)/emu/rendersw.c

$(SDLOBJ)/drawogl.o : $(SDLSRC)/drawogl.c $(SDLSRC)/texcopy.c $(SDLSRC)/scale2x.c $(SDLSRC)/scale2x_core.c

# draw13 depends
$(SDLOBJ)/draw13.o : $(SDLSRC)/blit13.h

# due to quirks of using /bin/sh, we need to explicitly specify the current path
CURPATH = ./

$(LIBOCORE): $(OSDCOREOBJS)
$(LIBOSD): $(OSDOBJS)

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
	
testlib:
	-echo LIBS: $(LIBS)
	-echo DEFS: $(DEFS)
