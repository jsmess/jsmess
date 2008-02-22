/******************************************************************************

    drivers/sorc.c

    Sord m5 system driver

    Thankyou to Roman Stec and Jan P. Naidr for the documentation and much
    help.

    http://falabella.lf2.cuni.cz/~naidr/sord/

    PI-5 is the parallel interface using a 8255.
    FD-5 is the disc operating system and disc interface.
    FD-5 is connected to M5 via PI-5.


    Kevin Thacker [MESS driver]

 ******************************************************************************/

#include "driver.h"
#include "deprecat.h"
#include "video/tms9928a.h"
#include "sound/sn76496.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "machine/centroni.h"
#include "devices/printer.h"
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "machine/8255ppi.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "devices/mflopimg.h"
#include "formats/sord_cas.h"
#include "formats/basicdsk.h"


#define SORD_DEBUG 1
#define LOG(x) do { if (SORD_DEBUG) logerror x; } while (0)

/*********************************************************************************************/
/* FD5 disk interface */
/* - Z80 CPU */
/* - 27128 ROM (16K) */
/* - 2x6116 RAM */
/* - Intel8272/NEC765 */
/* - IRQ of NEC765 is connected to INT of Z80 */
/* PI-5 interface is required. mode 2 of the 8255 is used to communicate with the FD-5 */

#include "machine/nec765.h"
#include "devices/cassette.h"


static MACHINE_RESET( sord_m5 );

static unsigned char fd5_databus;

static ADDRESS_MAP_START( sord_fd5_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x03fff) AM_ROM	/* internal rom */
	AM_RANGE(0x4000, 0x0ffff) AM_RAM
ADDRESS_MAP_END


static int obfa,ibfa, intra;
static int fd5_port_0x020_data;

/* stb and ack automatically set on read/write? */
static WRITE8_HANDLER(fd5_communication_w)
{
	cpu_yield();

	fd5_port_0x020_data = data;
	LOG(("fd5 0x020: %02x %04x\n",data,activecpu_get_pc()));
}

static  READ8_HANDLER(fd5_communication_r)
{
	int data;

	cpu_yield();

	data = (obfa<<3)|(ibfa<<2)|2;
	LOG(("fd5 0x030: %02x %04x\n",data, activecpu_get_pc()));

	return data;
}

static  READ8_HANDLER(fd5_data_r)
{
	cpu_yield();

	LOG(("fd5 0x010 r: %02x %04x\n",fd5_databus,activecpu_get_pc()));

	ppi8255_set_portC(0, 0x50);
	ppi8255_set_portC(0, 0x10);
	ppi8255_set_portC(0, 0x50);

	return fd5_databus;
}

static WRITE8_HANDLER(fd5_data_w)
{
	LOG(("fd5 0x010 w: %02x %04x\n",data,activecpu_get_pc()));

	fd5_databus = data;

	/* set stb on data write */
	ppi8255_set_portC(0, 0x50);
	ppi8255_set_portC(0, 0x40);
	ppi8255_set_portC(0, 0x50);

	cpu_yield();
}

static WRITE8_HANDLER(fd5_drive_control_w)
{
	int state;

	if (data==0)
		state = 0;
	else
		state = 1;

	LOG(("fd5 drive state w: %02x\n",state));

	floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 0), state);
	floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 0), state);
	floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 1), 1,1);
	floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 1), 1,1);
}

static WRITE8_HANDLER(fd5_tc_w)
{
	nec765_set_tc_state(1);
	nec765_set_tc_state(0);
}

