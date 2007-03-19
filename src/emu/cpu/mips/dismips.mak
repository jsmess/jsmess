..\..\..\dismips.exe: dismips.c mipsdasm.c r3kdasm.c mips3dsm.c
	gcc -O3 -Wall -I../.. -I../../debug -I../../windows -DINLINE="static __inline__" -DSTANDALONE -DMAME_DEBUG -DLSB_FIRST dismips.c mipsdasm.c r3kdasm.c mips3dsm.c -o../../../dismips
