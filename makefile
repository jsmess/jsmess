###########################################################################
#
#   makefile
#
#   Core makefile for building MAME and derivatives
#
#   Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
#   Visit http://mamedev.org for licensing and usage restrictions.
#
###########################################################################



###########################################################################
#################   BEGIN USER-CONFIGURABLE OPTIONS   #####################
###########################################################################


#-------------------------------------------------
# specify core target: mame, mess, etc.
# specify subtarget: mame, mess, tiny, etc.
# build rules will be included from 
# $(TARGET)/$(SUBTARGET).mak
#-------------------------------------------------

ifndef TARGET
TARGET = mame
endif

ifndef SUBTARGET
SUBTARGET = $(TARGET)
endif



#-------------------------------------------------
# specify operating system: windows, msdos, etc.
# build rules will be includes from 
# src/osd/$(MAMEOS)/$(MAMEOS).mak
#-------------------------------------------------

ifndef MAMEOS
MAMEOS = windows
endif



#-------------------------------------------------
# specify program options; see each option below 
# for details
#-------------------------------------------------

# uncomment next line to include the debugger
# DEBUG = 1

# uncomment next line to use DRC MIPS3 engine
X86_MIPS3_DRC = 1

# uncomment next line to use DRC PowerPC engine
X86_PPC_DRC = 1

# uncomment next line to use DRC Voodoo rasterizers
# X86_VOODOO_DRC = 1



#-------------------------------------------------
# specify build options; see each option below 
# for details
#-------------------------------------------------

# uncomment one of the next lines to build a target-optimized build
# ATHLON = 1
# I686 = 1
# P4 = 1
# PM = 1
# AMD64 = 1

# uncomment next line if you are building for a 64-bit target
# PTR64 = 1

# uncomment next line to build expat as part of MAME build
BUILD_EXPAT = 1

# uncomment next line to build zlib as part of MAME build
BUILD_ZLIB = 1

# uncomment next line to include the symbols
# SYMBOLS = 1

# uncomment next line to generate a link map for exception handling in windows
# MAP = 1

# specify optimization level or leave commented to use the default
# (default is OPTIMIZE = 3 normally, or OPTIMIZE = 0 with symbols)
# OPTIMIZE = 3


###########################################################################
##################   END USER-CONFIGURABLE OPTIONS   ######################
###########################################################################


#-------------------------------------------------
# sanity check the configuration
#-------------------------------------------------

# disable DRC cores for 64-bit builds
ifdef PTR64
X86_MIPS3_DRC =
X86_PPC_DRC =
endif

# specify a default optimization level if none explicitly stated
ifndef OPTIMIZE
ifndef SYMBOLS
OPTIMIZE = 3
else
OPTIMIZE = 0
endif
endif



#-------------------------------------------------
# platform-specific definitions
#-------------------------------------------------

# extension for executables
EXE = .exe

# compiler, linker and utilities
AR = @ar
CC = @gcc
LD = @gcc
MD = -mkdir.exe
RM = @rm -f



#-------------------------------------------------
# form the name of the executable
#-------------------------------------------------

ifeq ($(MAMEOS),msdos)
PREFIX = d
endif

# by default, compile for Pentium target
ifeq ($(TARGET),$(SUBTARGET))
NAME = $(TARGET)
else
NAME = $(TARGET)$(SUBTARGET)
endif
ARCH = -march=pentium

# architecture-specific builds get extra options
ifdef ATHLON
SUFFIX = at
ARCH = -march=athlon
endif

ifdef I686
SUFFIX = pp
ARCH = -march=pentiumpro
endif

ifdef P4
SUFFIX = p4
ARCH = -march=pentium4
endif

ifdef AMD64
SUFFIX = 64
ARCH = -march=athlon64
endif

ifdef PM
SUFFIX = pm
ARCH = -march=pentium3 -msse2
endif

# debug builds just get the 'd' suffix and nothing more
ifdef DEBUG
SUFFIX = d
endif

FULLNAME = $(PREFIX)$(NAME)$(SUFFIX)

EMULATOR = $(FULLNAME)$(EXE)

# build the targets in different object dirs, since mess changes
# some structures and thus they can't be linked against each other.
OBJ = obj/$(FULLNAME)

SRC = src



#-------------------------------------------------
# compile-time definitions
#-------------------------------------------------

DEFS = -DX86_ASM -DLSB_FIRST -DINLINE="static __inline__" -Dasm=__asm__ -DCRLF=3

ifdef PTR64
DEFS += -DPTR64
endif

ifdef DEBUG
DEFS += -DMAME_DEBUG
endif

ifdef X86_VOODOO_DRC
DEFS += -DVOODOO_DRC
endif



#-------------------------------------------------
# compile and linking flags
#-------------------------------------------------

CFLAGS = \
	-std=gnu89 \
	-I$(SRC)/$(TARGET) \
	-I$(SRC)/$(TARGET)/includes \
	-I$(OBJ)/$(TARGET)/layout \
	-I$(SRC)/emu \
	-I$(OBJ)/emu \
	-I$(OBJ)/emu/layout \
	-I$(SRC)/lib/util \
	-I$(SRC)/osd \
	-I$(SRC)/osd/$(MAMEOS) \

ifdef SYMBOLS
CFLAGS += -g
endif

