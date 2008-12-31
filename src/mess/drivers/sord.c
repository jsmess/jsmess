/******************************************************************************

    drivers/sord.c

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
#include "video/tms9928a.h"
#include "sound/sn76496.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "machine/centroni.h"
#include "devices/printer.h"
#include "machine/z80ctc.h"
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
	cpu_yield(space->cpu);

	fd5_port_0x020_data = data;
	LOG(("fd5 0x020: %02x %04x\n",data,cpu_get_pc(space->cpu)));
}

static  READ8_HANDLER(fd5_communication_r)
{
	int data;

	cpu_yield(space->cpu);

	data = (obfa<<3)|(ibfa<<2)|2;
	LOG(("fd5 0x030: %02x %04x\n",data, cpu_get_pc(space->cpu)));

	return data;
}

static  READ8_HANDLER(fd5_data_r)
{
	cpu_yield(space->cpu);

	LOG(("fd5 0x010 r: %02x %04x\n",fd5_databus,cpu_get_pc(space->cpu)));

	ppi8255_set_port_c((device_config*)device_list_find_by_tag( space->machine->config->devicelist, PPI8255, "ppi8255" ), 0x50);
	ppi8255_set_port_c((device_config*)device_list_find_by_tag( space->machine->config->devicelist, PPI8255, "ppi8255" ), 0x10);
	ppi8255_set_port_c((device_config*)device_list_find_by_tag( space->machine->config->devicelist, PPI8255, "ppi8255" ), 0x50);

	return fd5_databus;
}

static WRITE8_HANDLER(fd5_data_w)
{
	LOG(("fd5 0x010 w: %02x %04x\n",data,cpu_get_pc(space->cpu)));

	fd5_databus = data;

	/* set stb on data write */
	ppi8255_set_port_c((device_config*)device_list_find_by_tag( space->machine->config->devicelist, PPI8255, "ppi8255" ), 0x50);
	ppi8255_set_port_c((device_config*)device_list_find_by_tag( space->machine->config->devicelist, PPI8255, "ppi8255" ), 0x40);
	ppi8255_set_port_c((device_config*)device_list_find_by_tag( space->machine->config->devicelist, PPI8255, "ppi8255" ), 0x50);

	cpu_yield(space->cpu);
}

static WRITE8_HANDLER(fd5_drive_control_w)
{
	int state;

	if (data==0)
		state = 0;
	else
		state = 1;

	LOG(("fd5 drive state w: %02x\n",state));

	floppy_drive_set_motor_state(image_from_devtype_and_index(space->machine, IO_FLOPPY, 0), state);
	floppy_drive_set_motor_state(image_from_devtype_and_index(space->machine, IO_FLOPPY, 0), state);
	floppy_drive_set_ready_state(image_from_devtype_and_index(space->machine, IO_FLOPPY, 1), 1,1);
	floppy_drive_set_ready_state(image_from_devtype_and_index(space->machine, IO_FLOPPY, 1), 1,1);
}

static WRITE8_HANDLER(fd5_tc_w)
{
	device_config *fdc = (device_config*)device_list_find_by_tag( space->machine->config->devicelist, NEC765A, "nec765");
	nec765_set_tc_state(fdc, 1);
	nec765_set_tc_state(fdc, 0);
}

/* 0x020 fd5 writes to this port to communicate with m5 */
/* 0x010 data bus to read/write from m5 */
/* 0x030 fd5 reads from this port to communicate with m5 */
/* 0x040 */
/* 0x050 */
static ADDRESS_MAP_START(sord_fd5_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x000, 0x000) AM_DEVREAD( NEC765A, "nec765", nec765_status_r)
	AM_RANGE(0x001, 0x001) AM_DEVREADWRITE( NEC765A, "nec765", nec765_data_r, nec765_data_w)
	AM_RANGE(0x010, 0x010) AM_READWRITE(fd5_data_r, fd5_data_w)
	AM_RANGE(0x020, 0x020) AM_WRITE(fd5_communication_w)
	AM_RANGE(0x030, 0x030) AM_READ(fd5_communication_r)
	AM_RANGE(0x040, 0x040) AM_WRITE(fd5_drive_control_w)
	AM_RANGE(0x050, 0x050) AM_WRITE(fd5_tc_w)
ADDRESS_MAP_END

/* nec765 data request is connected to interrupt of z80 inside fd5 interface */
static NEC765_INTERRUPT( sord_fd5_fdc_interrupt )
{
	if (state)
	{
		cpu_set_input_line(device->machine->cpu[1], 0, HOLD_LINE);
	}
	else
	{
		cpu_set_input_line(device->machine->cpu[1], 0,CLEAR_LINE);
	}
}

