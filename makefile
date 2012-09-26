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

# Flags used to build the native tools.
NATIVE_MESS_FLAGS = PREFIX=native OSD=sdl NOWERROR=1 NOASM=1 NO_X11=1 \
										NO_DEBUGGER=1 CC=clang CXX=clang++ AR=ar LD=clang++

# Flags used to make the emscripten build
EMSCRIPTEN_MESS_FLAGS = TARGET=mess SUBTARGET=tiny OSD=sdl CROSS_BUILD=1 \
												NATIVE_OBJ="$(CURDIR)/mess/obj/sdl/nativemame" \
												TARGETOS=emscripten NOWERROR=1 NOASM=1 NO_X11=1 \
												NO_DEBUGGER=1

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