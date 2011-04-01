/***************************************************************************

        SM1800 (original name is CM1800 in cyrilic letters)

        10/12/2010 Skeleton driver.

        On board hardware :
            KR580VM80A central processing unit (i8080)
            KR580VG75  programmable CRT video display controller (i8275)
            KR580VV55  programmable parallel interface (i8255)
            KR580IK51  programmable serial interface/communication controller (i8251)
            KR580VV79  programmable peripheral device, keyboard and display controller (i8279)

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "machine/i8255a.h"
#include "machine/msm8251.h"
#include "video/i8275.h"

class sm1800_state : public driver_device
{
public:
	sm1800_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_irq_state;
};

static ADDRESS_MAP_START(sm1800_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x07ff ) AM_ROM
	//AM_RANGE( 0x0fb0, 0x0fff ) AM_DEVWRITE("i8275", i8275_dack_w)
	AM_RANGE( 0x1000, 0x17ff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sm1800_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x3c, 0x3d ) AM_DEVREADWRITE("i8275", i8275_r, i8275_w)
	AM_RANGE( 0x6c, 0x6f ) AM_DEVREADWRITE("i8255", i8255a_r, i8255a_w)
	//AM_RANGE( 0x74, 0x75 ) AM_DEVREADWRITE("i8279", i8279_r, i8279_w)
	AM_RANGE( 0x5c, 0x5c) AM_DEVREADWRITE("i8251", msm8251_data_r,msm8251_data_w)
	AM_RANGE( 0x5d, 0x5d) AM_DEVREADWRITE("i8251", msm8251_status_r,msm8251_control_w)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( sm1800 )
INPUT_PORTS_END

static IRQ_CALLBACK(sm1800_irq_callback)
{
	return 0xff;
}

static MACHINE_RESET(sm1800)
{
	device_set_irq_callback(machine.device("maincpu"), sm1800_irq_callback);
}


static SCREEN_UPDATE( sm1800 )
{
	device_t *devconf = screen->machine().device("i8275");
	i8275_update( devconf, bitmap, cliprect);
	SCREEN_UPDATE_CALL ( generic_bitmapped );
	return 0;
}

static INTERRUPT_GEN( sm1800_vblank_interrupt )
{
	sm1800_state *state = device->machine().driver_data<sm1800_state>();
	cputag_set_input_line(device->machine(), "maincpu", 0, state->m_irq_state ?  HOLD_LINE : CLEAR_LINE);
	state->m_irq_state ^= 1;
}

static I8275_DISPLAY_PIXELS(sm1800_display_pixels)
{
	int i;
	bitmap_t *bitmap = device->machine().generic.tmpbitmap;
	UINT8 *charmap = device->machine().region("gfx1")->base();
	UINT8 pixels = charmap[(linecount & 7) + (charcode << 3)] ^ 0xff;
	if (vsp) {
		pixels = 0;
	}
	if (lten) {
		pixels = 0xff;
	}
	if (rvv) {
		pixels ^= 0xff;
	}
	for(i=0;i<8;i++) {
		*BITMAP_ADDR16(bitmap, y, x + i) = (pixels >> (7-i)) & 1 ? (hlgt ? 2 : 1) : 0;
	}
}
const i8275_interface sm1800_i8275_interface = {
	"screen",
	8,
	0,
	DEVCB_NULL,
	DEVCB_NULL,
	sm1800_display_pixels
};


static WRITE8_DEVICE_HANDLER (sm1800_8255_portb_w )
{
}

static WRITE8_DEVICE_HANDLER (sm1800_8255_portc_w )
{
}

static READ8_DEVICE_HANDLER (sm1800_8255_porta_r )
{
	return 0xff;
}

static READ8_DEVICE_HANDLER (sm1800_8255_portc_r )
{
	return 0;
}

I8255A_INTERFACE( sm1800_ppi8255_interface )
{
	DEVCB_HANDLER(sm1800_8255_porta_r),
	DEVCB_NULL,
	DEVCB_HANDLER(sm1800_8255_portc_r),
	DEVCB_NULL,
	DEVCB_HANDLER(sm1800_8255_portb_w),
	DEVCB_HANDLER(sm1800_8255_portc_w)
};

static const rgb_t sm1800_palette[3] = {
	MAKE_RGB(0x00, 0x00, 0x00), // black
	MAKE_RGB(0xa0, 0xa0, 0xa0), // white
	MAKE_RGB(0xff, 0xff, 0xff)	// highlight
};

static PALETTE_INIT( sm1800 )
{
	palette_set_colors(machine, 0, sm1800_palette, ARRAY_LENGTH(sm1800_palette));
}

static MACHINE_CONFIG_START( sm1800, sm1800_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",I8080, XTAL_2MHz)
    MCFG_CPU_PROGRAM_MAP(sm1800_mem)
    MCFG_CPU_IO_MAP(sm1800_io)
	MCFG_CPU_VBLANK_INT("screen", sm1800_vblank_interrupt)

    MCFG_MACHINE_RESET(sm1800)

	MCFG_I8255A_ADD ("i8255", sm1800_ppi8255_interface )
	MCFG_I8275_ADD	("i8275", sm1800_i8275_interface)
	MCFG_MSM8251_ADD("i8251", default_msm8251_interface)
    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 480)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MCFG_SCREEN_UPDATE(sm1800)

    MCFG_PALETTE_LENGTH(3)
    MCFG_PALETTE_INIT(sm1800)

    MCFG_VIDEO_START(generic_bitmapped)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( sm1800 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "prog.bin", 0x0000, 0x0800, CRC(55736ad5) SHA1(b77f720f1b64b208dd6a5d4f9c9521d1284028e9))
	ROM_REGION(0x0800, "gfx1",0)
	ROM_LOAD( "font.bin", 0x0000, 0x0800, CRC(28ed9ebc) SHA1(f561136962a06a5dcb5a0436931d29e940155d24))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( ????, sm1800,  0,       0,	sm1800, 	sm1800, 	 0, 	  "<unknown>",   "SM1800",		GAME_NOT_WORKING | GAME_NO_SOUND)

