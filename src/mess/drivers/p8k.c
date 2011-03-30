/***************************************************************************

    P8000

    09/2009 Skeleton driver based on Matt Knoth's emulator.
    P8000emu (http://knothusa.net/Home.php) has been a great source of info.
    Other info from
      * http://www.pofo.de/P8000/notes/books/
      * http://www.pofo.de/P8000/

    TODO:
      * add Z8001 core so that we can handle the 16bit IO Map (Z8000 uses a
        8bit IO Map, and hence I commented out the whole map)
      * properly implement Z80 daisy chain in 16 bit board

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "cpu/z8000/z8000.h"
#include "cpu/z80/z80daisy.h"
#include "formats/basicdsk.h"
#include "imagedev/flopdrv.h"
#include "machine/upd765.h"
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "machine/z80dma.h"
#include "sound/beep.h"


class p8k_state : public driver_device
{
public:
	p8k_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};



static ADDRESS_MAP_START(p8k_memmap, AS_PROGRAM, 8)
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START(p8k_iomap, AS_IO, 8)
//  AM_RANGE(0x00, 0x07) // MH7489
	AM_RANGE(0x08, 0x0b) AM_DEVREADWRITE("z80ctc_0", z80ctc_r, z80ctc_w)
	AM_RANGE(0x0c, 0x0f) AM_DEVREADWRITE("z80pio_0", z80pio_ba_cd_r, z80pio_ba_cd_w)
	AM_RANGE(0x18, 0x1b) AM_DEVREADWRITE("z80pio_1", z80pio_ba_cd_r, z80pio_ba_cd_w)
	AM_RANGE(0x1c, 0x1f) AM_DEVREADWRITE("z80pio_2", z80pio_ba_cd_r, z80pio_ba_cd_w)
	AM_RANGE(0x20, 0x20) AM_DEVREADWRITE("i8272", upd765_data_r, upd765_data_w)
	AM_RANGE(0x21, 0x21) AM_DEVREAD("i8272", upd765_status_r)
	AM_RANGE(0x24, 0x27) AM_DEVREADWRITE("z80sio_0", z80sio_ba_cd_r, z80sio_ba_cd_w)
	AM_RANGE(0x28, 0x2b) AM_DEVREADWRITE("z80sio_1", z80sio_ba_cd_r, z80sio_ba_cd_w)
	AM_RANGE(0x2c, 0x2f) AM_DEVREADWRITE("z80ctc_1", z80ctc_r, z80ctc_w)
	AM_RANGE(0x3c, 0x3c) AM_DEVREADWRITE("z80dma", z80dma_r, z80dma_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(p8k_16_memmap, AS_PROGRAM, 16)
	AM_RANGE(0x00000, 0x03fff) AM_ROM
	AM_RANGE(0x04000, 0x07fff) AM_RAM
	AM_RANGE(0x08000, 0xfffff) AM_RAM
ADDRESS_MAP_END


#if 0 // we need a real Z8001 CPU core for the 16 bit IO map
// TODO: could any of the following be implemented through a DEVREADWRITE8 + suitable mask?

static READ16_DEVICE_HANDLER( p8k_16_sio_r )
{
	switch (offset & 0x06)
	{
	case 0x00:
		return (UINT16)z80sio_d_r(device, 0);
	case 0x02:
		return (UINT16)z80sio_d_r(device, 1);
	case 0x04:
		return (UINT16)z80sio_c_r(device, 0);
	case 0x06:
		return (UINT16)z80sio_c_r(device, 1);
	}

	return 0;
}

static WRITE16_DEVICE_HANDLER( p8k_16_sio_w )
{
	data &= 0xff;

	switch (offset & 0x06)
	{
	case 0x00:
		z80sio_d_w(device, 0, (UINT8)data);
		break;
	case 0x02:
		z80sio_d_w(device, 1, (UINT8)data);
		break;
	case 0x04:
		z80sio_c_w(device, 0, (UINT8)data);
		break;
	case 0x06:
		z80sio_c_w(device, 1, (UINT8)data);
		break;
	}
}

static READ16_DEVICE_HANDLER( p8k_16_pio_r )
{
	return (UINT16)z80pio_r(device, (offset & 0x06) >> 1);
}

static WRITE16_DEVICE_HANDLER( p8k_16_pio_w )
{
	z80pio_w(device, (offset & 0x06) >> 1, (UINT8)(data & 0xff));
}

static READ16_DEVICE_HANDLER( p8k_16_ctc_r )
{
	return (UINT16)z80ctc_r(device, (offset & 0x06) >> 1);
}

static WRITE16_DEVICE_HANDLER( p8k_16_ctc_w )
{
	z80ctc_w(device, (offset & 0x06) >> 1, (UINT8)(data & 0xff));
}

static ADDRESS_MAP_START(p8k_16_iomap, AS_IO, 16)
//  AM_RANGE(0x0fef0, 0x0feff) // clock
	AM_RANGE(0x0ff80, 0x0ff87) AM_DEVREADWRITE("z80sio_0", p8k_16_sio_r, p8k_16_sio_w)
	AM_RANGE(0x0ff88, 0x0ff8f) AM_DEVREADWRITE("z80sio_1", p8k_16_sio_r, p8k_16_sio_w)
	AM_RANGE(0x0ff90, 0x0ff97) AM_DEVREADWRITE("z80pio_0", p8k_16_pio_r, p8k_16_pio_w)
	AM_RANGE(0x0ff98, 0x0ff9f) AM_DEVREADWRITE("z80pio_1", p8k_16_pio_r, p8k_16_pio_w)
	AM_RANGE(0x0ffa0, 0x0ffa7) AM_DEVREADWRITE("z80pio_2", p8k_16_pio_r, p8k_16_pio_w)
	AM_RANGE(0x0ffa8, 0x0ffaf) AM_DEVREADWRITE("z80ctc_0", p8k_16_ctc_r, p8k_16_ctc_w)
	AM_RANGE(0x0ffb0, 0x0ffb7) AM_DEVREADWRITE("z80ctc_1", p8k_16_ctc_r, p8k_16_ctc_w)
//  AM_RANGE(0x0ffc0, 0x0ffc1) // SCR
//  AM_RANGE(0x0ffc8, 0x0ffc9) // SBR
//  AM_RANGE(0x0ffd0, 0x0ffd1) // NBR
//  AM_RANGE(0x0ffd8, 0x0ffd9) // SNVR
//  AM_RANGE(0x0ffe0, 0x0ffe1) // RETI
//  AM_RANGE(0x0fff0, 0x0fff1) // TRPL
//  AM_RANGE(0x0fff8, 0x0fff9) // IF1L
ADDRESS_MAP_END

#endif


/* Input ports */
static INPUT_PORTS_START( p8k )
INPUT_PORTS_END


