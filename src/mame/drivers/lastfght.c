/*

  Last Fighting
  (c) 2000 Subsino

  Main CPU: H8/3044


PCB Layout
----------

|------------------------------------------------------|
|TDA1519A                                              |
|     VOL                                              |
|                HM86171                       ULN2003 |
|   LM324                                              |
|           S-1                                ULN2003 |
|                                                      |
|                                    |-----|   ULN2003 |
|                        |-----|     |U1   |           |
|              V100.U7   |U6   |     |     |           |
|J                       |     |     |-----|           |
|A                       |-----|                       |
|M                                                     |
|M                                                     |
|A          KM428C256      32MHz     CXK58257          |
|     |----ROM BOARD------|                            |
|     |                   |          V106.U16          |
|     |          U19      |                         SW1|
|     |       |-------|   |          |-----|           |
|     |       |SUBSINO|   |          |H8   |           |
|     |       |9623EX |   | |-----|  |3044 |           |
|     |       |008    |   | |EPM  |  |-----|           |
|     |       |-------|   | |7032 |                    |
|     |     CN2           | |-----|             3V_BATT|
|-----|-------------------|----------------------------|
Notes:
      H8/3044 - Subsino re-badged Hitachi H8/3044 HD6433044A22F Microcontroller (QFP100)
                The H8/3044 is a H8/3002 with 24bit address bus and has 32k MASKROM and 2k RAM, clock input is 16.000MHz [32/2]
                MD0,MD1 & MD2 are configured to MODE 6 16M-Byte Expanded Mode with the on-chip 32k MASKROM enabled.
      EPM7032 - Altera EPM7032LC44-15T CPLD (PLCC44)
     CXK58257 - Sony CXK58257 32k x8 SRAM (SOP28)
    KM428C256 - Samsung Semiconductor KM428C256 256k x8 Dual Port DRAM (SOJ40)
     ULKN2003 - Toshiba ULN2003 High Voltage High Current Darlington Transistor Array comprising 7 NPN Darlinton pairs (DIP16)
      HM86171 - Hualon Microelectronics HMC HM86171 VGA 256 colour RAMDAC (DIP28)
      3V_BATT - 3 Volt Coin Battery. This is tied to the CXK58257 SRAM. It appears to be used as an EEPROM, as the game
                has on-board settings in test mode and there's no DIPs and no EEPROM.
          S-1 - ?? Probably some kind of audio OP AMP or DAC? (DIP8)
     TDA1519A - Philips TDA1519A 22W BTL/Dual 11W Audio Power Amplifier IC (SIL9)
          CN2 - 70 pin connector for connection of ROM board
          SW1 - Push Button Test Switch
        HSync - 15.75kHz
        VSync - 60Hz
    ROM BOARD - Small Daughterboard containing positions for 8x 16MBit SOP44 MASKROMs. Only positions 1-4 are populated.
   Custom ICs -
                U19     - SUBSINO 9623EX008 (QFP208)
                H8/3044 - SUBSINO SS9689 6433044A22F, rebadged Hitachi H8/3044 MCU (QFP100)
                U1      - SUBSINO SS9802 9933 (QFP100)
                U6      - SUBSINO SS9804 0001 (QFP100)
         ROMs -
                V106.U16 - MX27C4000 4MBit DIP32 EPROM; Main Program
                V100.U7  - ST M27C801 8MBit DIP32 EPROM; Audio Samples?

*/

#include "driver.h"
#include "cpu/h83002/h83002.h"

static int clr_offset=0;

INPUT_PORTS_START( lastfght )
INPUT_PORTS_END

static VIDEO_START( lastfght )
{
	return 0;
}

static VIDEO_UPDATE( lastfght )
{
	int x,y;
	int count;
	static int base = 0;

	fillbitmap( bitmap, get_black_pen(machine), cliprect );
	fillbitmap( priority_bitmap, 0, cliprect );

	if ( code_pressed_memory(KEYCODE_Z) )
		base += 512*512;

	if ( code_pressed_memory(KEYCODE_X) )
		base -= 512*512;

	count = base;

	for (y=0;y<512;y++)
	{
		for (x=0;x<512;x++)
		{
			UINT8 *gfxdata = memory_region( REGION_GFX1 );
			UINT8 data;
			data = gfxdata[count];
			count++;
			plot_pixel(bitmap,x,y,data);
		}
	}

	if(code_pressed(KEYCODE_Q))
	{
		cpunum_set_input_line(0, 0, PULSE_LINE);
	}

	if(code_pressed(KEYCODE_W))
	{
		cpunum_set_input_line(0, 1, PULSE_LINE);
	}

	if(code_pressed(KEYCODE_E))
	{
		cpunum_set_input_line(0, 2, PULSE_LINE);
	}

	if(code_pressed(KEYCODE_R))
	{
		cpunum_set_input_line(0, 3, PULSE_LINE);
	}

	if(code_pressed(KEYCODE_T))
	{
		cpunum_set_input_line(0, 4, PULSE_LINE);
	}

	if(code_pressed(KEYCODE_Y))
	{
		cpunum_set_input_line(0, 5, PULSE_LINE);
	}

	if(code_pressed(KEYCODE_U))
	{
		cpunum_set_input_line(0, 6, PULSE_LINE);
	}

	if(code_pressed(KEYCODE_I))
	{
		cpunum_set_input_line(0, 7, PULSE_LINE);
	}
	return 0;
}

