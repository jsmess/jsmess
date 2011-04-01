/****************************************************************************

    machine/mac.c

    Mac hardware - Mac 128k, 512k, 512ke, Plus, SE, Classic, II (SCSI, SCC, ADB, etc)

    Nate Woods
    Ernesto Corvi
    Raphael Nabet
    R. Belmont

    Mac Model Feature Summary:
                                CPU             FDC     Kbd/Mouse  PRAM     Video
         - Mac 128k             68k             IWM     orig       orig     Original
         - Mac 512k             68k             IWM     orig       orig     Original
         - Mac 512ke            68k             IWM     orig       orig     Original
         - Mac Plus             68k             IWM     orig       ext      Original
         - Mac SE               68k             IWM     MacII ADB  ext      Original
         - Mac Classic          68k             SWIM    MacII ADB  ext      Original
         - Mac Portable         68k (16 MHz)    SWIM    ADB-PMU    PMU      640x400 B&W
         - PowerBook 100        68k (16 MHz)    SWIM    ADB-PMU    PMU      640x400 B&W
         - Mac II               020             IWM     MacII ADB  ext      NuBus card
         - Mac IIx              030             SWIM    MacII ADB  ext      NuBus card
         - Mac IIfx             030             SWIM    ADB-IOP    ext      NuBus card
         - Mac SE/30            030             SWIM    MacII ADB  ext      Internal fake NuBus card
         - Mac IIcx             030             SWIM    MacII ADB  ext      NuBus card
         - Mac IIci             030             SWIM    Egret ADB  ext      Internal "RBV" type
         - Mac IIsi             030             SWIM    Egret ADB  n/a      Internal "RBV" type
         - Mac IIvx/IIvi        030             SWIM    Egret ADB  n/a      Internal "VASP" type
         - Mac LC               020             SWIM    Egret ADB  n/a      Internal "V8" type
         - Mac LC II            030             SWIM    Egret ADB  n/a      Internal "V8" type
         - Mac LC III           030             SWIM    Egret ADB  n/a      Internal "Sonora" type
         - Mac Classic II       030             SWIM    Egret ADB  n/a      Internal "Eagle" type

    Notes:
        - The Mac Plus boot code seems to check to see the extent of ROM
          mirroring to determine if SCSI is available.  If the ROM is mirrored,
          then SCSI is not available.  Thanks to R. Belmont for making this
          discovery.
        - On the SE and most later Macs, the first access to 4xxxxx turns off the overlay.
          However, the Mac II/IIx/IIcx (and others?) have the old-style VIA overlay control bit!
        - The Mac II can have either a 68551 PMMU fitted or an Apple custom that handles 24 vs. 32
          bit addressing mode.  The ROM is *not* 32-bit clean so Mac OS normally runs in 24 bit mode,
          but A/UX can run 32.
        - There are 5 known kinds of host-side ADB hardware:
          * "Mac II ADB" used in the SE, II, IIx, IIcx, SE/30, IIci, Quadra 610, Quadra 650, Quadra 700,
             Quadra 800, Centris 610 and Centris 650.  This is a bit-banger using the VIA and a simple PIC.
          * "ADB-PMU" used in the Mac Portable and all 680x0-based PowerBooks.
          * "ADB-EGRET" used in the IIsi, IIvi, IIvx, Classic II, LC, LC II, LC III, Performa 460,
             and Performa 600.  This is a 68HC05 with a different internal ROM than CUDA.
          * "ADB-IOP" (ADB driven by a 6502 coprocessor, similar to Lisa) used in the IIfx,
            Quadra 900, and Quadra 950.
          * "ADB-CUDA" (Apple's CUDA chip, which is a 68HC05 MCU) used in the Color Classic, LC 520,
            LC 55x, LC 57x, LC 58x, Quadra 630, Quadra 660AV, Quadra 840AV, PowerMac 6100/7100/8100,
            IIci, and PowerMac 5200.

`    TODO:
        - Mac Portable and PowerBook 100 are similar to this hardware, but we need ROMs!
        - Call the RTC timer
        - SE FDHD and Classic require SWIM support.  SWIM is IWM compatible with 800k drives, but
          becomes a different chip for 1.44MB.
        - Check that 0x600000-0x6fffff still address RAM when overlay bit is off
          (IM-III seems to say it does not on Mac 128k, 512k, and 512ke).
        - What on earth are 0x700000-0x7fffff mapped to ?

     Egret version spotting:
     341S0417 - 0x???? (?.??) - Color Classic
     341S0850 - 0x???? (?.??) - LC, LC II
     341S0851 - 0x0101 (1.01) - Classic II, IIsi, IIvx, LC III (probably also IIvi?)

     Cuda version spotting:
     341S0060 - 0x00020028 (2.40) - Performa/Quadra 6xx, PMac 6200, x400, some x500, Pippin, Gossamer G3, others?
     341S0262 - 0x???????? (?.??) - some PMac 6500
     341S0285 - 0x???????? (?.??) - PMac 4400 + Mac clones
     341S0788 - 0x00020025 (2.37) - LC 475/575/Quadra 605, Quadra 660AV/840AV, PMac 7200
     343S0788 - 0x???????? (?.??) - PMac x100 (typo - actually 341S0788?)

     Caboose version spotting:
     341S0853 - 0x0100 (1.00) - Quadra 950

****************************************************************************/

#define ADDRESS_MAP_MODERN

#include <time.h>

#include "emu.h"
#include "machine/6522via.h"
#include "machine/8530scc.h"
#include "cpu/m68000/m68000.h"
#include "machine/applefdc.h"
#include "devices/sonydriv.h"
#include "machine/ncr5380.h"
#include "sound/asc.h"
#include "includes/mac.h"
#include "debug/debugcpu.h"
#include "machine/ram.h"
#include "debugger.h"

#define ADB_IS_BITBANG	((mac->m_model == MODEL_MAC_SE || mac->m_model == MODEL_MAC_CLASSIC) || (mac->m_model >= MODEL_MAC_II && mac->m_model <= MODEL_MAC_IICI) || (mac->m_model == MODEL_MAC_SE30))
#define ADB_IS_BITBANG_CLASS	((m_model == MODEL_MAC_SE || m_model == MODEL_MAC_CLASSIC) || (m_model >= MODEL_MAC_II && m_model <= MODEL_MAC_IICI) || (m_model == MODEL_MAC_SE30))
#define ADB_IS_EGRET	(mac->m_model >= MODEL_MAC_LC && mac->m_model <= MODEL_MAC_COLOR_CLASSIC) || ((mac->m_model >= MODEL_MAC_IISI) && (mac->m_model <= MODEL_MAC_IIVI))
#define ADB_IS_PM	(mac->m_model >= MODEL_MAC_PORTABLE && mac->m_model <= MODEL_MAC_PB100)
#define ADB_IS_PM_CLASS	(m_model >= MODEL_MAC_PORTABLE && m_model <= MODEL_MAC_PB100)

#define AUDIO_IS_CLASSIC (mac->m_model < MODEL_MAC_II) && (mac->m_model != MODEL_MAC_PORTABLE) && (mac->m_model != MODEL_MAC_PB100)
#define AUDIO_IS_CLASSIC_CLASS (m_model < MODEL_MAC_II) && (m_model != MODEL_MAC_PORTABLE) && (m_model != MODEL_MAC_PB100)
#define MAC_HAS_VIA2	(m_model >= MODEL_MAC_II)

#define ASC_INTS_RBV	((mac->m_model >= MODEL_MAC_IICI) && (mac->m_model <= MODEL_MAC_IIVI)) || ((mac->m_model >= MODEL_MAC_LC) && (mac->m_model <= MODEL_MAC_COLOR_CLASSIC))

#ifdef MAME_DEBUG
#define LOG_VIA			0
#define LOG_RTC			0
#define LOG_MAC_IWM		0
#define LOG_GENERAL		0
#define LOG_KEYBOARD	0
#define LOG_MEMORY		0
#else
#define LOG_VIA			0
#define LOG_RTC			0
#define LOG_MAC_IWM		0
#define LOG_GENERAL		0
#define LOG_KEYBOARD	0
#define LOG_MEMORY		0
#endif

#define LOG_ADB			0
#define LOG_ADB_MCU_CMD		0

// ADB states
#define ADB_STATE_NEW_COMMAND	(0)
#define ADB_STATE_XFER_EVEN	(1)
#define ADB_STATE_XFER_ODD	(2)
#define ADB_STATE_IDLE		(3)
#define ADB_STATE_NOTINIT	(4)

// ADB commands
#define ADB_CMD_RESET		(0)
#define ADB_CMD_FLUSH		(1)

static TIMER_CALLBACK(mac_scanline_tick);
static TIMER_CALLBACK(mac_adb_tick);
static TIMER_CALLBACK(mac_6015_tick);
static TIMER_CALLBACK(mac_pmu_tick);
static int scan_keyboard(running_machine &machine);
static TIMER_CALLBACK(inquiry_timeout_func);
static void keyboard_receive(running_machine &machine, int val);
static READ8_DEVICE_HANDLER(mac_via_in_a);
static READ8_DEVICE_HANDLER(mac_via_in_b);
static READ8_DEVICE_HANDLER(mac_adb_via_in_cb2);
static WRITE8_DEVICE_HANDLER(mac_via_out_a);
static WRITE8_DEVICE_HANDLER(mac_via_out_b);
static WRITE8_DEVICE_HANDLER(mac_adb_via_out_cb2);
static WRITE8_DEVICE_HANDLER(mac_via_out_cb2);
static READ8_DEVICE_HANDLER(mac_via2_in_a);
static READ8_DEVICE_HANDLER(mac_via2_in_b);
static WRITE8_DEVICE_HANDLER(mac_via2_out_a);
static WRITE8_DEVICE_HANDLER(mac_via2_out_b);
static void mac_via_irq(device_t *device, int state);
static void mac_via2_irq(device_t *device, int state);
static offs_t mac_dasm_override(device_t &device, char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram, int options);

const via6522_interface mac_via6522_intf =
{
	DEVCB_HANDLER(mac_via_in_a), DEVCB_HANDLER(mac_via_in_b),
	DEVCB_NULL, DEVCB_NULL,
	DEVCB_NULL, DEVCB_NULL,
	DEVCB_HANDLER(mac_via_out_a), DEVCB_HANDLER(mac_via_out_b),
	DEVCB_NULL, DEVCB_NULL,
	DEVCB_NULL, DEVCB_HANDLER(mac_via_out_cb2),
	DEVCB_LINE(mac_via_irq)
};

const via6522_interface mac_via6522_adb_intf =
{
	DEVCB_HANDLER(mac_via_in_a), DEVCB_HANDLER(mac_via_in_b),
	DEVCB_NULL, DEVCB_NULL,
	DEVCB_NULL, DEVCB_HANDLER(mac_adb_via_in_cb2),
	DEVCB_HANDLER(mac_via_out_a), DEVCB_HANDLER(mac_via_out_b),
	DEVCB_NULL, DEVCB_NULL,
	DEVCB_NULL, DEVCB_HANDLER(mac_adb_via_out_cb2),
	DEVCB_LINE(mac_via_irq)
};

const via6522_interface mac_via6522_2_intf =
{
	DEVCB_HANDLER(mac_via2_in_a), DEVCB_HANDLER(mac_via2_in_b),
	DEVCB_NULL, DEVCB_NULL,
	DEVCB_NULL, DEVCB_NULL,
	DEVCB_HANDLER(mac_via2_out_a), DEVCB_HANDLER(mac_via2_out_b),
	DEVCB_NULL, DEVCB_NULL,
	DEVCB_NULL, DEVCB_NULL,
	DEVCB_LINE(mac_via2_irq)
};

// returns non-zero if this Mac has ADB
int mac_state::has_adb()
{
	return m_model >= MODEL_MAC_SE;
}

// handle disk enable lines
void mac_fdc_set_enable_lines(device_t *device, int enable_mask)
{
	mac_state *mac = device->machine().driver_data<mac_state>();

	if (mac->m_model != MODEL_MAC_SE)
	{
		sony_set_enable_lines(device, enable_mask);
	}
	else
	{
		if (enable_mask)
		{
			sony_set_enable_lines(device, mac->m_drive_select ? 1 : 2);
		}
		else
		{
			sony_set_enable_lines(device, enable_mask);
		}
	}
}

static void mac_install_memory(running_machine &machine, offs_t memory_begin, offs_t memory_end,
	offs_t memory_size, void *memory_data, int is_rom, const char *bank)
{
	address_space* space = machine.device("maincpu")->memory().space(AS_PROGRAM);
	offs_t memory_mask;

	memory_size = MIN(memory_size, (memory_end + 1 - memory_begin));
	memory_mask = memory_size - 1;

	if (!is_rom)
	{
		space->install_readwrite_bank(memory_begin, memory_end, memory_mask, 0, bank);
	}
	else
	{
		space->unmap_write(memory_begin, memory_end, memory_mask, 0);
		space->install_read_bank(memory_begin, memory_end, memory_mask, 0, bank);
	}

	memory_set_bankptr(machine, bank, memory_data);

	if (1) //LOG_MEMORY)
	{
		logerror("mac_install_memory(): bank=%s range=[0x%06x...0x%06x] mask=0x%06x ptr=0x%p\n",
			bank, memory_begin, memory_end, memory_mask, memory_data);
	}
}



/*
    Interrupt handling
*/

void mac_state::field_interrupts()
{
	int take_interrupt = -1;

	if (m_model < MODEL_MAC_II)
	{
		if ((m_scc_interrupt) || (m_scsi_interrupt))
		{
			take_interrupt = 2;
		}
		else if (m_via_interrupt)
		{
			take_interrupt = 1;
		}
	}
	else if (m_model < MODEL_MAC_POWERMAC_6100)
	{
		if (m_scc_interrupt)
		{
			take_interrupt = 4;
		}
		else if (m_via2_interrupt)
		{
			take_interrupt = 2;
		}
		else if (m_via_interrupt)
		{
			take_interrupt = 1;
		}
	}
	else
	{
		return;	// no interrupts for PowerPC yet
	}

	if (m_last_taken_interrupt != -1)
	{
		cputag_set_input_line(machine(), "maincpu", m_last_taken_interrupt, CLEAR_LINE);
		m_last_taken_interrupt = -1;
	}

	if (take_interrupt != -1)
	{
		cputag_set_input_line(machine(), "maincpu", take_interrupt, ASSERT_LINE);
		m_last_taken_interrupt = take_interrupt;
	}
}

void mac_state::set_scc_interrupt(int value)
{
	m_scc_interrupt = value;
	this->field_interrupts();
}

void mac_state::set_via_interrupt(int value)
{
	m_via_interrupt = value;
	this->field_interrupts();
}


void mac_state::set_via2_interrupt(int value)
{
	m_via2_interrupt = value;
	this->field_interrupts();
}

void mac_asc_irq(device_t *device, int state)
{
	mac_state *mac = device->machine().driver_data<mac_state>();

	if (ASC_INTS_RBV)
	{
		if (state)
		{
			mac->m_rbv_regs[3] |= 0x90;	// any VIA 2 interrupt | sound interrupt

			if (mac->m_rbv_ier & 0x10)	// ASC on RBV is CB1, bit 4 of IER/IFR
			{
				mac->m_rbv_ifr |= 0x90;
				mac->set_via2_interrupt(1);
			}
		}
		else
		{
			mac->m_rbv_regs[3] &= ~0x90;
			mac->m_rbv_ifr &= ~0x10;
		}

	}
	else if (mac->m_model >= MODEL_MAC_II)
	{
		mac->m_via2->write_cb1(state^1);
	}

	// todo: portable/pb100 hook up ASC IRQ differently
}

WRITE16_MEMBER ( mac_state::mac_autovector_w )
{
	if (LOG_GENERAL)
		logerror("mac_autovector_w: offset=0x%08x data=0x%04x\n", offset, data);

	/* This should throw an exception */

	/* Not yet implemented */
}

READ16_MEMBER ( mac_state::mac_autovector_r )
{
	if (LOG_GENERAL)
		logerror("mac_autovector_r: offset=0x%08x\n", offset);

	/* This should throw an exception */

	/* Not yet implemented */
	return 0;
}

static void set_scc_waitrequest(int waitrequest)
{
	if (LOG_GENERAL)
		logerror("set_scc_waitrequest: waitrequest=%i\n", waitrequest);

	/* Not Yet Implemented */
}

void mac_state::v8_resize()
{
	offs_t memory_size;
	UINT8 *memory_data;
	int is_rom;

	is_rom = (m_overlay) ? 1 : 0;

	// get what memory we're going to map
	if (is_rom)
	{
		/* ROM mirror */
		memory_size = machine().region("bootrom")->bytes();
		memory_data = machine().region("bootrom")->base();
		is_rom = TRUE;
	}
	else
	{
		/* RAM */
		memory_size = ram_get_size(m_ram);
		memory_data = ram_get_ptr(m_ram);
		is_rom = FALSE;
	}

//  printf("mac_v8_resize: memory_size = %x, ctrl bits %02x (overlay %d = %s)\n", memory_size, mac->m_rbv_regs[1] & 0xe0, mac->m_overlay, is_rom ? "ROM" : "RAM");

	if (is_rom)
	{
		mac_install_memory(machine(), 0x00000000, memory_size-1, memory_size, memory_data, is_rom, "bank1");
	}
	else
	{
		address_space* space = machine().device("maincpu")->memory().space(AS_PROGRAM);
		UINT32 onboard_amt, simm_amt, simm_size;
		static const UINT32 simm_sizes[4] = { 0, 2*1024*1024, 4*1024*1024, 8*1024*1024 };

		// force unmap of entire RAM region
		space->unmap_write(0, 0x9fffff, 0x9fffff, 0);

		// LC has 2 MB built-in, all other V8-style machines have 4 MB
		// we reserve the first 2 or 4 MB of mess_ram for the onboard,
		// RAM above that mark is the SIMM
		onboard_amt = ((m_model == MODEL_MAC_LC) || (m_model == MODEL_MAC_CLASSIC_II)) ? 2*1024*1024 : 4*1024*1024;
		simm_amt = (m_rbv_regs[1]>>6) & 3;	// size of SIMM RAM window
		simm_size = memory_size - onboard_amt;	// actual amount of RAM available for SIMMs

		// installing SIMM RAM?
		if (simm_amt != 0)
		{
//          printf("mac_v8_resize: SIMM region size is %x, SIMM size is %x, onboard size is %x\n", simm_sizes[simm_amt], simm_size, onboard_amt);

			if ((simm_amt > 0) && (simm_size > 0))
			{
				mac_install_memory(machine(), 0x000000, simm_sizes[simm_amt]-1, simm_sizes[simm_amt], memory_data + onboard_amt, is_rom, "bank1");
			}

			// onboard RAM sits immediately above the SIMM, if any
			if (simm_sizes[simm_amt] + onboard_amt <= 0x800000)
			{
				mac_install_memory(machine(), simm_sizes[simm_amt], simm_sizes[simm_amt] + onboard_amt - 1, onboard_amt, memory_data, is_rom, "bank2");
			}

			// a mirror of the first 2 MB of on board RAM always lives at 0x800000
			mac_install_memory(machine(), 0x800000, 0x9fffff, 0x200000, memory_data, is_rom, "bank3");
		}
		else
		{
//          printf("mac_v8_resize: SIMM off, mobo RAM at 0 and top\n");

			mac_install_memory(machine(), 0x000000, onboard_amt-1, onboard_amt, memory_data, is_rom, "bank1");
			mac_install_memory(machine(), 0x800000, 0x9fffff, 0x200000, memory_data, is_rom, "bank3");
		}
	}
}

