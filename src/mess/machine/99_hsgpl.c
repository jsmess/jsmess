/*
    SNUG HSGPL card emulation.

    Raphael Nabet, 2003.
*/

#include "driver.h"
#include "ti99_4x.h"
#include "99_peb.h"
#include "99_hsgpl.h"
#include "at29040.h"

/*
    Supports 16 banks of 8 GROMs (8kbytes each) with 16 associated banks of
    32kbytes (8kbytes*4) of module ROM, 2 banks of 8 GRAMs with 2 associated
    banks of 32 kbytes of RAM, and 512kbytes of DSR.  Roms are implemented with
    512kbyte EEPROMs (1 for DSR, 2 for GROMs, 1 for cartridge ROM).  RAM is
    implemented with 128kbyte SRAMs (1 for GRAM, 1 for cartridge RAM - only the
    first 64kbytes of the cartridge RAM chip is used).

    CRU bits:
       Name    Equates Meaning
    >0 DEN     DSRENA  DSR Enable
    >1 GRMENA  GRMENA  Enable GRAM instead of GROM in banks 0 and 1
    >2 BNKINH* BNKENA  Disable banking
    >3 PG0     PG0
    >4 PG1     PG1
    >5 PG2     PG2     Paging-Bits for DSR-area
    >6 PG3     PG3
    >7 PG4     PG4
    >8 PG5     PG5
    >9 CRDENA  CRDENA  Activate memory areas of HSGPL (i.e. enable HSGPL GROM and ROM6 ports)
    >A WRIENA  WRIENA  write enable for RAM and GRAM (and flash GROM!)
    >B SCENA   SCARTE  Activate SuperCart-banking
    >C LEDENA  LEDENA  light LED on
    >D -       -       free
    >E MBXENA  MBXENA  Activate MBX-Banking
    >F RAMENA  RAMENA  Enable RAM6000 instead of ROM6000 in banks 0 and 1


    Direct access ports for all memory areas (the original manual gives
    >9880->989C and >9C80->9C9C for ROM6000, but this is incorrect):

    Module bank Read    Write   Read ROM6000        Write ROM6000
                GROM    GROM
    0           >9800   >9C00   >9860 Offset >0000  >9C60 Offset >0000
    1           >9804   >9C04   >9860 Offset >8000  >9C60 Offset >8000
    2           >9808   >9C08   >9864 Offset >0000  >9C64 Offset >0000
    3           >980C   >9C0C   >9864 Offset >8000  >9C64 Offset >8000
    4           >9810   >9C10   >9868 Offset >0000  >9C68 Offset >0000
    5           >9814   >9C14   >9868 Offset >8000  >9C68 Offset >8000
    6           >9818   >9C18   >986C Offset >0000  >9C6C Offset >0000
    7           >981C   >9C1C   >986C Offset >8000  >9C6C Offset >8000
    8           >9820   >9C20   >9870 Offset >0000  >9C70 Offset >0000
    9           >9824   >9C24   >9870 Offset >8000  >9C70 Offset >8000
    10          >9828   >9C28   >9874 Offset >0000  >9C74 Offset >0000
    11          >982C   >9C2C   >9874 Offset >8000  >9C74 Offset >8000
    12          >9830   >9C30   >9878 Offset >0000  >9C78 Offset >0000
    13          >9834   >9C34   >9878 Offset >8000  >9C78 Offset >8000
    14          >9838   >9C38   >987C Offset >0000  >9C7C Offset >0000
    15          >983C   >9C3C   >987C Offset >8000  >9C7C Offset >8000

    Module bank Read    Write   Read RAM6000        Write RAM6000
                GRAM    GRAM
    16 (Ram)    >9880   >9C80   >98C0 Offset >0000  >9CC0 Offset >0000
    17 (Ram)    >9884   >9C84   >98C0 Offset >8000  >9CC0 Offset >8000

    DSR bank    Read    Write
    0 - 7       >9840   >9C40
    8 - 15      >9844   >9C44
    16 - 23     >9848   >9C48
    24 - 31     >984C   >9C4C
    32 - 39     >9850   >9C50
    40 - 47     >9854   >9C54
    48 - 55     >9858   >9C58
    56 - 63     >985C   >9C5C

    Note: Writing only works for areas set up as RAM.  To write to the
        FEEPROMs, you must used the algorithm specified by their respective
        manufacturer.
*/

