/*************************************************************************

    machine/mtx.c

    Memotech MTX 500, MTX 512 and RS 128

**************************************************************************/


/* Core includes */
#include "driver.h"
#include "includes/mtx.h"

/* Components */
#include "cpu/z80/z80.h"
#include "machine/z80ctc.h"
#include "machine/z80dart.h"
#include "video/tms9928a.h"

/* Devices */
#include "devices/printer.h"
#include "devices/snapquik.h"


#define MTX_PRT_BUSY		1
#define MTX_PRT_NOERROR		2
#define MTX_PRT_EMPTY		4
#define MTX_PRT_SELECTED	8



/*************************************
 *
 *  Global variables
 *
 *************************************/

static UINT8 key_sense;

static UINT8 mtx_relcpmh;
static UINT8 mtx_rampage;
static UINT8 mtx_rompage;

static UINT8 *mtx_null_mem = NULL;
static UINT8 *mtx_zero_mem = NULL;

static char mtx_prt_strobe = 0;
static char mtx_prt_data = 0;

static UINT8 *mtx_tape_buf = NULL;
static int mtx_tape_index = 0;
static char *mtx_tape_filename = NULL;



/*************************************
 *
 *  TMS9929A video chip
 *
 *************************************/

static void mtx_tms9929a_interrupt(int data)
{
//  logerror ("tms9929a_interrupt: %d\n", data);
	z80ctc_0_trg0_w(0, data ? 0 : 1);
}

static const TMS9928a_interface tms9928a_interface =
{
	TMS9929A,
	0x4000,
	0, 0,
	mtx_tms9929a_interrupt
};

INTERRUPT_GEN( mtx_interrupt )
{
	TMS9928A_interrupt();
}



/*************************************
 *
 *  Snapshots
 *
 *************************************/

SNAPSHOT_LOAD( mtx )
{
	UINT8 header[4];

	/* get the header */
	image_fread(image, &header, sizeof(header));

	return INIT_FAIL;
}



/*************************************
 *
 *  Cassette
 *
 *************************************/

READ8_HANDLER( mtx_cst_r )
{
	return 0xff;
}

WRITE8_HANDLER( mtx_cst_w )
{
}



/*************************************
 *
 *  Printer
 *
 *************************************/

static mess_image *mtx_printer_image(void)
{
	return image_from_devtype_and_index(IO_PRINTER, 0);
}

READ8_HANDLER( mtx_strobe_r )
{
	if (mtx_prt_strobe == 0)
		printer_output (mtx_printer_image (), mtx_prt_data);

	mtx_prt_strobe = 1;

	return 0;
}


READ8_HANDLER( mtx_prt_r )
{
	mtx_prt_strobe = 0;

	return MTX_PRT_NOERROR | (printer_status (mtx_printer_image (), 0)
			? MTX_PRT_SELECTED : 0);
}

WRITE8_HANDLER( mtx_prt_w )
{
	mtx_prt_data = data;
}



/*************************************
 *
 *  Keyboard
 *
 *************************************/

WRITE8_HANDLER( mtx_sense_w )
{
	key_sense = data;
}

READ8_HANDLER( mtx_key_lo_r )
{
	UINT8 data = 0xff;

	if (!(key_sense & 0x01)) data &= readinputportbytag("keyboard_low_0");
	if (!(key_sense & 0x02)) data &= readinputportbytag("keyboard_low_1");
	if (!(key_sense & 0x04)) data &= readinputportbytag("keyboard_low_2");
	if (!(key_sense & 0x08)) data &= readinputportbytag("keyboard_low_3");
	if (!(key_sense & 0x10)) data &= readinputportbytag("keyboard_low_4");
	if (!(key_sense & 0x20)) data &= readinputportbytag("keyboard_low_5");
	if (!(key_sense & 0x40)) data &= readinputportbytag("keyboard_low_6");
	if (!(key_sense & 0x80)) data &= readinputportbytag("keyboard_low_7");

	return data;
}