void mac_state::set_memory_overlay(int overlay)
{
	offs_t memory_size;
	UINT8 *memory_data;
	int is_rom;

	/* normalize overlay */
	overlay = overlay ? TRUE : FALSE;

	if (overlay != m_overlay)
	{
		/* set up either main RAM area or ROM mirror at 0x000000-0x3fffff */
		if (overlay)
		{
			/* ROM mirror */
			memory_size = machine().region("bootrom")->bytes();
			memory_data = machine().region("bootrom")->base();
			is_rom = TRUE;
		}
		else
		{
			/* RAM */
			memory_size = ram_get_size(m_ram);
			memory_data = ram_get_ptr(m_ram);
			is_rom = FALSE;
		}

		/* install the memory */
		if (((m_model >= MODEL_MAC_LC) && (m_model <= MODEL_MAC_COLOR_CLASSIC) && ((m_model != MODEL_MAC_LC_III) && (m_model != MODEL_MAC_LC_III_PLUS))) || (m_model == MODEL_MAC_CLASSIC_II))
		{
			m_overlay = overlay;
			this->v8_resize();
		}
		else if ((m_model >= MODEL_MAC_POWERMAC_6100) && (m_model >= MODEL_MAC_POWERMAC_8100))
		{
//          mac_install_memory(machine, 0x00000000, memory_size-1, memory_size, memory_data, is_rom, "bank1");
		}
		else if ((m_model == MODEL_MAC_IICI) || (m_model == MODEL_MAC_IISI))
		{
			// ROM is OK to flood to 3fffffff
			if (is_rom)
			{
				mac_install_memory(machine(), 0x00000000, 0x3fffffff, memory_size, memory_data, is_rom, "bank1");
			}
			else	// RAM: be careful not to populate ram B with a mirror or the ROM will get confused
			{
				mac_install_memory(machine(), 0x00000000, 0x03ffffff, memory_size, memory_data, is_rom, "bank1");
			}
		}
		else if ((m_model == MODEL_MAC_PORTABLE) || (m_model == MODEL_MAC_PB100))
		{
			mac_install_memory(machine(), 0x000000, 0x8fffff, memory_size, memory_data, is_rom, "bank1");
		}
		else if ((m_model >= MODEL_MAC_II) && (m_model <= MODEL_MAC_SE30))
		{
			mac_install_memory(machine(), 0x00000000, 0x3fffffff, memory_size, memory_data, is_rom, "bank1");
		}
		else if ((m_model == MODEL_MAC_LC_III) || (m_model == MODEL_MAC_LC_III_PLUS))	// up to 36 MB
		{
			mac_install_memory(machine(), 0x00000000, memory_size-1, memory_size, memory_data, is_rom, "bank1");
		}
		else
		{
			mac_install_memory(machine(), 0x00000000, 0x003fffff, memory_size, memory_data, is_rom, "bank1");
		}

		m_overlay = overlay;

		if (LOG_GENERAL)
			logerror("mac_set_memory_overlay: overlay=%i\n", overlay);
	}
}



/*
    R Nabet 000531 : added keyboard code
*/

/* *************************************************************************
 * non-ADB keyboard support
 *
 * The keyboard uses a i8021 (?) microcontroller.
 * It uses a bidirectional synchonous serial line, connected to the VIA (SR feature)
 *
 * Our emulation is more a hack than anything else - the keyboard controller is
 * not emulated, instead we interpret keyboard commands directly.  I made
 * many guesses, which may be wrong
 *
 * todo :
 * * find the correct model number for the Mac Plus keyboard ?
 * * emulate original Macintosh keyboards (2 layouts : US and international)
 *
 * references :
 * * IM III-29 through III-32 and III-39 through III-42
 * * IM IV-250
 * *************************************************************************/

/*
    scan_keyboard()

    scan the keyboard, and returns key transition code (or NULL ($7B) if none)
*/

static int scan_keyboard(running_machine &machine)
{
	int i, j;
	int keybuf;
	int keycode;
	static const char *const keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3", "KEY4", "KEY5", "KEY6" };
	mac_state *mac = machine.driver_data<mac_state>();

	if (mac->m_keycode_buf_index)
	{
		return mac->m_keycode_buf[--mac->m_keycode_buf_index];
	}

	for (i=0; i<7; i++)
	{
		keybuf = input_port_read(machine, keynames[i]);

		if (keybuf != mac->m_key_matrix[i])
		{
			/* if state has changed, find first bit which has changed */
			if (LOG_KEYBOARD)
				logerror("keyboard state changed, %d %X\n", i, keybuf);

			for (j=0; j<16; j++)
			{
				if (((keybuf ^ mac->m_key_matrix[i]) >> j) & 1)
				{
					/* update mac->m_key_matrix */
					mac->m_key_matrix[i] = (mac->m_key_matrix[i] & ~ (1 << j)) | (keybuf & (1 << j));

					if (i < 4)
					{
						/* create key code */
						keycode = (i << 5) | (j << 1) | 0x01;
						if (! (keybuf & (1 << j)))
						{
							/* key up */
							keycode |= 0x80;
						}
						return keycode;
					}
					else if (i < 6)
					{
						/* create key code */
						keycode = ((i & 3) << 5) | (j << 1) | 0x01;

						if ((keycode == 0x05) || (keycode == 0x0d) || (keycode == 0x11) || (keycode == 0x1b))
						{
							/* these keys cause shift to be pressed (for compatibility with mac 128/512) */
							if (keybuf & (1 << j))
							{
								/* key down */
								if (! (mac->m_key_matrix[3] & 0x0100))
								{
									/* shift key is really up */
									mac->m_keycode_buf[0] = keycode;
									mac->m_keycode_buf[1] = 0x79;
									mac->m_keycode_buf_index = 2;
									return 0x71;	/* "presses" shift down */
								}
							}
							else
							{	/* key up */
								if (! (mac->m_key_matrix[3] & 0x0100))
								{
									/* shift key is really up */
									mac->m_keycode_buf[0] = keycode | 0x80;;
									mac->m_keycode_buf[1] = 0x79;
									mac->m_keycode_buf_index = 2;
									return 0xF1;	/* "releases" shift */
								}
							}
						}

						if (! (keybuf & (1 << j)))
						{
							/* key up */
							keycode |= 0x80;
						}
						mac->m_keycode_buf[0] = keycode;
						mac->m_keycode_buf_index = 1;
						return 0x79;
					}
					else /* i == 6 */
					{
						/* create key code */
						keycode = (j << 1) | 0x01;
						if (! (keybuf & (1 << j)))
						{
							/* key up */
							keycode |= 0x80;
						}
						mac->m_keycode_buf[0] = keycode;
						mac->m_keycode_buf_index = 1;
						return 0x79;
					}
				}
			}
		}
	}

	return 0x7B;	/* return NULL */
}

/*
    power-up init
*/
static void keyboard_init(mac_state *mac)
{
	int i;

	/* init flag */
	mac->m_kbd_comm = FALSE;
	mac->m_kbd_receive = FALSE;
	mac->m_kbd_shift_reg=0;
	mac->m_kbd_shift_count=0;

	/* clear key matrix */
	for (i=0; i<7; i++)
	{
		mac->m_key_matrix[i] = 0;
	}

	/* purge transmission buffer */
	mac->m_keycode_buf_index = 0;
}

/******************* Keyboard <-> VIA communication ***********************/

static TIMER_CALLBACK(kbd_clock)
{
	int i;
	mac_state *mac = machine.driver_data<mac_state>();

	if (mac->m_kbd_comm == TRUE)
	{
		for (i=0; i<8; i++)
		{
			/* Put data on CB2 if we are sending*/
			if (mac->m_kbd_receive == FALSE)
				mac->m_via1->write_cb2(mac->m_kbd_shift_reg&0x80?1:0);
			mac->m_kbd_shift_reg <<= 1;
			mac->m_via1->write_cb1(0);
			mac->m_via1->write_cb1(1);
		}
		if (mac->m_kbd_receive == TRUE)
		{
			mac->m_kbd_receive = FALSE;
			/* Process the command received from mac */
			keyboard_receive(machine, mac->m_kbd_shift_reg & 0xff);
		}
		else
		{
			/* Communication is over */
			mac->m_kbd_comm = FALSE;
		}
	}
}

static void kbd_shift_out(running_machine &machine, int data)
{
	mac_state *mac = machine.driver_data<mac_state>();

	if (mac->m_kbd_comm == TRUE)
	{
		mac->m_kbd_shift_reg = data;
		machine.scheduler().timer_set(attotime::from_msec(1), FUNC(kbd_clock));
	}
}

static WRITE8_DEVICE_HANDLER(mac_via_out_cb2)
{
	mac_state *mac = device->machine().driver_data<mac_state>();

	if (mac->m_kbd_comm == FALSE && data == 0)
	{
		/* Mac pulls CB2 down to initiate communication */
		mac->m_kbd_comm = TRUE;
		mac->m_kbd_receive = TRUE;
		device->machine().scheduler().timer_set(attotime::from_usec(100), FUNC(kbd_clock));
	}
	if (mac->m_kbd_comm == TRUE && mac->m_kbd_receive == TRUE)
	{
		/* Shift in what mac is sending */
		mac->m_kbd_shift_reg = (mac->m_kbd_shift_reg & ~1) | data;
	}
}

/*
    called when inquiry times out (1/4s)
*/
static TIMER_CALLBACK(inquiry_timeout_func)
{
	if (LOG_KEYBOARD)
		logerror("keyboard enquiry timeout\n");
	kbd_shift_out(machine, 0x7B);	/* always send NULL */
}

/*
    called when a command is received from the mac
*/
static void keyboard_receive(running_machine &machine, int val)
{
	mac_state *mac = machine.driver_data<mac_state>();

	switch (val)
	{
	case 0x10:
		/* inquiry - returns key transition code, or NULL ($7B) if time out (1/4s) */
		if (LOG_KEYBOARD)
			logerror("keyboard command : inquiry\n");

		mac->m_keyboard_reply = scan_keyboard(machine);
		if (mac->m_keyboard_reply == 0x7B)
		{
			/* if NULL, wait until key pressed or timeout */
			mac->m_inquiry_timeout->adjust(
				attotime(0, DOUBLE_TO_ATTOSECONDS(0.25)), 0);
		}
		break;

	case 0x14:
		/* instant - returns key transition code, or NULL ($7B) */
		if (LOG_KEYBOARD)
			logerror("keyboard command : instant\n");

		kbd_shift_out(machine, scan_keyboard(machine));
		break;

	case 0x16:
		/* model number - resets keyboard, return model number */
		if (LOG_KEYBOARD)
			logerror("keyboard command : model number\n");

		{	/* reset */
			int i;

			/* clear key matrix */
			for (i=0; i<7; i++)
			{
				mac->m_key_matrix[i] = 0;
			}

			/* purge transmission buffer */
			mac->m_keycode_buf_index = 0;
		}

		/* format : 1 if another device (-> keypad ?) connected | next device (-> keypad ?) number 1-8
                            | keyboard model number 1-8 | 1  */
		/* keyboards :
            3 : mac 512k, US and international layout ? Mac plus ???
            other values : Apple II keyboards ?
        */
		/* keypads :
            ??? : standard keypad (always available on Mac Plus) ???
        */
		kbd_shift_out(machine, 0x17);	/* probably wrong */
		break;

	case 0x36:
		/* test - resets keyboard, return ACK ($7D) or NAK ($77) */
		if (LOG_KEYBOARD)
			logerror("keyboard command : test\n");

		kbd_shift_out(machine, 0x7D);	/* ACK */
		break;

	default:
		if (LOG_KEYBOARD)
			logerror("unknown keyboard command 0x%X\n", val);

		kbd_shift_out(machine, 0);
		break;
	}
}

/* *************************************************************************
 * Mouse
 * *************************************************************************/

void mac_state::mouse_callback()
{
	int		new_mx, new_my;
	int		x_needs_update = 0, y_needs_update = 0;
	mac_state *mac = machine().driver_data<mac_state>();

	new_mx = input_port_read(machine(), "MOUSE1");
	new_my = input_port_read(machine(), "MOUSE2");

	/* see if it moved in the x coord */
	if (new_mx != last_mx)
	{
		int		diff = new_mx - last_mx;

		/* check for wrap */
		if (diff > 0x80)
			diff = 0x100-diff;
		if  (diff < -0x80)
			diff = -0x100-diff;

		count_x += diff;

		last_mx = new_mx;
	}
	/* see if it moved in the y coord */
	if (new_my != last_my)
	{
		int		diff = new_my - last_my;

		/* check for wrap */
		if (diff > 0x80)
			diff = 0x100-diff;
		if  (diff < -0x80)
			diff = -0x100-diff;

		count_y += diff;

		last_my = new_my;
	}

	/* update any remaining count and then return */
	if (count_x)
	{
		if (count_x < 0)
		{
			count_x++;
			mac->m_mouse_bit_x = 0;
		}
		else
		{
			count_x--;
			mac->m_mouse_bit_x = 1;
		}
		x_needs_update = 1;
	}
	else if (count_y)
	{
		if (count_y < 0)
		{
			count_y++;
			mac->m_mouse_bit_y = 1;
		}
		else
		{
			count_y--;
			mac->m_mouse_bit_y = 0;
		}
		y_needs_update = 1;
	}

	if (x_needs_update || y_needs_update)
		/* assert Port B External Interrupt on the SCC */
		mac->scc_mouse_irq(x_needs_update, y_needs_update );
}

/* *************************************************************************
 * SCSI
 * *************************************************************************/

/*

From http://www.mac.m68k-linux.org/devel/plushw.php

The address of each register is computed as follows:

  $580drn

  where r represents the register number (from 0 through 7),
  n determines whether it a read or write operation
  (0 for reads, or 1 for writes), and
  d determines whether the DACK signal to the NCR 5380 is asserted.
  (0 for not asserted, 1 is for asserted)

Here's an example of the address expressed in binary:

  0101 1000 0000 00d0 0rrr 000n

Note:  Asserting the DACK signal applies only to write operations to
       the output data register and read operations from the input
       data register.

  Symbolic            Memory
  Location            Location   NCR 5380 Internal Register

  scsiWr+sODR+dackWr  $580201    Output Data Register with DACK
  scsiWr+sODR         $580001    Output Data Register
  scsiWr+sICR         $580011    Initiator Command Register
  scsiWr+sMR          $580021    Mode Register
  scsiWr+sTCR         $580031    Target Command Register
  scsiWr+sSER         $580041    Select Enable Register
  scsiWr+sDMAtx       $580051    Start DMA Send
  scsiWr+sTDMArx      $580061    Start DMA Target Receive
  scsiWr+sIDMArx      $580071    Start DMA Initiator Receive

  scsiRd+sIDR+dackRd  $580260    Current SCSI Data with DACK
  scsiRd+sCDR         $580000    Current SCSI Data
  scsiRd+sICR         $580010    Initiator Command Register
  scsiRd+sMR          $580020    Mode Registor
  scsiRd+sTCR         $580030    Target Command Register
  scsiRd+sCSR         $580040    Current SCSI Bus Status
  scsiRd+sBSR         $580050    Bus and Status Register
  scsiRd+sIDR         $580060    Input Data Register
  scsiRd+sRESET       $580070    Reset Parity/Interrupt
             */

READ16_MEMBER ( mac_state::macplus_scsi_r )
{
	int reg = (offset>>3) & 0xf;
	device_t *ncr = space.machine().device("ncr5380");

//  logerror("macplus_scsi_r: offset %x mask %x\n", offset, mem_mask);

	if ((reg == 6) && (offset == 0x130))
	{
		reg = R5380_CURDATA_DTACK;
	}

	return ncr5380_read_reg(ncr, reg)<<8;
}

READ32_MEMBER (mac_state::macii_scsi_drq_r)
{
	device_t *ncr = space.machine().device("ncr5380");

	switch (mem_mask)
	{
		case 0xff000000:
			return ncr5380_read_reg(ncr, R5380_CURDATA_DTACK)<<24;

		case 0xffff0000:
			return (ncr5380_read_reg(ncr, R5380_CURDATA_DTACK)<<24) | (ncr5380_read_reg(ncr, R5380_CURDATA_DTACK)<<16);

		case 0xffffffff:
			return (ncr5380_read_reg(ncr, R5380_CURDATA_DTACK)<<24) | (ncr5380_read_reg(ncr, R5380_CURDATA_DTACK)<<16) | (ncr5380_read_reg(ncr, R5380_CURDATA_DTACK)<<8) | ncr5380_read_reg(ncr, R5380_CURDATA_DTACK);

		default:
			logerror("macii_scsi_drq_r: unknown mem_mask %08x\n", mem_mask);
	}

	return 0;
}

WRITE32_MEMBER (mac_state::macii_scsi_drq_w)
{
	device_t *ncr = space.machine().device("ncr5380");

	switch (mem_mask)
	{
		case 0xff000000:
			ncr5380_write_reg(ncr, R5380_OUTDATA_DTACK, data>>24);
			break;

		case 0xffff0000:
			ncr5380_write_reg(ncr, R5380_OUTDATA_DTACK, data>>24);
			ncr5380_write_reg(ncr, R5380_OUTDATA_DTACK, data>>16);
			break;

		case 0xffffffff:
			ncr5380_write_reg(ncr, R5380_OUTDATA_DTACK, data>>24);
			ncr5380_write_reg(ncr, R5380_OUTDATA_DTACK, data>>16);
			ncr5380_write_reg(ncr, R5380_OUTDATA_DTACK, data>>8);
			ncr5380_write_reg(ncr, R5380_OUTDATA_DTACK, data&0xff);
			break;

		default:
			logerror("macii_scsi_drq_w: unknown mem_mask %08x\n", mem_mask);
			break;
	}
}

WRITE16_MEMBER ( mac_state::macplus_scsi_w )
{
	int reg = (offset>>3) & 0xf;
	device_t *ncr = space.machine().device("ncr5380");

//  logerror("macplus_scsi_w: data %x offset %x mask %x\n", data, offset, mem_mask);

	if ((reg == 0) && (offset == 0x100))
	{
		reg = R5380_OUTDATA_DTACK;
	}

	ncr5380_write_reg(ncr, reg, data);
}

WRITE16_MEMBER ( mac_state::macii_scsi_w )
{
	int reg = (offset>>3) & 0xf;
	device_t *ncr = space.machine().device("ncr5380");

//  logerror("macplus_scsi_w: data %x offset %x mask %x (PC=%x)\n", data, offset, mem_mask, cpu_get_pc(&space->device()));

	if ((reg == 0) && (offset == 0x100))
	{
		reg = R5380_OUTDATA_DTACK;
	}

	ncr5380_write_reg(ncr, reg, data>>8);
}

void mac_scsi_irq(running_machine &machine, int state)
{
	mac_state *mac = machine.driver_data<mac_state>();

	if ((mac->m_scsiirq_enable) && ((mac->m_model == MODEL_MAC_SE) || (mac->m_model == MODEL_MAC_CLASSIC)))
	{
		mac->m_scsi_interrupt = state;
		mac->field_interrupts();
	}
}

/* *************************************************************************
 * SCC
 *
 * Serial Communications Controller
 * *************************************************************************/

void mac_scc_irq(device_t *device, int status)
{
	mac_state *mac = device->machine().driver_data<mac_state>();

	mac->set_scc_interrupt(status);
}



void mac_state::scc_mouse_irq(int x, int y)
{
	device_t *scc = machine().device("scc");
	if (x && y)
	{
		if (m_last_was_x)
			scc8530_set_status(scc, 0x0a);
		else
			scc8530_set_status(scc, 0x02);

		m_last_was_x ^= 1;
	}
	else
	{
		if (x)
			scc8530_set_status(scc, 0x0a);
		else
			scc8530_set_status(scc, 0x02);
	}

	//cputag_set_input_line(machine, "maincpu", 2, ASSERT_LINE);
	this->set_scc_interrupt(1);
}



READ16_MEMBER ( mac_state::mac_scc_r )
{
	device_t *scc = space.machine().device("scc");
	UINT16 result;

	result = scc8530_r(scc, offset);
	return (result << 8) | result;
}



WRITE16_MEMBER ( mac_state::mac_scc_w )
{
	device_t *scc = space.machine().device("scc");
	scc8530_w(scc, offset, (UINT8) data);
}

WRITE16_MEMBER ( mac_state::mac_scc_2_w )
{
	device_t *scc = space.machine().device("scc");
	UINT8 wdata = data>>8;
	scc8530_w(scc, offset, wdata);
}



/* *************************************************************************
 * RTC
 *
 * Real Time Clock chip - contains clock information and PRAM.  This chip is
 * accessed through the VIA
 * *************************************************************************/
