/***************************************************************************

        Big Board 2

        12/05/2009 Skeleton driver.

        This is very much under construction. All that works is if you
        press a key it will display the signon message.

        Despite the name, this is not like the xerox or bigboard at all.

        It is compatible only if the software uses the same published
        calls to the bios. Everything else is different.

80 = sio ce
84 = ctca ce
88 = ctcb ce
8c = dma ce
c0 = prog
c4 = status 7,6,5,4 = sw1-4; 3 = kbdstb; 2 = motor; 1 = rxdb; 0 = rxda
c8 = sys1
cc = sys2
d0 = kbd
d4 = 1793 ce
d8 = port7
dc = 6845 ce

****************************************************************************/


#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "formats/basicdsk.h"
#include "imagedev/flopdrv.h"
#include "machine/ram.h"
#include "machine/z80ctc.h"
#include "machine/z80sio.h"
#include "machine/wd17xx.h"
#include "video/mc6845.h"
#include "machine/terminal.h"

#define SCREEN_TAG		"screen"

#define Z80_TAG			"u46"
#define Z80SIO_TAG		"u16"
#define Z80CTC_TAG		"u99"
#define WD1771_TAG		"u109"

class bigbord2_state : public driver_device
{
public:
	bigbord2_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, Z80_TAG),
		  m_6845(*this, "crtc"),
		  m_ctc_84(*this, "ctc_84"),
		  m_ctc_88(*this, "ctc_88"),
		  m_fdc(*this, WD1771_TAG),
		 // m_ram(*this, RAM_TAG),
		  m_floppy0(*this, FLOPPY_0),
		  m_floppy1(*this, FLOPPY_1)
	{ }

	virtual void machine_start();
	virtual void machine_reset();

	virtual void video_start();
	//virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_6845;
	required_device<device_t> m_ctc_84;
	required_device<device_t> m_ctc_88;
	required_device<device_t> m_fdc;
	//required_device<device_t> m_ram;
	required_device<device_t> m_floppy0;
	required_device<device_t> m_floppy1;

	DECLARE_WRITE8_MEMBER( scroll_w );
	//DECLARE_WRITE8_MEMBER( x120_system_w );
	DECLARE_READ8_MEMBER( bigbord2_c4_r );
	DECLARE_READ8_MEMBER( bigbord2_d0_r );
	DECLARE_READ8_MEMBER( kbpio_pa_r );
	DECLARE_WRITE8_MEMBER( kbpio_pa_w );
	DECLARE_WRITE8_MEMBER( bigbord2_kbd_put );
	DECLARE_READ8_MEMBER( kbpio_pb_r );
	DECLARE_WRITE_LINE_MEMBER( intrq_w );
	DECLARE_WRITE_LINE_MEMBER( drq_w );

	void scan_keyboard();
	//void bankswitch(int bank);
	void set_floppy_parameters(size_t length);
	void common_kbpio_pa_w(UINT8 data);

	/* keyboard state */
	UINT8 m_term_data;
	UINT8 m_term_status;

	/* video state */
	UINT8 *m_videoram;					/* video RAM */
	UINT8 *m_char_rom;					/* character ROM */
	UINT8 m_scroll;						/* vertical scroll */
	UINT8 m_framecnt;
	int m_ncset2;						/* national character set */
	int m_vatt;							/* X120 video attribute */
	int m_lowlite;						/* low light attribute */
	int m_chrom;						/* character ROM index */

	/* floppy state */
	int m_fdc_irq;						/* interrupt request */
	int m_fdc_drq;						/* data request */
	int m_8n5;							/* 5.25" / 8" drive select */
	int m_dsdd;							/* double sided disk detect */
};

/* Keyboard (external keyboard - so we use the 'terminal') */

READ8_MEMBER( bigbord2_state::bigbord2_c4_r )
{
	UINT8 ret = m_term_status | 0x17;
	m_term_status = 0;
	return ret;
}

READ8_MEMBER( bigbord2_state::bigbord2_d0_r )
{
	return m_term_data;
}

WRITE8_MEMBER( bigbord2_state::bigbord2_kbd_put )
{
	m_term_data = data;
	m_term_status = 8;
}

