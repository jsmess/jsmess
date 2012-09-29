################################################################################
#
#   makefile
#
#   Core makefile for building JSMESS.
#
#   The goal of this makefile is to change MESS's makefiles as little as
#   possible by moving any relevant makefile variable changes into this
#   makefile.
#
################################################################################

#-------------------------------------------------------------------------------
# User configurable variables
#-------------------------------------------------------------------------------
# You can change these variables with `make VARIABLE=VALUE`. :)
# Caveats (for now): No spaces in filenames / directory paths.

# Where should we look for video games?
GAME_DIR := $(CURDIR)/games
# Where should we look for system BIOS?
BIOS_DIR := $(CURDIR)/bios
# What game should be embed into the webpage? This should be specified without
# the GAME_DIR prefixed to it.
GAME := cosmofighter2.zip

#-------------------------------------------------------------------------------
# End user configurable variables
# The variables below are not intended to be changed by the user.
#-------------------------------------------------------------------------------

# We build this system if you do not explicitly specify one when invoking make.
DEFAULT_SYSTEM := colecovision

# Contains the name of the target representing the system we are building.
ifndef $(MAKECMDGOALS)
SYSTEM := $(DEFAULT_SYSTEM)
else
SYSTEM := $(MAKECMDGOALS)
endif

# Contains variables relevant to the target system.
include $(CURDIR)/make/$(SYSTEM).mak

# The name of the bitcode executable produced by making mess. It's always
# mess$(SUBTARGET).
MESS_EXE := mess$(SUBTARGET)

# List out the absolute paths to each file we plan to embed.
# TODO: This doesn't work if spaces are in the filenames.
# TODO: This isn't flexible if the BIOS changes.
EMBED_FILES :=  $(foreach AGAME,$(GAME),$(GAME_DIR)/$(AGAME))
ifdef $(BIOS)
EMBED_FILES := $(EMBED_FILES) \
               $(foreach $(BIOS),$(COLECO_BIOS),$(BIOS_DIR)/$(BIOS))
endif
# Convert EMBED_FILES into the flags needed by emscripten.
# TODO: This doesn't work if spaces are in the filepaths.
EMBED_FILES_FLAG := $(foreach FILE, $(EMBED_FILES),--embed-file $(FILE))

# Absolute directory path to emscripten / closure compiler.
EMSCRIPTEN_DIR := $(CURDIR)/third_party/emscripten
CLOSURE_DIR := $(EMSCRIPTEN_DIR)/third_party/closure-compiler

# Path to specific tools invoked during the build process.
EMMAKE := $(EMSCRIPTEN_DIR)/emmake
EMCC := $(EMSCRIPTEN_DIR)/emcc
CLOSURE := $(CLOSURE_DIR)/compiler.jar

# Used to build native tools. CC/CXX must be clang due to the additional flags
# we supply to the compiler for warnings and such.
NATIVE_CC := clang
NATIVE_CXX := clang++
NATIVE_LD := clang++
NATIVE_AR := ar
# The directory where the native object files will be built.
NATIVE_OBJ := $(CURDIR)/mess/obj/sdl/nativemame

# Are we on a 64 bit platform? If so, mame will append '64' to the native_obj
# directory.
# Logic copied from MESS Makefile.
UNAME := $(shell uname -a)
ifeq ($(firstword $(filter x86_64,$(UNAME))),x86_64)
NATIVE_OBJ := $(NATIVE_OBJ)64
endif
ifeq ($(firstword $(filter amd64,$(UNAME))),amd64)
NATIVE_OBJ := $(NATIVE_OBJ)64
endif
ifeq ($(firstword $(filter ppc64,$(UNAME))),ppc64)
NATIVE_OBJ := $(NATIVE_OBJ)64
endif

#-------------------------------------------------------------------------------
# Flags on flags (emscripten / MESS / buildtools / Java flags)
#-------------------------------------------------------------------------------

# Flags passed to emcc
EMCC_FLAGS := DISABLE_EXCEPTION_CATCHING=0

# Flags shared between the native tools build and emscripten build of MESS.
SHARED_MESS_FLAGS =  OSD=sdl       \ # Set the onscreen display to use SDL.
                     NOWERROR=1    \ # Disables -Werror flag.
                     NOASM=1       \ # No assembly allowed!
                     NO_X11=1      \ # Disable X11 support for the SDL OSD.
                     NO_DEBUGGER=1 \ # Mainly relevant to emscripten. Disables
                                     # the GTK(?) debugger.

# MESS makefile flags used to build the native tools.
NATIVE_MESS_FLAGS = PREFIX=native       \ # Prefix prevents us from accidentally
                                          # overwriting the tools when we build
                                          # emscripten.
                      CC=$(NATIVE_CC)   \ # Use the system's native tools.
                      CXX=$(NATIVE_CXX) \
                      AR=$(NATIVE_AR)   \
                      LD=$(NATIVE_LD)   \
                      OPTIMIZE=3          # Optimization is OK here.

# Flags passed to the MESS makefile when building with emscripten.
MESS_FLAGS =   TARGET=mess                \
               SUBTARGET=$(SUBTARGET)     \
               CROSS_BUILD=1              \ # Disable tool building.
               NATIVE_OBJ="$(NATIVE_OBJ)" \ # Use natively built tools
               TARGETOS=emscripten        \
               PTR64=0                    \ # Emscripten is 32-bit.
               SYMLEVEL=2                 \
               VERBOSE=1                  \
               OPTIMIZE=0                   # Emscripten ignores all
                                            # optimization flags on individual
                                            # modules.

MESS_MAKE_FLAGS := $(SHARED_MESS_FLAGS) $(MESS_MAKE_FLAGS)
NATIVE_MESS_FLAGS := $(SHARED_MESS_FLAGS) $(NATIVE_MESS_FLAGS)

# Used when invoking closure.
JVM_FLAGS := -Xmx1536m

#-------------------------------------------------------------------------------
# Build Rules
#-------------------------------------------------------------------------------

default: $(DEFAULT_SYSTEM)

clean:
	cd mess; make $(SHARED_FLAGS) $(NATIVE_MESS_FLAGS) clean
	cd mess; $(EMMAKE) make $(SHARED_FLAGS) $(EMSCRIPTEN_MESS_FLAGS) clean
	rm mess/messtiny.bc messtiny.js mess_closure.js

colecovision: $(MESS_EXE).js

# Runs closure on unoptimized JS.
$(MESS_EXE).js: $(MESS_EXE)_unopt.js
	java $(JVM_FLAGS) -jar $(CLOSURE) --js $(MESS_EXE)_unopt.js \
		--js_output_file $(MESS_EXE).js

# Runs emcc on LLVM bitcode version of MESS.
$(MESS_EXE)_unopt.js: mess/$(MESS_EXE).bc $(EMBED_FILES)
	$(EMCC) -s $(EMCC_FLAGS) $< -o $(MESS_EXE)_unopt.js --post-js post.js \
		$(EMBED_FILES_FLAG)

# Copies over the LLVM bitcode for MESS into a .bc file.
mess/$(MESS_EXE).bc: mess/$(MESS_EXE)
	cp $< $<.bc

# Compiles MESS to LLVM bitcode.
mess/$(MESS_EXE): buildtools
	cd mess; $(EMMAKE) make $(MESS_MAKE_FLAGS)

# Compiles buildtools required by MESS.
buildtools:
	cd mess; make $(NATIVE_MESS_FLAGS) buildtools