enum
{
	RTC_STATE_NORMAL,
	RTC_STATE_WRITE,
	RTC_STATE_XPCOMMAND,
	RTC_STATE_XPWRITE
};

/* init the rtc core */
void mac_state::rtc_init()
{
	m_rtc_rTCClk = 0;

	m_rtc_write_protect = 0;
	m_rtc_rTCEnb = 0;
	rtc_write_rTCEnb(1);
	m_rtc_state = RTC_STATE_NORMAL;
}

/* write the rTCEnb state */
void mac_state::rtc_write_rTCEnb(int val)
{
	if (val && (! m_rtc_rTCEnb))
	{
		/* rTCEnb goes high (inactive) */
		m_rtc_rTCEnb = 1;
		/* abort current transmission */
		m_rtc_data_byte = m_rtc_bit_count = m_rtc_data_dir = m_rtc_data_out = 0;
		m_rtc_state = RTC_STATE_NORMAL;
	}
	else if ((! val) && m_rtc_rTCEnb)
	{
		/* rTCEnb goes low (active) */
		m_rtc_rTCEnb = 0;
		/* abort current transmission */
		m_rtc_data_byte = m_rtc_bit_count = m_rtc_data_dir = m_rtc_data_out = 0;
		m_rtc_state = RTC_STATE_NORMAL;
	}
}

/* shift data (called on rTCClk high-to-low transition (?)) */
void mac_state::rtc_shift_data(int data)
{
	if (m_rtc_rTCEnb)
		/* if enable line inactive (high), do nothing */
		return;

	if (m_rtc_data_dir)
	{	/* RTC -> VIA transmission */
		m_rtc_data_out = (m_rtc_data_byte >> --m_rtc_bit_count) & 0x01;
		if (LOG_RTC)
			logerror("RTC shifted new data %d\n", m_rtc_data_out);
	}
	else
	{	/* VIA -> RTC transmission */
		m_rtc_data_byte = (m_rtc_data_byte << 1) | (data ? 1 : 0);

		if (++m_rtc_bit_count == 8)
		{	/* if one byte received, send to command interpreter */
			rtc_execute_cmd(m_rtc_data_byte);
		}
	}
}

/* called every second, to increment the Clock count */
void mac_state::rtc_incticks()
{
	if (LOG_RTC)
		logerror("rtc_incticks called\n");

	if (++m_rtc_seconds[0] == 0)
		if (++m_rtc_seconds[1] == 0)
			if (++m_rtc_seconds[2] == 0)
				++m_rtc_seconds[3];

	/*if (++m_rtc_seconds[4] == 0)
        if (++m_rtc_seconds[5] == 0)
            if (++m_rtc_seconds[6] == 0)
                ++m_rtc_seconds[7];*/
}

/* Executes a command.
Called when the first byte after "enable" is received, and when the data byte after a write command
is received. */
void mac_state::rtc_execute_cmd(int data)
{
	int i;

	if (LOG_RTC)
		logerror("rtc_execute_cmd: data=%x, state=%x\n", data, m_rtc_state);

	if (m_rtc_state == RTC_STATE_XPCOMMAND)
	{
		m_rtc_xpaddr = ((m_rtc_cmd & 7)<<5) | ((data&0x7c)>>2);
		if ((m_rtc_cmd & 0x80) != 0)
		{
			// read command
			if (LOG_RTC)
				logerror("RTC: Reading extended address %x = %x\n", m_rtc_xpaddr, m_rtc_ram[m_rtc_xpaddr]);

			m_rtc_data_dir = 1;
			m_rtc_data_byte = m_rtc_ram[m_rtc_xpaddr];
			m_rtc_state = RTC_STATE_NORMAL;
		}
		else
		{
			// write command
			m_rtc_state = RTC_STATE_XPWRITE;
			m_rtc_data_byte = 0;
			m_rtc_bit_count = 0;
		}
	}
	else if (m_rtc_state == RTC_STATE_XPWRITE)
	{
		if (LOG_RTC)
			logerror("RTC: writing %x to extended address %x\n", data, m_rtc_xpaddr);
		m_rtc_ram[m_rtc_xpaddr] = data;
		m_rtc_state = RTC_STATE_NORMAL;
	}
	else if (m_rtc_state == RTC_STATE_WRITE)
	{
		m_rtc_state = RTC_STATE_NORMAL;

		/* Writing an RTC register */
		i = (m_rtc_cmd >> 2) & 0x1f;
		if (m_rtc_write_protect && (i != 13))
			/* write-protection : only write-protect can be written again */
			return;
		switch(i)
		{
		case 0: case 1: case 2: case 3:	/* seconds register */
		case 4: case 5: case 6: case 7:	/* ??? (not described in IM III) */
			/* after various tries, I assumed m_rtc_seconds[4+i] is mapped to m_rtc_seconds[i] */
			if (LOG_RTC)
				logerror("RTC clock write, address = %X, data = %X\n", i, (int) m_rtc_data_byte);
			m_rtc_seconds[i & 3] = m_rtc_data_byte;
			break;

		case 8: case 9: case 10: case 11:	/* RAM address $10-$13 */
			if (LOG_RTC)
				logerror("RTC RAM write, address = %X, data = %X\n", (i & 3) + 0x10, (int) m_rtc_data_byte);
			m_rtc_ram[(i & 3) + 0x10] = m_rtc_data_byte;
			break;

		case 12:
			/* Test register - do nothing */
			if (LOG_RTC)
				logerror("RTC write to test register, data = %X\n", (int) m_rtc_data_byte);
			break;

		case 13:
			/* Write-protect register  */
			if (LOG_RTC)
				logerror("RTC write to write-protect register, data = %X\n", (int) m_rtc_data_byte&0x80);
			m_rtc_write_protect = (m_rtc_data_byte & 0x80) ? TRUE : FALSE;
			break;

		case 16: case 17: case 18: case 19:	/* RAM address $00-$0f */
		case 20: case 21: case 22: case 23:
		case 24: case 25: case 26: case 27:
		case 28: case 29: case 30: case 31:
			if (LOG_RTC)
				logerror("RTC RAM write, address = %X, data = %X\n", i & 15, (int) m_rtc_data_byte);
			m_rtc_ram[i & 15] = m_rtc_data_byte;
			break;

		default:
			printf("Unknown RTC write command : %X, data = %d\n", (int) m_rtc_cmd, (int) m_rtc_data_byte);
			break;
		}
	}
	else
	{
		// always save this byte to m_rtc_cmd
		m_rtc_cmd = m_rtc_data_byte;

		if ((m_rtc_cmd & 0x78) == 0x38)	// extended command
		{
			m_rtc_state = RTC_STATE_XPCOMMAND;
			m_rtc_data_byte = 0;
			m_rtc_bit_count = 0;
		}
		else
		{
			if (m_rtc_cmd & 0x80)
			{
				m_rtc_state = RTC_STATE_NORMAL;

				/* Reading an RTC register */
				m_rtc_data_dir = 1;
				i = (m_rtc_cmd >> 2) & 0x1f;
				switch(i)
				{
					case 0: case 1: case 2: case 3:
					case 4: case 5: case 6: case 7:
						m_rtc_data_byte = m_rtc_seconds[i & 3];
						if (LOG_RTC)
							logerror("RTC clock read, address = %X -> data = %X\n", i, m_rtc_data_byte);
						break;

					case 8: case 9: case 10: case 11:
						if (LOG_RTC)
							logerror("RTC RAM read, address = %X data = %x\n", (i & 3) + 0x10, m_rtc_ram[(i & 3) + 0x10]);
						m_rtc_data_byte = m_rtc_ram[(i & 3) + 0x10];
						break;

					case 16: case 17: case 18: case 19:
					case 20: case 21: case 22: case 23:
					case 24: case 25: case 26: case 27:
					case 28: case 29: case 30: case 31:
						if (LOG_RTC)
							logerror("RTC RAM read, address = %X data = %x\n", i & 15, m_rtc_ram[i & 15]);
						m_rtc_data_byte = m_rtc_ram[i & 15];
						break;

					default:
						if (LOG_RTC)
							logerror("Unknown RTC read command : %X\n", (int) m_rtc_cmd);
						m_rtc_data_byte = 0;
						break;
				}
			}
			else
			{
				/* Writing an RTC register */
				/* wait for extra data byte */
				if (LOG_RTC)
					logerror("RTC write, waiting for data byte : %X\n", (int) m_rtc_cmd);
				m_rtc_state = RTC_STATE_WRITE;
				m_rtc_data_byte = 0;
				m_rtc_bit_count = 0;
			}
		}
	}
}

/* should save PRAM to file */
/* TODO : save write_protect flag, save time difference with host clock */
NVRAM_HANDLER( mac )
{
	mac_state *mac = machine.driver_data<mac_state>();

	if (read_or_write)
	{
		if (LOG_RTC)
			logerror("Writing PRAM to file\n");
		file->write(mac->m_rtc_ram, sizeof(mac->m_rtc_ram));
	}
	else
	{
		if (file)
		{
			if (LOG_RTC)
				logerror("Reading PRAM from file\n");
			file->read(mac->m_rtc_ram, sizeof(mac->m_rtc_ram));
		}
		else
		{
			if (LOG_RTC)
				logerror("trashing PRAM\n");

			memset(mac->m_rtc_ram, 0, sizeof(mac->m_rtc_ram));

			// some Mac ROMs are buggy in the presence of
			// no NVRAM, so let's initialize it right
			mac->m_rtc_ram[0] = 0xa8;	// valid
			mac->m_rtc_ram[4] = 0xcc;
			mac->m_rtc_ram[5] = 0x0a;
			mac->m_rtc_ram[6] = 0xcc;
			mac->m_rtc_ram[7] = 0x0a;
			mac->m_rtc_ram[0xc] = 0x42;	// XPRAM valid for Plus/SE
			mac->m_rtc_ram[0xd] = 0x75;
			mac->m_rtc_ram[0xe] = 0x67;
			mac->m_rtc_ram[0xf] = 0x73;
			mac->m_rtc_ram[0x10] = 0x18;
			mac->m_rtc_ram[0x11] = 0x88;
			mac->m_rtc_ram[0x12] = 0x01;
			mac->m_rtc_ram[0x13] = 0x4c;

			if (mac->m_model >= MODEL_MAC_II)
			{
				mac->m_rtc_ram[0xc] = 0x4e;	// XPRAM valid is different for these
				mac->m_rtc_ram[0xd] = 0x75;
				mac->m_rtc_ram[0xe] = 0x4d;
				mac->m_rtc_ram[0xf] = 0x63;
				mac->m_rtc_ram[0x77] = 0x01;
				mac->m_rtc_ram[0x78] = 0xff;
				mac->m_rtc_ram[0x79] = 0xff;
				mac->m_rtc_ram[0x7a] = 0xff;
				mac->m_rtc_ram[0x7b] = 0xdf;
			}
		}

		{
			/* Now we copy the host clock into the Mac clock */
			/* Cool, isn't it ? :-) */
			system_time systime;
			struct tm mac_reference;
			UINT32 seconds;

			machine.base_datetime(systime);

			/* The count starts on 1st January 1904 */
			mac_reference.tm_sec = 0;
			mac_reference.tm_min = 0;
			mac_reference.tm_hour = 0;
			mac_reference.tm_mday = 1;
			mac_reference.tm_mon = 0;
			mac_reference.tm_year = 4;
			mac_reference.tm_isdst = 0;

			seconds = difftime((time_t) systime.time, mktime(& mac_reference));

			if (LOG_RTC)
				logerror("second count 0x%lX\n", (unsigned long) seconds);

			mac->m_rtc_seconds[0] = seconds & 0xff;
			mac->m_rtc_seconds[1] = (seconds >> 8) & 0xff;
			mac->m_rtc_seconds[2] = (seconds >> 16) & 0xff;
			mac->m_rtc_seconds[3] = (seconds >> 24) & 0xff;
		}
	}
}

/* ********************************** *
 * IWM Code specific to the Mac Plus  *
 * ********************************** */

READ16_MEMBER ( mac_state::mac_iwm_r )
{
	/* The first time this is called is in a floppy test, which goes from
     * $400104 to $400126.  After that, all access to the floppy goes through
     * the disk driver in the MacOS
     *
     * I just thought this would be on interest to someone trying to further
     * this driver along
     */

	UINT16 result = 0;
	device_t *fdc = space.machine().device("fdc");

	result = applefdc_r(fdc, (offset >> 8));

	if (LOG_MAC_IWM)
		printf("mac_iwm_r: offset=0x%08x mem_mask %04x = %02x (PC %x)\n", offset, mem_mask, result, cpu_get_pc(&space.device()));

	return (result << 8) | result;
}

WRITE16_MEMBER ( mac_state::mac_iwm_w )
{
	device_t *fdc = space.machine().device("fdc");

	if (LOG_MAC_IWM)
		printf("mac_iwm_w: offset=0x%08x data=0x%04x mask %04x (PC=%x)\n", offset, data, mem_mask, cpu_get_pc(&space.device()));

	if (ACCESSING_BITS_0_7)
		applefdc_w(fdc, (offset >> 8), data & 0xff);
	else
		applefdc_w(fdc, (offset >> 8), data>>8);
}

/* *************************************************************************
 * ADB (Mac II-style)
 * *************************************************************************/

#if LOG_ADB
static const char *const adb_statenames[4] = { "NEW", "EVEN", "ODD", "IDLE" };
#endif

int mac_state::adb_pollkbd(int update)
{
	int i, j, keybuf, report, codes[2], result;
	static const char *const keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3", "KEY4", "KEY5" };

	codes[0] = codes[1] = 0x80;	// key up
	report = result = 0;

	for (i = 0; i < 6; i++)
	{
		keybuf = input_port_read(machine(), keynames[i]);

		// any changes in this row?
		if ((keybuf != m_key_matrix[i]) && (report < 2))
		{
			// check each column bit
			for (j=0; j<16; j++)
			{
				if (((keybuf ^ m_key_matrix[i]) >> j) & 1)
				{
					// update m_key_matrix
					if (update)
					{
						m_key_matrix[i] = (m_key_matrix[i] & ~ (1 << j)) | (keybuf & (1 << j));
					}

					codes[report] = (i<<4)|j;

					// key up?
					if (!(keybuf & (1 << j)))
					{
						codes[report] |= 0x80;
					}

					// update modifier state
					if (update)
					{
						if (((i<<4)|j) == 0x39)
						{
							if (codes[report] & 0x80)
							{
								m_adb_modifiers &= ~0x20;
							}
							else
							{
								m_adb_modifiers |= 0x20;
							}
						}
						if (((i<<4)|j) == 0x36)
						{
							if (codes[report] & 0x80)
							{
								m_adb_modifiers &= ~0x8;
							}
							else
							{
								m_adb_modifiers |= 0x08;
							}
						}
						if (((i<<4)|j) == 0x38)
						{
							if (codes[report] & 0x80)
							{
								m_adb_modifiers &= ~0x4;
							}
							else
							{
								m_adb_modifiers |= 0x04;
							}
						}
						if (((i<<4)|j) == 0x3a)
						{
							if (codes[report] & 0x80)
							{
								m_adb_modifiers &= ~0x2;
							}
							else
							{
								m_adb_modifiers |= 0x02;
							}
						}
						if (((i<<4)|j) == 0x37)
						{
							if (codes[report] & 0x80)
							{
								m_adb_modifiers &= ~0x1;
							}
							else
							{
								m_adb_modifiers |= 0x01;
							}
						}
					}

					// we run out of keys we can track?
					report++;
					if (report == 2)
					{
						break;
					}
				}
			}

			// we run out of keys we can track?
			if (report == 2)
			{
				break;
			}
		}
	}

//  printf("ADB keyboard: update %d keys %02x %02x\n", update, codes[0], codes[1]);

	// figure out if there was a change
	if ((m_adb_currentkeys[0] != codes[0]) || (m_adb_currentkeys[1] != codes[1]))
	{
		result = 1;

		// if we want to update the current read, do so
		if (update)
		{
			m_adb_currentkeys[0] = codes[0];
			m_adb_currentkeys[1] = codes[1];
		}
	}

	return result;
}

int mac_state::adb_pollmouse()
{
	int NewX, NewY, NewButton;

	if (!m_adb_mouse_initialized)
	{
		return 0;
	}

	NewButton = input_port_read(machine(), "MOUSE0") & 0x01;
	NewX = input_port_read(machine(), "MOUSE2");
	NewY = input_port_read(machine(), "MOUSE1");

	if ((NewX != m_adb_lastmousex) || (NewY != m_adb_lastmousey) || (NewButton != m_adb_lastbutton))
	{
		return 1;
	}

	return 0;
}

void mac_state::adb_accummouse( UINT8 *MouseX, UINT8 *MouseY )
{
	int MouseCountX = 0, MouseCountY = 0;
	int NewX, NewY;

	NewX = input_port_read(machine(), "MOUSE2");
	NewY = input_port_read(machine(), "MOUSE1");

	/* see if it moved in the x coord */
	if (NewX != m_adb_lastmousex)
	{
		int diff = NewX - m_adb_lastmousex;

		/* check for wrap */
		if (diff > 0x80)
			diff = 0x100-diff;
		if  (diff < -0x80)
			diff = -0x100-diff;

		MouseCountX += diff;
		m_adb_lastmousex = NewX;
	}

	/* see if it moved in the y coord */
	if (NewY != m_adb_lastmousey)
	{
		int diff = NewY - m_adb_lastmousey;

		/* check for wrap */
		if (diff > 0x80)
			diff = 0x100-diff;
		if  (diff < -0x80)
			diff = -0x100-diff;

		MouseCountY += diff;
		m_adb_lastmousey = NewY;
	}

	m_adb_lastbutton = input_port_read(machine(), "MOUSE0") & 0x01;

	*MouseX = (UINT8)MouseCountX;
	*MouseY = (UINT8)MouseCountY;
}