static GENERIC_TERMINAL_INTERFACE( bigbord2_terminal_intf )
{
	DEVCB_DRIVER_MEMBER(bigbord2_state, bigbord2_kbd_put)
};

static READ8_DEVICE_HANDLER( bigbord2_sio_r )
{
	if (!offset)
		return z80sio_d_r(device, 0);
	else
	if (offset == 2)
	return z80sio_d_r(device, 1);
	else
	if (offset == 1)
		return z80sio_c_r(device, 0);
	else
	return z80sio_c_r(device, 1);
}

static WRITE8_DEVICE_HANDLER( bigbord2_sio_w )
{
	if (!offset)
		z80sio_d_w(device, 0, data);
	else
	if (offset == 2)
	z80sio_d_w(device, 1, data);
	else
	if (offset == 1)
		z80sio_c_w(device, 0, data);
	else
		z80sio_c_w(device, 1, data);
}


/* Read/Write Handlers */

#if 0
void bigbord2_state::bankswitch(int bank)
{
	address_space *program = m_maincpu->memory().space(AS_PROGRAM);
	UINT8 *ram = ram_get_ptr(m_ram);

	if (bank)
	{
		/* ROM */
		program->install_rom(0x0000, 0x0fff, machine.region("monitor")->base());
		program->unmap_readwrite(0x1000, 0x1fff);
		program->install_ram(0x3000, 0x3fff, m_videoram);
	}
	else
	{
		/* RAM */
		program->install_ram(0x0000, 0x3fff, ram);
	}
}
#endif

WRITE8_MEMBER( bigbord2_state::scroll_w )
{
	m_scroll = (offset >> 8) & 0x1f;
}

#ifdef UNUSED_CODE
WRITE8_MEMBER( bigbord2_state::x120_system_w )
{
	/*

        bit     signal      description

        0       DSEL0       drive select bit 0 (01=A, 10=B, 00=C, 11=D)
        1       DSEL1       drive select bit 1
        2       SIDE        side select
        3       VATT        video attribute (0=inverse, 1=blinking)
        4       BELL        bell trigger
        5       DENSITY     density (0=double, 1=single)
        6       _MOTOR      disk motor (0=on, 1=off)
        7       BANK        memory bank switch (0=RAM, 1=ROM/video)

    */
}
#endif

#if 0
WRITE8_MEMBER( bigbii_state::bell_w )
{
}

WRITE8_MEMBER( bigbii_state::slden_w )
{
	wd17xx_dden_w(m_fdc, offset ? CLEAR_LINE : ASSERT_LINE);
}

WRITE8_MEMBER( bigbii_state::chrom_w )
{
	m_chrom = offset;
}

WRITE8_MEMBER( bigbii_state::lowlite_w )
{
	m_lowlite = data;
}

WRITE8_MEMBER( bigbii_state::sync_w )
{
	if (offset)
	{
		/* set external clocks for synchronous sio A */
	}
	else
	{
		/* set internal clocks for asynchronous sio A */
	}
}
#endif

/* Memory Maps */

static ADDRESS_MAP_START( bigbord2_mem, AS_PROGRAM, 8, bigbord2_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x6000, 0x67ff) AM_RAM AM_BASE(m_videoram)
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x1000, 0x5fff) AM_RAM
	AM_RANGE(0x6800, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( bigbord2_io, AS_IO, 8, bigbord2_state )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x80, 0x83) AM_DEVREADWRITE_LEGACY(Z80SIO_TAG, bigbord2_sio_r, bigbord2_sio_w)
	//AM_RANGE(0x84, 0x87) AM_DEVREADWRITE_LEGACY(Z80CTC_TAG, z80ctc_r, z80ctc_w)
	//AM_RANGE(0x88, 0x8b) AM_DEVREADWRITE_LEGACY(Z80CTC_TAG, z80ctc_r, z80ctc_w)
	//AM_RANGE(0x8C, 0x8F) AM_DEVREADWRITE_LEGACY(Z80DMA_TAG, z80ctc_r, z80ctc_w)
	AM_RANGE(0xC4, 0xC7) AM_READ(bigbord2_c4_r)
	AM_RANGE(0xD0, 0xD3) AM_READ(bigbord2_d0_r)
	AM_RANGE(0xD4, 0xD7) AM_DEVREADWRITE_LEGACY(WD1771_TAG, wd17xx_r, wd17xx_w)
	AM_RANGE(0xDC, 0xDC) AM_MIRROR(2) AM_DEVWRITE_LEGACY("crtc", mc6845_address_w)
	AM_RANGE(0xDD, 0xDD) AM_MIRROR(2) AM_DEVREADWRITE_LEGACY("crtc",mc6845_register_r,mc6845_register_w)