static MACHINE_RESET( p8k )
{
}

static MACHINE_RESET( p8k_16 )
{
}

static VIDEO_START( p8k )
{
}

static SCREEN_UPDATE( p8k )
{
    return 0;
}


/***************************************************************************

    P8000 8bit Peripherals

****************************************************************************/

static void p8k_daisy_interrupt(device_t *device, int state)
{
	cputag_set_input_line(device->machine(), "maincpu", 0, state);
}

/* Z80 DMA */

static WRITE_LINE_DEVICE_HANDLER( p8k_dma_irq_w )
{
	if (state)
	{
		device_t *i8272 = device->machine().device("i8272");
		upd765_tc_w(i8272, state);
	}

	p8k_daisy_interrupt(device, state);
}

static UINT8 memory_read_byte(address_space *space, offs_t address) { return space->read_byte(address); }
static void memory_write_byte(address_space *space, offs_t address, UINT8 data) { space->write_byte(address, data); }

static Z80DMA_INTERFACE( p8k_dma_intf )
{
	DEVCB_LINE(p8k_dma_irq_w),
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),
	DEVCB_NULL,
	DEVCB_MEMORY_HANDLER("maincpu", PROGRAM, memory_read_byte),
	DEVCB_MEMORY_HANDLER("maincpu", PROGRAM, memory_write_byte),
	DEVCB_MEMORY_HANDLER("maincpu", IO, memory_read_byte),
	DEVCB_MEMORY_HANDLER("maincpu", IO, memory_write_byte)
};

/* Z80 CTC 0 */
// to implement: callbacks!
// manual states the callbacks should go to
// Baud Gen 3, FDC, System-Kanal

