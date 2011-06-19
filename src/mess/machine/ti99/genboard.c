/***************************************************************************
    Geneve 9640 system board

    Onboard SRAM configuration:
    There is an adjustable SRAM configuration on board, representing the
    various enhancements by users.

    The standard memory configuration as reported by chkdsk (32 KiB):
    557056 bytes of total memory

    With 64 KiB SRAM:
    589824 bytes of total memory

    With 384 KiB SRAM:
    917504 bytes of total memory

    The original 32 KiB SRAM memory needs to be expanded to 64 KiB for
    MDOS 2.50s and higher, or the system will lock up. Therefore the emulation
    default is 64 KiB.

    The ultimate expansion is a 512 KiB SRAM circuit wired to the gate array
    to provide 48 pages of fast static RAM. This also requires to build an
    adapter for a larger socket. From the 512 KiB, only 384 KiB will be
    accessed, since the higher pages are hidden behind the EPROM pages.

    === Address map ===
    p,q = page value bit (q = AMC, AMB, AMA)
    c = address offset within 8 KiB page

    p pqqq pppc cccc cccc cccc

    0 0... .... .... .... .... on-board dram 512 KiB

    0 1... .... .... .... .... on-board future expansion 512 KiB or Memex with Genmod

    1 00.. .... .... .... .... p-box AMA=0 (256 KiB)
    1 010. .... .... .... .... p-box AMA=1 AMB=0 (128 KiB)
    1 0110 .... .... .... .... p-box AMA=1 AMB=1 AMC=0 (64 KiB)

    1 0111 00.. .... .... .... p-box address block 0xxx, 2xxx
    1 0111 010. .... .... .... p-box address block 4xxx (DSR)
    1 0111 011. .... .... .... p-box address block 6xxx
    1 0111 100. .... .... .... p-box address block 8xxx (Speech at 0x9000)
    1 0111 101. .... .... .... p-box address block axxx
    1 0111 11.. .... .... .... p-box address block cxxx, exxx

    1 100. .... .... .... .... on-board sram (128K) -\
    1 101. .... .... .... .... on-board sram (128K) --+- maximum SRAM expansion
    1 1100 .... .... .... .... on-board sram (64K) --/
    1 1101 0... .... .... .... on-board sram (32K) - additional 32 KiB required for MDOS 2.50s and higher
    1 1101 1... .... .... .... on-board sram (32K) - standard setup

    1 111. ..0. .... .... .... on-board boot1
    1 111. ..1. .... .... .... on-board boot2

    The TI console (or more precise, the Flex Cable Interface) sets the AMA/B/C
    lines to 1. Most cards actually check for AMA/B/C=1. However, this decoding
    was forgotten in third party cards which cause the card address space
    to be mirrored. The usual DSR space at 4000-5fff which would be reachable
    via page 0xba is then mirrored on a number of other pages:

    10 xxx 010x = 82, 8a, 92, 9a, a2, aa, b2, ba

    Another block to take care of is 0xbc which covers 8000-9fff since this
    area contains the speech synthesizer port at 9000/9400.

    For the standard Geneve, only prefix 10 is routed to the P-Box. The Genmod
    modification wires these address lines to pins 8 and 9 in the P-Box as AMD and
    AME. This requires all cards to be equipped with an additional selection logic
    to detect AMD=0, AME=1. Otherwise these cards, although completely decoding the
    19-bit address, would reappear at 512 KiB distances.

    For the page numbers we get:
    Standard:
    00-3f are internal (DRAM)
    40-7f are internal expansion, never used
    80-bf are the P-Box address space (optional Memex and peripheral cards at that location)
    c0-ff are internal (SRAM, EPROM)

    Genmod:
    00-3f are the P-Box address space (expect Memex at that location)
    40-7f are the P-Box address space (expect Memex at that location)
    80-bf are the P-Box address space (expect Memex at that location)
    c0-ef are the P-Box address space (expect Memex at that location)
    f0-ff are internal (EPROM)

    Genmod's double switch box is also emulated. There are two switches:
    - Turbo mode: Activates or deactivates the wait state logic on the Geneve
      board. This switch may be changed at any time.
    - TI mode: Selects between the on-board memory, which is obviously required
      for the GPL interpreter, and the external Memex memory. This switch
      triggers a reset when changed.

    Michael Zapf, February 2011
***************************************************************************/

#include "emu.h"
#include "genboard.h"
#include "video/v9938.h"
#include "videowrp.h"
#include "sound/sn76496.h"
#include "machine/mm58274c.h"
#include "peribox.h"

#define KEYQUEUESIZE 256
#define MAXKEYMSGLENGTH 10
#define KEYAUTOREPEATDELAY 30
#define KEYAUTOREPEATRATE 6

#define SRAM_SIZE 384*1024   // maximum SRAM expansion on-board
#define DRAM_SIZE 512*1024

