/***************************************************************************

	drivers/bw2.c

	Bondwell BW 2

	- Z80L CPU 4MHz
	- 64KB RAM, expandable to 224KB RAM.
	- 4KB System ROM
	- MSM6255 LCD controller, 640x200 pixels
	- 16KB Video RAM
	- TMS2797 FDC controller
	- 8251 USART serial interface 
	- 8253 PIT timer
	- 8255 PPI

  http://www.thebattles.net/bondwell/
	
  http://www.vintage-computer.com/bondwell2.shtml

  TODO:
    - Add LCD controller

  http://www2.okisemi.com/site/productscatalog/displaydrivers/availabledocuments/Intro-7090.html
***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/serial.h"
#include "machine/8255ppi.h"
#include "machine/centroni.h"
#include "machine/msm8251.h"
#include "machine/wd17xx.h"
#include "machine/pit8253.h"
#include "devices/basicdsk.h"
#include "devices/printer.h"


/* Memory */

static void bw2_set_banks(UINT8 data)
{
	/*
	Y0  /RAM1  Memory bank 1
	Y1  /VRAM  Video memory
	Y2  /RAM2  Memory bank 2
	Y3  /RAM3  Memory bank 3
	Y4  /RAM4  Memory bank 4
	Y5  /RAM5  Memory bank 5
	Y6  /RAM6  Memory bank 6
	*/

	memory_set_bank(1, data);
}


/* Serial */

static DEVICE_IMAGE_LOAD( bw2_serial )
{
	/* filename specified */
	if (device_load_serial_device(image) == INIT_PASS)
	{
		/* setup transmit parameters */
		serial_device_setup(image, 9600 >> readinputportbytag("BAUD"), 8, 1, SERIAL_PARITY_NONE);

		/* connect serial chip to serial device */
		msm8251_connect_to_serial_device(image);

		serial_device_set_protocol(image, SERIAL_PROTOCOL_NONE);

		/* and start transmit */
		serial_device_set_transmit_state(image, 1);

		return INIT_PASS;
	}

	return INIT_FAIL;
}


/* Floppy */

static DEVICE_IMAGE_LOAD( bw2_floppy )
{
	if (image_has_been_created(image)) {
		return INIT_FAIL;
	}

	if (image_length(image) == 80*1*18*256)
	{
		if (device_load_basicdsk_floppy(image) == INIT_PASS)
		{
			basicdsk_set_geometry(image, 80, 1, 18, 256, 1, 0, FALSE);
			return INIT_PASS;
		}
	}

	return INIT_FAIL;
}

static void bw2_wd17xx_callback(running_machine *machine, wd17xx_state_t state, void *param)
{
	switch(state)
	{
		case WD17XX_IRQ_CLR:
			cpunum_set_input_line(machine, 0, 0, CLEAR_LINE);
			break;
		case WD17XX_IRQ_SET:
			cpunum_set_input_line(machine, 0, 0, HOLD_LINE);
			break;
		case WD17XX_DRQ_CLR:
			cpunum_set_input_line(machine, 0, INPUT_LINE_NMI, CLEAR_LINE);
			break;
		case WD17XX_DRQ_SET:
			cpunum_set_input_line(machine, 0, INPUT_LINE_NMI, ASSERT_LINE);
			break;
	}

}

static READ8_HANDLER( bw2_wd2797_r )
{
	UINT8 result = 0xff;

	switch(offset & 0x03)
	{
		case 0:
			result = wd17xx_status_r(machine, 0);
			break;
		case 1:
			result = wd17xx_track_r(machine, 0);
			break;
		case 2:
			result = wd17xx_sector_r(machine, 0);
			break;
		case 3:
			result = wd17xx_data_r(machine, 0);
			break;
	}

	return result;
}

static WRITE8_HANDLER( bw2_wd2797_w )
{
	switch(offset & 0x3)
	{
		case 0:
			wd17xx_command_w(machine, 0, data);
	
			/* disk head is encoded in the command byte */
			wd17xx_set_side((data & 0x02) ? 1 : 0);
			break;
		case 1:
			wd17xx_track_w(machine, 0, data);
			break;
		case 2:
			wd17xx_sector_w(machine, 0, data);
			break;
		case 3:
			wd17xx_data_w(machine, 0, data);
			break;
	};
}


