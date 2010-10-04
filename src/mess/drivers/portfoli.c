/*

	Atari Portfolio

	http://portfolio.wz.cz/
	http://www.pofowiki.de/doku.php
	http://www.best-electronics-ca.com/portfoli.htm

*/

/*

	Undumped cartridges:

	Utility-Card				HPC-701
	Finance-Card				HPC-702
	Science-Card				HPC-703	
	File Manager / Tutorial		HPC-704
	PowerBasic					HPC-705
	Instant Spell				HPC-709
	Hyperlist					HPC-713
	Bridge Baron				HPC-724
	Wine Companion				HPC-725	
	Diet / Cholesterol Counter	HPC-726	
	Astrologer					HPC-728	
	Stock Tracker				HPC-729	
	Chess						HPC-750

*/

/*

	TODO:

	- ERROR: Low Battery
	- main RAM is non-volatile
	- screen contrast
	- interrupts
	- system tick
	- speaker
	- serial interface
	- parallel interface
	- credit card memory

*/

#include "emu.h"
#include "includes/portfoli.h"
#include "cpu/i86/i86.h"
#include "devices/cartslot.h"
#include "devices/messram.h"
#include "devices/printer.h"
#include "machine/ctronics.h"
#include "machine/i8255a.h"
#include "machine/ins8250.h"
#include "sound/speaker.h"
#include "video/hd61830.h"

static WRITE8_HANDLER( ncc1_w )
{
	address_space *program = cputag_get_address_space(space->machine, M80C88A_TAG, ADDRESS_SPACE_PROGRAM);

	if (BIT(data, 0))
	{
		// system ROM
		UINT8 *rom = memory_region(space->machine, M80C88A_TAG);
		memory_install_rom(program, 0xc0000, 0xdffff, 0, 0, rom);
	}
	else
	{
		// credit card memory
		memory_unmap_readwrite(program, 0xc0000, 0xdffff, 0, 0);
	}
}

static READ8_HANDLER( pid_r )
{
	/*

		PID		peripheral

		00		communication card
		01		serial port
		02		parallel port
		03		printer peripheral
		04		modem
		05-3f	reserved
		40-7f	user peripherals
		80		file-transfer interface
		81-ff	reserved

	*/

	return 0xff;
}

static WRITE8_HANDLER( sivr_w )
{
	portfolio_state *state = space->machine->driver_data<portfolio_state>();

	state->sivr = data;
}

static ADDRESS_MAP_START( portfolio_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x3efff) AM_RAM
	AM_RANGE(0x3f000, 0x9efff) AM_RAM // expansion
	AM_RANGE(0xb0000, 0xb0fff) AM_MIRROR(0xf000) AM_RAM // video RAM
	AM_RANGE(0xc0000, 0xdffff) AM_ROM AM_REGION(M80C88A_TAG, 0) // or credit card memory
	AM_RANGE(0xe0000, 0xfffff) AM_ROM AM_REGION(M80C88A_TAG, 0x20000)
ADDRESS_MAP_END

static ADDRESS_MAP_START( portfolio_io, ADDRESS_SPACE_IO, 8 )
//	AM_RANGE(0x8000, 0x800f) Keyboard
	AM_RANGE(0x8010, 0x8010) AM_DEVREADWRITE(HD61830_TAG, hd61830_data_r, hd61830_data_w)
	AM_RANGE(0x8011, 0x8011) AM_DEVREADWRITE(HD61830_TAG, hd61830_status_r, hd61830_control_w)
//	AM_RANGE(0x8020, 0x8020) Speaker
	AM_RANGE(0x8060, 0x8060) AM_RAM AM_BASE_MEMBER(portfolio_state, contrast)
//	AM_RANGE(0x8070, 0x8077) AM_DEVREADWRITE(M82C50A_TAG, ins8250_r, ins8250_w) Serial Interface
//	AM_RANGE(0x8078, 0x807b) AM_DEVREADWRITE(M82C55A_TAG, i8255a_r, i8255a_w) Parallel Interface
	AM_RANGE(0x807c, 0x807c) AM_WRITE(ncc1_w)
	AM_RANGE(0x807f, 0x807f) AM_READWRITE(pid_r, sivr_w)
ADDRESS_MAP_END

static INPUT_PORTS_START( portfolio )
INPUT_PORTS_END

static PALETTE_INIT( portfolio )
{
	palette_set_color(machine, 0, MAKE_RGB(138, 146, 148));
	palette_set_color(machine, 1, MAKE_RGB(92, 83, 88));
}

static VIDEO_UPDATE( portfolio )
{
	portfolio_state *state = screen->machine->driver_data<portfolio_state>();

	hd61830_update(state->hd61830, bitmap, cliprect);

	return 0;
}

static WRITE8_DEVICE_HANDLER( ppi_pb_w )
{
	/*

        bit     signal

        PB0		strobe
        PB1		autofeed
        PB2		init/reset
        PB3		select in
        PB4		
        PB5		
        PB6		
        PB7		

    */

	portfolio_state *state = device->machine->driver_data<portfolio_state>();

	/* strobe */
	centronics_strobe_w(state->centronics, BIT(data, 0));
	
	/* autofeed */
	centronics_autofeed_w(state->centronics, BIT(data, 1));
	
	/* init/reset */
	centronics_init_w(state->centronics, BIT(data, 2));
	
	/* select in */
	//centronics_select_in_w(state->centronics, BIT(data, 3));
}

