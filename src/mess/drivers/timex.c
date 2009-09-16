/***************************************************************************

	NOTE: ****** Specbusy: press N, R, or E to boot *************


        Spectrum/Inves/TK90X etc. memory map:

    CPU:
        0000-3fff ROM
        4000-ffff RAM

        Spectrum 128/+2/+2a/+3 memory map:

        CPU:
                0000-3fff Banked ROM/RAM (banked rom only on 128/+2)
                4000-7fff Banked RAM
                8000-bfff Banked RAM
                c000-ffff Banked RAM

        TS2068 memory map: (Can't have both EXROM and DOCK active)
        The 8K EXROM can be loaded into multiple pages.

    CPU:
                0000-1fff     ROM / EXROM / DOCK (Cartridge)
                2000-3fff     ROM / EXROM / DOCK
                4000-5fff \
                6000-7fff  \
                8000-9fff  |- RAM / EXROM / DOCK
                a000-bfff  |
                c000-dfff  /
                e000-ffff /


Interrupts:

Changes:

29/1/2000   KT -    Implemented initial +3 emulation.
30/1/2000   KT -    Improved input port decoding for reading and therefore
            correct keyboard handling for Spectrum and +3.
31/1/2000   KT -    Implemented buzzer sound for Spectrum and +3.
            Implementation copied from Paul Daniel's Jupiter driver.
            Fixed screen display problems with dirty chars.
            Added support to load .Z80 snapshots. 48k support so far.
13/2/2000   KT -    Added Interface II, Kempston, Fuller and Mikrogen
            joystick support.
17/2/2000   DJR -   Added full key descriptions and Spectrum+ keys.
            Fixed Spectrum +3 keyboard problems.
17/2/2000   KT -    Added tape loading from WAV/Changed from DAC to generic
            speaker code.
18/2/2000   KT -    Added tape saving to WAV.
27/2/2000   KT -    Took DJR's changes and added my changes.
27/2/2000   KT -    Added disk image support to Spectrum +3 driver.
27/2/2000   KT -    Added joystick I/O code to the Spectrum +3 I/O handler.
14/3/2000   DJR -   Tape handling dipswitch.
26/3/2000   DJR -   Snapshot files are now classifed as snapshots not
            cartridges.
04/4/2000   DJR -   Spectrum 128 / +2 Support.
13/4/2000   DJR -   +4 Support (unofficial 48K hack).
13/4/2000   DJR -   +2a Support (rom also used in +3 models).
13/4/2000   DJR -   TK90X, TK95 and Inves support (48K clones).
21/4/2000   DJR -   TS2068 and TC2048 support (TC2048 Supports extra video
            modes but doesn't have bank switching or sound chip).
09/5/2000   DJR -   Spectrum +2 (France, Spain), +3 (Spain).
17/5/2000   DJR -   Dipswitch to enable/disable disk drives on +3 and clones.
27/6/2000   DJR -   Changed 128K/+3 port decoding (sound now works in Zub 128K).
06/8/2000   DJR -   Fixed +3 Floppy support
10/2/2001   KT  -   Re-arranged code and split into each model emulated.
            Code is split into 48k, 128k, +3, tc2048 and ts2048
            segments. 128k uses some of the functions in 48k, +3
            uses some functions in 128, and tc2048/ts2048 use some
            of the functions in 48k. The code has been arranged so
            these functions come in some kind of "override" order,
            read functions changed to use  READ8_HANDLER and write
            functions changed to use WRITE8_HANDLER.
            Added Scorpion256 preliminary.
18/6/2001   DJR -   Added support for Interface 2 cartridges.
xx/xx/2001  KS -    TS-2068 sound fixed.
            Added support for DOCK cartridges for TS-2068.
            Added Spectrum 48k Psycho modified rom driver.
            Added UK-2086 driver.
23/12/2001  KS -    48k machines are now able to run code in screen memory.
                Programs which keep their code in screen memory
                like monitors, tape copiers, decrunchers, etc.
                works now.
                Fixed problem with interrupt vector set to 0xffff (much
            more 128k games works now).
                A useful used trick on the Spectrum is to set
                interrupt vector to 0xffff (using the table
                which contain 0xff's) and put a byte 0x18 hex,
                the opcode for JR, at this address. The first
                byte of the ROM is a 0xf3 (DI), so the JR will
                jump to 0xfff4, where a long JP to the actual
                interrupt routine is put. Due to unideal
                bankswitching in MAME this JP were to 0001 what
                causes Spectrum to reset. Fixing this problem
                made much more software runing (i.e. Paperboy).
            Corrected frames per second value for 48k and 128k
            Sincalir machines.
                There are 50.08 frames per second for Spectrum
                48k what gives 69888 cycles for each frame and
                50.021 for Spectrum 128/+2/+2A/+3 what gives
                70908 cycles for each frame.
            Remaped some Spectrum+ keys.
                Presing F3 to reset was seting 0xf7 on keyboard
                input port. Problem occured for snapshots of
                some programms where it was readed as pressing
                key 4 (which is exit in Tapecopy by R. Dannhoefer
                for example).
            Added support to load .SP snapshots.
            Added .BLK tape images support.
                .BLK files are identical to .TAP ones, extension
                is an only difference.
08/03/2002  KS -    #FF port emulation added.
                Arkanoid works now, but is not playable due to
                completly messed timings.

Initialisation values used when determining which model is being emulated:
 48K        Spectrum doesn't use either port.
 128K/+2    Bank switches with port 7ffd only.
 +3/+2a     Bank switches with both ports.

Notes:
 1. No contented memory.
 2. No hi-res colour effects (need contended memory first for accurate timing).
 3. Multiface 1 and Interface 1 not supported.
 4. Horace and the Spiders cartridge doesn't run properly.
 5. Tape images not supported:
    .TZX, .SPC, .ITM, .PAN, .TAP(Warajevo), .VOC, .ZXS.
 6. Snapshot images not supported:
    .ACH, .PRG, .RAW, .SEM, .SIT, .SNX, .ZX, .ZXS, .ZX82.
 7. 128K emulation is not perfect - the 128K machines crash and hang while
    running quite a lot of games.
 8. Disk errors occur on some +3 games.
 9. Video hardware of all machines is timed incorrectly.
10. EXROM and HOME cartridges are not emulated.
11. The TK90X and TK95 roms output 0 to port #df on start up.
12. The purpose of this port is unknown (probably display mode as TS2068) and
    thus is not emulated.

Very detailed infos about the ZX Spectrum +3e can be found at

http://www.z88forever.org.uk/zxplus3e/

*******************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/spectrum.h"
#include "eventlst.h"
#include "devices/snapquik.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "sound/speaker.h"
#include "sound/ay8910.h"
#include "formats/tzx_cas.h"
#include "machine/beta.h"


static const ay8910_interface spectrum_ay_interface =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_NULL
};

/****************************************************************************************************/
/* TS2048 specific functions */


