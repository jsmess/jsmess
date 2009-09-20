/***************************************************************************
   
        Nanos

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "machine/z80ctc.h"
#include "machine/nec765.h"
#include "devices/flopdrv.h"
#include "formats/basicdsk.h"

static const UINT8 *FNT;

static ADDRESS_MAP_START(nanos_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x0fff ) AM_READWRITE(SMH_BANK(1), SMH_BANK(3))
	AM_RANGE( 0x1000, 0xffff ) AM_RAMBANK(2)
ADDRESS_MAP_END

static WRITE8_HANDLER(nanos_tc_w)
{
	const device_config *fdc = devtag_get_device(space->machine, "nec765");
	nec765_tc_w(fdc, BIT(data,1));
}


/* Z80-CTC Interface */

static void z80daisy_interrupt(const device_config *device, int state)
{
	cputag_set_input_line(device->machine, "maincpu", INPUT_LINE_IRQ0, state);
}

static WRITE8_DEVICE_HANDLER( ctc_z0_w )
{
}

static WRITE8_DEVICE_HANDLER( ctc_z1_w )
{
}

static WRITE8_DEVICE_HANDLER( ctc_z2_w )
{
}

static const z80ctc_interface ctc_intf =
{
	0,              	/* timer disables */
	z80daisy_interrupt,	/* interrupt handler */
	ctc_z0_w,			/* ZC/TO0 callback */
	ctc_z1_w,			/* ZC/TO1 callback */
	ctc_z2_w    		/* ZC/TO2 callback */
};

/* Z80-PIO Interface */

static const z80pio_interface pio1_intf =
{
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),	/* callback when change interrupt status */
	DEVCB_NULL,						/* port A read callback */
	DEVCB_NULL,						/* port B read callback */
	DEVCB_NULL,						/* port A write callback */
	DEVCB_NULL,						/* port B write callback */
	DEVCB_NULL,						/* portA ready active callback */
	DEVCB_NULL						/* portB ready active callback */
};

static const z80pio_interface pio2_intf =
{
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),	/* callback when change interrupt status */
	DEVCB_NULL,						/* port A read callback */
	DEVCB_NULL,						/* port B read callback */
	DEVCB_NULL,						/* port A write callback */
	DEVCB_NULL,						/* port B write callback */
	DEVCB_NULL,						/* portA ready active callback */
	DEVCB_NULL						/* portB ready active callback */
};

/* Z80-SIO Interface */

static const z80sio_interface sio_intf =
{
	z80daisy_interrupt,	/* interrupt handler */
	NULL,				/* DTR changed handler */
	NULL,				/* RTS changed handler */
	NULL,				/* BREAK changed handler */
	NULL,				/* transmit handler */
	NULL				/* receive handler */
};

/* Z80 Daisy Chain */

