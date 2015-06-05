################################################################################
#
#   makefile
#
#   Core makefile for building JSMESS.
#
#   Makefile goals:
#   * Try to impact MESS's actual makefiles as little as possible. If it is
#     possible to change something here through a makefile variable, do it.
#
#   * Make it self documenting. If you add a flag to the MESS_FLAGS, put a
#     comment describing why you added it. This allows for later refactoring
#     when the MAME/MESS/Emscripten/etc source changes.
#
#   * Make it modular. Isolate system-specific flags to their own subfiles.
#
################################################################################
#-------------------------------------------------------------------------------
# User configurable variables
#-------------------------------------------------------------------------------
# You can change these variables with `make VARIABLE=VALUE`. :)
# Caveats (for now): No spaces in filenames / directory paths.

# Where should we look for system BIOS?
BIOS_DIR := bios
# Where should we look for games?
GAME_DIR := games
# Where should we deposit all of the files we make?
OBJ_DIR := $(CURDIR)/build

#-------------------------------------------------------------------------------
# End user configurable variables
# The variables below are not intended to be changed by the user.
#-------------------------------------------------------------------------------
EMCC_FLAGS :=

ifndef DEBUG_NAME
DEBUG_NAME :=
endif

ifdef SYSTEM
include $(CURDIR)/systems/$(SYSTEM).mak
endif

#-------------------------------------------------------------------------------
#   This is where we hide all of our dirty Makefile secrets. Namely, all of the
#   gross details like 64-bit checking.
#-------------------------------------------------------------------------------
# Are we on a 64 bit platform? If so, mame will append '64' to the native_obj
# directory.
# Logic copied from MESS Makefile.

UNAME := $(shell uname -a)
ifeq ($(firstword $(filter x86_64,$(UNAME))),x86_64)
IS_64_BIT := 1
endif
ifeq ($(firstword $(filter amd64,$(UNAME))),amd64)
IS_64_BIT := 1
endif
ifeq ($(firstword $(filter ppc64,$(UNAME))),ppc64)
IS_64_BIT := 1
endif

MAME_DIR := $(CURDIR)/third_party/mame
EMSCRIPTEN_DIR := $(CURDIR)/third_party/emscripten
EMMAKE := $(EMSCRIPTEN_DIR)/emmake
EMCC := $(EMSCRIPTEN_DIR)/emcc

# Used to build native tools. CC/CXX must be clang due to the additional flags
# we supply to the compiler for warnings and such.

NATIVE_CC := clang
NATIVE_CXX := clang++
NATIVE_LD := clang++
NATIVE_AR := ar

# Final directory for built files.

OBJ_DIR     := $(OBJ_DIR)/$(SUBTARGET)
JS_OBJ_DIR     := $(OBJ_DIR)/$(SYSTEM)

ifndef TEMPLATE
TEMPLATE := default
endif

# The HTML template we'll be using.

TEMPLATE_DIR := $(CURDIR)/templates/$(TEMPLATE)

# All of the files in the template directory. Allows for 'smart' HTML rebuilding

TEMPLATE_FILES := $(shell ls $(TEMPLATE_DIR))
TEMPLATE_FILES := $(foreach TFILE,$(TEMPLATE_FILES),$(TEMPLATE_DIR)/$(TFILE))

# The name of the bitcode executable produced by making mess.

MESS_EXE := mess$(SUBTARGET)

ifeq ($(IS_64_BIT),1)
NATIVE_OBJ := $(MAME_DIR)/obj/nativesdl64
else
NATIVE_OBJ := $(MAME_DIR)/obj/nativesdl
endif

#-------------------------------------------------------------------------------
# Flags on flags (emscripten / MESS / buildtools / Java flags)
#-------------------------------------------------------------------------------
# We apply them one-at-a-time when we want to comment on them, since we can't
# put comments on multiline variable definitions. :(

# EMCC Flags (Emscripten)
# The second line consists of "voodoo settings". Change or remove if needed or testing.