typedef struct _genboard_state
{
	/* Mouse support */
	int		last_mx;
	int		last_my;

	/* Joystick support */
	int 	joySel;

	/* Keyboard support */
	UINT8	keyQueue[KEYQUEUESIZE];
	int 	keyQueueHead;
	int 	keyQueueLen;
	int 	keyInBuf;
	int 	keyReset;
	UINT32	keyStateSave[4];
	int 	keyNumLockState;
	int 	keyCtrlState;
	int 	keyAltState;
	int 	keyRealShiftState;
	int 	keyFakeShiftState;
	int 	keyFakeUnshiftState;
	int 	keyAutoRepeatKey;
	int 	keyAutoRepeatTimer;

	/* Mode flags */
	int		palvideo;
	int		capslock;
	int		keyboard_clock;
	int		keep_keybuf;
	int		zero_wait;

	int		genmod;
	int 	turbo;
	int		timode;

	/* GROM simulation */
	int		grom_address;
	int		gromraddr_LSB;
	int		gromwaddr_LSB;

	/* Memory */
	UINT8	*sram;
	UINT8	*dram;
	UINT8	*eprom;
	int		sram_mask, sram_val;

	/* Mapper */
	int 	geneve_mode;
	int		direct_mode;
	int		cartridge_size_8K;
	int		cartridge_secondpage;
	int		cartridge6_writable;
	int		cartridge7_writable;
	int		map[8];

	int		line_count;

	/* Devices */
	device_t	*cpu;
	device_t	*video;
	device_t  *tms9901;
	device_t	*clock;
	device_t	*peribox;
	device_t	*sound;

} genboard_state;

static const UINT8 keyboard_mf1_code[0xe] =
{
	/* extended keys that are equivalent to non-extended keys */
	0x1c,	/* keypad enter */
	0x1d,	/* right control */
	0x38,	/* alt gr */
	// extra codes are 0x5b for Left Windows, 0x5c for Right Windows, 0x5d
	// for Menu, 0x5e for power, 0x5f for sleep, 0x63 for wake, but I doubt
	// any Geneve program would take advantage of these. */

	// extended key that is equivalent to a non-extended key
	// with shift off
	0x35,	/* pad slash */

	// extended keys that are equivalent to non-extended keys
	// with numlock off
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

static void poll_keyboard(device_t *device);
static void poll_mouse(device_t *device);
static void read_key_if_possible(genboard_state *board);
static UINT8 get_recent_key(device_t *device);

static const char *const keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3", "KEY4", "KEY5", "KEY6", "KEY7" };

INLINE genboard_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == GENBOARD);

	return (genboard_state *)downcast<legacy_device_base *>(device)->token();
}

/*
    scanline interrupt
*/
INTERRUPT_GEN( geneve_hblank_interrupt )
{
	device_t *dev = device->machine().device("geneve_board");
	genboard_state *board = get_safe_token(dev);
	v9938_interrupt(device->machine(), 0);
	board->line_count++;
	if (board->line_count == 262)
	{
		board->line_count = 0;
		poll_keyboard(dev);
		poll_mouse(dev);
	}
}

void set_gm_switches(device_t *board, int number, int value)
{
	if (number==1)
	{
		// Turbo switch. May be changed at any time.
		logerror("Genmod: Setting turbo flag to %d\n", value);
		((genboard_state*)board)->turbo = value;
	}
	else
	{
		// TIMode switch. Causes reset when changed.
		logerror("Genmod: Setting timode flag to %d\n", value);
		((genboard_state*)board)->timode = value;
		board->machine().schedule_hard_reset();
	}
}
/****************************************************************************
    GROM simulation. The Geneve board simulated GROM circuits within its gate
    array.
*****************************************************************************/

/*
    Simulates GROM. The real Geneve does not use GROMs but simulates them
    within the gate array. Unlike with real GROMs, no address wrapping occurs,
    and the complete 64K space is available.
*/
static READ8_DEVICE_HANDLER( read_grom )
{
	genboard_state *board = get_safe_token(device);
	UINT8 reply;
	if (offset & 0x0002)
	{
		// GROM address handling
		board->gromwaddr_LSB = FALSE;

		if (board->gromraddr_LSB)
		{
			reply = board->grom_address & 0xff;
			board->gromraddr_LSB = FALSE;
		}
		else
		{
			reply = (board->grom_address >> 8) & 0xff;
			board->gromraddr_LSB = TRUE;
		}
	}
	else
	{
		// GROM data handling
		// GROMs are stored in pages 38..3f
		int page = 0x38;
		reply = board->dram[page*0x2000 + board->grom_address];
		board->grom_address = (board->grom_address + 1) & 0xffff;
		board->gromraddr_LSB = board->gromwaddr_LSB = FALSE;
	}
	return reply;
}

/*
    Simulates GROM. The real Geneve does not use GROMs but simulates them
    within the gate array.
*/
static WRITE8_DEVICE_HANDLER( write_grom )
{
	genboard_state *board = get_safe_token(device);
	if (offset & 0x0002)
	{
		// set address
		board->gromraddr_LSB = FALSE;
		if (board->gromwaddr_LSB)
		{
			board->grom_address = (board->grom_address & 0xff00) | data;
			board->grom_address = (board->grom_address + 1) & 0xffff;
			board->gromwaddr_LSB = FALSE;
		}
		else
		{
			board->grom_address = (board->grom_address & 0x00ff) | ((UINT16)data<<8);
			board->gromwaddr_LSB = TRUE;
		}
	}
	else
	{	// write GPL data
		// TODO: Check whether available in the real hardware.
		board->grom_address = (board->grom_address + 1) & 0xffff;
		board->gromraddr_LSB = board->gromwaddr_LSB = FALSE;
		logerror("Geneve: GROM write ignored\n");
	}
}

