/*
	Machine code for Myarc Geneve.
	Raphael Nabet, 2003.
*/

#include <math.h>
#include "driver.h"
#include "tms9901.h"
#include "mm58274c.h"
#include "video/v9938.h"
#include "audio/spchroms.h"
#include "ti99_4x.h"
#include "includes/geneve.h"
#include "99_peb.h"
#include "994x_ser.h"
#include "99_dsk.h"
#include "99_ide.h"
#include "99_usbsm.h"

#include "sound/tms5220.h"	/* for tms5220_set_variant() */
#include "sound/5220intf.h"
#include "sound/sn76496.h"


/* prototypes */
static void inta_callback(int state);
static void intb_callback(int state);

static void read_key_if_possible(void);
static void poll_keyboard(void);
static void poll_mouse(void);

static void tms9901_interrupt_callback(int intreq, int ic);
static int R9901_0(int offset);
static int R9901_1(int offset);
static int R9901_2(int offset);
static int R9901_3(int offset);

static void W9901_PE_bus_reset(int offset, int data);
static void W9901_VDP_reset(int offset, int data);
static void W9901_JoySel(int offset, int data);
static void W9901_KeyboardReset(int offset, int data);
static void W9901_ext_mem_wait_states(int offset, int data);
static void W9901_VDP_wait_states(int offset, int data);

/*
	pointers to memory areas
*/
/* pointer to boot ROM */
static UINT8 *ROM_ptr;
/* pointer to static RAM */
static UINT8 *SRAM_ptr;
/* pointer to dynamic RAM */
static UINT8 *DRAM_ptr;

/*
	Configuration
*/
/* TRUE if speech synthesizer present */
static char has_speech;
/* floppy disk controller type */
static fdc_kind_t fdc_kind;
/* TRUE if ide card present */
static char has_ide;
/* TRUE if rs232 card present */
static char has_rs232;
/* TRUE if genmod extension present */
/*static char has_genmod;*/
/* TRUE if usb-sm card present */
static char has_usb_sm;


/* tms9901 setup */
static const tms9901reset_param tms9901reset_param_ti99 =
{
	TMS9901_INT1 | TMS9901_INT2 | TMS9901_INT8 | TMS9901_INTB | TMS9901_INTC,	/* only input pins whose state is always known */

	{	/* read handlers */
		R9901_0,
		R9901_1,
		R9901_2,
		R9901_3
	},

	{	/* write handlers */
		W9901_PE_bus_reset,
		W9901_VDP_reset,
		W9901_JoySel,
		NULL,
		NULL,
		NULL,
		W9901_KeyboardReset,
		W9901_ext_mem_wait_states,
		NULL,
		W9901_VDP_wait_states,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	},

	/* interrupt handler */
	tms9901_interrupt_callback,

	/* clock rate = 3MHz */
	3000000.
};

/* keyboard interface */
static int JoySel;
enum
{
	KeyQueueSize = 256,
	MaxKeyMessageLen = 10,
	KeyAutoRepeatDelay = 30,
	KeyAutoRepeatRate = 6
};
static UINT8 KeyQueue[KeyQueueSize];
static int KeyQueueHead;
static int KeyQueueLen;
static int KeyInBuf;
static int KeyReset;
static UINT32 KeyStateSave[4];
static int KeyNumLockState;
static int KeyCtrlState;
static int KeyAltState;
static int KeyRealShiftState;
static int KeyFakeShiftState;
static int KeyFakeUnshiftState;
static int KeyAutoRepeatKey;
static int KeyAutoRepeatTimer;

static void machine_stop_geneve(running_machine *machine);

/*
	GROM support.

	The Geneve does not include any GROM, but it features a simple GROM emulator.

	One oddity is that, since the Geneve does not use a look-ahead buffer, GPL
	data is always off one byte, i.e. GPL bytes 0, 1, 2... are stored in bytes
	1, 2, 3... of pages >38->3f (byte 0 of page >38 is has GPL address >ffff).

	I think that I have once read that the geneve GROM emulator does not
	emulate wrap-around within a GROM, i.e. address >1fff is followed by >2000
	(instead of >0000 with a real GROM).
*/
static struct
{
	unsigned int addr;
	char raddr_LSB, waddr_LSB;
} GPL_port;

/*
	Cartridge support

	The Geneve does not have a cartridge port, but it has cartridge emulation.
*/
static int cartridge_page;


static int mode_flags;

static UINT8 page_lookup[8];