void mac_state::adb_talk()
{
	int addr, reg;

	addr = (m_adb_command>>4);
	reg = (m_adb_command & 3);

//  printf("Mac sent %x (cmd %d addr %d reg %d mr %d kr %d)\n", mac->m_adb_command, (mac->m_adb_command>>2)&3, addr, reg, m_adb_mouseaddr, m_adb_keybaddr);

	if (m_adb_waiting_cmd)
	{
		switch ((m_adb_command>>2)&3)
		{
			case 0:
			case 1:
				switch (reg)
				{
					case ADB_CMD_RESET:
						#if LOG_ADB
						printf("ADB RESET: reg %x address %x\n", reg, addr);
						#endif
						m_adb_direction = 0;
						m_adb_send = 0;
						break;

					case ADB_CMD_FLUSH:
						#if LOG_ADB
						printf("ADB FLUSH: reg %x address %x\n", reg, addr);
						#endif

						m_adb_direction = 0;
						m_adb_send = 0;
						break;

					default:	// reserved/unused
						break;
				}
				break;

			case 2:	// listen
				#if LOG_ADB
				printf("ADB LISTEN: reg %x address %x\n", reg, addr);
				#endif

				m_adb_direction = 1;	// input from Mac
				m_adb_listenreg = reg;
				m_adb_listenaddr = addr;
				m_adb_command = 0;
				break;

			case 3: // talk
				#if LOG_ADB
				printf("ADB TALK: reg %x address %x\n", reg, addr);
				#endif

				// keep track of what device the Mac last TALKed to
				m_adb_last_talk = addr;

				m_adb_direction = 0;	// output to Mac
				if (addr == m_adb_mouseaddr)
				{
					UINT8 mouseX, mouseY;

					#if LOG_ADB
					printf("Talking to mouse, register %x\n", reg);
					#endif

					switch (reg)
					{
						// read mouse
						case 0:
							if (m_adb_srq_switch)
							{
								m_adb_srq_switch = 0;
								mouseX = mouseY = 0;
							}
							else
							{
								this->adb_accummouse(&mouseX, &mouseY);
							}
							m_adb_buffer[0] = (m_adb_lastbutton & 0x01) ? 0x00 : 0x80;
							m_adb_buffer[0] |= mouseX & 0x7f;
							m_adb_buffer[1] = mouseY & 0x7f;
							m_adb_datasize = 2;
							break;

						// get ID/handler
						case 3:
							m_adb_buffer[0] = 0x60 | ((m_adb_mouseaddr<<8)&0xf);	// SRQ enable, no exceptional event
							m_adb_buffer[1] = 0x01;	// handler 1
							m_adb_datasize = 2;

							m_adb_mouse_initialized = 1;
							break;

						default:
							break;
					}
				}
				else if (addr == m_adb_keybaddr)
				{
					#if LOG_ADB
					printf("Talking to keyboard, register %x\n", reg);
					#endif

					switch (reg)
					{
						// read keyboard
						case 0:
							if (m_adb_srq_switch)
							{
								m_adb_srq_switch = 0;
							}
							else
							{
								this->adb_pollkbd(1);
							}
//                          printf("keyboard = %02x %02x\n", m_adb_currentkeys[0], m_adb_currentkeys[1]);
							m_adb_buffer[0] = m_adb_currentkeys[1];
							m_adb_buffer[1] = m_adb_currentkeys[0];
							m_adb_datasize = 2;
							break;

						// read modifier keys
						case 2:
							this->adb_pollkbd(1);
							m_adb_buffer[0] = m_adb_modifiers;	// nothing pressed
							m_adb_buffer[1] = 0;
							m_adb_datasize = 2;
							break;

						// get ID/handler
						case 3:
							m_adb_buffer[0] = 0x60 | ((m_adb_keybaddr<<8)&0xf);	// SRQ enable, no exceptional event
							m_adb_buffer[1] = 0x01;	// handler 1
							m_adb_datasize = 2;

							m_adb_keybinitialized = 1;
							break;

						default:
							break;
					}
				}
				else
				{
					#if LOG_ADB
					printf("ADB: talking to unconnected device\n");
					#endif
					m_adb_buffer[0] = m_adb_buffer[1] = 0;
					m_adb_datasize = 0;
				}
				break;
		}

		m_adb_waiting_cmd = 0;
	}
	else
	{
		#if LOG_ADB
		printf("Got LISTEN data %x for device %x reg %x\n", m_adb_command, m_adb_listenaddr, m_adb_listenreg);
		#endif

		if (m_adb_listenaddr == m_adb_mouseaddr)
		{
			if ((m_adb_listenreg == 3) && (m_adb_command > 0) && (m_adb_command < 16))
			{
				#if LOG_ADB
				printf("MOUSE: moving to address %x\n", m_adb_command);
				#endif
				m_adb_mouseaddr = m_adb_command&0x0f;
			}
		}
		else if (m_adb_listenaddr == m_adb_keybaddr)
		{
			if ((m_adb_listenreg == 3) && (m_adb_command > 0) && (m_adb_command < 16))
			{
				#if LOG_ADB
				printf("KEYBOARD: moving to address %x\n", m_adb_command);
				#endif
				m_adb_keybaddr = m_adb_command&0x0f;
			}
		}
	}
}

static TIMER_CALLBACK(mac_adb_tick)
{
	mac_state *mac = machine.driver_data<mac_state>();

	// do one clock transition on CB1 to advance the VIA shifter
	mac->m_adb_extclock ^= 1;
	mac->m_via1->write_cb1(mac->m_adb_extclock);
	mac->m_adb_extclock ^= 1;
	mac->m_via1->write_cb1(mac->m_adb_extclock);

	mac->m_adb_timer_ticks--;
	if (!mac->m_adb_timer_ticks)
	{
		mac->m_adb_timer->adjust(attotime::never);

		if ((mac->m_adb_direction) && (ADB_IS_BITBANG))
		{
			mac->adb_talk();
		}
		else if (ADB_IS_EGRET)
		{
			// Egret sending a response to the 680x0?
			if (mac->m_adb_state & 1)
			{
				if (mac->m_adb_datasize > 0)
				{
					mac->m_adb_send = mac->m_adb_buffer[mac->m_adb_datasize];
					#if LOG_ADB
					printf("Egret ADB: sending byte %02x, %d left\n", mac->m_adb_send, mac->m_adb_datasize-1);
					#endif

					mac->m_adb_datasize--;
				}
				else
				{
					switch (mac->m_adb_streaming)
					{
						case MCU_STREAMING_PRAMRD:
							#if LOG_ADB
							printf("Egret ADB: streaming PRAM byte %02x = %02x\n", mac->m_adb_stream_ptr, mac->m_rtc_ram[mac->m_adb_stream_ptr]);
							#endif
							mac->m_adb_send = mac->m_rtc_ram[mac->m_adb_stream_ptr++];
							if (mac->m_adb_stream_ptr >= 256)
							{
								#if LOG_ADB
								printf("Egret ADB: this is the last PRAM byte, dropping XS\n");
								#endif
								mac->m_adb_state &= ~1;
							}
							break;

						default:
							#if LOG_ADB
							printf("Egret ADB: trying to stream unhandled type\n");
							#endif
							mac->m_adb_send = 0;
							break;
					}
				}

//              mac->m_adb_timer_ticks = 8;
//              mac->m_adb_timer->adjust(attotime(0, ATTOSECONDS_IN_USEC(100)));

				if ((mac->m_adb_datasize == 0) && (mac->m_adb_streaming == MCU_STREAMING_NONE))
				{
					#if LOG_ADB
					printf("Egret ADB: this is the last byte, dropping XS\n");
					#endif
					mac->m_adb_state &= ~1;
				}
			}
			else
			{
				// it's not a command byte if SS is low
				if (mac->m_adb_state & 0x02)
				{
					#if LOG_ADB
					printf("Egret ADB: got command byte %02x [%02x]\n", mac->m_adb_command, mac->m_adb_datasize);
					#endif
					mac->m_adb_buffer[mac->m_adb_datasize++] = mac->m_adb_command;
					mac->m_adb_command = 0;
				}
			}
		}
	}
	else
	{
		mac->m_adb_timer->adjust(attotime(0, ATTOSECONDS_IN_USEC(200)));
	}
}

static READ8_DEVICE_HANDLER(mac_adb_via_in_cb2)
{
	UINT8 ret;
	mac_state *mac = device->machine().driver_data<mac_state>();

	ret = (mac->m_adb_send & 0x80)>>7;
	mac->m_adb_send <<= 1;

//  printf("VIA IN CB2 = %x\n", ret);

	return ret;
}

static WRITE8_DEVICE_HANDLER(mac_adb_via_out_cb2)
{
	mac_state *mac = device->machine().driver_data<mac_state>();

//        printf("VIA OUT CB2 = %x\n", data);
	mac->m_adb_command <<= 1;
	mac->m_adb_command |= data & 1;
}

static void mac_adb_newaction(mac_state *mac, int state)
{
	if (state != mac->m_adb_state)
	{
		#if LOG_ADB
		printf("New ADB state: %s\n", adb_statenames[state]);
		#endif

		mac->m_adb_state = state;
		mac->m_adb_timer_ticks = 8;

		switch (state)
		{
			case ADB_STATE_NEW_COMMAND:
				mac->m_adb_command = mac->m_adb_send = 0;
				mac->m_adb_direction = 1;	// Mac is shifting us a command
				mac->m_adb_waiting_cmd = 1;	// we're going to get a command
				mac->m_adb_irq_pending = 0;
				mac->m_adb_timer->adjust(attotime(0, ATTOSECONDS_IN_USEC(100)));
				break;

			case ADB_STATE_XFER_EVEN:
			case ADB_STATE_XFER_ODD:
				if (mac->m_adb_datasize > 0)
				{
					int i;

					// is something trying to send to the Mac?
					if (mac->m_adb_direction == 0)
					{
						// set up the byte
						mac->m_adb_send = mac->m_adb_buffer[0];
						mac->m_adb_datasize--;

						// move down the rest of the buffer, if any
						for (i = 0; i < mac->m_adb_datasize; i++)
						{
							mac->m_adb_buffer[i] = mac->m_adb_buffer[i+1];
						}
					}

				}
				else
				{
					mac->m_adb_send = 0;
					mac->m_adb_irq_pending = 1;
				}

				mac->m_adb_timer->adjust(attotime(0, ATTOSECONDS_IN_USEC(100)));
				break;

			case ADB_STATE_IDLE:
				mac->m_adb_irq_pending = 0;
				break;
		}
	}
}

static void mac_egret_response_std(mac_state *mac, int type, int flag, int cmd)
{
	mac->m_adb_send = 0xaa;
	mac->m_adb_buffer[3] = type;
	mac->m_adb_buffer[2] = flag;
	mac->m_adb_buffer[1] = cmd;
	mac->m_adb_state |= 1;
	mac->m_adb_timer_ticks = 8;
	mac->m_adb_datasize = 3;
	mac->m_adb_timer->adjust(attotime(0, ATTOSECONDS_IN_USEC(100)));
}

static void mac_egret_response_adb(mac_state *mac, int type, int flag, int cmd, int extra)
{
	mac->m_adb_send = 0xaa;
	mac->m_adb_buffer[4] = extra;
	mac->m_adb_buffer[3] = type;
	mac->m_adb_buffer[2] = flag;
	mac->m_adb_buffer[1] = cmd;
	mac->m_adb_state |= 1;
	mac->m_adb_timer_ticks = 8;
	mac->m_adb_datasize = 4;
	mac->m_adb_timer->adjust(attotime(0, ATTOSECONDS_IN_USEC(100)));
}

static void mac_egret_response_read_pram(mac_state *mac, int cmd, int addr)
{
	mac->m_adb_datasize = 4;

	mac->m_adb_send = 0xaa;
	mac->m_adb_buffer[4] = 1;	// type
	mac->m_adb_buffer[3] = 0;	// flag
	mac->m_adb_buffer[2] = cmd; // command
	mac->m_adb_buffer[1] = mac->m_rtc_ram[addr];

	mac->m_adb_state |= 1;
	mac->m_adb_timer_ticks = 8;
	mac->m_adb_timer->adjust(attotime(0, ATTOSECONDS_IN_USEC(100)));

	// read PRAM is a "streaming" command, don't drop the state line when we're out of data
	mac->m_adb_streaming = MCU_STREAMING_PRAMRD;
	mac->m_adb_stream_ptr = addr+1;
}

static void mac_egret_response_read_rtc(mac_state *mac)
{
	mac->m_adb_datasize = 7;

	mac->m_adb_send = 0xaa;
	mac->m_adb_buffer[7] = 1;	// type
	mac->m_adb_buffer[6] = 0;	// flag
	mac->m_adb_buffer[5] = 7;	// command
	mac->m_adb_buffer[4] = mac->m_rtc_seconds[3];
	mac->m_adb_buffer[3] = mac->m_rtc_seconds[2];
	mac->m_adb_buffer[2] = mac->m_rtc_seconds[1];
	mac->m_adb_buffer[1] = mac->m_rtc_seconds[0];

	mac->m_adb_state |= 1;
	mac->m_adb_timer_ticks = 8;
	mac->m_adb_timer->adjust(attotime(0, ATTOSECONDS_IN_USEC(100)));
}

static void mac_egret_mcu_exec(mac_state *mac)
{
	mac->m_adb_streaming = MCU_STREAMING_NONE;
	switch (mac->m_adb_buffer[1])
	{
		case 0x01:	// enable/disable ADB auto-polling
			#if LOG_ADB || LOG_ADB_MCU_CMD
			if (mac->m_adb_buffer[2])
			{
				printf("ADB: Egret enable ADB auto-poll\n");
			}
			else
			{
				printf("ADB: Egret disable ADB auto-poll\n");
			}
			#endif

			mac_egret_response_std(mac, 1, 0, 1);
			break;

		case 0x02: // read 6805 address
			{
				int addr = mac->m_adb_buffer[2]<<8 | mac->m_adb_buffer[3];

				#if LOG_ADB || LOG_ADB_MCU_CMD
				printf("ADB: Egret read 6805 address %x\n", addr);
				#endif

				// check if this is a sneaky PRAM read (PRAM is at 6805 address 0x100)
				if ((addr >= 0x100) && (addr <= 0x200))
				{
					mac_egret_response_read_pram(mac, 2, addr&0xff);
				}
				#if LOG_ADB
				else
				{
					printf("ADB: Egret unhandled direct read @ %x\n", addr);
				}
				#endif
			}
			break;

		case 0x03: // read RTC
			#if LOG_ADB || LOG_ADB_MCU_CMD
			printf("ADB: Egret read RTC = %08x\n", mac->m_rtc_seconds[3]<<24|mac->m_rtc_seconds[2]<<16|mac->m_rtc_seconds[1]<<8|mac->m_rtc_seconds[0]);
			#endif

			mac_egret_response_read_rtc(mac);
			break;

		case 0x07: // read PRAM
			#if LOG_ADB || LOG_ADB_MCU_CMD
			printf("ADB: Egret read PRAM from %x\n", mac->m_adb_buffer[2]<<8 | mac->m_adb_buffer[3]);
			#endif

			mac_egret_response_read_pram(mac, 7, mac->m_adb_buffer[2]<<8 | mac->m_adb_buffer[3]);
			break;

		case 0x08: // write 6805 address
			{
				int addr = mac->m_adb_buffer[2]<<8 | mac->m_adb_buffer[3];
				int len = mac->m_adb_datasize - 4;

				#if LOG_ADB || LOG_ADB_MCU_CMD
				printf("ADB: Egret write %d bytes to address %x\n", len, addr);
				#endif

				// check if this is a sneaky PRAM write (PRAM is at 6805 address 0x100)
				if ((addr >= 0x100) && (addr <= 0x200))
				{
					for (int i = 0; i < len; i++)
					{
						mac->m_rtc_ram[(addr-0x100)+i] = mac->m_adb_buffer[4+i];
					}
				}
				#if LOG_ADB || LOG_ADB_MCU_CMD
				else
				{
					printf("ADB: Egret unhandled direct write @ %x\n", addr);
				}
				#endif

				mac_egret_response_std(mac, 1, 0, 0x08);
			}
			break;

		case 0x0c: // write PRAM
			#if LOG_ADB || LOG_ADB_MCU_CMD
			printf("ADB: Egret write %02x to PRAM at %x\n", mac->m_adb_buffer[4], mac->m_adb_buffer[2]<<8 | mac->m_adb_buffer[3]);
			#endif

			mac->m_rtc_ram[mac->m_adb_buffer[2]<<8 | mac->m_adb_buffer[3]] = mac->m_adb_buffer[4];

			mac->m_adb_datasize = 4;
			mac->m_adb_buffer[4] = 1;	// type
			mac->m_adb_buffer[3] = 0;	// flag
			mac->m_adb_buffer[2] = 0x0c;	// command
			mac->m_adb_buffer[1] = 0;	// spare
			mac->m_adb_state |= 1;
			mac->m_adb_timer_ticks = 8;
			mac->m_adb_timer->adjust(attotime(0, ATTOSECONDS_IN_USEC(100)));
			break;

		case 0x0e: // send to DFAC
			#if LOG_ADB || LOG_ADB_MCU_CMD
			printf("ADB: Egret send %02x to DFAC\n", mac->m_adb_buffer[2]);
			#endif

			mac_egret_response_std(mac, 1, 0, 0x0e);
			break;

		case 0x19: // set device list bitmap
			#if LOG_ADB || LOG_ADB_MCU_CMD
			printf("ADB: Egret set device list bitmap %02x%02x\n", mac->m_adb_buffer[2], mac->m_adb_buffer[3]);
			#endif

			mac_egret_response_std(mac, 1, 0, 0x19);
			break;

		case 0x1b: // set one-second interrupt
			#if LOG_ADB || LOG_ADB_MCU_CMD
			if (mac->m_adb_buffer[2])
			{
				printf("ADB: Egret enable one-second IRQ\n");
			}
			else
			{
				printf("ADB: Egret disable one-second IRQ\n");
			}
			#endif

			mac_egret_response_std(mac, 1, 0, 0x1b);
			break;

		case 0x1c:	// enable/disable keyboard NMI
			#if LOG_ADB || LOG_ADB_MCU_CMD
			if (mac->m_adb_buffer[2])
			{
				printf("ADB: Egret enable keyboard NMI\n");
			}
			else
			{
				printf("ADB: Egret disable keyboard NMI\n");
			}
			#endif

			mac_egret_response_std(mac, 1, 0, 0x1c);
			break;

		default:
			#if LOG_ADB || LOG_ADB_MCU_CMD
			printf("ADB: Unknown Egret MCU command %02x\n", mac->m_adb_buffer[1]);
			#endif
			break;
	}
}

static void mac_egret_newaction(mac_state *mac, int state)
{
	if (state != mac->m_adb_state)
	{
		#if LOG_ADB
		printf("ADB: New Egret state: SS %d VF %d XS (68k %d MCU %d)\n", (state>>2)&1, (state>>1)&1, state&1, mac->m_adb_state&1);
		#endif

		// if bit 2 is high and stays high, the rising edge of bit 1 indicates the start of sending a byte to the MCU if XS isn't high
		if ((state & 0x04) && (mac->m_adb_state & 0x04) && (state & 0x02) && !(mac->m_adb_state & 0x02) && !(mac->m_adb_state & 0x01))
		{
			mac->m_adb_command = 0;
			mac->m_adb_timer_ticks = 8;
			mac->m_adb_timer->adjust(attotime(0, ATTOSECONDS_IN_USEC(100)));
		}

		// if bit 2 is high and stays high, the falling edge of bit 1, and we're in send phase, the MCU should clock out a byte
		if ((state & 0x04) && (mac->m_adb_state & 0x04) && !(state & 0x02) && (mac->m_adb_state & 0x02) && (mac->m_adb_state & 0x01))
		{
			mac->m_adb_timer_ticks = 8;
			mac->m_adb_timer->adjust(attotime(0, ATTOSECONDS_IN_USEC(100)));
		}

		// if bit 2 rises, bit 1 is 0, and MCU XS is high, the MCU should clock out a byte
		if ((state & 0x04) && !(mac->m_adb_state & 0x04) && !(state & 0x02) && (mac->m_adb_state & 0x01))
		{
			mac->m_adb_timer_ticks = 8;
			mac->m_adb_timer->adjust(attotime(0, ATTOSECONDS_IN_USEC(100)));
		}

		// if bit 2 drops and bit 1 is 1, terminate the command
		if ((state & 0x02) && !(state & 0x04) && (mac->m_adb_state & 0x04))
		{
			#if LOG_ADB
			printf("Egret ADB: SESSION dropped with VF=1, terminating command\n");
			#endif
			mac->m_adb_streaming = MCU_STREAMING_NONE;
			mac->m_adb_state &= ~1;
			mac->m_adb_datasize = 0;
		}

		// if bit 2 drops and bit 1 is zero, execute the command
		if (!(state & 0x02) && !(state & 0x04) && (mac->m_adb_state & 0x04) && (mac->m_adb_datasize > 0))
		{
			#if LOG_ADB || LOG_ADB_MCU_CMD
			int i;

			printf("Egret ADB: exec command with %d bytes: ", mac->m_adb_datasize);

			for (i = 0; i < mac->m_adb_datasize; i++)
			{
				printf("%02x ", mac->m_adb_buffer[i]);
			}

			printf("\n");
			#endif

			// exec command here
			switch (mac->m_adb_buffer[0])
			{
				case 0:	// ADB command
					switch (mac->m_adb_buffer[1] & 0xf)
					{
						case 0:		// reset
							#if LOG_ADB || LOG_ADB_MCU_CMD
							printf("Egret: ADB Reset\n");
							#endif
							mac_egret_response_adb(mac, 0, 0, 0, 0);
							break;

						case 1:		// flush
							#if LOG_ADB || LOG_ADB_MCU_CMD
							printf("Egret: ADB Flush\n");
							#endif
							mac_egret_response_std(mac, 1, 1, 0);
							break;

						default:
							switch ((mac->m_adb_buffer[1]>>2)&3)
							{
								case 2:	// ADB listen
									#if LOG_ADB || LOG_ADB_MCU_CMD
									printf("Egret: ADB listen to device %d\n", mac->m_adb_buffer[1]>>4);
									#endif
									mac_egret_response_adb(mac, 0, 2, 0, 0);
									break;

								case 3: // ADB talk
									#if LOG_ADB || LOG_ADB_MCU_CMD
									printf("Egret: ADB talk to device %d\n", mac->m_adb_buffer[1]>>4);
									#endif
									mac_egret_response_adb(mac, 0, 2, 0, 0);
									break;
							}

							#if LOG_ADB || LOG_ADB_MCU_CMD
							printf("Egret: Unhandled ADB command %02x\n", mac->m_adb_buffer[1]);
							#endif
							mac->m_adb_datasize = 0;
							break;

					}
					break;

				case 1: // general Egret MCU command
					mac_egret_mcu_exec(mac);
					break;

				case 2: // error
					mac->m_adb_datasize = 0;
					break;

				case 3: // one-second interrupt
					mac->m_adb_datasize = 0;
					break;
			}
		}

		mac->m_adb_state &= ~0x6;
		mac->m_adb_state |= state & 0x6;
	}
}

