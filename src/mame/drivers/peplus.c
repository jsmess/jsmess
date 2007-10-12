/**********************************************************************************


    PLAYER'S EDGE PLUS (PE+)

    Driver by Jim Stolis.

    --- Technical Notes ---

    Name:    Player's Edge Plus (PP0516) Double Bonus Draw Poker.
    Company: IGT - International Gaming Technology
    Year:    1987

    Hardware:

    CPU =  83c02             ; I8052 compatible
    VIDEO = SY6545           ; CRTC6845 compatible
    SND =  AY-3-8912         ; AY8910 compatible

    History:

    This form of video poker machine has the ability to use different game roms.  The operator
    changes the game by placing the rom at U68 on the motherboard.  This driver is currently valid
    for the PP0516 game rom, but should work with all other compatible game roms as cpu, video,
    sound, and inputs is concerned.  Some games can share the same color prom and graphic roms,
    but this is not always the case.  It is best to confirm the game, color and graphic combinations.

    The game code runs in two different modes, game mode and operator mode.  Game mode is what a
    normal player would see when playing.  Operator mode is for the machine operator to adjust
    machine settings and view coin counts.  The upper door must be open in order to enter operator
    mode and so it should be mapped to an input bank if you wish to support it.  The operator
    has two additional inputs (jackpot reset and self-test) to navigate with, along with the
    normal buttons available to the player.

    A normal machine keeps all coin counts and settings in a battery-backed ram, and will
    periodically update an external eeprom for an even more secure backup.  This eeprom
    also holds the current game state in order to recover the player from a full power failure.

***********************************************************************************/
#include "driver.h"
#include "sound/ay8910.h"
#include "cpu/i8051/i8051.h"

static tilemap *bg_tilemap;

/* Pointers to External RAM */
UINT8 *program_ram;
UINT8 *settings_ram;
UINT8 *set038_ram;

/* Variables used instead of CRTC6845 system */
UINT8 vid_register = 0;
UINT8 vid_low = 0;
UINT8 vid_high = 0;

/* Holds upper video address and palette number */
UINT8 *palette_ram;

UINT8 ramcheck;
/*****************
* Write Handlers *
******************/

WRITE8_HANDLER( peplus_bgcolor_w )
{
    int i;

	for (i = 0; i < 16; i++)
	{
		int bit0, bit1, bit2, r, g, b;

		/* red component */
		bit0 = (~data >> 0) & 0x01;
		bit1 = (~data >> 1) & 0x01;
		bit2 = (~data >> 2) & 0x01;
	    r = 0x21 * bit2 + 0x47 * bit1 + 0x97 * bit0;

		/* green component */
		bit0 = (~data >> 3) & 0x01;
		bit1 = (~data >> 4) & 0x01;
		bit2 = (~data >> 5) & 0x01;
		g = 0x21 * bit2 + 0x47 * bit1 + 0x97 * bit0;

		/* blue component */
		bit0 = (~data >> 6) & 0x01;
		bit1 = (~data >> 7) & 0x01;
        bit2 = 0;
		b = 0x21 * bit2 + 0x47 * bit1 + 0x97 * bit0;

		palette_set_color(Machine, (15 + (i*16)), MAKE_RGB(r, g, b));
	}
}

/*
    SY6545 - Transparent Memory Addressing
    The current CRTC6845 driver does not support these
    additional registers (R18, R19, R31)
*/
WRITE8_HANDLER( peplus_crtc_register_w )
{
    switch(data) {
        case 0x12:  /* Update Address High */
        case 0x13:  /* Update Address Low */
        case 0x1f:  /* Dummy Location */
            vid_register = data;
            break;
    }
}

WRITE8_HANDLER( peplus_crtc_address_w )
{
    switch(vid_register) {
        case 0x12:  /* Update Address High */
            vid_high = data;
            break;
        case 0x13:  /* Update Address Low */
            vid_low = data;
            break;
    }
}

