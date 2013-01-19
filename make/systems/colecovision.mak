################################################################################
# Contains Makefile logic required when building ColecoVision
################################################################################

# ColecoVision has a bios that is required to run the system.
BIOS := coleco.zip
# SUBTARGET for the MESS makefile.
SUBTARGET := colecovision
# Arguments to MESS when running a game.
MESS_ARGS = ["coleco","-verbose","-rompath",".","-cart",gamename,"-nokeepaspect"]

# System-specific flags that should be passed to MESS's makefile.
# MESS_FLAGS +=