/*********************************************************************

	apple2gs.h

	Apple IIgs code

*********************************************************************/

#ifndef APPLE2GS_H
#define APPLE2GS_H

// IIgs clocks as marked on the schematics
#define APPLE2GS_28M (28636360)	// IIGS master clock
#define APPLE2GS_14M (APPLE2GS_28M/2)
#define APPLE2GS_7M (APPLE2GS_28M/4)

extern UINT8 *apple2gs_slowmem;
extern UINT8 apple2gs_newvideo;
extern UINT16 apple2gs_bordercolor;

extern UINT8 apple2gs_docram[64*1024];

INTERRUPT_GEN( apple2gs_interrupt );

MACHINE_START( apple2gs );
MACHINE_RESET( apple2gs );
VIDEO_START( apple2gs );
VIDEO_UPDATE( apple2gs );

NVRAM_HANDLER( apple2gs );

void apple2gs_doc_irq(int state);

#endif /* APPLE2GS_H */
