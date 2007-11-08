/**********************************************************************************


    PLAYER'S EDGE PLUS (PE+)

    Driver by Jim Stolis.

    --- Technical Notes ---

    Name:    Player's Edge Plus (PP0516) Double Bonus Draw Poker.
    Company: IGT - International Gaming Technology
    Year:    1987

    Hardware:

    CPU =  INTEL 83c02       ; I8052 compatible
    VIDEO = ROCKWELL 6545    ; CRTC6845 compatible
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

#include "peplus.lh"
#include "pepp0188.lh"
#include "peset038.lh"

static tilemap *bg_tilemap;

/* Pointers to External RAM */
static UINT8 *program_ram;
static UINT8 *cmos_ram;

/* Variables used instead of CRTC6845 system */
static UINT8 vid_register = 0;
static UINT8 vid_low = 0;
static UINT8 vid_high = 0;

/* Holds upper video address and palette number */
static UINT8 *palette_ram;

/* IO Ports */
static UINT8 *io_port;


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
    ROCKWELL 6545 - Transparent Memory Addressing
    The current CRTC6845 driver does not support these
    additional registers (R18, R19, R31)
*/
WRITE8_HANDLER( peplus_crtc_mode_w )
{
    /* Mode Control - Register 8 */
    /* Sets CRT to Transparent Memory Addressing Mode */
}

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

    videoram[vid_address] = data;
	palette_ram[vid_address] = io_port[1];

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

WRITE8_HANDLER( peplus_io_w )
{
    io_port[offset] = data;
}

WRITE8_HANDLER( peplus_duart_w )
{
    // Used for Progressive Jackpot Communication
}

WRITE8_HANDLER( peplus_cmos_w )
{
    cmos_ram[offset] = data;
}

WRITE8_HANDLER( peplus_output_bank_a_w )
{
    output_set_value("coinlockout",(data >> 0) & 1); /* Coin Lockout */
    output_set_value("diverter",(data >> 1) & 1); /* Diverter */
    output_set_value("bell",(data >> 2) & 1); /* Bell */
    output_set_value("na1",(data >> 3) & 1); /* N/A */
    output_set_value("hopper1",(data >> 4) & 1); /* Hopper 1 */
    output_set_value("hopper2",(data >> 5) & 1); /* Hopper 2 */
    output_set_value("na2",(data >> 6) & 1); /* N/A */
    output_set_value("na3",(data >> 7) & 1); /* N/A */
}

WRITE8_HANDLER( peplus_output_bank_b_w )
{
    output_set_lamp_value(0,(data >> 0) & 1); /* Hold 2 3 4 Lt */
    output_set_lamp_value(1,(data >> 1) & 1); /* Deal Draw Lt */
    output_set_lamp_value(2,(data >> 2) & 1); /* Cash Out Lt */
    output_set_lamp_value(3,(data >> 3) & 1); /* Hold 1 Lt */
    output_set_lamp_value(4,(data >> 4) & 1); /* Bet Credits Lt */
    output_set_lamp_value(5,(data >> 5) & 1); /* Change Request Lt */
    output_set_lamp_value(6,(data >> 6) & 1); /* Door Open Lt */
    output_set_lamp_value(7,(data >> 7) & 1); /* Hold 5 Lt */
}

WRITE8_HANDLER( peplus_output_bank_c_w )
{
    output_set_value("coininmeter",(data >> 0) & 1); /* Coin In Meter */
    output_set_value("coinoutmeter",(data >> 1) & 1); /* Coin Out Meter */
    output_set_value("coindropmeter",(data >> 2) & 1); /* Coin Drop Meter */
    output_set_value("jackpotmeter",(data >> 3) & 1); /* Jackpot Meter */
    output_set_value("billacceptor",(data >> 4) & 1); /* Bill Acceptor Enabled */
    output_set_value("sdsout",(data >> 5) & 1); /* SDS Out */
    output_set_value("na4",(data >> 6) & 1); /* N/A */
    output_set_value("gamemeter",(data >> 7) & 1); /* Game Meter */
}


/****************
* Read Handlers *
****************/

READ8_HANDLER( peplus_duart_r )
{
    // Used for Progressive Jackpot Communication
    return 0x00;
}