READ8_HANDLER( mtx_key_hi_r )
{
	UINT8 data = readinputportbytag("country_code");

	if (!(key_sense & 0x01)) data &= readinputportbytag("keyboard_high_0");
	if (!(key_sense & 0x02)) data &= readinputportbytag("keyboard_high_1");
	if (!(key_sense & 0x04)) data &= readinputportbytag("keyboard_high_2");
	if (!(key_sense & 0x08)) data &= readinputportbytag("keyboard_high_3");
	if (!(key_sense & 0x10)) data &= readinputportbytag("keyboard_high_4");
	if (!(key_sense & 0x20)) data &= readinputportbytag("keyboard_high_5");
	if (!(key_sense & 0x40)) data &= readinputportbytag("keyboard_high_6");
	if (!(key_sense & 0x80)) data &= readinputportbytag("keyboard_high_7");

	return data;
}



/*************************************
 *
 *  Z80 CTC
 *
 *************************************/

static void mtx_ctc_interrupt(int state)
{
//  logerror("mtx_ctc_interrupt: %02x\n", state);
	cpunum_set_input_line(0, 0, state);
}

READ8_HANDLER( mtx_ctc_r )
{
	return z80ctc_0_r(offset);
}

WRITE8_HANDLER( mtx_ctc_w )
{
//  logerror("mtx_ctc_w: %02x\n", data);
	if (offset < 3)
		z80ctc_0_w(offset,data);
}

static z80ctc_interface mtx_ctc_intf =
{
	MTX_SYSTEM_CLOCK,
	0,
	mtx_ctc_interrupt,
	0,
	0,
	0
};



/*************************************
 *
 *  Z80 Dart
 *
 *************************************/

READ8_HANDLER( mtx_dart_data_r )
{
	return z80dart_d_r(0, offset);
}

READ8_HANDLER( mtx_dart_control_r )
{
	return z80dart_c_r(0, offset);
}

WRITE8_HANDLER( mtx_dart_data_w )
{
	z80dart_d_w(0, offset, data);
}

WRITE8_HANDLER( mtx_dart_control_w )
{
	z80dart_c_w(0, offset, data);
}

static const z80dart_interface mtx_dart_intf =
{
		MTX_SYSTEM_CLOCK,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
};



/*************************************
 *
 *  Bank switching
 *
 *************************************/

static void mtx_set_bank_offsets (unsigned int bank1, unsigned int bank2,
	                          unsigned int bank3, unsigned int bank4,
	                          unsigned int bank5, unsigned int bank6,
	                          unsigned int bank7, unsigned int bank8)
{
	UINT8 *romimage;

//  logerror ("CPM %d  RAM %x  ROM %x\n", mtx_relcpmh,
//          mtx_rampage, mtx_rompage);
//  logerror ("map: [0000] %04x [2000] %04x [4000] %04x [6000] %04x\n",
//          bank1, bank2, bank3, bank4);
//  logerror ("     [8000] %04x [a000] %04x [c000] %04x [e000] %04x\n",
//          bank5, bank6, bank7, bank8);

	romimage = memory_region(REGION_CPU1);

	if (mtx_relcpmh)
	{
		memory_set_bankptr (1, mess_ram + bank1);
		// bank 9 is handled by the mtx_trap_write function
		memory_set_bankptr (2, mess_ram + bank2);
		memory_set_bankptr (10, mess_ram + bank2);
	}
	else
	{
		memory_set_bankptr (1, romimage + bank1);
		// bank is 9 handled by the mtx_trap_write function
		memory_set_bankptr (2, romimage + bank2);
		memory_set_bankptr (10, mtx_null_mem);
	}

	if (bank3 < mess_ram_size)
	{
		memory_set_bankptr (3, mess_ram + bank3);
		memory_set_bankptr (11, mess_ram + bank3);
	}
	else
	{
		memory_set_bankptr (3, mtx_null_mem);
		memory_set_bankptr (11, mtx_zero_mem);
	}

	if (bank4 < mess_ram_size)
	{
		memory_set_bankptr (4, mess_ram + bank4);
		memory_set_bankptr (12, mess_ram + bank4);
	}
	else
	{
		memory_set_bankptr (4, mtx_null_mem);
		memory_set_bankptr (12, mtx_zero_mem);
	}

	if (bank5 < mess_ram_size)
	{
		memory_set_bankptr (5, mess_ram + bank5);
		memory_set_bankptr (13, mess_ram + bank5);
	}
	else
	{
		memory_set_bankptr (5, mtx_null_mem);
		memory_set_bankptr (13, mtx_zero_mem);
	}

	if (bank6 < mess_ram_size)
	{
		memory_set_bankptr (6, mess_ram + bank6);
		memory_set_bankptr (14, mess_ram + bank6);
	}
	else
	{
		memory_set_bankptr (6, mtx_null_mem);
		memory_set_bankptr (14, mtx_zero_mem);
	}

	memory_set_bankptr (7, mess_ram + bank7);
	memory_set_bankptr (15, mess_ram + bank7);

	memory_set_bankptr (8, mess_ram + bank8);
	memory_set_bankptr (16, mess_ram + bank8);
}

