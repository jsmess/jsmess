################################################################################
# Contains Makefile logic required when building Jupiter Ace
################################################################################

# Jupiter Ace has a bios that is required to run the system.
BIOS := ace.zip
# SUBTARGET for the MESS makefile.
SUBTARGET := ace
# Arguments to MESS when running a game.
MESS_ARGS = ["ace","-verbose","-rompath",".","-cart",gamename,"-window","-resolution","256x192","-nokeepaspect"]

# System-specific flags that should be passed to MESS's makefile.
# MESS_FLAGS +=