EMCC_FLAGS += -O3 -s DISABLE_EXCEPTION_CATCHING=2 -s USE_SDL=2 --memory-init-file 0
EMCC_FLAGS += -s NO_EXIT_RUNTIME=1 -s ASSERTIONS=0 -s COMPILER_ASSERTIONS=1

# Choose ONE of the following memory settings. (The least, the better.)
# If you run the system and it crashes complaining about memory, go to the
# next amount. (Eventually, this will be automatically chosen.)
#
# Hint: Most items appear to need at least 64mb of memory.

# EMCC_FLAGS += -s TOTAL_MEMORY=16777216      # 16mb
# EMCC_FLAGS += -s TOTAL_MEMORY=33554432      # 32mb
# EMCC_FLAGS += -s TOTAL_MEMORY=67108864      # 64mb
# EMCC_FLAGS += -s TOTAL_MEMORY=134217728     # 128mb
EMCC_FLAGS += -s TOTAL_MEMORY=268435456     # 256mb

# Additional controls and functions from the code, allowing direct JS manipulations.
# If radical changes happen to MESS/MAME code, these may not work and be dormant.

EMCC_FLAGS += -s EXCEPTION_CATCHING_WHITELIST='["__ZN15running_machine17start_all_devicesEv"]'
EMCC_FLAGS += -s EXPORTED_FUNCTIONS="['_main', '_malloc', \
'__Z14js_get_machinev', '__Z9js_get_uiv', '__Z12js_get_soundv', \
'__ZN10ui_manager12set_show_fpsEb', '__ZNK10ui_manager8show_fpsEv', \
'__ZN13sound_manager4muteEbh', '_SDL_PauseAudio']"

# Flags shared between the native tools build and emscripten build of MESS.

SHARED_MESS_FLAGS := OSD=sdl       # Set the OS-dependent layer to use SDL.
SHARED_MESS_FLAGS += NOWERROR=1    # Disables -Werror (c|cxx)flag.

# MESS makefile flags used to build the native tools.
NATIVE_MESS_FLAGS := PREFIX=native # Prefix prevents us from accidentally
                                   # overwriting the tools when we build
                                   # emscripten.

# Use the system's native tools, and optimize the build.

NATIVE_MESS_FLAGS += CC=$(NATIVE_CC) CXX=$(NATIVE_CXX) AR=$(NATIVE_AR) \
                     LD=$(NATIVE_LD) OPTIMIZE=3

# Flags passed to the MESS makefile when building with emscripten.
# Build the specific system in MESS.

MESS_FLAGS += TARGET=mess SUBTARGET=$(SUBTARGET) SYSTEM=$(SYSTEM)
MESS_FLAGS += VERBOSE=1  # Gives us detailed build information to make debugging
                         # build fails easier.
MESS_FLAGS += SDL_INSTALL_ROOT=$(CURDIR)/third_party/emscripten/system

# Uncomment for debug/profiling support
#MESS_FLAGS += SYMBOLS=1 SYMLEVEL=2
#EMCC_FLAGS += -g2
#DEBUG_NAME := -debug

# Uncomment for advanced debug support with source line numbers
#MESS_FLAGS += SYMBOLS=1 SYMLEVEL=2
#EMCC_FLAGS += --js-opts 0 -g4
#DEBUG_NAME := -debug

# The NATIVE_DEBUG flag allows us to build what emscripten is building natively.
# This is invaluable when testing new build targets.
# Thus, this flag guards adding the flags to MESS_FLAGS that enable special
# emscripten things that may not work in a native build.

ifdef NATIVE_DEBUG
EMMAKE :=
EMCC := echo
MESS_FLAGS += CC=$(NATIVE_CC) CXX=$(NATIVE_CXX) AR=$(NATIVE_AR) \
              LD=$(NATIVE_LD) OPTIMIZE=3
else