READ8_HANDLER( peplus_cmos_r )
{
    switch (offset)
    {
        case 0x0db:
            if ((readinputportbytag("DOOR") & 0x01) == 1) {
                cmos_ram[offset] = 0x01;
            }
        case 0xb8d:
            if ((readinputportbytag("DOOR") & 0x02) == 1)
                cmos_ram[offset] = 0x01;
    }

    // Fake A Bunch of Coins In Hack
    if ((readinputportbytag("SENSOR") & 0x01) == 1) {
        cmos_ram[0x01E] = 0x01;
    }

    return cmos_ram[offset];
}

/* External RAM Callback for I8052 */
READ32_HANDLER( peplus_external_ram_iaddr )
{
    if (mem_mask == 0xff) {
        return (io_port[2] << 8) | offset;
    } else {
        return offset;
    }
}

/* Last Color in Every Palette is bgcolor */
READ8_HANDLER( peplus_bgcolor_r )
{
    return palette_get_color(Machine, 15); /* Return bgcolor from First Palette */
}

READ8_HANDLER( peplus_dropdoor_r )
{
    return 0x00; /* Drop Door 0x00=Closed 0x02=Open */
}

READ8_HANDLER( peplus_c000_r )
{
    return 0xff; /* Unknown */
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
    UINT8 tempbank = 0x00;
    UINT8 coindetect = 0x00;
    if ((readinputportbytag("SENSOR") & 0x01) == 0x01) {
        coindetect = 0x07;
    }

    tempbank = (0x57 | (!(readinputportbytag("DOOR") & 0x01)<<5)) ^ (coindetect);
    return tempbank;
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
		bit0 = (~color_prom[i] >> 0) & 0x01;
		bit1 = (~color_prom[i] >> 1) & 0x01;
		bit2 = (~color_prom[i] >> 2) & 0x01;
	    r = 0x21 * bit2 + 0x47 * bit1 + 0x97 * bit0;

		/* green component */
		bit0 = (~color_prom[i] >> 3) & 0x01;
		bit1 = (~color_prom[i] >> 4) & 0x01;
		bit2 = (~color_prom[i] >> 5) & 0x01;
		g = 0x21 * bit2 + 0x47 * bit1 + 0x97 * bit0;

		/* blue component */
		bit0 = (~color_prom[i] >> 6) & 0x01;
		bit1 = (~color_prom[i] >> 7) & 0x01;
        bit2 = 0;
		b = 0x21 * bit2 + 0x47 * bit1 + 0x97 * bit0;

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


/**************
* Driver Init *
***************/

static DRIVER_INIT( peplus )
{
    /* External RAM callback */
    i8051_set_eram_iaddr_callback(peplus_external_ram_iaddr);

    // For testing only, cannot stay in final driver
    program_ram[0x990e] = 0xd3; /* SETB C - disable game test */
    program_ram[0x990f] = 0x22; /* RET - disable game test */

    program_ram[0xaf91] = 0x79; /* MOV R1, - disable I2C random read */
    program_ram[0xaf92] = 0x00; /* #000H - disable I2C random read */
    program_ram[0xaf93] = 0x22; /* RET - disable I2C random read */

    program_ram[0xd61d] = 0x22; /* RET - disable program checksum */

    program_ram[0x5e7e] = 0x01; /* Allow Autohold Feature */

    /* TODO: Machine settings and coin counts need to be loaded using eeprom data */
    /* below are some hardcoded examples of memory to patch */

    cmos_ram[0x129] = 0x08; /* Additional Operator Screens Enabled */
    cmos_ram[0x130] = 0x05; /* Maximum Coins In */
    cmos_ram[0x148] = 0xff; /* Maximum Payout Low Byte */
    cmos_ram[0x149] = 0xff; /* Maximum Payout High Byte */
    cmos_ram[0x14a] = 0xff; /* Maximum Cashout Low Byte */
    cmos_ram[0x14b] = 0xff; /* Maximum Cashout High Byte */
    cmos_ram[0x16b] = 0x03; /* Current Denomination = 25c */
    cmos_ram[0x16e] = 0x40; /* Background Color */
    cmos_ram[0x183] = 0x01; /* 0=Tones, 1=Music */
    cmos_ram[0x188] = 0x7d; /* Game Type */
    cmos_ram[0x248] = 0x01; /* SAS Mini System Address */
    cmos_ram[0x284] = 0x55; /* 0=Disabled, 0x55=Drop Door */
    cmos_ram[0x297] = 0x00; /* 0=Disabled, 0xa5=Bill Acceptor */
    cmos_ram[0xb8f] = 0x01; /* 0=No Double Up, 1=Double Up */
    cmos_ram[0xb90] = 0x01; /* 0=No Animation, 1=Animation */
    cmos_ram[0xb91] = 0x00; /* 0=No Paytable, 1=Paytable */
    cmos_ram[0xb93] = 0x01; /* 0=Disabled, 1=Autohold */
    cmos_ram[0xb94] = 0x04; /* 0-9=Deal Speed */
    cmos_ram[0xbd8] = 0xaa; /* Communication Type 0xaa=None, 0x5a=sas, 0xa5=miser */

    /* Mirrored in cmos scratch area to eeprom */
    cmos_ram[0xe01] = cmos_ram[0x188]; /* Game Type */
    cmos_ram[0xe2a] = cmos_ram[0x16b]; /* Current Denomination */
    cmos_ram[0xe2b] = cmos_ram[0x297]; /* Bill Acceptor */
    cmos_ram[0xe60] = cmos_ram[0x130]; /* Maximum Coins In */
    cmos_ram[0xe64] = cmos_ram[0x16e]; /* Background Color */
}

static DRIVER_INIT( pepp0188 )
{
    /* External RAM callback */
    i8051_set_eram_iaddr_callback(peplus_external_ram_iaddr);

    // For testing only, cannot stay in final driver
    program_ram[0x994b] = 0xd3; /* SETB C - disable game test */
    program_ram[0x994c] = 0x22; /* RET - disable game test */

    program_ram[0x9b3e] = 0x79; /* MOV R1, - disable I2C random read */
    program_ram[0x9b3f] = 0x00; /* #000H - disable I2C random read */
    program_ram[0x9b40] = 0x22; /* RET - disable I2C random read */

    program_ram[0xf429] = 0x22; /* RET - disable program checksum */

    program_ram[0x742f] = 0x01; /* Allow Autohold Feature */

    /* TODO: Machine settings and coin counts need to be loaded using eeprom data */
    /* below are some hardcoded examples of memory to patch */

    cmos_ram[0x12d] = 0x08; /* Additional Operator Screens Enabled */
    cmos_ram[0x134] = 0x05; /* Maximum Coins In */
    cmos_ram[0x14c] = 0xff; /* Maximum Payout Low Byte */
    cmos_ram[0x14d] = 0xff; /* Maximum Payout High Byte */
    cmos_ram[0x14e] = 0xff; /* Maximum Cashout Low Byte */
    cmos_ram[0x14f] = 0xff; /* Maximum Cashout High Byte */
    cmos_ram[0x16f] = 0x03; /* Current Denomination = 25c */
    cmos_ram[0x172] = 0x40; /* Background Color */
    cmos_ram[0x187] = 0x01; /* 0=Tones, 1=Music */
    cmos_ram[0x18c] = 0x85; /* Game Type */
    cmos_ram[0x22e] = 0x01; /* SAS Mini System Address */
    cmos_ram[0xb3a] = 0x55; /* 0=Disabled, 0x55=Drop Door */
    cmos_ram[0x289] = 0x00; /* 0=Disabled, 0xa5=Bill Acceptor */
    cmos_ram[0xaf6] = 0x01; /* 0=No Double Up, 1=Double Up */
    cmos_ram[0xaf7] = 0x01; /* 0=No Animation, 1=Animation */
    cmos_ram[0xaf8] = 0x00; /* 0=No Paytable, 1=Paytable */
    cmos_ram[0xafa] = 0x01; /* 0=Disabled, 1=Autohold */
    cmos_ram[0xafb] = 0x04; /* 0-9=Deal Speed */
    cmos_ram[0xb51] = 0xaa; /* Communication Type 0xaa=None, 0x5a=sas, 0xa5=miser */
    cmos_ram[0xbf5] = cmos_ram[0x134]; /* Maximum Coins In */

    /* Mirrored in cmos scratch area to eeprom */
    cmos_ram[0xe01] = cmos_ram[0x18c]; /* Game Type */
    cmos_ram[0xe2a] = cmos_ram[0x16f]; /* Current Denomination */
    cmos_ram[0xe2b] = cmos_ram[0x289]; /* Bill Acceptor */
    cmos_ram[0xe60] = cmos_ram[0x134]; /* Maximum Coins In */
    cmos_ram[0xe64] = cmos_ram[0x172]; /* Background Color */
}

static DRIVER_INIT( peset038 )
{
    /* External RAM callback */
    i8051_set_eram_iaddr_callback(peplus_external_ram_iaddr);

    // For testing only, cannot stay in final driver
    program_ram[0x302] = 0x22;  /* RET - disable memory test */
    program_ram[0x3b5] = 0x22;  /* RET - disable I2C random read */
    program_ram[0x289f] = 0x22; /* RET - disable program checksum */

    cmos_ram[0xe01] = 0x7d; /* Game Type */
}

/*************************
* Memory map information *
*************************/

static ADDRESS_MAP_START( peplus_map, ADDRESS_SPACE_PROGRAM, 8 )
    AM_RANGE(0x0000, 0xffff) AM_ROM AM_BASE(&program_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( peplus_datamap, ADDRESS_SPACE_DATA, 8 )
    // Battery-backed RAM
    AM_RANGE(0x0000, 0x0fff) AM_RAM AM_READWRITE(peplus_cmos_r, peplus_cmos_w) AM_BASE(&cmos_ram)

    // CRT Controller
    AM_RANGE(0x2008, 0x2008) AM_WRITE(peplus_crtc_mode_w)
    AM_RANGE(0x2080, 0x2080) AM_WRITE(peplus_crtc_register_w)
    AM_RANGE(0x2081, 0x2081) AM_WRITE(peplus_crtc_address_w)
    AM_RANGE(0x2083, 0x2083) AM_WRITE(peplus_crtc_display_w)

    // Sound and Dipswitches
	AM_RANGE(0x4000, 0x4000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x4004, 0x4004) AM_READ(input_port_3_r) AM_WRITE(AY8910_write_port_0_w)

    // Background Color Latch
    AM_RANGE(0x6000, 0x6000) AM_READ(peplus_bgcolor_r) AM_WRITE(peplus_bgcolor_w)

    // Video RAM
    AM_RANGE(0x7000, 0x73ff) AM_RAM AM_BASE(&videoram)

    // Input Bank A, Output Bank C
    AM_RANGE(0x8000, 0x8000) AM_READ(peplus_input_bank_a_r) AM_WRITE(peplus_output_bank_c_w)

    // Drop Door, I2C EEPROM Writes
    AM_RANGE(0x9000, 0x9000) AM_READ(peplus_dropdoor_r) AM_WRITE(peplus_eeprom_w)

    // Input Banks B & C, Output Bank B
    AM_RANGE(0xa000, 0xa000) AM_READ(input_port_0_r) AM_WRITE(peplus_output_bank_b_w)

    // Output Bank A
    AM_RANGE(0xc000, 0xc000) AM_READ(peplus_c000_r) AM_WRITE(peplus_output_bank_a_w)

    // DUART
    AM_RANGE(0xe000, 0xe005) AM_READWRITE(peplus_duart_r, peplus_duart_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( peplus_iomap, ADDRESS_SPACE_IO, 8 )
    ADDRESS_MAP_FLAGS(AMEF_ABITS(8))

    // I/O Ports
	AM_RANGE(0x00, 0x03) AM_WRITE(peplus_io_w) AM_BASE(&io_port)
ADDRESS_MAP_END

/*************************
*      Input ports       *
*************************/

static INPUT_PORTS_START( peplus )
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

    PORT_START_TAG("DOOR")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Upper Door") PORT_CODE(KEYCODE_O)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Lower Door") PORT_CODE(KEYCODE_I)

    PORT_START_TAG("SENSOR")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Coin In") PORT_CODE(KEYCODE_M)

    PORT_START_TAG("SW1")
	PORT_DIPNAME( 0x01, 0x01, "Line Frequency" )
	PORT_DIPSETTING(    0x01, "60HZ" )
	PORT_DIPSETTING(    0x00, "50HZ" )
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

static INPUT_PORTS_START( pepp0188 )
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

    PORT_START_TAG("DOOR")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Upper Door") PORT_CODE(KEYCODE_O)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Lower Door") PORT_CODE(KEYCODE_I)

    PORT_START_TAG("SENSOR")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Coin In") PORT_CODE(KEYCODE_M)

    PORT_START_TAG("SW1")
	PORT_DIPNAME( 0x01, 0x01, "Line Frequency" )
	PORT_DIPSETTING(    0x01, "60HZ" )
	PORT_DIPSETTING(    0x00, "50HZ" )
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

static INPUT_PORTS_START( peset038 )
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

    PORT_START_TAG("DOOR")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Upper Door") PORT_CODE(KEYCODE_O)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Lower Door") PORT_CODE(KEYCODE_I)

    PORT_START_TAG("SENSOR")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Coin In") PORT_CODE(KEYCODE_M)

    PORT_START_TAG("SW1")
	PORT_DIPNAME( 0x01, 0x01, "Line Frequency" )
	PORT_DIPSETTING(    0x01, "60HZ" )
	PORT_DIPSETTING(    0x00, "50HZ" )
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
    MDRV_CPU_ADD_TAG("main", I8052, 3686400*2)
	MDRV_CPU_PROGRAM_MAP(peplus_map, 0)
    MDRV_CPU_DATA_MAP(peplus_datamap, 0)
    MDRV_CPU_IO_MAP(peplus_iomap, 0)
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
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pepp0188 )
	MDRV_IMPORT_FROM(peplus)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( peset038 )
	MDRV_IMPORT_FROM(peplus)
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
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) )
ROM_END