int ts2068_port_ff_data = -1; /* Display enhancement control */
int ts2068_port_f4_data = -1; /* Horizontal Select Register */

static  READ8_HANDLER(ts2068_port_f4_r)
{
		return ts2068_port_f4_data;
}

static WRITE8_HANDLER(ts2068_port_f4_w)
{
		ts2068_port_f4_data = data;
		ts2068_update_memory(space->machine);
}

static  READ8_HANDLER(ts2068_port_ff_r)
{
		return ts2068_port_ff_data;
}

static WRITE8_HANDLER(ts2068_port_ff_w)
{
		/* Bits 0-2 Video Mode Select
           Bits 3-5 64 column mode ink/paper selection
                    (See ts2068_vh_screenrefresh for more info)
           Bit  6   17ms Interrupt Inhibit
           Bit  7   Cartridge (0) / EXROM (1) select
        */
		ts2068_port_ff_data = data;
		ts2068_update_memory(space->machine);
		logerror("Port %04x write %02x\n", offset, data);
}

/*******************************************************************
 *
 *      Bank switch between the 3 internal memory banks HOME, EXROM
 *      and DOCK (Cartridges). The HOME bank contains 16K ROM in the
 *      0-16K area and 48K RAM fills the rest. The EXROM contains 8K
 *      ROM and can appear in every 8K segment (ie 0-8K, 8-16K etc).
 *      The DOCK is empty and is meant to be occupied by cartridges
 *      you can plug into the cartridge dock of the 2068.
 *
 *      The address space is divided into 8 8K chunks. Bit 0 of port
 *      #f4 corresponds to the 0-8K chunk, bit 1 to the 8-16K chunk
 *      etc. If the bit is 0 then the chunk is controlled by the HOME
 *      bank. If the bit is 1 then the chunk is controlled by either
 *      the DOCK or EXROM depending on bit 7 of port #ff. Note this
 *      means that that the Z80 can't see chunks of the EXROM and DOCK
 *      at the same time.
 *
 *******************************************************************/