enum
{
	mf_PAL			= 0x001,	/* bit 5 */
	mf_keycaps		= 0x004,	/* bit 7 - exact meaning unknown */
	mf_keyclock		= 0x008,	/* bit 8 */
	mf_keyclear		= 0x010,	/* bit 9 */
	mf_genevemode	= 0x020,	/* bit 10 */
	mf_mapmode		= 0x040,	/* bit 11 */
	mf_cartpaging	= 0x080,	/* bit 12 */
	mf_cart6protect	= 0x100,	/* bit 13 */
	mf_cart7protect	= 0x200,	/* bit 14 */
	mf_waitstate	= 0x400		/* bit 15 - exact meaning unknown */
};

/* tms9995_ICount: used to implement memory waitstates (hack) */
/* NPW 23-Feb-2004 - externs no longer needed because we now use activecpu_adjust_icount() */



/*===========================================================================*/
/*
	General purpose code:
	initialization, cart loading, etc.
*/

DRIVER_INIT( geneve )
{
	/*has_genmod = FALSE;*/
}

DRIVER_INIT( genmod )
{
	/*has_genmod = TRUE;*/
}


MACHINE_START( geneve )
{
	mode_flags = /*0*/mf_mapmode;
	/* initialize page lookup */
	memset(page_lookup, 0, sizeof(page_lookup));

	/* set up RAM pointers */
	ROM_ptr = memory_region(REGION_CPU1) + offset_rom_geneve;
	SRAM_ptr = memory_region(REGION_CPU1) + offset_sram_geneve;
	DRAM_ptr = memory_region(REGION_CPU1) + offset_dram_geneve;

	/* Initialize GROMs */
	memset(& GPL_port, 0, sizeof(GPL_port));

	/* reset cartridge mapper */
	cartridge_page = 0;

	/* init tms9901 */
	tms9901_init(0, & tms9901reset_param_ti99);

	v9938_reset();

	mm58274c_init(0, 1);

	/* clear keyboard interface state (probably overkill, but can't harm) */
	JoySel = 0;
	KeyQueueHead = KeyQueueLen = 0;
	memset(KeyStateSave, 0, sizeof(KeyStateSave));
	KeyNumLockState = 0;
	KeyCtrlState = 0;
	KeyAltState = 0;
	KeyRealShiftState = 0;
	KeyFakeShiftState = 0;
	KeyFakeUnshiftState = 0;
	KeyAutoRepeatKey = 0;
	KeyInBuf = 0;
	KeyReset = 1;

	/* read config */
	has_speech = (readinputport(input_port_config) >> config_speech_bit) & config_speech_mask;
	fdc_kind = (readinputport(input_port_config) >> config_fdc_bit) & config_fdc_mask;
	has_ide = (readinputport(input_port_config) >> config_ide_bit) & config_ide_mask;
	has_rs232 = (readinputport(input_port_config) >> config_rs232_bit) & config_rs232_mask;
	has_usb_sm = (readinputport(input_port_config) >> config_usbsm_bit) & config_usbsm_mask;

	/* set up optional expansion hardware */
	ti99_peb_init(0, inta_callback, intb_callback);

	if (has_speech)
	{
		spchroms_interface speech_intf = { region_speech_rom };

		spchroms_config(& speech_intf);
	}

	switch (fdc_kind)
	{
	case fdc_kind_TI:
		ti99_fdc_init();
		break;
#if HAS_99CCFDC
	case fdc_kind_CC:
		ti99_ccfdc_init();
		break;
#endif
	case fdc_kind_BwG:
		ti99_bwg_init();
		break;
	case fdc_kind_hfdc:
		ti99_hfdc_init();
		break;
	case fdc_kind_none:
		break;
	}

	if (has_ide)
	{
		ti99_ide_init(TRUE);
		ti99_ide_load_memcard();
	}

	if (has_rs232)
		ti99_rs232_init();

	if (has_usb_sm)
		ti99_usbsm_init(TRUE);

	add_exit_callback(machine, machine_stop_geneve);
	return 0;
}

static void machine_stop_geneve(running_machine *machine)
{
	if (has_ide)
		ti99_ide_save_memcard();

	if (has_rs232)
		ti99_rs232_cleanup();

	tms9901_cleanup(0);
}


/*
	video initialization.
*/
VIDEO_START(geneve)
{
	return v9938_init(machine, MODEL_V9938, /*0x20000*/0x30000, tms9901_set_int2);	/* v38 with 128 kb of video RAM */
}

/*
	scanline interrupt
*/
void geneve_hblank_interrupt(void)
{
	static int line_count;
	v9938_interrupt();
	if (++line_count == 262)
	{
		line_count = 0;
		poll_keyboard();
		poll_mouse();
	}
}

/*
	inta is connected to both tms9901 IRQ1 line and to tms9995 INT4/EC line.
*/
static void inta_callback(int state)
{
	tms9901_set_single_int(0, 1, state);
	cpunum_set_input_line(0, 1, state ? ASSERT_LINE : CLEAR_LINE);
}

/*
	intb is connected to tms9901 IRQ12 line.
*/
static void intb_callback(int state)
{
	tms9901_set_single_int(0, 12, state);
}


