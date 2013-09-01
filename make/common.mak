################################################################################
#
#   common.mak
#
#   This is where we hide all of our dirty Makefile secrets. Namely, all of the
#   gross details like 64-bit checking.
#
################################################################################

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
# The HTML template we'll be using.
TEMPLATE_DIR := $(CURDIR)/templates/default
# All of the files in the template directory. Allows for 'smart' HTML rebuilding
TEMPLATE_FILES := $(shell ls $(TEMPLATE_DIR))
TEMPLATE_FILES := $(foreach TFILE,$(TEMPLATE_FILES),$(TEMPLATE_DIR)/$(TFILE))

# The name of the bitcode executable produced by making mess.
MESS_EXE := mess$(SUBTARGET)

ifeq ($(IS_64_BIT),1)
NATIVE_OBJ := $(CURDIR)/mess/obj/sdl/nativemame64
else
NATIVE_OBJ := $(CURDIR)/mess/obj/sdl/nativemame
endif
