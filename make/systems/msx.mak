################################################################################
# Contains Makefile logic required when building MSX
################################################################################

# Spectrum has a bios that is required to run the system.
BIOS := msx.zip
# SUBTARGET for the MESS makefile.
SUBTARGET := msx
MESS_ARGS := ["msx","-verbose","-rompath",".","-window","-resolution","280x216","-cart1",gamename,"-nokeepaspect"]

# System-specific flags that should be passed to MESS's makefile.
# MESS_FLAGS +=

EMCC_FLAGS += -s TOTAL_MEMORY=134217728