/*===========================================================================*/
#if 0
#pragma mark -
#pragma mark MEMORY HANDLERS
#endif
/*
	Memory handlers.
*/

/*
	TMS5200 speech chip read
*/
static  READ8_HANDLER ( geneve_speech_r )
{
	activecpu_adjust_icount(-8);		/* this is just a minimum, it can be more */

	return tms5220_status_r(offset);
}

#if 0

static void speech_kludge_callback(int dummy)
{
	if (! tms5220_ready_r())
	{
		/* Weirdly enough, we are always seeing some problems even though
		everything is working fine. */
		/*double time_to_ready = tms5220_time_to_ready();
		logerror("ti99/4a speech says aaargh!\n");
		logerror("(time to ready: %f -> %d)\n", time_to_ready, (int) ceil(3000000*time_to_ready));*/
	}
}

#endif

/*
	TMS5200 speech chip write
*/
static WRITE8_HANDLER ( geneve_speech_w )
{
	activecpu_adjust_icount(-32*4);		/* this is just an approx. minimum, it can be much more */

#if 1
	/* the stupid design of the tms5220 core means that ready is cleared when
	there are 15 bytes in FIFO.  It should be 16.  Of course, if it were the
	case, we would need to store the value on the bus, which would be more
	complex. */
	if (! tms5220_ready_r())
	{
		double time_to_ready = tms5220_time_to_ready();
		int cycles_to_ready = ceil(TIME_TO_CYCLES(0, time_to_ready));

		logerror("time to ready: %f -> %d\n", time_to_ready, (int) cycles_to_ready);

		activecpu_adjust_icount(-cycles_to_ready);
		timer_set(TIME_NOW, 0, /*speech_kludge_callback*/NULL);
	}
#endif

	tms5220_data_w(offset, data);
}

 READ8_HANDLER ( geneve_r )
{
	int page;

	if (mode_flags & mf_genevemode)
	{
		/* geneve mode */
		if ((offset >= 0xf100) && (offset < 0xf140))
		{
			/* memory-mapped registers */
			/* VDP ports 2 & 3 and sound port are write-only */
			switch (offset)
			{
			case 0xf100:
			case 0xf108:		/* mirror? */
				return v9938_vram_r(0);

			case 0xf102:
			case 0xf10a:		/* mirror? */
				return v9938_status_r(0);

			case 0xf110:
			case 0xf111:
			case 0xf112:
			case 0xf113:
			case 0xf114:
			case 0xf115:
			case 0xf116:
			case 0xf117:
				return page_lookup[offset-0xf110];

			case 0xf118:
				return KeyInBuf ? KeyQueue[KeyQueueHead] : 0;

			case 0xf130:
			case 0xf131:
			case 0xf132:
			case 0xf133:
			case 0xf134:
			case 0xf135:
			case 0xf136:
			case 0xf137:
			case 0xf138:
			case 0xf139:
			case 0xf13a:
			case 0xf13b:
			case 0xf13c:
			case 0xf13d:
			case 0xf13e:
			case 0xf13f:
				return mm58274c_r(0, offset-0xf130);

			default:
				logerror("unmapped read offs=%d\n", (int) offset);
				return 0;
			}
		}
	}
	else
	{
		/* ti99 mode */
		if ((offset >= 0x8000) && (offset < 0x8020))
		{
			/* 0x8000-0x801f */
			/* geneve memory-mapped registers */
			switch (offset)
			{
			case 0x8000:
			case 0x8001:
			case 0x8002:
			case 0x8003:
			case 0x8004:
			case 0x8005:
			case 0x8006:
			case 0x8007:
				return page_lookup[offset-0x8000];

			case 0x8008:
				return KeyInBuf ? KeyQueue[KeyQueueHead] : 0;

			case 0x8010:
			case 0x8011:
			case 0x8012:
			case 0x8013:
			case 0x8014:
			case 0x8015:
			case 0x8016:
			case 0x8017:
			case 0x8018:
			case 0x8019:
			case 0x801a:
			case 0x801b:
			case 0x801c:
			case 0x801d:
			case 0x801e:
			case 0x801f:
				return mm58274c_r(0, offset-0xf130);

			default:
				logerror("unmapped read offs=%d\n", (int) offset);
				return 0;
			}
		}
		else if ((offset >= 0x8400) && (offset < 0xa000))
		{
			/* 0x8400-0x9fff */
			/* ti99 memory-mapped registers */
			switch ((offset - 0x8000) >> 10)
			{
			case 2:
				/* VDP read */
				if (! (offset & 1))
				{
					if (offset & 2)
					{	/* read VDP status */
						return v9938_status_r(0);
					}
					else
					{	/* read VDP RAM */
						return v9938_vram_r(0);
					}
				}
				return 0;

			case 4:
				/* speech read */
				if ((! (offset & 1)) && has_speech)
				{
					return geneve_speech_r(0);
				}
				return 0;

			case 6:
				/* GPL read */
				if (! (offset & 1))
				{
					if (offset & 2)
					{	/* read GPL address */
						int reply;

						GPL_port.waddr_LSB = FALSE;	/* right??? */

						if (GPL_port.raddr_LSB)
						{
							reply = GPL_port.addr & 0xff;
							GPL_port.raddr_LSB = FALSE;
						}
						else
						{
							reply = GPL_port.addr >> 8;
							GPL_port.raddr_LSB = TRUE;
						}
						return reply;
					}
					else
					{	/* read GPL data */
						int reply;

						reply = DRAM_ptr[0x38*0x2000 + GPL_port.addr];
						GPL_port.addr = (GPL_port.addr + 1) & 0xffff;
						GPL_port.raddr_LSB = GPL_port.waddr_LSB = FALSE;

						return reply;
					}
				}
				return 0;

			default:
				logerror("unmapped read offs=%d\n", (int) offset);
				return 0;
			}
		}
	}

	page = offset >> 13;

	if (! (mode_flags & mf_mapmode))
	{
		if ((mode_flags & mf_genevemode) || (page != 3))
			page = page_lookup[page];
		else
			/* Cartridge page under ti99 emulation */
			page = ((mode_flags & mf_cartpaging) || ! cartridge_page) ? 0x36 : 0x37;
	}
	else
		page = page + 0xf0;

	offset &= 0x1fff;

	switch (page)
	{
	case 0xf0:
	case 0xf1:
		/* Boot ROM */
		return ROM_ptr[(page-0xf0)*0x2000 + offset];

	case 0xe8:
	case 0xe9:
	case 0xea:
	case 0xeb:
	case 0xec:
	case 0xed:
	case 0xee:
	case 0xef:
		/* SRAM */
		return SRAM_ptr[(page-0xe8)*0x2000 + offset];

#if 0
	case 0x7a:	/* mirror of 0xba??? */
		return 0;
#endif
	case 0xba:
		/* DSR space */
		return geneve_peb_r(offset);

	case 0xbc:
		/* speech space */
		if (has_speech)
		{
			if ((offset >= 0x1000) && (offset < 0x1400) && (! (offset & 1)))
				return geneve_speech_r(0);
			else
				return 0;
		}

	default:
		/* DRAM */
		return DRAM_ptr[page*0x2000 + offset];
	}

	logerror("unmapped read page=%d offs=%d\n", (int) page, (int) offset);
	return 0;
}