static struct
{
	UINT8 *GRAM_ptr;
	UINT8 *RAM6_ptr;

	UINT16 addr;
	char raddr_LSB, waddr_LSB;

	int cur_port;

	int cur_bank;

	int cru_reg;

	int is_image_writable;
} hsgpl;

enum
{
	cr_grmena  = 0x0002,
	cr_bnkinh  = 0x0004,
	cr_pg      = 0x01f8,
		cr_pg_mask = 0x3f,
		cr_pg_shift= 3,
	cr_crdena  = 0x0200,
		cr_crdena_bit = 9,
	cr_wriena  = 0x0400,
	cr_scena   = 0x0800,
	cr_ledena  = 0x1000,
	cr_free    = 0x2000,
	cr_mbxena  = 0x4000,
	cr_ramena  = 0x8000
};

static void hsgpl_cru_w(running_machine *machine, int offset, int data);
static READ8_HANDLER(hsgpl_dsr_r);

static const ti99_peb_card_handlers_t hsgpl_handlers =
{
	NULL,
	hsgpl_cru_w,
	hsgpl_dsr_r,
	NULL
};

enum
{
	feeprom_grom0 = 0,
	feeprom_grom1 = 1,
	feeprom_rom6  = 2,
	feeprom_dsr   = 3
};


static int ti99_hsgpl_file_load(running_machine *machine, mame_file *file)
{
	UINT8 version;
	UINT8 *rgn;


	/* version flag */
	if (mame_fread(file, & version, 1) != 1)
		return 1;
	if (version != 0)
		return 1;

	rgn = memory_region(machine, region_hsgpl);

	/* read DSR */
	at29c040a_init_data_ptr(feeprom_dsr, rgn + offset_hsgpl_dsr);
	if (at29c040a_file_load(feeprom_dsr, file))
		return 1;

	/* read GROM 0 */
	at29c040a_init_data_ptr(feeprom_grom0, rgn + offset_hsgpl_grom);
	if (at29c040a_file_load(feeprom_grom0, file))
		return 1;

	/* read GROM 1 */
	at29c040a_init_data_ptr(feeprom_grom1, rgn + offset_hsgpl_grom + 0x80000);
	if (at29c040a_file_load(feeprom_grom1, file))
		return 1;

	/* read ROM6 */
	at29c040a_init_data_ptr(feeprom_rom6, rgn + offset_hsgpl_rom6);
	if (at29c040a_file_load(feeprom_rom6, file))
		return 1;

	return 0;
}

static int ti99_hsgpl_file_save(mame_file *file)
{
	UINT8 version;


	/* version flag */
	version = 0;
	if (mame_fwrite(file, & version, 1) != 1)
		return 1;

	/* save DSR */
	if (at29c040a_file_save(feeprom_dsr, file))
		return 1;

	/* save GROM 0 */
	if (at29c040a_file_save(feeprom_grom0, file))
		return 1;

	/* save GROM 1 */
	if (at29c040a_file_save(feeprom_grom1, file))
		return 1;

	/* save ROM6 */
	if (at29c040a_file_save(feeprom_rom6, file))
		return 1;

	return 0;
}

static int ti99_hsgpl_get_dirty_flag(void)
{
	return at29c040a_get_dirty_flag(feeprom_dsr)
			|| at29c040a_get_dirty_flag(feeprom_grom0)
			|| at29c040a_get_dirty_flag(feeprom_grom1)
			|| at29c040a_get_dirty_flag(feeprom_rom6);
}



