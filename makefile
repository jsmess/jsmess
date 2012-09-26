###########################################################################
#
#   makefile
#
#   Core makefile for building JSMESS
#
###########################################################################

# Absolute directory path to emscripten / closure compiler.
EMSCRIPTEN_DIR = $(CURDIR)/third_party/emscripten
CLOSURE_DIR = $(EMSCRIPTEN_DIR)/third_party/closure-compiler

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

# Flags used to build the native tools.
NATIVE_MESS_FLAGS = PREFIX=native OSD=sdl NOWERROR=1 NOASM=1 NO_X11=1 \
										NO_DEBUGGER=1 CC=$(NATIVE_CC) CXX=$(NATIVE_CXX) \
										AR=$(NATIVE_AR) LD=$(NATIVE_LD)

# Flags used to make the emscripten build
EMSCRIPTEN_MESS_FLAGS = TARGET=mess SUBTARGET=tiny OSD=sdl CROSS_BUILD=1 \
												NATIVE_OBJ="$(NATIVE_OBJ)" \
												TARGETOS=emscripten NOWERROR=1 NOASM=1 NO_X11=1 \
												NO_DEBUGGER=1 PTR64=0

default:
	cd mess; make $(NATIVE_MESS_FLAGS) buildtools
	cd mess; $(EMMAKE) make $(EMSCRIPTEN_MESS_FLAGS)
	mv mess/messtiny mess/messtiny.bc
	$(EMSCRIPTEN_DIR)/emcc -s DISABLE_EXCEPTION_CATCHING=0 mess/messtiny.bc \
		-o messtiny.js --post-js post.js --embed-file ./bin/coleco.zip \
		--embed-file ./bin/cosmofighter2.zip
	java -Xmx1536m -jar $(CLOSURE) --js messtiny.js \
		--js_output_file mess_closure.js

clean:
	cd mess; make $(NATIVE_MESS_FLAGS) clean
	cd mess; $(EMMAKE) make $(EMSCRIPTEN_MESS_FLAGS) clean
	rm mess/messtiny.bc messtiny.js mess_closure.js