ADDRESS_MAP_END


/* Input Ports */

static INPUT_PORTS_START( bigbord2 )
INPUT_PORTS_END

/* Z80 PIO */

READ8_MEMBER( bigbord2_state::kbpio_pa_r )
{
	/*

        bit     signal          description

        0
        1
        2
        3       PBRDY           keyboard data available
        4       8/N5            8"/5.25" disk select (0=5.25", 1=8")
        5       400/460         double sided disk detect (only on Etch 2 PCB) (0=SS, 1=DS)
        6
        7

    */

	return (m_dsdd << 5) | (m_8n5 << 4);// | (z80pio_brdy_r(m_kbpio) << 3);
};

void bigbord2_state::common_kbpio_pa_w(UINT8 data)
{
	/*

        bit     signal          description

        0       _DVSEL1         drive select 1
        1       _DVSEL2         drive select 2
        2       _DVSEL3         side select
        3
        4
        5
        6       NCSET2          display character set (inverted and connected to chargen A10)
        7       BANK            bank switching (0=RAM, 1=ROM/videoram)

    */

	/* drive select */
	int dvsel1 = BIT(data, 0);
	int dvsel2 = BIT(data, 1);

	if (dvsel1) wd17xx_set_drive(m_fdc, 0);
	if (dvsel2) wd17xx_set_drive(m_fdc, 1);

	floppy_mon_w(m_floppy0, !dvsel1);
	floppy_mon_w(m_floppy1, !dvsel2);

	floppy_drive_set_ready_state(m_floppy0, dvsel1, 1);
	floppy_drive_set_ready_state(m_floppy1, dvsel2, 1);

	/* side select */
	wd17xx_set_side(m_fdc, BIT(data, 2));

	/* display character set */
	m_ncset2 = !BIT(data, 6);
}

WRITE8_MEMBER( bigbord2_state::kbpio_pa_w )
{
	common_kbpio_pa_w(data);

	/* bank switching */
	//bankswitch(BIT(data, 7));
}


/* Z80 SIO */

static void bigbord2_interrupt(device_t *device, int state)
{
	cputag_set_input_line(device->machine(), Z80_TAG, 0, state);
}

const z80sio_interface sio_intf =
{
	bigbord2_interrupt,	/* interrupt handler */
	0,			/* DTR changed handler */
	0,			/* RTS changed handler */
	0,			/* BREAK changed handler */
	0,			/* transmit handler - which channel is this for? */
	0			/* receive handler - which channel is this for? */
};


/* Z80 CTC */

static TIMER_DEVICE_CALLBACK( ctc_tick )
{
	//bigbord2_state *state = timer.machine().driver_data<bigbord2_state>();

	//z80ctc_trg0_w(state->m_ctc, 1);
	//z80ctc_trg0_w(state->m_ctc, 0);
}

static WRITE_LINE_DEVICE_HANDLER( ctc_z0_w )
{
//  z80ctc_trg1_w(device, state);
}

static WRITE_LINE_DEVICE_HANDLER( ctc_z2_w )
{
//  z80ctc_trg3_w(device, state);
}

static Z80CTC_INTERFACE( ctc_intf )
{
	0,              			/* timer disables */
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	/* interrupt handler */
	DEVCB_LINE(ctc_z0_w),		/* ZC/TO0 callback */
	DEVCB_LINE(z80ctc_trg2_w),	/* ZC/TO1 callback */
	DEVCB_LINE(ctc_z2_w)		/* ZC/TO2 callback */
};

/* Z80 Daisy Chain */

static const z80_daisy_config bigbord2_daisy_chain[] =
{
	{ Z80SIO_TAG },
	{ "ctc_84" },
	{ "ctc_88" },
	{ NULL }
};

/* WD1771 Interface */