static TIMER_CALLBACK(mac_pmu_tick)
{
	mac_state *mac = machine.driver_data<mac_state>();

	#if LOG_ADB
	printf("PM: timer tick, lowering ACK\n");
	#endif
	mac->m_pm_ack &= ~2;	// lower ACK to handshake next step
}

static void pmu_one_byte_reply(mac_state *mac, UINT8 result)
{
	mac->m_pm_out[0] = 1;	// length
	mac->m_pm_out[1] = result;
	mac->m_pm_slen = 2;
	mac->m_pmu_send_timer->adjust(attotime(0, ATTOSECONDS_IN_USEC(200)));
}

static void pmu_exec(mac_state *mac)
{
	mac->m_pm_sptr = 0;	// clear send pointer
	mac->m_pm_slen = 0;	// and send length
	mac->m_pm_dptr = 0;	// and recieve pointer

	switch (mac->m_pm_cmd[0])
	{
		case 0x10:	// subsystem power and clock ctrl (no response)
			break;

		case 0x32:	// write extended PRAM byte(s).  cmd[2] = address, cmd[3] = length, cmd[4...] = data
			if ((mac->m_pm_cmd[2] + mac->m_pm_cmd[3]) < 0x100)
			{
				int i;

				for (i = 0; i < mac->m_pm_cmd[3]; i++)
				{
					mac->m_rtc_ram[mac->m_pm_cmd[2] + i] = mac->m_pm_cmd[4+i];
				}
			}
			break;

		case 0x39:	// read first 20 bytes of PRAM
			{
				int i;

				mac->m_pm_out[0] = 20;
				for (i = 0; i < 20; i++)
				{
					mac->m_pm_out[1 + i] = mac->m_rtc_ram[i];
				}
				mac->m_pm_slen = 21;
				mac->m_pmu_send_timer->adjust(attotime(0, ATTOSECONDS_IN_USEC(200)));
			}
			break;

		case 0x3a:	// read extended PRAM byte(s).  cmd[2] = address, cmd[3] = length
			if ((mac->m_pm_cmd[2] + mac->m_pm_cmd[3]) < 0x100)
			{
				int i;

				mac->m_pm_out[0] = mac->m_pm_cmd[3];
				for (i = 0; i < mac->m_pm_cmd[3]; i++)
				{
					mac->m_pm_out[1 + i] = mac->m_rtc_ram[mac->m_pm_cmd[2] + i];
				}
				mac->m_pm_slen = mac->m_pm_cmd[3] + 1;
				mac->m_pmu_send_timer->adjust(attotime(0, ATTOSECONDS_IN_USEC(200)));
			}
			break;

		case 0x68:	// read battery/charger level
			pmu_one_byte_reply(mac, 1);
			break;

		case 0x78:	// read interrupt flag
			pmu_one_byte_reply(mac, 0);
			break;

		case 0xec:	// PMU self-test (send 1 count byte + reply)
			pmu_one_byte_reply(mac, 0);
			break;

		default:
			fatalerror("PMU: Unhandled command %02x\n", mac->m_pm_cmd[0]);
			break;
	}

	if (mac->m_pm_slen > 0)
	{
		mac->m_pm_state = 1;
	}
}

void mac_state::adb_vblank()
{
	if (m_adb_state == ADB_STATE_IDLE)
	{
		if (this->adb_pollmouse())
		{
			// if the mouse was the last TALK, we can just send the new data
			// otherwise we need to pull SRQ
			if (m_adb_last_talk == m_adb_mouseaddr)
			{
				// repeat last TALK to get updated data
				m_adb_waiting_cmd = 1;
				this->adb_talk();

				m_adb_timer_ticks = 8;
				this->m_adb_timer->adjust(attotime(0, ATTOSECONDS_IN_USEC(100)));
			}
			else
			{
				m_adb_irq_pending = 1;
				m_adb_command = m_adb_send = 0;
				m_adb_timer_ticks = 1;	// one tick should be sufficient to make it see  the IRQ
				this->m_adb_timer->adjust(attotime(0, ATTOSECONDS_IN_USEC(100)));
				m_adb_srq_switch = 1;
			}
		}
		else if (this->adb_pollkbd(0))
		{
			if (m_adb_last_talk == m_adb_keybaddr)
			{
				// repeat last TALK to get updated data
				m_adb_waiting_cmd = 1;
				this->adb_talk();

				m_adb_timer_ticks = 8;
				this->m_adb_timer->adjust(attotime(0, ATTOSECONDS_IN_USEC(100)));
			}
			else
			{
				m_adb_irq_pending = 1;
				m_adb_command = m_adb_send = 0;
				m_adb_timer_ticks = 1;	// one tick should be sufficient to make it see  the IRQ
				this->m_adb_timer->adjust(attotime(0, ATTOSECONDS_IN_USEC(100)));
				m_adb_srq_switch = 1;
			}
		}
	}
}

void mac_state::adb_reset()
{
	int i;

	m_adb_srq_switch = 0;
	m_adb_irq_pending = 0;		// no interrupt
	m_adb_timer_ticks = 0;
	m_adb_command = 0;
	m_adb_extclock = 0;
	m_adb_send = 0;
	m_adb_waiting_cmd = 0;
	m_adb_streaming = MCU_STREAMING_NONE;
	m_adb_state = 0;
	if (ADB_IS_BITBANG_CLASS)
	{
		m_adb_state = ADB_STATE_NOTINIT;
	}
	m_adb_direction = 0;
	m_adb_datasize = 0;
	m_adb_last_talk = -1;

	// mouse
	m_adb_mouseaddr = 3;
	m_adb_lastmousex = m_adb_lastmousey = m_adb_lastbutton = 0;
	m_adb_mouse_initialized = 0;

	// keyboard
	m_adb_keybaddr = 2;
	m_adb_keybinitialized = 0;
	m_adb_currentkeys[0] = m_adb_currentkeys[1] = 0x80;
	m_adb_modifiers = 0;
	for (i=0; i<7; i++)
	{
		m_key_matrix[i] = 0;
	}
}

/* *************************************************************************
 * VIA
 * *************************************************************************
 *
 *
 * PORT A
 *
 *  bit 7               R   SCC Wait/Request
 *  bit 6               W   Main/Alternate screen buffer select
 *  bit 5               W   Floppy Disk Line Selection
 *  bit 4               W   Overlay/Normal memory mapping
 *  bit 3               W   Main/Alternate sound buffer
 *  bit 2-0             W   Sound Volume
 *
 *
 * PORT B
 *
 *  bit 7               W   Sound enable
 *  bit 6               R   Video beam in display
 *  bit 5   (pre-ADB)   R   Mouse Y2
 *          (ADB)       W   ADB ST1
 *  bit 4   (pre-ADB)   R   Mouse X2
 *          (ADB)       W   ADB ST0
 *  bit 3   (pre-ADB)   R   Mouse button (active low)
 *          (ADB)       R   ADB INT
 *  bit 2               W   Real time clock enable
 *  bit 1               W   Real time clock data clock
 *  bit 0               RW  Real time clock data
 *
 */

#define PA6 0x40
#define PA4 0x10
#define PA2 0x04
#define PA1 0x02

static READ8_DEVICE_HANDLER(mac_via_in_a)
{
	mac_state *mac = device->machine().driver_data<mac_state>();

//  printf("VIA1 IN_A (PC %x)\n", cpu_get_pc(device->machine().device("maincpu")));

	switch (mac->m_model)
	{
		case MODEL_MAC_PORTABLE:
		case MODEL_MAC_PB100:
			#if LOG_ADB
//          printf("Read PM data %x\n", mac->m_pm_data_recv);
			#endif
			return mac->m_pm_data_recv;

		case MODEL_MAC_CLASSIC:
		case MODEL_MAC_II:
		case MODEL_MAC_II_FDHD:
		case MODEL_MAC_IIX:
		case MODEL_MAC_POWERMAC_6100:
		case MODEL_MAC_POWERMAC_7100:
		case MODEL_MAC_POWERMAC_8100:
			return 0x81;		// bit 0 must be set on most Macs to avoid attempting to boot from AppleTalk

		case MODEL_MAC_SE30:
			return 0x81 | PA6;

		case MODEL_MAC_LC:		// apollo board is 0x38, box 0x11 (17), vid hw 0x21
		case MODEL_MAC_LC_II:
			return 0x81 | PA6 | PA4 | PA2;

		case MODEL_MAC_IICI:
			return 0x81 | PA6 | PA2 | PA1;

		case MODEL_MAC_IISI:
			return 0x81 | PA4 | PA2 | PA1;

		case MODEL_MAC_IIFX:
			return 0x81 | PA6 | PA1;

		case MODEL_MAC_IICX:
			return 0x81 | PA6;

		case MODEL_MAC_PB170:		// since the decoders are different, these are allowed to "collide"
		case MODEL_MAC_CLASSIC_II:
			return 0x81 | PA4 | PA1;

		case MODEL_MAC_QUADRA_700:
			return 0x81 | PA6;

		case MODEL_MAC_QUADRA_900:
			return 0x81 | PA6 | PA4;

		case MODEL_MAC_COLOR_CLASSIC:
			return 0x81 | PA1;

		default:
			return 0x80;

	}

	return 0x80;
}

static READ8_DEVICE_HANDLER(mac_via_in_b)
{
	int val = 0;
	mac_state *mac = device->machine().driver_data<mac_state>();

	// portable/PB100 is pretty different
	if (mac->m_model >= MODEL_MAC_PORTABLE && mac->m_model <= MODEL_MAC_PB100)
	{
//      printf("Read VIA B: PM_ACK %x\n", mac->m_pm_ack);
		val = 0x80 | 0x04 | mac->m_pm_ack;	// SCC wait/request (bit 2 must be set at 900c1a or startup tests always fail)
	}
	else
	{
		/* video beam in display (! VBLANK && ! HBLANK basically) */
		if (device->machine().primary_screen->vpos() >= MAC_V_VIS)
			val |= 0x40;

		if (ADB_IS_BITBANG)
		{
			val |= mac->m_adb_state<<4;

			if (!mac->m_adb_irq_pending)
			{
				val |= 0x08;
			}
		}
		else if (ADB_IS_EGRET)
		{
			val |= mac->m_adb_state<<3;
			val ^= 0x08;	// maybe it's reversed?
		}
		else
		{
			if (mac->m_mouse_bit_y)	/* Mouse Y2 */
				val |= 0x20;
			if (mac->m_mouse_bit_x)	/* Mouse X2 */
				val |= 0x10;
			if ((input_port_read(device->machine(), "MOUSE0") & 0x01) == 0)
				val |= 0x08;
		}
		if (mac->m_rtc_data_out)
			val |= 1;
	}

//  printf("VIA1 IN_B = %02x (PC %x)\n", val, cpu_get_pc(device->machine().device("maincpu")));

	return val;
}

static WRITE8_DEVICE_HANDLER(mac_via_out_a)
{
	device_t *sound = device->machine().device("custom");
	device_t *fdc = device->machine().device("fdc");
	mac_state *mac = device->machine().driver_data<mac_state>();

//  printf("VIA1 OUT A: %02x (PC %x)\n", data, cpu_get_pc(device->machine().device("maincpu")));

	if (mac->m_model >= MODEL_MAC_PORTABLE && mac->m_model <= MODEL_MAC_PB100)
	{
		#if LOG_ADB
//      printf("%02x to PM\n", data);
		#endif
		mac->m_pm_data_send = data;
		return;
	}

	set_scc_waitrequest((data & 0x80) >> 7);
	mac->m_screen_buffer = (data & 0x40) >> 6;
	sony_set_sel_line(fdc,(data & 0x20) >> 5);
	if (mac->m_model == MODEL_MAC_SE)	// on SE this selects which floppy drive (0 = upper, 1 = lower)
	{
		mac->m_drive_select = ((data & 0x10) >> 4);
	}

	if (mac->m_model < MODEL_MAC_SE)	// SE no longer has dual buffers
	{
		mac_set_sound_buffer(sound, (data & 0x08) >> 3);
	}

	if (mac->m_model < MODEL_MAC_II)
	{
		mac_set_volume(sound, data & 0x07);
	}

	/* Early Mac models had VIA A4 control overlaying.  In the Mac SE (and
     * possibly later models), overlay was set on reset, but cleared on the
     * first access to the ROM. */
	if (mac->m_model < MODEL_MAC_SE)
	{
		mac->set_memory_overlay((data & 0x10) >> 4);
	}
}

static WRITE8_DEVICE_HANDLER(mac_via_out_b)
{
	device_t *sound = device->machine().device("custom");
	int new_rtc_rTCClk;
	mac_state *mac = device->machine().driver_data<mac_state>();

//  printf("VIA1 OUT B: %02x (PC %x)\n", data, cpu_get_pc(device->machine().device("maincpu")));

	if (mac->m_model >= MODEL_MAC_PORTABLE && mac->m_model <= MODEL_MAC_PB100)
	{
		device_t *fdc = device->machine().device("fdc");

		sony_set_sel_line(fdc,(data & 0x20) >> 5);
		mac->m_drive_select = ((data & 0x10) >> 4);

		if ((data & 1) && !(mac->m_pm_req & 1))
		{
			#if LOG_ADB
			printf("PM: 68k dropping /REQ\n");
			#endif

			if (mac->m_pm_state == 0)	 // do this in receive state only
			{
				mac->m_pm_data_recv = 0xff;
				mac->m_pm_ack |= 2;

				// check if length byte matches
				if ((mac->m_pm_dptr >= 2) && (mac->m_pm_cmd[1] == (mac->m_pm_dptr-2)))
				{
					pmu_exec(mac);
					#if LOG_ADB
					printf("PMU exec: command %02x length %d\n", mac->m_pm_cmd[0], mac->m_pm_cmd[1]);
					#endif
				}
			}
		}
		else if (!(data & 1) && (mac->m_pm_req & 1))
		{
			if (mac->m_pm_state == 0)
			{
				#if LOG_ADB
				printf("PM: 68k asserting /REQ, clocking in byte [%d] = %02x\n", mac->m_pm_dptr, mac->m_pm_data_send);
				#endif
				mac->m_pm_ack &= ~2;	// clear, we're waiting for more bytes
				mac->m_pm_cmd[mac->m_pm_dptr++] = mac->m_pm_data_send;
			}
			else	// receiving, so this is different
			{
				mac->m_pm_data_recv = mac->m_pm_out[mac->m_pm_sptr++];
				mac->m_pm_slen--;
				mac->m_pm_ack |= 2;	// raise ACK to indicate available byte
				#if LOG_ADB
				printf("PM: 68k asserted /REQ, sending byte %02x\n", mac->m_pm_data_recv);
				#endif

				// another byte to send?
				if (mac->m_pm_slen)
				{
					mac->m_pmu_send_timer->adjust(attotime(0, ATTOSECONDS_IN_USEC(100)));
				}
				else
				{
					mac->m_pm_state = 0;	// back to receive state
					mac->m_pmu_send_timer->adjust(attotime::never);
				}
			}
		}

		mac->m_pm_req = data & 1;
		return;
	}

	if (AUDIO_IS_CLASSIC)
	{
		mac_enable_sound(sound, (data & 0x80) == 0);
	}

	// SE and Classic have SCSI enable/disable here
	if ((mac->m_model == MODEL_MAC_SE) || (mac->m_model == MODEL_MAC_CLASSIC))
	{
		mac->m_scsiirq_enable = (data & 0x40) ? 0 : 1;
//      printf("VIAB & 0x40 = %02x, IRQ enable %d\n", data & 0x40, mac->m_scsiirq_enable);
	}

	if (mac->m_model == MODEL_MAC_SE30)
	{
		// 0x40 = 0 means enable vblank on SE/30
		mac->m_se30_vbl_enable = (data & 0x40) ? 0 : 1;

		// clear the interrupt if we disabled it
		if (!mac->m_se30_vbl_enable)
		{
			mac->nubus_slot_interrupt(0xe, 0);
		}
	}

	mac->rtc_write_rTCEnb(data & 0x04);
	new_rtc_rTCClk = (data >> 1) & 0x01;
	if ((! new_rtc_rTCClk) && (mac->m_rtc_rTCClk))
		mac->rtc_shift_data(data & 0x01);
	mac->m_rtc_rTCClk = new_rtc_rTCClk;

	if (ADB_IS_BITBANG)
	{
		mac_adb_newaction(mac, (data & 0x30) >> 4);
	}
	else if (ADB_IS_EGRET)
	{
		mac_egret_newaction(mac, (data & 0x30) >> 3);
	}
}

static void mac_via_irq(device_t *device, int state)
{
	mac_state *mac = device->machine().driver_data<mac_state>();

	/* interrupt the 68k (level 1) */
	mac->set_via_interrupt(state);
}

READ16_MEMBER ( mac_state::mac_via_r )
{
	UINT16 data;

	offset >>= 8;
	offset &= 0x0f;

	if (LOG_VIA)
		logerror("mac_via_r: offset=0x%02x\n", offset);
	data = m_via1->read(space, offset);

	return (data & 0xff) | (data << 8);
}

WRITE16_MEMBER ( mac_state::mac_via_w )
{
	offset >>= 8;
	offset &= 0x0f;

	if (LOG_VIA)
		logerror("mac_via_w: offset=0x%02x data=0x%08x\n", offset, data);

	if (ACCESSING_BITS_8_15)
		m_via1->write(space, offset, (data >> 8) & 0xff);
}

/* *************************************************************************
 * VIA 2 (on Mac IIs and PowerMacs)
 * *************************************************************************/

static void mac_via2_irq(device_t *device, int state)
{
	mac_state *mac = device->machine().driver_data<mac_state>();
	mac->set_via2_interrupt(state);
}

READ16_MEMBER ( mac_state::mac_via2_r )
{
	int data;

	offset >>= 8;
	offset &= 0x0f;

	data = m_via2->read(space, offset);

//  if (LOG_VIA)
		logerror("mac_via2_r: offset=0x%02x = %02x (PC=%x)\n", offset*2, data, cpu_get_pc(space.machine().device("maincpu")));

	return (data & 0xff) | (data << 8);
}

WRITE16_MEMBER ( mac_state::mac_via2_w )
{
	offset >>= 8;
	offset &= 0x0f;

//  if (LOG_VIA)
		logerror("mac_via2_w: offset=%x data=0x%08x (PC=%x)\n", offset, data, cpu_get_pc(space.machine().device("maincpu")));

	if (ACCESSING_BITS_8_15)
		m_via2->write(space, offset, (data >> 8) & 0xff);
}


static READ8_DEVICE_HANDLER(mac_via2_in_a)
{
	UINT8 result = 0xc0;
	mac_state *mac = device->machine().driver_data<mac_state>();

	result |= mac->m_nubus_irq_state;

	return result;
}

static READ8_DEVICE_HANDLER(mac_via2_in_b)
{
	mac_state *mac =device->machine().driver_data<mac_state>();

//  logerror("VIA2 IN B (PC %x)\n", cpu_get_pc(device->machine().device("maincpu")));

	if ((mac->m_model == MODEL_MAC_LC) || (mac->m_model == MODEL_MAC_LC_II) || (mac->m_model == MODEL_MAC_CLASSIC_II))
	{
		return 0x4f;
	}

	if (mac->m_model == MODEL_MAC_SE30)
	{
		return 0x87;	// bits 3 and 6 are tied low on SE/30
	}

	return 0xcf;		// indicate no NuBus transaction error
}