# Do not build the buildtools; use the ones we build natively.
# We identify ourselves as the "emscripten" OS, allowing us to modify the MESS
# Makefiles in a principled way.
# Emscripten targets a 32-bit machine, since 64-bit arithmetic is...
# troublesome in JavaScript.
# Emscripten ignores all optimization flags while compiling C/C++ code.
# OPTIMIZE=3 should match -O3 in EMCC_FLAGS

MESS_FLAGS += CROSS_BUILD=1 NATIVE_OBJ="$(NATIVE_OBJ)" TARGETOS=emscripten \
              PTR64=0 OPTIMIZE=3
endif

MESS_FLAGS        := $(SHARED_MESS_FLAGS) $(MESS_FLAGS)
NATIVE_MESS_FLAGS := $(SHARED_MESS_FLAGS) $(NATIVE_MESS_FLAGS)

BIOS_FILES := $(foreach BIOS_FILE,$(BIOS),$(BIOS_DIR)/$(BIOS_FILE))

JSMESS_VERSION := $(shell git rev-parse --abbrev-ref HEAD) (commit $(shell git rev-parse HEAD))
JSMESS_MESS_BUILD_VERSION := $(shell tail --lines=1 third_party/mame/src/version.c | cut -d '"' -f 2)commit $(shell cat .git/modules/third_party/mame/HEAD))
JSMESS_EMCC_VERSION := $(shell third_party/emscripten/emcc --version | grep commit)

# Set the GAME variable for a ROM image in the "games" directory to have a playable game.

ifdef GAME
GAME_FILE := $(GAME_DIR)/$(GAME)
else
GAME_FILE :=
endif

#-------------------------------------------------------------------------------
# Build Rules
#-------------------------------------------------------------------------------
# PHONY targets are those that are not based on files. Making them 'PHONY'
# means that a file with the same name as the target cannot prevent execution
# of the target.
.PHONY: default clean buildtools

default: $(JS_OBJ_DIR)/index.html

# Runs a webserver so you can test a given system.
test: $(JS_OBJ_DIR)/index.html
	@echo "Visit http://localhost:8000 to test $(SYSTEM). Use CTRL+C to kill the webserver"
	cd $(JS_OBJ_DIR); python -m SimpleHTTPServer 8000

# Compiles buildtools required by MESS.
buildtools:
	@cd $(MAME_DIR); make $(NATIVE_MESS_FLAGS) buildtools

clean:
	cd $(MAME_DIR); make $(SHARED_FLAGS) $(NATIVE_MESS_FLAGS) clean
	cd $(MAME_DIR); $(EMMAKE) make $(SHARED_FLAGS) $(EMSCRIPTEN_MESS_FLAGS) clean