void ts2068_update_memory(running_machine *machine)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	unsigned char *ChosenROM, *ExROM, *DOCK;
	read8_space_func rh;
	write8_space_func wh;

	DOCK = timex_cart_data;

	ExROM = memory_region(machine, "maincpu") + 0x014000;

	if (ts2068_port_f4_data & 0x01)
	{
		if (ts2068_port_ff_data & 0x80)
		{
				rh = SMH_BANK(1);
				wh = SMH_UNMAP;
				memory_set_bankptr(machine, 1, ExROM);
				logerror("0000-1fff EXROM\n");
		}
		else
		{
			if (timex_cart_type == TIMEX_CART_DOCK)
			{
				memory_set_bankptr(machine, 1, DOCK);
				rh = SMH_BANK(1);
				if (timex_cart_chunks&0x01)
					wh = SMH_BANK(9);
				else
					wh = SMH_UNMAP;
			}
			else
			{
				rh = SMH_NOP;
				wh = SMH_UNMAP;
			}
			logerror("0000-1fff Cartridge\n");
		}
	}
	else
	{
		ChosenROM = memory_region(machine, "maincpu") + 0x010000;
		memory_set_bankptr(machine, 1, ChosenROM);
		rh = SMH_BANK(1);
		wh = SMH_UNMAP;
		logerror("0000-1fff HOME\n");
	}
	memory_install_read8_handler(space, 0x0000, 0x1fff, 0, 0, rh);
	memory_install_write8_handler(space, 0x0000, 0x1fff, 0, 0, wh);

	if (ts2068_port_f4_data & 0x02)
	{
		if (ts2068_port_ff_data & 0x80)
		{
			memory_set_bankptr(machine, 2, ExROM);
			rh = SMH_BANK(2);
			wh = SMH_UNMAP;
			logerror("2000-3fff EXROM\n");
		}
		else
		{
			if (timex_cart_type == TIMEX_CART_DOCK)
			{
				memory_set_bankptr(machine, 2, DOCK+0x2000);
				rh = SMH_BANK(2);
				if (timex_cart_chunks&0x02)
					wh = SMH_BANK(10);
				else
					wh = SMH_UNMAP;
			}
			else
			{
				rh = SMH_NOP;
				wh = SMH_UNMAP;
			}
			logerror("2000-3fff Cartridge\n");
		}
	}
	else
	{
		ChosenROM = memory_region(machine, "maincpu") + 0x012000;
		memory_set_bankptr(machine, 2, ChosenROM);
		rh = SMH_BANK(2);
		wh = SMH_UNMAP;
		logerror("2000-3fff HOME\n");
	}
	memory_install_read8_handler(space, 0x2000, 0x3fff, 0, 0, rh);
	memory_install_write8_handler(space, 0x2000, 0x3fff, 0, 0, wh);

	if (ts2068_port_f4_data & 0x04)
	{
		if (ts2068_port_ff_data & 0x80)
		{
			memory_set_bankptr(machine, 3, ExROM);
			rh = SMH_BANK(3);
			wh = SMH_UNMAP;
			logerror("4000-5fff EXROM\n");
		}
		else
		{
			if (timex_cart_type == TIMEX_CART_DOCK)
			{
				memory_set_bankptr(machine, 3, DOCK+0x4000);
				rh = SMH_BANK(3);
				if (timex_cart_chunks&0x04)
					wh = SMH_BANK(11);
				else
					wh = SMH_UNMAP;
			}
			else
			{
				rh = SMH_NOP;
				wh = SMH_UNMAP;
			}
			logerror("4000-5fff Cartridge\n");
		}
	}
	else
	{
		memory_set_bankptr(machine, 3, mess_ram);
		memory_set_bankptr(machine, 11, mess_ram);
		rh = SMH_BANK(3);
		wh = SMH_BANK(11);
		logerror("4000-5fff RAM\n");
	}
	memory_install_read8_handler(space, 0x4000, 0x5fff, 0, 0, rh);
	memory_install_write8_handler(space, 0x4000, 0x5fff, 0, 0, wh);

	if (ts2068_port_f4_data & 0x08)
	{
		if (ts2068_port_ff_data & 0x80)
		{
			memory_set_bankptr(machine, 4, ExROM);
			rh = SMH_BANK(4);
			wh = SMH_UNMAP;
			logerror("6000-7fff EXROM\n");
		}
		else
		{
				if (timex_cart_type == TIMEX_CART_DOCK)
				{
					memory_set_bankptr(machine, 4, DOCK+0x6000);
					rh = SMH_BANK(4);
					if (timex_cart_chunks&0x08)
						wh = SMH_BANK(12);
					else
						wh = SMH_UNMAP;
				}
				else
				{
					rh = SMH_NOP;
					wh = SMH_UNMAP;
				}
				logerror("6000-7fff Cartridge\n");
		}
	}
	else
	{
		memory_set_bankptr(machine, 4, mess_ram + 0x2000);
		memory_set_bankptr(machine, 12, mess_ram + 0x2000);
		rh = SMH_BANK(4);
		wh = SMH_BANK(12);
		logerror("6000-7fff RAM\n");
	}
	memory_install_read8_handler(space, 0x6000, 0x7fff, 0, 0, rh);
	memory_install_write8_handler(space, 0x6000, 0x7fff, 0, 0, wh);

	if (ts2068_port_f4_data & 0x10)
	{
		if (ts2068_port_ff_data & 0x80)
		{
			memory_set_bankptr(machine, 5, ExROM);
			rh = SMH_BANK(5);
			wh = SMH_UNMAP;
			logerror("8000-9fff EXROM\n");
		}
		else
		{
			if (timex_cart_type == TIMEX_CART_DOCK)
			{
				memory_set_bankptr(machine, 5, DOCK+0x8000);
				rh = SMH_BANK(5);
				if (timex_cart_chunks&0x10)
					wh = SMH_BANK(13);
				else
					wh = SMH_UNMAP;
			}
			else
			{
				rh = SMH_NOP;
				wh = SMH_UNMAP;
			}
			logerror("8000-9fff Cartridge\n");
		}
	}
	else
	{
		memory_set_bankptr(machine, 5, mess_ram + 0x4000);
		memory_set_bankptr(machine, 13, mess_ram + 0x4000);
		rh = SMH_BANK(5);
		wh = SMH_BANK(13);
		logerror("8000-9fff RAM\n");
	}
	memory_install_read8_handler(space, 0x8000, 0x9fff, 0, 0,rh);
	memory_install_write8_handler(space, 0x8000, 0x9fff, 0, 0,wh);

	if (ts2068_port_f4_data & 0x20)
	{
		if (ts2068_port_ff_data & 0x80)
		{
			memory_set_bankptr(machine, 6, ExROM);
			rh = SMH_BANK(6);
			wh = SMH_UNMAP;
			logerror("a000-bfff EXROM\n");
		}
		else
		{
			if (timex_cart_type == TIMEX_CART_DOCK)
			{
				memory_set_bankptr(machine, 6, DOCK+0xa000);
				rh = SMH_BANK(6);
				if (timex_cart_chunks&0x20)
					wh = SMH_BANK(14);
				else
					wh = SMH_UNMAP;
			}
			else
			{
				rh = SMH_NOP;
				wh = SMH_UNMAP;
			}
			logerror("a000-bfff Cartridge\n");
		}
	}
	else
	{
		memory_set_bankptr(machine, 6, mess_ram + 0x6000);
		memory_set_bankptr(machine, 14, mess_ram + 0x6000);
		rh = SMH_BANK(6);
		wh = SMH_BANK(14);
		logerror("a000-bfff RAM\n");
	}
	memory_install_read8_handler(space, 0xa000, 0xbfff, 0, 0, rh);
	memory_install_write8_handler(space, 0xa000, 0xbfff, 0, 0, wh);

	if (ts2068_port_f4_data & 0x40)
	{
		if (ts2068_port_ff_data & 0x80)
		{
			memory_set_bankptr(machine, 7, ExROM);
			rh = SMH_BANK(7);
			wh = SMH_UNMAP;
			logerror("c000-dfff EXROM\n");
		}
		else
		{
			if (timex_cart_type == TIMEX_CART_DOCK)
			{
				memory_set_bankptr(machine, 7, DOCK+0xc000);
				rh = SMH_BANK(7);
				if (timex_cart_chunks&0x40)
					wh = SMH_BANK(15);
				else
					wh = SMH_UNMAP;
			}
			else
			{
				rh = SMH_NOP;
				wh = SMH_UNMAP;
			}
			logerror("c000-dfff Cartridge\n");
		}
	}
	else
	{
		memory_set_bankptr(machine, 7, mess_ram + 0x8000);
		memory_set_bankptr(machine, 15, mess_ram + 0x8000);
		rh = SMH_BANK(7);
		wh = SMH_BANK(15);
		logerror("c000-dfff RAM\n");
	}
	memory_install_read8_handler(space, 0xc000, 0xdfff, 0, 0, rh);
	memory_install_write8_handler(space, 0xc000, 0xdfff, 0, 0, wh);

	if (ts2068_port_f4_data & 0x80)
	{
		if (ts2068_port_ff_data & 0x80)
		{
			memory_set_bankptr(machine, 8, ExROM);
			rh = SMH_BANK(8);
			wh = SMH_UNMAP;
			logerror("e000-ffff EXROM\n");
		}
		else
		{
			if (timex_cart_type == TIMEX_CART_DOCK)
			{
				memory_set_bankptr(machine, 8, DOCK+0xe000);
				rh = SMH_BANK(8);
				if (timex_cart_chunks&0x80)
					wh = SMH_BANK(16);
				else
					wh = SMH_UNMAP;
			}
			else
			{
				rh = SMH_NOP;
				wh = SMH_UNMAP;
			}
			logerror("e000-ffff Cartridge\n");
		}
	}
	else
	{
		memory_set_bankptr(machine, 8, mess_ram + 0xa000);
		memory_set_bankptr(machine, 16, mess_ram + 0xa000);
		rh = SMH_BANK(8);
		wh = SMH_BANK(16);
		logerror("e000-ffff RAM\n");
	}
	memory_install_read8_handler(space, 0xe000, 0xffff, 0, 0, rh);
	memory_install_write8_handler(space, 0xe000, 0xffff, 0, 0, wh);
}