/****************************************************************************
    Mapper
*****************************************************************************/
/*
Geneve mode:
f100 / fff5: v9938_r
f110 / fff8: mapper_r
f118 / ffff: key_r
f130 / fff0: clock_r

TI mode:
8000 / fff8: mapper_r
8008 / ffff: key_r
8010 / fff0: clock_r
8800 / fffd: v9938_r
9000 / ffc0: speech_r
9800 / ffc0: grom_r

*/
READ8_DEVICE_HANDLER( geneve_r )
{
	genboard_state *board = get_safe_token(device);
	UINT8 value = 0;
	int page;

	// Premature access. The CPU reads the start vector before RESET.
	if (board->peribox==NULL) return 0;

	if (!board->zero_wait)
		device_adjust_icount(board->cpu, -4);

	UINT32	physaddr;

	if (board->geneve_mode)
	{
		// TODO: shortcut offset & 0xffc0 = 0xf100
		if ((offset & 0xfff5)==0xf100)
		{
			// video
			// ++++ ++++ ++++ -+-+
			// 1111 0001 0000 0000
			// 1111 0001 0000 0010
			// 1111 0001 0000 1000
			// 1111 0001 0000 1010
			gen_v9938_rz(board->video, offset, &value);
			return value;
		}
		if ((offset & 0xfff8)==0xf110)
		{
			// mapper
			value = board->map[offset & 0x0007];
			return value;
		}
		if (offset == 0xf118)
		{
			// key
			value = get_recent_key(device);
			return value;
		}
		if ((offset & 0xfff0)==0xf130)
		{
			// clock
			value = mm58274c_r(board->clock, offset & 0x00f);
			return value;
		}
	}
	else // TI mode
	{
		if ((offset & 0xfff8)==0x8000)
		{
			// mapper
			value = board->map[offset & 0x0007];
			return value;
		}
		if (offset == 0x8008)
		{
			// key
			value = get_recent_key(device);
			return value;
		}
		if ((offset & 0xfff0)==0x8010)
		{
			// clock
			value = mm58274c_r(board->clock, offset & 0x00f);
			return value;
		}
		if ((offset & 0xfc01)==0x8800)
		{
			// video
			// ++++ ++-- ---- ---+
			// 1000 1000 0000 00x0
			gen_v9938_rz(board->video, offset, &value);
			return value;
		}
		if ((offset & 0xfc01)==0x9000)
		{
			// speech
			// ++++ ++-- ---- ---+
			// 1001 0000 0000 0000
			// We need to add the address prefix bits
			ti99_peb_data_rz(board->peribox, offset | ((board->genmod)? 0x170000 : 0x070000), &value);
			return value;
		}
		if ((offset & 0xfc01)==0x9800)
		{
			// grom simulation
			// ++++ ++-- ---- ---+
			// 1001 1000 0000 00x0
			value = read_grom(device, offset);
			return value;
		}
	}

	page = (offset & 0xe000) >> 13;

	if (board->direct_mode)
	{
		physaddr = 0x1e0000; // points to boot eprom
	}
	else
	{
		if (!board->geneve_mode && page==3)
		{
			// Cartridge paging in TI mode
			// See also cartridge type "paged" in gromport.h
			// value 0x36 = 0 0110 110x xxxx xxxx xxxx (page 1)
			// value 0x37 = 0 0110 111x xxxx xxxx xxxx (page 2)
			// Only use this if there are 2*8 KiB cartridge ROM
			if (!board->cartridge_size_8K && board->cartridge_secondpage) physaddr = 0x06e000;
			else physaddr = 0x06c000;
		}
		else
		{
			physaddr = (board->map[page] << 13);
		}
	}

	physaddr |= (offset & 0x1fff);

	if (!board->genmod)
	{
		// Standard Geneve
		if ((physaddr & 0x180000)==0x000000)
		{
			// DRAM. One wait state.
			device_adjust_icount(board->cpu, -4);
			value = board->dram[physaddr & 0x07ffff];
			return value;
		}

		if ((physaddr & 0x180000)==0x080000)
		{
			// On-board memory expansion for standard Geneve (never used)
			device_adjust_icount(board->cpu, -4);
			return 0;
		}

		if ((physaddr & 0x1e0000)==0x1e0000)
		{
			// 1 111. ..xx xxxx xxxx xxxx on-board eprom (16K)
			// mirrored for f0, f2, f4, ...; f1, f3, f5, ...
			value = board->eprom[physaddr & 0x003fff];
			return value;
		}

		if ((physaddr & 0x180000)==0x180000)
		{
			if ((physaddr & board->sram_mask)==board->sram_val)
			{
				value = board->sram[physaddr & ~board->sram_mask];
			}
			// Return in any case
			return value;
		}

		// Route everything else to the P-Box
		//   0x000000-0x07ffff for the stock Geneve (AMC,AMB,AMA,A0 ...,A15)
		//   0x000000-0x1fffff for the GenMod.(AME,AMD,AMC,AMB,AMA,A0 ...,A15)
		// Add a wait state
		device_adjust_icount(board->cpu, -4);
		physaddr = (physaddr & 0x0007ffff);  // 19 bit address (with AMA..AMC)
		ti99_peb_data_rz(board->peribox, physaddr, &value);
		return value;
	}
	else
	{
		// GenMod mode
		if (!board->turbo)
		{
			device_adjust_icount(board->cpu, -4);
		}
		if ((board->timode) && ((physaddr & 0x180000)==0x000000))
		{
			// DRAM. One wait state.
			value = board->dram[physaddr & 0x07ffff];
			return value;
		}

		if ((physaddr & 0x1e0000)==0x1e0000)
		{
			// 1 111. ..xx xxxx xxxx xxxx on-board eprom (16K)
			// mirrored for f0, f2, f4, ...; f1, f3, f5, ...
			value = board->eprom[physaddr & 0x003fff];
			return value;
		}
		// Route everything else to the P-Box
		physaddr = (physaddr & 0x001fffff);  // 21 bit address for Genmod
		ti99_peb_data_rz(board->peribox, physaddr, &value);
		return value;
	}
}