WRITE_LINE_MEMBER( bigbord2_state::intrq_w )
{
	int halt = cpu_get_reg(m_maincpu, Z80_HALT);

	m_fdc_irq = state;

	if (halt && state)
		device_set_input_line(m_maincpu, INPUT_LINE_NMI, ASSERT_LINE);
	else
		device_set_input_line(m_maincpu, INPUT_LINE_NMI, CLEAR_LINE);
}

WRITE_LINE_MEMBER( bigbord2_state::drq_w )
{
	int halt = cpu_get_reg(m_maincpu, Z80_HALT);

	m_fdc_drq = state;

	if (halt && state)
		device_set_input_line(m_maincpu, INPUT_LINE_NMI, ASSERT_LINE);
	else
		device_set_input_line(m_maincpu, INPUT_LINE_NMI, CLEAR_LINE);
}

static const wd17xx_interface fdc_intf =
{
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(bigbord2_state, intrq_w),
	DEVCB_DRIVER_LINE_MEMBER(bigbord2_state, drq_w),
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};


/* Video */

void bigbord2_state::video_start()
{
	/* find memory regions */
	m_char_rom = m_machine.region("chargen")->base();
}


void bigbord2_state::set_floppy_parameters(size_t length)
{
	switch (length)
	{
	case 77*1*26*128: // 250K 8" SSSD
		m_8n5 = 1;
		m_dsdd = 0;
		break;

	case 77*1*26*256: // 500K 8" SSDD
		m_8n5 = 1;
		m_dsdd = 0;
		break;

	case 40*1*18*128: // 90K 5.25" SSSD
		m_8n5 = 0;
		m_dsdd = 0;
		break;

	case 40*2*18*128: // 180K 5.25" DSSD
		m_8n5 = 0;
		m_dsdd = 1;
		break;
	}
}

static void bigbord2_load_proc(device_image_interface &image)
{
	bigbord2_state *state = image.device().machine().driver_data<bigbord2_state>();

	state->set_floppy_parameters(image.length());
}

/* Machine Initialization */

void bigbord2_state::machine_start()
{
	// set floppy load procs
	floppy_install_load_proc(m_floppy0, bigbord2_load_proc);
	floppy_install_load_proc(m_floppy1, bigbord2_load_proc);

	/* register for state saving */
	state_save_register_global(m_machine, m_term_data);
	state_save_register_global(m_machine, m_scroll);
	state_save_register_global(m_machine, m_ncset2);
	state_save_register_global(m_machine, m_vatt);
	state_save_register_global(m_machine, m_fdc_irq);
	state_save_register_global(m_machine, m_fdc_drq);
	state_save_register_global(m_machine, m_8n5);
	state_save_register_global(m_machine, m_dsdd);
}

void bigbord2_state::machine_reset()
{
	//bankswitch(1);
}