static ADDRESS_MAP_START (ts2068_io, ADDRESS_SPACE_IO, 8)	
	AM_RANGE(0x1f, 0x1f) AM_READ( spectrum_port_1f_r ) AM_MIRROR(0xff00)
	AM_RANGE(0x7f, 0x7f) AM_READ( spectrum_port_7f_r ) AM_MIRROR(0xff00)
	AM_RANGE(0xdf, 0xdf) AM_READ( spectrum_port_df_r ) AM_MIRROR(0xff00)
	AM_RANGE(0xf4, 0xf4) AM_READWRITE( ts2068_port_f4_r,ts2068_port_f4_w ) AM_MIRROR(0xff00)
	AM_RANGE(0xf5, 0xf5) AM_DEVWRITE( "ay8912", ay8910_address_w ) AM_MIRROR(0xff00)
	AM_RANGE(0xf6, 0xf6) AM_DEVREADWRITE("ay8912", ay8910_r, ay8910_data_w ) AM_MIRROR(0xff00)
	AM_RANGE(0xfe, 0xfe) AM_READWRITE( spectrum_port_fe_r,spectrum_port_fe_w )  AM_MIRROR(0xff00)  AM_MASK(0xffff)
	AM_RANGE(0xff, 0xff) AM_READWRITE( ts2068_port_ff_r,ts2068_port_ff_w ) AM_MIRROR(0xff00)
