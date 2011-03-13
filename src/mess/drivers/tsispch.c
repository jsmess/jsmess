/******************************************************************************
*
*  Telesensory Systems Inc./Speech Plus
*  1500 and 2000 series
*  Prose 2020
*  By Jonathan Gevaryahu AKA Lord Nightmare and Kevin 'kevtris' Horton
*
*  Skeleton Driver
*
*  DONE:
*  Skeleton Written
*  Load cpu and dsp roms and mapper proms
*  Successful compile
*  Successful run
*  Correctly Interleave 8086 CPU roms
*  Debug LEDs hooked to popmessage
*
*  TODO:
*  Correct memory maps and io maps
*  Correctly load UPD7720 roms either as UPD7725 data (shuffle some bits around) or make UPD7720 CPU core
*  Attach peripherals, timers and uarts
*  Add dipswitches and jumpers
*  Hook terminal to serial UARTS
*  Everything else 
*
*  Notes:
*  Text in rom indicates there is a test mode 'activated by switch s4 dash 7'
******************************************************************************/
#define ADDRESS_MAP_MODERN

/* Core includes */
#include "emu.h"
#include "includes/tsispch.h"
#include "cpu/upd7725/upd7725.h"
#include "cpu/i86/i86.h"
#include "machine/terminal.h"

static GENERIC_TERMINAL_INTERFACE( tsispch_terminal_intf )
{
	DEVCB_NULL // for now...
};

WRITE16_MEMBER( tsispch_state::led_w )
{
	tsispch_state *state = machine->driver_data<tsispch_state>();
	UINT16 data2 = data >> 8;
	state->statusLED = data2&0xFF;
	//fprintf(stderr,"0x03400 LED write: %02X\n", data);
	//popmessage("LED status: %02X\n", data2&0xFF);
#ifdef VERBOSE
	logerror("8086: LED status: %02X\n", data2&0xFF);
#endif
	popmessage("LED status: %x %x %x %x %x %x %x %x\n", BIT(data2,7), BIT(data2,6), BIT(data2,5), BIT(data2,4), BIT(data2,3), BIT(data2,2), BIT(data2,1), BIT(data2,0));
}

/* Reset */
void tsispch_state::machine_reset()
{
	// clear fifos (TODO: memset would work better here...)
	int i;
	for (i=0; i<32; i++) infifo[i] = 0;
	infifo_tail_ptr = infifo_head_ptr = 0;
	fprintf(stderr,"machine reset\n");
}

DRIVER_INIT( prose2k )
{
	UINT8 *dspsrc = (UINT8 *)machine->region("dspprgload")->base();
	UINT32 *dspprg = (UINT32 *)machine->region("dspprg")->base();
	fprintf(stderr,"driver init\n");
    // unpack 24 bit data into 32 bit space
	// TODO: unpack such that it can actually RUN as upd7725 code; this requires
	//       some minor shifting
        for (int i = 0; i < 0x600; i+= 3)
        {
            *dspprg = dspsrc[0+i]<<24 | dspsrc[1+i]<<16 | dspsrc[2+i]<<8;
            dspprg++;
        }
}

/******************************************************************************
 Address Maps
******************************************************************************/

static ADDRESS_MAP_START(i8086_mem, ADDRESS_SPACE_PROGRAM, 16, tsispch_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0x02BFF) AM_RAM // unverified; tested on startup test
	AM_RANGE(0x02C00, 0x02FFF) AM_RAM // unverified; 2F32-2F92 may be special ram for talking to the DSP?
	//AM_RANGE(0x03200, 0x03201) // UART 1
	//AM_RANGE(0x03202, 0x03203) // UART 2
	AM_RANGE(0x03400, 0x03401) AM_WRITE(led_w) // write is the 8 debug leds; reading here may be dipswitches?
	AM_RANGE(0xc0000, 0xfffff) AM_ROM // correct
ADDRESS_MAP_END

static ADDRESS_MAP_START(i8086_io, ADDRESS_SPACE_IO, 16, tsispch_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x990, 0x991) AM_NOP // wrong; to force correct compile
ADDRESS_MAP_END