static const struct nec765_interface sord_fd5_nec765_interface=
{
	sord_fd5_fdc_interrupt,
	NULL,
	NULL,
	NEC765_RDY_PIN_CONNECTED
};

static MACHINE_RESET( sord_m5_fd5 )
{
	floppy_drive_set_geometry(image_from_devtype_and_index(machine, IO_FLOPPY, 0), FLOPPY_DRIVE_SS_40);
	floppy_drive_set_geometry(image_from_devtype_and_index(machine, IO_FLOPPY, 1), FLOPPY_DRIVE_SS_40);
	MACHINE_RESET_CALL(sord_m5);
	ppi8255_set_port_c((device_config*)device_list_find_by_tag( machine->config->devicelist, PPI8255, "ppi8255" ), 0x50);
}


static const device_config *cassette_device_image(running_machine *machine)
{
	return device_list_find_by_tag( machine->config->devicelist, CASSETTE, "cassette" );
}

/*********************************************************************************************/
/* PI-5 */

static READ8_DEVICE_HANDLER(sord_ppi_porta_r)
{
	cpu_yield(device->machine->cpu[0]);

	return fd5_databus;
}

static READ8_DEVICE_HANDLER(sord_ppi_portb_r)
{
	cpu_yield(device->machine->cpu[0]);

	LOG(("m5 read from pi5 port b %04x\n",cpu_get_pc(device->machine->cpu[0])));

	return 0x0ff;
}

static READ8_DEVICE_HANDLER(sord_ppi_portc_r)
{
	cpu_yield(device->machine->cpu[0]);

	LOG(("m5 read from pi5 port c %04x\n",cpu_get_pc(device->machine->cpu[0])));

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

static WRITE8_DEVICE_HANDLER(sord_ppi_porta_w)
{
	cpu_yield(device->machine->cpu[0]);

	fd5_databus = data;
}

static WRITE8_DEVICE_HANDLER(sord_ppi_portb_w)
{
	cpu_yield(device->machine->cpu[0]);

	/* f0, 40 */
	/* 1111 */
	/* 0100 */

	if (data==0x0f0)
	{
		cpu_set_input_line(device->machine->cpu[1], INPUT_LINE_RESET, ASSERT_LINE);
		cpu_set_input_line(device->machine->cpu[1], INPUT_LINE_RESET, CLEAR_LINE);
	}
	LOG(("m5 write to pi5 port b: %02x %04x\n",data,cpu_get_pc(device->machine->cpu[0])));
}

/* A,  B,  C,  D,  E,   F,  G,  H,  I,  J, K,  L,  M,   N, O, P, Q, R,   */
/* 41, 42, 43, 44, 45, 46, 47, 48, 49, 4a, 4b, 4c, 4d, 4e, 4f, 50, 51, 52*/

/* C,H,N */


static WRITE8_DEVICE_HANDLER(sord_ppi_portc_w)
{
	obfa = (data & 0x80) ? 1 : 0;
	intra = (data & 0x08) ? 1 : 0;
	ibfa = (data & 0x20) ? 1 : 0;

	cpu_yield(device->machine->cpu[0]);
	LOG(("m5 write to pi5 port c: %02x %04x\n",data,cpu_get_pc(device->machine->cpu[0])));
}

static const ppi8255_interface sord_ppi8255_interface =
{
	sord_ppi_porta_r,
	sord_ppi_portb_r,
	sord_ppi_portc_r,
	sord_ppi_porta_w,
	sord_ppi_portb_w,
	sord_ppi_portc_w
};

/*********************************************************************************************/


static void sord_m5_ctc_interrupt(const device_config *device, int state)
{
	//logerror("interrupting ctc %02x\r\n ",state);
	cpu_set_input_line(device->machine->cpu[0], 0, state);
}

static const z80ctc_interface	sord_m5_ctc_intf =
{
	0,
	sord_m5_ctc_interrupt,
	0,
	0,
	0
};

static READ8_HANDLER ( sord_keyboard_r )
{
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7",
										"LINE8", "LINE9", "LINE10", "LINE11", "LINE12", "LINE13", "LINE14", "LINE15",
										"RESET"};

	return input_port_read(space->machine, keynames[offset]);
}

static ADDRESS_MAP_START( sord_m5_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x01fff) AM_ROM	/* internal rom */
	AM_RANGE(0x2000, 0x06fff) AM_ROMBANK(1)
	AM_RANGE(0x7000, 0x0ffff) AM_RAM
