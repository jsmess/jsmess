/*********************************************************************

	apple2gs.h

	Apple IIgs code

*********************************************************************/

#ifndef APPLE2GS_H
#define APPLE2GS_H

extern UINT8 *apple2gs_slowmem;
extern UINT8 apple2gs_newvideo;
extern UINT16 apple2gs_bordercolor;

INTERRUPT_GEN( apple2gs_interrupt );

MACHINE_START( apple2gs );
VIDEO_START( apple2gs );
VIDEO_UPDATE( apple2gs );

NVRAM_HANDLER( apple2gs );

void apple2gs_doc_irq(int state);

#endif /* APPLE2GS_H */