static READ8_DEVICE_HANDLER( ppi_pc_r )
{
	/*

        bit     signal

        PC0		paper
        PC1		select
        PC2		
        PC3		error
        PC4		busy
        PC5		ack
        PC6		
        PC7		

    */

	portfolio_state *state = device->machine->driver_data<portfolio_state>();

	UINT8 data = 0;

	/* paper end */
	data |= centronics_pe_r(state->centronics);

	/* select */
	data |= centronics_vcc_r(state->centronics) << 1;

	/* error */
	data |= centronics_fault_r(state->centronics) << 3;

	/* busy */
	data |= centronics_busy_r(state->centronics) << 4;

	/* acknowledge */
	data |= centronics_ack_r(state->centronics) << 5;

	return data;
}

static I8255A_INTERFACE( ppi_intf )
{
	DEVCB_NULL,													// Port A read
	DEVCB_NULL,													// Port B read
	DEVCB_HANDLER(ppi_pc_r),									// Port C read
	DEVCB_DEVICE_HANDLER(CENTRONICS_TAG, centronics_data_w),	// Port A write
	DEVCB_HANDLER(ppi_pb_w),									// Port B write
	DEVCB_NULL													// Port C write
};

static INS8250_INTERRUPT( i8250_intrpt_w )
{
	// connected to EINT
}

const ins8250_interface i8250_intf =
{
	XTAL_1_8432MHz,
	i8250_intrpt_w,
	NULL,
	NULL,
	NULL
};

static DEVICE_IMAGE_LOAD( portfolio_cart )
{
	return IMAGE_INIT_FAIL;
}

static MACHINE_START( portfolio )
{
	portfolio_state *state = machine->driver_data<portfolio_state>();
	address_space *program = cputag_get_address_space(machine, M80C88A_TAG, ADDRESS_SPACE_PROGRAM);

	/* find devices */
	state->hd61830 = machine->device(HD61830_TAG);
	state->centronics = machine->device(CENTRONICS_TAG);
	state->speaker = machine->device(SPEAKER_TAG);

	/* memory expansions */
	switch (messram_get_size(machine->device("messram")))
	{
	case 128 * 1024:
		memory_unmap_readwrite(program, 0x3f000, 0x9efff, 0, 0);
		break;

	case 384 * 1024:
		memory_unmap_readwrite(program, 0x5f000, 0x9efff, 0, 0);
		break;
	}
}

static MACHINE_CONFIG_START( portfolio, portfolio_state )
    /* basic machine hardware */
    MDRV_CPU_ADD(M80C88A_TAG, I8088, XTAL_4_9152MHz)
    MDRV_CPU_PROGRAM_MAP(portfolio_mem)
    MDRV_CPU_IO_MAP(portfolio_io)
	
	MDRV_MACHINE_START(portfolio)

    /* video hardware */
	MDRV_SCREEN_ADD(SCREEN_TAG, LCD)
	MDRV_SCREEN_REFRESH_RATE(72)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(240, 64)
	MDRV_SCREEN_VISIBLE_AREA(0, 240-1, 0, 64-1)

	MDRV_DEFAULT_LAYOUT(layout_lcd)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(portfolio)

	MDRV_VIDEO_UPDATE(portfolio)

	MDRV_HD61830_ADD(HD61830_TAG, XTAL_4_9152MHz/2/2, SCREEN_TAG)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SPEAKER_TAG, SPEAKER_SOUND, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_I8255A_ADD(M82C55A_TAG, ppi_intf)
	MDRV_CENTRONICS_ADD(CENTRONICS_TAG, standard_centronics)
	MDRV_INS8250_ADD(M82C50A_TAG, i8250_intf) // should be MDRV_INS8250A_ADD

	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_INTERFACE("portfolio_cart")
	MDRV_CARTSLOT_LOAD(portfolio_cart)

	/* software lists */
//	MDRV_SOFTWARE_LIST_ADD("cart_list", "pofo")

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("128K")
	MDRV_RAM_EXTRA_OPTIONS("384K,640K")
MACHINE_CONFIG_END

ROM_START( pofo )
    ROM_REGION( 0x40000, M80C88A_TAG, 0 )
	ROM_LOAD( "rom b.u4", 0x00000, 0x20000, BAD_DUMP CRC(c9852766) SHA1(c74430281bc717bd36fd9b5baec1cc0f4489fe82) ) // dumped with debug.com
	ROM_LOAD( "rom a.u3", 0x20000, 0x20000, BAD_DUMP CRC(b8fb730d) SHA1(1b9d82b824cab830256d34912a643a7d048cd401) ) // dumped with debug.com
ROM_END

/*    YEAR  NAME    PARENT  COMPAT  MACHINE     INPUT       INIT    COMPANY     FULLNAME        FLAGS */
COMP( 1989, pofo,	0,		0,		portfolio,	portfolio,	0,		"Atari",	"Portfolio",	GAME_NOT_WORKING | GAME_NO_SOUND )
