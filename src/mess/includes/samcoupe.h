/*****************************************************************************
 *
 * includes/coupe.h
 *
 * SAM Coupe
 *
 * Driver by Lee Hammerton
 *
 ****************************************************************************/

#ifndef SAMCOUPE_H_
#define SAMCOUPE_H_


#define SAM_LINE_INT	0x01
#define SAM_FRAME_INT	0x08


typedef struct _coupe_asic coupe_asic;
struct _coupe_asic
{
	UINT8 lmpr, hmpr, vmpr; /* memory pages */
	UINT8 lext, hext;       /* extended memory page */
	UINT8 border;           /* border */
	UINT8 clut[16];         /* color lookup table, 16 entries */
	UINT8 line_int;         /* line interrupt */
	UINT8 status;           /* status register */
};


/*----------- defined in drivers/samcoupe.c -----------*/

void samcoupe_irq(const device_config *device, UINT8 src);


/*----------- defined in machine/samcoupe.c -----------*/

void samcoupe_update_memory(const address_space *space);

WRITE8_HANDLER( samcoupe_ext_mem_w );
MACHINE_RESET( samcoupe );


/*----------- defined in video/samcoupe.c -----------*/

VIDEO_UPDATE( samcoupe );


#endif /* SAMCOUPE_H_ */
