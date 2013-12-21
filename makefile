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

# Function that generates a command to run sed with inplace replacement.
# Unfortunately, GNU and BSD sed are slightly different. BSD sed returns an
# error code when you run --help, which we abuse in this statement.
# Left command is GNU, right command is BSD.
# As this is a function, do not use := for immediate resolution!
SED_I = sed --help >/dev/null 2>&1 && sed -i $(1) || sed -i '' $(1)


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

# Flags passed to emcc
EMCC_FLAGS += -O2 -s DISABLE_EXCEPTION_CATCHING=0 -s ALIASING_FUNCTION_POINTERS=1 -s OUTLINING_LIMIT=20000 -s TOTAL_MEMORY=33554432
EMCC_FLAGS += -s EXPORTED_FUNCTIONS="['_main', '_malloc', \
'__Z15ui_set_show_fpsi', '__Z15ui_get_show_fpsv']"

# Flags shared between the native tools build and emscripten build of MESS.
SHARED_MESS_FLAGS := OSD=sdl       # Set the onscreen display to use SDL.
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
# OPTIMIZE=2 should match -O2 in EMCC_FLAGS
MESS_FLAGS += CROSS_BUILD=1 NATIVE_OBJ="$(NATIVE_OBJ)" TARGETOS=emscripten \
              PTR64=0 OPTIMIZE=2
endif

MESS_FLAGS        := $(SHARED_MESS_FLAGS) $(MESS_FLAGS)
NATIVE_MESS_FLAGS := $(SHARED_MESS_FLAGS) $(NATIVE_MESS_FLAGS)

BIOS_FILES := $(foreach BIOS_FILE,$(BIOS),$(BIOS_DIR)/$(BIOS_FILE))

JSMESS_MESS_BUILD_VERSION := $(shell tail --lines=1 third_party/mame/src/version.c | cut -d '"' -f 2)commit $(shell cat .git/modules/third_party/mame/HEAD))
JSMESS_EMCC_VERSION := $(shell third_party/emscripten/emcc --version | grep commit)


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
$(JS_OBJ_DIR)/index.html: $(JS_OBJ_DIR) $(TEMPLATE_FILES) $(BIOS_FILES) $(OBJ_DIR)/$(MESS_EXE)$(DEBUG_NAME).js.gz
	@cp $(OBJ_DIR)/$(MESS_EXE)$(DEBUG_NAME).js.gz $(JS_OBJ_DIR)/
	@cp $(OBJ_DIR)/$(MESS_EXE)$(DEBUG_NAME).js $(JS_OBJ_DIR)/
	-@cp $(OBJ_DIR)/$(MESS_EXE)$(DEBUG_NAME).js.map $(JS_OBJ_DIR)/
	@cp -r $(TEMPLATE_DIR)/* $(JS_OBJ_DIR)/
	@rm $(JS_OBJ_DIR)/pre.js
	@rm $(JS_OBJ_DIR)/post.js
	@$(call SED_I,'s/BIOS_FILES/$(BIOS)/g' $(JS_OBJ_DIR)/messloader.js)
	@$(call SED_I,'s/MESS_SRC/$(MESS_EXE)$(DEBUG_NAME).js/g' $(JS_OBJ_DIR)/messloader.js)
	@$(call SED_I,'s/MESS_ARGS/$(MESS_ARGS)/g' $(JS_OBJ_DIR)/messloader.js)
	@echo "----------------------------------------------------------------------"
	@echo "Compilation complete!"
	@echo "System: $(SYSTEM)"
	@echo "Parent: $(SUBTARGET)"
	@echo "Output File: $(JS_OBJ_DIR)/index.html"
	@echo "----------------------------------------------------------------------"
	@echo "In $(JS_OBJ_DIR)/messloader.js,"
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
	$(EMCC) $(EMCC_FLAGS) $< -o $(OBJ_DIR)/$(MESS_EXE)$(DEBUG_NAME).js --pre-js $(TEMPLATE_DIR)/pre.js --post-js $(TEMPLATE_DIR)/post.js
	@$(call SED_I,'s/JSMESS_MESS_BUILD_VERSION/$(subst /,\/, $(JSMESS_MESS_BUILD_VERSION))/' $(OBJ_DIR)/$(MESS_EXE)$(DEBUG_NAME).js)
	@$(call SED_I,'s/JSMESS_EMCC_VERSION/$(JSMESS_EMCC_VERSION)/' $(OBJ_DIR)/$(MESS_EXE)$(DEBUG_NAME).js)
	@$(call SED_I,'s/JSMESS_EMCC_FLAGS/$(subst ",\\", $(EMCC_FLAGS))/' $(OBJ_DIR)/$(MESS_EXE)$(DEBUG_NAME).js)
	@$(call SED_I,'s/JSMESS_MESS_FLAGS/$(subst ",\\", $(subst /,\/, $(MESS_FLAGS)))/' $(OBJ_DIR)/$(MESS_EXE)$(DEBUG_NAME).js)

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