int ti99_hsgpl_load_memcard(running_machine *machine)
{
	file_error filerr;
	mame_file *file;

	filerr = mame_fopen(SEARCHPATH_MEMCARD, "hsgpl.nv", OPEN_FLAG_READ, &file);
	if (filerr != FILERR_NONE)
		return /*1*/0;
	if (ti99_hsgpl_file_load(machine, file))
	{
		mame_fclose(file);
		return 1;
	}

	mame_fclose(file);
	return 0;
}

int ti99_hsgpl_save_memcard(running_machine *machine)
{
	file_error filerr;
	mame_file *file;

	if (ti99_hsgpl_get_dirty_flag())
	{
		filerr = mame_fopen(SEARCHPATH_MEMCARD, "hsgpl.nv", OPEN_FLAG_WRITE, &file);
		if (filerr != FILERR_NONE)
			return 1;
		if (ti99_hsgpl_file_save(file))
		{
			mame_fclose(file);
			return 1;
		}

		mame_fclose(file);
	}

	return 0;
}

/*
    Initialize hsgpl card, set up handlers
*/
void ti99_hsgpl_init(running_machine *machine)
{
	UINT8 *rgn = memory_region(machine, region_hsgpl);
	hsgpl.GRAM_ptr = rgn + offset_hsgpl_gram;
	hsgpl.RAM6_ptr = rgn + offset_hsgpl_ram6;

	at29c040a_init_data_ptr(feeprom_grom0, rgn + offset_hsgpl_grom);
	at29c040a_init(machine,feeprom_grom0);
	at29c040a_init_data_ptr(feeprom_grom1, rgn + offset_hsgpl_grom + 0x80000);
	at29c040a_init(machine,feeprom_grom1);
	at29c040a_init_data_ptr(feeprom_rom6, rgn + offset_hsgpl_rom6);
	at29c040a_init(machine,feeprom_rom6);
	at29c040a_init_data_ptr(feeprom_dsr, rgn + offset_hsgpl_dsr);
	at29c040a_init(machine,feeprom_dsr);
}

/*
    Reset hsgpl card
*/
void ti99_hsgpl_reset(running_machine *machine)
{
	hsgpl.addr = 0;
	hsgpl.raddr_LSB = 0;
	hsgpl.waddr_LSB = 0;
	hsgpl.cur_port = 0;
	hsgpl.cur_bank = 0;
	hsgpl.cru_reg = 0;
	ti99_set_hsgpl_crdena(/*0*/1);

	ti99_peb_set_card_handlers(0x1b00, & hsgpl_handlers);
}


/*
    Write hsgpl CRU interface
*/
static void hsgpl_cru_w(running_machine *machine, int offset, int data)
{
	offset &= 0xf;

	if (data)
		hsgpl.cru_reg |= 1 << offset;
	else
		hsgpl.cru_reg &= ~ (1 << offset);

	if (offset == cr_crdena_bit)
		ti99_set_hsgpl_crdena(data);
}

/*
    read a byte in hsgpl DSR space
*/
static  READ8_HANDLER(hsgpl_dsr_r)
{
	int dsr_page = (hsgpl.cru_reg >> cr_pg_shift) & cr_pg_mask;

	return at29c040a_r(feeprom_dsr, offset + 0x2000 * dsr_page);
}