WRITE8_HANDLER ( geneve_w )
{
	int page;

	if (mode_flags & mf_genevemode)
	{
		/* geneve mode */
		if ((offset >= 0xf100) && (offset < 0xf140))
		{
			/* memory-mapped registers */
			/* VDP ports 2 & 3 and sound port are write-only */
			switch (offset)
			{
			case 0xf100:
			case 0xf108:		/* mirror? */
				v9938_vram_w(0, data);
				return;

			case 0xf102:
			case 0xf10a:		/* mirror? */
				v9938_command_w(0, data);
				return;

			case 0xf104:
			case 0xf10c:		/* mirror? */
				v9938_palette_w(0, data);
				return;

			case 0xf106:
			case 0xf10e:		/* mirror? */
				v9938_register_w(0, data);
				return;

			case 0xf110:
			case 0xf111:
			case 0xf112:
			case 0xf113:
			case 0xf114:
			case 0xf115:
			case 0xf116:
			case 0xf117:
				page_lookup[offset-0xf110] = data;
				return;

			/*case 0xf118:	// read-only register???
				key_buf = data;
				return*/

			case 0xf120:
				SN76496_0_w(0, data);
				break;

			case 0xf130:
			case 0xf131:
			case 0xf132:
			case 0xf133:
			case 0xf134:
			case 0xf135:
			case 0xf136:
			case 0xf137:
			case 0xf138:
			case 0xf139:
			case 0xf13a:
			case 0xf13b:
			case 0xf13c:
			case 0xf13d:
			case 0xf13e:
			case 0xf13f:
				mm58274c_w(0, offset-0xf130, data);
				return;

			default:
				logerror("unmapped write offs=%d data=%d\n", (int) offset, (int) data);
				return;
			}
		}
	}
	else
	{
		/* ti99 mode */
		if ((offset >= 0x8000) && (offset < 0x8020))
		{
			/* 0x8000-0x801f */
			/* geneve memory-mapped registers */
			switch (offset)
			{
			case 0x8000:
			case 0x8001:
			case 0x8002:
			case 0x8003:
			case 0x8004:
			case 0x8005:
			case 0x8006:
			case 0x8007:
				page_lookup[offset-0x8000] = data;
				return;

			/*case 0x8008:	// read-only register???
				key_buf = data;
				return*/

			case 0x8010:
			case 0x8011:
			case 0x8012:
			case 0x8013:
			case 0x8014:
			case 0x8015:
			case 0x8016:
			case 0x8017:
			case 0x8018:
			case 0x8019:
			case 0x801a:
			case 0x801b:
			case 0x801c:
			case 0x801d:
			case 0x801e:
			case 0x801f:
				mm58274c_w(0, offset-0xf130, data);
				return;

			default:
				logerror("unmapped write offs=%d data=%d\n", (int) offset, (int) data);
				return;
			}
		}
		else if ((offset >= 0x8400) && (offset < 0xa000))
		{
			/* 0x8400-0x9fff */
			/* ti99 memory-mapped registers */
			switch ((offset - 0x8000) >> 10)
			{
			case 1:
				/* sound write */
				SN76496_0_w(0, data);
				return;

			case 3:
				/* VDP write */
				if (! (offset & 1))
				{

					switch ((offset >> 1) & 3)
					{
					case 0:
						/* write VDP RAM */
						v9938_vram_w(0, data);
						break;
					case 1:
						/* write VDP address */
						v9938_command_w(0, data);
						break;
					case 2:
						/* write palette */
						v9938_palette_w(0, data);
						break;
					case 3:
						/* write register */
						v9938_register_w(0, data);
						break;
					}
				}
				return;

			case 5:
				/* speech write */
				if ((! (offset & 1)) && has_speech)
				{
					geneve_speech_w(0, data);
				}
				return;

			case 7:
				/* GPL write */
				if (! (offset & 1))
				{
					if (offset & 2)
					{	/* write GPL address */
						GPL_port.raddr_LSB = FALSE;

						if (GPL_port.waddr_LSB)
						{
							GPL_port.addr = (GPL_port.addr & 0xFF00) | data;

							/* increment */
							GPL_port.addr = (GPL_port.addr + 1) & 0xffff;

							GPL_port.waddr_LSB = FALSE;
						}
						else
						{
							GPL_port.addr = ((int) data << 8) | (GPL_port.addr & 0xFF);
	
							GPL_port.waddr_LSB = TRUE;
						}
					}
					else
					{	/* write GPL data: probably not implemented */
						GPL_port.addr = (GPL_port.addr + 1) & 0xffff;
						GPL_port.raddr_LSB = GPL_port.waddr_LSB = FALSE;
					}
				}
				return;

			default:
				logerror("unmapped write offs=%d data=%d\n", (int) offset, (int) data);
				return;
			}
		}
	}

	page = offset >> 13;

	if (! (mode_flags & mf_mapmode))
	{
		if ((mode_flags & mf_genevemode) || (page != 3))
			page = page_lookup[page];
		else
		{
			/* Cartridge page under ti99 emulation */
			if (! (mode_flags & mf_cartpaging))
			{	/* bankswitching */
				cartridge_page = (offset & 2) != 0;
				return;
			}
			else
			{
				/* writing to (non-bankswitched) cart -> page 0x36 */
				page = 0x36;
				if ((offset < 0x7000) ? (! (mode_flags & mf_cart6protect)) : (! (mode_flags & mf_cart7protect)))
				{
					/* Trying to write a write-protected cartridge */
					logerror("Ignoring write to a write-protected cartridge\n");
					return;
				}
			}
		}
	}
	else
		page = page + 0xf0;

	offset &= 0x1fff;

	switch (page)
	{
	case 0xf0:
	case 0xf1:
		/* Boot ROM */
		return;

	case 0xe8:
	case 0xe9:
	case 0xea:
	case 0xeb:
	case 0xec:
	case 0xed:
	case 0xee:
	case 0xef:
		/* SRAM */
		SRAM_ptr[(page-0xe8)*0x2000 + offset] = data;
		return;

#if 0
	case 0x7a:	/* mirror of 0xba??? */
		return;
#endif
	case 0xba:
		/* DSR space */
		geneve_peb_w(offset, data);
		return;

	case 0xbc:
		/* speech space */
		if (has_speech)
		{
			if ((offset >= 0x1400) && (offset < 0x1800) && (! (offset & 1)))
				geneve_speech_w(0, data);
			return;
		}

	default:
		/* DRAM */
		DRAM_ptr[page*0x2000 + offset] = data;
		return;
	}

	logerror("unmapped write page=%d offs=%d data=%d\n", (int) page, (int) offset, (int) data);
	return;
}