WRITE8_DEVICE_HANDLER( geneve_w )
{
	genboard_state *board = get_safe_token(device);
	UINT32	physaddr;
	int page;

	if (board->peribox==NULL) return;

	if (!board->zero_wait)
		device_adjust_icount(board->cpu, -4);

	if (board->geneve_mode)
	{
		if ((offset & 0xfff1)==0xf100)
		{
			// video
			// ++++ ++++ ++++ ---+
			// 1111 0001 0000 .cc0
			gen_v9938_w(board->video, offset, data);
			return;
		}
		if ((offset & 0xfff8)==0xf110)
		{
			// mapper
			board->map[offset & 0x0007] = data;
			return;
		}
		if ((offset & 0xfff1)==0xf120)
		{
			// sound
			// ++++ ++++ ++++ ---+
			sn76496_w(board->sound, 0, data);
			return;
		}
		if ((offset & 0xfff0)==0xf130)
		{
			// clock
			// ++++ ++++ ++++ ----
			mm58274c_w(board->clock, offset & 0x00f, data);
			return;
		}
	}
	else
	{
		// TI mode
		if ((offset & 0xfff8)==0x8000)
		{
			// mapper
			board->map[offset & 0x0007] = data;
			return;
		}
		// No key write at 8008
		if ((offset & 0xfff0)==0x8010)
		{
			// clock
			mm58274c_w(board->clock, offset & 0x00f, data);
			return;
		}
		if ((offset & 0xfc01)==0x8400)
		{
			// sound
			// ++++ ++-- ---- ---+
			// 1000 0100 0000 0000
			sn76496_w(board->sound, 0, data);
			return;
		}
		if ((offset & 0xfc01)==0x8c00)
		{
			// video
			// ++++ ++-- ---- ---+
			// 1000 1100 0000 00c0
			gen_v9938_w(board->video, offset, data);
			return;
		}
		if ((offset & 0xfc01)==0x9400)
		{
			// speech
			// ++++ ++-- ---- ---+
			// 1001 0100 0000 0000
			// We need to add the address prefix bits
			ti99_peb_data_w(board->peribox, offset | ((board->genmod)? 0x170000 : 0x070000), data);
			return;
		}
		if ((offset & 0xfc01)==0x9c00)
		{
			// grom simulation
			// ++++ ++-- ---- ---+
			// 1001 1100 0000 00c0
			write_grom(device, offset, data);
			return;
		}
	}

	page = (offset & 0xe000) >> 13;
	if (board->direct_mode)
	{
		physaddr = 0x1e0000; // points to boot eprom
	}
	else
	{
		if (!board->geneve_mode && page==3)
		{
			if (!board->cartridge_size_8K)
			{
				// Writing to 0x6000 selects page 1,
				// writing to 0x6002 selects page 2
				board->cartridge_secondpage = ((offset & 0x0002)!=0);
				return;
			}
			else
			{
				// writing into cartridge rom space (no bankswitching)
				if ((((offset & 0x1000)==0x0000) && !board->cartridge6_writable)
					|| (((offset & 0x1000)==0x1000) && !board->cartridge7_writable))
				{
					logerror("Geneve: Writing to protected cartridge space %04x ignored\n", offset);
					return;
				}
				else
					// TODO: Check whether secondpage is really ignored
					physaddr = 0x06c000;
			}
		}
		else
			physaddr = (board->map[page] << 13);
	}

	physaddr |= offset & 0x1fff;

	if (!board->genmod)
	{
		if ((physaddr & 0x180000)==0x000000)
		{
			// DRAM write. One wait state. (only for normal Geneve)
			device_adjust_icount(board->cpu, -4);
			board->dram[physaddr & 0x07ffff] = data;
			return;
		}

		if ((physaddr & 0x180000)==0x080000)
		{
			// On-board memory expansion for standard Geneve (never used)
			return;
		}

		if ((physaddr & 0x1e0000)==0x1e0000)
		{
			// 1 111. ..xx xxxx xxxx xxxx on-board eprom (16K)
			// mirrored for f0, f2, f4, ...; f1, f3, f5, ...
			// Ignore EPROM write
			return;
		}

		if ((physaddr & 0x180000)==0x180000)
		{
			if ((physaddr & board->sram_mask)==board->sram_val)
			{
				board->sram[physaddr & ~board->sram_mask] = data;
			}
			// Return in any case
			return;
		}
		// Route everything else to the P-Box
		// Add a wait state
		device_adjust_icount(board->cpu, -4);

		// only AMA, AMB, AMC are used; AMD and AME are not used
		physaddr = (physaddr & 0x0007ffff);  // 19 bit address
		ti99_peb_data_w(board->peribox, physaddr, data);
	}
	else
	{
		// GenMod mode
		if (!board->turbo)
		{
			device_adjust_icount(board->cpu, -4);
		}
		if ((board->timode) && ((physaddr & 0x180000)==0x000000))
		{
			// DRAM. One wait state.
			board->dram[physaddr & 0x07ffff] = data;
			return;
		}

		if ((physaddr & 0x1e0000)==0x1e0000)
		{
			// 1 111. ..xx xxxx xxxx xxxx on-board eprom (16K)
			// mirrored for f0, f2, f4, ...; f1, f3, f5, ...
			// Ignore EPROM write
			return;
		}
		// Route everything else to the P-Box
		physaddr = (physaddr & 0x001fffff);  // 21 bit address for Genmod
		ti99_peb_data_w(board->peribox, physaddr, data);
	}
}