/*
    GPL read
*/
READ16_HANDLER ( ti99_hsgpl_gpl_r )
{
	int port;
	int reply;

	//activecpu_adjust_icount(-4);

	port = hsgpl.cur_port = (offset & 0x1FE) >> 1;

	if (offset & 1)
	{	/* read GPL address */
		hsgpl.waddr_LSB = FALSE;	/* right??? */

		if (hsgpl.raddr_LSB)
		{
			reply = ((hsgpl.addr + 1) << 8) & 0xff00;
			hsgpl.raddr_LSB = FALSE;
		}
		else
		{
			reply = (hsgpl.addr + 1) & 0xff00;
			hsgpl.raddr_LSB = TRUE;
		}
	}
	else
	{	/* read GPL data */
		//reply = (data_ptr ? ((int) data_ptr[hsgpl.addr]) : 0) << 8;   /* read data */

		switch (port)
		{
		case 0:
		case 1:
			if (hsgpl.cru_reg & cr_grmena)
			{
				reply = hsgpl.GRAM_ptr[hsgpl.addr + 0x10000*port];
				break;
			}
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			reply = at29c040a_r(feeprom_grom0, hsgpl.addr + 0x10000*port);
			break;

		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			reply = at29c040a_r(feeprom_grom1, hsgpl.addr + 0x10000*(port-8));
			break;

		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
			reply = at29c040a_r(feeprom_dsr, hsgpl.addr + 0x10000*(port-16));
			break;

		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
		case 31:
			/* The HSGPL software manual says 32-47, but this is incorrect */
			reply = at29c040a_r(feeprom_rom6, hsgpl.addr + 0x10000*(port-24));
			break;

		case 32:
		case 33:
			/* This has been verified */
			reply = hsgpl.GRAM_ptr[hsgpl.addr + 0x10000*(port-32)];
			break;

		case 48:
		case 49:		/* this mirror is mistakenly used by the hsgpl DSR */
			/* This has been verified. */
			reply = hsgpl.RAM6_ptr[hsgpl.addr];
			break;

		default:
			logerror("unknown GPL port\n");
			reply = 0;
			break;
		}

		reply <<= 8;


		/* increment address */
		hsgpl.addr++;
		hsgpl.raddr_LSB = hsgpl.waddr_LSB = FALSE;
	}

	return reply;
}

/*
    GPL write
*/
WRITE16_HANDLER ( ti99_hsgpl_gpl_w )
{
	int port;


	//activecpu_adjust_icount(-4);

	port = hsgpl.cur_port = (offset & 0x1FE) >> 1;

	if (offset & 1)
	{	/* write GPL address */
		hsgpl.raddr_LSB = FALSE;

		if (hsgpl.waddr_LSB)
		{
			hsgpl.addr = (hsgpl.addr & 0xFF00) | ((data >> 8) & 0xFF);

			hsgpl.waddr_LSB = FALSE;
		}
		else
		{
			hsgpl.addr = (data & 0xFF00) | (hsgpl.addr & 0xFF);

			hsgpl.waddr_LSB = TRUE;
		}
	}
	else
	{	/* write GPL data */
		data = (data >> 8) & 0xFF;

		if (hsgpl.cru_reg & cr_wriena)
		{
			switch (port)
			{
			case 0:
			case 1:
				if (hsgpl.cru_reg & cr_grmena)
				{
					hsgpl.GRAM_ptr[hsgpl.addr + 0x10000*port] = data;
					break;
				}
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
				at29c040a_w(feeprom_grom0, hsgpl.addr + 0x10000*port, data);
				break;

			case 8:
			case 9:
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
				at29c040a_w(feeprom_grom1, hsgpl.addr + 0x10000*(port-8), data);
				break;

			case 16:
			case 17:
			case 18:
			case 19:
			case 20:
			case 21:
			case 22:
			case 23:
				at29c040a_w(feeprom_dsr, hsgpl.addr + 0x10000*(port-16), data);
				break;

			case 24:
			case 25:
			case 26:
			case 27:
			case 28:
			case 29:
			case 30:
			case 31:
				/* The HSGPL manual says 32-47, but this is incorrect */
				at29c040a_w(feeprom_rom6, hsgpl.addr + 0x10000*(port-24), data);
				break;

			case 32:
			case 33:
				/* This has been verified */
				hsgpl.GRAM_ptr[hsgpl.addr + 0x10000*(port-32)] = data;
				break;

			case 48:
			case 49:		/* this mirror is mistakenly used by the hsgpl DSR */
				/* This has been verified. */
				hsgpl.RAM6_ptr[hsgpl.addr] = data;
				break;

			default:
				logerror("unknown GPL port\n");
				break;
			}
		}

		hsgpl.addr++;
		hsgpl.raddr_LSB = hsgpl.waddr_LSB = FALSE;
	}
}