/* PPI */

static WRITE8_HANDLER( bw2_ppi8255_a_w )
{
	/*
	PA0     KB0 Keyboard line select 0
	PA1     KB1 Keyboard line select 1
	PA2     KB2 Keyboard line select 2
	PA3     KB3 Keyboard line select 3
	PA4     /DS0 Drive select 0
	PA5     /DS1 Drive select 1
	PA6     Select RS232 connector
	PA7     /STROBE to centronics printer
	*/

	switch (~data & 0x30 << 4)
	{
		case 0:
			wd17xx_set_drive(0);
			break;
		case 1:
			wd17xx_set_drive(1);
			break;
	}

	/* assumption: select is tied low */
	centronics_write_handshake(0, CENTRONICS_SELECT | CENTRONICS_NO_RESET, CENTRONICS_SELECT | CENTRONICS_NO_RESET);
	centronics_write_handshake(0, (data & 0x80) ? 0 : CENTRONICS_STROBE, CENTRONICS_STROBE);
}

static READ8_HANDLER( bw2_ppi8255_b_r )
{
	/*
	PB0     Keyboard column status of selected line
	PB1     Keyboard column status of selected line
	PB2     Keyboard column status of selected line
	PB3     Keyboard column status of selected line
	PB4     Keyboard column status of selected line
	PB5     Keyboard column status of selected line
	PB6     Keyboard column status of selected line
	PB7     Keyboard column status of selected line
	*/
	
	UINT8 row;

	row = ppi8255_peek(0, 0) & 0x0f;
	if (row <= 9)
	{
		return readinputport(row);
	}

	return 0xff;
}

static WRITE8_HANDLER( bw2_ppi8255_c_w )
{
	/*
	PC0     Memory bank select
	PC1     Memory bank select
	PC2     Memory bank select
	PC3     Not connected
	*/

	bw2_set_banks(data & 0x07);
}

static READ8_HANDLER( bw2_ppi8255_c_r )
{
	/*
	PC4     BUSY from centronics printer
	PC5     MTFDBK motor feedback
	PC6     RLSD Carrier detect from RS232
	PC7     /PROT Write protected disk
	*/

	UINT8 data;

	/* assumption: select is tied low */
	centronics_write_handshake(0, CENTRONICS_SELECT | CENTRONICS_NO_RESET, CENTRONICS_SELECT | CENTRONICS_NO_RESET);
	data = ((centronics_read_handshake(0) & CENTRONICS_NOT_BUSY) == 0) ? 0x10 : 0;

	return data;
}

static const ppi8255_interface bw2_ppi8255_interface =
{
	1,
	{ NULL },
	{ bw2_ppi8255_b_r },
	{ bw2_ppi8255_c_r },
	{ bw2_ppi8255_a_w },
	{ NULL },
	{ bw2_ppi8255_c_w },
};


/* PIT */

static void bw2_timer0_w(int state)
{
	msm8251_transmit_clock();
	msm8251_receive_clock();
}

static void bw2_timer2_w(int state)
{
	
}

static const struct pit8253_config bw2_pit8253_interface =
{
	TYPE8253,
	{
		{
			XTAL_4MHz,	/* 8251 USART TXC, RXC */
			bw2_timer0_w,
			NULL
		},
		{
			11000,		/* LCD controller */
			NULL,
			NULL
		},
		{
			3,		/* Floppy /MTRON */
			bw2_timer2_w,
			NULL
		}
	}
};


/* USART */

static const struct msm8251_interface bw2_msm8251_interface =
{
	NULL,
	NULL,
	NULL
};


/* Printer */

static const CENTRONICS_CONFIG bw2_centronics_config[1] =
{
	{
		PRINTER_IBM,
		NULL
	}
};

static WRITE8_HANDLER( bw2_centronics_data_w )
{
	centronics_write_data(0, data);
}


/* Video */

static VIDEO_START( bw2 )
{
}

static VIDEO_UPDATE( bw2 )
{
	return 0;
}

