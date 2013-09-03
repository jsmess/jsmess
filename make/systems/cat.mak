##################################################################
#                                                                
#    JSMESS-specific makefile for Cat.
#                                                                
##################################################################

# Cat BIOS File Location
#         
# The filename of the .zip file containing the machine's ROM files.
# Most computer and console systems will require this collection.
#
BIOS := cat.zip

# SUBTARGET Location
# 
# The MESS source of the driver (may be completely different).
#
SUBTARGET := cat

# MESS Arguments for Cat
#         
# Arguments that will be passed to the JSMESS routine to properly 
# emulate the system and provide the type of files to read and screen
# settings. Can be modified later.
#
MESS_ARGS := ["cat","-verbose","-rompath",".","-window","-resolution","672x344","-nokeepaspect"]

# MESS Compilation Flags
# 
# Some systems need additional compilation flags to work properly, especially
# with regards to memory usage.
#
# MESS_FLAGS +=
# EMCC_FLAGS +=