ADDRESS_MAP_END



static READ8_DEVICE_HANDLER(sord_ctc_r)
{
	unsigned char data;

	data = z80ctc_r(device, offset & 0x03);

	logerror("sord ctc r: %04x %02x\n",(offset & 0x03), data);

	return data;
}

static WRITE8_DEVICE_HANDLER(sord_ctc_w)
{
	logerror("sord ctc w: %04x %02x\n",(offset & 0x03), data);

	z80ctc_w(device, offset & 0x03, data);
}

static READ8_HANDLER(sord_sys_r)
{
	unsigned char data;
	int printer_handshake;

	data = 0;

	/* cassette read */
	if (cassette_input(cassette_device_image(space->machine)) >=0)
		data |=(1<<0);

	printer_handshake = centronics_read_handshake(space->machine, 0);

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
		cassette_device_image(space->machine),
		(data & 0x02) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED,
		CASSETTE_MASK_MOTOR);

	/* cassette data */
	cassette_output(cassette_device_image(space->machine), (data & (1<<0)) ? -1.0 : 1.0);

	/* assumption: select is tied low */
	centronics_write_handshake(space->machine, 0, CENTRONICS_SELECT | CENTRONICS_NO_RESET, CENTRONICS_SELECT| CENTRONICS_NO_RESET);
	centronics_write_handshake(space->machine, 0, handshake, CENTRONICS_STROBE);

	logerror("sys write: %02x\n",data);
}

static WRITE8_HANDLER(sord_printer_w)
{
//  logerror("centronics w: %02x\n",data);
	centronics_write_data(space->machine, 0,data);
}