static WRITE16_HANDLER(colordac_w)
{
	if(ACCESSING_LSB)
	{
		colorram[clr_offset]=data;
		palette_set_color(Machine,clr_offset/3,pal6bit(colorram[(clr_offset/3)*3]),pal6bit(colorram[(clr_offset/3)*3+1]),pal6bit(colorram[(clr_offset/3)*3+2]));
		clr_offset=(clr_offset+1)%768;
	}

	if(ACCESSING_MSB)
	{
		clr_offset=(data>>8)*3;
	}
}

static ADDRESS_MAP_START( lastfght_map, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(24) )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM AM_REGION(REGION_CPU1, 0)
	AM_RANGE(0x080000, 0x0fffff) AM_ROM AM_REGION(REGION_CPU1, 0)
	AM_RANGE(0x200000, 0x20ffff) AM_RAM
	AM_RANGE(0x600008, 0x600009) AM_WRITE(colordac_w)
	AM_RANGE(0x800000, 0x80001f) AM_RAM // blitter regs ?
	AM_RANGE(0xc00000, 0xc0001f) AM_RAM // protection ?
	AM_RANGE(0xff0000, 0xffffff) AM_RAM
ADDRESS_MAP_END

static INTERRUPT_GEN( unknown_interrupt )
{
	cpunum_set_input_line(0, cpu_getiloops(), PULSE_LINE);
}


static MACHINE_DRIVER_START( lastfght )
	MDRV_CPU_ADD(H83002, 16000000)
	MDRV_CPU_PROGRAM_MAP( lastfght_map, 0 )
//  MDRV_CPU_VBLANK_INT(unknown_interrupt,6)

	MDRV_SCREEN_REFRESH_RATE( 60 )
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC( 0 ))

	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE( 512, 512 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 512-1, 0, 512-1 )
	MDRV_PALETTE_LENGTH( 256 )

	MDRV_VIDEO_START( lastfght )
	MDRV_VIDEO_UPDATE( lastfght )

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
MACHINE_DRIVER_END

ROM_START( lastfght )
	// H8/3044 program
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD( "v106.u16",     0x000000, 0x080000, CRC(7aec89f4) SHA1(7cff00844ad82a0f8d19b1bd07ba3a2bced69d66) )

	// graphics (512x256? pages)
	ROM_REGION( 0x800000, REGION_GFX1, 0 )
	ROM_LOAD( "1.b1",         0x000000, 0x200000, CRC(6c438136) SHA1(138934e948bbd6bd80f354f037badedef6cd8cb1) )
	ROM_LOAD( "2.b2",         0x200000, 0x200000, CRC(9710bcff) SHA1(0291385489a065ed895c99ae7197fdeac0a0e2a0) )
	ROM_LOAD( "3.b3",         0x400000, 0x200000, CRC(4236c79a) SHA1(94f093d12c096d38d1e7278796f6d58e4ba14e2e) )
	ROM_LOAD( "4.b4",         0x600000, 0x200000, CRC(68153b0f) SHA1(46ddf37d5885f411e0e6de9c7e8969ba3a00f17f) )

	// samples?
	ROM_REGION( 0x100000, REGION_SOUND1, 0 )
	ROM_LOAD( "v100.u7",      0x000000, 0x100000, CRC(c134378c) SHA1(999c75f3a7890421cfd904a926ca377ee43a6825) )
ROM_END

static DRIVER_INIT(lastfght)
{
	colorram=auto_malloc(768);

	// remove comment to pass initial check (protection ? hw?)
	// memory_region(REGION_CPU1)[0x00355]=0x40;
}

GAME(2000, lastfght, 0, lastfght, lastfght, lastfght, ROT0, "Subsino", "Last Fighting", GAME_NOT_WORKING|GAME_NO_SOUND)