static WRITE8_DEVICE_HANDLER(mac_via2_out_a)
{
//  logerror("VIA2 OUT A: %02x (PC %x)\n", data, cpu_get_pc(device->machine().device("maincpu")));
}

static WRITE8_DEVICE_HANDLER(mac_via2_out_b)
{
	mac_state *mac = device->machine().driver_data<mac_state>();

//  logerror("VIA2 OUT B: %02x (PC %x)\n", data, cpu_get_pc(device->machine().device("maincpu")));

	if (mac->m_model == MODEL_MAC_II)
	{
		m68k_set_hmmu_enable(mac->m_maincpu, (data & 0x8) ? M68K_HMMU_DISABLE : M68K_HMMU_ENABLE_II);
	}

	// chain 60.15 Hz to VIA1
	mac->m_via1->write_ca1(data>>7);
}

// This signal is generated internally on RBV, V8, Sonora, VASP, Eagle, etc.
static TIMER_CALLBACK(mac_6015_tick)
{
	mac_state *mac = machine.driver_data<mac_state>();

	mac->m_via1->write_ca1(0);
	mac->m_via1->write_ca1(1);
}

/* *************************************************************************
 * Main
 * *************************************************************************/

void mac_state::machine_start()
{
	if (has_adb())
	{
		this->m_adb_timer = machine().scheduler().timer_alloc(FUNC(mac_adb_tick));
		this->m_adb_timer->adjust(attotime::never);

		// also allocate PMU timer
		if (ADB_IS_PM_CLASS)
		{
			m_pmu_send_timer = machine().scheduler().timer_alloc(FUNC(mac_pmu_tick));
			this->m_adb_timer->adjust(attotime::never);
		}

	}
	this->m_scanline_timer = machine().scheduler().timer_alloc(FUNC(mac_scanline_tick));
	this->m_scanline_timer->adjust(machine().primary_screen->time_until_pos(0, 0));

	m_6015_timer = machine().scheduler().timer_alloc(FUNC(mac_6015_tick));
	m_6015_timer->adjust(attotime::never);
}

void mac_state::machine_reset()
{
	// stop 60.15 Hz timer
	m_6015_timer->adjust(attotime::never);

	// start 60.15 Hz timer for most systems
	if (((m_model >= MODEL_MAC_IICI) && (m_model <= MODEL_MAC_IIVI)) || (m_model >= MODEL_MAC_LC))
	{
		m_6015_timer->adjust(attotime::from_hz(60.15), 0, attotime::from_hz(60.15));
	}

	// clear PMU response timer
	if (ADB_IS_PM_CLASS)
	{
		this->m_adb_timer->adjust(attotime::never);
	}

	// default to 32-bit mode on LC
	if (m_model == MODEL_MAC_LC)
	{
		m68k_set_hmmu_enable(m_maincpu, M68K_HMMU_DISABLE);
	}

	m_last_taken_interrupt = -1;

	/* initialize real-time clock */
	rtc_init();

	/* setup the memory overlay */
	if (m_model < MODEL_MAC_POWERMAC_6100)	// no overlay for PowerPC
	{
		m_overlay = -1;	// insure no match
		this->set_memory_overlay(1);
	}

	/* setup videoram */
	this->m_screen_buffer = 1;

	/* setup sound */
	if (AUDIO_IS_CLASSIC_CLASS)
	{
		mac_set_sound_buffer(machine().device("custom"), 0);
	}
	else if (MAC_HAS_VIA2)	// prime CB1 for ASC and slot interrupts
	{
		m_via2->write_ca1(1);
		m_via2->write_cb1(1);
	}

	if (has_adb())
	{
		this->adb_reset();
	}

	if ((m_model == MODEL_MAC_SE) || (m_model == MODEL_MAC_CLASSIC))
	{
		mac_set_sound_buffer(machine().device("custom"), 1);

		// classic will fail RAM test and try to boot appletalk if RAM is not all zero
		memset(ram_get_ptr(m_ram), 0, ram_get_size(m_ram));
	}

	m_scsi_interrupt = 0;
	if (machine().device<cpu_device>("maincpu")->debug())
	{
		machine().device<cpu_device>("maincpu")->debug()->set_dasm_override(mac_dasm_override);
	}

	m_drive_select = 0;
	m_scsiirq_enable = 0;
	m_via2_vbl = 0;
	m_se30_vbl_enable = 0;
	m_nubus_irq_state = 0xff;
	m_keyboard_reply = 0;
	m_kbd_comm = 0;
	m_kbd_receive = 0;
	m_kbd_shift_reg = 0;
	m_kbd_shift_count = 0;
	m_mouse_bit_x = m_mouse_bit_y = 0;
	m_rtc_rTCEnb = 0;
	m_rtc_rTCClk = 0;
	m_rtc_bit_count = 0;
	m_rtc_data_dir = 0;
	m_rtc_data_out = 0;
	m_rtc_cmd = 0;
	m_rtc_write_protect = 0;
	m_rtc_state = 0;
	m_pm_data_send = m_pm_data_recv = m_pm_ack = m_pm_req = m_pm_dptr = 0;
	m_pm_state = 0;

	memset(m_egregs, 0xff, sizeof(m_egregs));
}



static STATE_POSTLOAD( mac_state_load )
{
	int overlay;
	mac_state *mac = machine.driver_data<mac_state>();

	overlay = mac->m_overlay;
	mac->m_overlay = -1;
	mac->set_memory_overlay(overlay);
}


DIRECT_UPDATE_HANDLER (overlay_opbaseoverride)
{
	mac_state *mac = machine->driver_data<mac_state>();

	if (mac->m_overlay != -1)
	{
		if ((mac->m_model == MODEL_MAC_PORTABLE) || (mac->m_model == MODEL_MAC_PB100))
		{
			if ((address >= 0x900000) && (address <= 0x9fffff))
			{
				mac->set_memory_overlay(0);		// kill the overlay
			}
		}
		else if ((mac->m_model == MODEL_MAC_SE) || (mac->m_model == MODEL_MAC_CLASSIC))
		{
			if ((address >= 0x400000) && (address <= 0x4fffff))
			{
				mac->set_memory_overlay(0);		// kill the overlay
			}
		}
		else if ((mac->m_model == MODEL_MAC_LC) || (mac->m_model == MODEL_MAC_LC_II) || (mac->m_model == MODEL_MAC_CLASSIC_II))
		{
			if ((address >= 0xa00000) && (address <= 0xafffff))
			{
				mac->set_memory_overlay(0);		// kill the overlay
			}
		}
		else
		{
			if ((address >= 0x40000000) && (address <= 0x4fffffff))
			{
				mac->set_memory_overlay(0);		// kill the overlay
			}
		}
	}

	return address;
}

static void mac_driver_init(running_machine &machine, model_t model)
{
	mac_state *mac = machine.driver_data<mac_state>();

	mac->m_overlay = 1;
	mac->m_scsi_interrupt = 0;
	mac->m_model = model;

	if ((mac->m_model == MODEL_MAC_PORTABLE) || (mac->m_model == MODEL_MAC_PB100))
	{
	}
	else if (model < MODEL_MAC_II)
	{
		/* set up RAM mirror at 0x600000-0x6fffff (0x7fffff ???) */
		mac_install_memory(machine, 0x600000, 0x6fffff, ram_get_size(mac->m_ram), ram_get_ptr(mac->m_ram), FALSE, "bank2");

		/* set up ROM at 0x400000-0x43ffff (-0x5fffff for mac 128k/512k/512ke) */
		mac_install_memory(machine, 0x400000, (model >= MODEL_MAC_PLUS) ? 0x43ffff : 0x5fffff,
			machine.region("bootrom")->bytes(), machine.region("bootrom")->base(), TRUE, "bank3");
	}

	mac->m_overlay = -1;
	mac->set_memory_overlay(1);

	memset(ram_get_ptr(mac->m_ram), 0, ram_get_size(mac->m_ram));

	if ((model == MODEL_MAC_SE) || (model == MODEL_MAC_CLASSIC) || (model == MODEL_MAC_CLASSIC_II) || (model == MODEL_MAC_LC) ||
	    (model == MODEL_MAC_LC_II) || (model == MODEL_MAC_LC_III) || (model == MODEL_MAC_LC_III_PLUS) || ((mac->m_model >= MODEL_MAC_II) && (mac->m_model <= MODEL_MAC_SE30)) ||
	    (model == MODEL_MAC_PORTABLE) || (model == MODEL_MAC_PB100))
	{
		machine.device("maincpu")->memory().space(AS_PROGRAM)->set_direct_update_handler(direct_update_delegate_create_static(overlay_opbaseoverride, machine));
	}

	/* setup keyboard */
	keyboard_init(mac);

	mac->m_inquiry_timeout = machine.scheduler().timer_alloc(FUNC(inquiry_timeout_func));

	/* save state stuff */
	machine.state().register_postload(mac_state_load, NULL);
}

#define MAC_DRIVER_INIT(label, model)	\
DRIVER_INIT( label )	\
{	\
	mac_driver_init(machine, model );	\
}

MAC_DRIVER_INIT(mac128k512k, MODEL_MAC_128K512K)
MAC_DRIVER_INIT(mac512ke, MODEL_MAC_512KE)
MAC_DRIVER_INIT(macplus, MODEL_MAC_PLUS)
MAC_DRIVER_INIT(macse, MODEL_MAC_SE)
MAC_DRIVER_INIT(macclassic, MODEL_MAC_CLASSIC)
MAC_DRIVER_INIT(maclc, MODEL_MAC_LC)
MAC_DRIVER_INIT(maclc2, MODEL_MAC_LC_II)
MAC_DRIVER_INIT(maclc3, MODEL_MAC_LC_III)
MAC_DRIVER_INIT(maclc3plus, MODEL_MAC_LC_III_PLUS)
MAC_DRIVER_INIT(maciici, MODEL_MAC_IICI)
MAC_DRIVER_INIT(maciisi, MODEL_MAC_IISI)
MAC_DRIVER_INIT(macii, MODEL_MAC_II)
MAC_DRIVER_INIT(macse30, MODEL_MAC_SE30)
MAC_DRIVER_INIT(macclassic2, MODEL_MAC_CLASSIC_II)
MAC_DRIVER_INIT(maclrcclassic, MODEL_MAC_COLOR_CLASSIC)
MAC_DRIVER_INIT(macpm6100, MODEL_MAC_POWERMAC_6100)
MAC_DRIVER_INIT(macpm7100, MODEL_MAC_POWERMAC_7100)
MAC_DRIVER_INIT(macpm8100, MODEL_MAC_POWERMAC_8100)
MAC_DRIVER_INIT(macprtb, MODEL_MAC_PORTABLE)
MAC_DRIVER_INIT(macpb100, MODEL_MAC_PB100)

// make the appletalk init fail instead of hanging on the II FDHD/IIx/IIcx/SE30 ROM
static void patch_appletalk_iix(running_machine &machine)
{
	UINT32 *ROM = (UINT32 *)machine.region("bootrom")->base();

	ROM[0x2cc94/4] = 0x6bbe709e;	// bmi 82cc54 moveq #-$62, d0
	ROM[0x370c/4] = 0x4e714e71;	// nop nop - disable ROM checksum
	ROM[0x3710/4] = 0x4e714ed6;	// nop jmp (a6) - disable ROM checksum
}

DRIVER_INIT(maciicx)
{
	patch_appletalk_iix(machine);
	mac_driver_init(machine, MODEL_MAC_IICX);
}

DRIVER_INIT(maciifdhd)
{
	patch_appletalk_iix(machine);
	mac_driver_init(machine, MODEL_MAC_II_FDHD);
}

DRIVER_INIT(maciix)
{
	patch_appletalk_iix(machine);
	mac_driver_init(machine, MODEL_MAC_IIX);
}

void mac_state::nubus_slot_interrupt(UINT8 slot, UINT32 state)
{
	static const UINT8 masks[6] = { 0x1, 0x2, 0x4, 0x8, 0x10, 0x20 };
	mac_state *mac = machine().driver_data<mac_state>();

	slot -= 9;

	if (state)
	{
		mac->m_nubus_irq_state &= ~masks[slot];
	}
	else
	{
		mac->m_nubus_irq_state |= masks[slot];
	}

	if ((mac->m_nubus_irq_state & 0x3f) != 0x3f)
	{
		// HACK: sometimes we miss an ack (possible misbehavior in the VIA?)
		if (m_via2->read_ca1() == 0)
		{
			m_via2->write_ca1(1);
		}

		m_via2->write_ca1(0);
	}
	else
	{
		m_via2->write_ca1(1);
	}
}

void mac_state::vblank_irq()
{
	/* handle ADB keyboard/mouse */
	if (has_adb())
	{
		this->adb_vblank();
	}

	/* handle keyboard */
	if (m_kbd_comm == TRUE)
	{
		int keycode = scan_keyboard(machine());

		if (keycode != 0x7B)
		{
			/* if key pressed, send the code */

			logerror("keyboard enquiry successful, keycode %X\n", keycode);

			m_inquiry_timeout->reset();
			kbd_shift_out(machine(), keycode);
		}
	}

	/* signal VBlank on CA1 input on the VIA */
	if (m_model < MODEL_MAC_II)
	{
		ca1_data ^= 1;
		m_via1->write_ca1(ca1_data);
	}

	if (++irq_count == 60)
	{
		irq_count = 0;

		ca2_data ^= 1;
		/* signal 1 Hz irq on CA2 input on the VIA */
		m_via1->write_ca2(ca2_data);

		rtc_incticks();
	}

	// handle SE/30 vblank IRQ
	if (m_model == MODEL_MAC_SE30)
	{
		if (m_se30_vbl_enable)
		{
			m_via2_vbl ^= 1;
			if (!m_via2_vbl)
			{
				this->nubus_slot_interrupt(0xe, 1);
			}
		}
	}
}

static TIMER_CALLBACK(mac_scanline_tick)
{
	int scanline;
	mac_state *mac = machine.driver_data<mac_state>();

	if (AUDIO_IS_CLASSIC)
	{
		mac_sh_updatebuffer(machine.device("custom"));
	}

	scanline = machine.primary_screen->vpos();
	if (scanline == MAC_V_VIS)
		mac->vblank_irq();

	/* check for mouse changes at 10 irqs per frame */
	if (mac->m_model <= MODEL_MAC_PLUS)
	{
		if (!(scanline % 10))
			mac->mouse_callback();
	}

	mac->m_scanline_timer->adjust(machine.primary_screen->time_until_pos((scanline+1) % MAC_V_TOTAL, 0));
}



/* *************************************************************************
 * Trap Tracing
 *
 * This is debug code that will output diagnostics regarding OS traps called
 * *************************************************************************/