/* 0x020 fd5 writes to this port to communicate with m5 */
/* 0x010 data bus to read/write from m5 */
/* 0x030 fd5 reads from this port to communicate with m5 */
/* 0x040 */
/* 0x050 */
static ADDRESS_MAP_START( readport_sord_fd5 , ADDRESS_SPACE_IO, 8)
	AM_RANGE( 0x000, 0x000) AM_READ( nec765_status_r)
	AM_RANGE( 0x001, 0x001) AM_READ( nec765_data_r)
	AM_RANGE( 0x010, 0x010) AM_READ( fd5_data_r)
	AM_RANGE( 0x030, 0x030) AM_READ( fd5_communication_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport_sord_fd5 , ADDRESS_SPACE_IO, 8)
	AM_RANGE( 0x001, 0x001) AM_WRITE( nec765_data_w)
	AM_RANGE( 0x010, 0x010) AM_WRITE( fd5_data_w)
	AM_RANGE( 0x020, 0x020) AM_WRITE( fd5_communication_w)
	AM_RANGE( 0x040, 0x040) AM_WRITE( fd5_drive_control_w)
	AM_RANGE( 0x050, 0x050) AM_WRITE( fd5_tc_w)
ADDRESS_MAP_END

/* nec765 data request is connected to interrupt of z80 inside fd5 interface */
static void sord_fd5_fdc_interrupt(int state)
{
	if (state)
	{
		cpunum_set_input_line(Machine, 1, 0, HOLD_LINE);
	}
	else
	{
		cpunum_set_input_line(Machine, 1, 0,CLEAR_LINE);
	}
}

static const struct nec765_interface sord_fd5_nec765_interface=
{
	sord_fd5_fdc_interrupt,
	NULL
};

static void sord_fd5_init(void)
{
	nec765_init(&sord_fd5_nec765_interface,NEC765A);
}

static MACHINE_RESET( sord_m5_fd5 )
{
	floppy_drive_set_geometry(image_from_devtype_and_index(IO_FLOPPY, 0), FLOPPY_DRIVE_SS_40);
	floppy_drive_set_geometry(image_from_devtype_and_index(IO_FLOPPY, 1), FLOPPY_DRIVE_SS_40);
	sord_fd5_init();
	MACHINE_RESET_CALL(sord_m5);
	ppi8255_set_portC(0, 0x50);
}


static mess_image *cassette_device_image(void)
{
	return image_from_devtype_and_index(IO_CASSETTE, 0);
}

/*********************************************************************************************/
/* PI-5 */

static  READ8_HANDLER(sord_ppi_porta_r)
{
	cpu_yield();

	return fd5_databus;
}

static  READ8_HANDLER(sord_ppi_portb_r)
{
	cpu_yield();

	LOG(("m5 read from pi5 port b %04x\n",activecpu_get_pc()));

	return 0x0ff;
}

static  READ8_HANDLER(sord_ppi_portc_r)
{
	cpu_yield();

	LOG(("m5 read from pi5 port c %04x\n",activecpu_get_pc()));

/* from fd5 */
/* 00 = 0000 = write */
/* 02 = 0010 = write */
/* 06 = 0110 = read */
/* 04 = 0100 = read */

/* m5 expects */
/*00 = READ */
/*01 = POTENTIAL TO READ BUT ALSO RESET */
/*10 = WRITE */
/*11 = FORCE RESET AND WRITE */

	/* FD5 bit 0 -> M5 bit 2 */
	/* FD5 bit 2 -> M5 bit 1 */
	/* FD5 bit 1 -> M5 bit 0 */
	return (
			/* FD5 bit 0-> M5 bit 2 */
			((fd5_port_0x020_data & 0x01)<<2) |
			/* FD5 bit 2-> M5 bit 1 */
			((fd5_port_0x020_data & 0x04)>>1) |
			/* FD5 bit 1-> M5 bit 0 */
			((fd5_port_0x020_data & 0x02)>>1)
			);
}

static WRITE8_HANDLER(sord_ppi_porta_w)
{
	cpu_yield();

	fd5_databus = data;
}

static WRITE8_HANDLER(sord_ppi_portb_w)
{
	cpu_yield();

	/* f0, 40 */
	/* 1111 */
	/* 0100 */

	if (data==0x0f0)
	{
		cpunum_set_input_line(Machine, 1, INPUT_LINE_RESET, ASSERT_LINE);
		cpunum_set_input_line(Machine, 1, INPUT_LINE_RESET, CLEAR_LINE);
	}
	LOG(("m5 write to pi5 port b: %02x %04x\n",data,activecpu_get_pc()));
}

