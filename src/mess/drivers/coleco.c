/***************************************************************************

  coleco.c

  Driver file to handle emulation of the Colecovision.

  Marat Fayzullin (ColEm source)
  Marcel de Kogel (AdamEm source)
  Mike Balfour
  Ben Bruscella
  Sean Young

  NEWS:
    - Modified memory map, now it has only 1k of RAM mapped on 8k Slot
    - Modified I/O map, now it is handled as on a real ColecoVision:
        The I/O map is broken into 4 write and 4 read ports:
            80-9F (W) = Set both controllers to keypad mode
            80-9F (R) = Not Connected

            A0-BF (W) = Video Chip (TMS9928A), A0=0 -> Write Register 0 , A0=1 -> Write Register 1
            A0-BF (R) = Video Chip (TMS9928A), A0=0 -> Read Register 0 , A0=1 -> Read Register 1

            C0-DF (W) = Set both controllers to joystick mode
            C0-DF (R) = Not Connected

            E0-FF (W) = Sound Chip (SN76489A)
            E0-FF (R) = Read Controller data, A1=0 -> read controller 1, A1=1 -> read controller 2

    - Modified paddle handler, now it is handled as on a real ColecoVision
    - Added support for a Driving Controller (Expansion Module #2), enabled via configuration
    - Added support for a Roller Controller (Trackball), enabled via configuration
    - Added support for two Super Action Controller, enabled via configuracion

    EXTRA CONTROLLERS INFO:

        -Driving Controller (Expansion Module #2). It consist of a steering wheel and a gas pedal. Only one
         can be used on a real ColecoVision. The gas pedal is not analog, internally it is just a switch.
         On a real ColecoVision, when the Driving Controller is enabled, the controller 1 do not work because
         have been replaced by the Driving Controller, and controller 2 have to be used to start game, gear
         shift, etc.
         Driving Controller is just a spinner on controller 1 socket similar to the one on Roller Controller
         and Super Action Controllers so you can use Roller Controller or Super Action Controllers to play
         games requiring Driving Controller.

        -Roller Controller. Basically a trackball with four buttons (the two fire buttons from player 1 and
         the two fire buttons from player 2). Only one Roller Controller can be used on a real ColecoVision.
     Roller Controller is connected to both controller sockets and both controllers are conected to the Roller
     Controller, it uses the spinner pins of both sockets to generate the X and Y signals (X from controller 1
     and the Y from controller 2)

        -Super Action Controllers. It is a hand controller with a keypad, four buttons (the two from
         the player pad and two more), and a spinner. This was made primarily for two player sport games, but
         will work for every other ColecoVision game.

***************************************************************************/

#include "driver.h"
#include "sound/sn76496.h"
#include "video/tms9928a.h"
#include "includes/coleco.h"
#include "devices/cartslot.h"
#include "deprecat.h"

READ8_HANDLER(coleco_video_r)
{
    return ((offset & 0x01) ? TMS9928A_register_r(machine, 1) : TMS9928A_vram_r(machine, 0));
}

WRITE8_HANDLER(coleco_video_w)
{
    (offset & 0x01) ? TMS9928A_register_w(machine, 1, data) : TMS9928A_vram_w(machine, 0, data);
}

static ADDRESS_MAP_START( coleco_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x5fff) AM_NOP
	AM_RANGE(0x6000, 0x63ff) AM_RAM AM_MIRROR(0x1c00)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( coleco_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x80, 0x9f) AM_WRITE(coleco_paddle_toggle_off)
	AM_RANGE(0xa0, 0xbf) AM_READWRITE(coleco_video_r, coleco_video_w)
	AM_RANGE(0xc0, 0xdf) AM_WRITE(coleco_paddle_toggle_on)
	AM_RANGE(0xe0, 0xff) AM_READWRITE(coleco_paddle_r, SN76496_0_w)
ADDRESS_MAP_END