CFLAGS += -Wall \
	-Wpointer-arith \
	-Wbad-function-cast \
	-Wcast-align \
	-Wstrict-prototypes \
	-Wundef \
	-Wformat-security \
	-Wwrite-strings \
	-Wdeclaration-after-statement \
	-Wno-unused-functions \

ifneq ($(OPTIMIZE),0)
CFLAGS += -Werror -DNDEBUG $(ARCH) -fno-strict-aliasing
endif

CFLAGS += -O$(OPTIMIZE)

# extra options needed *only* for the osd files
CFLAGSOSDEPEND = $(CFLAGS)

LDFLAGS = -WO

ifdef SYMBOLS
LDFLAGS =
else
LDFLAGS += -s
endif

ifdef MAP
MAPFLAGS = -Wl,-Map,$(FULLNAME).map
else
MAPFLAGS =
endif



#-------------------------------------------------
# define the standard object directory; other
# projects can add their object directories to
# this variable
#-------------------------------------------------

OBJDIRS = $(OBJ)



#-------------------------------------------------
# define standard libarires for CPU and sounds
#-------------------------------------------------

LIBEMU = $(OBJ)/libemu.a
LIBCPU = $(OBJ)/libcpu.a
LIBSOUND = $(OBJ)/libsound.a
LIBUTIL = $(OBJ)/libutil.a
LIBOCORE = $(OBJ)/libocore.a
LIBOSD = $(OBJ)/libosd.a

VERSIONOBJ = $(OBJ)/version.o



#-------------------------------------------------
# either build or link against the included 
# libraries
#-------------------------------------------------

# start with an empty set of libs
LIBS = 

# add expat XML library
ifdef BUILD_EXPAT
CFLAGS += -I$(SRC)/lib/expat
EXPAT = $(OBJ)/libexpat.a
else
LIBS += -lexpat
EXPAT =
endif

# add ZLIB compression library
ifdef BUILD_ZLIB
CFLAGS += -I$(SRC)/lib/zlib
ZLIB = $(OBJ)/libz.a
else
LIBS += -lz
ZLIB =
endif



#-------------------------------------------------
# 'all' target needs to go here, before the 
# include files which define additional targets
#-------------------------------------------------

all: maketree emulator tools



#-------------------------------------------------
# include the various .mak files
#-------------------------------------------------

# include OS-specific rules first
include $(SRC)/osd/$(MAMEOS)/$(MAMEOS).mak

# then the various core pieces
include $(SRC)/$(TARGET)/$(SUBTARGET).mak
include $(SRC)/emu/emu.mak
include $(SRC)/lib/lib.mak
include $(SRC)/tools/tools.mak

# combine the various definitions to one
CDEFS = $(DEFS) $(COREDEFS) $(CPUDEFS) $(SOUNDDEFS)



#-------------------------------------------------
# primary targets
#-------------------------------------------------

emulator: maketree $(EMULATOR)

tools: maketree $(TOOLS)

maketree: $(sort $(OBJDIRS))

clean:
	@echo Deleting object tree $(OBJ)...
	$(RM) -r $(OBJ)
	@echo Deleting $(EMULATOR)...
	$(RM) $(EMULATOR)
	@echo Deleting $(TOOLS)...
	$(RM) $(TOOLS)
ifdef MAP
	@echo Deleting $(FULLNAME).map...
	$(RM) $(FULLNAME).map
endif



#-------------------------------------------------
# directory targets
#-------------------------------------------------

$(sort $(OBJDIRS)):
	$(MD) -p $@



#-------------------------------------------------
# executable targets and dependencies
#-------------------------------------------------

$(EMULATOR): $(VERSIONOBJ) $(DRVLIBS) $(LIBOSD) $(LIBEMU) $(LIBCPU) $(LIBSOUND) $(LIBUTIL) $(EXPAT) $(ZLIB) $(LIBOCORE) $(RESFILE)
# always recompile the version string
	$(CC) $(CDEFS) $(CFLAGS) -c $(SRC)/version.c -o $(VERSIONOBJ)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@ $(MAPFLAGS)



#-------------------------------------------------
# generic rules
#-------------------------------------------------

$(OBJ)/osd/$(MAMEOS)/%.o: $(SRC)/osd/$(MAMEOS)/%.c | $(OSPREBUILD)
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGSOSDEPEND) -c $< -o $@

$(OBJ)/%.o: $(SRC)/%.c | $(OSPREBUILD)
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -c $< -o $@

$(OBJ)/%.pp: $(SRC)/%.c | $(OSPREBUILD)
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -E $< -o $@

$(OBJ)/%.s: $(SRC)/%.c | $(OSPREBUILD)
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -S $< -o $@

$(OBJ)/%.lh: $(SRC)/%.lay $(FILE2STR)
	@echo Converting $<...
	@$(FILE2STR) $< $@ layout_$(basename $(notdir $<))

$(OBJ)/%.fh: $(SRC)/%.png $(PNG2BDC)
	@echo Converting $<...
	@$(PNG2BDC) $< $(OBJ)/temp.bdc
	@$(FILE2STR) $(OBJ)/temp.bdc $@ font_$(basename $(notdir $<)) UINT8

$(OBJ)/%.a:
	@echo Archiving $@...
	$(RM) $@
	$(AR) -cr $@ $^
