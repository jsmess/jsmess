/******************************************************************************
*
*  Self Contained zexall 'Z80 instruction exercizer' test driver
*  By Jonathan Gevaryahu AKA Lord Nightmare
*  Zexall originally written by Frank Cringle for ZX Spectrum
*  Modularized Spectrum-independent Zexall binary supplied by Blargg
*  Serial interface binary/preloader at 0x0000-0x00FF written by Kevin 'kevtris' Horton
*
*
* mem map:
Ram 0000-FFFF (preloaded with binary)
Special calls take place for three ram values (this interface was designed by kevtris):
FFFD - 'ack' - shared ram with output device; z80 reads from here and considers the byte at FFFF read if this value incremented
FFFE - 'req' - shared ram with output device; z80 writes an incrementing value to FFFE to indicate that there is a byte waiting at FFFF and hence requesting the output device on the other end do something about it, until FFFD is incremented by the output device to acknowledge reciept
FFFF - 'data' - shared ram with output device; z80 writes the data to be sent to output device here
One i/o port is used:
0001 - bit 0 controls whether interrupt timer is enabled (1) or not (0), this is a holdover from a project of kevtris' and can be ignored.

******************************************************************************/

/* Core includes */
#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/terminal.h"

/* Defines */

/* Components */
static UINT8 *main_ram;

static struct
{
	UINT8 data[8]; // unused; to suppress the scalar initializer warning
	UINT8 out_data; // byte written to 0xFFFF
	UINT8 out_req; // byte written to 0xFFFE
	UINT8 out_req_last; // old value at 0xFFFE before the most recent write
	UINT8 out_ack; // byte written to 0xFFFC
} zexall={ {0}};

/* Devices */

static DRIVER_INIT( zexall )
{
	zexall.out_ack = 0;
	zexall.out_req = 0;
	zexall.out_req_last = 0;
	zexall.out_data = 0;
}

static MACHINE_RESET( zexall )
{
	UINT8 *rom = memory_region(machine, "romcode");
	UINT8 *ram = main_ram;
	int i;
    /* fill main ram with zexall code */
    for (i = 0; i < 0x228a; i++)
	{ ram[i] = rom[i];}
	//fprintf(stderr,"MACHINE_RESET: copied romcode to maincpu\n");
}

static READ8_HANDLER( zexall_output_ack_r )
{
	running_device *devconf = space->machine->device("terminal");
// spit out the byte in out_byte if out_req is not equal to out_req_last
	if (zexall.out_req != zexall.out_req_last)
	{
	terminal_write(devconf,0,zexall.out_data);
	fprintf(stderr,"%c",zexall.out_data);
	zexall.out_req_last = zexall.out_req;
	zexall.out_ack++;
	}
	return zexall.out_ack;
}

static WRITE8_HANDLER( zexall_output_ack_w )
{
	zexall.out_ack = data;
}

static READ8_HANDLER( zexall_output_req_r )
{
	return zexall.out_req;
}

static WRITE8_HANDLER( zexall_output_req_w )
{
	zexall.out_req_last = zexall.out_req;
	zexall.out_req = data;
}

static READ8_HANDLER( zexall_output_data_r )
{
	return zexall.out_data;
}

static WRITE8_HANDLER( zexall_output_data_w )
{
	zexall.out_data = data;
}

/******************************************************************************
 Address Maps
******************************************************************************/

static ADDRESS_MAP_START(z80_mem, ADDRESS_SPACE_PROGRAM, 8)
    ADDRESS_MAP_UNMAP_HIGH
    AM_RANGE(0x0000, 0xfffc) AM_RAM AM_BASE(&main_ram)
	AM_RANGE(0xfffd, 0xfffd) AM_READWRITE(zexall_output_ack_r,zexall_output_ack_w)
	AM_RANGE(0xfffe, 0xfffe) AM_READWRITE(zexall_output_req_r,zexall_output_req_w)
	AM_RANGE(0xffff, 0xffff) AM_READWRITE(zexall_output_data_r,zexall_output_data_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(z80_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0001, 0x0001) AM_NOP // really a disable/enable for some sort of interrupt timer on kev's hardware, which is completely irrelevant for the zexall test
ADDRESS_MAP_END


/******************************************************************************
 Input Ports
******************************************************************************/
static INPUT_PORTS_START( zexall )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END

/******************************************************************************
 Machine Drivers
******************************************************************************/
static WRITE8_DEVICE_HANDLER( null_kbd_put )
{
}

static GENERIC_TERMINAL_INTERFACE( dectalk_terminal_intf )
{
	DEVCB_HANDLER(null_kbd_put)
};

static MACHINE_DRIVER_START(zexall)
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", Z80, XTAL_3_579545MHz*10)  
    MDRV_CPU_PROGRAM_MAP(z80_mem)
    MDRV_CPU_IO_MAP(z80_io)
    MDRV_QUANTUM_TIME(HZ(60))
    MDRV_MACHINE_RESET(zexall)

    /* video hardware */
	MDRV_IMPORT_FROM( generic_terminal )
	MDRV_GENERIC_TERMINAL_ADD(TERMINAL_TAG,dectalk_terminal_intf)

MACHINE_DRIVER_END



/******************************************************************************
 ROM Definitions
******************************************************************************/

ROM_START(zexall)

    ROM_REGION(0x10000, "romcode", 0)
    ROM_LOAD("zex.bin", 0x00000, 0x2289, CRC(77E0A1DF) SHA1(CC8F84724E3837783816D92A6DFB8E5975232C66))
ROM_END



/******************************************************************************
 Drivers
******************************************************************************/

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT   INIT      COMPANY                     FULLNAME                                                    FLAGS */
COMP( 2009, zexall,   0,          0,      zexall,   zexall, zexall,      "Frank Cringle & MESSDEV",   "ZEXALL Z80 instruction set exerciser (modified for MESS)", GAME_NO_SOUND_HW )