ADDRESS_MAP_END

static ADDRESS_MAP_START (ts2068_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x1fff) AM_READWRITE( SMH_BANK(1), SMH_BANK(9) )
	AM_RANGE(0x2000, 0x3fff) AM_READWRITE( SMH_BANK(2), SMH_BANK(10) )
	AM_RANGE(0x4000, 0x5fff) AM_READWRITE( SMH_BANK(3), SMH_BANK(11) )
	AM_RANGE(0x6000, 0x7fff) AM_READWRITE( SMH_BANK(4), SMH_BANK(12) )
	AM_RANGE(0x8000, 0x9fff) AM_READWRITE( SMH_BANK(5), SMH_BANK(13) )
	AM_RANGE(0xa000, 0xbfff) AM_READWRITE( SMH_BANK(6), SMH_BANK(14) )
	AM_RANGE(0xc000, 0xdfff) AM_READWRITE( SMH_BANK(7), SMH_BANK(15) )
	AM_RANGE(0xe000, 0xffff) AM_READWRITE( SMH_BANK(8), SMH_BANK(16) )
ADDRESS_MAP_END


static MACHINE_RESET( ts2068 )
{
	ts2068_port_ff_data = 0;
	ts2068_port_f4_data = 0;
	ts2068_update_memory(machine);

	MACHINE_RESET_CALL(spectrum);
}