WRITE8_HANDLER( mtx_bankswitch_w )
{
	unsigned int bank1, bank2, bank3, bank4;
	unsigned int bank5, bank6, bank7, bank8;

	mtx_relcpmh = (data & 0x80) >> 7;
	mtx_rampage = (data & 0x0f);
	mtx_rompage = (data & 0x70) >> 4;

	if (mtx_relcpmh)
	{
		bank1 = 0xe000 + mtx_rampage * 0xc000;
		bank2 = 0xc000 + mtx_rampage * 0xc000;
		bank3 = 0xa000 + mtx_rampage * 0xc000;
		bank4 = 0x8000 + mtx_rampage * 0xc000;
		bank5 = 0x6000 + mtx_rampage * 0xc000;
		bank6 = 0x4000 + mtx_rampage * 0xc000;
	}
	else
	{
		bank1 = 0x0;
		bank2 = 0x2000 + mtx_rompage * 0x2000;
		bank3 = 0xa000 + mtx_rampage * 0x8000;
		bank4 = 0x8000 + mtx_rampage * 0x8000;
		bank5 = 0x6000 + mtx_rampage * 0x8000;
		bank6 = 0x4000 + mtx_rampage * 0x8000;
	}

	bank7 = 0x2000;
	bank8 = 0x0;

	mtx_set_bank_offsets (bank1, bank2, bank3, bank4,
                              bank5, bank6, bank7, bank8);
}

static void mtx_virt_to_phys (const char * what, int vaddress,
		int * ramspace, int * paddress)
{
	int offset = vaddress & 0x1fff;
	int vbase  = vaddress & ~0x1fff;
	int pbase = 0x20000;

	*ramspace = mtx_relcpmh || vaddress >= 0x4000;

	if (0xc000 <= vaddress && vaddress < 0x10000)
		pbase = 0xe000 - vbase;
	else if (mtx_relcpmh)
		pbase = 0xe000 - vbase + mtx_rampage * 0xc000;
	else if (0 <= vaddress && vaddress < 0x2000)
		pbase = 0;
	else if (0x2000 <= vaddress && vaddress < 0x4000)
		pbase = 0x2000 + mtx_rompage * 0x2000;
	else if (0x4000 <= vaddress && vaddress < 0xc000)
		pbase = (0xe000 - vbase) + mtx_rampage * 0x8000;

	*paddress = pbase + offset;

//  logerror ("%s (%d,%d,%d,%04x) -> (%d,%06x)\n", what,
//          mtx_relcpmh, mtx_rompage, mtx_rampage, vaddress,
//          *ramspace, *paddress);
}



/*************************************
 *
 *  Tape hack
 *
 *************************************/

