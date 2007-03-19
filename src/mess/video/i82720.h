/******************************************************************************

	i82720.h

	Video driver for the Intel 82720 and NEC uPD7220 GDC.
	Per Ola Ingvarsson
	Tomas Karlsson
	
 ******************************************************************************/
 
#include "driver.h"

#define COMPIS_PALETTE_SIZE           16
/*
** The I/O functions
*/

READ8_HANDLER ( compis_gdc_r );
WRITE8_HANDLER ( compis_gdc_w );

/*
** This next function must be called 50 times per second,
** to generate the necessary interrupts
*/
void compis_gdc_vblank_int (void);
#define GDC_LPF         262     /* number of lines in a single frame */
INTERRUPT_GEN(gdc_line_interrupt);
/*
** Call this function to render the screen.
*/
VIDEO_START ( compis_gdc );
VIDEO_UPDATE( compis_gdc );
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
typedef struct compis_gdc_interface
{
	compis_gdc_modes mode;
	UINT32	vramsize;
} compis_gdc_interface;

extern void mdrv_compisgdc(machine_config *machine,
                           const compis_gdc_interface *intf);

#define MDRV_COMPISGDC(intf)		mdrv_compisgdc(machine, (intf));
