/***************************************************************************

 SAM Coupe Driver - Written By Lee Hammerton

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "deprecat.h"
#include "includes/samcoupe.h"
#include "devices/basicdsk.h"
#include "machine/wd17xx.h"
#include "machine/msm6242.h"


#define LMPR_RAM0    0x20	/* If bit set ram is paged into bank 0, else its rom0 */
#define LMPR_ROM1    0x40	/* If bit set rom1 is paged into bank 3, else its ram */
#define HMPR_MCNTRL  0x80	/* If set external RAM is enabled */


struct samcoupe_asic samcoupe_regs;


static void samcoupe_update_bank(int bank, UINT8 *memory, int is_readonly)
{
	read8_machine_func rh = SMH_NOP;
	write8_machine_func wh = SMH_NOP;

	if (memory)
	{
		memory_set_bankptr(bank, memory);
		rh = (read8_machine_func) (STATIC_BANK1 + (FPTR)bank - 1);
		wh = is_readonly ? SMH_UNMAP : (write8_machine_func) (STATIC_BANK1 + (FPTR)bank - 1);
	}

	memory_install_read8_handler(Machine, 0,  ADDRESS_SPACE_PROGRAM, ((bank-1) * 0x4000), ((bank-1) * 0x4000) + 0x3FFF, 0, 0, rh);
	memory_install_write8_handler(Machine, 0, ADDRESS_SPACE_PROGRAM, ((bank-1) * 0x4000), ((bank-1) * 0x4000) + 0x3FFF, 0, 0, wh);
}


static void samcoupe_install_ext_mem(void)
{
	UINT8 *mem;

	/* bank 3 */
	if (samcoupe_regs.lext >> 6 < mess_ram_size >> 20)
		mem = &mess_ram[(mess_ram_size & 0xfffff) + (samcoupe_regs.lext >> 6) * 0x100000 + (samcoupe_regs.lext & 0x3f) * 0x4000];
	else
		mem = NULL;

	samcoupe_update_bank(3, mem, FALSE);

	/* bank 4 */
	if (samcoupe_regs.hext >> 6 < mess_ram_size >> 20)
		mem = &mess_ram[(mess_ram_size & 0xfffff) + (samcoupe_regs.hext >> 6) * 0x100000 + (samcoupe_regs.hext & 0x3f) * 0x4000];
	else
		mem = NULL;

	samcoupe_update_bank(4, mem, FALSE);
}


void samcoupe_update_memory(void)
{
	const int PAGE_MASK = ((mess_ram_size & 0xfffff) / 0x4000) - 1;
	UINT8 *rom = memory_region(REGION_CPU1);
	UINT8 *memory;
	int is_readonly;

	/* BANK1 */
    if (samcoupe_regs.lmpr & LMPR_RAM0)   /* Is ram paged in at bank 1 */
	{
		if ((samcoupe_regs.lmpr & 0x1F) <= PAGE_MASK)
			memory = &mess_ram[(samcoupe_regs.lmpr & PAGE_MASK) * 0x4000];
		else
			memory = NULL;	/* Attempt to page in non existant ram region */
		is_readonly = FALSE;
	}
	else
	{
		memory = rom;	/* Rom0 paged in */
		is_readonly = TRUE;
	}
	samcoupe_update_bank(1, memory, is_readonly);


	/* BANK2 */
	if (((samcoupe_regs.lmpr + 1) & 0x1f) <= PAGE_MASK)
		memory = &mess_ram[((samcoupe_regs.lmpr + 1) & PAGE_MASK) * 0x4000];
	else
		memory = NULL;	/* Attempt to page in non existant ram region */
	samcoupe_update_bank(2, memory, FALSE);

	/* only update bank 3 and 4 when external memory is not enabled */
	if (samcoupe_regs.hmpr & HMPR_MCNTRL)
	{
		samcoupe_install_ext_mem();
	}
	else
	{
		/* BANK3 */
		if ((samcoupe_regs.hmpr & 0x1F) <= PAGE_MASK )
			memory = &mess_ram[(samcoupe_regs.hmpr & PAGE_MASK)*0x4000];
		else
			memory = NULL;	/* Attempt to page in non existant ram region */
		samcoupe_update_bank(3, memory, FALSE);


		/* BANK4 */
		if (samcoupe_regs.lmpr & LMPR_ROM1)	/* Is Rom1 paged in at bank 4 */
		{
			memory = rom + 0x4000;
			is_readonly = TRUE;
		}
		else
		{
			if (((samcoupe_regs.hmpr + 1) & 0x1f) <= PAGE_MASK)
				memory = &mess_ram[((samcoupe_regs.hmpr + 1) & PAGE_MASK) * 0x4000];
			else
				memory = NULL;	/* Attempt to page in non existant ram region */
			is_readonly = FALSE;
		}
		samcoupe_update_bank(4, memory, FALSE);
	}

	/* video memory location */
	if (samcoupe_regs.vmpr & 0x40)	/* if bit set in 2 bank screen mode */
		videoram = &mess_ram[((samcoupe_regs.vmpr & 0x1e) & PAGE_MASK) * 0x4000];
	else
		videoram = &mess_ram[((samcoupe_regs.vmpr & 0x1f) & PAGE_MASK) * 0x4000];
}


WRITE8_HANDLER( samcoupe_ext_mem_w )
{
	if (offset & 1)
		samcoupe_regs.hext = data;
	else
		samcoupe_regs.lext = data;

	/* external RAM enabled? */
	if (samcoupe_regs.hmpr & HMPR_MCNTRL)
	{
		samcoupe_install_ext_mem();
	}
}


static READ8_HANDLER( samcoupe_rtc_r )
{
	const device_config *rtc = device_list_find_by_tag(machine->config->devicelist, MSM6242, "sambus_clock");
	return msm6242_r(rtc, offset >> 12);
}


static WRITE8_HANDLER( samcoupe_rtc_w )
{
	const device_config *rtc = device_list_find_by_tag(machine->config->devicelist, MSM6242, "sambus_clock");
	msm6242_w(rtc, offset >> 12, data);
}


MACHINE_RESET( samcoupe )
{
	memset(&samcoupe_regs, 0, sizeof(samcoupe_regs));
	
    samcoupe_regs.lmpr = 0x0f;      /* ROM0 paged in, ROM1 paged out RAM Banks */
    samcoupe_regs.hmpr = 0x01;
    samcoupe_regs.vmpr = 0x81;
    samcoupe_regs.line_int = 0xff;  /* line interrupts disabled */
    samcoupe_regs.status = 0x1f;    /* no interrupts active */

	if (input_port_read(machine, "config") & 0x01)
	{
		/* install RTC */
		memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_IO, 0xef, 0xef, 0xffff, 0xff00, samcoupe_rtc_r, samcoupe_rtc_w);
	}
	else
	{
		/* no RTC support */
		memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_IO, 0xef, 0xef, 0xffff, 0xff00, SMH_UNMAP, SMH_UNMAP);
	}

    samcoupe_update_memory();
}


MACHINE_START( samcoupe )
{
    wd17xx_init(machine, WD_TYPE_1772, NULL, NULL);
}
