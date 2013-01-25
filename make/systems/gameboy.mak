################################################################################
# Contains Makefile logic required when building ColecoVision
################################################################################

# ColecoVision has a bios that is required to run the system.
BIOS := gameboy.zip
# SUBTARGET for the MESS makefile.
SUBTARGET := gameboy
# Arguments to MESS when running a game.
MESS_ARGS = ["gameboy","-verbose","-rompath",".","-cart",gamename,"-window","-resolution","256x192","-nokeepaspect"]

# System-specific flags that should be passed to MESS's makefile.
# MESS_FLAGS +=