/* A,  B,  C,  D,  E,   F,  G,  H,  I,  J, K,  L,  M,   N, O, P, Q, R,   */
/* 41, 42, 43, 44, 45, 46, 47, 48, 49, 4a, 4b, 4c, 4d, 4e, 4f, 50, 51, 52*/

/* C,H,N */


static WRITE8_HANDLER(sord_ppi_portc_w)
{
	obfa = (data & 0x80) ? 1 : 0;
	intra = (data & 0x08) ? 1 : 0;
	ibfa = (data & 0x20) ? 1 : 0;

	cpu_yield();
	LOG(("m5 write to pi5 port c: %02x %04x\n",data,activecpu_get_pc()));
}

static const ppi8255_interface sord_ppi8255_interface =
{
	1,
	{sord_ppi_porta_r},
	{sord_ppi_portb_r},
	{sord_ppi_portc_r},
	{sord_ppi_porta_w},
	{sord_ppi_portb_w},
	{sord_ppi_portc_w}
};

/*********************************************************************************************/


static void sord_m5_ctc_interrupt(int state)
{
	//logerror("interrupting ctc %02x\r\n ",state);
	cpunum_set_input_line(Machine, 0, 0, state);
}

static z80ctc_interface	sord_m5_ctc_intf =
{
	3800000,
	0,
	sord_m5_ctc_interrupt,
	0,
	0,
    0
};

static READ8_HANDLER ( sord_keyboard_r )
{
	return readinputport(offset);
}

static ADDRESS_MAP_START( sord_m5_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x01fff) AM_ROM	/* internal rom */
	AM_RANGE(0x2000, 0x06fff) AM_ROMBANK(1)
	AM_RANGE(0x7000, 0x0ffff) AM_RAM
ADDRESS_MAP_END



static READ8_HANDLER(sord_ctc_r)
{
	unsigned char data;

	data = z80ctc_0_r(offset & 0x03);

	logerror("sord ctc r: %04x %02x\n",(offset & 0x03), data);

	return data;
}

static WRITE8_HANDLER(sord_ctc_w)
{
	logerror("sord ctc w: %04x %02x\n",(offset & 0x03), data);

	z80ctc_0_w(offset & 0x03, data);
}

static  READ8_HANDLER(sord_sys_r)
{
	unsigned char data;
	int printer_handshake;

	data = 0;

	/* cassette read */
	if (cassette_input(cassette_device_image()) >=0)
		data |=(1<<0);

	printer_handshake = centronics_read_handshake(0);

	/* if printer is not online, it is busy */
	if ((printer_handshake & CENTRONICS_ONLINE)!=0)
	{
		data|=(1<<1);
	}

	/* bit 7 must be 0 for saving and loading to work */

	logerror("sys read: %02x\n",data);

	return data;
}

/* write */
/* bit 0 is strobe to printer or cassette write data */
/* bit 1 is cassette remote */

/* read */
/* bit 0 is cassette read data */
/* bit 1 is printer busy */

static WRITE8_HANDLER(sord_sys_w)
{
	int handshake;

	handshake = 0;
	if (data & (1<<0))
	{
		handshake = CENTRONICS_STROBE;
	}

	/* cassette remote */
	cassette_change_state(
		cassette_device_image(),
		(data & 0x02) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED,
		CASSETTE_MASK_MOTOR);

	/* cassette data */
	cassette_output(cassette_device_image(), (data & (1<<0)) ? -1.0 : 1.0);

	/* assumption: select is tied low */
	centronics_write_handshake(0, CENTRONICS_SELECT | CENTRONICS_NO_RESET, CENTRONICS_SELECT| CENTRONICS_NO_RESET);
	centronics_write_handshake(0, handshake, CENTRONICS_STROBE);

	logerror("sys write: %02x\n",data);
}

