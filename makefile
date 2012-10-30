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

# Where should we look for video games?
GAME_DIR := $(CURDIR)/games
# Where should we look for system BIOS?
BIOS_DIR := $(CURDIR)/bios
# Where should we deposit all of the files we make?
OBJ_DIR := $(CURDIR)/build
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
ifndef MAKECMDGOALS
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
ifdef BIOS
EMBED_FILES += $(foreach BIOS_FILE,$(BIOS),$(BIOS_DIR)/$(BIOS_FILE))
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

# Strips the extention off of game.
GAME_NAME   := $(patsubst %.zip,%,$(GAME))
HTML_OUTPUT := index.html

# Final directory for built files.
OBJ_DIR     := $(OBJ_DIR)/$(SYSTEM)/$(GAME_NAME)
# The HTML template we'll be using.
TEMPLATE_DIR := $(CURDIR)/templates/$(SYSTEM)
# All of the files in the template directory. Allows for 'smart' HTML rebuilding
TEMPLATE_FILES := $(shell ls $(TEMPLATE_DIR))
TEMPLATE_FILES := $(foreach TFILE,$(TEMPLATE_FILES),$(TEMPLATE_DIR)/$(TFILE))

# Function that generates a command to run sed with inplace replacement.
# Unfortunately, it GNU and BSD sed are slightly different. BSD sed returns an
# error code when you run --help, which we abuse in this statement.
# Left command is GNU, right command is BSD.
# As this is a function, do not use := for immediate resolution!
SED_I = sed --help >/dev/null 2>&1 && sed -i $(1) || sed -i '' $(1)

#-------------------------------------------------------------------------------
# Flags on flags (emscripten / MESS / buildtools / Java flags)
#-------------------------------------------------------------------------------
# We apply them one-at-a-time when we want to comment on them, since we can't
# put comments on multiline variable definitions. :(

# Flags passed to emcc
EMCC_FLAGS := DISABLE_EXCEPTION_CATCHING=0

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
MESS_FLAGS += TARGET=mess SUBTARGET=$(SUBTARGET)
MESS_FLAGS += SYMLEVEL=2 # TODO: Unsure why we set this.
MESS_FLAGS += VERBOSE=1  # Gives us detailed build information to make debugging
                         # build fails easier.

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

# Used when invoking closure.
JVM_FLAGS := -Xmx1536m

#-------------------------------------------------------------------------------
# Build Rules
#-------------------------------------------------------------------------------

default: $(DEFAULT_SYSTEM)

clean:
	cd mess; make $(SHARED_FLAGS) $(NATIVE_MESS_FLAGS) clean
	cd mess; $(EMMAKE) make $(SHARED_FLAGS) $(EMSCRIPTEN_MESS_FLAGS) clean
#	rm -r $(OBJ_DIR)

# TODO: Make this less dumb.
colecovision: $(OBJ_DIR)/$(HTML_OUTPUT)
atari2600: $(OBJ_DIR)/$(HTML_OUTPUT)
channelf: $(OBJ_DIR)/$(HTML_OUTPUT)
pet2001: $(OBJ_DIR)/$(HTML_OUTPUT)
ti99_4: $(OBJ_DIR)/$(HTML_OUTPUT)
ibm5150: $(OBJ_DIR)/$(HTML_OUTPUT)
c64: $(OBJ_DIR)/$(HTML_OUTPUT)

# Creates a final HTML file.
# Portability notes: sed on the Mac is BSD sed. -i requires an option, so we
# give it an empty string. GNU sed's -i does not require an option.
$(OBJ_DIR)/$(HTML_OUTPUT): $(OBJ_DIR) $(TEMPLATE_FILES) $(OBJ_DIR)/$(MESS_EXE).js
	@cp -r $(TEMPLATE_DIR)/* $(OBJ_DIR)/
	@$(call SED_I,'s/MESS_SRC/$(MESS_EXE).js/g' $(OBJ_DIR)/messloader.js)
	@$(call SED_I,'s/GAME_NAME/$(GAME_NAME)/g' $(OBJ_DIR)/messloader.js)
	@$(call SED_I,'s/GAME_FILE/$(GAME)/g' $(OBJ_DIR)/messloader.js)
	@echo "----------------------------------------------------------------------"
	@echo "Compilation complete!"
	@echo "System: $(SYSTEM)"
	@echo "Game: $(GAME_NAME)"
	@echo "Output File: $(OBJ_DIR)/$(HTML_OUTPUT)"
	@echo "----------------------------------------------------------------------"

# Creates the object directory.
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

# Runs closure on unoptimized JS.
$(OBJ_DIR)/$(MESS_EXE).js: $(OBJ_DIR)/$(MESS_EXE)_unopt.js
	java $(JVM_FLAGS) -jar $(CLOSURE) --js $(OBJ_DIR)/$(MESS_EXE)_unopt.js \
		--js_output_file $(OBJ_DIR)/$(MESS_EXE).js

# Runs emcc on LLVM bitcode version of MESS.
$(OBJ_DIR)/$(MESS_EXE)_unopt.js: $(OBJ_DIR)/$(MESS_EXE).bc post.js $(EMBED_FILES)
	$(EMCC) -s $(EMCC_FLAGS) $< -o $(OBJ_DIR)/$(MESS_EXE)_unopt.js \
	--post-js post.js $(EMBED_FILES_FLAG)

# Copies over the LLVM bitcode for MESS into a .bc file.
$(OBJ_DIR)/$(MESS_EXE).bc: mess/$(MESS_EXE)
	@cp $< $(OBJ_DIR)/$(MESS_EXE).bc

# Compiles MESS to LLVM bitcode.
mess/$(MESS_EXE): buildtools
	@cd mess; $(EMMAKE) make $(MESS_FLAGS)

# Compiles buildtools required by MESS.
buildtools:
	@cd mess; make $(NATIVE_MESS_FLAGS) buildtools

# Ensures that embedded files actually exist, or else emscripten will just issue
# a warning.
# Make it a double colon rule so it executes more than once (once for each
# embedded file)
$(EMBED_FILES)::
	@if [ ! -f $@ ]; then echo "File $@ does not exist!"; exit 1; fi
