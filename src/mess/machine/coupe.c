/***************************************************************************

 SAM Coupe Driver - Written By Lee Hammerton

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "includes/coupe.h"
#include "devices/basicdsk.h"
#include "machine/wd17xx.h"
#include "machine/msm6242.h"


#define LMPR_RAM0	0x20	/* If bit set ram is paged into bank 0, else its rom0 */
#define LMPR_ROM1	0x40	/* If bit set rom1 is paged into bank 3, else its ram */


struct coupe_asic coupe_regs;


static void coupe_update_bank(int bank, UINT8 *memory, int is_readonly)
{
	read8_machine_func rh;
	write8_machine_func wh;

	if (memory)
		memory_set_bankptr(bank, memory);

	rh = !memory ? SMH_NOP :								(read8_machine_func) (STATIC_BANK1 + (FPTR)bank - 1);
	wh = !memory ? SMH_NOP : (is_readonly ? SMH_UNMAP :	(write8_machine_func) (STATIC_BANK1 + (FPTR)bank - 1));

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
    if (coupe_regs.lmpr & LMPR_RAM0)   /* Is ram paged in at bank 1 */
	{
		if ((coupe_regs.lmpr & 0x1F) <= PAGE_MASK)
			memory = &mess_ram[(coupe_regs.lmpr & PAGE_MASK) * 0x4000];
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
	if (( (coupe_regs.lmpr+1) & 0x1F) <= PAGE_MASK)
		memory = &mess_ram[((coupe_regs.lmpr+1) & PAGE_MASK) * 0x4000];
	else
		memory = NULL;	/* Attempt to page in non existant ram region */
	coupe_update_bank(2, memory, FALSE);


	/* BANK3 */
	if ( (coupe_regs.hmpr & 0x1F) <= PAGE_MASK )
		memory = &mess_ram[(coupe_regs.hmpr & PAGE_MASK)*0x4000];
	else
		memory = NULL;	/* Attempt to page in non existant ram region */
	coupe_update_bank(3, memory, FALSE);


	/* BANK4 */
	if (coupe_regs.lmpr & LMPR_ROM1)	/* Is Rom1 paged in at bank 4 */
	{
		memory = rom + 0x4000;
		is_readonly = TRUE;
	}
	else
	{
		if (( (coupe_regs.hmpr+1) & 0x1F) <= PAGE_MASK)
			memory = &mess_ram[((coupe_regs.hmpr+1) & PAGE_MASK) * 0x4000];
		else
			memory = NULL;	/* Attempt to page in non existant ram region */
		is_readonly = FALSE;
	}
	coupe_update_bank(4, memory, FALSE);


	if (coupe_regs.vmpr & 0x40)	/* if bit set in 2 bank screen mode */
		videoram = &mess_ram[((coupe_regs.vmpr&0x1E) & PAGE_MASK) * 0x4000];
	else
		videoram = &mess_ram[((coupe_regs.vmpr&0x1F) & PAGE_MASK) * 0x4000];
}


static READ8_HANDLER( coupe_rtc_r )
{
	return msm6242_r(machine, offset >> 12);
}


static WRITE8_HANDLER( coupe_rtc_w )
{
	msm6242_w(machine, offset >> 12, data);
}


MACHINE_RESET( coupe )
{
	memset(&coupe_regs, 0, sizeof(coupe_regs));
	
    coupe_regs.lmpr = 0x0f;      /* ROM0 paged in, ROM1 paged out RAM Banks */
    coupe_regs.hmpr = 0x01;
    coupe_regs.vmpr = 0x81;
    coupe_regs.line_int = 0xff;  /* line interrupts disabled */
    coupe_regs.status = 0x1f;    /* no interrupts active */

	if (input_port_read(machine, "config") & 0x01)
	{
		/* install RTC */
		memory_install_readwrite8_handler(0, ADDRESS_SPACE_IO, 0xef, 0xef, 0xffff, 0xff00, coupe_rtc_r, coupe_rtc_w);
	}
	else
	{
		/* no RTC support */
		memory_install_readwrite8_handler(0, ADDRESS_SPACE_IO, 0xef, 0xef, 0xffff, 0xff00, SMH_UNMAP, SMH_UNMAP);
	}

    coupe_update_memory();
}


MACHINE_START( coupe )
{
    wd17xx_init(machine, WD_TYPE_1772, NULL, NULL);
}