static const z80_daisy_chain nanos_daisy_chain[] =
{
	{ "z80pio" },
	{ "z80pio_0" },
	{ "z80pio_1" },
	{ "z80sio_0" },
	{ "z80ctc_0" },
	{ "z80sio_1" },
	{ "z80ctc_1" },
	{ NULL }
};
static ADDRESS_MAP_START( nanos_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)	
	/* CPU card */
	AM_RANGE(0x00, 0x03) AM_DEVREADWRITE("z80pio", z80pio_r, z80pio_w)
	
	/* I/O card */
	AM_RANGE(0x80, 0x83) AM_DEVREADWRITE("z80pio_0", z80pio_r, z80pio_w)
	AM_RANGE(0x84, 0x84) AM_DEVREADWRITE("z80sio_0", z80sio_d_r, z80sio_d_w)
	AM_RANGE(0x85, 0x85) AM_DEVREADWRITE("z80sio_0", z80sio_c_r, z80sio_c_w)
	AM_RANGE(0x86, 0x86) AM_DEVREADWRITE("z80sio_0", z80sio_d_r, z80sio_d_w)
	AM_RANGE(0x87, 0x87) AM_DEVREADWRITE("z80sio_0", z80sio_c_r, z80sio_c_w)
	AM_RANGE(0x88, 0x8B) AM_DEVREADWRITE("z80pio_1", z80pio_r, z80pio_w)
	AM_RANGE(0x8C, 0x8F) AM_DEVREADWRITE("z80ctc_0", z80ctc_r, z80ctc_w)
	
	/* FDC card */
	AM_RANGE(0x92, 0x92) AM_WRITE(nanos_tc_w)
	AM_RANGE(0x94, 0x94) AM_DEVREAD("nec765", nec765_status_r)
	AM_RANGE(0x95, 0x95) AM_DEVREADWRITE("nec765", nec765_data_r, nec765_data_w)
	/* V24+IFSS card */
	AM_RANGE(0xA0, 0xA0) AM_DEVREADWRITE("z80sio_1", z80sio_d_r, z80sio_d_w)
	AM_RANGE(0xA1, 0xA1) AM_DEVREADWRITE("z80sio_1", z80sio_c_r, z80sio_c_w)
	AM_RANGE(0xA2, 0xA2) AM_DEVREADWRITE("z80sio_1", z80sio_d_r, z80sio_d_w)
	AM_RANGE(0xA3, 0xA3) AM_DEVREADWRITE("z80sio_1", z80sio_c_r, z80sio_c_w)
	AM_RANGE(0xA4, 0xA7) AM_DEVREADWRITE("z80ctc_1", z80ctc_r, z80ctc_w)
	
	/* 256-k RAM card I  - 	64k OS-Memory + 192k-RAM-Floppy */
	//AM_RANGE(0xC0, 0xC7)
	
	/* 256-k RAM card II - 	64k OS-Memory + 192k-RAM-Floppy */
	//AM_RANGE(0xC8, 0xCF)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( nanos )
	PORT_START("LINEC")	
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("LINE0")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)

	PORT_START("LINE1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(":") PORT_CODE(KEYCODE_QUOTE) 
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON) 
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA) 
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS) 
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH) 

	PORT_START("LINE2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("@") PORT_CODE(KEYCODE_END)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)

	PORT_START("LINE3")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
                                                 
	PORT_START("LINE4")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
                        
	PORT_START("LINE5")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_OPENBRACE) 
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\\") PORT_CODE(KEYCODE_BACKSLASH)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("]")  PORT_CODE(KEYCODE_CLOSEBRACE)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("~")  PORT_CODE(KEYCODE_TILDE)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("DEL")PORT_CODE(KEYCODE_BACKSPACE)
	
	PORT_START("LINE6")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_UP)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_DOWN)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("LF") PORT_CODE(KEYCODE_RALT) 
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB) 
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER) 	
INPUT_PORTS_END


static VIDEO_START( nanos )
{
	FNT = memory_region(machine, "gfx1");
}

static VIDEO_UPDATE( nanos )
{
//	static UINT8 framecnt=0;
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x;

//	framecnt++;

	for (y = 0; y < 25; y++)
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + 80; x++)
			{
				if (ra < 8)
				{
					chr = mess_ram[0xf800+ x];

					/* get pattern of pixels for that character scanline */
					gfx = FNT[(chr<<3) | ra ];
				}
				else
					gfx = 0;

				/* Display a scanline of a character (8 pixels) */
				*p = ( gfx & 0x80 ) ? 1 : 0; p++;
				*p = ( gfx & 0x40 ) ? 1 : 0; p++;
				*p = ( gfx & 0x20 ) ? 1 : 0; p++;
				*p = ( gfx & 0x10 ) ? 1 : 0; p++;
				*p = ( gfx & 0x08 ) ? 1 : 0; p++;
				*p = ( gfx & 0x04 ) ? 1 : 0; p++;
				*p = ( gfx & 0x02 ) ? 1 : 0; p++;
				*p = ( gfx & 0x01 ) ? 1 : 0; p++;
			}
		}
		ma+=80;
	}
	return 0;
}

UINT8 key_command;
UINT8 last_code = 0;
UINT8 key_pressed = 0xff;
static READ8_DEVICE_HANDLER (nanos_port_a_r)
{	
	UINT8 retVal;
	if (key_command==0)  {
		return key_pressed;
	} else {
		retVal = last_code;
		last_code = 0;
		return retVal;
	}
}