/*===========================================================================*/
#if 0
#pragma mark -
#pragma mark CRU HANDLERS
#endif

WRITE8_HANDLER ( geneve_peb_mode_cru_w )
{
	if ((offset >= /*0x770*/0x775) && (offset < 0x780))
	{
		int old_flags = mode_flags;

		/* tms9995 user flags */
		if (data)
			mode_flags |= 1 << (offset - 0x775);
		else
			mode_flags &= ~(1 << (offset - 0x775));

		if ((offset == 0x778) && data && (! (old_flags & mf_keyclock)))
		{
			/* set mf_keyclock */
			read_key_if_possible();
		}
		if (offset == 0x779)
		{
			if (data && (! (old_flags & mf_keyclear)))
			{
				/* set mf_keyclear: enable key input */
				/* shift in new key immediately if possible */
				read_key_if_possible();
			}
			else if ((! data) && (old_flags & mf_keyclear))
			{
				/* clear mf_keyclear: clear key input */
				if (KeyQueueLen)
				{
					KeyQueueHead = (KeyQueueHead + 1) % KeyQueueSize;
					KeyQueueLen--;
				}
				/* clear keyboard interrupt */
				tms9901_set_single_int(0, 8, 0);
				KeyInBuf = 0;
			}
		}
	}

	geneve_peb_cru_w(offset, data);
}