static INPUT_PORTS_START( coleco )
    PORT_START_TAG("IN0")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("0 (pad 1)") PORT_CODE(KEYCODE_0)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("1 (pad 1)") PORT_CODE(KEYCODE_1)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("2 (pad 1)") PORT_CODE(KEYCODE_2)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("3 (pad 1)") PORT_CODE(KEYCODE_3)
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("4 (pad 1)") PORT_CODE(KEYCODE_4)
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("5 (pad 1)") PORT_CODE(KEYCODE_5)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("6 (pad 1)") PORT_CODE(KEYCODE_6)
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("7 (pad 1)") PORT_CODE(KEYCODE_7)

    PORT_START_TAG("IN1")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("8 (pad 1)") PORT_CODE(KEYCODE_8)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("9 (pad 1)") PORT_CODE(KEYCODE_9)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("# (pad 1)") PORT_CODE(KEYCODE_MINUS)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(". (pad 1)") PORT_CODE(KEYCODE_EQUALS)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
    PORT_BIT( 0xb0, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START_TAG("IN2")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_BIT( 0xb0, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START_TAG("IN3")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("0 (pad 2)") PORT_CODE(KEYCODE_0_PAD)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("1 (pad 2)") PORT_CODE(KEYCODE_1_PAD)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("2 (pad 2)") PORT_CODE(KEYCODE_2_PAD)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("3 (pad 2)") PORT_CODE(KEYCODE_3_PAD)
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("4 (pad 2)") PORT_CODE(KEYCODE_4_PAD)
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("5 (pad 2)") PORT_CODE(KEYCODE_5_PAD)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("6 (pad 2)") PORT_CODE(KEYCODE_6_PAD)
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("7 (pad 2)") PORT_CODE(KEYCODE_7_PAD)

    PORT_START_TAG("IN4")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("8 (pad 2)") PORT_CODE(KEYCODE_8_PAD)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("9 (pad 2)") PORT_CODE(KEYCODE_9_PAD)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("# (pad 2)") PORT_CODE(KEYCODE_MINUS_PAD)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(". (pad 2)") PORT_CODE(KEYCODE_PLUS_PAD)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )		PORT_PLAYER(2)
    PORT_BIT( 0xb0, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START_TAG("IN5")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )	PORT_PLAYER(2)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )	PORT_PLAYER(2)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )	PORT_PLAYER(2)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )	PORT_PLAYER(2)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )		PORT_PLAYER(2)
    PORT_BIT( 0xb0, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START_TAG("IN6")
    PORT_DIPNAME( 0x07, 0x00, "Extra Controllers" )
    PORT_DIPSETTING(	0x00, DEF_STR( None ) )
    PORT_DIPSETTING(	0x01, "Driving Controller" )
    PORT_DIPSETTING(	0x02, "Roller Controller" )
    PORT_DIPSETTING(	0x04, "Super Action Controllers" )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("SAC Blue Button P1")	PORT_CODE(KEYCODE_Z)
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("SAC Purple Button P1")	PORT_CODE(KEYCODE_X)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("SAC Blue Button P2")	PORT_CODE(KEYCODE_Q) PORT_PLAYER(2)
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("SAC Purple Button P2")	PORT_CODE(KEYCODE_W) PORT_PLAYER(2)

    PORT_START_TAG("IN7")	// Extra Controls (Driving Controller, SAC P1 slider, Roller Controller X Axis)
    PORT_BIT( 0x0f, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(20) PORT_KEYDELTA(10) PORT_CODE_DEC(KEYCODE_L) PORT_CODE_INC(KEYCODE_J) PORT_RESET

    PORT_START_TAG("IN8")	// Extra Controls (SAC P2 slider, Roller Controller Y Axis)
    PORT_BIT( 0x0f, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(20) PORT_KEYDELTA(10) PORT_CODE_DEC(KEYCODE_I) PORT_CODE_INC(KEYCODE_K) PORT_RESET PORT_PLAYER(2)
INPUT_PORTS_END



/***************************************************************************

  The interrupts come from the vdp. The vdp (tms9928a) interrupt can go up
  and down; the Coleco only uses nmi interrupts (which is just a pulse). They
  are edge-triggered: as soon as the vdp interrupt line goes up, an interrupt
  is generated. Nothing happens when the line stays up or goes down.

  To emulate this correctly, we set a callback in the tms9928a (they
  can occur mid-frame). At every frame we call the TMS9928A_interrupt
  because the vdp needs to know when the end-of-frame occurs, but we don't
  return an interrupt.

***************************************************************************/

extern int JoyStat[2];

static INTERRUPT_GEN( coleco_interrupt )
{
    TMS9928A_interrupt();
}

static void coleco_vdp_interrupt (int state)
{
	static int last_state = 0;

    // only if it goes up
	if (state && !last_state) cpunum_set_input_line(Machine, 0, INPUT_LINE_NMI, PULSE_LINE);
	last_state = state;
}

static TIMER_CALLBACK(paddle_callback)
{
    int port7 = readinputportbytag("IN7");
    int port8 = readinputportbytag("IN8");

    if (port7 == 0)
		JoyStat[0] = 0;
    else if (port7 & 0x08)
		JoyStat[0] = -1;
    else
		JoyStat[0] = 1;

    if (port8 == 0)
		JoyStat[1] = 0;
    else if (port8 & 0x08)
		JoyStat[1] = -1;
    else
		JoyStat[1] = 1;

    if (JoyStat[0] || JoyStat[1])
		cpunum_set_input_line(machine, 0, 0, HOLD_LINE);
}

static const TMS9928a_interface tms9928a_interface =
{
	TMS99x8A,
	0x4000,
	0, 0,
	coleco_vdp_interrupt
};

static MACHINE_START( coleco )
{
	TMS9928A_configure(&tms9928a_interface);
}

static MACHINE_RESET(coleco)
{
    cpunum_set_input_line_vector(0, 0, 0xff);
	memset(&memory_region(REGION_CPU1)[0x6000], 0xff, 0x400);	// initialize RAM
    timer_pulse(ATTOTIME_IN_MSEC(20), NULL, 0, paddle_callback);
}

static MACHINE_DRIVER_START( coleco )
	// basic machine hardware
	MDRV_CPU_ADD(Z80, 7159090/2)	// 3.579545 MHz
	MDRV_CPU_PROGRAM_MAP(coleco_map, 0)
	MDRV_CPU_IO_MAP(coleco_io_map, 0)
	MDRV_CPU_VBLANK_INT("main", coleco_interrupt)

	MDRV_MACHINE_START(coleco)
	MDRV_MACHINE_RESET(coleco)

    // video hardware
	MDRV_IMPORT_FROM(tms9928a)
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76489A, 7159090/2)	/* 3.579545 MHz */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

ROM_START (coleco)
    ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "coleco.rom", 0x0000, 0x2000, CRC(3aa93ef3) SHA1(45bedc4cbdeac66c7df59e9e599195c778d86a92) )
	ROM_CART_LOAD(0, "rom,col,bin", 0x8000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START (colecoa)
    // differences to 0x3aa93ef3 modified characters, added a pad 2 related fix
    ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "colecoa.rom", 0x0000, 0x2000, CRC(39bb16fc) SHA1(99ba9be24ada3e86e5c17aeecb7a2d68c5edfe59) )
	ROM_CART_LOAD(0, "rom,col,bin", 0x8000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START (colecob)
    ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "svi603.rom", 0x0000, 0x2000, CRC(19e91b82) SHA1(8a30abe5ffef810b0f99b86db38b1b3c9d259b78) )
	ROM_CART_LOAD(0, "rom,col,bin", 0x8000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

static void coleco_cartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_VERIFY:						info->imgverify = coleco_cart_verify; break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(coleco)
	CONFIG_DEVICE(coleco_cartslot_getinfo)
SYSTEM_CONFIG_END

//    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT     INIT    CONFIG  COMPANY   FULLNAME
CONS( 1982, coleco,   0,		0,	coleco,   coleco,   0,		coleco,	"Coleco", "ColecoVision" , 0)
CONS( 1982, colecoa,  coleco,	0,		coleco,   coleco,   0,		coleco,	"Coleco", "ColecoVision (Thick Characters)" , 0)
CONS( 1983, colecob,  coleco,	0,		coleco,   coleco,   0,		coleco,	"Spectravideo", "SVI-603 Coleco Game Adapter" , 0)