static WRITE8_HANDLER(sord_printer_w)
{
//  logerror("centronics w: %02x\n",data);
	centronics_write_data(0,data);
}

static ADDRESS_MAP_START( sord_m5_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x0f)					AM_READWRITE(sord_ctc_r,			sord_ctc_w)
	AM_RANGE(0x10, 0x10) AM_MIRROR(0x0e)	AM_READWRITE(TMS9928A_vram_r,		TMS9928A_vram_w)
	AM_RANGE(0x11, 0x11) AM_MIRROR(0x0e)	AM_READWRITE(TMS9928A_register_r,	TMS9928A_register_w)
	AM_RANGE(0x20, 0x2f)					AM_WRITE(							SN76496_0_w)
	AM_RANGE(0x30, 0x3f)					AM_READ(sord_keyboard_r)
	AM_RANGE(0x40, 0x40)					AM_WRITE(							sord_printer_w)
	AM_RANGE(0x50, 0x50)					AM_READWRITE(sord_sys_r,			sord_sys_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( srdm5fd5_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x0f)					AM_READWRITE(sord_ctc_r,			sord_ctc_w)
	AM_RANGE(0x10, 0x10) AM_MIRROR(0x0e)	AM_READWRITE(TMS9928A_vram_r,		TMS9928A_vram_w)
	AM_RANGE(0x11, 0x11) AM_MIRROR(0x0e)	AM_READWRITE(TMS9928A_register_r,	TMS9928A_register_w)
	AM_RANGE(0x20, 0x2f)					AM_WRITE(							SN76496_0_w)
	AM_RANGE(0x30, 0x3f)					AM_READ(sord_keyboard_r)
	AM_RANGE(0x40, 0x40)					AM_WRITE(							sord_printer_w)
	AM_RANGE(0x50, 0x50)					AM_READWRITE(sord_sys_r,			sord_sys_w)
	AM_RANGE(0x70, 0x73)					AM_READWRITE(ppi8255_0_r,			ppi8255_0_w)
ADDRESS_MAP_END



static const CENTRONICS_CONFIG sordm5_cent_config[1] =
{
	{
		PRINTER_CENTRONICS,
		NULL
	},
};

static void sordm5_video_interrupt_callback(int state)
{
	if (state)
	{
		z80ctc_0_trg3_w(0,1);
		z80ctc_0_trg3_w(0,0);
	}
}

static const TMS9928a_interface tms9928a_interface =
{
	TMS9929A,
	0x4000,
	0, 0,
	sordm5_video_interrupt_callback
};

static MACHINE_START( sord_m5 )
{
	TMS9928A_configure(&tms9928a_interface);
}

static MACHINE_RESET( sord_m5 )
{
	z80ctc_init(0, &sord_m5_ctc_intf);

	/* PI-5 interface connected to Sord M5 */
	ppi8255_init(&sord_ppi8255_interface);

//  cassette_timer = timer_pulse(TIME_IN_HZ(11025), NULL, 0, cassette_timer_callback);
	TMS9928A_reset ();
	z80ctc_reset(0);

	/* should be done in a special callback to work properly! */
	memory_set_bankptr(1, memory_region(REGION_USER1));

	centronics_config(0, sordm5_cent_config);
	/* assumption: select is tied low */
	centronics_write_handshake(0, CENTRONICS_SELECT | CENTRONICS_NO_RESET, CENTRONICS_SELECT| CENTRONICS_NO_RESET);
}


static INPUT_PORTS_START(sord_m5)
	/* line 0 */
	PORT_START
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL)
    PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("FUNC") PORT_CODE(KEYCODE_LALT)
    PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("LSHIFT") PORT_CODE(KEYCODE_LSHIFT)
    PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RSHIFT") PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT  (0x10, 0x00, IPT_UNUSED)
	PORT_BIT  (0x20, 0x00, IPT_UNUSED)
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE)
    PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER)
	/* line 1 */
	PORT_START
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
    PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
    PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
    PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
    PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
    PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
    PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
    PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
	/* line 2 */
	PORT_START
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
    PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
    PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
    PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
    PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
    PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
    PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
    PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
	/* line 3 */
	PORT_START
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
    PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
    PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
    PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
    PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
    PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
    PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
    PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
	/* line 4 */
	PORT_START
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
    PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
    PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
    PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
    PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
    PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
    PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
    PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA)
	/* line 5 */
	PORT_START
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
    PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
    PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)
    PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("^") PORT_CODE(KEYCODE_EQUALS)
    PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)
    PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH)
    PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("_") PORT_CODE(KEYCODE_MINUS_PAD)
    PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\\") PORT_CODE(KEYCODE_BACKSLASH)
	/* line 6 */
	PORT_START
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
    PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
    PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("@") PORT_CODE(KEYCODE_QUOTE)
    PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_OPENBRACE)
    PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
    PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_PLUS_PAD)
    PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(":") PORT_CODE(KEYCODE_COLON)
    PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_CLOSEBRACE)
	/* line 7 */
	PORT_START
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("JOY 1 RIGHT") PORT_CODE(JOYCODE_X_RIGHT_SWITCH)
    PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("JOY 1 UP") PORT_CODE(JOYCODE_Y_UP_SWITCH)
    PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("JOY 1 LEFT") PORT_CODE(JOYCODE_X_LEFT_SWITCH)
    PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("JOY 1 DOWN") PORT_CODE(JOYCODE_Y_DOWN_SWITCH)
    PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("JOY 2 RIGHT") PORT_CODE(JOYCODE_X_RIGHT_SWITCH)
    PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("JOY 2 UP") PORT_CODE(JOYCODE_Y_UP_SWITCH)
    PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("JOY 2 LEFT") PORT_CODE(JOYCODE_X_LEFT_SWITCH)
    PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("JOY 2 DOWN") PORT_CODE(JOYCODE_Y_DOWN_SWITCH)
	/* line 8 */
	PORT_START
	PORT_BIT (0x0ff, 0x000, IPT_UNUSED)
	/* line 9 */
	PORT_START
	PORT_BIT (0x0ff, 0x000, IPT_UNUSED)
	/* line 10 */
	PORT_START
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RSHIFT") PORT_CODE(KEYCODE_2_PAD)
    PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RSHIFT") PORT_CODE(KEYCODE_3_PAD)
    PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RSHIFT") PORT_CODE(KEYCODE_4_PAD)
    PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RSHIFT") PORT_CODE(KEYCODE_5_PAD)
    PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RSHIFT") PORT_CODE(KEYCODE_6_PAD)
    PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RSHIFT") PORT_CODE(KEYCODE_7_PAD)
    PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RSHIFT") PORT_CODE(KEYCODE_8_PAD)
    PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RSHIFT") PORT_CODE(KEYCODE_9_PAD)