WRITE8_HANDLER( peplus_crtc_display_w )
{
    UINT16 vid_address = (vid_high<<8) | vid_low;

    /* P1 holds the following: */
    /* high nibble = palette, low nibble = high address */
    UINT8 p1 = i8051_get_intram(P1);

    videoram[vid_address] = data;
	palette_ram[vid_address] = p1;

    tilemap_mark_tile_dirty(bg_tilemap, vid_address);

    /* Transparent Memory Addressing increments the update address register */
    if (vid_low == 0xff) {
        vid_high++;
    }
    vid_low++;
}

WRITE8_HANDLER( peplus_eeprom_w )
{
    // EEPROM is a X2404P 4K-bit Serial I2C Bus
}

WRITE8_HANDLER( peplus_ramcheck_w )
{
    ramcheck = data;
}


/****************
* Read Handlers *
****************/

/* External RAM Callback for I8052 */
READ32_HANDLER( peplus_external_ram_iaddr )
{
    UINT8 p2 = i8051_get_intram(P2);

    if (offset < 0x1000) {
        return (p2 << 8) | offset;
    } else {
        return offset;
    }
}

/* Last Color in Every Palette is bgcolor */
READ8_HANDLER( peplus_bgcolor_r )
{
    return palette_get_color(Machine, 15); /* Return bgcolor from First Palette */
}


/* Insufficient Code, will need more to handle cmos and coin pulses */
READ8_HANDLER( peplus_input_bank_a_r )
{
    /*
        Bit 0 = COIN DETECTOR A
        Bit 1 = COIN DETECTOR B
        Bit 2 = COIN DETECTOR C
        Bit 3 = COIN OUT
        Bit 4 = HOPPER FULL
        Bit 5 = DOOR OPEN
        Bit 6 = LOW BATTERY
        Bit 7 = CMOS ACK (0=ACK)
    */
    return 0x77;
}

READ8_HANDLER( peplus_ramcheck_r )
{
    if (ramcheck == 0x3e)
        return 0xff;
    else
        return ramcheck;
}


/*************************
* Temporary Memory Hacks *
*************************/

/* Hack to test coins in */
READ8_HANDLER( peplus_coinsin_r )
{
    return 0x05;
}

/* Hack to test credits */
READ8_HANDLER( peplus_credits_r )
{
    return 0x20;
}

/****************************
* Video/Character functions *
****************************/

static TILE_GET_INFO( get_bg_tile_info )
{
	int pr = palette_ram[tile_index];
    int vr = videoram[tile_index];

	int code = ((pr & 0x0f)*256) | vr;
    int color = (pr>>4) & 0x0f;

	SET_TILE_INFO(0, code, color, 0);
}

VIDEO_START( peplus )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, TILEMAP_TYPE_PEN, 8, 8, 40, 25);
    palette_ram = auto_malloc(0x3000);
    memset(palette_ram, 0, 0x3000);
}

VIDEO_UPDATE( peplus )
{
    tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);

	return 0;
}

PALETTE_INIT( peplus )
{
/*  prom bits
    7654 3210
    ---- -xxx   red component.
    --xx x---   green component.
    xx-- ----   blue component.
*/
	int i;

	for (i = 0;i < machine->drv->total_colors;i++)
	{
		int bit0, bit1, bit2, r, g, b;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
	    r = 0x21 * bit2 + 0x47 * bit1 + 0x97 * bit0;

		/* green component */
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		g = 0x21 * bit2 + 0x47 * bit1 + 0x97 * bit0;

		/* blue component */
		bit0 = (color_prom[i] >> 6) & 0x01;
		bit1 = (color_prom[i] >> 7) & 0x01;
        bit2 = 0;
		b = 0x21 * bit2 + 0x47 * bit1 + 0x97 * bit0;

        /* TODO */
        /* Add code to load last color in each palette with bgcolor */
        /* Using default in color prom instead for now */

		palette_set_color(machine, i, MAKE_RGB(r, g, b));
	}
}


/*************************
*    Graphics Layouts    *
*************************/

