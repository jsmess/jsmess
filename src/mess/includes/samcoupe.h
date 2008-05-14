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


struct samcoupe_asic
{
	UINT8 lmpr, hmpr, vmpr; /* memory pages */
	UINT8 lext, hext;       /* extended memory page */
	UINT8 border;           /* border */
	UINT8 clut[16];         /* color lookup table, 16 entries */
	UINT8 line_int;         /* line interrupt */
	UINT8 status;           /* status register */
};


/*----------- defined in drivers/samcoupe.c -----------*/

void samcoupe_irq(running_machine *machine, UINT8 src);


/*----------- defined in machine/samcoupe.c -----------*/

extern struct samcoupe_asic samcoupe_regs; 

void samcoupe_update_memory(void);

WRITE8_HANDLER( samcoupe_ext_mem_w );
MACHINE_START( samcoupe );
MACHINE_RESET( samcoupe );


/*----------- defined in video/samcoupe.c -----------*/

VIDEO_UPDATE( samcoupe );


#endif /* SAMCOUPE_H_ */