/*===========================================================================*/
#if 0
#pragma mark -
#pragma mark KEYBOARD INTERFACE
#endif

static void read_key_if_possible(void)
{
	/* if keyboard reset is not asserted, and key clock is enabled, and key
	buffer clear is disabled, and key queue is not empty. */
	if ((! KeyReset) && (mode_flags & mf_keyclock) && (mode_flags & mf_keyclear) && KeyQueueLen)
	{
		tms9901_set_single_int(0, 8, 1);
		KeyInBuf = 1;
	}
}

INLINE void post_in_KeyQueue(int keycode)
{
	KeyQueue[(KeyQueueHead+KeyQueueLen) % KeyQueueSize] = keycode;
	KeyQueueLen++;
}

static void poll_keyboard(void)
{
	UINT32 keystate;
	UINT32 key_transitions;
	int i, j;
	int keycode;
	int pressed;

	static const UINT8 keyboard_mf1_code[0xe] =
	{
		/* extended keys that are equivalent to non-extended keys */
		0x1c,	/* keypad enter */
		0x1d,	/* right control */
		0x38,	/* alt gr */
		/* extra codes are 0x5b for Left Windows, 0x5c for Right Windows, 0x5d
		for Menu, 0x5e for power, 0x5f for sleep, 0x63 for wake, but I doubt
		any Geneve program would take advantage of these. */

		/* extended key that is equivalent to a non-extended key
		with shift off */
		0x35,	/* pad slash */

		/* extended keys that are equivalent to non-extended keys
		with numlock off */
		0x47,	/* home */
		0x48,	/* up */
		0x49,	/* page up */
		0x4b,	/* left */
		0x4d,	/* right */
		0x4f,	/* end */
		0x50,	/* down */
		0x51,	/* page down */
		0x52,	/* insert */
		0x53	/* delete */
	};


	if (KeyReset)
		return;

	/* Poll keyboard */
	for (i=0; (i<4) && (KeyQueueLen <= (KeyQueueSize-MaxKeyMessageLen)); i++)
	{
		keystate = readinputport(input_port_keyboard_geneve + i*2)
					| (readinputport(input_port_keyboard_geneve + i*2 + 1) << 16);
		key_transitions = keystate ^ KeyStateSave[i];
		if (key_transitions)
		{
			for (j=0; (j<32) && (KeyQueueLen <= (KeyQueueSize-MaxKeyMessageLen)); j++)
			{
				if ((key_transitions >> j) & 1)
				{
					keycode = (i << 5) | j;
					pressed = ((keystate >> j) & 1);
					if (pressed)
						KeyStateSave[i] |= (1 << j);
					else
						KeyStateSave[i] &= ~ (1 << j);

					/* Update auto-repeat */
					if (pressed)
					{
						KeyAutoRepeatKey = keycode;
						KeyAutoRepeatTimer = KeyAutoRepeatDelay+1;
					}
					else /*if (keycode == KeyAutoRepeatKey)*/
						KeyAutoRepeatKey = 0;

					/* Release Fake Shift/Unshift if another key is pressed */
					/* We do so if a key is released, though it is actually
					required only if it is a modifier key */
					/*if (pressed)*/
					{
						if (KeyFakeShiftState)
						{
							/* Fake shift release */
							post_in_KeyQueue(0xe0);
							post_in_KeyQueue(0xaa);
							KeyFakeShiftState = 0;
						}
						if (KeyFakeUnshiftState)
						{
							/* Fake shift press */
							post_in_KeyQueue(0xe0);
							post_in_KeyQueue(0x2a);
							KeyFakeUnshiftState = 0;
						}
					}

					/* update shift and numlock state */
					if ((keycode == 0x2a) || (keycode == 0x36))
						KeyRealShiftState = KeyRealShiftState + (pressed ? +1 : -1);
					if ((keycode == 0x1d) || (keycode == 0x61))
						KeyCtrlState = KeyCtrlState + (pressed ? +1 : -1);
					if ((keycode == 0x38) || (keycode == 0x62))
						KeyAltState = KeyAltState + (pressed ? +1 : -1);
					if ((keycode == 0x45) && pressed)
						KeyNumLockState = ! KeyNumLockState;

					if ((keycode >= 0x60) && (keycode < 0x6e))
					{	/* simpler extended keys */
						/* these keys are emulated */

						if ((keycode >= 0x63) && pressed)
						{
							/* Handle shift state */
							if (keycode == 0x63)
							{	/* non-shifted key */
								if (KeyRealShiftState)
									/* Fake shift unpress */
									KeyFakeUnshiftState = 1;
							}
							else /*if (keycode >= 0x64)*/
							{	/* non-numlock mode key */
								if (KeyNumLockState & ! KeyRealShiftState)
									/* Fake shift press if numlock is active */
									KeyFakeShiftState = 1;
								else if ((! KeyNumLockState) & KeyRealShiftState)
									/* Fake shift unpress if shift is down */
									KeyFakeUnshiftState = 1;
							}

							if (KeyFakeShiftState)
							{
								post_in_KeyQueue(0xe0);
								post_in_KeyQueue(0x2a);
							}

							if (KeyFakeUnshiftState)
							{
								post_in_KeyQueue(0xe0);
								post_in_KeyQueue(0xaa);
							}
						}

						keycode = keyboard_mf1_code[keycode-0x60];
						if (! pressed)
							keycode |= 0x80;
						post_in_KeyQueue(0xe0);
						post_in_KeyQueue(keycode);
					}
					else if (keycode == 0x6e)
					{	/* emulate Print Screen / System Request (F13) key */
						/* this is a bit complex, as Alt+PrtScr -> SysRq */
						/* Additionally, Ctrl+PrtScr involves no fake shift press */
						if (KeyAltState)
						{
							/* SysRq */
							keycode = 0x54;
							if (! pressed)
								keycode |= 0x80;
							post_in_KeyQueue(keycode);
						}
						else
						{
							/* Handle shift state */
							if (pressed && (! KeyRealShiftState) && (! KeyCtrlState))
							{	/* Fake shift press */
								post_in_KeyQueue(0xe0);
								post_in_KeyQueue(0x2a);
								KeyFakeShiftState = 1;
							}

							keycode = 0x37;
							if (! pressed)
								keycode |= 0x80;
							post_in_KeyQueue(0xe0);
							post_in_KeyQueue(keycode);
						}
					}
					else if (keycode == 0x6f)
					{	/* emulate pause (F15) key */
						/* this is a bit complex, as Pause -> Ctrl+NumLock and
						Ctrl+Pause -> Ctrl+ScrLock.  Furthermore, there is no
						repeat or release. */
						if (pressed)
						{
							if (KeyCtrlState)
							{
								post_in_KeyQueue(0xe0);
								post_in_KeyQueue(0x46);
								post_in_KeyQueue(0xe0);
								post_in_KeyQueue(0xc6);
							}
							else
							{
								post_in_KeyQueue(0xe1);
								post_in_KeyQueue(0x1d);
								post_in_KeyQueue(0x45);
								post_in_KeyQueue(0xe1);
								post_in_KeyQueue(0x9d);
								post_in_KeyQueue(0xc5);
							}
						}
					}
					else
					{
						if (! pressed)
							keycode |= 0x80;
						post_in_KeyQueue(keycode);
					}
					read_key_if_possible();
				}
			}
		}
	}

	/* Handle auto-repeat */
	if ((KeyQueueLen <= (KeyQueueSize-MaxKeyMessageLen)) && KeyAutoRepeatKey && (--KeyAutoRepeatTimer == 0))
	{
		if ((KeyAutoRepeatKey >= 0x60) && (KeyAutoRepeatKey < 0x6e))
		{
			post_in_KeyQueue(0xe0);
			post_in_KeyQueue(keyboard_mf1_code[KeyAutoRepeatKey-0x60]);
		}
		else if (KeyAutoRepeatKey == 0x6e)
		{
			if (KeyAltState)
				post_in_KeyQueue(0x54);
			else
			{
				post_in_KeyQueue(0xe0);
				post_in_KeyQueue(0x37);
			}
		}
		else if (KeyAutoRepeatKey == 0x6f)
			;
		else
		{
			post_in_KeyQueue(KeyAutoRepeatKey);
		}
		read_key_if_possible();
		KeyAutoRepeatTimer = KeyAutoRepeatRate;
	}
}