static const gfx_layout charlayout =
{
	8,8,    /* 8x8 characters */
    0x1000, /* 4096 characters */
	4,  /* 4 bitplanes */
    { 0x1000*8*8*3, 0x1000*8*8*2, 0x1000*8*8*1, 0x1000*8*8*0 }, /* bitplane offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


/******************************
* Graphics Decode Information *
******************************/

static GFXDECODE_START( peplus )
	GFXDECODE_ENTRY( REGION_GFX1, 0x00000, charlayout, 0, 16 )
GFXDECODE_END


/***********************
*    Sound Interface   *
***********************/

static struct AY8910interface ay8910_interface =
{
	input_port_3_r  /* SW1 is accessed via sound portA */
};


/**************
* Driver Init *
***************/

static DRIVER_INIT( peplus )
{
    /* External RAM callback */
    i8051_set_eram_iaddr_callback(peplus_external_ram_iaddr);

    // Set038 Chip - For testing only, cannot stay in final driver
    //program_ram[0x302] = 0x22;  /* disable memory test */
    //program_ram[0x289f] = 0x22; /* disable program checksum */
    //set038_ram[0x00] = 0xfe;

    // PP0516 - For testing only, cannot stay in final driver
    program_ram[0x9a24] = 0x22; /* disable memory test */
    program_ram[0xd61d] = 0x22; /* disable program checksum */

    /* TODO: Machine settings and coin counts need to be loaded using save state system */
    /* below are some hardcoded examples of memory to patch */

    settings_ram[0x030] = 0x05; /* Maximum Coins - 5 */
    settings_ram[0x06b] = 0x03; /* Current Denomination - 25c */
    set038_ram[0x2a] = settings_ram[0x06b];
    settings_ram[0x197] = 0x00; /* Bill Acceptor - Disabled */
    set038_ram[0x2b] = settings_ram[0x197];
}

/*************************
* Memory map information *
*************************/

static ADDRESS_MAP_START( peplus_map, ADDRESS_SPACE_PROGRAM, 8 )
    AM_RANGE(0x0000, 0xffff) AM_ROM AM_BASE(&program_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( peplus_datamap, ADDRESS_SPACE_DATA, 8 )
    AM_RANGE(0x2080, 0x2080) AM_WRITE(peplus_crtc_register_w)
    AM_RANGE(0x2081, 0x2081) AM_WRITE(peplus_crtc_address_w)
    AM_RANGE(0x2083, 0x2083) AM_WRITE(peplus_crtc_display_w)
    AM_RANGE(0x20db, 0x20db) AM_READ(input_port_1_r) /* upper door */
    AM_RANGE(0x2100, 0x21ff) AM_RAM AM_BASE(&settings_ram)
    AM_RANGE(0x2b8d, 0x2b8d) AM_READ(input_port_2_r) /* lower door */
    //AM_RANGE(0x2b9b, 0x2b9b) AM_READ(peplus_coinsin_r) /* hack to test coins in */
    //AM_RANGE(0x2b9d, 0x2b9d) AM_READ(peplus_credits_r) /* hack to test credits */
    AM_RANGE(0x2e01, 0x2eff) AM_RAM AM_BASE(&set038_ram)
	AM_RANGE(0x4000, 0x4000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x4004, 0x4004) AM_WRITE(AY8910_write_port_0_w)
    //AM_RANGE(0x4175, 0x4175) AM_READ(peplus_ramcheck_r) AM_WRITE(peplus_ramcheck_w)

    AM_RANGE(0x6000, 0x6000) AM_READ(peplus_bgcolor_r) AM_WRITE(peplus_bgcolor_w)
    AM_RANGE(0x7000, 0x73ff) AM_RAM AM_BASE(&videoram)
    AM_RANGE(0x8000, 0x8000) AM_READ(peplus_input_bank_a_r)
    AM_RANGE(0x9000, 0x9000) AM_WRITE(peplus_eeprom_w)
    AM_RANGE(0xa000, 0xa000) AM_READ(input_port_0_r)
    AM_RANGE(0x0000, 0xffff) AM_RAM
ADDRESS_MAP_END

/*************************
*      Input ports       *
*************************/

INPUT_PORTS_START( peplus )
	PORT_START_TAG("IN0")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Jackpot Reset") PORT_CODE(KEYCODE_L)
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("Self Test") PORT_CODE(KEYCODE_K)
    PORT_BIT( 0x03, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("Hold 1") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("Hold 2") PORT_CODE(KEYCODE_X)
    PORT_BIT( 0x05, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_NAME("Hold 3") PORT_CODE(KEYCODE_C)
    PORT_BIT( 0x06, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_NAME("Hold 4") PORT_CODE(KEYCODE_V)
    PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME("Hold 5") PORT_CODE(KEYCODE_B)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON9 ) PORT_NAME("Deal-Spin-Start") PORT_CODE(KEYCODE_Q)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON10 ) PORT_NAME("Max Bet") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x30, IP_ACTIVE_HIGH, IPT_UNKNOWN )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON12 ) PORT_NAME("Play Credit") PORT_CODE(KEYCODE_R)
	PORT_BIT( 0x50, IP_ACTIVE_HIGH, IPT_BUTTON13 ) PORT_NAME("Cashout") PORT_CODE(KEYCODE_T)
	PORT_BIT( 0x60, IP_ACTIVE_HIGH, IPT_BUTTON14 ) PORT_NAME("Change Request") PORT_CODE(KEYCODE_Y)
    PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_BUTTON15 ) PORT_NAME("Bill Acceptor") PORT_CODE(KEYCODE_U)
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START_TAG("IN1")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Upper Door") PORT_CODE(KEYCODE_O)

    PORT_START_TAG("IN2")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Lower Door") PORT_CODE(KEYCODE_I)

    PORT_START_TAG("SW1")
	PORT_DIPNAME( 0x01, 0x01, "0-60HZ, 1-50HZ" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


/*************************
*     Machine Driver     *
*************************/

static MACHINE_DRIVER_START( peplus )
	// basic machine hardware
    MDRV_CPU_ADD(I8052, 3686400*2)
	MDRV_CPU_PROGRAM_MAP(peplus_map, 0)
    MDRV_CPU_DATA_MAP(peplus_datamap, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE((52+1)*8, (31+1)*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 40*8-1, 0*8, 25*8-1)

	MDRV_GFXDECODE(peplus)
	MDRV_PALETTE_LENGTH(16*16)

	MDRV_PALETTE_INIT(peplus)
	MDRV_VIDEO_START(peplus)
	MDRV_VIDEO_UPDATE(peplus)

	// sound hardware
    MDRV_SOUND_ADD(AY8912, 20000000/12)
	MDRV_SPEAKER_STANDARD_MONO("mono")
    MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)
MACHINE_DRIVER_END


/*************************
*        Rom Load        *
*************************/

ROM_START( peplus )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "pp0516.u68",   0x00000, 0x10000, CRC(d9da6e13) SHA1(421678d9cb42daaf5b21074cc3900db914dd26cf) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mro-cg740.u72",	 0x00000, 0x8000, CRC(72667f6c) SHA1(89843f472cc0329317cfc643c63bdfd11234b194) )
    ROM_LOAD( "mgo-cg740.u73",	 0x08000, 0x8000, CRC(7437254a) SHA1(bba166dece8af58da217796f81117d0b05752b87) )
    ROM_LOAD( "mbo-cg740.u74",	 0x10000, 0x8000, CRC(92e8c33e) SHA1(05344664d6fdd3f4205c50fa4ca76fc46c18cf8f) )
    ROM_LOAD( "mxo-cg740.u75",	 0x18000, 0x8000, CRC(ce4cbe0b) SHA1(4bafcd68be94a5deaae9661584fa0fc940b834bb) )

	ROM_REGION( 0x100, REGION_PROMS, 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(9cd834bd) SHA1(ad66099d8ed8f784fb20b97df7c9dda51b19ff3f) )
ROM_END


/*************************
*      Game Drivers      *
*************************/

/*    YEAR  NAME      PARENT  MACHINE   INPUT     INIT      ROT    COMPANY                                  FULLNAME                                             FLAGS   */
GAME( 1987, peplus,   0,      peplus,   peplus,   peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0516) Double Bonus Poker",    0 )
