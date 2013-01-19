################################################################################
# Contains Makefile logic required when building Channel F
################################################################################

# ColecoVision has a bios that is required to run the system.
BIOS := channelf.zip
# SUBTARGET for the MESS makefile.
SUBTARGET := channelf
MESS_ARGS := ["channelf","-verbose","-rompath",".","-cart",gamename,"-window","-resolution","102x58","-nokeepaspect"]

# System-specific flags that should be passed to MESS's makefile.
# MESS_FLAGS +=
