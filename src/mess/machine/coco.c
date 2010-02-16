/***************************************************************************

  machine/coco.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

  References:
        There are two main references for the info for this driver
        - Tandy Color Computer Unravelled Series
                    (http://www.giftmarket.org/unravelled/unravelled.shtml)
        - Assembly Language Programming For the CoCo 3 by Laurence A. Tepolt
        - Kevin K. Darlings GIME reference
                    (http://www.cris.com/~Alxevans/gime.txt)
        - Sock Masters's GIME register reference
                    (http://www.axess.com/twilight/sock/gime.html)
        - Robert Gault's FAQ
                    (http://home.att.net/~robert.gault/Coco/FAQ/FAQ_main.htm)
        - Discussions with L. Curtis Boyle (LCB) and John Kowalski (JK)

  TODO:
        - Implement unimplemented SAM registers
        - Implement unimplemented interrupts (serial)
        - Choose and implement more appropriate ratios for the speed up poke
        - Handle resets correctly

  In the CoCo, all timings should be exactly relative to each other.  This
  table shows how all clocks are relative to each other (info: JK):
        - Main CPU Clock                0.89 MHz
        - Horizontal Sync Interrupt     15.7 kHz/63.5us (57 clock cycles)
        - Vertical Sync Interrupt       60 Hz           (14934 clock cycles)
        - Composite Video Color Carrier 3.58 MHz/279ns  (1/4 clock cycles)

  It is also noting that the CoCo 3 had two sets of VSync interrupts.  To quote
  John Kowalski:

    One other thing to mention is that the old vertical interrupt and the new
    vertical interrupt are not the same..  The old one is triggered by the
    video's vertical sync pulse, but the new one is triggered on the next scan
    line *after* the last scan line of the active video display.  That is : new
    vertical interrupt triggers somewheres around scan line 230 of the 262 line
    screen (if a 200 line graphics mode is used, a bit earlier if a 192 line
    mode is used and a bit later if a 225 line mode is used).  The old vsync
    interrupt triggers on scanline zero.

    230 is just an estimate [(262-200)/2+200].  I don't think the active part
    of the screen is exactly centered within the 262 line total.  I can
    research that for you if you want an exact number for scanlines before the
    screen starts and the scanline that the v-interrupt triggers..etc.

Dragon Alpha code added 21-Oct-2004,
            Phill Harvey-Smith (afra@aurigae.demon.co.uk)

            Added AY-8912 and FDC code 30-Oct-2004.

Fixed Dragon Alpha NMI enable/disable, following circuit traces on a real machine.
    P.Harvey-Smith, 11-Aug-2005.

Re-implemented Alpha NMI enable/disable, using direct PIA reads, rather than
keeping track of it in a variable in the driver.
    P.Harvey-Smith, 25-Sep-2006.

Radically re-wrote memory emulation code for CoCo 1/2 & Dragon machines, the
new code emulates the memory mapping of the SAM, dependent on what size of
RAM chips it is programed to use, including proper mirroring of the RAM.

Replaced the kludged emulation of the cart line, with a timer based trigger
this is set to toggle at 1Hz, this seems to be good enough to trigger the
cartline, but is so slow in real terms that it should have very little
impact on the emulation speed.

Re-factored the code common to all machines, and seperated the code different,
into callbacks/functions unique to the machines, in preperation for splitting
the code for individual machine types into seperate files, I have preposed, that
the CoCo 1/2 should stay in coco.c, and that the coco3 and dragon specifc code
should go into coco3.c and dragon.c which should (hopefully) make the code
easier to manage.
    P.Harvey-Smith, Dec 2006-Feb 2007
***************************************************************************/

#include <math.h>
#include <assert.h>

#include "emu.h"
#include "debug/debugcpu.h"
#include "cpu/m6809/m6809.h"
#include "machine/6821pia.h"
#include "includes/coco.h"
#include "includes/cococart.h"
#include "machine/6883sam.h"
#include "machine/6551.h"
#include "video/coco6847.h"
#include "formats/cocopak.h"
#include "devices/bitbngr.h"
#include "devices/printer.h"
#include "devices/cassette.h"
#include "machine/wd17xx.h"
#include "sound/dac.h"
#include "sound/ay8910.h"
#include "devices/cococart.h"
#include "devices/cartslot.h"
#include "devices/flopdrv.h"
#include "devices/messram.h"
#include "crsshair.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

/* this MUX delay was introduced to fix bug #655, but the delay was originally
 * 16us.  This was reduced to 8us to fix bug #1608 */
#define JOYSTICK_MUX_DELAY				ATTOTIME_IN_USEC(8)



/***************************************************************************
    LOCAL VARIABLES / PROTOTYPES
***************************************************************************/

/*common vars/calls */
static UINT8 *coco_rom;
static int dclg_state, dclg_output_h, dclg_output_v, dclg_timer;
static UINT8 (*update_keyboard)(running_machine *machine);
static emu_timer *update_keyboard_timer;
static emu_timer *mux_sel1_timer;
static emu_timer *mux_sel2_timer;
static UINT8 mux_sel1, mux_sel2;

static WRITE8_DEVICE_HANDLER ( d_pia1_pb_w );
static WRITE8_DEVICE_HANDLER ( d_pia1_pa_w );
static READ8_DEVICE_HANDLER ( d_pia1_pa_r );
static WRITE8_DEVICE_HANDLER ( d_pia0_pa_w );
static WRITE8_DEVICE_HANDLER ( d_pia0_pb_w );
static WRITE8_DEVICE_HANDLER ( d_pia1_cb2_w);
static WRITE8_DEVICE_HANDLER ( d_pia0_cb2_w);
static WRITE8_DEVICE_HANDLER ( d_pia1_ca2_w);
static WRITE8_DEVICE_HANDLER ( d_pia0_ca2_w);
static WRITE_LINE_DEVICE_HANDLER( d_pia0_irq_a );
static WRITE_LINE_DEVICE_HANDLER( d_pia0_irq_b );
static WRITE_LINE_DEVICE_HANDLER( d_pia1_firq_a );
static WRITE_LINE_DEVICE_HANDLER( d_pia1_firq_b );
static SAM6883_SET_PAGE_ONE_MODE( d_sam_set_pageonemode );
static SAM6883_SET_MPU_RATE( d_sam_set_mpurate );
static SAM6883_SET_MEMORY_SIZE( d_sam_set_memorysize);
static SAM6883_SET_MAP_TYPE( d_sam_set_maptype );

/* CoCo 1 specific */
static READ8_DEVICE_HANDLER ( d_pia1_pb_r_coco );

/* CoCo 2 specific */
static READ8_DEVICE_HANDLER ( d_pia1_pb_r_coco2 );

/* CoCo 3 specific */
UINT8 coco3_gimereg[16];
static int coco3_enable_64k;
static UINT8 coco3_mmu[16];
static UINT8 coco3_interupt_line;
static UINT8 gime_firq, gime_irq;

static WRITE_LINE_DEVICE_HANDLER( coco3_pia0_irq_a );
static WRITE_LINE_DEVICE_HANDLER( coco3_pia0_irq_b );
static WRITE_LINE_DEVICE_HANDLER( coco3_pia1_firq_a );
static WRITE_LINE_DEVICE_HANDLER( coco3_pia1_firq_b );

static SAM6883_SET_MAP_TYPE( coco3_sam_set_maptype );
static SAM6883_GET_RAMBASE( coco3_sam_get_rambase );

/* Dragon 32 specific */
static READ8_DEVICE_HANDLER ( d_pia1_pb_r_dragon32 );

/* Dragon 64 / 64 Plus / Tano (Alpha) specific */
static WRITE8_DEVICE_HANDLER ( dragon64_pia1_pb_w );

/* Dragon 64 / Alpha shared */
static void dragon_page_rom(running_machine *machine, int romswitch);

/* Dragon Alpha specific */
static WRITE8_DEVICE_HANDLER ( dgnalpha_pia2_pa_w );
static WRITE_LINE_DEVICE_HANDLER( d_pia2_firq_a );
static WRITE_LINE_DEVICE_HANDLER( d_pia2_firq_b );
static int dgnalpha_just_reset;		/* Reset flag used to ignore first NMI after reset */

/* Dragon Plus specific */
static int dragon_plus_reg;			/* Dragon plus control reg */

/* Memory map related CoCo 1/2 and Dragons only */
static UINT8 *bas_rom_bank;			/* Dragon 64 / Alpha rom bank in use, basic rom bank on coco */
static UINT8 *bottom_32k;			/* ram to be mapped into bottom 32K */
static void setup_memory_map(running_machine *machine);


/* These sets of defines control logging.  When MAME_DEBUG is off, all logging
 * is off.  There is a different set of defines for when MAME_DEBUG is on so I
 * don't have to worry abount accidently committing a version of the driver
 * with logging enabled after doing some debugging work
 *
 * Logging options marked as "Sparse" are logging options that happen rarely,
 * and should not get in the way.  As such, these options are always on
 * (assuming MAME_DEBUG is on).  "Frequent" options are options that happen
 * enough that they might get in the way.
 */
#define LOG_PAK			0	/* [Sparse]   Logging on PAK trailers */
#define LOG_INT_MASKING	1	/* [Sparse]   Logging on changing GIME interrupt masks */
#define LOG_INT_COCO3	0
#define LOG_GIME		0
#define LOG_MMU			0
#define LOG_TIMER       0
#define LOG_CART	1		/* Log cart type selections is_dosrom and friends */

#define GIME_TYPE_1987	0

static CPU_DISASSEMBLE(coco_dasm_override);


/* ----------------------------------------------------------------------- */
/* The bordertop and borderbottom return values are used for calculating
 * field sync timing.  Unfortunately, I cannot seem to find an agreement
 * about how exactly field sync works..
 *
 * What I do know is that FS goes high at the top of the screen, and goes
 * low (forcing an VBORD interrupt) at the bottom of the visual area.
 *
 * Unfortunately, I cannot get a straight answer about how many rows each
 * of the three regions (leading edge --> visible top; visible top -->
 * visible bottom/trailing edge; visible bottom/trailing edge --> leading
 * edge) takes up.  Adding the fact that each of the different LPR
 * settings most likely has a different set of values.  Here is a summary
 * of what I know from different sources:
 *
 * In the January 1987 issue of Rainbow Magazine, there is a program called
 * COLOR3 that uses midframe palette rotation to show all 64 colors on the
 * screen at once.  The first box is at line 32, but it waits for 70 HSYNC
 * transitions before changing
 *
 * SockMaster email: 43/192/28, 41/199/23, 132/0/131, 26/225/12
 * m6847 reference:  38/192/32
 * COLOR3            38/192/32
 *
 * Notes of interest:  Below are some observations of key programs and what
 * they rely on:
 *
 *      COLOR3:
 *           (fs_pia_flip ? fall_scanline : rise_scanline) = border_top - 32
 */

const struct coco3_video_vars coco3_vidvars =
{
	/* border tops */
	38,	36, 132, 25,

	/* gime flip (hs/fs) */
	0, 0,

	/* pia flip (hs/fs) */
	1, 1,

	/* rise scanline & fall scanline */
	258, 6
};



/* ----------------------------------------------------------------------- */

/* PIA 0 */
const pia6821_interface coco_pia_intf_0 =
{
	/*inputs : A/B,CA/B1,CA/B2 */
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	/*outputs: A/B,CA/B2       */
	DEVCB_HANDLER(d_pia0_pa_w),
	DEVCB_HANDLER(d_pia0_pb_w),
	DEVCB_HANDLER(d_pia0_ca2_w),
	DEVCB_HANDLER(d_pia0_cb2_w),
	/*irqs   : A/B             */
	DEVCB_LINE(d_pia0_irq_a),
	DEVCB_LINE(d_pia0_irq_b)
};

/* PIA 1 */
const pia6821_interface coco_pia_intf_1 =
{
	/*inputs : A/B,CA/B1,CA/B2 */
	DEVCB_HANDLER(d_pia1_pa_r),
	DEVCB_HANDLER(d_pia1_pb_r_coco),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	/*outputs: A/B,CA/B2       */
	DEVCB_HANDLER(d_pia1_pa_w),
	DEVCB_HANDLER(d_pia1_pb_w),
	DEVCB_HANDLER(d_pia1_ca2_w),
	DEVCB_HANDLER(d_pia1_cb2_w),
	/*irqs   : A/B             */
	DEVCB_LINE(d_pia1_firq_a),
	DEVCB_LINE(d_pia1_firq_b)
};

/* PIA 0 */
const pia6821_interface coco2_pia_intf_0 =
{
	/*inputs : A/B,CA/B1,CA/B2 */
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	/*outputs: A/B,CA/B2       */
	DEVCB_HANDLER(d_pia0_pa_w),
	DEVCB_HANDLER(d_pia0_pb_w),
	DEVCB_HANDLER(d_pia0_ca2_w),
	DEVCB_HANDLER(d_pia0_cb2_w),
	/*irqs   : A/B             */
	DEVCB_LINE(d_pia0_irq_a),
	DEVCB_LINE(d_pia0_irq_b)
};

/* PIA 1 */
const pia6821_interface coco2_pia_intf_1 =
{
	/*inputs : A/B,CA/B1,CA/B2 */
	DEVCB_HANDLER(d_pia1_pa_r),
	DEVCB_HANDLER(d_pia1_pb_r_coco2),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	/*outputs: A/B,CA/B2       */
	DEVCB_HANDLER(d_pia1_pa_w),
	DEVCB_HANDLER(d_pia1_pb_w),
	DEVCB_HANDLER(d_pia1_ca2_w),
	DEVCB_HANDLER(d_pia1_cb2_w),
	/*irqs   : A/B             */
	DEVCB_LINE(d_pia1_firq_a),
	DEVCB_LINE(d_pia1_firq_b)
};

/* PIA 0 */
const pia6821_interface coco3_pia_intf_0 =
{
	/*inputs : A/B,CA/B1,CA/B2 */
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	/*outputs: A/B,CA/B2       */
	DEVCB_HANDLER(d_pia0_pa_w),
	DEVCB_HANDLER(d_pia0_pb_w),
	DEVCB_HANDLER(d_pia0_ca2_w),
	DEVCB_HANDLER(d_pia0_cb2_w),
	/*irqs   : A/B             */
	DEVCB_LINE(coco3_pia0_irq_a),
	DEVCB_LINE(coco3_pia0_irq_b)
};

