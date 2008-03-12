/**************************************************************************

	Interrupt System Hardware for Bally/Midway games

 	Mike@Dissfulfils.co.uk

**************************************************************************/

#include "driver.h"
#include "includes/astrocde.h"
#include "cpu/z80/z80.h"

#ifdef MAME_DEBUG
#define VERBOSE 1
#else
#define VERBOSE 0
#endif
#define LOG(x) do { if (VERBOSE) logerror x; } while (0)


/****************************************************************************
 * Scanline Interrupt System
 ****************************************************************************/

static emu_timer *scanline_timer;

static int NextScanInt=0;			/* Normal */

static int screen_interrupts_enabled;
static int screen_interrupt_mode;
static int lightpen_interrupts_enabled;
static int lightpen_interrupt_mode;


WRITE8_HANDLER ( astrocade_interrupt_enable_w )
{

	screen_interrupts_enabled = data & 0x08;
	screen_interrupt_mode = data & 0x04;
	lightpen_interrupts_enabled = data & 0x02;
	lightpen_interrupt_mode = data & 0x01;

	LOG(("Interrupt Flag set to %02x\n",data & 0x0f));
}

WRITE8_HANDLER ( astrocade_interrupt_w )
{
	/* A write to 0F triggers an interrupt at that scanline */

	LOG(("Scanline interrupt set to %02x\n",data));

    NextScanInt = data;
}


static TIMER_CALLBACK( astrocde_scanline_callback )
{
	int scanline = video_screen_get_vpos(0);
		
	/* scanline interrupt? */
	if (screen_interrupts_enabled && (screen_interrupt_mode == 0) && (scanline == NextScanInt))
		cpunum_set_input_line(machine, 0, 0, HOLD_LINE);

	/* are we on the last scanline? */
	if (++scanline == machine->screen[0].height)
		scanline = 0;

	/* wait for next scanline */
	timer_adjust_oneshot(scanline_timer, video_screen_get_time_until_pos(0, scanline, 0), 0);
}


WRITE8_HANDLER( astrocade_interrupt_vector_w )
{
	cpunum_set_input_line_vector(0, 0, data);
	cpunum_set_input_line(machine, 0, 0, CLEAR_LINE);
}


MACHINE_RESET( astrocde )
{
	/* wait for the first scanline */
	timer_adjust_oneshot(scanline_timer, video_screen_get_time_until_pos(0, 0, 0), 0);
}


DRIVER_INIT( astrocde )
{
	scanline_timer = timer_alloc(astrocde_scanline_callback, NULL);
}
