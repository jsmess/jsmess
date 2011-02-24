/******************************************************************************

    i82720.h

    Video driver for the Intel 82720 and NEC uPD7220 GDC.
    Per Ola Ingvarsson
    Tomas Karlsson

 ******************************************************************************/

#include "emu.h"

#define COMPIS_PALETTE_SIZE           16
/*
** The I/O functions
*/

READ8_HANDLER ( compis_gdc_r );
WRITE8_HANDLER ( compis_gdc_w );

READ16_HANDLER ( compis_gdc_16_r );
WRITE16_HANDLER ( compis_gdc_16_w );

/*
** This next function must be called 50 times per second,
** to generate the necessary interrupts
*/
void compis_gdc_vblank_int (void);
#define GDC_LPF         262     /* number of lines in a single frame */
/*
** Call this function to render the screen.
*/
PALETTE_INIT( compis_gdc );
VIDEO_START ( compis_gdc );
SCREEN_UPDATE( compis_gdc );
/*
** The different modes
*/
typedef enum
{
	GDC_MODE_HRG,
	GDC_MODE_UHRG
} compis_gdc_modes;

/*
** MachineDriver video declarations for the i82720 chip
*/
typedef struct _compis_gdc_interface
{
	compis_gdc_modes mode;
	UINT32	vramsize;
} compis_gdc_interface;

void compis_init(const compis_gdc_interface *intf);
