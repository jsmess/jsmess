/*****************************************************************************
 *
 * includes/apple2gs.h
 *
 * Apple IIgs
 *
 ****************************************************************************/

#ifndef APPLE2GS_H_
#define APPLE2GS_H_


// IIgs clocks as marked on the schematics
#define APPLE2GS_28M  (XTAL_28_63636MHz) // IIGS master clock
#define APPLE2GS_14M  (APPLE2GS_28M/2)
#define APPLE2GS_7M   (APPLE2GS_28M/4)

// screen dimensions
#define BORDER_LEFT	(32)
#define BORDER_RIGHT	(32)
#define BORDER_TOP	(16)	// (plus bottom)


/*----------- defined in machine/apple2gs.c -----------*/

extern UINT8 *apple2gs_slowmem;
extern UINT8 apple2gs_newvideo;

extern UINT8 apple2gs_docram[64*1024];

MACHINE_START( apple2gs );
MACHINE_RESET( apple2gs );

NVRAM_HANDLER( apple2gs );

void apple2gs_doc_irq(const device_config *device, int state);


/*----------- defined in video/apple2gs.c -----------*/

extern UINT16 apple2gs_bordercolor;

VIDEO_START( apple2gs );
VIDEO_UPDATE( apple2gs );


#endif /* APPLE2GS_H_ */
