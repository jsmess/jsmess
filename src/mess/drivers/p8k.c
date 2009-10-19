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
      * add floppy emulation to 8 bit board
      * properly implement Z80 daisy chain in 16 bit board

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/z8000/z8000.h"
#include "cpu/z80/z80daisy.h"
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "machine/z80dma.h"
#include "sound/beep.h"


static READ8_DEVICE_HANDLER( sio2_r )
{
	switch (offset)
	{
	case 0:
		return z80sio_d_r(device, 0);
	case 1:
		return z80sio_c_r(device, 0);
	case 2:
		return z80sio_d_r(device, 1);
	case 3:
		return z80sio_c_r(device, 1);
	}

	return 0;
}

static WRITE8_DEVICE_HANDLER( sio2_w )
{
	switch (offset)
	{
	case 0:
		z80sio_d_w(device, 0, data);
		break;
	case 1:
		z80sio_c_w(device, 0, data);
		break;
	case 2:
		z80sio_d_w(device, 1, data);
		break;
	case 3:
		z80sio_c_w(device, 1, data);
		break;
	}
}

static ADDRESS_MAP_START(p8k_memmap, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START(p8k_iomap, ADDRESS_SPACE_IO, 8)
//  AM_RANGE(0x00, 0x07) // MH7489
	AM_RANGE(0x08, 0x0b) AM_DEVREADWRITE("z80ctc_0", z80ctc_r, z80ctc_w)
	AM_RANGE(0x0c, 0x0f) AM_DEVREADWRITE("z80pio_0", z80pio_alt_r, z80pio_alt_w)
	AM_RANGE(0x18, 0x1b) AM_DEVREADWRITE("z80pio_1", z80pio_alt_r, z80pio_alt_w)
	AM_RANGE(0x1c, 0x1f) AM_DEVREADWRITE("z80pio_2", z80pio_alt_r, z80pio_alt_w)	// we still need to add the FDC related part of this one
//  AM_RANGE(0x20, 0x23) // FDC
	AM_RANGE(0x24, 0x27) AM_DEVREADWRITE("z80sio_0", sio2_r, sio2_w)
	AM_RANGE(0x28, 0x2b) AM_DEVREADWRITE("z80sio_1", sio2_r, sio2_w)
	AM_RANGE(0x2c, 0x2f) AM_DEVREADWRITE("z80ctc_1", z80ctc_r, z80ctc_w)
	AM_RANGE(0x3c, 0x3c) AM_DEVREADWRITE("z80dma", z80dma_r, z80dma_w)	// FDC related
ADDRESS_MAP_END

static ADDRESS_MAP_START(p8k_16_memmap, ADDRESS_SPACE_PROGRAM, 16)
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

static ADDRESS_MAP_START(p8k_16_iomap, ADDRESS_SPACE_IO, 16)
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

static VIDEO_UPDATE( p8k )
{
    return 0;
}


/***************************************************************************

    P8000 8bit Peripherals

****************************************************************************/

static void p8k_daisy_interrupt(const device_config *device, int state)
{
	cputag_set_input_line(device->machine, "maincpu", 0, state);
}

/* Z80 DMA */

static READ8_DEVICE_HANDLER(p8k_dma_read_byte)
{
	const address_space *space = cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	return memory_read_byte(space, offset);
}

static WRITE8_DEVICE_HANDLER(p8k_dma_write_byte)
{
	const address_space *space = cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	memory_write_byte(space, offset, data);
}

// still to implement: dma io read/write which handle FDC. also, the interrupt should deal with FDC as well, with p8k_daisy_interrupt as a fallback
static const z80dma_interface p8k_dma_intf =
{
	"maincpu",
	p8k_dma_read_byte,
	p8k_dma_write_byte,
	NULL, NULL, NULL, NULL,
	p8k_daisy_interrupt
};

/* Z80 CTC 0 */
// to implement: callbacks!
// manual states the callbacks should go to
// Baud Gen 3, FDC, System-Kanal

static const z80ctc_interface p8k_ctc_0_intf =
{
	0,			/* timer disables */
	p8k_daisy_interrupt,	/* interrupt handler */
	NULL,			/* ZC/TO0 callback */
	NULL,			/* ZC/TO1 callback */
	NULL    		/* ZC/TO2 callback */
};

/* Z80 CTC 1 */
// to implement: callbacks!
// manual states the callbacks should go to
// Baud Gen 0, Baud Gen 1, Baud Gen 2,

static const z80ctc_interface p8k_ctc_1_intf =
{
	0,			/* timer disables */
	p8k_daisy_interrupt,	/* interrupt handler */
	NULL,			/* ZC/TO0 callback */
	NULL,			/* ZC/TO1 callback */
	NULL,			/* ZC/TO2 callback */
};

/* Z80 PIO 0 */

static const z80pio_interface p8k_pio_0_intf =
{
	DEVCB_LINE(p8k_daisy_interrupt),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/* Z80 PIO 1 */

static const z80pio_interface p8k_pio_1_intf =
{
	DEVCB_LINE(p8k_daisy_interrupt),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/* Z80 PIO 2 */

static const z80pio_interface p8k_pio_2_intf =
{
	DEVCB_LINE(p8k_daisy_interrupt),
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

static const z80_daisy_chain p8k_daisy_chain[] =
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


/***************************************************************************

    P8000 16bit Peripherals

****************************************************************************/

static void p8k_16_daisy_interrupt(const device_config *device, int state)
{
	// this must be studied a little bit more :-)
}

/* Z80 CTC 0 */

static const z80ctc_interface p8k_16_ctc_0_intf =
{
	0,				/* timer disables */
	p8k_16_daisy_interrupt,	/* interrupt handler */
	NULL,				/* ZC/TO0 callback */
	NULL,				/* ZC/TO1 callback */
	NULL    			/* ZC/TO2 callback */
};

/* Z80 CTC 1 */

static const z80ctc_interface p8k_16_ctc_1_intf =
{
	0,				/* timer disables */
	p8k_16_daisy_interrupt,	/* interrupt handler */
	NULL,				/* ZC/TO0 callback */
	NULL,				/* ZC/TO1 callback */
	NULL,				/* ZC/TO2 callback */
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

static const z80_daisy_chain p8k_16_daisy_chain[] =
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

/***************************************************************************

    Machine Drivers

****************************************************************************/

static MACHINE_DRIVER_START( p8k )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, XTAL_4MHz )
	MDRV_CPU_CONFIG(p8k_daisy_chain)
	MDRV_CPU_PROGRAM_MAP(p8k_memmap)
	MDRV_CPU_IO_MAP(p8k_iomap)

//  MDRV_MACHINE_START( p8000_8 )
	MDRV_MACHINE_RESET(p8k)

	/* peripheral hardware */
	MDRV_Z80DMA_ADD("z80dma", XTAL_4MHz, p8k_dma_intf)
	MDRV_Z80CTC_ADD("z80ctc_0", 1229000, p8k_ctc_0_intf)	/* 1.22MHz clock */
	MDRV_Z80CTC_ADD("z80ctc_1", 1229000, p8k_ctc_1_intf)	/* 1.22MHz clock */
	MDRV_Z80SIO_ADD("z80sio_0", 9600, p8k_sio_0_intf)	/* 9.6kBaud default */
	MDRV_Z80SIO_ADD("z80sio_1", 9600, p8k_sio_1_intf)	/* 9.6kBaud default */
	MDRV_Z80PIO_ADD("z80pio_0", p8k_pio_0_intf)
	MDRV_Z80PIO_ADD("z80pio_1", p8k_pio_1_intf)
	MDRV_Z80PIO_ADD("z80pio_2", p8k_pio_2_intf)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD("beep", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB15)
	MDRV_SCREEN_REFRESH_RATE(15)
	MDRV_SCREEN_SIZE(640,480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(p8k)
	MDRV_VIDEO_UPDATE(p8k)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( p8k_16 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z8000, XTAL_4MHz )	// actually z8001, appropriate changes pending
	MDRV_CPU_CONFIG(p8k_16_daisy_chain)
	MDRV_CPU_PROGRAM_MAP(p8k_16_memmap)
//  MDRV_CPU_IO_MAP(p8k_16_iomap)

//  MDRV_MACHINE_START( p8000_16 )
	MDRV_MACHINE_RESET(p8k_16)

	/* peripheral hardware */
	MDRV_Z80CTC_ADD("z80ctc_0", XTAL_4MHz, p8k_16_ctc_0_intf)
	MDRV_Z80CTC_ADD("z80ctc_1", XTAL_4MHz, p8k_16_ctc_1_intf)
	MDRV_Z80SIO_ADD("z80sio_0", 9600, p8k_16_sio_0_intf)
	MDRV_Z80SIO_ADD("z80sio_1", 9600, p8k_16_sio_1_intf)
	MDRV_Z80PIO_ADD("z80pio_0", p8k_16_pio_0_intf )
	MDRV_Z80PIO_ADD("z80pio_1", p8k_16_pio_1_intf )
	MDRV_Z80PIO_ADD("z80pio_2", p8k_16_pio_2_intf )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD("beep", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB15)
	MDRV_SCREEN_REFRESH_RATE(15)
	MDRV_SCREEN_SIZE(640,480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(p8k)
	MDRV_VIDEO_UPDATE(p8k)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( p8000 )
	ROM_REGION( 0x2000, "maincpu", 0 )
	ROM_LOAD("mon8_1_3.1",	0x0000, 0x1000, CRC(ad1bb118) SHA1(2332963acd74d5d1a009d9bce8a2b108de01d2a5))
	ROM_LOAD("mon8_2_3.1",	0x1000, 0x1000, CRC(daced7c2) SHA1(f1f778e72568961b448020fc543ed6e81bbe81b1))

	ROM_REGION( 0x1000, "proms", 0 )
	ROM_LOAD("p8t_zs",    0x0000, 0x0800, CRC(f9321251) SHA1(a6a796b58d50ec4a416f2accc34bd76bc83f18ea))
      ROM_LOAD("p8tdzs.2",  0x0800, 0x0800, CRC(32736503) SHA1(6a1d7c55dddc64a7d601dfdbf917ce1afaefbb0a))
ROM_END

ROM_START( p8000_16 )
      ROM_REGION16_BE( 0x4000, "maincpu", 0 )
      ROM_LOAD16_BYTE("mon16_1h_3.1_udos",   0x0000, 0x1000, CRC(0c3c28da) SHA1(0cd35444c615b404ebb9cf80da788593e573ddb5))
      ROM_LOAD16_BYTE("mon16_1l_3.1_udos",   0x0001, 0x1000, CRC(e8857bdc) SHA1(f89c65cbc479101130c71806fd3ddc28e6383f12))
      ROM_LOAD16_BYTE("mon16_2h_3.1_udos",   0x2000, 0x1000, CRC(cddf58d5) SHA1(588bad8df75b99580459c7a8e898a3396907e3a4))
      ROM_LOAD16_BYTE("mon16_2l_3.1_udos",   0x2001, 0x1000, CRC(395ee7aa) SHA1(d72fadb1608cd0915cd5ce6440897303ac5a12a6))

	ROM_REGION( 0x1000, "proms", 0 )
	ROM_LOAD("p8t_zs",    0x0000, 0x0800, CRC(f9321251) SHA1(a6a796b58d50ec4a416f2accc34bd76bc83f18ea))
      ROM_LOAD("p8tdzs.2",  0x0800, 0x0800, CRC(32736503) SHA1(6a1d7c55dddc64a7d601dfdbf917ce1afaefbb0a))
ROM_END

/* Driver */

/*    YEAR  NAME        PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY                   FULLNAME       FLAGS */
COMP( 1989, p8000,      0,      0,       p8k,       p8k,     0,      0,   "EAW electronic Treptow", "P8000 (8bit Board)",  GAME_NOT_WORKING)
COMP( 1989, p8000_16,   p8000,  0,       p8k_16,    p8k,     0,      0,   "EAW electronic Treptow", "P8000 (16bit Board)",  GAME_NOT_WORKING)