/*
    Cartridge space read
*/
READ16_HANDLER ( ti99_hsgpl_rom6_r )
{
	int port = hsgpl.cur_port;
	int reply;

	switch (port)
	{
	case 0:
	case 1:
		if (hsgpl.cru_reg & cr_ramena)
		{
			reply = hsgpl.RAM6_ptr[1 + 2*offset + 0x2000*hsgpl.cur_bank + 0x8000*port];
			reply |= hsgpl.RAM6_ptr[2*offset + 0x2000*hsgpl.cur_bank + 0x8000*port] << 8;
			break;
		}
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		reply = at29c040a_r(feeprom_rom6, 1 + 2*offset + 0x2000*hsgpl.cur_bank + 0x8000*port);
		reply |= at29c040a_r(feeprom_rom6, 2*offset + 0x2000*hsgpl.cur_bank + 0x8000*port) << 8;
		break;

	case 32:
	case 33:
		reply = hsgpl.RAM6_ptr[1 + 2*offset + 0x2000*hsgpl.cur_bank + 0x8000*(port-32)];
		reply |= hsgpl.RAM6_ptr[2*offset + 0x2000*hsgpl.cur_bank + 0x8000*(port-32)] << 8;
		break;

	default:
		logerror("unknown >6000 port\n");
		reply = 0;
		break;
	}

	return reply;
}

/*
    Cartridge space write
*/
WRITE16_HANDLER ( ti99_hsgpl_rom6_w )
{
	int port = hsgpl.cur_port;

	if (! (hsgpl.cru_reg & cr_bnkinh))
	{
		hsgpl.cur_bank = offset & 3;
		return;		/* right??? */
	}

	if ((hsgpl.cru_reg & cr_mbxena) && (offset == 0x07ff) && ACCESSING_BITS_8_15)
	{	/* MBX: mapper at 0x6ffe */
		hsgpl.cur_bank = (data >> 8) & 0x3;
		return;
	}

	/* MBX: RAM in 0x6c00-0x6ffd (it is unclear whether the MBX RAM area is
    enabled/disabled by the wriena bit).  I guess RAM is unpaged, but it is
    not implemented */
	if ((hsgpl.cru_reg & cr_wriena) || ((hsgpl.cru_reg & cr_mbxena) && (offset >= 0x0600) && (offset <= 0x07fe)))
	{
		switch (port)
		{
		case 0:
		case 1:
			if (hsgpl.cru_reg & cr_ramena)
			{
				if (ACCESSING_BITS_0_7)
					hsgpl.RAM6_ptr[1 + 2*offset + 0x2000*hsgpl.cur_bank + 0x8000*port] = data;
				if (ACCESSING_BITS_8_15)
					hsgpl.RAM6_ptr[2*offset + 0x2000*hsgpl.cur_bank + 0x8000*port] = data >> 8;
				break;
			}
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			/* feeprom is normally written to using GPL ports, and I don't know
            writing through >6000 page is enabled */
			/*at29c040a_w(feeprom_rom6, 1 + 2*offset + 0x2000*hsgpl.cur_bank + 0x8000*port, data);
            at29c040a_w(feeprom_rom6, 2*offset + 0x2000*hsgpl.cur_bank + 0x8000*port, data >> 8);*/
			break;

		case 32:
		case 33:
			if (ACCESSING_BITS_0_7)
				hsgpl.RAM6_ptr[1 + 2*offset + 0x2000*hsgpl.cur_bank + 0x8000*(port-32)] = data;
			if (ACCESSING_BITS_8_15)
				hsgpl.RAM6_ptr[2*offset + 0x2000*hsgpl.cur_bank + 0x8000*(port-32)] = data >> 8;
			break;

		default:
			logerror("unknown >6000 port\n");
			break;
		}
	}
}
