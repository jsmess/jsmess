/***************************************************************************
   
        JR-100 National / Panasonic

        23/08/2010 Initial driver version

****************************************************************************/

#include "emu.h"
#include "cpu/m6800/m6800.h"
#include "machine/6522via.h"
#include "devices/cassette.h"
#include "sound/wave.h"
#include "sound/beep.h"

static UINT8 *jr100_vram;
static UINT8 *jr100_pcg;
static UINT8 keyboard_line;
static bool jr100_use_pcg;

static ADDRESS_MAP_START(jr100_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_RAM
	AM_RANGE(0xc000, 0xc0ff) AM_RAM AM_BASE(&jr100_pcg)
	AM_RANGE(0xc100, 0xc3ff) AM_RAM AM_BASE(&jr100_vram)
	AM_RANGE(0xc800, 0xc80f) AM_DEVREADWRITE("via", via_r, via_w)
	AM_RANGE(0xe000, 0xffff) AM_ROM	
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( jr100 )
	PORT_START("LINE0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) // Z
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) // X
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) // C
	PORT_BIT(0xE0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START("LINE1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) // A
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) // S
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) // D
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) // F
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) // G
	PORT_BIT(0xE0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START("LINE2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) // Q
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) // W
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) // E
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) // R
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) // T
	PORT_BIT(0xE0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START("LINE3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) // 1
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) // 2
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) // 3
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) // 4
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) // 5
	PORT_BIT(0xE0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START("LINE4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) // 6
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) // 7
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) // 8
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) // 9
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) // 0
	PORT_BIT(0xE0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START("LINE5")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) // Y
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) // U
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) // I
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) // O
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) // P
	PORT_BIT(0xE0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START("LINE6")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) // H
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) // J
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) // K
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) // L
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) // ;
	PORT_BIT(0xE0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START("LINE7")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) // V
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) // B
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) // N
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) // M
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) // ,
	PORT_BIT(0xE0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START("LINE8")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP) // .
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE) // space
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE) // :
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER) // enter
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS) // -
	PORT_BIT(0xE0, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END


static MACHINE_RESET(jr100) 
{	
}

static VIDEO_START( jr100 )
{
}

static VIDEO_UPDATE( jr100 )
{
	int x,y,xi,yi;

	UINT8 *rom_pcg = memory_region(screen->machine, "maincpu") + 0xe000;
	for (y = 0; y < 24; y++)
	{
		for (x = 0; x < 32; x++)
		{
			UINT8 tile = jr100_vram[x + y*32];
			UINT8 attr = tile >> 7;
			// ATTR is inverted for normal char or use PCG in case of CMODE1
			UINT8 *gfx_data = rom_pcg;
			if (jr100_use_pcg && attr) {
				gfx_data = jr100_pcg;
				attr = 0; // clear attr so bellow code stay same
			}
			tile &= 0x7f;
			for(yi=0;yi<8;yi++)
			{
				for(xi=0;xi<8;xi++)
				{
					UINT8 pen = (gfx_data[(tile*8)+yi]>>(7-xi) & 1);
					*BITMAP_ADDR16(bitmap, y*8+yi, x*8+xi) = attr ^ pen;
				}
			}
		}
	}

	return 0;
}

static const gfx_layout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static GFXDECODE_START( jr100 )
	GFXDECODE_ENTRY( "maincpu", 0xe000, tiles8x8_layout, 0, 1 )
GFXDECODE_END

static const char *const keynames[] = {
	"LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7",
	"LINE8", NULL, NULL, NULL, NULL, NULL, NULL, NULL
};


static READ8_DEVICE_HANDLER(jr100_via_read_b)
{
	UINT8 val = 0x1f;
	if (keynames[keyboard_line]) {
		val = input_port_read(device->machine, keynames[keyboard_line]);
	}
	return val;
}

static WRITE8_DEVICE_HANDLER(jr100_via_write_a )
{
	keyboard_line = data & 0x0f;
}

static WRITE8_DEVICE_HANDLER(jr100_via_write_b )
{
	jr100_use_pcg = (data & 0x20) ? TRUE : FALSE;
}

static const via6522_interface jr100_via_intf =
{
	DEVCB_NULL,											/* in_a_func */
	DEVCB_HANDLER(jr100_via_read_b),					/* in_b_func */
	DEVCB_NULL,     									/* in_ca1_func */
	DEVCB_NULL,      									/* in_cb1_func */
	DEVCB_NULL,      									/* in_ca2_func */
	DEVCB_NULL,      									/* in_cb2_func */
	DEVCB_HANDLER(jr100_via_write_a),					/* out_a_func */
	DEVCB_HANDLER(jr100_via_write_b),   				/* out_b_func */
	DEVCB_NULL,      									/* out_ca1_func */
	DEVCB_NULL,      									/* out_cb1_func */
	DEVCB_NULL,      									/* out_ca2_func */
	DEVCB_NULL,      									/* out_cb2_func */
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0)    /* irq_func */	
};
static const cassette_config jr100_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_SPEAKER_ENABLED | CASSETTE_MOTOR_ENABLED),
	NULL
};

static TIMER_DEVICE_CALLBACK( cassette_tick )
{

}

static MACHINE_DRIVER_START( jr100 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M6802, XTAL_14_31818MHz / 16)
    MDRV_CPU_PROGRAM_MAP(jr100_mem)

    MDRV_MACHINE_RESET(jr100)
	
    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 192) /* border size not accurate */
	MDRV_SCREEN_VISIBLE_AREA(0, 256 - 1, 0, 192 - 1)

	MDRV_GFXDECODE(jr100)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(jr100)
    MDRV_VIDEO_UPDATE(jr100)
	
	MDRV_VIA6522_ADD("via", XTAL_14_31818MHz / 16, jr100_via_intf)
	
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_CASSETTE_ADD( "cassette", jr100_cassette_config )	
	MDRV_TIMER_ADD_PERIODIC("cassette_timer", cassette_tick, HZ(XTAL_14_31818MHz / 16))
MACHINE_DRIVER_END


/* ROM definition */
ROM_START( jr100u )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "jr100.rom", 0xe000, 0x2000, CRC(f589dd8d) SHA1(78a51f2ae055bf4dc1b0887a6277f5dbbd8ba512))
ROM_END

/* Driver */

/*   YEAR  NAME    PARENT  COMPAT   MACHINE  INPUT  INIT  		COMPANY   FULLNAME       FLAGS */
//COMP( 1981, jr100,  0,       0, 	jr100, 	jr100, 	 0,  	  "National",   "JR-100",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1981, jr100u,  0,       0, 	jr100, 	jr100, 	 0,  	  "Panasonic",   "JR-100U",		GAME_NOT_WORKING | GAME_NO_SOUND)

