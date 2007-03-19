/**************************************************************************

	Interrupt System Hardware for Bally/Midway games

 	Mike@Dissfulfils.co.uk

**************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/astrocde.h"

/****************************************************************************
 * Scanline Interrupt System
 ****************************************************************************/

int CurrentScan=0;
static int NextScanInt=0;			/* Normal */

static int screen_interrupts_enabled;
static int screen_interrupt_mode;
static int lightpen_interrupts_enabled;
static int lightpen_interrupt_mode;

DEVICE_LOAD(astrocade_rom)
{
	int size = 0;

    /* load a cartidge  */
	size = image_fread(image, memory_region(REGION_CPU1) + 0x2000, 0x8000);
	return 0;
}

WRITE8_HANDLER ( astrocade_interrupt_enable_w )
{

	screen_interrupts_enabled = data & 0x08;
	screen_interrupt_mode = data & 0x04;
	lightpen_interrupts_enabled = data & 0x02;
	lightpen_interrupt_mode = data & 0x01;

#ifdef MAME_DEBUG
    logerror("Interrupt Flag set to %02x\n",data & 0x0f);
#endif
}

WRITE8_HANDLER ( astrocade_interrupt_w )
{
	/* A write to 0F triggers an interrupt at that scanline */

#ifdef MAME_DEBUG
	logerror("Scanline interrupt set to %02x\n",data);
#endif

    NextScanInt = data;
}

INTERRUPT_GEN( astrocade_interrupt )
{
    CurrentScan++;

    if (CurrentScan == Machine->drv->cpu[0].vblank_interrupts_per_frame)
		CurrentScan = 0;

    if (CurrentScan < 204) astrocade_copy_line(CurrentScan);

    /* Scanline interrupt enabled ? */
    if ((screen_interrupts_enabled) && (screen_interrupt_mode == 0)
	                                && (CurrentScan == NextScanInt))
		cpunum_set_input_line(0, 0, HOLD_LINE);
}

WRITE8_HANDLER( astrocade_interrupt_vector_w )
{
	cpunum_set_input_line_vector(0, 0, data);
	cpunum_set_input_line(0, 0, CLEAR_LINE);
}