/*
Geneve mode:
f100 / fff1: v9938_w
f110 / fff8: mapper_w
f120 / fff0: sound_w
f130 / fff0: clock_w

TI mode:
8000 / fff8: mapper_w
8010 / fff0: clock_w
8400 / fc00: sound_w
8c00 / fff9: v9938_w
9400 / fc00: speech_w
9c00 / fffd: grom_w

*/

/****************************************************************************
    CRU handling
*****************************************************************************/

#define CRU_CONTROL_BASE 0x1ee0

WRITE8_DEVICE_HANDLER ( geneve_cru_w )
{
	genboard_state *board = get_safe_token(device);

	int addroff = (offset + 0x0800) << 1;
	int rising_edge = FALSE;
	int falling_edge = FALSE;

	if ((addroff & 0x1fe0) == CRU_CONTROL_BASE)
	{
		int bit = (addroff & 0x0001e)>>1;
		switch (bit)
		{
		case 5:
			board->palvideo = data;
			break;
		case 7:
			board->capslock = data;
			break;
		case 8:
			rising_edge = (!board->keyboard_clock && data);
			board->keyboard_clock = data;
			if (rising_edge)
				read_key_if_possible(board);
			break;
		case 9:
			rising_edge = (!board->keep_keybuf && data);
			falling_edge = (board->keep_keybuf && !data);
			board->keep_keybuf = data;

			if (rising_edge)
				read_key_if_possible(board);
			else
			{
				if (falling_edge)
				{
					if (board->keyQueueLen != 0)
					{
						board->keyQueueHead = (board->keyQueueHead + 1) % KEYQUEUESIZE;
						board->keyQueueLen--;
					}
					/* clear keyboard interrupt */
					tms9901_set_single_int(board->tms9901, 8, 0);
					board->keyInBuf = 0;
				}
			}
			break;
		case 10:
			board->geneve_mode = data;
			break;
		case 11:
			board->direct_mode = data;
			break;
		case 12:
			board->cartridge_size_8K = data;
			break;
		case 13:
			board->cartridge6_writable = data;
			break;
		case 14:
			board->cartridge7_writable = data;
			break;
		case 15:
			board->zero_wait = data;
			break;
		default:
			logerror("geneve: set CRU address %04x=%02x ignored\n", addroff, data);
			break;
		}
	}
	else
	{
		ti99_peb_cru_w(board->peribox, addroff, data);
	}
}


READ8_DEVICE_HANDLER ( geneve_cru_r )
{
	UINT8 value = 0;
	genboard_state *board = get_safe_token(device);
	int addroff = (offset + 0x0100) << 4;

	// TMS9995-internal CRU locations (1ee0-1efe) are handled within the 99xxcore
	// implementation (read_single_cru), so we don't arrive here
	ti99_peb_cru_rz(board->peribox, addroff, &value);
	return value;
}

/****************************************************************************
    Keyboard support
*****************************************************************************/

UINT8 get_recent_key(device_t *device)
{
	genboard_state *board = get_safe_token(device);
	if (board->keyInBuf)
		return board->keyQueue[board->keyQueueHead];
	else
		return 0;
}

static void read_key_if_possible(genboard_state *board)
{
	// if keyboard reset is not asserted, and key clock is enabled, and key
	// buffer clear is disabled, and key queue is not empty. */
	if ((!board->keyReset) && (board->keyboard_clock) && (board->keep_keybuf) && (board->keyQueueLen != 0))
	{
		tms9901_set_single_int(board->tms9901, 8, 1);
		board->keyInBuf = 1;
	}
}

INLINE void post_in_keyQueue(genboard_state *board, int keycode)
{
	board->keyQueue[(board->keyQueueHead + board->keyQueueLen) % KEYQUEUESIZE] = keycode;
	board->keyQueueLen++;
}