static void poll_mouse(void)
{
	static int last_mx = 0, last_my = 0;
	int new_mx, new_my;
	int delta_x, delta_y, buttons;

	buttons = readinputport(input_port_mouse_buttons_geneve);
	new_mx = readinputport(input_port_mouse_deltax_geneve);
	new_my = readinputport(input_port_mouse_deltay_geneve);

	/* compute x delta */
	delta_x = new_mx - last_mx;

	/* check for wrap */
	if (delta_x > 0x80)
		delta_x = 0x100-delta_x;
	if  (delta_x < -0x80)
		delta_x = -0x100-delta_x;

	last_mx = new_mx;

	/* compute y delta */
	delta_y = new_my - last_my;

	/* check for wrap */
	if (delta_y > 0x80)
		delta_y = 0x100-delta_y;
	if  (delta_y < -0x80)
		delta_y = -0x100-delta_y;

	last_my = new_my;

	v9938_update_mouse_state(delta_x, delta_y, buttons & 3);
}


/*===========================================================================*/
#if 0
#pragma mark -
#pragma mark TMS9901 INTERFACE
#endif
/*
	Geneve-specific tms9901 I/O handlers

	See mess/machine/tms9901.c for generic tms9901 CRU handlers.
*/
/*
	TMS9901 interrupt handling on a Geneve.

	Geneve uses the following interrupts:
	INT1: external interrupt (used by RS232 controller, for instance) -
	  connected to tms9995 int4, too.
	INT2: VDP interrupt
	TMS9901 timer interrupt? (overrides INT3)
	INT8: keyboard interrupt???
	INT10: mouse interrupt???
	INT11: clock interrupt???
	INT12: INTB interrupt from PE-bus
*/