/* PIA 1 */
const pia6821_interface coco3_pia_intf_1 =
{
	/*inputs : A/B,CA/B1,CA/B2 */
	DEVCB_HANDLER(d_pia1_pa_r),
	DEVCB_HANDLER(d_pia1_pb_r_coco2),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	/*outputs: A/B,CA/B2       */
	DEVCB_HANDLER(d_pia1_pa_w),
	DEVCB_HANDLER(d_pia1_pb_w),
	DEVCB_HANDLER(d_pia1_ca2_w),
	DEVCB_HANDLER(d_pia1_cb2_w),
	/*irqs   : A/B             */
	DEVCB_LINE(coco3_pia1_firq_a),
	DEVCB_LINE(coco3_pia1_firq_b)
};

/* PIA 0 */
const pia6821_interface dragon32_pia_intf_0 =
{
	/*inputs : A/B,CA/B1,CA/B2 */
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	/*outputs: A/B,CA/B2       */
	DEVCB_HANDLER(d_pia0_pa_w),
	DEVCB_HANDLER(d_pia0_pb_w),
	DEVCB_HANDLER(d_pia0_ca2_w),
	DEVCB_HANDLER(d_pia0_cb2_w),
	/*irqs   : A/B             */
	DEVCB_LINE(d_pia0_irq_a),
	DEVCB_LINE(d_pia0_irq_b)
};

/* PIA 1 */
const pia6821_interface dragon32_pia_intf_1 =
{
	/*inputs : A/B,CA/B1,CA/B2 */
	DEVCB_HANDLER(d_pia1_pa_r),
	DEVCB_HANDLER(d_pia1_pb_r_dragon32),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	/*outputs: A/B,CA/B2       */
	DEVCB_HANDLER(d_pia1_pa_w),
	DEVCB_HANDLER(d_pia1_pb_w),
	DEVCB_HANDLER(d_pia1_ca2_w),
	DEVCB_HANDLER(d_pia1_cb2_w),
	/*irqs   : A/B             */
	DEVCB_LINE(d_pia1_firq_a),
	DEVCB_LINE(d_pia1_firq_b)
};

/* PIA 0 */
const pia6821_interface dragon64_pia_intf_0 =
{
	/*inputs : A/B,CA/B1,CA/B2 */
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	/*outputs: A/B,CA/B2       */
	DEVCB_HANDLER(d_pia0_pa_w),
	DEVCB_HANDLER(d_pia0_pb_w),
	DEVCB_HANDLER(d_pia0_ca2_w),
	DEVCB_HANDLER(d_pia0_cb2_w),
	/*irqs   : A/B             */
	DEVCB_LINE(d_pia0_irq_a),
	DEVCB_LINE(d_pia0_irq_b)
};

/* PIA 1 */
const pia6821_interface dragon64_pia_intf_1 =
{
	/*inputs : A/B,CA/B1,CA/B2 */
	DEVCB_HANDLER(d_pia1_pa_r),
	DEVCB_HANDLER(d_pia1_pb_r_coco),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	/*outputs: A/B,CA/B2       */
	DEVCB_HANDLER(d_pia1_pa_w),
	DEVCB_HANDLER(dragon64_pia1_pb_w),
	DEVCB_HANDLER(d_pia1_ca2_w),
	DEVCB_HANDLER(d_pia1_cb2_w),
	/*irqs   : A/B             */
	DEVCB_LINE(d_pia1_firq_a),
	DEVCB_LINE(d_pia1_firq_b)
};

/* PIA 0 and 1 as Dragon 64 */
/* PIA 0 */
const pia6821_interface dgnalpha_pia_intf_0 =
{
	/*inputs : A/B,CA/B1,CA/B2 */
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	/*outputs: A/B,CA/B2       */
	DEVCB_HANDLER(d_pia0_pa_w),
	DEVCB_HANDLER(d_pia0_pb_w),
	DEVCB_HANDLER(d_pia0_ca2_w),
	DEVCB_HANDLER(d_pia0_cb2_w),
	/*irqs   : A/B             */
	DEVCB_LINE(d_pia0_irq_a),
	DEVCB_LINE(d_pia0_irq_b)
};

/* PIA 1 */
const pia6821_interface dgnalpha_pia_intf_1 =
{
	/*inputs : A/B,CA/B1,CA/B2 */
	DEVCB_HANDLER(d_pia1_pa_r),
	DEVCB_HANDLER(d_pia1_pb_r_coco),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	/*outputs: A/B,CA/B2       */
	DEVCB_HANDLER(d_pia1_pa_w),
	DEVCB_HANDLER(d_pia1_pb_w),
	DEVCB_HANDLER(d_pia1_ca2_w),
	DEVCB_HANDLER(d_pia1_cb2_w),
	/*irqs   : A/B             */
	DEVCB_LINE(d_pia1_firq_a),
	DEVCB_LINE(d_pia1_firq_b)
};