static void poll_keyboard(device_t *device)
{
	genboard_state *board = get_safe_token(device);

	UINT32 keystate;
	UINT32 key_transitions;
	int i, j;
	int keycode;
	int pressed;

	if (board->keyReset)
		return;

	/* Poll keyboard */
	for (i = 0; (i < 4) && (board->keyQueueLen <= (KEYQUEUESIZE-MAXKEYMSGLENGTH)); i++)
	{
		keystate = input_port_read(device->machine(), keynames[2*i]) | (input_port_read(device->machine(), keynames[2*i + 1]) << 16);
		key_transitions = keystate ^ board->keyStateSave[i];
		if (key_transitions)
		{
			for (j = 0; (j < 32) && (board->keyQueueLen <= (KEYQUEUESIZE-MAXKEYMSGLENGTH)); j++)
			{
				if ((key_transitions >> j) & 1)
				{
					keycode = (i << 5) | j;
					pressed = ((keystate >> j) & 1);
					if (pressed)
						board->keyStateSave[i] |= (1 << j);
					else
						board->keyStateSave[i] &= ~ (1 << j);

					/* Update auto-repeat */
					if (pressed)
					{
						board->keyAutoRepeatKey = keycode;
						board->keyAutoRepeatTimer = KEYAUTOREPEATDELAY+1;
					}
					else /*if (keycode == board->keyAutoRepeatKey)*/
						board->keyAutoRepeatKey = 0;

					// Release Fake Shift/Unshift if another key is pressed
					// We do so if a key is released, though it is actually
					// required only if it is a modifier key
					/*if (pressed)*/
					//{
					if (board->keyFakeShiftState)
					{
						/* Fake shift release */
						post_in_keyQueue(board, 0xe0);
						post_in_keyQueue(board, 0xaa);
						board->keyFakeShiftState = 0;
					}
					if (board->keyFakeUnshiftState)
					{
						/* Fake shift press */
						post_in_keyQueue(board, 0xe0);
						post_in_keyQueue(board, 0x2a);
						board->keyFakeUnshiftState = 0;
					}
					//}

					/* update shift and numlock state */
					if ((keycode == 0x2a) || (keycode == 0x36))
						board->keyRealShiftState = board->keyRealShiftState + (pressed ? +1 : -1);
					if ((keycode == 0x1d) || (keycode == 0x61))
						board->keyCtrlState = board->keyCtrlState + (pressed ? +1 : -1);
					if ((keycode == 0x38) || (keycode == 0x62))
						board->keyAltState = board->keyAltState + (pressed ? +1 : -1);
					if ((keycode == 0x45) && pressed)
						board->keyNumLockState = ! board->keyNumLockState;

					if ((keycode >= 0x60) && (keycode < 0x6e))
					{	/* simpler extended keys */
						/* these keys are emulated */

						if ((keycode >= 0x63) && pressed)
						{
							/* Handle shift state */
							if (keycode == 0x63)
							{	/* non-shifted key */
								if (board->keyRealShiftState)
									/* Fake shift unpress */
									board->keyFakeUnshiftState = 1;
							}
							else /*if (keycode >= 0x64)*/
							{	/* non-numlock mode key */
								if (board->keyNumLockState & ! board->keyRealShiftState)
									/* Fake shift press if numlock is active */
									board->keyFakeShiftState = 1;
								else if ((! board->keyNumLockState) & board->keyRealShiftState)
									/* Fake shift unpress if shift is down */
									board->keyFakeUnshiftState = 1;
							}

							if (board->keyFakeShiftState)
							{
								post_in_keyQueue(board, 0xe0);
								post_in_keyQueue(board, 0x2a);
							}

							if (board->keyFakeUnshiftState)
							{
								post_in_keyQueue(board, 0xe0);
								post_in_keyQueue(board, 0xaa);
							}
						}

						keycode = keyboard_mf1_code[keycode-0x60];
						if (! pressed)
							keycode |= 0x80;
						post_in_keyQueue(board, 0xe0);
						post_in_keyQueue(board, keycode);
					}
					else if (keycode == 0x6e)
					{	/* emulate Print Screen / System Request (F13) key */
						/* this is a bit complex, as Alt+PrtScr -> SysRq */
						/* Additionally, Ctrl+PrtScr involves no fake shift press */
						if (board->keyAltState)
						{
							/* SysRq */
							keycode = 0x54;
							if (! pressed)
								keycode |= 0x80;
							post_in_keyQueue(board, keycode);
						}
						else
						{
							/* Handle shift state */
							if (pressed && (! board->keyRealShiftState) && (! board->keyCtrlState))
							{	/* Fake shift press */
								post_in_keyQueue(board, 0xe0);
								post_in_keyQueue(board, 0x2a);
								board->keyFakeShiftState = 1;
							}

							keycode = 0x37;
							if (! pressed)
								keycode |= 0x80;
							post_in_keyQueue(board, 0xe0);
							post_in_keyQueue(board, keycode);
						}
					}
					else if (keycode == 0x6f)
					{	// emulate pause (F15) key
						// this is a bit complex, as Pause -> Ctrl+NumLock and
						// Ctrl+Pause -> Ctrl+ScrLock.  Furthermore, there is no
						// repeat or release.
						if (pressed)
						{
							if (board->keyCtrlState)
							{
								post_in_keyQueue(board, 0xe0);
								post_in_keyQueue(board, 0x46);
								post_in_keyQueue(board, 0xe0);
								post_in_keyQueue(board, 0xc6);
							}
							else
							{
								post_in_keyQueue(board, 0xe1);
								post_in_keyQueue(board, 0x1d);
								post_in_keyQueue(board, 0x45);
								post_in_keyQueue(board, 0xe1);
								post_in_keyQueue(board, 0x9d);
								post_in_keyQueue(board, 0xc5);
							}
						}
					}
					else
					{
						if (! pressed)
							keycode |= 0x80;
						post_in_keyQueue(board, keycode);
					}
					read_key_if_possible(board);
				}
			}
		}
	}

	/* Handle auto-repeat */
	if ((board->keyQueueLen <= (KEYQUEUESIZE-MAXKEYMSGLENGTH)) && board->keyAutoRepeatKey && (--board->keyAutoRepeatTimer == 0))
	{
		if ((board->keyAutoRepeatKey >= 0x60) && (board->keyAutoRepeatKey < 0x6e))
		{
			post_in_keyQueue(board, 0xe0);
			post_in_keyQueue(board, keyboard_mf1_code[board->keyAutoRepeatKey-0x60]);
		}
		else if (board->keyAutoRepeatKey == 0x6e)
		{
			if (board->keyAltState)
				post_in_keyQueue(board, 0x54);
			else
			{
				post_in_keyQueue(board, 0xe0);
				post_in_keyQueue(board, 0x37);
			}
		}
		else if (board->keyAutoRepeatKey == 0x6f)
			;
		else
		{
			post_in_keyQueue(board, board->keyAutoRepeatKey);
		}
		read_key_if_possible(board);
		board->keyAutoRepeatTimer = KEYAUTOREPEATRATE;
	}
}

