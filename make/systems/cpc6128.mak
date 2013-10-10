################################################################################
# Contains Makefile logic required when building ColecoVision
################################################################################

# ColecoVision has a bios that is required to run the system.
BIOS := cpc6128.zip
# SUBTARGET for the MESS makefile.
SUBTARGET := cpc6128
# Arguments to MESS when running a game.
MESS_ARGS = ["cpc6128","-verbose","-rompath",".","-flop1",gamename,"-window","-resolution","384x272","-nokeepaspect"]

# System-specific flags that should be passed to MESS's makefile.
# MESS_FLAGS +=