/* PIA 2 */
const pia6821_interface dgnalpha_pia_intf_2 =
{
	/*inputs : A/B,CA/B1,CA/B2 */
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	/*outputs: A/B,CA/B2       */
	DEVCB_HANDLER(dgnalpha_pia2_pa_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	/*irqs   : A/B             */
	DEVCB_LINE(d_pia2_firq_a),
	DEVCB_LINE(d_pia2_firq_b)
};

const sam6883_interface coco_sam_intf =
{
	NULL,
	d_sam_set_pageonemode,
	d_sam_set_mpurate,
	d_sam_set_memorysize,
	d_sam_set_maptype
};

const sam6883_interface coco3_sam_intf =
{
	coco3_sam_get_rambase,
	NULL,
	d_sam_set_mpurate,
	NULL,
	coco3_sam_set_maptype
};



/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/***************************************************************************
  PAK files

  PAK files were originally for storing Program PAKs, but the file format has
  evolved into a snapshot file format, with a file format so convoluted and
  changing to make it worthy of Microsoft.
***************************************************************************/

static int load_pak_into_region(running_device *image, int *pakbase, int *paklen, UINT8 *mem, int segaddr, int seglen)
{
	if (*paklen)
	{
		if (*pakbase < segaddr)
		{
			/* We have to skip part of the PAK file */
			int skiplen;

			skiplen = segaddr - *pakbase;
			if (image_fseek(image, skiplen, SEEK_CUR))
			{
				if (LOG_PAK)
					logerror("Could not fully read PAK.\n");
				return 1;
			}

			*pakbase += skiplen;
			*paklen -= skiplen;
		}

		if (*pakbase < segaddr + seglen)
		{
			mem += *pakbase - segaddr;
			seglen -= *pakbase - segaddr;

			if (seglen > *paklen)
				seglen = *paklen;

			if (image_fread(image, mem, seglen) < seglen)
			{
				if (LOG_PAK)
					logerror("Could not fully read PAK.\n");
				return 1;
			}

			*pakbase += seglen;
			*paklen -= seglen;
		}
	}
	return 0;
}

static void pak_load_trailer(running_machine *machine, const pak_decodedtrailer *trailer)
{
	coco_state *state = (coco_state *)machine->driver_data;

	cpu_set_reg(devtag_get_device(machine, "maincpu"), M6809_PC, trailer->reg_pc);
	cpu_set_reg(devtag_get_device(machine, "maincpu"), M6809_X, trailer->reg_x);
	cpu_set_reg(devtag_get_device(machine, "maincpu"), M6809_Y, trailer->reg_y);
	cpu_set_reg(devtag_get_device(machine, "maincpu"), M6809_U, trailer->reg_u);
	cpu_set_reg(devtag_get_device(machine, "maincpu"), M6809_S, trailer->reg_s);
	cpu_set_reg(devtag_get_device(machine, "maincpu"), M6809_DP, trailer->reg_dp);
	cpu_set_reg(devtag_get_device(machine, "maincpu"), M6809_B, trailer->reg_b);
	cpu_set_reg(devtag_get_device(machine, "maincpu"), M6809_A, trailer->reg_a);
	cpu_set_reg(devtag_get_device(machine, "maincpu"), M6809_CC, trailer->reg_cc);

	/* I seem to only be able to get a small amount of the PIA state from the
     * snapshot trailers. Thus I am going to configure the PIA myself. The
     * following PIA writes are the same thing that the CoCo ROM does on
     * startup. I wish I had a better solution
     */

	pia6821_w(state->pia_0, 1, 0x00);
	pia6821_w(state->pia_0, 3, 0x00);
	pia6821_w(state->pia_0, 0, 0x00);
	pia6821_w(state->pia_0, 2, 0xff);
	pia6821_w(state->pia_1, 1, 0x00);
	pia6821_w(state->pia_1, 3, 0x00);
	pia6821_w(state->pia_1, 0, 0xfe);
	pia6821_w(state->pia_1, 2, 0xf8);

	pia6821_w(state->pia_0, 1, trailer->pia[1]);
	pia6821_w(state->pia_0, 0, trailer->pia[0]);
	pia6821_w(state->pia_0, 3, trailer->pia[3]);
	pia6821_w(state->pia_0, 2, trailer->pia[2]);

	pia6821_w(state->pia_1, 1, trailer->pia[5]);
	pia6821_w(state->pia_1, 0, trailer->pia[4]);
	pia6821_w(state->pia_1, 3, trailer->pia[7]);
	pia6821_w(state->pia_1, 2, trailer->pia[6]);

	/* For some reason, specifying use of high ram seems to screw things up;
     * I'm not sure whether it is because I'm using the wrong method to get
     * access that bit or or whether it is something else.  So that is why
     * I am specifying 0x7fff instead of 0xffff here
     */
	sam6883_set_state(state->sam, trailer->sam, 0x7fff);
}

static int generic_pak_load(running_device *image, int rambase_index, int rombase_index, int pakbase_index)
{
	UINT8 *ROM;
	UINT8 *rambase;
	UINT8 *rombase;
	UINT8 *pakbase;
	int paklength;
	int pakstart;
	pak_header header;
	int trailerlen;
	UINT8 trailerraw[500];
	pak_decodedtrailer trailer;
	int trailer_load = 0;

	ROM = memory_region(image->machine, "maincpu");
	rambase = &messram_get_ptr(devtag_get_device(image->machine, "messram"))[rambase_index];
	rombase = &ROM[rombase_index];
	pakbase = &ROM[pakbase_index];

	if (messram_get_size(devtag_get_device(image->machine, "messram")) < 0x10000)
	{
		if (LOG_PAK)
			logerror("Cannot load PAK files without at least 64k.\n");
		return INIT_FAIL;
	}

	if (image_fread(image, &header, sizeof(header)) < sizeof(header))
	{
		if (LOG_PAK)
			logerror("Could not fully read PAK.\n");
		return INIT_FAIL;
	}

	paklength = header.length ? LITTLE_ENDIANIZE_INT16(header.length) : 0x10000;
	pakstart = LITTLE_ENDIANIZE_INT16(header.start);

	if (image_fseek(image, paklength, SEEK_CUR))
	{
		if (LOG_PAK)
			logerror("Could not fully read PAK.\n");
		return INIT_FAIL;
	}

	trailerlen = image_fread(image, trailerraw, sizeof(trailerraw));
	if (trailerlen)
	{
		if (pak_decode_trailer(trailerraw, trailerlen, &trailer))
		{
			if (LOG_PAK)
				logerror("Invalid or unknown PAK trailer.\n");
			return INIT_FAIL;
		}

		trailer_load = 1;
	}

	if (image_fseek(image, sizeof(pak_header), SEEK_SET))
	{
		if (LOG_PAK)
			logerror("Unexpected error while reading PAK.\n");
		return INIT_FAIL;
	}

	/* Now that we are done reading the trailer; we can cap the length */
	if (paklength > 0xff00)
		paklength = 0xff00;

	/* PAK files reflect the fact that JeffV's emulator did not appear to
     * differentiate between RAM and ROM memory.  So what we do when a PAK
     * loads is to copy the ROM into RAM, load the PAK into RAM, and then
     * copy the part of RAM corresponding to PAK ROM to the actual PAK ROM
     * area.
     *
     * It is ugly, but it reflects the way that JeffV's emulator works
     */

	memcpy(rambase + 0x8000, rombase, 0x4000);
	memcpy(rambase + 0xC000, pakbase, 0x3F00);

	/* Get the RAM portion */
	if (load_pak_into_region(image, &pakstart, &paklength, rambase, 0x0000, 0xff00))
		return INIT_FAIL;

	memcpy(pakbase, rambase + 0xC000, 0x3F00);

	if (trailer_load)
		pak_load_trailer(image->machine, &trailer);
	return INIT_PASS;
}

SNAPSHOT_LOAD ( coco_pak )
{
	return generic_pak_load(image, 0x0000, 0x0000, 0x4000);
}

SNAPSHOT_LOAD ( coco3_pak )
{
	return generic_pak_load(image, (0x70000 % messram_get_size(devtag_get_device(image->machine, "messram"))), 0x0000, 0xc000);
}

/***************************************************************************
  Quickloads
***************************************************************************/

QUICKLOAD_LOAD ( coco )
{
	UINT8 preamble;
	UINT16 block_length;
	UINT16 block_address;
	const UINT8 *ptr;
	UINT32 length;
	UINT32 position = 0;
	UINT32 i;
	int done = FALSE;

	/* access the pointer and the length */
	ptr = (const UINT8*)image_ptr(image);
	length = image_length(image);

	while(!done && (position + 5 <= length))
	{
		/* read this block */
		preamble		= ptr[position + 0];
		block_length	= ((UINT16) ptr[position + 1]) << 8 | ((UINT16) ptr[position + 2]) << 0;
		block_address	= ((UINT16) ptr[position + 3]) << 8 | ((UINT16) ptr[position + 4]) << 0;
		position += 5;

		if (preamble != 0)
		{
			/* start address - just set the address and return */
			cpu_set_reg(devtag_get_device(image->machine, "maincpu"), REG_GENPC, block_address);
			done = TRUE;
		}
		else
		{
			const address_space *space = cputag_get_address_space( image->machine, "maincpu", ADDRESS_SPACE_PROGRAM );

			/* data block - need to cap the maximum length of the block */
			block_length = MIN(block_length, length - position);

			/* read the block into memory */
			for (i = 0; i < block_length; i++)
			{
				memory_write_byte(space, block_address + i, ptr[position + i]);
			}

			/* and advance */
			position += block_length;
		}
	}
	return INIT_PASS;
}

/***************************************************************************
  ROM files

  ROM files are simply raw dumps of cartridges.  I believe that they should
  be used in place of PAK files, when possible
***************************************************************************/

DEVICE_IMAGE_LOAD(coco_rom)
{
	UINT8 *dest = memory_region(image->machine, "cart");
	UINT16 destlength = (UINT16) memory_region_length(image->machine, "cart");
	UINT8 *rombase;
	int   romsize;

	romsize = image_length(image);

	/* The following hack is for Arkanoid running on the CoCo2.
        The issuse is the CoCo2 hardware only allows the cartridge
        interface to access 0xC000-0xFEFF (16K). The Arkanoid ROM is
        32K starting at 0x8000. The first 16K is totally inaccessable
        from a CoCo2. Thus we need to skip ahead in the ROM file. On
        the CoCo3 the entire 32K ROM is accessable. */

	if (image_crc(image) == 0x25C3AA70)     /* Test for Arkanoid  */
	{
		if ( destlength == 0x4000 )						/* Test if CoCo2      */
		{
			image_fseek( image, 0x4000, SEEK_SET );			/* Move ahead in file */
			romsize -= 0x4000;							/* Adjust ROM size    */
		}
	}

	if (romsize > destlength)
	{
		romsize = destlength;
	}

	image_fread(image, dest, romsize);

	/* Now we need to repeat the mirror the ROM throughout the ROM memory */
	rombase = dest;
	dest += romsize;
	destlength -= romsize;
	while(destlength > 0)
	{
		if (romsize > destlength)
			romsize = destlength;
		memcpy(dest, rombase, romsize);
		dest += romsize;
		destlength -= romsize;
	}
	return INIT_PASS;
}

DEVICE_IMAGE_UNLOAD(coco_rom)
{
	UINT8 *dest = memory_region(image->machine, "cart");
	UINT16 destlength = (UINT16) memory_region_length(image->machine, "cart");
	memset(dest, 0, destlength);
}

/***************************************************************************
  Interrupts

  The Dragon/CoCo2 have two PIAs.  These PIAs can trigger interrupts.  PIA0
  is set up to trigger IRQ on the CPU, and PIA1 can trigger FIRQ.  Each PIA
  has two output lines, and an interrupt will be triggered if either of these
  lines are asserted.

  -----  IRQ
  6809 |-<----------- PIA0
       |
       |
       |
       |
       |
       |-<----------- PIA1
  -----

  The CoCo 3 still supports these interrupts, but the GIME can chose whether
  "old school" interrupts are generated, or the new ones generated by the GIME

  -----  IRQ
  6809 |-<----------- PIA0
       |       |                ------
       |       -<-------<-------|    |
       |                        |GIME|
       |       -<-------<-------|    |
       | FIRQ  |                ------
       |-<----------- PIA1
  -----

  In an email discussion with JK, he informs me that when GIME interrupts are
  enabled, this actually does not prevent PIA interrupts.  Apparently JeffV's
  CoCo 3 emulator did not handle this properly.
***************************************************************************/

enum
{
	COCO3_INT_TMR	= 0x20,		/* Timer */
	COCO3_INT_HBORD	= 0x10,		/* Horizontal border sync */
	COCO3_INT_VBORD	= 0x08,		/* Vertical border sync */
	COCO3_INT_EI2	= 0x04,		/* Serial data */
	COCO3_INT_EI1	= 0x02,		/* Keyboard */
	COCO3_INT_EI0	= 0x01,		/* Cartridge */

	COCO3_INT_ALL	= 0x3f
};

static void d_recalc_irq(running_machine *machine)
{
	coco_state *state = (coco_state *)machine->driver_data;
	UINT8 pia0_irq_a = pia6821_get_irq_a(state->pia_0);
	UINT8 pia0_irq_b = pia6821_get_irq_b(state->pia_0);

	if (pia0_irq_a || pia0_irq_b)
		cputag_set_input_line(machine, "maincpu", M6809_IRQ_LINE, ASSERT_LINE);
	else
		cputag_set_input_line(machine, "maincpu", M6809_IRQ_LINE, CLEAR_LINE);
}

static void d_recalc_firq(running_machine *machine)
{
	coco_state *state = (coco_state *)machine->driver_data;
	UINT8 pia1_firq_a = pia6821_get_irq_a(state->pia_1);
	UINT8 pia1_firq_b = pia6821_get_irq_b(state->pia_1);
	UINT8 pia2_firq_a = (state->pia_2 != NULL) ? pia6821_get_irq_a(state->pia_2) : 0x00;
	UINT8 pia2_firq_b = (state->pia_2 != NULL) ? pia6821_get_irq_b(state->pia_2) : 0x00;

	if (pia1_firq_a || pia1_firq_b || pia2_firq_a || pia2_firq_b)
		cputag_set_input_line(machine, "maincpu", M6809_FIRQ_LINE, ASSERT_LINE);
	else
		cputag_set_input_line(machine, "maincpu", M6809_FIRQ_LINE, CLEAR_LINE);
}

static void coco3_recalc_irq(running_machine *machine)
{
	if ((coco3_gimereg[0] & 0x20) && gime_irq)
		cputag_set_input_line(machine, "maincpu", M6809_IRQ_LINE, ASSERT_LINE);
	else
		d_recalc_irq(machine);
}

static void coco3_recalc_firq(running_machine *machine)
{
	if ((coco3_gimereg[0] & 0x10) && gime_firq)
		cputag_set_input_line(machine, "maincpu", M6809_FIRQ_LINE, ASSERT_LINE);
	else
		d_recalc_firq(machine);
}

static WRITE_LINE_DEVICE_HANDLER( d_pia0_irq_a )
{
	d_recalc_irq(device->machine);
}

static WRITE_LINE_DEVICE_HANDLER( d_pia0_irq_b )
{
	d_recalc_irq(device->machine);
}

static WRITE_LINE_DEVICE_HANDLER( d_pia1_firq_a )
{
	d_recalc_firq(device->machine);
}

static WRITE_LINE_DEVICE_HANDLER( d_pia1_firq_b )
{
	d_recalc_firq(device->machine);
}

/* Dragon Alpha second PIA IRQ lines also cause FIRQ */
static WRITE_LINE_DEVICE_HANDLER( d_pia2_firq_a )
{
	d_recalc_firq(device->machine);
}

static WRITE_LINE_DEVICE_HANDLER( d_pia2_firq_b )
{
	d_recalc_firq(device->machine);
}

static WRITE_LINE_DEVICE_HANDLER( coco3_pia0_irq_a )
{
	coco3_recalc_irq(device->machine);
}

static WRITE_LINE_DEVICE_HANDLER( coco3_pia0_irq_b )
{
	coco3_recalc_irq(device->machine);
}

static WRITE_LINE_DEVICE_HANDLER( coco3_pia1_firq_a )
{
	coco3_recalc_firq(device->machine);
}

static WRITE_LINE_DEVICE_HANDLER( coco3_pia1_firq_b )
{
	coco3_recalc_firq(device->machine);
}

static void coco3_raise_interrupt(running_machine *machine, UINT8 mask, int state)
{
	int lowtohigh;

	lowtohigh = state && ((coco3_interupt_line & mask) == 0);

	if (state)
		coco3_interupt_line |= mask;
	else
		coco3_interupt_line &= ~mask;

	if (lowtohigh)
	{
		if ((coco3_gimereg[0] & 0x20) && (coco3_gimereg[2] & mask))
		{
			gime_irq |= (coco3_gimereg[2] & mask);
			coco3_recalc_irq(machine);

			if (LOG_INT_COCO3)
				logerror("CoCo3 Interrupt: Raising IRQ; scanline=%i\n", video_screen_get_vpos(machine->primary_screen));
		}
		if ((coco3_gimereg[0] & 0x10) && (coco3_gimereg[3] & mask))
		{
			gime_firq |= (coco3_gimereg[3] & mask);
			coco3_recalc_firq(machine);

			if (LOG_INT_COCO3)
				logerror("CoCo3 Interrupt: Raising FIRQ; scanline=%i\n", video_screen_get_vpos(machine->primary_screen));
		}
	}
}



void coco3_horizontal_sync_callback(running_machine *machine,int data)
{
	coco_state *state = (coco_state *)machine->driver_data;
	pia6821_ca1_w(state->pia_0, 0, data);
	coco3_raise_interrupt(machine, COCO3_INT_HBORD, data);
}



void coco3_field_sync_callback(running_machine *machine,int data)
{
	coco_state *state = (coco_state *)machine->driver_data;
	pia6821_cb1_w(state->pia_0, 0, data);
}

void coco3_gime_field_sync_callback(running_machine *machine)
{
	/* the CoCo 3 VBORD interrupt triggers right after the display */
	coco3_raise_interrupt(machine, COCO3_INT_VBORD, 1);
	coco3_raise_interrupt(machine, COCO3_INT_VBORD, 0);
}



/***************************************************************************
  Halt line
***************************************************************************/

static TIMER_CALLBACK(d_recalc_interrupts)
{
	d_recalc_firq(machine);
	d_recalc_irq(machine);
}

static TIMER_CALLBACK(coco3_recalc_interrupts)
{
	coco3_recalc_firq(machine);
	coco3_recalc_irq(machine);
}

static timer_fired_func recalc_interrupts;

#ifdef UNUSED_FUNCTION
void coco_set_halt_line(running_machine *machine, int halt_line)
{
	cpunum_set_input_line(machine, 0, INPUT_LINE_HALT, halt_line);
	if (halt_line == CLEAR_LINE)
		timer_set(machine, cputag_clocks_to_attotime(machine, "maincpu", 1), NULL, 0, recalc_interrupts);
}
#endif


/***************************************************************************
  Hires Joystick
***************************************************************************/

static int coco_hiresjoy_ca = 1;
static attotime coco_hiresjoy_xtransitiontime;
static attotime coco_hiresjoy_ytransitiontime;

static attotime coco_hiresjoy_computetransitiontime( running_machine *machine, UINT8 port, UINT8 axis )
{
	double val;
	UINT8 hires = input_port_read_safe(machine, "hires_intf", 0x00);
	/* this function gets only called for hi-res joystick. hence, hires can only take values
       0x01, 0x03 (hi-res) or 0x02, 0x04 (hi-res coco3max) */
	UINT8 coco3max = (hires == 0x02 || hires == 0x04) ? 1 : 0;

	static const char *const port_tags[4] = {"joystick_rx", "joystick_ry", "joystick_lx", "joystick_ly"};

	val = input_port_read_safe(machine, port_tags[axis + 2 * port], 0) / 255.0;

	if (coco3max)
	{
		/* CoCo MAX 3 Interface */
		val = val * 2500.0 + 400.0;
	}
	else
	{
		/* Normal Hires Interface */
		val = val * 4160.0 + 592.0;
	}

	return attotime_add(timer_get_time(machine), attotime_mul(COCO_CPU_SPEED, val));
}

static void coco_hiresjoy_w( running_machine *machine, int data, UINT8 port )
{
	if (!data && coco_hiresjoy_ca)
	{
		/* Hi to lo */
		coco_hiresjoy_xtransitiontime = coco_hiresjoy_computetransitiontime(machine, port, 0);
		coco_hiresjoy_ytransitiontime = coco_hiresjoy_computetransitiontime(machine, port, 1);
	}
	else if (data && !coco_hiresjoy_ca)
	{
		/* Lo to hi */
		coco_hiresjoy_xtransitiontime = attotime_zero;
		coco_hiresjoy_ytransitiontime = attotime_zero;
	}
	coco_hiresjoy_ca = data;
	(*update_keyboard)(machine);
}

static int coco_hiresjoy_readone( running_machine *machine, attotime transitiontime )
{
	return attotime_compare(timer_get_time(machine), transitiontime) >= 0;
}

static int coco_hiresjoy_rx( running_machine *machine )
{
	return coco_hiresjoy_readone(machine, coco_hiresjoy_xtransitiontime);
}

static int coco_hiresjoy_ry( running_machine *machine )
{
	return coco_hiresjoy_readone(machine, coco_hiresjoy_ytransitiontime);
}

/***************************************************************************
  Sound MUX

  The sound MUX has 4 possible settings, depend on SELA and SELB inputs:

  00    - DAC (digital - analog converter)
  01    - CSN (cassette)
  10    - SND input from cartridge (NYI because we only support the FDC)
  11    - Grounded (0)

  Note on the Dragon Alpha state 11, selects the AY-3-8912, this is currently
  un-implemented - phs.

  Source - Tandy Color Computer Service Manual
***************************************************************************/

#define SOUNDMUX_STATUS_ENABLE	4
#define SOUNDMUX_STATUS_SEL2	2
#define SOUNDMUX_STATUS_SEL1	1

static running_device *cassette_device_image(running_machine *machine)
{
	coco_state *state = (coco_state *)machine->driver_data;
	return state->cassette_device;
}

static running_device *bitbanger_image(running_machine *machine)
{
	coco_state *state = (coco_state *)machine->driver_data;
	return state->bitbanger_device;
}

static running_device *printer_image(running_machine *machine)
{
	coco_state *state = (coco_state *)machine->driver_data;
	return state->printer_device;
}

static running_device *cococart_device(running_machine *machine)
{
	coco_state *state = (coco_state *)machine->driver_data;
	return state->cococart_device;
}

static int get_soundmux_status(running_machine *machine)
{
	coco_state *state = (coco_state *)machine->driver_data;

	int soundmux_status = 0;
	if (pia6821_get_output_cb2(state->pia_1))
		soundmux_status |= SOUNDMUX_STATUS_ENABLE;
	if (mux_sel1)
		soundmux_status |= SOUNDMUX_STATUS_SEL1;
	if (mux_sel2)
		soundmux_status |= SOUNDMUX_STATUS_SEL2;
	return soundmux_status;
}

static void soundmux_update(running_machine *machine)
{
	/* This function is called whenever the MUX (selector switch) is changed
     * It mainly turns on and off the cassette audio depending on the switch.
     * It also calls a function into the cartridges device to tell it if it is
     * switch on or off.
     */
	cassette_state new_state;
	int soundmux_status = get_soundmux_status(machine);

	switch(soundmux_status) {
	case SOUNDMUX_STATUS_ENABLE | SOUNDMUX_STATUS_SEL1:
		/* CSN */
		new_state = CASSETTE_SPEAKER_ENABLED;
		break;
	default:
		new_state = CASSETTE_SPEAKER_MUTED;
		break;
	}

	coco_cartridge_set_line(
		cococart_device(machine),
		COCOCART_LINE_SOUND_ENABLE,
		(soundmux_status == (SOUNDMUX_STATUS_ENABLE|SOUNDMUX_STATUS_SEL2)
			? COCOCART_LINE_VALUE_ASSERT : COCOCART_LINE_VALUE_CLEAR));

	cassette_change_state(cassette_device_image(machine), new_state, CASSETTE_MASK_SPEAKER);
}

static void coco_sound_update(running_machine *machine)
{
	coco_state *state = (coco_state *)machine->driver_data;

	/* Call this function whenever you need to update the sound. It will
     * automatically mute any devices that are switched out.
     */
	UINT8 dac = pia6821_get_output_a(state->pia_1) & 0xFC;
	UINT8 pia1_pb1 = (pia6821_get_output_b(state->pia_1) & 0x02) ? 0x80 : 0x00;
	int soundmux_status = get_soundmux_status(machine);

	switch(soundmux_status)
	{
		case SOUNDMUX_STATUS_ENABLE:
			/* DAC */
			dac_data_w(state->dac, pia1_pb1 + (dac >> 1) );  /* Mixing the two sources */
			break;
		case SOUNDMUX_STATUS_ENABLE | SOUNDMUX_STATUS_SEL1:
			/* CSN */
			dac_data_w(state->dac, pia1_pb1); /* Mixing happens elsewhere */
			break;
		case SOUNDMUX_STATUS_ENABLE | SOUNDMUX_STATUS_SEL2:
			/* CART Sound */
			dac_data_w(state->dac, pia1_pb1); /* To do: mix in cart signal */
			break;
		default:
			/* This pia line is always connected to the output */
			dac_data_w(state->dac, pia1_pb1);
			break;
	}
}

/*
    Dragon Alpha AY-3-8912
*/

READ8_HANDLER ( dgnalpha_psg_porta_read )
{
	return 0;
}

WRITE8_HANDLER( dgnalpha_psg_porta_write )
{
	running_device *fdc = devtag_get_device(space->machine, "wd2797");
	/* Bits 0..3 are the drive select lines for the internal floppy interface */
	/* Bit 4 is the motor on, in the real hardware these are inverted on their way to the drive */
	/* Bits 5,6,7 are connected to /DDEN, ENP and 5/8 on the WD2797 */
	switch (data & 0xF)
	{
		case(0x01) :
			wd17xx_set_drive(fdc,0);
			break;
		case(0x02) :
			wd17xx_set_drive(fdc,1);
			break;
		case(0x04) :
			wd17xx_set_drive(fdc,2);
			break;
		case(0x08) :
			wd17xx_set_drive(fdc,3);
			break;
	}
}

/***************************************************************************
  PIA0 ($FF00-$FF1F) (Chip U8)

  PIA0 PA0-PA7  - Keyboard/Joystick read
  PIA0 PB0-PB7  - Keyboard write
  PIA0 CA1      - M6847 HS (Horizontal Sync)
  PIA0 CA2      - SEL1 (Used by sound mux and joystick)
  PIA0 CB1      - M6847 FS (Field Sync)
  PIA0 CB2      - SEL2 (Used by sound mux and joystick)
***************************************************************************/

static WRITE8_DEVICE_HANDLER( d_pia0_ca2_w )
{
	timer_adjust_oneshot(mux_sel1_timer, JOYSTICK_MUX_DELAY, data);
}

static TIMER_CALLBACK( coco_update_sel1_timerproc )
{
	mux_sel1 = param;
	(*update_keyboard)(machine);
	soundmux_update(machine);
}

static WRITE8_DEVICE_HANDLER( d_pia0_cb2_w )
{
	timer_adjust_oneshot(mux_sel2_timer, JOYSTICK_MUX_DELAY, data);
}

static TIMER_CALLBACK( coco_update_sel2_timerproc )
{
	mux_sel2 = param;
	(*update_keyboard)(machine);
	soundmux_update(machine);
}


static attotime get_relative_time( running_machine *machine, attotime absolute_time )
{
	attotime result;
	attotime now = timer_get_time(machine);

	if (attotime_compare(absolute_time, now) > 0)
		result = attotime_sub(absolute_time, now);
	else
		result = attotime_never;
	return result;
}



static UINT8 coco_update_keyboard( running_machine *machine )
{
	coco_state *state = (coco_state *)machine->driver_data;
	UINT8 porta = 0x7f, port_za = 0x7f;
	int joyval;
	static const int joy_rat_table[] = {15, 24, 42, 33 };
	static const int dclg_table[] = {0, 14, 30, 49 };
	attotime dclg_time = attotime_zero;
	UINT8 pia0_pb;
	UINT8 dac = pia6821_get_output_a(state->pia_1) & 0xfc;
	UINT8 hires = input_port_read_safe(machine, "hires_intf", 0x00);
	int joystick_axis, joystick, joy_mask, joy_shift, ctrl, hires_val, cc3m_val;

	static const char *const portnames[2][4][2] =
			{
				{ {0}, {"joystick_rx", "joystick_ry"}, {"rat_mouse_rx", "rat_mouse_ry"}, {"dclg_rx", "dclg_ry"} },	/* right ports */
				{ {0}, {"joystick_lx", "joystick_ly"}, {"rat_mouse_lx", "rat_mouse_ly"}, {"dclg_lx", "dclg_ly"} },	/* left ports */
			};
	static const char *const button_names[4] = { "empty", "joystick_buttons", "rat_mouse_buttons", "dclg_triggers" };



	pia0_pb = pia6821_get_output_b(state->pia_0);
	joystick_axis = mux_sel1;
	joystick = mux_sel2;

	/* which kind of controller is selected? */
	joy_mask = joystick ? 0xf0 : 0x0f;
	joy_shift = joystick ? 4 : 0;
	ctrl = (input_port_read_safe(machine, "ctrl_sel", 0x00) & joy_mask) >> joy_shift;

	/* is any Hi-Res Interface turned on? prepare masks to check it */
	hires_val = joystick ? 0x03 : 0x01;
	cc3m_val = joystick ? 0x04 : 0x02;

	/* poll keyboard keys */
	if ((input_port_read(machine, "row0") | pia0_pb) != 0xff) porta &= ~0x01;
	if ((input_port_read(machine, "row1") | pia0_pb) != 0xff) porta &= ~0x02;
	if ((input_port_read(machine, "row2") | pia0_pb) != 0xff) porta &= ~0x04;
	if ((input_port_read(machine, "row3") | pia0_pb) != 0xff) porta &= ~0x08;
	if ((input_port_read(machine, "row4") | pia0_pb) != 0xff) porta &= ~0x10;
	if ((input_port_read(machine, "row5") | pia0_pb) != 0xff) porta &= ~0x20;
	if ((input_port_read(machine, "row6") | pia0_pb) != 0xff) porta &= ~0x40;

	if ((input_port_read(machine, "row0") | pia6821_get_port_b_z_mask(state->pia_0)) != 0xff) port_za &= ~0x01;
	if ((input_port_read(machine, "row1") | pia6821_get_port_b_z_mask(state->pia_0)) != 0xff) port_za &= ~0x02;
	if ((input_port_read(machine, "row2") | pia6821_get_port_b_z_mask(state->pia_0)) != 0xff) port_za &= ~0x04;
	if ((input_port_read(machine, "row3") | pia6821_get_port_b_z_mask(state->pia_0)) != 0xff) port_za &= ~0x08;
	if ((input_port_read(machine, "row4") | pia6821_get_port_b_z_mask(state->pia_0)) != 0xff) port_za &= ~0x10;
	if ((input_port_read(machine, "row5") | pia6821_get_port_b_z_mask(state->pia_0)) != 0xff) port_za &= ~0x20;
	if ((input_port_read(machine, "row6") | pia6821_get_port_b_z_mask(state->pia_0)) != 0xff) port_za &= ~0x40;

	switch (ctrl)
	{
		case 0x01: /* Joystick */
			if (hires == hires_val || hires == cc3m_val)
			{
				/* Hi-Res Joystick or Hi-Res CoCo3Max Joystick */
				if (joystick_axis ? coco_hiresjoy_ry(machine) : coco_hiresjoy_rx(machine))
					porta |= 0x80;
			}
			else
			{
				joyval = input_port_read_safe(machine, portnames[joystick][ctrl][joystick_axis], 0x00);
				if (dac <= joyval)
					porta |= 0x80;
			}
			break;

		case 0x02: /* Rat Mouse */
			joyval = input_port_read_safe(machine, portnames[joystick][ctrl][joystick_axis], 0x00);
			if ((dac >> 2) <= joy_rat_table[joyval])
				porta |= 0x80;
			break;

		case 0x03: /* Diecom Light Gun */
			if ((video_screen_get_vpos(machine->primary_screen) == input_port_read_safe(machine, portnames[joystick][ctrl][1], 0)))
			{
				/* If gun is pointing at the current scan line, set hit bit and cache horizontal timer value */
				dclg_output_h |= 0x02;
				dclg_timer = input_port_read_safe(machine, portnames[joystick][ctrl][0], 0) << 1;
			}

			if ((dac >> 2) <= dclg_table[(joystick_axis ? dclg_output_h : dclg_output_v) & 0x03])
				porta |= 0x80;

			if ((dclg_state == 7))
			{
				/* While in state 7, prepare to chech next video frame for a hit */
				dclg_time = video_screen_get_time_until_pos(machine->primary_screen, input_port_read_safe(machine, portnames[joystick][ctrl][1], 0), 0);
			}

			break;

		default: /* No Controller */
			break;
	}

	if (attotime_compare(dclg_time, attotime_zero))
	{
		/* schedule lightgun events */
		timer_reset(update_keyboard_timer, dclg_time);
	}
	else
	{
		/* schedule hires joystick events */
		attotime xtrans = get_relative_time(machine, coco_hiresjoy_xtransitiontime);
		attotime ytrans = get_relative_time(machine, coco_hiresjoy_ytransitiontime);

		timer_reset(update_keyboard_timer,
			(attotime_compare(xtrans, ytrans) > 0) ? ytrans : xtrans);
	}

	/* sample joystick buttons */
	porta &= ~input_port_read_safe(machine, button_names[ctrl], 0);
	port_za &= ~input_port_read_safe(machine, button_names[ctrl], 0);

	pia6821_set_input_a(state->pia_0, porta, port_za);
	return porta;
}



static UINT8 coco3_update_keyboard( running_machine *machine )
{
	/* the CoCo 3 keyboard update routine must also check for the GIME EI1 interrupt */
	UINT8 porta = coco_update_keyboard(machine);
	coco3_raise_interrupt(machine, COCO3_INT_EI1, ((porta & 0x7f) == 0x7f) ? CLEAR_LINE : ASSERT_LINE);
	return porta;
}



/* three functions that update the keyboard in varying ways */
static WRITE8_DEVICE_HANDLER( d_pia0_pb_w )            { (*update_keyboard)(device->machine); }
INPUT_CHANGED( coco_keyboard_changed )                 { (*update_keyboard)(field->port->machine); }
static TIMER_CALLBACK( coco_update_keyboard_timerproc) { (*update_keyboard)(machine); }

static WRITE8_DEVICE_HANDLER( d_pia0_pa_w )
{
	UINT8 ctrl = input_port_read_safe(device->machine, "ctrl_sel", 0x00);
	UINT8 hires = input_port_read_safe(device->machine, "hires_intf", 0x00);

	/* Hi-Res CC3Max Joystick (either in Left or in Right Port) writes here */
	if ((ctrl & 0x0f) == 0x01 && hires == 0x02)
		coco_hiresjoy_w(device->machine, data & 0x04, 0);
	else if ((ctrl & 0xf0) == 0x10 && hires == 0x04)
		coco_hiresjoy_w(device->machine, data & 0x08, 1);
}


/***************************************************************************
  PIA1 ($FF20-$FF3F) (Chip U4)

  PIA1 PA0      - CASSDIN
  PIA1 PA1      - RS232 OUT (CoCo), Printer Strobe (Dragon)
  PIA1 PA2-PA7  - DAC
  PIA1 PB0      - RS232 IN
  PIA1 PB1      - Single bit sound
  PIA1 PB2      - RAMSZ (32/64K, 16K, and 4K three position switch)
  PIA1 PB3      - M6847 CSS
  PIA1 PB4      - M6847 INT/EXT and M6847 GM0
  PIA1 PB5      - M6847 GM1
  PIA1 PB6      - M6847 GM2
  PIA1 PB7      - M6847 A/G
  PIA1 CA1      - CD (Carrier Detect; NYI)
  PIA1 CA2      - CASSMOT (Cassette Motor)
  PIA1 CB1      - CART (Cartridge Detect)
  PIA1 CB2      - SNDEN (Sound Enable)
***************************************************************************/

static WRITE8_DEVICE_HANDLER ( d_pia1_cb2_w )
{
	soundmux_update(device->machine);
}

/* Printer output functions used by d_pia1_pa_w */
static void (*printer_out)(running_machine *machine, int data);

/* Printer output for the CoCo, output to bitbanger port */
static void printer_out_coco(running_machine *machine, int data)
{
	bitbanger_output(bitbanger_image(machine), (data & 2) >> 1);
}

/* Printer output for the Dragon, output to Paralel port */
static void printer_out_dragon(running_machine *machine, int data)
{
	coco_state *state = (coco_state *)machine->driver_data;

	/* If strobe bit is high send data from pia0 port b to dragon parallel printer */
	if (data & 0x02)
	{
		printer_output(printer_image(machine), pia6821_get_output_b(state->pia_0));
	}
}

static WRITE8_DEVICE_HANDLER ( d_pia1_pa_w )
{
	/*
     *  This port appears at $FF20
     *
     *  Bits
     *  7-2:    DAC to speaker or cassette
     *    1:    Serial out (CoCo), Printer strobe (Dragon)
     */
	UINT8 dac = pia6821_get_output_a(device) & 0xfc;
	static int dclg_previous_bit;
	UINT8 ctrl = input_port_read_safe(device->machine, "ctrl_sel", 0x00);
	UINT8 hires = input_port_read_safe(device->machine, "hires_intf", 0x00);

	coco_sound_update(device->machine);

	/* Diecom Light Gun (either in Left or in Right Port) writes here */
	if ((ctrl & 0x0f) == 0x03 || (ctrl & 0xf0) == 0x30)
	{
		int dclg_this_bit = ((data & 2) >> 1);

		if (dclg_previous_bit == 1)
		{
			if (dclg_this_bit == 0)
			{
				/* Clock Diecom Light gun interface on a high to low transistion */
				dclg_state++;
				dclg_state &= 0x0f;

				/* Clear hit bit for every transistion */
				dclg_output_h &= ~0x02;

				if (dclg_state > 7)
				{
					/* Bit shift timer data on state 8 thru 15 */
					if (((dclg_timer >> (dclg_state - 8 + 1)) & 0x01) == 1)
						dclg_output_v |= 0x01;
					else
						dclg_output_v &= ~0x01;

					/* Bit 9 of timer is only avaiable if state == 8*/
					if (dclg_state == 8 && (((dclg_timer >> 9) & 0x01) == 1))
						dclg_output_v |= 0x02;
					else
						dclg_output_v &= ~0x02;
				}

				/* During state 15, this bit is high. */
				if (dclg_state == 15)
					dclg_output_h |= 0x01;
				else
					dclg_output_h &= ~0x01;
			}
		}

		dclg_previous_bit = dclg_this_bit;

	}
	else
	{
		cassette_output(cassette_device_image(device->machine), ((int) dac - 0x80) / 128.0);
	}

	(*update_keyboard)(device->machine);

	/* Hi-Res Joystick (either in Left or in Right Port) writes here */
	if ((ctrl & 0x0f) == 0x01 && hires == 0x01)
		coco_hiresjoy_w(device->machine, dac >= 0x80, 0);
	else if ((ctrl & 0xf0) == 0x10 && hires == 0x03)
		coco_hiresjoy_w(device->machine, dac >= 0x80, 1);

	/* Handle printer output, serial for CoCos, parallel for Dragons */
	printer_out(device->machine, data);
}

/*
 * This port appears at $FF23
 *
 * The CoCo 1/2 and Dragon kept the gmode and vmode separate, and the CoCo
 * 3 tied them together.  In the process, the CoCo 3 dropped support for the
 * semigraphics modes
 */

static WRITE8_DEVICE_HANDLER( d_pia1_pb_w )
{
	coco6847_video_changed();

	/* PB1 will drive the sound output.  This is a rarely
     * used single bit sound mode. It is always connected thus
     * cannot be disabled.
     *
     * Source:  Page 31 of the Tandy Color Computer Serice Manual
     */
	coco_sound_update(device->machine);
}

static WRITE8_DEVICE_HANDLER( dragon64_pia1_pb_w )
{
	int ddr;

	d_pia1_pb_w(device, 0, data);

	ddr = ~pia6821_get_port_b_z_mask(device);

	/* If bit 2 of the pia1 ddrb is 1 then this pin is an output so use it */
	/* to control the paging of the 32k and 64k basic roms */
	/* Otherwise it set as an input, with an EXTERNAL pull-up so it should */
	/* always be high (enabling 32k basic rom) */
	if (ddr & 0x04)
	{
		dragon_page_rom(device->machine, data & 0x04);
	}
}

/***************************************************************************
  PIA2 ($FF24-$FF28) on Daragon Alpha/Professional

    PIA2 PA0        bcdir to AY-8912
    PIA2 PA1        bc0 to AY-8912
    PIA2 PA2        Rom switch, 0=basic rom, 1=boot rom.
    PIA2 PA3-PA7    Unknown/unused ?
    PIA2 PB0-PB7    connected to D0..7 of the AY8912.
    CB1             DRQ from WD2797 disk controler.
***************************************************************************/

static WRITE8_DEVICE_HANDLER( dgnalpha_pia2_pa_w )
{
	running_device *ay8912 = devtag_get_device(device->machine, "ay8912");
	int	bc_flags;		/* BCDDIR/BC1, as connected to PIA2 port a bits 0 and 1 */

	/* If bit 2 of the pia2 ddra is 1 then this pin is an output so use it */
	/* to control the paging of the boot and basic roms */
	/* Otherwise it set as an input, with an internal pull-up so it should */
	/* always be high (enabling boot rom) */
	/* PIA FIXME if (pia_get_ddr_a(2) & 0x04) */
	{
		dragon_page_rom(device->machine, data & 0x04);	/* bit 2 controls boot or basic rom */
	}

	/* Bits 0 and 1 for pia2 port a control the BCDIR and BC1 lines of the */
	/* AY-8912 */
	bc_flags = data & 0x03;	/* mask out bits */

	switch (bc_flags)
	{
		case 0x00	:		/* Inactive, do nothing */
			break;
		case 0x01	:		/* Write to selected port */
			ay8910_data_w(ay8912, 0, pia6821_get_output_b(device));
			break;
		case 0x02	:		/* Read from selected port */
			pia6821_portb_w(device, 0, ay8910_r(ay8912, 0));
			break;
		case 0x03	:		/* Select port to write to */
			ay8910_address_w(ay8912, 0, pia6821_get_output_b(device));
			break;
	}
}

/* Controls rom paging in Dragon 64, and Dragon Alpha */
/* On 64, switches between the two versions of the basic rom mapped in at 0x8000 */
/* on the alpha switches between the Boot/Diagnostic rom and the basic rom */
static void dragon_page_rom(running_machine *machine, int romswitch)
{
	UINT8 *bank;

	if (romswitch)
		bank = coco_rom;		/* This is the 32k mode basic(64)/boot rom(alpha)  */
	else
		bank = coco_rom + 0x8000;	/* This is the 64k mode basic(64)/basic rom(alpha) */

	bas_rom_bank = bank;			/* Record which rom we are using so that the irq routine */
						/* uses the vectors from the correct rom ! (alpha) */

	setup_memory_map(machine);			/* Setup memory map as needed */
}

/********************************************************************************************/
/* Dragon Alpha onboard FDC */
/********************************************************************************************/

/* The NMI line on the alphaAlpha is gated through IC16 (early PLD), and is gated by pia2 CA2  */
static WRITE_LINE_DEVICE_HANDLER( dgnalpha_fdc_intrq_w )
{
	coco_state *cstate = (coco_state *)device->machine->driver_data;

	if (state)
	{
		if(dgnalpha_just_reset)
		{
			dgnalpha_just_reset = 0;
		}
		else
		{
			if (pia6821_get_output_ca2_z(cstate->pia_2))
				cputag_set_input_line(device->machine, "maincpu", INPUT_LINE_NMI, ASSERT_LINE);
        }
	}
	else
		cputag_set_input_line(device->machine, "maincpu", INPUT_LINE_NMI, CLEAR_LINE);
}

/* The DRQ line goes through pia2 cb1, in exactly the same way as DRQ from DragonDos does */
/* for pia1 cb1 */
static WRITE_LINE_DEVICE_HANDLER( dgnalpha_fdc_drq_w )
{
	coco_state *cstate = (coco_state *)device->machine->driver_data;
	pia6821_cb1_w(cstate->pia_2, 0, state ? CARTLINE_ASSERTED : CARTLINE_CLEAR);
}

/* The Dragon Alpha hardware reverses the order of the WD2797 registers */
READ8_HANDLER(dgnalpha_wd2797_r)
{
	running_device *fdc = devtag_get_device(space->machine, "wd2797");
	int result = 0;

	switch(offset & 0x03)
	{
		case 0:
			result = wd17xx_data_r(fdc, 0);
			break;
		case 1:
			result = wd17xx_sector_r(fdc, 0);
			break;
		case 2:
			result = wd17xx_track_r(fdc, 0);
			break;
		case 3:
			result = wd17xx_status_r(fdc, 0);
			break;
		default:
			break;
	}

	return result;
}

WRITE8_HANDLER(dgnalpha_wd2797_w)
{
	running_device *fdc = devtag_get_device(space->machine, "wd2797");
    switch(offset & 0x3)
	{
		case 0:
			wd17xx_data_w(fdc, 0, data);
			break;
		case 1:
			wd17xx_sector_w(fdc, 0, data);
			break;
		case 2:
			wd17xx_track_w(fdc, 0, data);
			break;
		case 3:
            wd17xx_command_w(fdc, 0, data);

			/* disk head is encoded in the command byte */
			wd17xx_set_side(fdc,(data & 0x02) ? 1 : 0);
			break;
	};
}

READ8_HANDLER(dgnalpha_modem_r)
{
	return 0xFF;
}

WRITE8_HANDLER(dgnalpha_modem_w)
{
}

static WRITE8_DEVICE_HANDLER ( d_pia1_ca2_w )
{
	cassette_change_state(
		cassette_device_image(device->machine),
		data ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED,
		CASSETTE_MASK_MOTOR);
}



static READ8_DEVICE_HANDLER ( d_pia1_pa_r )
{
	return (cassette_input(cassette_device_image(device->machine)) >= 0) ? 1 : 0;
}



static READ8_DEVICE_HANDLER ( d_pia1_pb_r_coco )
{
	coco_state *state = (coco_state *)device->machine->driver_data;

	/* This handles the reading of the memory sense switch (pb2) for the CoCo 1,
     * and serial-in (pb0). Serial-in not yet implemented. */
	int result;

	/* For the CoCo 1, the logic has been changed to only select 64K rams
       if there is more than 16K of memory, as the Color Basic 1.0 rom
       can only configure 4K or 16K ram banks (as documented in "Color
       Basic Unreveled"), doing this allows this  allows the coco driver
       to access 32K of ram, and also allows the cocoe driver to access
       the full 64K, as this uses Color Basic 1.2, which can configure 64K rams */

	if (messram_get_size(devtag_get_device(device->machine, "messram")) > 0x8000)		/* 1 bank of 64K rams */
		result = (pia6821_get_output_b(state->pia_0) & 0x80) >> 5;
	else if (messram_get_size(devtag_get_device(device->machine, "messram")) >= 0x4000)	/* 1 or 2 banks of 16K rams */
		result = 0x04;
	else
		result = 0x00;			/* 4K Rams */

	return result;
}

static READ8_DEVICE_HANDLER ( d_pia1_pb_r_dragon32 )
{
	/* This handles the reading of the memory sense switch (pb2) for the Dragon 32,
     * and pb0, is the printer /busy line. */

	int result;

	/* Of the Dragon machines, Only the Dragon 32 needs the ram select bit
       as both the 64 and Alpha, always have 64K rams, also the meaning of
       the bit is different with respect to the CoCo 1 */

	if (messram_get_size(devtag_get_device(device->machine, "messram")) > 0x8000)
		result = 0x00;		/* 1 bank of 64K, rams */
	else
		result = 0x04;		/* 2 banks of 16K rams */

	return result;
}

static READ8_DEVICE_HANDLER ( d_pia1_pb_r_coco2 )
{
	coco_state *state = (coco_state *)device->machine->driver_data;

	/* This handles the reading of the memory sense switch (pb2) for the CoCo 2 and 3,
     * and serial-in (pb0). Serial-in not yet implemented.
     */
	int result;

	if (messram_get_size(devtag_get_device(device->machine, "messram")) <= 0x1000)
		result = 0x00;					/* 4K: wire pia1_pb2 low */
	else if (messram_get_size(devtag_get_device(device->machine, "messram")) <= 0x4000)
		result = 0x04;					/* 16K: wire pia1_pb2 high */
	else
		result = (pia6821_get_output_b(state->pia_0) & 0x40) >> 4;		/* 32/64K: wire output of pia0_pb6 to input pia1_pb2  */
	return result;
}


/*
    Compusense Dragon Plus Control register
*/

/* The read handler will eventually return the 6845 status */
READ8_HANDLER ( dgnplus_reg_r )
{
	return 0;
}

/*
    When writing the bits have the following meanings :

    bit value   purpose
    0   0   First 2k of memory map determined by bits 1 & 2
        1   6845 display RAM mapped into first 2K of map,

    2,1 0,0 Normal bottom 32K or ram mapped (from mainboard).
        0,1 First 32K of plus RAM mapped into $0000-$7FFF
        1,0 Second 32K of plus RAM mapped into $0000-$7FFF
        1,1 Undefined. I will assume that it's the same as 00.
    3-7     Unused.
*/
WRITE8_HANDLER ( dgnplus_reg_w )
{
	int map;

	dragon_plus_reg = data;

	map = (data & 0x06)>>1;

	switch (map)
	{
		case 0x00	: bottom_32k=&messram_get_ptr(devtag_get_device(space->machine, "messram"))[0x00000]; break;
		case 0x01	: bottom_32k=&messram_get_ptr(devtag_get_device(space->machine, "messram"))[0x10000]; break;
		case 0x02	: bottom_32k=&messram_get_ptr(devtag_get_device(space->machine, "messram"))[0x18000]; break;
		case 0x03	: bottom_32k=&messram_get_ptr(devtag_get_device(space->machine, "messram"))[0x00000]; break;
		default	: bottom_32k=&messram_get_ptr(devtag_get_device(space->machine, "messram"))[0x00000]; break; // Just to shut the compiler up !
	}

	setup_memory_map(space->machine);
}



/***************************************************************************
  Misc
***************************************************************************/

static SAM6883_SET_MPU_RATE( d_sam_set_mpurate )
{
	/* The infamous speed up poke.
     *
     * This was a SAM switch that occupied 4 addresses:
     *
     *      $FFD9   (set)   R1
     *      $FFD8   (clear) R1
     *      $FFD7   (set)   R0
     *      $FFD6   (clear) R0
     *
     * R1:R0 formed the following states:
     *      00  - slow          0.89 MHz
     *      01  - dual speed    ???
     *      1x  - fast          1.78 MHz
     *
     * R1 controlled whether the video addressing was speeded up and R0
     * did the same for the CPU.  On pre-CoCo 3 machines, setting R1 caused
     * the screen to display garbage because the M6847 could not display
     * fast enough.
     *
     * TODO:  Make the overclock more accurate.  In dual speed, ROM was a fast
     * access but RAM was not.  I don't know how to simulate this.
     */
    cpu_set_clockscale(devtag_get_device(device->machine, "maincpu"), val ? 2 : 1);
}

READ8_HANDLER(dgnalpha_mapped_irq_r)
{
	return bas_rom_bank[0x3ff0 + offset];
}

static void setup_memory_map(running_machine *machine)
{
	/*
    The following table contains the RAM block mappings for the CoCo 1/2 and Dragon computers
    This replicates the behavior of the SAM ram size programming bits which in ther real hardware
    allowed the use of various sizes of RAM chips, as follows :-

    1 or 2 banks of 4K
    1 or 2 banks of 16K
    1 bank of 64K
    up to 64K of static ram.

    For the 4K and 16K chip sizes, if the second bank was empty it would be mapped to nothing.
    For the 4K and 16K chip sizes, the banks would be mirrored at chip size*2 intervals.

    The following table holds the data required to implement this.

    Note though it is technically possible to have 2 banks of 4K rams, for a total of 8K, I
    have never seen a machine with this configuration, so I have not implemented it.
    */

	struct coco_meminfo
	{
		offs_t	start;		/* start address of bank */
		offs_t	end;		/* End address of bank */
		int	wbank4;		/* Bank to map when 4K rams */
		int	wbank16_1;	/* Bank to map when one bank of 16K rams */
		int	wbank16_2;	/* Bank to map when two banks of 16K rams */
		int	wbank64;	/* Bank to map when 64K rams */
	};

	static const struct coco_meminfo memmap[] =
	{
		{ 0x0000, 0x0FFF, 1, 1, 1, 1  },
		{ 0x1000, 0x1FFF, 0, 2, 2, 2  },
		{ 0x2000, 0x2FFF, 1, 3, 3, 3  },
		{ 0x3000, 0x3FFF, 0, 4, 4, 4  },
		{ 0x4000, 0x4FFF, 1, 0, 5, 5  },
		{ 0x5000, 0x5FFF, 0, 0, 6, 6  },
		{ 0x6000, 0x6FFF, 1, 0, 7, 7  },
		{ 0x7000, 0x7FFF, 0, 0, 8, 8  },
		{ 0x8000, 0x8FFF, 1, 1, 1, 9  },
		{ 0x9000, 0x9FFF, 0, 2, 2, 10 },
		{ 0xA000, 0xAFFF, 1, 3, 3, 11 },
		{ 0xB000, 0xBFFF, 0, 4, 4, 12 },
		{ 0xC000, 0xCFFF, 1, 0, 5, 13 },
		{ 0xD000, 0xDFFF, 0, 0, 6, 14 },
		{ 0xE000, 0xEFFF, 1, 0, 7, 15 },
		{ 0xF000, 0xFEFF, 0, 0, 8, 16 }
	};

	/* We need to init these vars from the sam, as this may be called from outside the sam callbacks */
	const address_space *space = cputag_get_address_space( machine, "maincpu", ADDRESS_SPACE_PROGRAM );
	coco_state *state = (coco_state *)machine->driver_data;
	UINT8 memsize	= sam6883_memorysize(state->sam);
	UINT8 maptype	= sam6883_maptype(state->sam);
	//UINT8 pagemode  = sam6883_pagemode(machine);
	int 		last_ram_block;		/* Last block that will be RAM, dependent on maptype */
	int 		block_index;		/* Index of block being processed */
	int		wbank;			/* bank no to go in this block */
	UINT8		*offset;		/* offset into coco rom for rom mapping */
	char	bank[10];

	/* Set last RAM block dependent on map type */
	if (maptype)
		last_ram_block=15;
	else
		last_ram_block=7;

	/* Map RAM blocks */
	for(block_index=0;block_index<=last_ram_block;block_index++)
	{
		sprintf(bank,"bank%d",block_index+1);
		/* Lookup the apropreate wbank value dependent on ram size */
		if (memsize==0)
			wbank=memmap[block_index].wbank4;
		else if ((memsize==1) && (messram_get_size(devtag_get_device(machine, "messram")) == 0x4000))	/* one bank of 16K rams */
			wbank=memmap[block_index].wbank16_1;
		else if ((memsize==1) && (messram_get_size(devtag_get_device(machine, "messram")) == 0x8000))	/* two banks of 16K rams */
			wbank=memmap[block_index].wbank16_2;
		else
			wbank=memmap[block_index].wbank64;

		/* If wbank is 0 then there is no ram here so set it up to return 0, note this may change in the future */
		/* as all unmapped addresses on the CoCo/Dragon seem to always return 126 (0x7E) */
		/* If wbank is a positive integer, it contains the bank number to map into this block */
		if(wbank!=0)
		{
			/* This deals with the bottom 32K page switch and the Dragon Plus paged ram */
			if(block_index<8)
				memory_set_bankptr(machine, bank, &bottom_32k[memmap[wbank-1].start]);
			else
				memory_set_bankptr(machine, bank,&messram_get_ptr(devtag_get_device(machine, "messram"))[memmap[wbank-1].start]);

			memory_install_readwrite_bank(space, memmap[block_index].start, memmap[block_index].end, 0, 0, bank);
		}
		else
		{
			memory_nop_readwrite(space, memmap[block_index].start, memmap[block_index].end, 0, 0);
		}
	}

	/* If in maptype 0 we need to map in the rom also, for now this just maps in the system and cart roms */
	if(!maptype)
	{
		UINT8 *cart_rom = memory_region(machine, "cart");
		UINT32 cart_length = memory_region_length(machine, "cart");

		for(block_index=0;block_index<=7;block_index++)
		{
			sprintf(bank,"bank%d",block_index+9);
			/* If we are in the BASIC rom area $8000-$BFFF, then we map to the bas_rom_bank */
			/* as this may be in a different block of coco_rom, in the Dragon 64 and Alpha */
			/* as these machines have mutiple bios roms that can ocupy this area */
			if (block_index < 4)
				offset = &bas_rom_bank[0x1000*block_index];
			else
				offset = &cart_rom[(0x1000*(block_index-4)) % cart_length];

			memory_set_bankptr(machine, bank,offset);
			memory_nop_write(space, memmap[block_index+8].start, memmap[block_index+8].end, 0, 0);
		}
	}
}


static SAM6883_SET_PAGE_ONE_MODE( d_sam_set_pageonemode )
{
	/* Page mode - allowed switching between the low 32k and the high 32k,
     * assuming that 64k wasn't enabled
     *
     * TODO:  Actually implement this.  Also find out what the CoCo 3 did with
     * this (it probably ignored it)
     */

	if (!sam6883_maptype(device))		// Ignored in maptype 1
	{
		if((messram_get_size(devtag_get_device(device->machine, "messram"))>0x8000) && val)
			bottom_32k=&messram_get_ptr(devtag_get_device(device->machine, "messram"))[0x8000];
		else
			bottom_32k=messram_get_ptr(devtag_get_device(device->machine, "messram"));

		setup_memory_map(device->machine);
	}
}

static SAM6883_SET_MEMORY_SIZE( d_sam_set_memorysize )
{
	/* Memory size - allowed restricting memory accesses to something less than
     * 32k
     *
     * This was a SAM switch that occupied 4 addresses:
     *
     *      $FFDD   (set)   R1
     *      $FFDC   (clear) R1
     *      $FFDB   (set)   R0
     *      $FFDA   (clear) R0
     *
     * R1:R0 formed the following states:
     *      00  - 4k
     *      01  - 16k
     *      10  - 64k
     *      11  - static RAM (??)
     *
     * If something less than 64k was set, the low RAM would be smaller and
     * mirror the other parts of the RAM
     *
     * TODO:  Find out what "static RAM" is
     * TODO:  This should affect _all_ memory accesses, not just video ram
     * TODO:  Verify that the CoCo 3 ignored this
     */

	setup_memory_map(device->machine);
}



/***************************************************************************
  CoCo 3 Timer

  The CoCo 3 had a timer that had would activate when first written to, and
  would decrement over and over again until zero was reached, and at that
  point, would flag an interrupt.  At this point, the timer starts back up
  again.

  I am deducing that the timer interrupt line was asserted if the timer was
  zero and unasserted if the timer was non-zero.  Since we never truly track
  the timer, we just use timer callback (coco3_timer_callback() asserts the
  line)

  Most CoCo 3 docs, including the specs that Tandy released, say that the
  high speed timer is 70ns (half of the speed of the main clock crystal).
  However, it seems that this is in error, and the GIME timer is really a
  280ns timer (one eighth the speed of the main clock crystal.  Gault's
  FAQ agrees with this
***************************************************************************/

static emu_timer *coco3_gime_timer;

static void coco3_timer_reset(running_machine *machine)
{
	/* reset the timer; take the value stored in $FF94-5 and start the timer ticking */
	UINT16 timer_value;
	m6847_timing_type timing;
	attotime delay_time;

	/* value is from 0-4095 */
	timer_value = ((coco3_gimereg[4] & 0x0F) * 0x100) | coco3_gimereg[5];

	if (timer_value > 0)
	{
		/* depending on the GIME type, cannonicalize the value */
		if (GIME_TYPE_1987)
			timer_value += 1;	/* the 1987 GIME reset to the value plus one */
		else
			timer_value += 2;	/* the 1986 GIME reset to the value plus two */

		/* choose which timing clock source */
		timing = (coco3_gimereg[1] & 0x20) ? M6847_CLOCK : M6847_HSYNC;

		/* determine the delay time */
		delay_time = coco6847_time_delay(machine, timing, timer_value);
		if (LOG_TIMER)
			logerror("coco3_reset_timer(): delay_time=%g\n", attotime_to_double(delay_time));

		/* and adjust the timer */
		timer_adjust_oneshot(coco3_gime_timer, delay_time, 0);
	}
	else
	{
		/* timer is shut off */
		timer_reset(coco3_gime_timer, attotime_never);
		if (LOG_TIMER)
			logerror("coco3_reset_timer(): timer is off\n");
	}
}



static TIMER_CALLBACK(coco3_timer_proc)
{
	coco3_timer_reset(machine);
	coco3_vh_blink();
	coco3_raise_interrupt(machine, COCO3_INT_TMR, 1);
	coco3_raise_interrupt(machine, COCO3_INT_TMR, 0);
}



static void coco3_timer_init(running_machine *machine)
{
	coco3_gime_timer = timer_alloc(machine, coco3_timer_proc, NULL);
}



/***************************************************************************
  MMU
***************************************************************************/

/* Dragon 64/Alpha and Tano Dragon 64 sams are now handled in exactly the same */
/* way as the CoCo 1/2 and Dragon 32, this now has been reduced to a call to */
/* setup_memory_map() - 2007-01-02 phs. */

static SAM6883_SET_MAP_TYPE( d_sam_set_maptype )
{
	if(val)
		bottom_32k=messram_get_ptr(devtag_get_device(device->machine, "messram"));	// Always reset, when in maptype 1

	setup_memory_map(device->machine);
}

/*************************************
 *
 *  CoCo 3
 *
 *************************************/


/*
 * coco3_mmu_translate() takes a zero counted bank index and an offset and
 * translates it into a physical RAM address.  The following logical memory
 * addresses have the following bank indexes:
 *
 *  Bank 0      $0000-$1FFF
 *  Bank 1      $2000-$3FFF
 *  Bank 2      $4000-$5FFF
 *  Bank 3      $6000-$7FFF
 *  Bank 4      $8000-$9FFF
 *  Bank 5      $A000-$BFFF
 *  Bank 6      $C000-$DFFF
 *  Bank 7      $E000-$FDFF
 *  Bank 8      $FE00-$FEFF
 *
 * The result represents a physical RAM address.  Since ROM/Cartidge space is
 * outside of the standard RAM memory map, ROM addresses get a "physical RAM"
 * address that has bit 31 set.  For example, ECB would be $80000000-
 * $80001FFFF, CB would be $80002000-$80003FFFF etc.  It is possible to force
 * this function to use a RAM address, which is used for video since video can
 * never reference ROM.
 */
offs_t coco3_mmu_translate(running_machine *machine,int bank, int offset)
{
	int forceram;
	UINT32 block;
	offs_t result;

	/* Bank 8 is the 0xfe00 block; and it is treated differently */
	if (bank == 8)
	{
		if (coco3_gimereg[0] & 8)
		{
			/* this GIME register fixes logical addresses $FExx to physical
             * addresses $7FExx ($1FExx if 128k */
			assert(offset < 0x200);
			return ((messram_get_size(devtag_get_device(machine, "messram")) - 0x200) & 0x7ffff) + offset;
		}
		bank = 7;
		offset += 0x1e00;
		forceram = 1;
	}
	else
	{
		forceram = 0;
	}

	/* Perform the MMU lookup */
	if (coco3_gimereg[0] & 0x40)
	{
		if (coco3_gimereg[1] & 1)
			bank += 8;
		block = coco3_mmu[bank];
		block |= ((UINT32) ((coco3_gimereg[11] >> 4) & 0x03)) << 8;
	}
	else
	{
		block = bank + 56;
	}

	/* Are we actually in ROM?
     *
     * In our world, ROM is represented by memory blocks 0x40-0x47
     *
     * 0    Extended Color Basic
     * 1    Color Basic
     * 2    Reset Initialization
     * 3    Super Extended Color Basic
     * 4-7  Cartridge ROM
     *
     * This is the level where ROM is mapped, according to Tepolt (p21)
     */
	if (((block & 0x3f) >= 0x3c) && !coco3_enable_64k && !forceram)
	{
		static const UINT8 rommap[4][4] =
		{
			{ 0, 1, 6, 7 },
			{ 0, 1, 6, 7 },
			{ 0, 1, 2, 3 },
			{ 4, 5, 6, 7 }
		};
		block = rommap[coco3_gimereg[0] & 3][(block & 0x3f) - 0x3c];
		result = (block * 0x2000 + offset) | 0x80000000;
	}
	else
	{
		result = ((block * 0x2000) + offset) % messram_get_size(devtag_get_device(machine, "messram"));
	}
	return result;
}



static void coco3_mmu_update(running_machine *machine, int lowblock, int hiblock)
{
	static const struct
	{
		offs_t start;
		offs_t end;
	}
	bank_info[] =
	{
		{ 0x0000, 0x1fff },
		{ 0x2000, 0x3fff },
		{ 0x4000, 0x5fff },
		{ 0x6000, 0x7fff },
		{ 0x8000, 0x9fff },
		{ 0xa000, 0xbfff },
		{ 0xc000, 0xdfff },
		{ 0xe000, 0xfdff },
		{ 0xfe00, 0xfeff }
	};

	const address_space *space = cputag_get_address_space( machine, "maincpu", ADDRESS_SPACE_PROGRAM );
	int i, offset;
	UINT8 *readbank;
	UINT8 *cart_rom = memory_region(machine, "cart");
	char bank[10];

	for (i = lowblock; i <= hiblock; i++)
	{
		sprintf(bank,"bank%d",i+1);

		offset = coco3_mmu_translate(machine, i, 0);
		if (offset & 0x80000000)
		{
			/* an offset into the CoCo 3 ROM */
			if (offset & 0x8000)
				readbank = &cart_rom[offset & ~0x80008000];
			else
				readbank = &coco_rom[offset & ~0x80000000];
			memory_unmap_write(space, bank_info[i].start, bank_info[i].end, 0, 0);
		}
		else
		{
			/* offset into normal RAM */
			readbank = &messram_get_ptr(devtag_get_device(machine, "messram"))[offset];
			memory_install_write_bank(space, bank_info[i].start, bank_info[i].end, 0, 0, bank);
		}

		/* set up the banks */
		memory_set_bankptr(machine, bank, readbank);

		if (LOG_MMU)
		{
			logerror("CoCo3 GIME MMU: Logical $%04x ==> Physical $%05x\n",
				(i == 8) ? 0xfe00 : i * 0x2000,
				offset);
		}
	}
}



READ8_HANDLER(coco3_mmu_r)
{
	/* The high two bits are floating (high resistance).  Therefore their
     * value is undefined.  But we are exposing them anyways here
     */
	return coco3_mmu[offset];
}



WRITE8_HANDLER(coco3_mmu_w)
{
	coco3_mmu[offset] = data;

	/* Did we modify the live MMU bank? */
	if ((offset >> 3) == (coco3_gimereg[1] & 1))
	{
		offset &= 7;
		coco3_mmu_update(space->machine, offset, (offset == 7) ? 8 : offset);
	}
}



/***************************************************************************
  GIME Registers (Reference: Super Extended Basic Unravelled)
***************************************************************************/

READ8_HANDLER(coco3_gime_r)
{
	UINT8 result = 0;

	switch(offset) {
	case 2:	/* Read pending IRQs */
		result = gime_irq;
		if (result) {
			gime_irq = 0;
			coco3_recalc_irq(space->machine);
		}
		break;

	case 3:	/* Read pending FIRQs */
		result = gime_firq;
		if (result) {
			gime_firq = 0;
			coco3_recalc_firq(space->machine);
		}
		break;

	case 4:	/* Timer MSB/LSB; these arn't readable */
	case 5:
		/* JK tells me that these values are indeterminate; and $7E appears
         * to be the value most commonly returned
         */
		result = 0x7e;
		break;

	default:
		result = coco3_gimereg[offset];
		break;
	}
	return result;
}



WRITE8_HANDLER(coco3_gime_w)
{
	/* take note if timer was $0000; see $FF95 for details */
	int timer_was_off = (coco3_gimereg[4] == 0x00) && (coco3_gimereg[5] == 0x00);
	coco3_gimereg[offset] = data;

	if (LOG_GIME)
		logerror("CoCo3 GIME: $%04x <== $%02x pc=$%04x\n", offset + 0xff90, data, cpu_get_pc(space->cpu));

	/* Features marked with '!' are not yet implemented */
	switch(offset)
	{
		case 0:
			/*  $FF90 Initialization register 0
            *         Bit 7 COCO 1=CoCo compatible mode
            *         Bit 6 MMUEN 1=MMU enabled
            *         Bit 5 IEN 1 = GIME chip IRQ enabled
            *         Bit 4 FEN 1 = GIME chip FIRQ enabled
            *         Bit 3 MC3 1 = RAM at FEXX is constant
            *         Bit 2 MC2 1 = standard SCS (Spare Chip Select)
            *         Bit 1 MC1 ROM map control
            *         Bit 0 MC0 ROM map control
            */
			coco3_mmu_update(space->machine, 0, 8);
			{
				if (coco3_gimereg[0] & 0x04)
				{
					running_device *device = cococart_device(space->machine);
					memory_install_read8_device_handler(space, device, 0xFF40, 0xFF5F, 0, 0, coco_cartridge_r);
					memory_install_write8_device_handler(space, device, 0xFF40, 0xFF5F, 0, 0, coco_cartridge_w);
				}
				else
				{
					memory_nop_readwrite(space, 0xFF40, 0xFF5F, 0, 0);
				}
			}
			break;

		case 1:
			/*  $FF91 Initialization register 1
            *         Bit 7 Unused
            *         Bit 6 Unused
            *         Bit 5 TINS Timer input select; 1 = 280 nsec, 0 = 63.5 usec
            *         Bit 4 Unused
            *         Bit 3 Unused
            *         Bit 2 Unused
            *         Bit 1 Unused
            *         Bit 0 TR Task register select
            */
			coco3_mmu_update(space->machine, 0, 8);
			coco3_timer_reset(space->machine);
			break;

		case 2:
			/*  $FF92 Interrupt request enable register
            *         Bit 7 Unused
            *         Bit 6 Unused
            *         Bit 5 TMR Timer interrupt
            *         Bit 4 HBORD Horizontal border interrupt
            *         Bit 3 VBORD Vertical border interrupt
            *       ! Bit 2 EI2 Serial data interrupt
            *         Bit 1 EI1 Keyboard interrupt
            *         Bit 0 EI0 Cartridge interrupt
            */
			if (LOG_INT_MASKING)
			{
				logerror("CoCo3 IRQ: Interrupts { %s%s%s%s%s%s} enabled\n",
					(data & 0x20) ? "TMR " : "",
					(data & 0x10) ? "HBORD " : "",
					(data & 0x08) ? "VBORD " : "",
					(data & 0x04) ? "EI2 " : "",
					(data & 0x02) ? "EI1 " : "",
					(data & 0x01) ? "EI0 " : "");
			}
			coco_cartridge_twiddle_q_lines(cococart_device(space->machine));
			break;

		case 3:
			/*  $FF93 Fast interrupt request enable register
            *         Bit 7 Unused
            *         Bit 6 Unused
            *         Bit 5 TMR Timer interrupt
            *         Bit 4 HBORD Horizontal border interrupt
            *         Bit 3 VBORD Vertical border interrupt
            *       ! Bit 2 EI2 Serial border interrupt
            *         Bit 1 EI1 Keyboard interrupt
            *         Bit 0 EI0 Cartridge interrupt
            */
			if (LOG_INT_MASKING)
			{
				logerror("CoCo3 FIRQ: Interrupts { %s%s%s%s%s%s} enabled\n",
					(data & 0x20) ? "TMR " : "",
					(data & 0x10) ? "HBORD " : "",
					(data & 0x08) ? "VBORD " : "",
					(data & 0x04) ? "EI2 " : "",
					(data & 0x02) ? "EI1 " : "",
					(data & 0x01) ? "EI0 " : "");
			}
			coco_cartridge_twiddle_q_lines(cococart_device(space->machine));
			break;

		case 4:
			/*  $FF94 Timer register MSB
            *         Bits 4-7 Unused
            *         Bits 0-3 High order four bits of the timer
            */
			coco3_timer_reset(space->machine);
			break;

		case 5:
			/*  $FF95 Timer register LSB
            *         Bits 0-7 Low order eight bits of the timer
            */
			if (timer_was_off && (coco3_gimereg[5] != 0x00))
	                {
	                        /* Writes to $FF95 do not cause the timer to reset, but MESS
                            * will invoke coco3_timer_reset() if $FF94/5 was previously
                            * $0000.  The reason for this is because the timer is not
                            * actually off when $FF94/5 are loaded with $0000; rather it
                            * is continuously reloading the GIME's internal countdown
                            * register, even if it isn't causing interrupts to be raised.
                            *
                            * Failure to do this was the cause of bug #1065.  Special
                            * thanks to John Kowalski for pointing me in the right
                            * direction
                */
				coco3_timer_reset(space->machine);
	                }
			break;

		case 8:
			/*  $FF98 Video Mode Register
            *         Bit 7 BP 0 = Text modes, 1 = Graphics modes
            *         Bit 6 Unused
            *       ! Bit 5 BPI Burst Phase Invert (Color Set)
            *         Bit 4 MOCH 1 = Monochrome on Composite
            *       ! Bit 3 H50 1 = 50 Hz power, 0 = 60 Hz power
            *         Bits 0-2 LPR Lines per row
            */
			break;

		case 9:
			/*  $FF99 Video Resolution Register
            *         Bit 7 Undefined
            *         Bits 5-6 LPF Lines per Field (Number of Rows)
            *         Bits 2-4 HRES Horizontal Resolution
            *         Bits 0-1 CRES Color Resolution
            */
			break;

		case 10:
			/*  $FF9A Border Register
            *         Bits 6,7 Unused
            *         Bits 0-5 BRDR Border color
            */
			break;

		case 12:
			/*  $FF9C Vertical Scroll Register
            *         Bits 4-7 Reserved
            *       ! Bits 0-3 VSC Vertical Scroll bits
            */
			break;

		case 11:
		case 13:
		case 14:
			/*  $FF9B,$FF9D,$FF9E Vertical Offset Registers
            *
            *   According to JK, if an odd value is placed in $FF9E on the 1986
            *   GIME, the GIME crashes
            *
            *  The reason that $FF9B is not mentioned in offical documentation
            *  is because it is only meaninful in CoCo 3's with the 2MB upgrade
            */
			break;

		case 15:
			/*
            *   $FF9F Horizontal Offset Register
            *         Bit 7 HVEN Horizontal Virtual Enable
            *         Bits 0-6 X0-X6 Horizontal Offset Address
            *
            *  Unline $FF9D-E, this value can be modified mid frame
            */
			break;
	}
}



static SAM6883_GET_RAMBASE( coco3_sam_get_rambase )
{
	UINT32 video_base;
	video_base = coco3_get_video_base(0xE0, 0x3F);
	return &messram_get_ptr(devtag_get_device(device->machine, "messram"))[video_base % messram_get_size(devtag_get_device(device->machine, "messram"))];
}




static SAM6883_SET_MAP_TYPE( coco3_sam_set_maptype )
{
	coco3_enable_64k = val;
	coco3_mmu_update(device->machine, 4, 8);
}



/***************************************************************************
    CARTRIDGE EXPANSION SLOT

    The bulk of the cartridge handling is in cococart.c and related code,
    whereas this interfaces with the main CoCo emulation.  This code also
    implements the hacks required to simulate the state where a CoCo
    cartridge ties the CART line to Q, which cannot be emulated by
    conventional techniques.

    When the CART line is set to Q, we begin a "twiddle" session -
    specifically the CART line is toggled on and off twice to ensure that
    if the PIA is accepting an interrupt, that it will receive it.  We also
    start twiddle sessions when CART is set to Q and the PIA is written to,
    in order to pick up any interrupt enables that might happen
***************************************************************************/

/*-------------------------------------------------
    coco_cart_w - call for CART line
-------------------------------------------------*/

void coco_cart_w(running_device *device, int data)
{
	coco_state *state = (coco_state *)device->machine->driver_data;
	pia6821_cb1_w(state->pia_1, 0, data ? ASSERT_LINE : CLEAR_LINE);
}


/*-------------------------------------------------
    coco3_cart_w - calls coco_cart_w and
    in addition will raise the GIME interrupt
-------------------------------------------------*/

void coco3_cart_w(running_device *device, int data)
{
	coco3_raise_interrupt(device->machine, COCO3_INT_EI0, data ? ASSERT_LINE : CLEAR_LINE);
	coco_cart_w(device, data);
}



/*-------------------------------------------------
    coco_halt_w - sets the HALT line
-------------------------------------------------*/

void coco_halt_w(running_device *device, int data)
{
	cputag_set_input_line(device->machine, "maincpu", INPUT_LINE_HALT, data ? ASSERT_LINE : CLEAR_LINE);
}



/*-------------------------------------------------
    coco_nmi_w - sets the NMI
-------------------------------------------------*/

void coco_nmi_w(running_device *device, int data)
{
	cputag_set_input_line(device->machine, "maincpu", INPUT_LINE_NMI, data ? ASSERT_LINE : CLEAR_LINE);
}



/*-------------------------------------------------
    coco_pia_1_w - wrapper for pia_1_w() that will
    also call twiddle Q lines, if necessary
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER(coco_pia_1_w)
{
	pia6821_w(device, offset, data);
	coco_cartridge_twiddle_q_lines(cococart_device(device->machine));
}



/***************************************************************************/

/* struct to hold callbacks and initializers to pass to generic_init_machine */
typedef struct _machine_init_interface machine_init_interface;
struct _machine_init_interface
{
	timer_fired_func recalc_interrupts_;			/* recalculate inturrupts callback */
	void (*printer_out_)(running_machine *machine, int data);				/* printer output callback */
};



/* New generic_init_machine, this sets up the the machine parameters common to all machines in */
/* the CoCo and Dragon families, it has been modified to accept a single structure of type */
/* machine_init_interface, which makes the init code a little clearer */
/* Note sam initialization has been moved to generic_coco12_dragon_init, as it is now identical */
/* for everything except the coco3, so it made sense not to pass it as a parameter */
static void generic_init_machine(running_machine *machine, const machine_init_interface *init)
{
	coco_state *state = (coco_state *)(coco_state *) machine->driver_data;

	/* locate devices */
	state->cococart_device	= devtag_get_device(machine, "coco_cartslot");
	state->cassette_device	= devtag_get_device(machine, "cassette");
	state->bitbanger_device	= devtag_get_device(machine, "bitbanger");
	state->printer_device	= devtag_get_device(machine, "printer");
	state->dac				= devtag_get_device(machine, "dac");
	state->sam				= devtag_get_device(machine, "sam");
	state->pia_0			= devtag_get_device(machine, "pia_0");
	state->pia_1			= devtag_get_device(machine, "pia_1");
	state->pia_2			= devtag_get_device(machine, "pia_2");

	/* clear static variables */
	coco_hiresjoy_ca = 1;
	coco_hiresjoy_xtransitiontime = attotime_zero;
	coco_hiresjoy_ytransitiontime = attotime_zero;

	/* set up function pointers */
	update_keyboard = coco_update_keyboard;
	recalc_interrupts = init->recalc_interrupts_;

	/* this timer is used to schedule keyboard updating */
	update_keyboard_timer = timer_alloc(machine, coco_update_keyboard_timerproc, NULL);

	/* these are the timers to delay the MUX switching */
	mux_sel1_timer = timer_alloc(machine, coco_update_sel1_timerproc, NULL);
	mux_sel2_timer = timer_alloc(machine, coco_update_sel2_timerproc, NULL);

	/* setup ROM */
	coco_rom = memory_region(machine, "maincpu");

	/* setup default rom bank */
	bas_rom_bank = coco_rom;

	/* setup default pointer for botom 32K of ram */
	bottom_32k = messram_get_ptr(devtag_get_device(machine, "messram"));

	/* setup printer output callback */
	printer_out = init->printer_out_;

	debug_cpu_set_dasm_override(devtag_get_device(machine, "maincpu"), CPU_DISASSEMBLE_NAME(coco_dasm_override));

	state_save_register_global(machine, mux_sel1);
	state_save_register_global(machine, mux_sel2);
}

/* Setup for hardware common to CoCo 1/2 & Dragon machines, calls genertic_init_machine, to process */
/* the setup common with the CoCo3, and then does the init that is not common ! */
static void generic_coco12_dragon_init(running_machine *machine, const machine_init_interface *init)
{
	/* Set default RAM mapping */
	memory_set_bankptr(machine, "bank1", &messram_get_ptr(devtag_get_device(machine, "messram"))[0]);

	/* Do generic Inits */
	generic_init_machine(machine, init);
}

/******* Lightgun Setup **********/

static void update_lightgun( running_machine *machine )
{
	/* is there a Diecom Light Gun either in Left or in Right Port? */
	UINT8 ctrl = input_port_read_safe(machine, "ctrl_sel", 0x00);
	int is_lightgun = ((ctrl & 0x0f) == 0x03 || (ctrl & 0xf0) == 0x30) ? 1 :0;

	crosshair_set_screen(machine, 0, is_lightgun ? CROSSHAIR_SCREEN_ALL : CROSSHAIR_SCREEN_NONE);
}

INPUT_CHANGED( coco_joystick_mode_changed )
{
	update_lightgun(field->port->machine);
}

static TIMER_CALLBACK( update_lightgun_timer_callback )
{
	update_lightgun(machine);
}

/******* Machine Setups Dragons **********/

MACHINE_START( dragon32 )
{
	machine_init_interface init;

	/* Setup machine initialization */
	memset(&init, 0, sizeof(init));
	init.recalc_interrupts_	= d_recalc_interrupts;
	init.printer_out_		= printer_out_dragon;

	generic_coco12_dragon_init(machine, &init);

	/* need to specify lightgun crosshairs */
	timer_set(machine, attotime_zero, NULL, 0, update_lightgun_timer_callback);
}

MACHINE_START( dragon64 )
{
	machine_init_interface init;

	/* Setup machine initialization */
	memset(&init, 0, sizeof(init));
	init.recalc_interrupts_	= d_recalc_interrupts;
	init.printer_out_		= printer_out_dragon;

	generic_coco12_dragon_init(machine, &init);

	/* need to specify lightgun crosshairs */
	timer_set(machine, attotime_zero, NULL, 0, update_lightgun_timer_callback);
}

MACHINE_START( tanodr64 )
{
	machine_init_interface init;

	/* Setup machine initialization */
	memset(&init, 0, sizeof(init));
	init.recalc_interrupts_	= d_recalc_interrupts;
	init.printer_out_		= printer_out_dragon;

	generic_coco12_dragon_init(machine, &init);

	/* need to specify lightgun crosshairs */
	timer_set(machine, attotime_zero, NULL, 0, update_lightgun_timer_callback);
}

MACHINE_START( dgnalpha )
{
	machine_init_interface init;

	/* Setup machine initialization */
	memset(&init, 0, sizeof(init));
	init.recalc_interrupts_	= d_recalc_interrupts;
	init.printer_out_		= printer_out_dragon;

	generic_coco12_dragon_init(machine, &init);

	/* need to specify lightgun crosshairs */
	timer_set(machine, attotime_zero, NULL, 0, update_lightgun_timer_callback);
}

MACHINE_RESET( dgnalpha )
{
	running_device *fdc = devtag_get_device(machine, "wd2797");
	wd17xx_set_complete_command_delay(fdc,20);

	/* dgnalpha_just_reset, is here to flag that we should ignore the first irq generated */
	/* by the WD2797, it is reset to 0 after the first inurrupt */
	dgnalpha_just_reset=1;

	/* need to specify lightgun crosshairs */
	timer_set(machine, attotime_zero, NULL, 0, update_lightgun_timer_callback);
}

const wd17xx_interface dgnalpha_wd17xx_interface =
{
	DEVCB_NULL,
	DEVCB_LINE(dgnalpha_fdc_intrq_w),
	DEVCB_LINE(dgnalpha_fdc_drq_w),
	{FLOPPY_0, FLOPPY_1, FLOPPY_2, FLOPPY_3}
};

/******* Machine Setups CoCos **********/

MACHINE_START( coco )
{
	machine_init_interface init;

	/* Setup machine initialization */
	memset(&init, 0, sizeof(init));
	init.recalc_interrupts_	= d_recalc_interrupts;
	init.printer_out_		= printer_out_coco;

	generic_coco12_dragon_init(machine, &init);

	/* need to specify lightgun crosshairs */
	timer_set(machine, attotime_zero, NULL, 0, update_lightgun_timer_callback);
}

MACHINE_START( coco2 )
{
	machine_init_interface init;

	/* Setup machine initialization */
	memset(&init, 0, sizeof(init));
	init.recalc_interrupts_	= d_recalc_interrupts;
	init.printer_out_		= printer_out_coco;

	generic_coco12_dragon_init(machine, &init);

	/* need to specify lightgun crosshairs */
	timer_set(machine, attotime_zero, NULL, 0, update_lightgun_timer_callback);
}

MACHINE_RESET( coco3 )
{
	int i;

	/* Tepolt verifies that the GIME registers are all cleared on initialization */
	coco3_enable_64k = 0;
	gime_irq = 0;
	gime_firq = 0;
	for (i = 0; i < 8; i++)
	{
		coco3_mmu[i] = coco3_mmu[i + 8] = 56 + i;
		coco3_gimereg[i] = 0;
	}
	coco3_mmu_update(machine, 0, 8);
}

static STATE_POSTLOAD( coco3_state_postload )
{
	coco3_mmu_update(machine, 0, 8);
}

MACHINE_START( coco3 )
{
	machine_init_interface init;

	/* Setup machine initialization */
	memset(&init, 0, sizeof(init));
	init.recalc_interrupts_	= coco3_recalc_interrupts;
	init.printer_out_		= printer_out_coco;

	generic_init_machine(machine, &init);

	/* CoCo 3 specific function pointers */
	update_keyboard = coco3_update_keyboard;

	coco3_timer_init(machine);

	coco3_interupt_line = 0;

	/* set up state save variables */
	state_save_register_global_array(machine, coco3_mmu);
	state_save_register_global_array(machine, coco3_gimereg);
	state_save_register_global(machine, coco3_interupt_line);
	state_save_register_global(machine, gime_irq);
	state_save_register_global(machine, gime_firq);
	state_save_register_postload(machine, coco3_state_postload, NULL);

	/* need to specify lightgun crosshairs */
	timer_set(machine, attotime_zero, NULL, 0, update_lightgun_timer_callback);
}



/***************************************************************************
  OS9 Syscalls for disassembly
****************************************************************************/

static const char *const os9syscalls[] =
{
	"F$Link",          /* Link to Module */
	"F$Load",          /* Load Module from File */
	"F$UnLink",        /* Unlink Module */
	"F$Fork",          /* Start New Process */
	"F$Wait",          /* Wait for Child Process to Die */
	"F$Chain",         /* Chain Process to New Module */
	"F$Exit",          /* Terminate Process */
	"F$Mem",           /* Set Memory Size */
	"F$Send",          /* Send Signal to Process */
	"F$Icpt",          /* Set Signal Intercept */
	"F$Sleep",         /* Suspend Process */
	"F$SSpd",          /* Suspend Process */
	"F$ID",            /* Return Process ID */
	"F$SPrior",        /* Set Process Priority */
	"F$SSWI",          /* Set Software Interrupt */
	"F$PErr",          /* Print Error */
	"F$PrsNam",        /* Parse Pathlist Name */
	"F$CmpNam",        /* Compare Two Names */
	"F$SchBit",        /* Search Bit Map */
	"F$AllBit",        /* Allocate in Bit Map */
	"F$DelBit",        /* Deallocate in Bit Map */
	"F$Time",          /* Get Current Time */
	"F$STime",         /* Set Current Time */
	"F$CRC",           /* Generate CRC */
	"F$GPrDsc",        /* get Process Descriptor copy */
	"F$GBlkMp",        /* get System Block Map copy */
	"F$GModDr",        /* get Module Directory copy */
	"F$CpyMem",        /* Copy External Memory */
	"F$SUser",         /* Set User ID number */
	"F$UnLoad",        /* Unlink Module by name */
	"F$Alarm",         /* Color Computer Alarm Call (system wide) */
	NULL,
	NULL,
	"F$NMLink",        /* Color Computer NonMapping Link */
	"F$NMLoad",        /* Color Computer NonMapping Load */
	NULL,
	NULL,
	"F$TPS",           /* Return System's Ticks Per Second */
	"F$TimAlm",        /* COCO individual process alarm call */
	"F$VIRQ",          /* Install/Delete Virtual IRQ */
	"F$SRqMem",        /* System Memory Request */
	"F$SRtMem",        /* System Memory Return */
	"F$IRQ",           /* Enter IRQ Polling Table */
	"F$IOQu",          /* Enter I/O Queue */
	"F$AProc",         /* Enter Active Process Queue */
	"F$NProc",         /* Start Next Process */
	"F$VModul",        /* Validate Module */
	"F$Find64",        /* Find Process/Path Descriptor */
	"F$All64",         /* Allocate Process/Path Descriptor */
	"F$Ret64",         /* Return Process/Path Descriptor */
	"F$SSvc",          /* Service Request Table Initialization */
	"F$IODel",         /* Delete I/O Module */
	"F$SLink",         /* System Link */
	"F$Boot",          /* Bootstrap System */
	"F$BtMem",         /* Bootstrap Memory Request */
	"F$GProcP",        /* Get Process ptr */
	"F$Move",          /* Move Data (low bound first) */
	"F$AllRAM",        /* Allocate RAM blocks */
	"F$AllImg",        /* Allocate Image RAM blocks */
	"F$DelImg",        /* Deallocate Image RAM blocks */
	"F$SetImg",        /* Set Process DAT Image */
	"F$FreeLB",        /* Get Free Low Block */
	"F$FreeHB",        /* Get Free High Block */
	"F$AllTsk",        /* Allocate Process Task number */
	"F$DelTsk",        /* Deallocate Process Task number */
	"F$SetTsk",        /* Set Process Task DAT registers */
	"F$ResTsk",        /* Reserve Task number */
	"F$RelTsk",        /* Release Task number */
	"F$DATLog",        /* Convert DAT Block/Offset to Logical */
	"F$DATTmp",        /* Make temporary DAT image (Obsolete) */
	"F$LDAXY",         /* Load A [X,[Y]] */
	"F$LDAXYP",        /* Load A [X+,[Y]] */
	"F$LDDDXY",        /* Load D [D+X,[Y]] */
	"F$LDABX",         /* Load A from 0,X in task B */
	"F$STABX",         /* Store A at 0,X in task B */
	"F$AllPrc",        /* Allocate Process Descriptor */
	"F$DelPrc",        /* Deallocate Process Descriptor */
	"F$ELink",         /* Link using Module Directory Entry */
	"F$FModul",        /* Find Module Directory Entry */
	"F$MapBlk",        /* Map Specific Block */
	"F$ClrBlk",        /* Clear Specific Block */
	"F$DelRAM",        /* Deallocate RAM blocks */
	"F$GCMDir",        /* Pack module directory */
	"F$AlHRam",        /* Allocate HIGH RAM Blocks */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"F$RegDmp",        /* Ron Lammardo's debugging register dump call */
	"F$NVRAM",         /* Non Volatile RAM (RTC battery backed static) read/write */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"I$Attach",        /* Attach I/O Device */
	"I$Detach",        /* Detach I/O Device */
	"I$Dup",           /* Duplicate Path */
	"I$Create",        /* Create New File */
	"I$Open",          /* Open Existing File */
	"I$MakDir",        /* Make Directory File */
	"I$ChgDir",        /* Change Default Directory */
	"I$Delete",        /* Delete File */
	"I$Seek",          /* Change Current Position */
	"I$Read",          /* Read Data */
	"I$Write",         /* Write Data */
	"I$ReadLn",        /* Read Line of ASCII Data */
	"I$WritLn",        /* Write Line of ASCII Data */
	"I$GetStt",        /* Get Path Status */
	"I$SetStt",        /* Set Path Status */
	"I$Close",         /* Close Path */
	"I$DeletX"         /* Delete from current exec dir */
};


static CPU_DISASSEMBLE(coco_dasm_override)
{
	unsigned call;
	unsigned result = 0;

	/* check for SWI2 instruction */
	if ((oprom[0] == 0x10) && (oprom[1] == 0x3F))
	{
		call = oprom[2];
		if ((call >= 0) && (call < ARRAY_LENGTH(os9syscalls)) && os9syscalls[call])
		{
			sprintf(buffer, "OS9   %s", os9syscalls[call]);
			result = 3;
		}
	}
	return result;
}