static WRITE8_HANDLER( bw2_msm6255_lcd_data_w )
{
	logerror("MSM6255 LCD - data write = %02x\n", data);
}

static WRITE8_HANDLER( bw2_msm6255_lcd_instr_w )
{
	logerror("MSM6255 LCD - instr write = %02x\n", data);
}


/* Machine */

static DRIVER_INIT( bw2 )
{
	videoram_size = 0x4000;
	videoram = auto_malloc(videoram_size);
}

static MACHINE_RESET( bw2 )
{
	memory_set_bank(1, 0);
}

static MACHINE_START( bw2 )
{
	centronics_config(0, bw2_centronics_config);

	msm8251_init(&bw2_msm8251_interface);
	pit8253_init(0, &bw2_pit8253_interface);

	ppi8255_init(&bw2_ppi8255_interface);

	wd17xx_init(machine, WD_TYPE_2793, bw2_wd17xx_callback, NULL);
	wd17xx_set_density(DEN_FM_HI);

	memory_configure_bank(1, 0, 1, memory_region(REGION_CPU1), 0);
	memory_configure_bank(1, 1, 1, videoram, 0);
	memory_configure_bank(1, 2, 6, mess_ram, 0);
}

static ADDRESS_MAP_START( bw2_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_RAMBANK(1)
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( bw2_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE( 0x00, 0x03 ) AM_READWRITE( ppi8255_0_r, ppi8255_0_w )
//	AM_RANGE( 0x10, 0x13 ) AM_READWRITE( pit8253_0_r, pit8253_0_w )
	AM_RANGE( 0x20, 0x20 ) AM_WRITE( bw2_msm6255_lcd_data_w )
	AM_RANGE( 0x21, 0x21 ) AM_WRITE( bw2_msm6255_lcd_instr_w )
	AM_RANGE( 0x40, 0x40 ) AM_READWRITE( msm8251_data_r, msm8251_data_w )
	AM_RANGE( 0x41, 0x41 ) AM_READWRITE( msm8251_status_r, msm8251_control_w )
	AM_RANGE( 0x50, 0x50 ) AM_WRITE( bw2_centronics_data_w )
	AM_RANGE( 0x60, 0x63 ) AM_READWRITE( bw2_wd2797_r, bw2_wd2797_w )
ADDRESS_MAP_END

/*
  Keyboard matrix
        X0    X1    X2    X3    X4    X5    X6    X7   
     +-----+-----+-----+-----+-----+-----+-----+-----+
  Y9 |CAPS |     |     |     |     |     |     |     |
     +-----+-----+-----+-----+-----+-----+-----+-----+
  Y8 |SHIFT|     |SPACE|  Z  |  A  |  Q  | 2 " | F1  |
     +-----+-----+-----+-----+-----+-----+-----+-----+
  Y7 |CTRL | - = | \ | |  X  |  S  |  W  | 3 # | F2  |
     +-----+-----+-----+-----+-----+-----+-----+-----+
  Y6 | @ ` |  P  | UP  |  C  |  D  |  E  | 4 $ | F3  |
     +-----+-----+-----+-----+-----+-----+-----+-----+
  Y5 | 1 ! |     |     |  V  |  F  |  R  | 5 % | F4  |
     +-----+-----+-----+-----+-----+-----+-----+-----+
  Y4 | ESC |     |     |  B  |  G  |  T  | 6 & | F5  |
     +-----+-----+-----+-----+-----+-----+-----+-----+
  Y3 | TAB | : * |ENTER|  N  |  H  |  Y  | 7 ´ | F6  |
     +-----+-----+-----+-----+-----+-----+-----+-----+
  Y2 |DOWN | ^ ~ | [ { |  M  |  J  |  U  | 8 ( | F7  |
     +-----+-----+-----+-----+-----+-----+-----+-----+
  Y1 |RIGHT| ; + | ] } | , < |  K  |  I  | 9 ) | F8  |
     +-----+-----+-----+-----+-----+-----+-----+-----+
  Y0 |LEFT | BS  | / ? | . > |  L  |  O  | 0 _ | DEL |
     +-----+-----+-----+-----+-----+-----+-----+-----+
*/

static INPUT_PORTS_START( bw2 )
 PORT_START /* 0 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LEFT") PORT_CODE(KEYCODE_LEFT)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BS") PORT_CODE(KEYCODE_BACKSPACE)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("l L") PORT_CODE(KEYCODE_L)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("o O") PORT_CODE(KEYCODE_O)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 _") PORT_CODE(KEYCODE_0)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL)
 PORT_START /* 1 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RIGHT") PORT_CODE(KEYCODE_RIGHT)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("; +") PORT_CODE(KEYCODE_COLON)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("] }") PORT_CODE(KEYCODE_CLOSEBRACE)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("k K") PORT_CODE(KEYCODE_K)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("i I") PORT_CODE(KEYCODE_I)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 )") PORT_CODE(KEYCODE_9)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F8") PORT_CODE(KEYCODE_F8)
 PORT_START /* 2 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DOWN") PORT_CODE(KEYCODE_DOWN)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("^ ~") PORT_CODE(KEYCODE_TILDE)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[ {") PORT_CODE(KEYCODE_OPENBRACE)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("m M") PORT_CODE(KEYCODE_M)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("j J") PORT_CODE(KEYCODE_J)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("u U") PORT_CODE(KEYCODE_U)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 (") PORT_CODE(KEYCODE_8)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F7") PORT_CODE(KEYCODE_F7)
 PORT_START /* 3 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(": *") PORT_CODE(KEYCODE_EQUALS)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("n N") PORT_CODE(KEYCODE_N)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("h H") PORT_CODE(KEYCODE_H)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("y Y") PORT_CODE(KEYCODE_Y)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 '") PORT_CODE(KEYCODE_7)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F6") PORT_CODE(KEYCODE_F6)
 PORT_START /* 4 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_UNUSED)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_UNUSED)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("b B") PORT_CODE(KEYCODE_B)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("g G") PORT_CODE(KEYCODE_G)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("t T") PORT_CODE(KEYCODE_T)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 &") PORT_CODE(KEYCODE_6)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5)
 PORT_START /* 5 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 !") PORT_CODE(KEYCODE_1)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_UNUSED)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_UNUSED)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("v V") PORT_CODE(KEYCODE_V)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("f F") PORT_CODE(KEYCODE_F)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("r R") PORT_CODE(KEYCODE_R)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 %") PORT_CODE(KEYCODE_5)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)
 PORT_START /* 6 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@ `") PORT_CODE(KEYCODE_QUOTE)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("p P") PORT_CODE(KEYCODE_P)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("UP") PORT_CODE(KEYCODE_UP)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("c C") PORT_CODE(KEYCODE_C)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("d D") PORT_CODE(KEYCODE_D)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("e E") PORT_CODE(KEYCODE_E)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 $") PORT_CODE(KEYCODE_4)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)
 PORT_START /* 7 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("- =") PORT_CODE(KEYCODE_MINUS)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\ |") PORT_CODE(KEYCODE_BACKSLASH)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("x X") PORT_CODE(KEYCODE_X)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("s S") PORT_CODE(KEYCODE_S)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("w W") PORT_CODE(KEYCODE_W)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 #") PORT_CODE(KEYCODE_3)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
 PORT_START /* 8 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_UNUSED)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("z Z") PORT_CODE(KEYCODE_Z)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("a A") PORT_CODE(KEYCODE_A)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("q Q") PORT_CODE(KEYCODE_Q)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 \"") PORT_CODE(KEYCODE_2)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
 PORT_START /* 9 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CAPS") PORT_CODE(KEYCODE_CAPSLOCK)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_UNUSED)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_UNUSED)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_UNUSED)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_UNUSED)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)
 PORT_START_TAG("BAUD")
  PORT_CONFNAME( 0x05, 0x05, "Baud rate")
  PORT_CONFSETTING( 0x00, "9600 baud" )
  PORT_CONFSETTING( 0x01, "4800 baud" )
  PORT_CONFSETTING( 0x02, "2400 baud" )
  PORT_CONFSETTING( 0x03, "1200 baud" )
  PORT_CONFSETTING( 0x04, "600 baud" )
  PORT_CONFSETTING( 0x05, "300 baud" )
INPUT_PORTS_END

static MACHINE_DRIVER_START( bw2 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG( "main", Z80, XTAL_16MHz/4 )
	MDRV_CPU_PROGRAM_MAP( bw2_mem, 0 )
	MDRV_CPU_IO_MAP( bw2_io, 0 )

	MDRV_MACHINE_START( bw2 )
	MDRV_MACHINE_RESET( bw2 )

	/* video hardware */
	MDRV_SCREEN_ADD( "main", LCD )
	MDRV_SCREEN_REFRESH_RATE( 60 )
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MDRV_SCREEN_SIZE( 640, 200 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 640-1, 0, 200-1 )

	MDRV_PALETTE_LENGTH( 2 )
	MDRV_PALETTE_INIT( black_and_white )
	MDRV_VIDEO_START( bw2 )
	MDRV_VIDEO_UPDATE( bw2 )
MACHINE_DRIVER_END

/***************************************************************************

  System driver(s)

***************************************************************************/

ROM_START( bw2 )
	ROM_REGION(0x1000, REGION_CPU1, 0)
	ROM_SYSTEM_BIOS(0, "20", "BW 2 v2.0")
	ROMX_LOAD("bw2-20.bin", 0x0000, 0x1000, CRC(86f36471) SHA1(a3e2ba4edd50ff8424bb0675bdbb3b9f13c04c9d), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "12", "BW 2 v1.2")
	ROMX_LOAD("bw2-12.bin", 0x0000, 0x1000, CRC(0ab42d10) SHA1(430b232631eee9b715151b8d191b7eb9449ac513), ROM_BIOS(2))
ROM_END

static void bw2_printer_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* printer */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:
			info->i = 1;
			break;
	
		default:
			printer_device_getinfo(devclass, state, info);
			break;
	}
}