/****************************************************************************************************/
/* TC2048 specific functions */


static WRITE8_HANDLER (tc2048_port_ff_w)
{
		ts2068_port_ff_data = data;
		logerror("Port %04x write %02x\n", offset, data);
}

static ADDRESS_MAP_START (tc2048_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x00, 0x00) AM_READWRITE(spectrum_port_fe_r,spectrum_port_fe_w) AM_MIRROR(0xfffe) AM_MASK(0xffff) 
	AM_RANGE(0x1f, 0x1f) AM_READ(spectrum_port_1f_r) AM_MIRROR(0xff00)
	AM_RANGE(0x7f, 0x7f) AM_READ(spectrum_port_7f_r) AM_MIRROR(0xff00)
	AM_RANGE(0xdf, 0xdf) AM_READ(spectrum_port_df_r) AM_MIRROR(0xff00)
	AM_RANGE(0xff, 0xff) AM_READWRITE(ts2068_port_ff_r,tc2048_port_ff_w)  AM_MIRROR(0xff00)	
ADDRESS_MAP_END

static ADDRESS_MAP_START (tc2048_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x3fff) AM_ROM
	AM_RANGE( 0x4000, 0xffff) AM_READWRITE( SMH_BANK(1), SMH_BANK(2) )
ADDRESS_MAP_END

static MACHINE_RESET( tc2048 )
{
	memory_set_bankptr(machine, 1, mess_ram);
	memory_set_bankptr(machine, 2, mess_ram);
	ts2068_port_ff_data = 0;

	MACHINE_RESET_CALL(spectrum);
}

