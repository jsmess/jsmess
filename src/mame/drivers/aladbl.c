#include "driver.h"
#include "megadriv.h"

/* it probably works, start is shift (start on md pad), but the credit system isn't emulated
   properly yet, it's probably what the MCU on the board is for

   left as non-working until i've had more time to investigate

   */

/*

CPU
Main cpu 68000P10
Work ram 64kb (62256 x2)
Sound cpu z80B
Sound ram 8kb (76c88-6264 x1)
Sound ic Ym2612 (identified by pins,code was been erased.Named on board as TA07)

Other ics
Microchip PIC16C57 (probably it contains the MD modified bios)
Osc 50 Mhz
There are present 3 flat-pack chips with code erased again and named TA04,TA05,TA06 on board,which i have
identified (generically) by looking the pcb as:
TA04-Intercommunication and sync generator chip
TA05-Input controller
TA06-VDP (probably MD clone) Uses 2x D41264 SIL package as video ram

ROMs

M3,M4 main program
M1,M2 graphics
All eproms are 27c040

Notes:

Dip-switch 8 x1

------------------------

This romset comes from a bootleg pcb.The game is a coin-op conversion of the one developed for the Megadrive
console.I cannot know gameplay differences since pcb is faulty.

However,hardware is totally different.It seems to be based on Sega Mega Drive hardware with cpu clock increased,
and since exists an "unlicensed" porting of the game for this system probably the "producers" are the same.

*/

ROM_START( aladbl )
	ROM_REGION( 0x400000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "m1.bin", 0x000001, 0x080000,  CRC(5e2671e4) SHA1(54705c7614fc7b5a1065478fa41f51dd1d8045b7) )
	ROM_LOAD16_BYTE( "m2.bin", 0x000000, 0x080000,  CRC(142a0366) SHA1(6c94aa9936cd11ccda503b52019a6721e64a32f0) )
	ROM_LOAD16_BYTE( "m3.bin", 0x100001, 0x080000,  CRC(0feeeb19) SHA1(bd567a33077ab9997871d21736066140d50e3d70) )
	ROM_LOAD16_BYTE( "m4.bin", 0x100000, 0x080000,  CRC(bc712661) SHA1(dfd554d000399e17b4ddc69761e572195ed4e1f0))
ROM_END

READ16_HANDLER( aladbl_r )
{
	if (activecpu_get_pc()==0x1b2d24) return 0x1200;
	if (activecpu_get_pc()==0x1b2a56) return mame_rand(Machine); // coins
	if (activecpu_get_pc()==0x1b2a72) return 0x0000;//mame_rand(Machine);
	if (activecpu_get_pc()==0x1b2d4e) return mame_rand(Machine);

	printf("read at %06x\n",activecpu_get_pc());

	return 0x00;
}

DRIVER_INIT( aladbl )
{
	// 220000 = writes to mcu? 330000 = reads?
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x330000, 0x330001, 0, 0, aladbl_r);
	driver_init_megadrij(machine);
}

GAME( 1993, aladbl ,  0,   megadriv,    megadriv,     aladbl,  ROT0,   "bootleg / Sega", "Aladdin (bootleg of Japanese Megadrive version)", GAME_NOT_WORKING )
