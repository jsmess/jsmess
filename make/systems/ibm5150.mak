################################################################################
# Contains Makefile logic required when building Channel F
################################################################################

# ColecoVision has a bios that is required to run the system.
BIOS := ibm5150.zip
# SUBTARGET for the MESS makefile.
SUBTARGET := ibm5150
MESS_ARGS := ["ibm5150","-verbose","-rompath",".","-window","-resolution","320x200","-nokeepaspect"]

# System-specific flags that should be passed to MESS's makefile.
# MESS_FLAGS +=
EMCC_FLAGS += -s ALLOW_MEMORY_GROWTH=1 -s ASM_JS=0
