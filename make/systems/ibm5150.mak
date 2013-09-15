##################################################################
#                                                                
#    JSMESS-specific makefile for IBM PC 5150.
#                                                                
##################################################################

# IBM PC 5150 BIOS File Location
#         
# The filename of the .zip file containing the machine's ROM files.
# Most computer and console systems will require this collection.
#
BIOS := ibm5150.zip

# SUBTARGET Location
# 
# The MESS source of the driver (may be completely different).
#
SUBTARGET := ibmpc

# MESS Arguments for IBM PC 5150
#         
# Arguments that will be passed to the JSMESS routine to properly 
# emulate the system and provide the type of files to read and screen
# settings. Can be modified later.
#
MESS_ARGS := ["ibm5150","-verbose","-rompath",".","-cass",gamename,"-window","-resolution","640x200","-nokeepaspect"]

# MESS Compilation Flags
# 
# Some systems need additional compilation flags to work properly, especially
# with regards to memory usage.
#
# MESS_FLAGS +=
EMCC_FLAGS += -s TOTAL_MEMORY=33554432