static FLOPPY_OPTIONS_START( bigbord2 )
	FLOPPY_OPTION( sssd8, "dsk", "8\" SSSD", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([77])
		SECTORS([26])
		SECTOR_LENGTH([128])
		FIRST_SECTOR_ID([1]))
	FLOPPY_OPTION( ssdd8, "dsk", "8\" SSDD", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([77])
		SECTORS([26])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
	FLOPPY_OPTION( sssd5, "dsk", "5.25\" SSSD", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([18])
		SECTOR_LENGTH([128])
		FIRST_SECTOR_ID([1]))
	FLOPPY_OPTION( ssdd5, "dsk", "5.25\" SSDD", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([40])
		SECTORS([18])
		SECTOR_LENGTH([128])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static const floppy_config bigbord2_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSDD,
	FLOPPY_OPTIONS_NAME(bigbord2),
	NULL
};

/* F4 Character Displayer */
static const gfx_layout bigbord2_charlayout =
{
	8, 16,					/* 8 x 8 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{  0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8, 8*8,  9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16					/* every char takes 8 bytes */
};

static GFXDECODE_START( bigbord2 )
	GFXDECODE_ENTRY( "chargen", 0x0000, bigbord2_charlayout, 0, 1 )
GFXDECODE_END

static SCREEN_UPDATE( bigbord2 )
{
	bigbord2_state *state = screen->machine().driver_data<bigbord2_state>();
	state->m_framecnt++;
	mc6845_update(state->m_6845, bitmap, cliprect);
	return 0;
}

MC6845_UPDATE_ROW( bigbord2_update_row )
{
	bigbord2_state *state = device->machine().driver_data<bigbord2_state>();
	UINT8 chr,gfx,inv;
	UINT16 mem,x;
	UINT16 *p = BITMAP_ADDR16(bitmap, y, 0);

	for (x = 0; x < x_count; x++)				// for each character
	{
		inv=0;
		if (x == cursor_x) inv^=0xff;
		mem = (ma + x) & 0x7ff;
		chr = state->m_videoram[mem];

		/* get pattern of pixels for that character scanline */
		gfx = state->m_char_rom[(chr<<4) | ra ] ^ inv;

		/* Display a scanline of a character */
		*p++ = BIT( gfx, 7 ) ? 1 : 0;
		*p++ = BIT( gfx, 6 ) ? 1 : 0;
		*p++ = BIT( gfx, 5 ) ? 1 : 0;
		*p++ = BIT( gfx, 4 ) ? 1 : 0;
		*p++ = BIT( gfx, 3 ) ? 1 : 0;
		*p++ = BIT( gfx, 2 ) ? 1 : 0;
		*p++ = BIT( gfx, 1 ) ? 1 : 0;
		*p++ = BIT( gfx, 0 ) ? 1 : 0;
	}
}

static const mc6845_interface bigbord2_crtc = {
	"screen",			/* name of screen */
	8,			/* number of dots per character */
	NULL,
	bigbord2_update_row,		/* handler to display a scanline */
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};


/* Machine Drivers */

#define MAIN_CLOCK XTAL_8MHz / 2

static MACHINE_CONFIG_START( bigbord2, bigbord2_state )
	/* basic machine hardware */
	MCFG_CPU_ADD(Z80_TAG, Z80, MAIN_CLOCK)
	MCFG_CPU_PROGRAM_MAP(bigbord2_mem)
	MCFG_CPU_IO_MAP(bigbord2_io)
	MCFG_CPU_CONFIG(bigbord2_daisy_chain)

	/* video hardware */
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_RAW_PARAMS(XTAL_10_69425MHz, 700, 0, 560, 260, 0, 240)
	MCFG_SCREEN_UPDATE(bigbord2)
	MCFG_GFXDECODE(bigbord2)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)

	/* keyboard */
	MCFG_TIMER_ADD_PERIODIC("ctc", ctc_tick, attotime::from_hz(MAIN_CLOCK))

	/* devices */
	MCFG_Z80SIO_ADD(Z80SIO_TAG, MAIN_CLOCK, sio_intf)
	MCFG_Z80CTC_ADD("ctc_84", MAIN_CLOCK, ctc_intf)
	MCFG_Z80CTC_ADD("ctc_88", MAIN_CLOCK, ctc_intf)
	MCFG_WD1771_ADD(WD1771_TAG, fdc_intf)
	MCFG_FLOPPY_2_DRIVES_ADD(bigbord2_floppy_config)
	MCFG_MC6845_ADD("crtc", MC6845, XTAL_16MHz / 8, bigbord2_crtc)
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, bigbord2_terminal_intf)
MACHINE_CONFIG_END


/* ROMs */


ROM_START( bigbord2 )
	ROM_REGION( 0x1000, Z80_TAG, 0 )
	ROM_LOAD( "bigbrdii.bin", 0x0000, 0x1000, CRC(c588189e) SHA1(4133903171ee8b9fcf12cc72de843af782b4a645) )

	// internal to 8002 chip (undumped) we will use one from 'vta2000' for now
	ROM_REGION( 0x2000, "chargen", ROMREGION_INVERT )
	ROM_LOAD( "bdp-15_14.rom", 0x0000, 0x2000, CRC(a1dc4f8e) SHA1(873fd211f44713b713d73163de2d8b5db83d2143) )
ROM_END
/* System Drivers */

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT       INIT    COMPANY                         FULLNAME        FLAGS */
COMP( 1983, bigbord2,   bigboard,          0,      bigbord2,   bigbord2,   0,      "Digital Research Computers",   "Big Board II", GAME_NOT_WORKING | GAME_NO_SOUND)