# Creates a final HTML file.
$(JS_OBJ_DIR)/index.html: $(JS_OBJ_DIR) $(TEMPLATE_FILES) $(BIOS_FILES) $(GAME_FILE) $(OBJ_DIR)/$(MESS_EXE)$(DEBUG_NAME).js.gz
	-@cp $(GAME_FILE) $(JS_OBJ_DIR)/
	@cp $(OBJ_DIR)/$(MESS_EXE)$(DEBUG_NAME).js.gz $(JS_OBJ_DIR)/
	@cp $(OBJ_DIR)/$(MESS_EXE)$(DEBUG_NAME).js $(JS_OBJ_DIR)/
	-@cp $(OBJ_DIR)/$(MESS_EXE)$(DEBUG_NAME).js.mem $(JS_OBJ_DIR)/
	-@cp $(OBJ_DIR)/$(MESS_EXE)$(DEBUG_NAME).js.map $(JS_OBJ_DIR)/
	@cp -r $(TEMPLATE_DIR)/* $(JS_OBJ_DIR)/
	@rm $(JS_OBJ_DIR)/pre.js
	@rm $(JS_OBJ_DIR)/post.js
	@sed -e 's/BIOS_FILES/$(BIOS)/g' \
	     -e 's/GAME_FILE/$(GAME)/g' \
	     -e 's/MESS_SRC/$(MESS_EXE)$(DEBUG_NAME).js/g' \
	     -e 's/MESS_ARGS/$(MESS_ARGS)/g' \
		 $(TEMPLATE_DIR)/messloader.js > $(JS_OBJ_DIR)/messloader.js
	@echo "----------------------------------------------------------------------"
	@echo "Compilation complete!"
	@echo "System: $(SYSTEM)"
	@echo "Parent: $(SUBTARGET)"
	@echo "Output File: $(JS_OBJ_DIR)/index.html"
	@echo "----------------------------------------------------------------------"
	@echo "If you didn't specify a GAME= file on the command line,"
	@echo "in $(JS_OBJ_DIR)/messloader.js,"
	@echo "change 'gamename' at the top to the filename of a game in that"
	@echo "directory!"
	@echo "----------------------------------------------------------------------"

# Creates the object directory.
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(JS_OBJ_DIR):
	@mkdir -p $(JS_OBJ_DIR)

# Compresses the executable.
$(OBJ_DIR)/$(MESS_EXE)$(DEBUG_NAME).js.gz: $(OBJ_DIR)/$(MESS_EXE)$(DEBUG_NAME).js
	@gzip -f -c $< > $(OBJ_DIR)/$(MESS_EXE)$(DEBUG_NAME).js.gz

# Runs emcc on LLVM bitcode version of MESS.
$(OBJ_DIR)/$(MESS_EXE)$(DEBUG_NAME).js: $(MAME_DIR)/$(MESS_EXE)$(DEBUG_NAME).bc $(TEMPLATE_DIR)/pre.js $(TEMPLATE_DIR)/post.js
	@sed -e 's/JSMESS_JSMESS_VERSION/$(subst /,\/,$(JSMESS_VERSION))/' \
	     -e 's/JSMESS_MESS_BUILD_VERSION/$(subst /,\/,$(JSMESS_MESS_BUILD_VERSION))/' \
	     -e 's/JSMESS_EMCC_VERSION/$(subst /,\/,$(JSMESS_EMCC_VERSION))/' \
	     -e 's/JSMESS_EMCC_FLAGS/$(subst ",\\",$(EMCC_FLAGS))/' \
	     -e 's/JSMESS_MESS_FLAGS/$(subst ",\\",$(subst /,\/,$(MESS_FLAGS)))/' \
	     $(TEMPLATE_DIR)/pre.js > $(OBJ_DIR)/pre.js
	$(EMCC) $(EMCC_FLAGS) $< -o $(OBJ_DIR)/$(MESS_EXE)$(DEBUG_NAME).js --pre-js $(OBJ_DIR)/pre.js --post-js $(TEMPLATE_DIR)/post.js
	@rm $(OBJ_DIR)/pre.js

# Copies over the LLVM bitcode for MESS into a .bc file.
$(MAME_DIR)/$(MESS_EXE)$(DEBUG_NAME).bc: $(MAME_DIR)/$(MESS_EXE)
	@cp $< $(MAME_DIR)/$(MESS_EXE)$(DEBUG_NAME).bc

# Compiles MESS to LLVM bitcode.
$(MAME_DIR)/$(MESS_EXE): buildtools
	@cd $(MAME_DIR); $(EMMAKE) make $(MESS_FLAGS)

# Ensures that required files actually exist and, if so, copies it over to the
# build directory.
# Make it a double colon rule so it executes more than once (once for each
# embedded file)
$(BIOS_FILES)::
	@if [ ! -f $@ ]; then echo "File $@ does not exist!"; exit 1; fi
	@cp $@ $(JS_OBJ_DIR)

$(GAME_FILE):
	@if [ ! -f $@ ]; then echo "File $@ does not exist!"; exit 1; fi
	@cp $@ $(JS_OBJ_DIR)/
