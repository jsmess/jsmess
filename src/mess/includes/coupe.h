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


struct coupe_asic
{
	UINT8 lmpr, hmpr, vmpr; /* memory pages */	
	UINT8 clut[16];         /* color lookup table, 16 entries */
	UINT8 line_int;         /* line interrupt */
	UINT8 status;           /* status register */
};


/*----------- defined in drivers/coupe.c -----------*/

void coupe_irq(running_machine *machine, UINT8 src);


/*----------- defined in machine/coupe.c -----------*/

extern struct coupe_asic coupe_regs; 

void coupe_update_memory(void);

DEVICE_IMAGE_LOAD( coupe_floppy );
MACHINE_START( coupe );
MACHINE_RESET( coupe );


/*----------- defined in video/coupe.c -----------*/

VIDEO_UPDATE( coupe );


#endif /* COUPE_H_ */