static MACHINE_DRIVER_START( ts2068 )
	MDRV_IMPORT_FROM( spectrum_128 )
	MDRV_CPU_REPLACE("maincpu", Z80, XTAL_14_112MHz/4)        /* From Schematic; 3.528 MHz */
	MDRV_CPU_PROGRAM_MAP(ts2068_mem)
	MDRV_CPU_IO_MAP(ts2068_io)

	MDRV_MACHINE_RESET( ts2068 )

    /* video hardware */
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(TS2068_SCREEN_WIDTH, TS2068_SCREEN_HEIGHT)
	MDRV_SCREEN_VISIBLE_AREA(0, TS2068_SCREEN_WIDTH-1, 0, TS2068_SCREEN_HEIGHT-1)

	MDRV_VIDEO_UPDATE( ts2068 )
	MDRV_VIDEO_EOF( ts2068 )

	/* sound */
	MDRV_SOUND_REPLACE("ay8912", AY8912, XTAL_14_112MHz/8)        /* From Schematic; 1.764 MHz */
	MDRV_SOUND_CONFIG(spectrum_ay_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)	

	/* cartridge */
	MDRV_CARTSLOT_MODIFY("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("dck")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(timex_cart)
	MDRV_CARTSLOT_UNLOAD(timex_cart)		
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( uk2086 )
	MDRV_IMPORT_FROM( ts2068 )
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE(50)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( tc2048 )
	MDRV_IMPORT_FROM( spectrum )
	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_PROGRAM_MAP(tc2048_mem)
	MDRV_CPU_IO_MAP(tc2048_io)

	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE(50)

	MDRV_MACHINE_RESET( tc2048 )

    /* video hardware */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(TS2068_SCREEN_WIDTH, SPEC_SCREEN_HEIGHT)
	MDRV_SCREEN_VISIBLE_AREA(0, TS2068_SCREEN_WIDTH-1, 0, SPEC_SCREEN_HEIGHT-1)

	MDRV_VIDEO_START( spectrum_128 )
	MDRV_VIDEO_UPDATE( tc2048 )
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(tc2048)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("tc2048.rom",0x0000,0x4000, CRC(f1b5fa67) SHA1(febb2d495b6eda7cdcb4074935d6e9d9f328972d))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(ts2068)
	ROM_REGION(0x16000,"maincpu",0)
	ROM_LOAD("ts2068_h.rom",0x10000,0x4000, CRC(bf44ec3f) SHA1(1446cb2780a9dedf640404a639fa3ae518b2d8aa))
	ROM_LOAD("ts2068_x.rom",0x14000,0x2000, CRC(ae16233a) SHA1(7e265a2c1f621ed365ea23bdcafdedbc79c1299c))
ROM_END

ROM_START(uk2086)
	ROM_REGION(0x16000,"maincpu",0)
	ROM_LOAD("uk2086_h.rom",0x10000,0x4000, CRC(5ddc0ca2) SHA1(1d525fe5cdc82ab46767f665ad735eb5363f1f51))
	ROM_LOAD("ts2068_x.rom",0x14000,0x2000, CRC(ae16233a) SHA1(7e265a2c1f621ed365ea23bdcafdedbc79c1299c))
ROM_END

static SYSTEM_CONFIG_START(ts2068)
	CONFIG_RAM_DEFAULT(48 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(tc2048)
	CONFIG_RAM_DEFAULT(48 * 1024)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT    COMPAT  MACHINE     INPUT       INIT    CONFIG      COMPANY     FULLNAME */
COMP( 1984, tc2048,   spectrum, 0,		tc2048,		spectrum,	0,		tc2048,		"Timex of Portugal",	"TC-2048" , 0)
COMP( 1983, ts2068,   spectrum, 0,		ts2068,		spectrum,	0,		ts2068,		"Timex Sinclair",		"TS-2068" , 0)
COMP( 1986, uk2086,   spectrum, 0,		uk2086,		spectrum,	0,		ts2068,		"Unipolbrit",			"UK-2086 ver. 1.2" , 0)
