###########################################################################
#
#   makefile
#
#   Core makefile for building JSMESS
#
###########################################################################

#-------------------------------------------------------------------------------
# Variables
#-------------------------------------------------------------------------------

# Absolute directory path to emscripten / closure compiler.
EMSCRIPTEN_DIR = $(CURDIR)/third_party/emscripten
CLOSURE_DIR = $(EMSCRIPTEN_DIR)/third_party/closure-compiler

# Path to specific tools invoked during the build process.
EMMAKE = $(EMSCRIPTEN_DIR)/emmake
CLOSURE = $(CLOSURE_DIR)/compiler.jar

# Used to build native tools. CC/CXX must be clang due to the additional flags
# we supply to the compiler for warnings and such.
NATIVE_CC = clang
NATIVE_CXX = clang++
NATIVE_LD = clang++
NATIVE_AR = ar
# The directory where the native object files will be built.
NATIVE_OBJ := $(CURDIR)/mess/obj/sdl/nativemame

# Are we on a 64 bit platform? If so, mame will append '64' to the native_obj
# directory.
# Logic copied from MESS Makefile.
UNAME = $(shell uname -a)
ifeq ($(firstword $(filter x86_64,$(UNAME))),x86_64)
NATIVE_OBJ := $(NATIVE_OBJ)64
endif
ifeq ($(firstword $(filter amd64,$(UNAME))),amd64)
NATIVE_OBJ := $(NATIVE_OBJ)64
endif
ifeq ($(firstword $(filter ppc64,$(UNAME))),ppc64)
NATIVE_OBJ := $(NATIVE_OBJ)64
endif

# Flags shared between the native tools build and emscripten build.
# TODO: Document why each of these are set / what they do.
SHARED_FLAGS = OSD=sdl       \
               NOWERROR=1    \
               NOASM=1       \
               NO_X11=1      \
               NO_DEBUGGER=1 \

# Flags used to build the native tools.
# TODO: Document why each of these are set / what they do.
NATIVE_MESS_FLAGS = PREFIX=native     \
                    CC=$(NATIVE_CC)   \
                    CXX=$(NATIVE_CXX) \
                    AR=$(NATIVE_AR)   \
                    LD=$(NATIVE_LD)   \
                    OPTIMIZE=3

# Flags used to make the emscripten build
# TODO: Document why each of these are set / what they do.
EMSCRIPTEN_MESS_FLAGS = TARGET=mess                \
                        SUBTARGET=tiny             \
                        CROSS_BUILD=1              \
                        NATIVE_OBJ="$(NATIVE_OBJ)" \
                        TARGETOS=emscripten        \
                        PTR64=0                    \
                        SYMLEVEL=2                 \
                        VERBOSE=1                  \
                        OPTIMIZE=0

#-------------------------------------------------------------------------------
# Build Rules
#-------------------------------------------------------------------------------

default:
	@if [ ! -f $(GAME) ]; then echo "Missing game file: $(GAME)"; exit 1; fi
	@if [ ! -f $(BIOS) ]; then echo "Missing BIOS: $(BIOS)"; exit 1; fi
	cd mess; make $(SHARED_FLAGS) $(NATIVE_MESS_FLAGS) buildtools
	cd mess; $(EMMAKE) make $(SHARED_FLAGS) $(EMSCRIPTEN_MESS_FLAGS)
	mv mess/messtiny mess/messtiny.bc
	$(EMSCRIPTEN_DIR)/emcc -s DISABLE_EXCEPTION_CATCHING=0 mess/messtiny.bc \
		-o messtiny.js --post-js post.js --embed-file $(BIOS) \
		--embed-file $(GAME)
	java -Xmx1536m -jar $(CLOSURE) --js messtiny.js \
		--js_output_file mess_closure.js

clean:
	cd mess; make $(SHARED_FLAGS) $(NATIVE_MESS_FLAGS) clean
	cd mess; $(EMMAKE) make $(SHARED_FLAGS) $(EMSCRIPTEN_MESS_FLAGS) clean
	rm mess/messtiny.bc messtiny.js mess_closure.js