static const char *lookup_trap(UINT16 opcode)
{
	static const struct
	{
		UINT16 trap;
		const char *name;
	} traps[] =
	{
		{ 0xA000, "_Open" },
		{ 0xA001, "_Close" },
		{ 0xA002, "_Read" },
		{ 0xA003, "_Write" },
		{ 0xA004, "_Control" },
		{ 0xA005, "_Status" },
		{ 0xA006, "_KillIO" },
		{ 0xA007, "_GetVolInfo" },
		{ 0xA008, "_Create" },
		{ 0xA009, "_Delete" },
		{ 0xA00A, "_OpenRF" },
		{ 0xA00B, "_Rename" },
		{ 0xA00C, "_GetFileInfo" },
		{ 0xA00D, "_SetFileInfo" },
		{ 0xA00E, "_UnmountVol" },
		{ 0xA00F, "_MountVol" },
		{ 0xA010, "_Allocate" },
		{ 0xA011, "_GetEOF" },
		{ 0xA012, "_SetEOF" },
		{ 0xA013, "_FlushVol" },
		{ 0xA014, "_GetVol" },
		{ 0xA015, "_SetVol" },
		{ 0xA016, "_FInitQueue" },
		{ 0xA017, "_Eject" },
		{ 0xA018, "_GetFPos" },
		{ 0xA019, "_InitZone" },
		{ 0xA01B, "_SetZone" },
		{ 0xA01C, "_FreeMem" },
		{ 0xA01F, "_DisposePtr" },
		{ 0xA020, "_SetPtrSize" },
		{ 0xA021, "_GetPtrSize" },
		{ 0xA023, "_DisposeHandle" },
		{ 0xA024, "_SetHandleSize" },
		{ 0xA025, "_GetHandleSize" },
		{ 0xA027, "_ReallocHandle" },
		{ 0xA029, "_HLock" },
		{ 0xA02A, "_HUnlock" },
		{ 0xA02B, "_EmptyHandle" },
		{ 0xA02C, "_InitApplZone" },
		{ 0xA02D, "_SetApplLimit" },
		{ 0xA02E, "_BlockMove" },
		{ 0xA02F, "_PostEvent" },
		{ 0xA030, "_OSEventAvail" },
		{ 0xA031, "_GetOSEvent" },
		{ 0xA032, "_FlushEvents" },
		{ 0xA033, "_VInstall" },
		{ 0xA034, "_VRemove" },
		{ 0xA035, "_OffLine" },
		{ 0xA036, "_MoreMasters" },
		{ 0xA038, "_WriteParam" },
		{ 0xA039, "_ReadDateTime" },
		{ 0xA03A, "_SetDateTime" },
		{ 0xA03B, "_Delay" },
		{ 0xA03C, "_CmpString" },
		{ 0xA03D, "_DrvrInstall" },
		{ 0xA03E, "_DrvrRemove" },
		{ 0xA03F, "_InitUtil" },
		{ 0xA040, "_ResrvMem" },
		{ 0xA041, "_SetFilLock" },
		{ 0xA042, "_RstFilLock" },
		{ 0xA043, "_SetFilType" },
		{ 0xA044, "_SetFPos" },
		{ 0xA045, "_FlushFile" },
		{ 0xA047, "_SetTrapAddress" },
		{ 0xA049, "_HPurge" },
		{ 0xA04A, "_HNoPurge" },
		{ 0xA04B, "_SetGrowZone" },
		{ 0xA04C, "_CompactMem" },
		{ 0xA04D, "_PurgeMem" },
		{ 0xA04E, "_AddDrive" },
		{ 0xA04F, "_RDrvrInstall" },
		{ 0xA050, "_CompareString" },
		{ 0xA051, "_ReadXPRam" },
		{ 0xA052, "_WriteXPRam" },
		{ 0xA054, "_UprString" },
		{ 0xA055, "_StripAddress" },
		{ 0xA056, "_LowerText" },
		{ 0xA057, "_SetAppBase" },
		{ 0xA058, "_InsTime" },
		{ 0xA059, "_RmvTime" },
		{ 0xA05A, "_PrimeTime" },
		{ 0xA05B, "_PowerOff" },
		{ 0xA05C, "_MemoryDispatch" },
		{ 0xA05D, "_SwapMMUMode" },
		{ 0xA05E, "_NMInstall" },
		{ 0xA05F, "_NMRemove" },
		{ 0xA060, "_FSDispatch" },
		{ 0xA061, "_MaxBlock" },
		{ 0xA063, "_MaxApplZone" },
		{ 0xA064, "_MoveHHi" },
		{ 0xA065, "_StackSpace" },
		{ 0xA067, "_HSetRBit" },
		{ 0xA068, "_HClrRBit" },
		{ 0xA069, "_HGetState" },
		{ 0xA06A, "_HSetState" },
		{ 0xA06C, "_InitFS" },
		{ 0xA06D, "_InitEvents" },
		{ 0xA06E, "_SlotManager" },
		{ 0xA06F, "_SlotVInstall" },
		{ 0xA070, "_SlotVRemove" },
		{ 0xA071, "_AttachVBL" },
		{ 0xA072, "_DoVBLTask" },
		{ 0xA075, "_SIntInstall" },
		{ 0xA076, "_SIntRemove" },
		{ 0xA077, "_CountADBs" },
		{ 0xA078, "_GetIndADB" },
		{ 0xA079, "_GetADBInfo" },
		{ 0xA07A, "_SetADBInfo" },
		{ 0xA07B, "_ADBReInit" },
		{ 0xA07C, "_ADBOp" },
		{ 0xA07D, "_GetDefaultStartup" },
		{ 0xA07E, "_SetDefaultStartup" },
		{ 0xA07F, "_InternalWait" },
		{ 0xA080, "_GetVideoDefault" },
		{ 0xA081, "_SetVideoDefault" },
		{ 0xA082, "_DTInstall" },
		{ 0xA083, "_SetOSDefault" },
		{ 0xA084, "_GetOSDefault" },
		{ 0xA085, "_PMgrOp" },
		{ 0xA086, "_IOPInfoAccess" },
		{ 0xA087, "_IOPMsgRequest" },
		{ 0xA088, "_IOPMoveData" },
		{ 0xA089, "_SCSIAtomic" },
		{ 0xA08A, "_Sleep" },
		{ 0xA08B, "_CommToolboxDispatch" },
		{ 0xA08D, "_DebugUtil" },
		{ 0xA08F, "_DeferUserFn" },
		{ 0xA090, "_SysEnvirons" },
		{ 0xA092, "_EgretDispatch" },
		{ 0xA094, "_ServerDispatch" },
		{ 0xA09E, "_PowerMgrDispatch" },
		{ 0xA09F, "_PowerDispatch" },
		{ 0xA0A4, "_HeapDispatch" },
		{ 0xA0AC, "_FSMDispatch" },
		{ 0xA0AE, "_VADBProc" },
		{ 0xA0DD, "_PPC" },
		{ 0xA0FE, "_TEFindWord" },
		{ 0xA0FF, "_TEFindLine" },
		{ 0xA11A, "_GetZone" },
		{ 0xA11D, "_MaxMem" },
		{ 0xA11E, "_NewPtr" },
		{ 0xA122, "_NewHandle" },
		{ 0xA126, "_HandleZone" },
		{ 0xA128, "_RecoverHandle" },
		{ 0xA12F, "_PPostEvent" },
		{ 0xA146, "_GetTrapAddress" },
		{ 0xA148, "_PtrZone" },
		{ 0xA162, "_PurgeSpace" },
		{ 0xA166, "_NewEmptyHandle" },
		{ 0xA193, "_Microseconds" },
		{ 0xA198, "_HWPriv" },
		{ 0xA1AD, "_Gestalt" },
		{ 0xA200, "_HOpen" },
		{ 0xA207, "_HGetVInfo" },
		{ 0xA208, "_HCreate" },
		{ 0xA209, "_HDelete" },
		{ 0xA20A, "_HOpenRF" },
		{ 0xA20B, "_HRename" },
		{ 0xA20C, "_HGetFileInfo" },
		{ 0xA20D, "_HSetFileInfo" },
		{ 0xA20E, "_HUnmountVol" },
		{ 0xA210, "_AllocContig" },
		{ 0xA214, "_HGetVol" },
		{ 0xA215, "_HSetVol" },
		{ 0xA22E, "_BlockMoveData" },
		{ 0xA241, "_HSetFLock" },
		{ 0xA242, "_HRstFLock" },
		{ 0xA247, "_SetOSTrapAddress" },
		{ 0xA256, "_StripText" },
		{ 0xA260, "_HFSDispatch" },
		{ 0xA285, "_IdleUpdate" },
		{ 0xA28A, "_SlpQInstall" },
		{ 0xA31E, "_NewPtrClear" },
		{ 0xA322, "_NewHandleClear" },
		{ 0xA346, "_GetOSTrapAddress" },
		{ 0xA3AD, "_NewGestalt" },
		{ 0xA456, "_UpperText" },
		{ 0xA458, "_InsXTime" },
		{ 0xA485, "_IdleState" },
		{ 0xA48A, "_SlpQRemove" },
		{ 0xA51E, "_NewPtrSys" },
		{ 0xA522, "_NewHandleSys" },
		{ 0xA562, "_PurgeSpaceSys" },
		{ 0xA5AD, "_ReplaceGestalt" },
		{ 0xA647, "_SetToolBoxTrapAddress" },
		{ 0xA656, "_StripUpperText" },
		{ 0xA685, "_SerialPower" },
		{ 0xA71E, "_NewPtrSysClear" },
		{ 0xA722, "_NewHandleSysClear" },
		{ 0xA746, "_GetToolBoxTrapAddress" },
		{ 0xA7AD, "_GetGestaltProcPtr" },
		{ 0xA800, "_SoundDispatch" },
		{ 0xA801, "_SndDisposeChannel" },
		{ 0xA802, "_SndAddModifier" },
		{ 0xA803, "_SndDoCommand" },
		{ 0xA804, "_SndDoImmediate" },
		{ 0xA805, "_SndPlay" },
		{ 0xA806, "_SndControl" },
		{ 0xA807, "_SndNewChannel" },
		{ 0xA808, "_InitProcMenu" },
		{ 0xA809, "_GetControlVariant" },
		{ 0xA80A, "_GetWVariant" },
		{ 0xA80B, "_PopUpMenuSelect" },
		{ 0xA80C, "_RGetResource" },
		{ 0xA811, "_TESelView" },
		{ 0xA812, "_TEPinScroll" },
		{ 0xA813, "_TEAutoView" },
		{ 0xA814, "_SetFractEnable" },
		{ 0xA815, "_SCSIDispatch" },
		{ 0xA817, "_CopyMask" },
		{ 0xA819, "_XMunger" },
		{ 0xA81A, "_HOpenResFile" },
		{ 0xA81B, "_HCreateResFile" },
		{ 0xA81D, "_InvalMenuBar" },
		{ 0xA821, "_MaxSizeRsrc" },
		{ 0xA822, "_ResourceDispatch" },
		{ 0xA823, "_AliasDispatch" },
		{ 0xA824, "_HFSUtilDispatch" },
		{ 0xA825, "_MenuDispatch" },
		{ 0xA826, "_InsertMenuItem" },
		{ 0xA827, "_HideDialogItem" },
		{ 0xA828, "_ShowDialogItem" },
		{ 0xA82A, "_ComponentDispatch" },
		{ 0xA833, "_ScrnBitMap" },
		{ 0xA834, "_SetFScaleDisable" },
		{ 0xA835, "_FontMetrics" },
		{ 0xA836, "_GetMaskTable" },
		{ 0xA837, "_MeasureText" },
		{ 0xA838, "_CalcMask" },
		{ 0xA839, "_SeedFill" },
		{ 0xA83A, "_ZoomWindow" },
		{ 0xA83B, "_TrackBox" },
		{ 0xA83C, "_TEGetOffset" },
		{ 0xA83D, "_TEDispatch" },
		{ 0xA83E, "_TEStyleNew" },
		{ 0xA847, "_FracCos" },
		{ 0xA848, "_FracSin" },
		{ 0xA849, "_FracSqrt" },
		{ 0xA84A, "_FracMul" },
		{ 0xA84B, "_FracDiv" },
		{ 0xA84D, "_FixDiv" },
		{ 0xA84E, "_GetItemCmd" },
		{ 0xA84F, "_SetItemCmd" },
		{ 0xA850, "_InitCursor" },
		{ 0xA851, "_SetCursor" },
		{ 0xA852, "_HideCursor" },
		{ 0xA853, "_ShowCursor" },
		{ 0xA854, "_FontDispatch" },
		{ 0xA855, "_ShieldCursor" },
		{ 0xA856, "_ObscureCursor" },
		{ 0xA858, "_BitAnd" },
		{ 0xA859, "_BitXOr" },
		{ 0xA85A, "_BitNot" },
		{ 0xA85B, "_BitOr" },
		{ 0xA85C, "_BitShift" },
		{ 0xA85D, "_BitTst" },
		{ 0xA85E, "_BitSet" },
		{ 0xA85F, "_BitClr" },
		{ 0xA860, "_WaitNextEvent" },
		{ 0xA861, "_Random" },
		{ 0xA862, "_ForeColor" },
		{ 0xA863, "_BackColor" },
		{ 0xA864, "_ColorBit" },
		{ 0xA865, "_GetPixel" },
		{ 0xA866, "_StuffHex" },
		{ 0xA867, "_LongMul" },
		{ 0xA868, "_FixMul" },
		{ 0xA869, "_FixRatio" },
		{ 0xA86A, "_HiWord" },
		{ 0xA86B, "_LoWord" },
		{ 0xA86C, "_FixRound" },
		{ 0xA86D, "_InitPort" },
		{ 0xA86E, "_InitGraf" },
		{ 0xA86F, "_OpenPort" },
		{ 0xA870, "_LocalToGlobal" },
		{ 0xA871, "_GlobalToLocal" },
		{ 0xA872, "_GrafDevice" },
		{ 0xA873, "_SetPort" },
		{ 0xA874, "_GetPort" },
		{ 0xA875, "_SetPBits" },
		{ 0xA876, "_PortSize" },
		{ 0xA877, "_MovePortTo" },
		{ 0xA878, "_SetOrigin" },
		{ 0xA879, "_SetClip" },
		{ 0xA87A, "_GetClip" },
		{ 0xA87B, "_ClipRect" },
		{ 0xA87C, "_BackPat" },
		{ 0xA87D, "_ClosePort" },
		{ 0xA87E, "_AddPt" },
		{ 0xA87F, "_SubPt" },
		{ 0xA880, "_SetPt" },
		{ 0xA881, "_EqualPt" },
		{ 0xA882, "_StdText" },
		{ 0xA883, "_DrawChar" },
		{ 0xA884, "_DrawString" },
		{ 0xA885, "_DrawText" },
		{ 0xA886, "_TextWidth" },
		{ 0xA887, "_TextFont" },
		{ 0xA888, "_TextFace" },
		{ 0xA889, "_TextMode" },
		{ 0xA88A, "_TextSize" },
		{ 0xA88B, "_GetFontInfo" },
		{ 0xA88C, "_StringWidth" },
		{ 0xA88D, "_CharWidth" },
		{ 0xA88E, "_SpaceExtra" },
		{ 0xA88F, "_OSDispatch" },
		{ 0xA890, "_StdLine" },
		{ 0xA891, "_LineTo" },
		{ 0xA892, "_Line" },
		{ 0xA893, "_MoveTo" },
		{ 0xA894, "_Move" },
		{ 0xA895, "_ShutDown" },
		{ 0xA896, "_HidePen" },
		{ 0xA897, "_ShowPen" },
		{ 0xA898, "_GetPenState" },
		{ 0xA899, "_SetPenState" },
		{ 0xA89A, "_GetPen" },
		{ 0xA89B, "_PenSize" },
		{ 0xA89C, "_PenMode" },
		{ 0xA89D, "_PenPat" },
		{ 0xA89E, "_PenNormal" },
		{ 0xA89F, "_Moof" },
		{ 0xA8A0, "_StdRect" },
		{ 0xA8A1, "_FrameRect" },
		{ 0xA8A2, "_PaintRect" },
		{ 0xA8A3, "_EraseRect" },
		{ 0xA8A4, "_InverRect" },
		{ 0xA8A5, "_FillRect" },
		{ 0xA8A6, "_EqualRect" },
		{ 0xA8A7, "_SetRect" },
		{ 0xA8A8, "_OffsetRect" },
		{ 0xA8A9, "_InsetRect" },
		{ 0xA8AA, "_SectRect" },
		{ 0xA8AB, "_UnionRect" },
		{ 0xA8AD, "_PtInRect" },
		{ 0xA8AE, "_EmptyRect" },
		{ 0xA8AF, "_StdRRect" },
		{ 0xA8B0, "_FrameRoundRect" },
		{ 0xA8B1, "_PaintRoundRect" },
		{ 0xA8B2, "_EraseRoundRect" },
		{ 0xA8B3, "_InverRoundRect" },
		{ 0xA8B4, "_FillRoundRect" },
		{ 0xA8B5, "_ScriptUtil" },
		{ 0xA8B6, "_StdOval" },
		{ 0xA8B7, "_FrameOval" },
		{ 0xA8B8, "_PaintOval" },
		{ 0xA8B9, "_EraseOval" },
		{ 0xA8BA, "_InvertOval" },
		{ 0xA8BB, "_FillOval" },
		{ 0xA8BC, "_SlopeFromAngle" },
		{ 0xA8BD, "_StdArc" },
		{ 0xA8BE, "_FrameArc" },
		{ 0xA8BF, "_PaintArc" },
		{ 0xA8C0, "_EraseArc" },
		{ 0xA8C1, "_InvertArc" },
		{ 0xA8C2, "_FillArc" },
		{ 0xA8C3, "_PtToAngle" },
		{ 0xA8C4, "_AngleFromSlope" },
		{ 0xA8C5, "_StdPoly" },
		{ 0xA8C6, "_FramePoly" },
		{ 0xA8C7, "_PaintPoly" },
		{ 0xA8C8, "_ErasePoly" },
		{ 0xA8C9, "_InvertPoly" },
		{ 0xA8CA, "_FillPoly" },
		{ 0xA8CB, "_OpenPoly" },
		{ 0xA8CC, "_ClosePoly" },
		{ 0xA8CD, "_KillPoly" },
		{ 0xA8CE, "_OffsetPoly" },
		{ 0xA8CF, "_PackBits" },
		{ 0xA8D0, "_UnpackBits" },
		{ 0xA8D1, "_StdRgn" },
		{ 0xA8D2, "_FrameRgn" },
		{ 0xA8D3, "_PaintRgn" },
		{ 0xA8D4, "_EraseRgn" },
		{ 0xA8D5, "_InverRgn" },
		{ 0xA8D6, "_FillRgn" },
		{ 0xA8D7, "_BitMapToRegion" },
		{ 0xA8D8, "_NewRgn" },
		{ 0xA8D9, "_DisposeRgn" },
		{ 0xA8DA, "_OpenRgn" },
		{ 0xA8DB, "_CloseRgn" },
		{ 0xA8DC, "_CopyRgn" },
		{ 0xA8DD, "_SetEmptyRgn" },
		{ 0xA8DE, "_SetRecRgn" },
		{ 0xA8DF, "_RectRgn" },
		{ 0xA8E0, "_OffsetRgn" },
		{ 0xA8E1, "_InsetRgn" },
		{ 0xA8E2, "_EmptyRgn" },
		{ 0xA8E3, "_EqualRgn" },
		{ 0xA8E4, "_SectRgn" },
		{ 0xA8E5, "_UnionRgn" },
		{ 0xA8E6, "_DiffRgn" },
		{ 0xA8E7, "_XOrRgn" },
		{ 0xA8E8, "_PtInRgn" },
		{ 0xA8E9, "_RectInRgn" },
		{ 0xA8EA, "_SetStdProcs" },
		{ 0xA8EB, "_StdBits" },
		{ 0xA8EC, "_CopyBits" },
		{ 0xA8ED, "_StdTxMeas" },
		{ 0xA8EE, "_StdGetPic" },
		{ 0xA8EF, "_ScrollRect" },
		{ 0xA8F0, "_StdPutPic" },
		{ 0xA8F1, "_StdComment" },
		{ 0xA8F2, "_PicComment" },
		{ 0xA8F3, "_OpenPicture" },
		{ 0xA8F4, "_ClosePicture" },
		{ 0xA8F5, "_KillPicture" },
		{ 0xA8F6, "_DrawPicture" },
		{ 0xA8F7, "_Layout" },
		{ 0xA8F8, "_ScalePt" },
		{ 0xA8F9, "_MapPt" },
		{ 0xA8FA, "_MapRect" },
		{ 0xA8FB, "_MapRgn" },
		{ 0xA8FC, "_MapPoly" },
		{ 0xA8FD, "_PrGlue" },
		{ 0xA8FE, "_InitFonts" },
		{ 0xA8FF, "_GetFName" },
		{ 0xA900, "_GetFNum" },
		{ 0xA901, "_FMSwapFont" },
		{ 0xA902, "_RealFont" },
		{ 0xA903, "_SetFontLock" },
		{ 0xA904, "_DrawGrowIcon" },
		{ 0xA905, "_DragGrayRgn" },
		{ 0xA906, "_NewString" },
		{ 0xA907, "_SetString" },
		{ 0xA908, "_ShowHide" },
		{ 0xA909, "_CalcVis" },
		{ 0xA90A, "_CalcVBehind" },
		{ 0xA90B, "_ClipAbove" },
		{ 0xA90C, "_PaintOne" },
		{ 0xA90D, "_PaintBehind" },
		{ 0xA90E, "_SaveOld" },
		{ 0xA90F, "_DrawNew" },
		{ 0xA910, "_GetWMgrPort" },
		{ 0xA911, "_CheckUpDate" },
		{ 0xA912, "_InitWindows" },
		{ 0xA913, "_NewWindow" },
		{ 0xA914, "_DisposeWindow" },
		{ 0xA915, "_ShowWindow" },
		{ 0xA916, "_HideWindow" },
		{ 0xA917, "_GetWRefCon" },
		{ 0xA918, "_SetWRefCon" },
		{ 0xA919, "_GetWTitle" },
		{ 0xA91A, "_SetWTitle" },
		{ 0xA91B, "_MoveWindow" },
		{ 0xA91C, "_HiliteWindow" },
		{ 0xA91D, "_SizeWindow" },
		{ 0xA91E, "_TrackGoAway" },
		{ 0xA91F, "_SelectWindow" },
		{ 0xA920, "_BringToFront" },
		{ 0xA921, "_SendBehind" },
		{ 0xA922, "_BeginUpDate" },
		{ 0xA923, "_EndUpDate" },
		{ 0xA924, "_FrontWindow" },
		{ 0xA925, "_DragWindow" },
		{ 0xA926, "_DragTheRgn" },
		{ 0xA927, "_InvalRgn" },
		{ 0xA928, "_InvalRect" },
		{ 0xA929, "_ValidRgn" },
		{ 0xA92A, "_ValidRect" },
		{ 0xA92B, "_GrowWindow" },
		{ 0xA92C, "_FindWindow" },
		{ 0xA92D, "_CloseWindow" },
		{ 0xA92E, "_SetWindowPic" },
		{ 0xA92F, "_GetWindowPic" },
		{ 0xA930, "_InitMenus" },
		{ 0xA931, "_NewMenu" },
		{ 0xA932, "_DisposeMenu" },
		{ 0xA933, "_AppendMenu" },
		{ 0xA934, "_ClearMenuBar" },
		{ 0xA935, "_InsertMenu" },
		{ 0xA936, "_DeleteMenu" },
		{ 0xA937, "_DrawMenuBar" },
		{ 0xA938, "_HiliteMenu" },
		{ 0xA939, "_EnableItem" },
		{ 0xA93A, "_DisableItem" },
		{ 0xA93B, "_GetMenuBar" },
		{ 0xA93C, "_SetMenuBar" },
		{ 0xA93D, "_MenuSelect" },
		{ 0xA93E, "_MenuKey" },
		{ 0xA93F, "_GetItmIcon" },
		{ 0xA940, "_SetItmIcon" },
		{ 0xA941, "_GetItmStyle" },
		{ 0xA942, "_SetItmStyle" },
		{ 0xA943, "_GetItmMark" },
		{ 0xA944, "_SetItmMark" },
		{ 0xA945, "_CheckItem" },
		{ 0xA946, "_GetMenuItemText" },
		{ 0xA947, "_SetMenuItemText" },
		{ 0xA948, "_CalcMenuSize" },
		{ 0xA949, "_GetMenuHandle" },
		{ 0xA94A, "_SetMFlash" },
		{ 0xA94B, "_PlotIcon" },
		{ 0xA94C, "_FlashMenuBar" },
		{ 0xA94D, "_AppendResMenu" },
		{ 0xA94E, "_PinRect" },
		{ 0xA94F, "_DeltaPoint" },
		{ 0xA950, "_CountMItems" },
		{ 0xA951, "_InsertResMenu" },
		{ 0xA952, "_DeleteMenuItem" },
		{ 0xA953, "_UpdtControl" },
		{ 0xA954, "_NewControl" },
		{ 0xA955, "_DisposeControl" },
		{ 0xA956, "_KillControls" },
		{ 0xA957, "_ShowControl" },
		{ 0xA958, "_HideControl" },
		{ 0xA959, "_MoveControl" },
		{ 0xA95A, "_GetControlReference" },
		{ 0xA95B, "_SetControlReference" },
		{ 0xA95C, "_SizeControl" },
		{ 0xA95D, "_HiliteControl" },
		{ 0xA95E, "_GetControlTitle" },
		{ 0xA95F, "_SetControlTitle" },
		{ 0xA960, "_GetControlValue" },
		{ 0xA961, "_GetControlMinimum" },
		{ 0xA962, "_GetControlMaximum" },
		{ 0xA963, "_SetControlValue" },
		{ 0xA964, "_SetControlMinimum" },
		{ 0xA965, "_SetControlMaximum" },
		{ 0xA966, "_TestControl" },
		{ 0xA967, "_DragControl" },
		{ 0xA968, "_TrackControl" },
		{ 0xA969, "_DrawControls" },
		{ 0xA96A, "_GetControlAction" },
		{ 0xA96B, "_SetControlAction" },
		{ 0xA96C, "_FindControl" },
		{ 0xA96E, "_Dequeue" },
		{ 0xA96F, "_Enqueue" },
		{ 0xA970, "_GetNextEvent" },
		{ 0xA971, "_EventAvail" },
		{ 0xA972, "_GetMouse" },
		{ 0xA973, "_StillDown" },
		{ 0xA974, "_Button" },
		{ 0xA975, "_TickCount" },
		{ 0xA976, "_GetKeys" },
		{ 0xA977, "_WaitMouseUp" },
		{ 0xA978, "_UpdtDialog" },
		{ 0xA97B, "_InitDialogs" },
		{ 0xA97C, "_GetNewDialog" },
		{ 0xA97D, "_NewDialog" },
		{ 0xA97E, "_SelectDialogItemText" },
		{ 0xA97F, "_IsDialogEvent" },
		{ 0xA980, "_DialogSelect" },
		{ 0xA981, "_DrawDialog" },
		{ 0xA982, "_CloseDialog" },
		{ 0xA983, "_DisposeDialog" },
		{ 0xA984, "_FindDialogItem" },
		{ 0xA985, "_Alert" },
		{ 0xA986, "_StopAlert" },
		{ 0xA987, "_NoteAlert" },
		{ 0xA988, "_CautionAlert" },
		{ 0xA98B, "_ParamText" },
		{ 0xA98C, "_ErrorSound" },
		{ 0xA98D, "_GetDialogItem" },
		{ 0xA98E, "_SetDialogItem" },
		{ 0xA98F, "_SetDialogItemText" },
		{ 0xA990, "_GetDialogItemText" },
		{ 0xA991, "_ModalDialog" },
		{ 0xA992, "_DetachResource" },
		{ 0xA993, "_SetResPurge" },
		{ 0xA994, "_CurResFile" },
		{ 0xA995, "_InitResources" },
		{ 0xA996, "_RsrcZoneInit" },
		{ 0xA997, "_OpenResFile" },
		{ 0xA998, "_UseResFile" },
		{ 0xA999, "_UpdateResFile" },
		{ 0xA99A, "_CloseResFile" },
		{ 0xA99B, "_SetResLoad" },
		{ 0xA99C, "_CountResources" },
		{ 0xA99D, "_GetIndResource" },
		{ 0xA99E, "_CountTypes" },
		{ 0xA99F, "_GetIndType" },
		{ 0xA9A0, "_GetResource" },
		{ 0xA9A1, "_GetNamedResource" },
		{ 0xA9A2, "_LoadResource" },
		{ 0xA9A3, "_ReleaseResource" },
		{ 0xA9A4, "_HomeResFile" },
		{ 0xA9A5, "_SizeRsrc" },
		{ 0xA9A6, "_GetResAttrs" },
		{ 0xA9A7, "_SetResAttrs" },
		{ 0xA9A8, "_GetResInfo" },
		{ 0xA9A9, "_SetResInfo" },
		{ 0xA9AA, "_ChangedResource" },
		{ 0xA9AB, "_AddResource" },
		{ 0xA9AC, "_AddReference" },
		{ 0xA9AD, "_RmveResource" },
		{ 0xA9AE, "_RmveReference" },
		{ 0xA9AF, "_ResError" },
		{ 0xA9B0, "_WriteResource" },
		{ 0xA9B1, "_CreateResFile" },
		{ 0xA9B2, "_SystemEvent" },
		{ 0xA9B3, "_SystemClick" },
		{ 0xA9B4, "_SystemTask" },
		{ 0xA9B5, "_SystemMenu" },
		{ 0xA9B6, "_OpenDeskAcc" },
		{ 0xA9B7, "_CloseDeskAcc" },
		{ 0xA9B8, "_GetPattern" },
		{ 0xA9B9, "_GetCursor" },
		{ 0xA9BA, "_GetString" },
		{ 0xA9BB, "_GetIcon" },
		{ 0xA9BC, "_GetPicture" },
		{ 0xA9BD, "_GetNewWindow" },
		{ 0xA9BE, "_GetNewControl" },
		{ 0xA9BF, "_GetRMenu" },
		{ 0xA9C0, "_GetNewMBar" },
		{ 0xA9C1, "_UniqueID" },
		{ 0xA9C2, "_SysEdit" },
		{ 0xA9C3, "_KeyTranslate" },
		{ 0xA9C4, "_OpenRFPerm" },
		{ 0xA9C5, "_RsrcMapEntry" },
		{ 0xA9C6, "_SecondsToDate" },
		{ 0xA9C7, "_DateToSeconds" },
		{ 0xA9C8, "_SysBeep" },
		{ 0xA9C9, "_SysError" },
		{ 0xA9CA, "_PutIcon" },
		{ 0xA9CB, "_TEGetText" },
		{ 0xA9CC, "_TEInit" },
		{ 0xA9CD, "_TEDispose" },
		{ 0xA9CE, "_TETextBox" },
		{ 0xA9CF, "_TESetText" },
		{ 0xA9D0, "_TECalText" },
		{ 0xA9D1, "_TESetSelect" },
		{ 0xA9D2, "_TENew" },
		{ 0xA9D3, "_TEUpdate" },
		{ 0xA9D4, "_TEClick" },
		{ 0xA9D5, "_TECopy" },
		{ 0xA9D6, "_TECut" },
		{ 0xA9D7, "_TEDelete" },
		{ 0xA9D8, "_TEActivate" },
		{ 0xA9D9, "_TEDeactivate" },
		{ 0xA9DA, "_TEIdle" },
		{ 0xA9DB, "_TEPaste" },
		{ 0xA9DC, "_TEKey" },
		{ 0xA9DD, "_TEScroll" },
		{ 0xA9DE, "_TEInsert" },
		{ 0xA9DF, "_TESetAlignment" },
		{ 0xA9E0, "_Munger" },
		{ 0xA9E1, "_HandToHand" },
		{ 0xA9E2, "_PtrToXHand" },
		{ 0xA9E3, "_PtrToHand" },
		{ 0xA9E4, "_HandAndHand" },
		{ 0xA9E5, "_InitPack" },
		{ 0xA9E6, "_InitAllPacks" },
		{ 0xA9EF, "_PtrAndHand" },
		{ 0xA9F0, "_LoadSeg" },
		{ 0xA9F1, "_UnLoadSeg" },
		{ 0xA9F2, "_Launch" },
		{ 0xA9F3, "_Chain" },
		{ 0xA9F4, "_ExitToShell" },
		{ 0xA9F5, "_GetAppParms" },
		{ 0xA9F6, "_GetResFileAttrs" },
		{ 0xA9F7, "_SetResFileAttrs" },
		{ 0xA9F8, "_MethodDispatch" },
		{ 0xA9F9, "_InfoScrap" },
		{ 0xA9FA, "_UnloadScrap" },
		{ 0xA9FB, "_LoadScrap" },
		{ 0xA9FC, "_ZeroScrap" },
		{ 0xA9FD, "_GetScrap" },
		{ 0xA9FE, "_PutScrap" },
		{ 0xA9FF, "_Debugger" },
		{ 0xAA00, "_OpenCPort" },
		{ 0xAA01, "_InitCPort" },
		{ 0xAA02, "_CloseCPort" },
		{ 0xAA03, "_NewPixMap" },
		{ 0xAA04, "_DisposePixMap" },
		{ 0xAA05, "_CopyPixMap" },
		{ 0xAA06, "_SetPortPix" },
		{ 0xAA07, "_NewPixPat" },
		{ 0xAA08, "_DisposePixPat" },
		{ 0xAA09, "_CopyPixPat" },
		{ 0xAA0A, "_PenPixPat" },
		{ 0xAA0B, "_BackPixPat" },
		{ 0xAA0C, "_GetPixPat" },
		{ 0xAA0D, "_MakeRGBPat" },
		{ 0xAA0E, "_FillCRect" },
		{ 0xAA0F, "_FillCOval" },
		{ 0xAA10, "_FillCRoundRect" },
		{ 0xAA11, "_FillCArc" },
		{ 0xAA12, "_FillCRgn" },
		{ 0xAA13, "_FillCPoly" },
		{ 0xAA14, "_RGBForeColor" },
		{ 0xAA15, "_RGBBackColor" },
		{ 0xAA16, "_SetCPixel" },
		{ 0xAA17, "_GetCPixel" },
		{ 0xAA18, "_GetCTable" },
		{ 0xAA19, "_GetForeColor" },
		{ 0xAA1A, "_GetBackColor" },
		{ 0xAA1B, "_GetCCursor" },
		{ 0xAA1C, "_SetCCursor" },
		{ 0xAA1D, "_AllocCursor" },
		{ 0xAA1E, "_GetCIcon" },
		{ 0xAA1F, "_PlotCIcon" },
		{ 0xAA20, "_OpenCPicture" },
		{ 0xAA21, "_OpColor" },
		{ 0xAA22, "_HiliteColor" },
		{ 0xAA23, "_CharExtra" },
		{ 0xAA24, "_DisposeCTable" },
		{ 0xAA25, "_DisposeCIcon" },
		{ 0xAA26, "_DisposeCCursor" },
		{ 0xAA27, "_GetMaxDevice" },
		{ 0xAA28, "_GetCTSeed" },
		{ 0xAA29, "_GetDeviceList" },
		{ 0xAA2A, "_GetMainDevice" },
		{ 0xAA2B, "_GetNextDevice" },
		{ 0xAA2C, "_TestDeviceAttribute" },
		{ 0xAA2D, "_SetDeviceAttribute" },
		{ 0xAA2E, "_InitGDevice" },
		{ 0xAA2F, "_NewGDevice" },
		{ 0xAA30, "_DisposeGDevice" },
		{ 0xAA31, "_SetGDevice" },
		{ 0xAA32, "_GetGDevice" },
		{ 0xAA35, "_InvertColor" },
		{ 0xAA36, "_RealColor" },
		{ 0xAA37, "_GetSubTable" },
		{ 0xAA38, "_UpdatePixMap" },
		{ 0xAA39, "_MakeITable" },
		{ 0xAA3A, "_AddSearch" },
		{ 0xAA3B, "_AddComp" },
		{ 0xAA3C, "_SetClientID" },
		{ 0xAA3D, "_ProtectEntry" },
		{ 0xAA3E, "_ReserveEntry" },
		{ 0xAA3F, "_SetEntries" },
		{ 0xAA40, "_QDError" },
		{ 0xAA41, "_SetWinColor" },
		{ 0xAA42, "_GetAuxWin" },
		{ 0xAA43, "_SetControlColor" },
		{ 0xAA44, "_GetAuxiliaryControlRecord" },
		{ 0xAA45, "_NewCWindow" },
		{ 0xAA46, "_GetNewCWindow" },
		{ 0xAA47, "_SetDeskCPat" },
		{ 0xAA48, "_GetCWMgrPort" },
		{ 0xAA49, "_SaveEntries" },
		{ 0xAA4A, "_RestoreEntries" },
		{ 0xAA4B, "_NewColorDialog" },
		{ 0xAA4C, "_DelSearch" },
		{ 0xAA4D, "_DelComp" },
		{ 0xAA4E, "_SetStdCProcs" },
		{ 0xAA4F, "_CalcCMask" },
		{ 0xAA50, "_SeedCFill" },
		{ 0xAA51, "_CopyDeepMask" },
		{ 0xAA52, "_HFSPinaforeDispatch" },
		{ 0xAA53, "_DictionaryDispatch" },
		{ 0xAA54, "_TextServicesDispatch" },
		{ 0xAA56, "_SpeechRecognitionDispatch" },
		{ 0xAA57, "_DockingDispatch" },
		{ 0xAA59, "_MixedModeDispatch" },
		{ 0xAA5A, "_CodeFragmentDispatch" },
		{ 0xAA5C, "_OCEUtils" },
		{ 0xAA5D, "_DigitalSignature" },
		{ 0xAA5E, "_TBDispatch" },
		{ 0xAA60, "_DeleteMCEntries" },
		{ 0xAA61, "_GetMCInfo" },
		{ 0xAA62, "_SetMCInfo" },
		{ 0xAA63, "_DisposeMCInfo" },
		{ 0xAA64, "_GetMCEntry" },
		{ 0xAA65, "_SetMCEntries" },
		{ 0xAA66, "_MenuChoice" },
		{ 0xAA67, "_ModalDialogMenuSetup" },
		{ 0xAA68, "_DialogDispatch" },
		{ 0xAA73, "_ControlDispatch" },
		{ 0xAA74, "_AppearanceDispatch" },
		{ 0xAA7E, "_SysDebugDispatch" },
		{ 0xAA80, "_AVLTreeDispatch" },
		{ 0xAA81, "_FileMappingDispatch" },
		{ 0xAA90, "_InitPalettes" },
		{ 0xAA91, "_NewPalette" },
		{ 0xAA92, "_GetNewPalette" },
		{ 0xAA93, "_DisposePalette" },
		{ 0xAA94, "_ActivatePalette" },
		{ 0xAA95, "_NSetPalette" },
		{ 0xAA96, "_GetPalette" },
		{ 0xAA97, "_PmForeColor" },
		{ 0xAA98, "_PmBackColor" },
		{ 0xAA99, "_AnimateEntry" },
		{ 0xAA9A, "_AnimatePalette" },
		{ 0xAA9B, "_GetEntryColor" },
		{ 0xAA9C, "_SetEntryColor" },
		{ 0xAA9D, "_GetEntryUsage" },
		{ 0xAA9E, "_SetEntryUsage" },
		{ 0xAAA1, "_CopyPalette" },
		{ 0xAAA2, "_PaletteDispatch" },
		{ 0xAAA3, "_CodecDispatch" },
		{ 0xAAA4, "_ALMDispatch" },
		{ 0xAADB, "_CursorDeviceDispatch" },
		{ 0xAAF2, "_ControlStripDispatch" },
		{ 0xAAF3, "_ExpansionManager" },
		{ 0xAB1D, "_QDExtensions" },
		{ 0xABC3, "_NQDMisc" },
		{ 0xABC9, "_IconDispatch" },
		{ 0xABCA, "_DeviceLoop" },
		{ 0xABEB, "_DisplayDispatch" },
		{ 0xABED, "_DragDispatch" },
		{ 0xABF1, "_GestaltValueDispatch" },
		{ 0xABF2, "_ThreadDispatch" },
		{ 0xABF6, "_CollectionMgr" },
		{ 0xABF8, "_StdOpcodeProc" },
		{ 0xABFC, "_TranslationDispatch" },
		{ 0xABFF, "_DebugStr" }
	};

	int i;

	for (i = 0; i < ARRAY_LENGTH(traps); i++)
	{
		if (traps[i].trap == opcode)
			return traps[i].name;
	}
	return NULL;
}