static ADDRESS_MAP_START( sord_m5_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x0f)					AM_DEVREADWRITE(Z80CTC, "z80ctc", sord_ctc_r,			sord_ctc_w)
	AM_RANGE(0x10, 0x10) AM_MIRROR(0x0e)	AM_READWRITE(TMS9928A_vram_r,		TMS9928A_vram_w)
	AM_RANGE(0x11, 0x11) AM_MIRROR(0x0e)	AM_READWRITE(TMS9928A_register_r,	TMS9928A_register_w)
	AM_RANGE(0x20, 0x2f)					AM_WRITE(							sn76496_0_w)
	AM_RANGE(0x30, 0x3f)					AM_READ(sord_keyboard_r)
	AM_RANGE(0x40, 0x40)					AM_WRITE(							sord_printer_w)
	AM_RANGE(0x50, 0x50)					AM_READWRITE(sord_sys_r,			sord_sys_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( srdm5fd5_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x0f)					AM_DEVREADWRITE(Z80CTC, "z80ctc", sord_ctc_r,			sord_ctc_w)
	AM_RANGE(0x10, 0x10) AM_MIRROR(0x0e)	AM_READWRITE(TMS9928A_vram_r,		TMS9928A_vram_w)
	AM_RANGE(0x11, 0x11) AM_MIRROR(0x0e)	AM_READWRITE(TMS9928A_register_r,	TMS9928A_register_w)
	AM_RANGE(0x20, 0x2f)					AM_WRITE(							sn76496_0_w)
	AM_RANGE(0x30, 0x3f)					AM_READ(sord_keyboard_r)
	AM_RANGE(0x40, 0x40)					AM_WRITE(							sord_printer_w)
	AM_RANGE(0x50, 0x50)					AM_READWRITE(sord_sys_r,			sord_sys_w)
	AM_RANGE(0x70, 0x73)					AM_DEVREADWRITE(PPI8255, "ppi8255", ppi8255_r, ppi8255_w)
ADDRESS_MAP_END



static const CENTRONICS_CONFIG sordm5_cent_config[1] =
{
	{
		PRINTER_CENTRONICS,
		NULL
	},
};

static void sordm5_video_interrupt_callback(running_machine *machine, int state)
{
	if (state)
	{
		z80ctc_trg3_w(device_list_find_by_tag(machine->config->devicelist, Z80CTC, "z80ctc"), 0, 1);
		z80ctc_trg3_w(device_list_find_by_tag(machine->config->devicelist, Z80CTC, "z80ctc"), 0, 0);
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
//  cassette_timer = timer_pulse(machine, TIME_IN_HZ(11025), NULL, 0, cassette_timer_callback);
	TMS9928A_reset ();

	/* should be done in a special callback to work properly! */
	memory_set_bankptr(machine, 1, memory_region(machine, "user1"));

	centronics_config(machine, 0, sordm5_cent_config);
	/* assumption: select is tied low */
	centronics_write_handshake(machine, 0, CENTRONICS_SELECT | CENTRONICS_NO_RESET, CENTRONICS_SELECT| CENTRONICS_NO_RESET);
}

/* 2008-05 FP: 
Small note about natural keyboard: currently
- "Reset" is mapped to 'Esc'
- "Func" is mapped to 'F1'
*/
static INPUT_PORTS_START(sord_m5)
	PORT_START("LINE0")
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Func") PORT_CODE(KEYCODE_TAB) PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_LSHIFT)		PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_RSHIFT)		PORT_CHAR(UCHAR_MAMEKEY(RSHIFT))
	PORT_BIT (0x10, 0x00, IPT_UNUSED)
	PORT_BIT (0x20, 0x00, IPT_UNUSED)
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)		PORT_CHAR(' ')
	PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER)		PORT_CHAR(13)

	PORT_START("LINE1")
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) 			PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) 			PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) 			PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) 			PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) 			PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) 			PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) 			PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) 			PORT_CHAR('8') PORT_CHAR('(')

	PORT_START("LINE2")
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) 			PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) 			PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) 			PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) 			PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) 			PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) 			PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) 			PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) 			PORT_CHAR('i') PORT_CHAR('I')

	PORT_START("LINE3")
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) 			PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) 			PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) 			PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) 			PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) 			PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) 			PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) 			PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) 			PORT_CHAR('k') PORT_CHAR('K')

	PORT_START("LINE4")
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) 			PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) 			PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) 			PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_V)			PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) 			PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) 			PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_M)			PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) 		PORT_CHAR(',') PORT_CHAR('<')

	PORT_START("LINE5")
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) 			PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) 			PORT_CHAR('0')
	PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS) 		PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS) 	PORT_CHAR('^') PORT_CHAR('~')
	PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP) 		PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH) 		PORT_CHAR('/') PORT_CHAR('?') PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("_  Triangle") PORT_CODE(KEYCODE_TILDE) PORT_CHAR('_')
	PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH2)	PORT_CHAR('\\') PORT_CHAR('|')

	PORT_START("LINE6")
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) 			PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_P)			PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('@') PORT_CHAR('`') PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_L)			PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON)		PORT_CHAR(';') PORT_CHAR('+') PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE)		PORT_CHAR(':') PORT_CHAR('*') PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH)	PORT_CHAR(']') PORT_CHAR('}')

	PORT_START("LINE7")
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Joystick 1 Right") PORT_CODE(JOYCODE_X_RIGHT_SWITCH)
	PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Joystick 1 Up") PORT_CODE(JOYCODE_Y_UP_SWITCH)
	PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Joystick 1 Left") PORT_CODE(JOYCODE_X_LEFT_SWITCH)
	PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Joystick 1 Down") PORT_CODE(JOYCODE_Y_DOWN_SWITCH)
	PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Joystick 2 Right") PORT_CODE(JOYCODE_X_RIGHT_SWITCH)
	PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Joystick 2 Up") PORT_CODE(JOYCODE_Y_UP_SWITCH)
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Joystick 2 Left") PORT_CODE(JOYCODE_X_LEFT_SWITCH)
	PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Joystick 2 Down") PORT_CODE(JOYCODE_Y_DOWN_SWITCH)

	PORT_START("LINE8")
	PORT_BIT (0x0ff, 0x000, IPT_UNUSED)

	PORT_START("LINE9")
	PORT_BIT (0x0ff, 0x000, IPT_UNUSED)

	PORT_START("LINE10")
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RSHIFT") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RSHIFT") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RSHIFT") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RSHIFT") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RSHIFT") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RSHIFT") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RSHIFT") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RSHIFT") PORT_CODE(KEYCODE_9_PAD)
	/*  PORT_BIT (0x0ff, 0x000, IPT_UNUSED) */

	PORT_START("LINE11")
	PORT_BIT (0x0ff, 0x000, IPT_UNUSED)

	PORT_START("LINE12")
	PORT_BIT (0x0ff, 0x000, IPT_UNUSED)

	PORT_START("LINE13")
	PORT_BIT (0x0ff, 0x000, IPT_UNUSED)

	PORT_START("LINE14")
	PORT_BIT (0x0ff, 0x000, IPT_UNUSED)

	PORT_START("LINE15")
	PORT_BIT (0x0ff, 0x000, IPT_UNUSED)

	PORT_START("RESET")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Reset") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC)) // 1st line, 1st key from right!