static UINT8 mtx_peek(int vaddress)
{
	UINT8 * romimage;
	int ramspace;
	int paddress;

	romimage = memory_region(REGION_CPU1);
	mtx_virt_to_phys("peek", vaddress, &ramspace, &paddress);

	if (paddress > mess_ram_size) {
		logerror("peek into non-existing memory\n");
		return 0;
	}

	if (ramspace)
		return mess_ram[paddress];
	else
		return romimage[paddress];
}

static void mtx_poke(int vaddress, UINT8 data)
{
	int ramspace;
	int paddress;

	mtx_virt_to_phys ("poke", vaddress, &ramspace, &paddress);
	if (!ramspace)
		logerror ("poke into the ROM address space\n");
	else if (paddress >= mess_ram_size)
		logerror ("poke into non-existing RAM\n");
	else
    		mess_ram[paddress] = data;
}

/*
 * A filename is at most 14 characters long, always ending with a space.
 * The save file can be at most 65536 long.
 * Note: the empty string is saved as a single space.
 */
static void mtx_save_hack(int start, int length)
{
	mame_file * tape_file;
	file_error filerr;
	int bytes_saved;
	int i;

	assert(length <= 32768);

//  logerror("mtx_save_hack: start=%#x  length=%#x (%d)  index=%#x (%d)\n",
//          start, length, length, mtx_save_index, mtx_save_index);

	if ((start > 0xc000) && (length == 20))  /* Save the header segment */
	{
		for (i = 0; i < 18; i++)
			mtx_tape_buf[i] = mtx_peek(start + i);
		length = 18;

		memcpy(mtx_tape_filename, mtx_tape_buf + 1, 15);
		for (i = 14; i > 0 && mtx_tape_filename[i] == 0x20; i--)
			;
		mtx_tape_filename[i + 1] = '\0';
		mtx_tape_index = 0;

		filerr = mame_fopen(SEARCHPATH_IMAGE, mtx_tape_filename,
                        	OPEN_FLAG_WRITE|OPEN_FLAG_CREATE, &tape_file);
	}
	else  /* Save system variable, body, or variable store segment */
	{
		for (i = 0; i < length; i++)
			mtx_tape_buf[i] = mtx_peek(start + i);

		filerr = mame_fopen(SEARCHPATH_IMAGE, mtx_tape_filename,
    	                    OPEN_FLAG_WRITE, &tape_file);
	}

	if (filerr == FILERR_NONE)
	{
		mame_fseek(tape_file, mtx_tape_index, SEEK_SET);
		bytes_saved = mame_fwrite(tape_file, mtx_tape_buf, length);
		logerror("saved %d bytes from %d:%04x into '%s' at %d\n", bytes_saved,
		         mtx_rampage, start, mtx_tape_filename, mtx_tape_index);
		if (length > bytes_saved)
			logerror("wrote too few bytes from '%s'\n", mtx_tape_filename);

		mame_fclose(tape_file);
		mtx_tape_index += length;
	}
	else
		logerror("cannot open '%s' for saving\n", mtx_tape_filename);
}

/*
 * A filename is at most 14 characters long, ending with a space.
 * At most 65536 bytes are loaded from the file.
 * Note: the empty string is loaded as a single space (ugly),
 *       not the 'next' file.
 */