/*
	set the state of int2 (called by tms9928 core)
*/
/*void tms9901_set_int2(int state)
{
	tms9901_set_single_int(0, 2, state);
}*/

/*
	Called by the 9901 core whenever the state of INTREQ and IC0-3 changes
*/
static void tms9901_interrupt_callback(int intreq, int ic)
{
	/* INTREQ is connected to INT1 (IC0-3 are not connected) */
	cpunum_set_input_line(0, 0, intreq ? ASSERT_LINE : CLEAR_LINE);
}

/*
	Read pins INT3*-INT7* of Geneve 9901.

	signification:
	 (bit 1: INT1 status)
	 (bit 2: INT2 status)
	 bit 3-7: joystick status
*/
static int R9901_0(int offset)
{
	int answer;

	answer = readinputport(input_port_joysticks_geneve) >> (JoySel * 8);

	return (answer);
}

/*
	Read pins INT8*-INT15* of Geneve 9901.

	signification:
	 (bit 0: keyboard interrupt)
	 bit 1: unused
	 bit 2: mouse right button
	 (bit 3: clock interrupt)
	 (bit 4: INTB from PE-bus)
	 bit 5 & 7: used as output
	 bit 6: unused
*/
static int R9901_1(int offset)
{
	int answer;

	answer = (readinputport(input_port_mouse_buttons_geneve) & 4) ^ 4;

	return answer;
}

/*
	Read pins P0-P7 of Geneve 9901.
*/
static int R9901_2(int offset)
{
	return 0;
}

/*
	Read pins P8-P15 of Geneve 9901.
	bit 4: mouse right button
*/
static int R9901_3(int offset)
{
	int answer = 0;

	if (! (readinputport(input_port_mouse_buttons_geneve) & 4))
		answer |= 0x10;

	return answer;
}


/*
	Write PE bus reset line
*/
static void W9901_PE_bus_reset(int offset, int data)
{
}

/*
	Write VDP reset line
*/
static void W9901_VDP_reset(int offset, int data)
{
}

/*
	Write joystick select line
*/
static void W9901_JoySel(int offset, int data)
{
	JoySel = data;
}

static void W9901_KeyboardReset(int offset, int data)
{
	KeyReset = ! data;
	if (KeyReset)
	{
		/* reset -> clear keyboard key queue, but not geneve key buffer */
		KeyQueueLen = KeyInBuf ? 1 : 0;
		memset(KeyStateSave, 0, sizeof(KeyStateSave));
		KeyNumLockState = 0;
		KeyCtrlState = 0;
		KeyAltState = 0;
		KeyRealShiftState = 0;
		KeyFakeShiftState = 0;
		KeyFakeUnshiftState = 0;
		KeyAutoRepeatKey = 0;
	}
	/*else
		poll_keyboard();*/
}

/*
	Write external mem cycles (0=long, 1=short)
*/
static void W9901_ext_mem_wait_states(int offset, int data)
{
}

/*
	Write vdp wait cycles (1=add 15 cycles, 0=add none)
*/
static void W9901_VDP_wait_states(int offset, int data)
{
}



