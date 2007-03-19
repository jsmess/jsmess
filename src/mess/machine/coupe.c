/***************************************************************************

 SAM Coupe Driver - Written By Lee Hammerton

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/coupe.h"
#include "devices/basicdsk.h"
#include "machine/wd17xx.h"

UINT8 LMPR,HMPR,VMPR;	/* Bank Select Registers (Low Page p250, Hi Page p251, Video Page p252) */
UINT8 CLUT[16]; 		/* 16 entries in a palette (no line affects supported yet!) */
UINT8 SOUND_ADDR;		/* Current Address in sound registers */
UINT8 SOUND_REG[32];	/* 32 sound registers */
UINT8 LINE_INT; 		/* Line interrupt */
UINT8 LPEN,HPEN;		/* ??? */
UINT8 CURLINE;			/* Current scanline */
UINT8 STAT; 			/* returned when port 249 read */

extern UINT8 *sam_screen;

DEVICE_LOAD( coupe_floppy )
{
	if (device_load_basicdsk_floppy(image)==INIT_PASS)
	{
		basicdsk_set_geometry(image, 80, 2, 10, 512, 1, 0, FALSE);
		return INIT_PASS;
	}
	return INIT_FAIL;
}



static void coupe_update_bank(int bank, UINT8 *memory, int is_readonly)
{
	read8_handler rh;
	write8_handler wh;

	if (memory)
		memory_set_bankptr(bank, memory);

	rh = !memory ? MRA8_NOP :							(read8_handler) (STATIC_BANK1 + bank - 1);
	wh = !memory ? MWA8_NOP : (is_readonly ? MWA8_ROM :	(write8_handler) (STATIC_BANK1 + bank - 1));

	memory_install_read8_handler(0,  ADDRESS_SPACE_PROGRAM, ((bank-1) * 0x4000), ((bank-1) * 0x4000) + 0x3FFF, 0, 0, rh);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, ((bank-1) * 0x4000), ((bank-1) * 0x4000) + 0x3FFF, 0, 0, wh);
}



void coupe_update_memory(void)
{
	UINT8 *rom = memory_region(REGION_CPU1);
	int PAGE_MASK = (mess_ram_size / 0x4000) - 1;
	UINT8 *memory;
	int is_readonly;

	/* BANK1 */
    if (LMPR & LMPR_RAM0)   /* Is ram paged in at bank 1 */
	{
		if ((LMPR & 0x1F) <= PAGE_MASK)
			memory = &mess_ram[(LMPR & PAGE_MASK) * 0x4000];
		else
			memory = NULL;	/* Attempt to page in non existant ram region */
		is_readonly = FALSE;
	}
	else
	{
		memory = rom;	/* Rom0 paged in */
		is_readonly = TRUE;
	}
	coupe_update_bank(1, memory, is_readonly);


	/* BANK2 */
	if (( (LMPR+1) & 0x1F) <= PAGE_MASK)
		memory = &mess_ram[((LMPR+1) & PAGE_MASK) * 0x4000];
	else
		memory = NULL;	/* Attempt to page in non existant ram region */
	coupe_update_bank(2, memory, FALSE);


	/* BANK3 */
	if ( (HMPR & 0x1F) <= PAGE_MASK )
		memory = &mess_ram[(HMPR & PAGE_MASK)*0x4000];
	else
		memory = NULL;	/* Attempt to page in non existant ram region */
	coupe_update_bank(3, memory, FALSE);


	/* BANK4 */
	if (LMPR & LMPR_ROM1)	/* Is Rom1 paged in at bank 4 */
	{
		memory = rom + 0x4000;
		is_readonly = TRUE;
	}
	else
	{
		if (( (HMPR+1) & 0x1F) <= PAGE_MASK)
			memory = &mess_ram[((HMPR+1) & PAGE_MASK) * 0x4000];
		else
			memory = NULL;	/* Attempt to page in non existant ram region */
		is_readonly = FALSE;
	}
	coupe_update_bank(4, memory, FALSE);


	if (VMPR & 0x40)	/* if bit set in 2 bank screen mode */
		sam_screen = &mess_ram[((VMPR&0x1E) & PAGE_MASK) * 0x4000];
	else
		sam_screen = &mess_ram[((VMPR&0x1F) & PAGE_MASK) * 0x4000];
}

MACHINE_RESET( coupe )
{
    LMPR = 0x0F;            /* ROM0 paged in, ROM1 paged out RAM Banks */
    HMPR = 0x01;
    VMPR = 0x81;

    LINE_INT = 0xFF;
    LPEN = 0x00;
    HPEN = 0x00;

    STAT = 0x1F;

    CURLINE = 0x00;

    coupe_update_memory();

    wd179x_init(WD_TYPE_177X,NULL);
}
