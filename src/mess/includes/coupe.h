/*****************************************************************************
 *
 * includes/coupe.h
 *
 * SAM Coupe
 *
 * Driver by Lee Hammerton
 *
 ****************************************************************************/

#ifndef COUPE_H_
#define COUPE_H_


/*----------- defined in drivers/coupe.c -----------*/

void coupe_irq(running_machine *machine, UINT8 src);


/*----------- defined in machine/coupe.c -----------*/

extern UINT8 LMPR,HMPR,VMPR;/* Bank Select Registers (Low Page p250, Hi Page p251, Video Page p252) */
extern UINT8 CLUT[16];		/* 16 entries in a palette (no line affects supported yet!) */
extern UINT8 LINE_INT;		/* Line interrupt register */
extern UINT8 STAT;			/* returned when port 249 read */

void coupe_update_memory(void);

DEVICE_IMAGE_LOAD( coupe_floppy );
MACHINE_START( coupe );
MACHINE_RESET( coupe );


/*----------- defined in video/coupe.c -----------*/

VIDEO_UPDATE( coupe );


#endif /* COUPE_H_ */