static void mtx_load_hack(int start, int length)
{
	mame_file * tape_file;
	file_error filerr;
	int bytes_loaded;
	int i;

	assert(length <= 32768);

//  logerror("mtx_load_hack: start=%#x  length=%#x (%d)\n", start, length, length);

	if ((start > 0xc000) && (length == 18))
	{
		for (i = 0; i < 15; i++)
			mtx_tape_filename[i] = mtx_peek(start - 0xf + i);

		for (i = 14; i > 0 && mtx_tape_filename[i] == 0x20; i--)
			;

		mtx_tape_filename[i+1] = '\0';
		mtx_tape_index = 0;
	}

	filerr = mame_fopen(SEARCHPATH_IMAGE, mtx_tape_filename, OPEN_FLAG_READ,
	                    &tape_file);
	if (filerr == FILERR_NONE)
	{
		mame_fseek(tape_file, mtx_tape_index, SEEK_SET);
		bytes_loaded = mame_fread(tape_file, mtx_tape_buf, length);
		logerror("loaded %d bytes from '%s' at %d into %d:%04x\n", bytes_loaded,
		         mtx_tape_filename, mtx_tape_index, mtx_rampage, start);
		if (length > bytes_loaded)
			logerror("read too few bytes from '%s'\n", mtx_tape_filename);

		for (i = 0; i < length; i++)
			mtx_poke(start + i, mtx_tape_buf[i]);

		mame_fclose(tape_file);
		mtx_tape_index += length;
	}
	else
		logerror("cannot open '%s' for loading\n", mtx_tape_filename);
}

static void mtx_verify_hack(int start, int length)
{
//  logerror("mtx_verify_hack: start=0x%x, length=0x%x (%d)  not implemented\n", start, length, length);
}

WRITE8_HANDLER( mtx_trap_write )
{
	int pc;
	int start;
	int length;

	if (mtx_relcpmh)
	{
		mtx_poke(offset, data);
		return;
	}

	pc = activecpu_get_reg(Z80_PC);
	if((offset == 0x0aae) && (pc == 0x0ab1))
	{
		start = activecpu_get_reg(Z80_HL);
		length = activecpu_get_reg(Z80_DE);

		// logerror("PC %04x\nStart %04x, Length %04x, 0xFD67 %02x, 0xFD68 %02x index 0x%04x\n", pc, start, length, mess_ram[0xfd67], mess_ram[0xfd68], mtx_tape_size);

		if(mtx_peek(0xfd68) == 0)
			mtx_save_hack(start, length);
		else if(mtx_peek(0xfd67) == 0)
			mtx_load_hack(start, length);
		else
			mtx_verify_hack(start, length);
	}
}



/*************************************
 *
 *  Machine initialization
 *
 *************************************/

MACHINE_START( mtx512 )
{
	UINT8 * romimage;

	TMS9928A_configure(&tms9928a_interface);

	mtx_null_mem = (UINT8 *) auto_malloc (16384);
	mtx_zero_mem = (UINT8 *) auto_malloc (16384);
	memset (mtx_zero_mem, 0, 16384);

	mtx_tape_buf = auto_malloc (32768);
	mtx_tape_index = 0;
	mtx_tape_filename = auto_malloc (16);
	mtx_tape_filename[0] = 0;

	z80ctc_init(0, &mtx_ctc_intf);

	// Patch the Rom (Sneaky........ Who needs to trap opcodes?)
	romimage = memory_region(REGION_CPU1);
	romimage[0x0aae] = 0x32;	// ld ($0aae),a
	romimage[0x0aaf] = 0xae;
	romimage[0x0ab0] = 0x0a;
	romimage[0x0ab1] = 0xc9;	// ret

	// Set up the starting memory configuration
	mtx_relcpmh = 0;
	mtx_rampage = 0;
	mtx_rompage = 0;
	mtx_set_bank_offsets (0, 0x2000, 0xa0000, 0x8000,
			      0x6000, 0x4000, 0x2000, 0);
}

DRIVER_INIT( rs128 )
{
	z80dart_init(0, &mtx_dart_intf);

	/* install handlers for dart interface */
	memory_install_readwrite8_handler(0, ADDRESS_SPACE_IO, 0x0c, 0x0d, 0, 0, mtx_dart_data_r, mtx_dart_data_w);
	memory_install_readwrite8_handler(0, ADDRESS_SPACE_IO, 0x0e, 0x0f, 0, 0, mtx_dart_control_r, mtx_dart_control_w);
}

MACHINE_RESET( rs128 )
{
	z80dart_reset(0);
}