static ADDRESS_MAP_START(dsp_prg_map, ADDRESS_SPACE_PROGRAM, 32, tsispch_state)
    AM_RANGE(0x0000, 0x01ff) AM_ROM AM_REGION("dspprg", 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START(dsp_data_map, ADDRESS_SPACE_DATA, 16, tsispch_state)
    AM_RANGE(0x0000, 0x01ff) AM_ROM AM_REGION("dspdata", 0)
ADDRESS_MAP_END


/******************************************************************************
 Input Ports
******************************************************************************/
static INPUT_PORTS_START( prose2k )
INPUT_PORTS_END

/******************************************************************************
 Machine Drivers
******************************************************************************/
static MACHINE_CONFIG_START( prose2k, tsispch_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu", I8086, 4772720) /* TODO: correct clock speed */
    MCFG_CPU_PROGRAM_MAP(i8086_mem)
    MCFG_CPU_IO_MAP(i8086_io)

    MCFG_CPU_ADD("dsp", UPD7725, 8000000) /* TODO: correct clock and correct dsp type is UPD77P20 */
    MCFG_CPU_PROGRAM_MAP(dsp_prg_map)
    MCFG_CPU_DATA_MAP(dsp_data_map)

    /* sound hardware */
    //MCFG_SPEAKER_STANDARD_MONO("mono")
    //MCFG_SOUND_ADD("dac", DAC, 0) /* TODO: correctly figure out how the DAC works */
    //MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

    MCFG_FRAGMENT_ADD( generic_terminal )
    MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG,tsispch_terminal_intf)
MACHINE_CONFIG_END

/******************************************************************************
 ROM Definitions
******************************************************************************/
ROM_START( prose2k )
	ROM_REGION(0x100000,"maincpu", 0)
	// prose 2000/2020 firmware version 3.4.1
	ROMX_LOAD( "2.u22",   0xc0000, 0x10000, CRC(201D3114) SHA1(549EF1AA28D5664D4198CBC1826B31020D6C4870),ROM_SKIP(1))
	ROMX_LOAD( "3.u45",   0xc0001, 0x10000, CRC(190C77B6) SHA1(2B90B3C227012F2085719E6283DA08AFB36F394F),ROM_SKIP(1))
	ROMX_LOAD( "0.u21",   0xe0000, 0x10000, CRC(3FAE874A) SHA1(E1D3E7BA309B29A9C3EDBE3D22BECF82EAE50A31),ROM_SKIP(1))
	ROMX_LOAD( "1.u44",   0xe0001, 0x10000, CRC(BDBB0785) SHA1(6512A8C2641E032EF6BB0889490D82F5D4399575),ROM_SKIP(1))

	// TSI/Speech plus DSP firmware v3.21 8/9/88
	ROM_REGION( 0x600, "dspprgload", 0) // packed 24 bit data
	ROM_LOAD( "dsp_prog.bin", 0x0000, 0x0600, CRC(9E46425A) SHA1(80A915D731F5B6863AEEB448261149FF15E5B786))
	ROM_REGION( 0x800, "dspprg", ROMREGION_ERASEFF) // for unpacking 24 bit data into
	ROM_REGION( 0x400, "dspdata", 0)
	ROM_LOAD( "dsp_data.bin", 0x0000, 0x0400, CRC(F4E4DD16) SHA1(6E184747DB2F26E45D0E02907105FF192E51BABA))

	// mapping proms:
	// L - always low; H - always high
	// U77: unknown; input is ?; output bits 0bLLLLzyxH
	//      x - unknown
	//      y - unknown
	//      z - unknown
	//
	// U79: unknown; input is ?; output bits 0bLLLLyHHx
	//      x - unknown
	//      y - unknown 
	//
	// U81: maps ROMS to C0000-FFFFF: input is A15-A19; output bits: 0bHyxHHHHH
	//      x - to /CE of roms 2 and 3
	//      y - to /CE of roms 0 and 1
	ROM_REGION(0x1000, "proms", 0)
	ROM_LOAD( "u77.bin", 0x0000, 0x0020, CRC(A88757FC) SHA1(9066D6DBC009D7A126D75B8461CA464DDF134412))
	ROM_LOAD( "u79.bin", 0x0020, 0x0020, CRC(A165B090) SHA1(BFC413C79915C68906033741318C070AD5DD0F6B))
	ROM_LOAD( "u81.bin", 0x0040, 0x0020, CRC(62E1019B) SHA1(ACADE372EDB08FD0DCB1FA3AF806C22C47081880))
	ROM_END


/******************************************************************************
 Drivers
******************************************************************************/

/*    YEAR  NAME	PARENT	COMPAT	MACHINE		INPUT	INIT	COMPANY     FULLNAME            FLAGS */
COMP( 1984?, prose2k,	0,		0,		prose2k,		prose2k,	prose2k,	"Telesensory Systems Inc/Speech Plus",	"Prose 2000/2020",	GAME_NOT_WORKING | GAME_NO_SOUND )