/****************************************************************************
    Mouse support
****************************************************************************/

static void poll_mouse(device_t *device)
{
	genboard_state *board = get_safe_token(device);

	int new_mx, new_my;
	int delta_x, delta_y, buttons;

	buttons = input_port_read(device->machine(), "MOUSE0");
	new_mx = input_port_read(device->machine(), "MOUSEX");
	new_my = input_port_read(device->machine(), "MOUSEY");

	/* compute x delta */
	delta_x = new_mx - board->last_mx;

	/* check for wrap */
	if (delta_x > 0x80)
		delta_x = 0x100-delta_x;
	if  (delta_x < -0x80)
		delta_x = -0x100-delta_x;

	board->last_mx = new_mx;

	/* compute y delta */
	delta_y = new_my - board->last_my;

	/* check for wrap */
	if (delta_y > 0x80)
		delta_y = 0x100-delta_y;
	if  (delta_y < -0x80)
		delta_y = -0x100-delta_y;

	board->last_my = new_my;

	video_update_mouse(board->video, delta_x, delta_y, buttons & 3);
}

/****************************************************************************
    TMS9901 and attached devices handling
****************************************************************************/

/*
    Called by the 9901 core whenever the state of INTREQ and IC0-3 changes
*/
static TMS9901_INT_CALLBACK( tms9901_interrupt_callback )
{
	/* INTREQ is connected to INT1 (IC0-3 are not connected) */
	cputag_set_input_line(device->machine(), "maincpu", 0, intreq? ASSERT_LINE : CLEAR_LINE);
}