static READ8_DEVICE_HANDLER (nanos_port_b_r)
{
	return 0xff;
}


static WRITE8_DEVICE_HANDLER (nanos_port_b_w)
{
	key_command = BIT(data,1); 
	if (BIT(data,7)) {
		memory_set_bankptr(device->machine, 1, memory_region(device->machine, "maincpu"));
	} else {
		memory_set_bankptr(device->machine, 1, mess_ram);
	}
}
static UINT8 row_number(UINT8 code) {
	if BIT(code,0) return 0;
	if BIT(code,1) return 1;
	if BIT(code,2) return 2;
	if BIT(code,3) return 3;
	if BIT(code,4) return 4;
	if BIT(code,5) return 5;
	if BIT(code,6) return 6;
	if BIT(code,7) return 7;	
	return 0;
}

static TIMER_CALLBACK(keyboard_callback)
{
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6" };
	
	int i;
	UINT8 code;
	UINT8 key_code = 0;
	UINT8 shift = input_port_read(machine, "LINEC") & 0x02 ? 1 : 0;
	UINT8 ctrl =  input_port_read(machine, "LINEC") & 0x01 ? 1 : 0;
	key_pressed = 0xff;	
	for(i = 0; i < 7; i++) 
	{
		
		code = 	input_port_read(machine, keynames[i]);
		if (code != 0) 
		{
			if (i==0 && shift==0) {
				key_code = 0x30 + row_number(code) + 8*i; // for numbers and some signs
			}
			if (i==0 && shift==1) {
				key_code = 0x20 + row_number(code) + 8*i; // for shifted numbers
			}
			if (i==1 && shift==0) {
				if (row_number(code) < 4) {
					key_code = 0x30 + row_number(code) + 8*i; // for numbers and some signs
				} else {
					key_code = 0x20 + row_number(code) + 8*i; // for numbers and some signs
				}
			}
			if (i==1 && shift==1) {
				if (row_number(code) < 4) {
					key_code = 0x20 + row_number(code) + 8*i; // for numbers and some signs
				} else {
					key_code = 0x30 + row_number(code) + 8*i; // for numbers and some signs
				}
			}
			if (i>=2 && i<=4 && shift==1 && ctrl==0) {
				key_code = 0x60 + row_number(code) + (i-2)*8; // for small letters
			}
			if (i>=2 && i<=4 && shift==0 && ctrl==0) {
				key_code = 0x40 + row_number(code) + (i-2)*8; // for big letters
			}
			if (i>=2 && i<=4 && ctrl==1) {
				key_code = 0x00 + row_number(code) + (i-2)*8; // for CTRL + letters
			}
			if (i==5 && shift==1 && ctrl==0) {
				if (row_number(code)<7) {
					key_code = 0x60 + row_number(code) + (i-2)*8; // for small letters
				} else {
					key_code = 0x40 + row_number(code) + (i-2)*8; // for signs it is switched
				}
			}
			if (i==5 && shift==0 && ctrl==0) {
				if (row_number(code)<7) {
					key_code = 0x40 + row_number(code) + (i-2)*8; // for small letters
				} else {
					key_code = 0x60 + row_number(code) + (i-2)*8; // for signs it is switched
				}
			}
			if (i==5 && shift==0 && ctrl==1) {
				key_code = 0x00 + row_number(code) + (i-2)*8; // for letters + ctrl
			}
			if (i==6) {
				switch(row_number(code)) 
				{
					case 0: key_code = 0x11; break;
					case 1: key_code = 0x12; break;
					case 2: key_code = 0x13; break;
					case 3: key_code = 0x14; break;
					case 4: key_code = 0x20; break; // Space
					case 5: key_code = 0x0D; break; // Enter
					case 6: key_code = 0x09; break; // TAB
					case 7: key_code = 0x0A; break; // LF
				}
			}
			last_code = key_code;
		}
	}	
	if (key_code==0){		
		key_pressed = 0xf7;				
	}
}