ROM_START( pepp0188 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "pp0188.u68",   0x00000, 0x10000, CRC(cf36a53c) SHA1(99b578538ab24d9ff91971b1f77599272d1dbfc6) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mro-cg740.u72",	 0x00000, 0x8000, CRC(72667f6c) SHA1(89843f472cc0329317cfc643c63bdfd11234b194) )
    ROM_LOAD( "mgo-cg740.u73",	 0x08000, 0x8000, CRC(7437254a) SHA1(bba166dece8af58da217796f81117d0b05752b87) )
    ROM_LOAD( "mbo-cg740.u74",	 0x10000, 0x8000, CRC(92e8c33e) SHA1(05344664d6fdd3f4205c50fa4ca76fc46c18cf8f) )
    ROM_LOAD( "mxo-cg740.u75",	 0x18000, 0x8000, CRC(ce4cbe0b) SHA1(4bafcd68be94a5deaae9661584fa0fc940b834bb) )

	ROM_REGION( 0x100, REGION_PROMS, 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) )
ROM_END

ROM_START( peset038 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "set038.u68",   0x00000, 0x10000, CRC(9c4b1d1a) SHA1(8a65cd1d8e2d74c7b66f4dfc73e7afca8458e979) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mro-cg740.u72",	 0x00000, 0x8000, CRC(72667f6c) SHA1(89843f472cc0329317cfc643c63bdfd11234b194) )
    ROM_LOAD( "mgo-cg740.u73",	 0x08000, 0x8000, CRC(7437254a) SHA1(bba166dece8af58da217796f81117d0b05752b87) )
    ROM_LOAD( "mbo-cg740.u74",	 0x10000, 0x8000, CRC(92e8c33e) SHA1(05344664d6fdd3f4205c50fa4ca76fc46c18cf8f) )
    ROM_LOAD( "mxo-cg740.u75",	 0x18000, 0x8000, CRC(ce4cbe0b) SHA1(4bafcd68be94a5deaae9661584fa0fc940b834bb) )

	ROM_REGION( 0x100, REGION_PROMS, 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) )
ROM_END


/*************************
*      Game Drivers      *
*************************/

/*    YEAR  NAME      PARENT  MACHINE   INPUT     INIT      ROT    COMPANY                                  FULLNAME                                             FLAGS   */
GAME( 1987, peplus,   0,      peplus,   peplus,   peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0516) Double Bonus Poker",    0 )
GAME( 1987, pepp0188, 0,      pepp0188, pepp0188, pepp0188, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0188) Standard Draw Poker",   0 )
GAME( 1987, peset038, 0,      peset038, peset038, peset038, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (Set038) Set Chip",              0 )