static offs_t mac_dasm_override(device_t &device, char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram, int options)
{
	UINT16 opcode;
	unsigned result = 0;
	const char *trap;

	opcode = oprom[0]<<8 | oprom[1];
	if ((opcode & 0xF000) == 0xA000)
	{
		trap = lookup_trap(opcode);
		if (trap)
		{
			strcpy(buffer, trap);
			result = 2;
		}
	}
	return result;
}



#ifdef MAC_TRACETRAP
static void mac_tracetrap(const char *cpu_name_local, int addr, int trap)
{
	typedef struct
	{
		int csCode;
		const char *name;
	} sonycscodeentry;

	static const sonycscodeentry cscodes[] =
	{
		{ 1, "KillIO" },
		{ 5, "VerifyDisk" },
		{ 6, "FormatDisk" },
		{ 7, "EjectDisk" },
		{ 8, "SetTagBuffer" },
		{ 9, "TrackCacheControl" },
		{ 23, "ReturnDriveInfo" }
	};

	static const char *const scsisels[] =
	{
		"SCSIReset",	/* $00 */
		"SCSIGet",		/* $01 */
		"SCSISelect",	/* $02 */
		"SCSICmd",		/* $03 */
		"SCSIComplete",	/* $04 */
		"SCSIRead",		/* $05 */
		"SCSIWrite",	/* $06 */
		NULL,			/* $07 */
		"SCSIRBlind",	/* $08 */
		"SCSIWBlind",	/* $09 */
		"SCSIStat",		/* $0A */
		"SCSISelAtn",	/* $0B */
		"SCSIMsgIn",	/* $0C */
		"SCSIMsgOut",	/* $0D */
	};

	int i, a0, a7, d0, d1;
	int csCode, ioVRefNum, ioRefNum, ioCRefNum, ioCompletion, ioBuffer, ioReqCount, ioPosOffset;
	char *s;
	unsigned char *mem;
	char buf[256];
	const char *trapstr;

	trapstr = lookup_trap(trap);
	if (trapstr)
		strcpy(buf, trapstr);
	else
		sprintf(buf, "Trap $%04x", trap);

	s = &buf[strlen(buf)];
	mem = mac_ram_ptr;
	a0 = cpu_get_reg(M68K_A0);
	a7 = cpu_get_reg(M68K_A7);
	d0 = cpu_get_reg(M68K_D0);
	d1 = cpu_get_reg(M68K_D1);

	switch(trap)
	{
	case 0xa004:	/* _Control */
		ioVRefNum = *((INT16*) (mem + a0 + 22));
		ioCRefNum = *((INT16*) (mem + a0 + 24));
		csCode = *((UINT16*) (mem + a0 + 26));
		sprintf(s, " ioVRefNum=%i ioCRefNum=%i csCode=%i", ioVRefNum, ioCRefNum, csCode);

		for (i = 0; i < ARRAY_LENGTH(cscodes); i++)
		{
			if (cscodes[i].csCode == csCode)
			{
				strcat(s, "=");
				strcat(s, cscodes[i].name);
				break;
			}
		}
		break;

	case 0xa002:	/* _Read */
		ioCompletion = (*((INT16*) (mem + a0 + 12)) << 16) + *((INT16*) (mem + a0 + 14));
		ioVRefNum = *((INT16*) (mem + a0 + 22));
		ioRefNum = *((INT16*) (mem + a0 + 24));
		ioBuffer = (*((INT16*) (mem + a0 + 32)) << 16) + *((INT16*) (mem + a0 + 34));
		ioReqCount = (*((INT16*) (mem + a0 + 36)) << 16) + *((INT16*) (mem + a0 + 38));
		ioPosOffset = (*((INT16*) (mem + a0 + 46)) << 16) + *((INT16*) (mem + a0 + 48));
		sprintf(s, " ioCompletion=0x%08x ioVRefNum=%i ioRefNum=%i ioBuffer=0x%08x ioReqCount=%i ioPosOffset=%i",
			ioCompletion, ioVRefNum, ioRefNum, ioBuffer, ioReqCount, ioPosOffset);
		break;

	case 0xa04e:	/* _AddDrive */
		sprintf(s, " drvrRefNum=%i drvNum=%i qEl=0x%08x", (int) (INT16) d0, (int) (INT16) d1, a0);
		break;

	case 0xa9a0:	/* _GetResource */
		/* HACKHACK - the 'type' output assumes that the host is little endian
         * since this is just trace code it isn't much of an issue
         */
		sprintf(s, " type='%c%c%c%c' id=%i", (char) mem[a7+3], (char) mem[a7+2],
			(char) mem[a7+5], (char) mem[a7+4], *((INT16*) (mem + a7)));
		break;

	case 0xa815:	/* _SCSIDispatch */
		i = *((UINT16*) (mem + a7));
		if (i < ARRAY_LENGTH(scsisels))
			if (scsisels[i])
				sprintf(s, " (%s)", scsisels[i]);
		break;
	}

	logerror("mac_trace_trap: %s at 0x%08x: %s\n",cpu_name_local, addr, buf);
}
#endif