static void bw2_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:
			info->i = 2;
			break;
	
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:
			info->load = DEVICE_IMAGE_LOAD_NAME(bw2_floppy);
			break;
	
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:
			strcpy(info->s = device_temp_str(), "dsk");
			break;
	
		default:
			legacybasicdsk_device_getinfo(devclass, state, info);
			break;
	}
}

static void bw2_serial_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* serial */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_TYPE:
			info->i = IO_SERIAL;
			break;
		case MESS_DEVINFO_INT_READABLE:
			info->i = 1;
			break;
		case MESS_DEVINFO_INT_WRITEABLE:
			info->i = 0;
			break;
		case MESS_DEVINFO_INT_CREATABLE:
			info->i = 0;
			break;
		case MESS_DEVINFO_INT_COUNT:
			info->i = 1;
			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_START:
			info->start = DEVICE_START_NAME(serial_device);
			break;
		case MESS_DEVINFO_PTR_LOAD:
			info->load = DEVICE_IMAGE_LOAD_NAME(bw2_serial);
			break;
		case MESS_DEVINFO_PTR_UNLOAD:
			info->unload = DEVICE_IMAGE_UNLOAD_NAME(serial_device);
			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:
			strcpy(info->s = device_temp_str(), "txt");
			break;
	}
}

SYSTEM_CONFIG_START( bw2 )
	CONFIG_DEVICE( bw2_printer_getinfo )
	CONFIG_DEVICE( bw2_floppy_getinfo )
	CONFIG_DEVICE( bw2_serial_getinfo )
	CONFIG_RAM_DEFAULT( 64 * 1024 )
	CONFIG_RAM( 96 * 1024 )
	CONFIG_RAM( 128 * 1024 )
	CONFIG_RAM( 160 * 1024 )
	CONFIG_RAM( 192 * 1024 )
	CONFIG_RAM( 224 * 1024 )
SYSTEM_CONFIG_END

/*    YEAR  NAME    PARENT  COMPAT  MACHINE   INPUT   INIT    CONFIG  COMPANY      FULLNAME  FLAGS */
COMP( 1985, bw2,    0,      0,      bw2,      bw2,    bw2,    bw2,    "Bondwell",  "BW 2",   GAME_NOT_WORKING )