static Z80CTC_INTERFACE( p8k_ctc_0_intf )
{
	0,			/* timer disables */
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),	/* interrupt handler */
	DEVCB_NULL,			/* ZC/TO0 callback */
	DEVCB_NULL,			/* ZC/TO1 callback */
	DEVCB_NULL  		/* ZC/TO2 callback */
};

/* Z80 CTC 1 */
// to implement: callbacks!
// manual states the callbacks should go to
// Baud Gen 0, Baud Gen 1, Baud Gen 2,

static Z80CTC_INTERFACE( p8k_ctc_1_intf )
{
	0,			/* timer disables */
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),	/* interrupt handler */
	DEVCB_NULL,			/* ZC/TO0 callback */
	DEVCB_NULL,			/* ZC/TO1 callback */
	DEVCB_NULL,			/* ZC/TO2 callback */
};

/* Z80 PIO 0 */

static Z80PIO_INTERFACE( p8k_pio_0_intf )
{
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/* Z80 PIO 1 */

static Z80PIO_INTERFACE( p8k_pio_1_intf )
{
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/* Z80 PIO 2 */

static Z80PIO_INTERFACE( p8k_pio_2_intf )
{
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/* Z80 SIO 0 */

static WRITE8_DEVICE_HANDLER( pk8_sio_0_serial_transmit )
{
// send character to terminal
}

static const z80sio_interface p8k_sio_0_intf =
{
	p8k_daisy_interrupt,			/* interrupt handler */
	NULL,					/* DTR changed handler */
	NULL,					/* RTS changed handler */
	NULL,					/* BREAK changed handler */
	pk8_sio_0_serial_transmit,	/* transmit handler */
	NULL					/* receive handler */
};

/* Z80 SIO 1 */

static WRITE8_DEVICE_HANDLER( pk8_sio_1_serial_transmit )
{
// send character to terminal
}

static const z80sio_interface p8k_sio_1_intf =
{
	p8k_daisy_interrupt,			/* interrupt handler */
	NULL,					/* DTR changed handler */
	NULL,					/* RTS changed handler */
	NULL,					/* BREAK changed handler */
	pk8_sio_1_serial_transmit,	/* transmit handler */
	NULL					/* receive handler */
};

/* Z80 Daisy Chain */

static const z80_daisy_config p8k_daisy_chain[] =
{
	{ "z80dma" },	/* FDC related */
	{ "z80pio_2" },
	{ "z80ctc_0" },
	{ "z80sio_0" },
	{ "z80sio_1" },
	{ "z80pio_0" },
	{ "z80pio_1" },
	{ "z80ctc_1" },
	{ NULL }
};

/* Intel 8272 Interface */

static WRITE_LINE_DEVICE_HANDLER( p8k_i8272_irq_w )
{
	device_t *z80pio = device->machine().device("z80pio_2");

	z80pio_pb_w(z80pio, 0, (state) ? 0x10 : 0x00);
}

static const struct upd765_interface p8k_i8272_intf =
{
	DEVCB_LINE(p8k_i8272_irq_w),
	DEVCB_DEVICE_LINE("z80dma", z80dma_rdy_w),
	NULL,
	UPD765_RDY_PIN_CONNECTED,
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};

static const floppy_config p8k_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(default),
	NULL
};

/***************************************************************************

    P8000 16bit Peripherals

****************************************************************************/

static void p8k_16_daisy_interrupt(device_t *device, int state)
{
	// this must be studied a little bit more :-)
}

/* Z80 CTC 0 */

static Z80CTC_INTERFACE( p8k_16_ctc_0_intf )
{
	0,				/* timer disables */
	DEVCB_LINE(p8k_16_daisy_interrupt),	/* interrupt handler */
	DEVCB_NULL,				/* ZC/TO0 callback */
	DEVCB_NULL,				/* ZC/TO1 callback */
	DEVCB_NULL  			/* ZC/TO2 callback */
};

/* Z80 CTC 1 */

static Z80CTC_INTERFACE( p8k_16_ctc_1_intf )
{
	0,				/* timer disables */
	DEVCB_LINE(p8k_16_daisy_interrupt),	/* interrupt handler */
	DEVCB_NULL,				/* ZC/TO0 callback */
	DEVCB_NULL,				/* ZC/TO1 callback */
	DEVCB_NULL				/* ZC/TO2 callback */
};

/* Z80 PIO 0 */

static const z80pio_interface p8k_16_pio_0_intf =
{
	DEVCB_LINE(p8k_16_daisy_interrupt),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/* Z80 PIO 1 */

static const z80pio_interface p8k_16_pio_1_intf =
{
	DEVCB_LINE(p8k_16_daisy_interrupt),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/* Z80 PIO 2 */

static const z80pio_interface p8k_16_pio_2_intf =
{
	DEVCB_LINE(p8k_16_daisy_interrupt),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/* Z80 SIO 0 */

static WRITE8_DEVICE_HANDLER( pk8_16_sio_0_serial_transmit )
{
// send character to terminal
}

static const z80sio_interface p8k_16_sio_0_intf =
{
	p8k_16_daisy_interrupt,			/* interrupt handler */
	NULL,					/* DTR changed handler */
	NULL,					/* RTS changed handler */
	NULL,					/* BREAK changed handler */
	pk8_16_sio_0_serial_transmit,	/* transmit handler */
	NULL					/* receive handler */
};

/* Z80 SIO 1 */

static WRITE8_DEVICE_HANDLER( pk8_16_sio_1_serial_transmit )
{
// send character to terminal
}

static const z80sio_interface p8k_16_sio_1_intf =
{
	p8k_16_daisy_interrupt,			/* interrupt handler */
	NULL,					/* DTR changed handler */
	NULL,					/* RTS changed handler */
	NULL,					/* BREAK changed handler */
	pk8_16_sio_1_serial_transmit,	/* transmit handler */
	NULL					/* receive handler */
};

/* Z80 Daisy Chain */

static const z80_daisy_config p8k_16_daisy_chain[] =
{
	{ "z80ctc_0" },
	{ "z80ctc_1" },
	{ "z80sio_0" },
	{ "z80sio_1" },
	{ "z80pio_0" },
	{ "z80pio_1" },
	{ "z80pio_2" },
	{ NULL }
};

/* F4 Character Displayer */
static const gfx_layout p8k_charlayout =
{
	8, 12,					/* 8 x 12 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8 },
	8*16					/* every char takes 16 bytes */
};

static GFXDECODE_START( p8k )
	GFXDECODE_ENTRY( "chargen", 0x0000, p8k_charlayout, 0, 1 )
GFXDECODE_END


/***************************************************************************

    Machine Drivers

****************************************************************************/

static MACHINE_CONFIG_START( p8k, p8k_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, XTAL_4MHz )
	MCFG_CPU_CONFIG(p8k_daisy_chain)
	MCFG_CPU_PROGRAM_MAP(p8k_memmap)
	MCFG_CPU_IO_MAP(p8k_iomap)

//  MCFG_MACHINE_START( p8000_8 )
	MCFG_MACHINE_RESET(p8k)

	/* peripheral hardware */
	MCFG_Z80DMA_ADD("z80dma", XTAL_4MHz, p8k_dma_intf)
	MCFG_Z80CTC_ADD("z80ctc_0", 1229000, p8k_ctc_0_intf)	/* 1.22MHz clock */
	MCFG_Z80CTC_ADD("z80ctc_1", 1229000, p8k_ctc_1_intf)	/* 1.22MHz clock */
	MCFG_Z80SIO_ADD("z80sio_0", 9600, p8k_sio_0_intf)	/* 9.6kBaud default */
	MCFG_Z80SIO_ADD("z80sio_1", 9600, p8k_sio_1_intf)	/* 9.6kBaud default */
	MCFG_Z80PIO_ADD("z80pio_0", 1229000, p8k_pio_0_intf)
	MCFG_Z80PIO_ADD("z80pio_1", 1229000, p8k_pio_1_intf)
	MCFG_Z80PIO_ADD("z80pio_2", 1229000, p8k_pio_2_intf)
	MCFG_UPD765A_ADD("i8272", p8k_i8272_intf)
	MCFG_FLOPPY_2_DRIVES_ADD(p8k_floppy_config)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_SOUND_ADD("beep", BEEP, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_RGB15)
	MCFG_SCREEN_REFRESH_RATE(15)
	MCFG_SCREEN_SIZE(640,480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_SCREEN_UPDATE(p8k)

	MCFG_GFXDECODE(p8k)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)

	MCFG_VIDEO_START(p8k)
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( p8k_16, p8k_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z8001, XTAL_4MHz )	// actually z8001, appropriate changes pending
	MCFG_CPU_CONFIG(p8k_16_daisy_chain)
	MCFG_CPU_PROGRAM_MAP(p8k_16_memmap)
//  MCFG_CPU_IO_MAP(p8k_16_iomap)

//  MCFG_MACHINE_START( p8000_16 )
	MCFG_MACHINE_RESET(p8k_16)

	/* peripheral hardware */
	MCFG_Z80CTC_ADD("z80ctc_0", XTAL_4MHz, p8k_16_ctc_0_intf)
	MCFG_Z80CTC_ADD("z80ctc_1", XTAL_4MHz, p8k_16_ctc_1_intf)
	MCFG_Z80SIO_ADD("z80sio_0", 9600, p8k_16_sio_0_intf)
	MCFG_Z80SIO_ADD("z80sio_1", 9600, p8k_16_sio_1_intf)
	MCFG_Z80PIO_ADD("z80pio_0", XTAL_4MHz, p8k_16_pio_0_intf )
	MCFG_Z80PIO_ADD("z80pio_1", XTAL_4MHz, p8k_16_pio_1_intf )
	MCFG_Z80PIO_ADD("z80pio_2", XTAL_4MHz, p8k_16_pio_2_intf )

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_SOUND_ADD("beep", BEEP, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_RGB15)
	MCFG_SCREEN_REFRESH_RATE(15)
	MCFG_SCREEN_SIZE(640,480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_SCREEN_UPDATE(p8k)

	MCFG_GFXDECODE(p8k)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)

	MCFG_VIDEO_START(p8k)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( p8000 )
	ROM_REGION( 0x2000, "maincpu", 0 )
	ROM_LOAD("mon8_1_3.1",	0x0000, 0x1000, CRC(ad1bb118) SHA1(2332963acd74d5d1a009d9bce8a2b108de01d2a5))
	ROM_LOAD("mon8_2_3.1",	0x1000, 0x1000, CRC(daced7c2) SHA1(f1f778e72568961b448020fc543ed6e81bbe81b1))

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD("p8t_zs",    0x0000, 0x0800, CRC(f9321251) SHA1(a6a796b58d50ec4a416f2accc34bd76bc83f18ea))
	ROM_LOAD("p8tdzs.2",  0x0800, 0x0800, CRC(32736503) SHA1(6a1d7c55dddc64a7d601dfdbf917ce1afaefbb0a))
ROM_END

ROM_START( p8000_16 )
	ROM_REGION16_BE( 0x4000, "maincpu", 0 )
	ROM_LOAD16_BYTE("mon16_1h_3.1_udos",   0x0000, 0x1000, CRC(0c3c28da) SHA1(0cd35444c615b404ebb9cf80da788593e573ddb5))
	ROM_LOAD16_BYTE("mon16_1l_3.1_udos",   0x0001, 0x1000, CRC(e8857bdc) SHA1(f89c65cbc479101130c71806fd3ddc28e6383f12))
	ROM_LOAD16_BYTE("mon16_2h_3.1_udos",   0x2000, 0x1000, CRC(cddf58d5) SHA1(588bad8df75b99580459c7a8e898a3396907e3a4))
	ROM_LOAD16_BYTE("mon16_2l_3.1_udos",   0x2001, 0x1000, CRC(395ee7aa) SHA1(d72fadb1608cd0915cd5ce6440897303ac5a12a6))

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD("p8t_zs",    0x0000, 0x0800, CRC(f9321251) SHA1(a6a796b58d50ec4a416f2accc34bd76bc83f18ea))
	ROM_LOAD("p8tdzs.2",  0x0800, 0x0800, CRC(32736503) SHA1(6a1d7c55dddc64a7d601dfdbf917ce1afaefbb0a))
ROM_END

/* Driver */

/*    YEAR  NAME        PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY                   FULLNAME       FLAGS */
COMP( 1989, p8000,      0,      0,       p8k,       p8k,     0,      "EAW electronic Treptow", "P8000 (8bit Board)",  GAME_NOT_WORKING)
COMP( 1989, p8000_16,   p8000,  0,       p8k_16,    p8k,     0,      "EAW electronic Treptow", "P8000 (16bit Board)",  GAME_NOT_WORKING)