/*  PORT_BIT (0x0ff, 0x000, IPT_UNUSED) */
	/* line 11 */
	PORT_START
	PORT_BIT (0x0ff, 0x000, IPT_UNUSED)
	/* line 12 */
	PORT_START
	PORT_BIT (0x0ff, 0x000, IPT_UNUSED)
	/* line 13 */
	PORT_START
	PORT_BIT (0x0ff, 0x000, IPT_UNUSED)
	/* line 14 */
	PORT_START
	PORT_BIT (0x0ff, 0x000, IPT_UNUSED)
	/* line 15 */
	PORT_START
	PORT_BIT (0x0ff, 0x000, IPT_UNUSED)
	PORT_START
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RESET") PORT_CODE(KEYCODE_ESC)

INPUT_PORTS_END


static const struct z80_irq_daisy_chain sord_m5_daisy_chain[] =
{
	{z80ctc_reset, z80ctc_irq_state, z80ctc_irq_ack, z80ctc_irq_reti, 0},
	{0,0,0,0,-1}
};



static INTERRUPT_GEN( sord_interrupt )
{
	if (TMS9928A_interrupt())
		cpunum_set_input_line(machine, 0, 0, HOLD_LINE);
}

static MACHINE_DRIVER_START( sord_m5 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3800000)
	MDRV_CPU_PROGRAM_MAP(sord_m5_mem, 0)
	MDRV_CPU_IO_MAP(sord_m5_io, 0)
	MDRV_CPU_VBLANK_INT(sord_interrupt, 1)
	MDRV_CPU_CONFIG( sord_m5_daisy_chain )
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START( sord_m5 )
	MDRV_MACHINE_RESET( sord_m5 )

	/* video hardware */
	MDRV_IMPORT_FROM(tms9928a)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76489A, 3579545)	/* 3.579545 MHz */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( sord_m5_fd5 )
	MDRV_IMPORT_FROM( sord_m5 )

	MDRV_CPU_REPLACE("main", Z80, 3800000)
	MDRV_CPU_IO_MAP(srdm5fd5_io, 0)

	MDRV_CPU_ADD(Z80, 3800000)
	MDRV_CPU_PROGRAM_MAP(sord_fd5_mem, 0)
	MDRV_CPU_IO_MAP(readport_sord_fd5,writeport_sord_fd5)

	MDRV_INTERLEAVE(20)
	MDRV_MACHINE_RESET( sord_m5_fd5 )
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(sordm5)
	ROM_REGION(0x010000, REGION_CPU1, 0)
	ROM_LOAD("sordint.rom",0x0000, 0x02000, CRC(78848d39) SHA1(ac042c4ae8272ad6abe09ae83492ef9a0026d0b2))
	ROM_REGION(0x5000, REGION_USER1, 0)
	ROM_CART_LOAD(0, "rom", 0x0000, 0x5000, ROM_NOMIRROR)