INPUT_PORTS_END


static const z80_daisy_chain sord_m5_daisy_chain[] =
{
	{ Z80CTC, "z80ctc" },
	{ NULL }
};



static INTERRUPT_GEN( sord_interrupt )
{
	if (TMS9928A_interrupt(device->machine))
		cpu_set_input_line(device->machine->cpu[0], 0, HOLD_LINE);
}

static const cassette_config sordm5_cassette_config =
{
	sordm5_cassette_formats,
	NULL,
	CASSETTE_PLAY
};

static MACHINE_DRIVER_START( sord_m5 )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", Z80, 3800000)
	MDRV_CPU_PROGRAM_MAP(sord_m5_mem, 0)
	MDRV_CPU_IO_MAP(sord_m5_io, 0)
	MDRV_CPU_VBLANK_INT("main", sord_interrupt)
	MDRV_CPU_CONFIG( sord_m5_daisy_chain )
	MDRV_QUANTUM_TIME(HZ(60))

	MDRV_MACHINE_START( sord_m5 )
	MDRV_MACHINE_RESET( sord_m5 )

	MDRV_PPI8255_ADD( "ppi8255", sord_ppi8255_interface )

	MDRV_Z80CTC_ADD( "z80ctc", 3800000, sord_m5_ctc_intf )

	/* video hardware */
	MDRV_IMPORT_FROM(tms9928a)
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("sn76489a", SN76489A, 3579545)	/* 3.579545 MHz */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* printer */
	MDRV_PRINTER_ADD("printer")

	MDRV_CASSETTE_ADD( "cassette", sordm5_cassette_config )
	
	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom")
	MDRV_CARTSLOT_MANDATORY
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( sord_m5_fd5 )
	MDRV_IMPORT_FROM( sord_m5 )

	MDRV_CPU_REPLACE("main", Z80, 3800000)
	MDRV_CPU_IO_MAP(srdm5fd5_io, 0)

	MDRV_CPU_ADD("floppy", Z80, 3800000)
	MDRV_CPU_PROGRAM_MAP(sord_fd5_mem, 0)
	MDRV_CPU_IO_MAP(sord_fd5_io, 0)

	MDRV_QUANTUM_TIME(HZ(1200))
	MDRV_MACHINE_RESET( sord_m5_fd5 )
	
	MDRV_NEC765A_ADD("nec765", sord_fd5_nec765_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(sordm5)
	ROM_REGION(0x010000, "main", 0)
	ROM_LOAD("sordint.rom",0x0000, 0x02000, CRC(78848d39) SHA1(ac042c4ae8272ad6abe09ae83492ef9a0026d0b2))
	ROM_REGION(0x5000, "user1", 0)
	ROM_CART_LOAD("cart", 0x0000, 0x5000, ROM_NOMIRROR)
ROM_END


ROM_START(srdm5fd5)
	ROM_REGION(0x10000, "main", 0)
	ROM_LOAD("sordint.rom",0x0000, 0x02000, CRC(78848d39) SHA1(ac042c4ae8272ad6abe09ae83492ef9a0026d0b2))
	ROM_REGION(0x10000, "floppy", 0)
	ROM_LOAD("sordfd5.rom",0x0000, 0x04000, NO_DUMP)
	ROM_REGION(0x5000, "user1", 0)
	ROM_CART_LOAD("cart", 0x0000, 0x5000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

static FLOPPY_OPTIONS_START( sordm5 )
	FLOPPY_OPTION( sordm5, "dsk", "Sord M5 disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([18])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END


static SYSTEM_CONFIG_START(sordm5)
	CONFIG_RAM_DEFAULT(64 * 1024)
SYSTEM_CONFIG_END

static void srdm5fd5_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_FLOPPY_OPTIONS:				info->p = (void *) floppyoptions_sordm5; break;

		default:										floppy_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START(srdm5fd5)
	CONFIG_IMPORT_FROM(sordm5)
	CONFIG_DEVICE(srdm5fd5_floppy_getinfo)
SYSTEM_CONFIG_END

/*    YEAR  NAME        PARENT  COMPAT  MACHINE         INPUT       INIT    CONFIG      COMPANY     FULLNAME */
COMP(1983, sordm5,	0,	0,	sord_m5,	sord_m5,	0,	sordm5,		"Sord",	"Sord M5", 0)
COMP(1983, srdm5fd5,	sordm5,	0,	sord_m5_fd5,	sord_m5,	0,	srdm5fd5,	"Sord",	"Sord M5 + PI5 + FD5", 0 )
