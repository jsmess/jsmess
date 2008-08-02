/*****************************************************************************
 *
 * includes/geneve.h
 *
 * header file for the Geneve driver
 *
 ****************************************************************************/

#ifndef GENEVE_H_
#define GENEVE_H_


/* defines */

/* offsets for "|" */
enum
{
	offset_rom_geneve = 0x0000,			/* boot ROM (16 kbytes) */
	offset_sram_geneve = 0x4000,		/* static RAM (32 or 64 kbytes) */
	offset_dram_geneve = 0x14000,		/* dynamic RAM (512 kbytes) or SRAM (2MBytes) with Memex board and GenMod extension */
	region_cpu1_len_geneve = 0x214000	/* total len */
};


/*----------- defined in machine/geneve.c -----------*/

/* prototypes for machine code */

DRIVER_INIT( geneve );
DRIVER_INIT( genmod );

MACHINE_START( geneve );
MACHINE_RESET( geneve );

VIDEO_START( geneve );
INTERRUPT_GEN( geneve_hblank_interrupt );

READ8_HANDLER ( geneve_r );
WRITE8_HANDLER ( geneve_w );

WRITE8_HANDLER ( geneve_peb_mode_cru_w );


#endif /* GENEVE_H_ */