/*
    Read pins INT3*-INT7* of Geneve 9901.

    signification:
     (bit 1: INT1 status)
     (bit 2: INT2 status)
     bit 3-7: joystick status
*/
static READ8_DEVICE_HANDLER( R9901_0 )
{
	device_t *dev = device->machine().device("geneve_board");
	genboard_state *board = get_safe_token(dev);
	int answer;
	answer = input_port_read(device->machine(), "JOY") >> (board->joySel * 8);
	return answer;
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
static READ8_DEVICE_HANDLER( R9901_1 )
{
	int answer;
	answer = (input_port_read(device->machine(), "MOUSE0") & 4) ^ 4;
	return answer;
}

/*
    Read pins P0-P7 of Geneve 9901.
*/
static READ8_DEVICE_HANDLER( R9901_2 )
{
	return 0;
}

/*
    Read pins P8-P15 of Geneve 9901.
    bit 4: mouse right button
*/
static READ8_DEVICE_HANDLER( R9901_3 )
{
	int answer = 0;

	if (! (input_port_read(device->machine(), "MOUSE0") & 4))
		answer |= 0x10;

	return answer;
}

/*
    Write PE bus reset line
*/
static WRITE8_DEVICE_HANDLER( W9901_PE_bus_reset )
{
}

/*
    Write VDP reset line
*/
static WRITE8_DEVICE_HANDLER( W9901_VDP_reset )
{
}

/*
    Write joystick select line
*/
static WRITE8_DEVICE_HANDLER( W9901_joySel )
{
	device_t *dev = device->machine().device("geneve_board");
	genboard_state *board = get_safe_token(dev);
	board->joySel = data;
}

static WRITE8_DEVICE_HANDLER( W9901_keyboardReset )
{
	device_t *dev = device->machine().device("geneve_board");
	genboard_state *board = get_safe_token(dev);

	board->keyReset = !data;

	if (board->keyReset)
	{
		/* reset -> clear keyboard key queue, but not geneve key buffer */
		board->keyQueueLen = board->keyInBuf ? 1 : 0;
		memset(board->keyStateSave, 0, sizeof(board->keyStateSave));
		board->keyNumLockState = 0;
		board->keyCtrlState = 0;
		board->keyAltState = 0;
		board->keyRealShiftState = 0;
		board->keyFakeShiftState = 0;
		board->keyFakeUnshiftState = 0;
		board->keyAutoRepeatKey = 0;
	}
	/*else
        poll_keyboard(space->machine());*/
}

/*
    Write external mem cycles (0=long, 1=short)
*/
static WRITE8_DEVICE_HANDLER( W9901_ext_mem_wait_states )
{
}

/*
    Write vdp wait cycles (1=add 15 cycles, 0=add none)
*/
static WRITE8_DEVICE_HANDLER( W9901_VDP_wait_states )
{
}

/* tms9901 setup */
const tms9901_interface tms9901_wiring_geneve =
{
	TMS9901_INT1 | TMS9901_INT2 | TMS9901_INT8 | TMS9901_INTB | TMS9901_INTC,	/* only input pins whose state is always known */

	{	/* read handlers */
		R9901_0,					// INTA, VDPint, Joystick
		R9901_1,					// Keyb, Mouse, Clock, INTB
		R9901_2,					// -
		R9901_3						// Mouse right button
	},

	{	/* write handlers */
		W9901_PE_bus_reset,
		W9901_VDP_reset,
		W9901_joySel,
		NULL,
		NULL,
		NULL,
		W9901_keyboardReset,
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

/*
    inta is connected to both tms9901 IRQ1 line and to tms9995 INT4/EC line.
*/
WRITE_LINE_DEVICE_HANDLER( board_inta )
{
	tms9901_set_single_int(device->machine().device("tms9901"), 1, state);
	cputag_set_input_line(device->machine(), "maincpu", 1, state ? ASSERT_LINE : CLEAR_LINE);
}

/*
    intb is connected to tms9901 IRQ12 line.
*/
WRITE_LINE_DEVICE_HANDLER( board_intb )
{
	tms9901_set_single_int(device->machine().device("tms9901"), 12, state);

}

WRITE_LINE_DEVICE_HANDLER( board_ready )
{
	logerror("READY line set ... not yet connected, level=%02x\n", state);
}

/*
    set the state of int2 (called by the v9938 core)
*/
void tms9901_gen_set_int2(running_machine &machine, int state)
{
	tms9901_set_single_int(machine.device("tms9901"), 2, state);
}

/***************************************************************************
    DEVICE LIFECYCLE FUNCTIONS
***************************************************************************/

static const mm58274c_interface geneve_mm58274c_interface =
{
	1,	/*  mode 24*/
	0   /*  first day of week */
};

static MACHINE_CONFIG_FRAGMENT( genboard )
	MCFG_MM58274C_ADD("mm58274c", geneve_mm58274c_interface)
MACHINE_CONFIG_END

static DEVICE_START( genboard )
{
	logerror("Starting Geneve board\n");
	genboard_state *board = get_safe_token(device);

	board->video = device->siblingdevice("video");
	board->tms9901 = device->siblingdevice("tms9901");
	board->peribox = device->siblingdevice("peribox");
	board->cpu = device->siblingdevice("maincpu");
	board->sound = device->siblingdevice("soundgen");

	board->clock = device->subdevice("mm58274c");

	board->sram = (UINT8*)malloc(SRAM_SIZE);
	board->dram = (UINT8*)malloc(DRAM_SIZE);
	board->eprom = device->machine().region("maincpu")->base();
	assert(board->video && board->tms9901);
	assert(board->peribox && board->cpu);
	assert(board->sound && board->clock);
	assert(board->eprom);
	board->geneve_mode = 0;
	board->direct_mode = 1;

	memset(board->sram, 0x00, SRAM_SIZE);
	memset(board->dram, 0x00, DRAM_SIZE);
}

static DEVICE_STOP( genboard )
{
	logerror("Stopping Geneve board\n");
	genboard_state *board = get_safe_token(device);
	free(board->sram);
	free(board->dram);
}

static DEVICE_RESET( genboard )
{
	genboard_state *board = get_safe_token(device);
	logerror("Resetting Geneve board\n");
	board->joySel = 0;
	board->keyQueueHead = board->keyQueueLen = 0;
	memset(board->keyStateSave, 0, sizeof(board->keyStateSave));
	board->keyNumLockState = 0;
	board->keyCtrlState = 0;
	board->keyAltState = 0;
	board->keyRealShiftState = 0;
	board->keyFakeShiftState = 0;
	board->keyFakeUnshiftState = 0;
	board->keyAutoRepeatKey = 0;
	board->keyInBuf = 0;
	board->keyReset = 1;
	board->last_mx = 0;
	board->last_my = 0;
	board->line_count = 0;

	board->palvideo = 0;
	board->capslock = 0;
	board->keyboard_clock = 0;
	board->keep_keybuf = 0;
	board->zero_wait = 1;

	board->geneve_mode = 0;
	board->direct_mode = 1;
	board->cartridge_size_8K = 0;
	board->cartridge_secondpage = 0;
	board->cartridge6_writable = 0;
	board->cartridge7_writable = 0;
	for (int i=0; i < 8; i++) board->map[i] = 0;

	board->genmod = FALSE;
	if (input_port_read(device->machine(), "MODE")==0)
	{
		switch (input_port_read(device->machine(), "BOOTROM"))
		{
		case GENEVE_098:
			logerror("Geneve 9640: Using 0.98 boot eprom\n");
			board->eprom = device->machine().region("maincpu")->base() + 0x4000;
			break;
		case GENEVE_100:
			logerror("Geneve 9640: Using 1.00 boot eprom\n");
			board->eprom = device->machine().region("maincpu")->base();
			break;
		}
	}
	else
	{
		logerror("Geneve 9640: Using GenMod modification\n");
		board->eprom = device->machine().region("maincpu")->base() + 0x8000;
		if (board->eprom[0] != 0xf0)
		{
			fatalerror("GenMod boot rom missing.\n");
		}
		board->genmod = TRUE;
		board->turbo = input_port_read(device->machine(), "GENMODDIPS") & GM_TURBO;
		board->timode = input_port_read(device->machine(), "GENMODDIPS") & GM_TIM;
	}

	switch (input_port_read(device->machine(), "SRAM"))
	{
/*  1 100. .... .... .... .... on-board sram (128K) -+
    1 101. .... .... .... .... on-board sram (128K) -+-- maximum SRAM expansion
    1 1100 .... .... .... .... on-board sram (64K) --+
    1 1101 0... .... .... .... on-board sram (32K) - additional 32 KiB required for MDOS 2.50s and higher
    1 1101 1... .... .... .... on-board sram (32K) - standard setup
*/
	case 0: // 32 KiB
		board->sram_mask =	0x1f8000;
		board->sram_val =	0x1d8000;
		break;
	case 1: // 64 KiB
		board->sram_mask =	0x1f0000;
		board->sram_val =	0x1d0000;
		break;
	case 2: // 384 KiB (actually 512 KiB, but the EPROM masks the upper 128 KiB)
		board->sram_mask =	0x180000;
		board->sram_val =   0x180000;
		break;
	}
}

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##genboard##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET | DT_HAS_MACHINE_CONFIG
#define DEVTEMPLATE_NAME                "Geneve system board"
#define DEVTEMPLATE_FAMILY              "Internal component"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( GENBOARD, genboard );