static MACHINE_RESET(nanos) 
{	
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	
	memory_install_write8_handler(space, 0x0000, 0x0fff, 0, 0, SMH_BANK(3));		
	memory_install_write8_handler(space, 0x1000, 0xffff, 0, 0, SMH_BANK(2));	
	
	memory_set_bankptr(machine, 1, memory_region(machine, "maincpu"));
	memory_set_bankptr(machine, 2, mess_ram + 0x1000);
	memory_set_bankptr(machine, 3, mess_ram);
	
	floppy_drive_set_motor_state(floppy_get_device(space->machine, 0), 1);
	
	floppy_drive_set_ready_state(floppy_get_device(space->machine, 0), 1,1);
	timer_pulse(machine, ATTOTIME_IN_HZ(24000), NULL, 0, keyboard_callback);
}

static const z80pio_interface nanos_z80pio_intf =
{
	DEVCB_NULL,	/* callback when change interrupt status */
	DEVCB_HANDLER(nanos_port_a_r),
	DEVCB_HANDLER(nanos_port_b_r),
	DEVCB_NULL,
	DEVCB_HANDLER(nanos_port_b_w),
	DEVCB_NULL,
	DEVCB_NULL
};


static const nec765_interface nanos_nec765_interface =
{
	DEVCB_NULL,
	NULL,
	NULL,
	NEC765_RDY_PIN_NOT_CONNECTED,
	{FLOPPY_0,FLOPPY_1, FLOPPY_2, FLOPPY_3}
};

static FLOPPY_OPTIONS_START(nanos)
	FLOPPY_OPTION(nanos, "img", "NANOS disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([5])
		SECTOR_LENGTH([1024])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static const floppy_config nanos_floppy_config =
{
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(nanos),
	DO_NOT_KEEP_GEOMETRY
};

static MACHINE_DRIVER_START( nanos )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(nanos_mem)
    MDRV_CPU_IO_MAP(nanos_io)	
    MDRV_CPU_CONFIG(nanos_daisy_chain)


    MDRV_MACHINE_RESET(nanos)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(80*8, 25*10)
	MDRV_SCREEN_VISIBLE_AREA(0,80*8-1,0,25*10-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(nanos)
    MDRV_VIDEO_UPDATE(nanos)
    
    /* devices */
	MDRV_Z80CTC_ADD( "z80ctc_0", XTAL_4MHz, ctc_intf)
	MDRV_Z80CTC_ADD( "z80ctc_1", XTAL_4MHz, ctc_intf)
	MDRV_Z80PIO_ADD( "z80pio_0", pio1_intf)
	MDRV_Z80PIO_ADD( "z80pio_1", pio2_intf)
	MDRV_Z80SIO_ADD( "z80sio_0", XTAL_4MHz, sio_intf)
	MDRV_Z80SIO_ADD( "z80sio_1", XTAL_4MHz, sio_intf)
	MDRV_Z80PIO_ADD( "z80pio", nanos_z80pio_intf )
	/* NEC765 */
	MDRV_NEC765A_ADD("nec765", nanos_nec765_interface)    
	
	MDRV_FLOPPY_4_DRIVES_ADD(nanos_floppy_config)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(nanos)
	CONFIG_RAM_DEFAULT(64 * 1024)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( nanos )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "k7634_1.rom", 0x0000, 0x0800, CRC(8e34e6ac) SHA1(fd342f6effe991823c2a310737fbfcba213c4fe3))
	ROM_LOAD( "k7634_2.rom", 0x0800, 0x0180, CRC(4e01b02b) SHA1(8a279da886555c7470a1afcbb3a99693ea13c237))

	ROM_REGION( 0x0800, "gfx1", 0 )
	ROM_LOAD( "zg_nanos.rom", 0x0000, 0x0800, CRC(5682d3f9) SHA1(5b738972c815757821c050ee38b002654f8da163))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, nanos,  0,       0, 	nanos, 	nanos, 	 0,  	  nanos,  	 "Ingenieurhochschule fur Seefahrt Warnemunde/Wustrow",   "Nanos",		GAME_NOT_WORKING)