ROM_END


ROM_START(srdm5fd5)
	ROM_REGION(0x10000, REGION_CPU1, 0)
	ROM_LOAD("sordint.rom",0x0000, 0x02000, CRC(78848d39) SHA1(ac042c4ae8272ad6abe09ae83492ef9a0026d0b2))
	ROM_REGION(0x10000, REGION_CPU2, 0)
	ROM_LOAD("sordfd5.rom",0x0000, 0x04000, NO_DUMP)
	ROM_REGION(0x5000, REGION_USER1, 0)
ROM_END

static FLOPPY_OPTIONS_START( sordm5 )
	FLOPPY_OPTION( sordm5, "dsk",			"Sord M5 disk image",	basicdsk_identify_default,	basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([18])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static void sordm5_printer_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* printer */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										printer_device_getinfo(devclass, state, info); break;
	}
}

static void sordm5_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_CASSETTE_FORMATS:				info->p = (void *) sordm5_cassette_formats; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(sordm5)
	CONFIG_RAM_DEFAULT(64 * 1024)
	CONFIG_DEVICE(sordm5_printer_getinfo)
	CONFIG_DEVICE(sordm5_cassette_getinfo)
	CONFIG_DEVICE(cartslot_device_getinfo)
SYSTEM_CONFIG_END

static void srdm5fd5_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_FLOPPY_OPTIONS:				info->p = (void *) floppyoptions_sordm5; break;

		default:										floppy_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(srdm5fd5)
	CONFIG_IMPORT_FROM(sordm5)
	CONFIG_DEVICE(srdm5fd5_floppy_getinfo)
SYSTEM_CONFIG_END

/*    YEAR  NAME        PARENT  COMPAT  MACHINE         INPUT       INIT    CONFIG      COMPANY     FULLNAME */
COMP( 1983, sordm5,		0,		0,		sord_m5,		sord_m5,	0,		sordm5,		"Sord",		"Sord M5", 0)
COMP(1983, srdm5fd5,	0,		0,		sord_m5_fd5,	sord_m5,	0,		srdm5fd5,	"Sord",		"Sord M5 + PI5 + FD5", GAME_NOT_WORKING)
