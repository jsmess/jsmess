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
# Where should we deposit all of the files we make?
OBJ_DIR := $(CURDIR)/build

#-------------------------------------------------------------------------------
# End user configurable variables
# The variables below are not intended to be changed by the user.
#-------------------------------------------------------------------------------
include $(CURDIR)/make/common.mak

ifdef SYSTEM
include $(CURDIR)/make/systems/$(SYSTEM).mak
endif

#-------------------------------------------------------------------------------
# Flags on flags (emscripten / MESS / buildtools / Java flags)
#-------------------------------------------------------------------------------
# We apply them one-at-a-time when we want to comment on them, since we can't
# put comments on multiline variable definitions. :(

# Flags passed to emcc
#EMCC_FLAGS := -s DISABLE_EXCEPTION_CATCHING=0 -s ALLOW_MEMORY_GROWTH=1 -O2
#EMCC_FLAGS := -s DISABLE_EXCEPTION_CATCHING=0 -s ALLOW_MEMORY_GROWTH=1 -O2 --closure 0 -s RELOOP=0
#EMCC_FLAGS := -s LABEL_DEBUG=1 -s HEADLESS=1 -s DISABLE_EXCEPTION_CATCHING=0 -s ALLOW_MEMORY_GROWTH=1 -s RELOOP=1 -O1
#EMCC_FLAGS := -s DISABLE_EXCEPTION_CATCHING=0 -s ALLOW_MEMORY_GROWTH=1 -s RELOOP=1 -O1
#-s LABEL_DEBUG=1
#-O1 --llvm-opts 3 --llvm-lto 1 --closure 1
EMCC_FLAGS := -O1 -s DISABLE_EXCEPTION_CATCHING=0 --llvm-opts 3 -s ALIASING_FUNCTION_POINTERS=1

# Flags shared between the native tools build and emscripten build of MESS.
SHARED_MESS_FLAGS := OSD=sdl       # Set the onscreen display to use SDL.
SHARED_MESS_FLAGS += NOWERROR=1    # Disables -Werror (c|cxx)flag.
SHARED_MESS_FLAGS += NOASM=1       # No assembly allowed!
SHARED_MESS_FLAGS += NO_X11=1      # Disable X11 support for the SDL OSD.
SHARED_MESS_FLAGS += NO_DEBUGGER=1 # Mainly relevant to emscripten. Disables
                                   # the GTK(?) debugger.

# MESS makefile flags used to build the native tools.
NATIVE_MESS_FLAGS := PREFIX=native # Prefix prevents us from accidentally
                                   # overwriting the tools when we build
                                   # emscripten.
# Use the system's native tools, and optimize the build.
NATIVE_MESS_FLAGS += CC=$(NATIVE_CC) CXX=$(NATIVE_CXX) AR=$(NATIVE_AR) \
                     LD=$(NATIVE_LD) OPTIMIZE=3

# Flags passed to the MESS makefile when building with emscripten.
# Build the specific system in MESS.
MESS_FLAGS += TARGET=mess SUBTARGET=$(SYSTEM)
MESS_FLAGS += VERBOSE=1  # Gives us detailed build information to make debugging
                         # build fails easier.
# Uncomment for -g support to get C source line numbers in JS
#MESS_FLAGS += SYMBOLS=1

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
MESS_FLAGS += CROSS_BUILD=1 NATIVE_OBJ="$(NATIVE_OBJ)" TARGETOS=emscripten \
              PTR64=0 OPTIMIZE=0
endif

MESS_FLAGS        := $(SHARED_MESS_FLAGS) $(MESS_FLAGS)
NATIVE_MESS_FLAGS := $(SHARED_MESS_FLAGS) $(NATIVE_MESS_FLAGS)

BIOS_FILES := $(foreach BIOS_FILE,$(BIOS),$(BIOS_DIR)/$(BIOS_FILE))

#-------------------------------------------------------------------------------
# Build Rules
#-------------------------------------------------------------------------------
# PHONY targets are those that are not based on files. Making them 'PHONY'
# means that a file with the same name as the target cannot prevent execution
# of the target.
.PHONY: default clean buildtools

default: $(OBJ_DIR)/index.html

# Runs a webserver so you can test a given system.
test: $(OBJ_DIR)/index.html
	@echo "Visit http://localhost:8000 to test $(SYSTEM). Use CTRL+C to kill the webserver"
	cd $(OBJ_DIR); python -m SimpleHTTPServer 8000

# Compiles buildtools required by MESS.
buildtools:
	@cd mess; make $(NATIVE_MESS_FLAGS) buildtools

clean:
	cd mess; make $(SHARED_FLAGS) $(NATIVE_MESS_FLAGS) clean
	cd mess; $(EMMAKE) make $(SHARED_FLAGS) $(EMSCRIPTEN_MESS_FLAGS) clean

# Creates a final HTML file.
$(OBJ_DIR)/index.html: $(OBJ_DIR) $(TEMPLATE_FILES) $(BIOS_FILES) $(OBJ_DIR)/$(MESS_EXE).js.gz
	@cp -r $(TEMPLATE_DIR)/* $(OBJ_DIR)/
	@$(call SED_I,'s/BIOS_FILES/$(BIOS)/g' $(OBJ_DIR)/messloader.js)
	@$(call SED_I,'s/MESS_SRC/$(MESS_EXE).js.gz/g' $(OBJ_DIR)/messloader.js)
	@$(call SED_I,'s/MESS_ARGS/$(MESS_ARGS)/g' $(OBJ_DIR)/messloader.js)
	@echo "----------------------------------------------------------------------"
	@echo "Compilation complete!"
	@echo "System: $(SYSTEM)"
	@echo "Output File: $(OBJ_DIR)/index.html"
	@echo "----------------------------------------------------------------------"
	@echo "In $(OBJ_DIR)/messloader.js,"
	@echo "change 'gamename' at the top to the filename of a game in that"
	@echo "directory!"
	@echo "----------------------------------------------------------------------"

# Creates the object directory.
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

# Compresses the executable.
$(OBJ_DIR)/$(MESS_EXE).js.gz: $(OBJ_DIR)/$(MESS_EXE).js
	gzip -f -c $(OBJ_DIR)/$(MESS_EXE).js > $(OBJ_DIR)/$(MESS_EXE).js.gz

# Runs emcc on LLVM bitcode version of MESS.
$(OBJ_DIR)/$(MESS_EXE).js: mess/$(MESS_EXE).bc post.js
	$(EMCC) $(EMCC_FLAGS) $< -o $(OBJ_DIR)/$(MESS_EXE).js --post-js post.js

# Copies over the LLVM bitcode for MESS into a .bc file.
mess/$(MESS_EXE).bc: mess/$(MESS_EXE)
	@cp $< mess/$(MESS_EXE).bc

# Compiles MESS to LLVM bitcode.
mess/$(MESS_EXE): buildtools
	@cd mess; $(EMMAKE) make $(MESS_FLAGS)

# Ensures that required files actually exist and, if so, copies it over to the
# build directory.
# Make it a double colon rule so it executes more than once (once for each
# embedded file)
$(BIOS_FILES)::
	@if [ ! -f $@ ]; then echo "File $@ does not exist!"; exit 1; fi
	@cp $@ $(OBJ_DIR)
