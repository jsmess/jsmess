/*
** mtx.c : driver for Memotech MTX512
** 
** The memory address space of the MTX512 is divided into 8 banks,
** with a size of 8KB each. These banks are mapped differently based
** upon the memory model, selected ROM and RAM page, and the amount
** of available RAM memory. There are two memory models: a ROM-based
** model and a CPM model. The memory model, ROM, and RAM page are
** selected at IO port 0 (i.e., with out (0),a)
**
** In the ROM-based model, bank 1 en bank 2 map to ROM images. Bank 1
** maps to the OSROM image; bank2 maps to either the BASICROM image
** or the ASSEMROM image, depending on the selected ROM page. The
** available RAM is allocated to the banks from top to bottom. Bank 8
** and bank 7, always start at offset 0x0 and 0x2000, respectively.
** Bank 6, 5, 4, and 3 are mapped depending on the selected RAM page
** and the available memory.
**
** In the CPM model, all banks map to RAM memory. The available RAM
** is (again) allocated from top to bottom. Bank 8 and bank 7, always
** start at offset 0x0 and 0x2000, respectively. Bank 6, 5, 4, 3, 2,
** and 1 are mapped depending on the read-only memory register and
** the avaiable memory.
*/

#include "driver.h"
#include "cpuintrf.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "video/generic.h"
#include "video/tms9928a.h"
#include "sound/sn76496.h"
#include "devices/cartslot.h"
#include "devices/printer.h"


#define MTX_SYSTEM_CLOCK		4000000

#define MTX_PRT_BUSY		1
#define MTX_PRT_NOERROR		2
#define MTX_PRT_EMPTY		4
#define MTX_PRT_SELECTED	8


static unsigned char key_sense;

static unsigned char mtx_relcpmh;
static unsigned char mtx_rampage;
static unsigned char mtx_rompage;

static unsigned char * mtx_null_mem = NULL;
static unsigned char * mtx_zero_mem = NULL;

static char mtx_prt_strobe = 0;
static char mtx_prt_data = 0;

static unsigned char * mtx_tape_buf = NULL;
static int mtx_tape_index = 0;
static char * mtx_tape_filename = NULL;


static WRITE8_HANDLER ( mtx_cst_w )
{
}

static  READ8_HANDLER ( mtx_psg_r )
{
	return 0xff;
}

static WRITE8_HANDLER ( mtx_psg_w )
{
	SN76496_0_w(offset,data);
}

static mess_image * mtx_printer_image (void)
{
	return image_from_devtype_and_index (IO_PRINTER, 0);
}

static  READ8_HANDLER ( mtx_strobe_r )
{
	if (mtx_prt_strobe == 0)
		printer_output (mtx_printer_image (), mtx_prt_data);

	mtx_prt_strobe = 1;

	return 0;
}

static  READ8_HANDLER ( mtx_prt_r )
{
	mtx_prt_strobe = 0;

	return MTX_PRT_NOERROR | (printer_status (mtx_printer_image (), 0)
			? MTX_PRT_SELECTED : 0);
}

static  WRITE8_HANDLER ( mtx_prt_w )
{
	mtx_prt_data = data;
}

static  READ8_HANDLER ( mtx_vdp_r )
{
	if (offset & 0x01)
		return TMS9928A_register_r(0);
	else
		return TMS9928A_vram_r(0);
}

static WRITE8_HANDLER ( mtx_vdp_w )
{
	if (offset & 0x01)
		TMS9928A_register_w(0, data);
	else
		TMS9928A_vram_w(0, data);
}

static WRITE8_HANDLER ( mtx_sense_w )
{
	key_sense = data;
}

static  READ8_HANDLER ( mtx_key_lo_r )
{
	unsigned char rtn = 0;

	if (key_sense==0xfe)
		rtn = readinputport(0);

	if (key_sense==0xfd)
		rtn = readinputport(1);

	if (key_sense==0xfb)
		rtn = readinputport(2);

	if (key_sense==0xf7)
		rtn = readinputport(3);

	if (key_sense==0xef)
		rtn = readinputport(4);

	if (key_sense==0xdf)
		rtn = readinputport(5);

	if (key_sense==0xbf)
		rtn = readinputport(6);

	if (key_sense==0x7f)
		rtn = readinputport(7);

	return(rtn);
}

static  READ8_HANDLER ( mtx_key_hi_r )
{
	unsigned char rtn = 0;

	unsigned char tmp;
	tmp = ((readinputport(10) & 0x03) << 2) | 0xf0;
	rtn = tmp;

	if (key_sense==0xfe)
		rtn = (readinputport(8) & 0x03) | tmp;

	if (key_sense==0xfd)
		rtn = ((readinputport(8) >> 2) & 0x03) | tmp;

	if (key_sense==0xfb)
		rtn = ((readinputport(8) >> 4) & 0x03) | tmp;

	if (key_sense==0xf7)
		rtn = ((readinputport(8) >> 6) & 0x03) | tmp;

	if (key_sense==0xef)
		rtn = (readinputport(9) & 0x03) | tmp;

	if (key_sense==0xdf)
		rtn = ((readinputport(9) >> 2) & 0x03) | tmp;

	if (key_sense==0xbf)
		rtn = ((readinputport(9) >> 4) & 0x03) | tmp;

	if (key_sense==0x7f)
		rtn = ((readinputport(9) >> 6) & 0x03) | tmp;

	return(rtn);
}

static void mtx_ctc_interrupt(int state)
{
	//logerror("interrupting ctc %02x\r\n ",state);
	cpunum_set_input_line(0, 0, state);
}

static  READ8_HANDLER ( mtx_ctc_r )
{
	return z80ctc_0_r(offset);
}

static WRITE8_HANDLER ( mtx_ctc_w )
{
	//logerror("CTC W: %02x\r\n",data);
	if (offset < 3)
		z80ctc_0_w(offset,data);
}

static z80ctc_interface	mtx_ctc_intf =
{
	MTX_SYSTEM_CLOCK,
	0,
	mtx_ctc_interrupt,
	0,
	0,
	0
};

static void mtx_tms9929A_interrupt (int data)
{
//	logerror ("tms9929A_interrupt: %d\n", data);
	z80ctc_0_trg0_w (0, data ? 0 : 1);
}

static void mtx_set_bank_offsets (unsigned int bank1, unsigned int bank2, 
	                          unsigned int bank3, unsigned int bank4,
	                          unsigned int bank5, unsigned int bank6,
	                          unsigned int bank7, unsigned int bank8)
{
	unsigned char * romimage;

//	logerror ("CPM %d  RAM %x  ROM %x\n", mtx_relcpmh,
//			mtx_rampage, mtx_rompage);
//	logerror ("map: [0000] %04x [2000] %04x [4000] %04x [6000] %04x\n",
//			bank1, bank2, bank3, bank4);
//	logerror ("     [8000] %04x [a000] %04x [c000] %04x [e000] %04x\n",
//			bank5, bank6, bank7, bank8);

	romimage = memory_region (REGION_CPU1);

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

static WRITE8_HANDLER ( mtx_bankswitch_w )
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

//	logerror ("%s (%d,%d,%d,%04x) -> (%d,%06x)\n", what,
//			mtx_relcpmh, mtx_rompage, mtx_rampage, vaddress,
//			*ramspace, *paddress);
}

static unsigned char mtx_peek (int vaddress)
{
	unsigned char * romimage;
	int ramspace;
	int paddress;

	romimage = memory_region (REGION_CPU1);
	mtx_virt_to_phys ("peek", vaddress, &ramspace, &paddress);

	if (paddress > mess_ram_size) {
		logerror ("peek into non-existing memory\n");
		return 0;
	}

	if (ramspace)
		return mess_ram[paddress];
	else 
		return romimage[paddress];
}

static void mtx_poke (int vaddress, unsigned char data)
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

//	logerror("mtx_save_hack: start=%#x  length=%#x (%d)  index=%#x (%d)\n",
//			start, length, length, mtx_save_index, mtx_save_index);

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

//	logerror("mtx_load_hack: start=%#x  length=%#x (%d)\n", start, length, length);

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
	// logerror("mtx_verify_hack:  start=0x%x  length=0x%x (%d)  not implemented\n", start, length, length);
}

static WRITE8_HANDLER ( mtx_trap_write )
{
	int pc;
	int start;
	int length;

	if (mtx_relcpmh)
	{
		mtx_poke (offset, data);
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

static void mtx_printer_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										printer_device_getinfo(devclass, state, info); break;
	}
}

static const TMS9928a_interface tms9928a_interface =
{
	TMS9929A,
	0x4000,
	0, 0,
	mtx_tms9929A_interrupt
};

static MACHINE_START( mtx512 )
{
	unsigned char * romimage;

	TMS9928A_configure(&tms9928a_interface);

	mtx_null_mem = (unsigned char *) auto_malloc (16384);
	mtx_zero_mem = (unsigned char *) auto_malloc (16384);
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

static INTERRUPT_GEN( mtx_interrupt )
{
	TMS9928A_interrupt();
}

static ADDRESS_MAP_START( mtx_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x1fff) AM_READWRITE( MRA8_BANK1, mtx_trap_write )
	AM_RANGE( 0x2000, 0x3fff) AM_READWRITE( MRA8_BANK2, MWA8_BANK10 )
	AM_RANGE( 0x4000, 0x5fff) AM_READWRITE( MRA8_BANK3, MWA8_BANK11 )
	AM_RANGE( 0x6000, 0x7fff) AM_READWRITE( MRA8_BANK4, MWA8_BANK12 )
	AM_RANGE( 0x8000, 0x9fff) AM_READWRITE( MRA8_BANK5, MWA8_BANK13 )
	AM_RANGE( 0xa000, 0xbfff) AM_READWRITE( MRA8_BANK6, MWA8_BANK14 )
	AM_RANGE( 0xc000, 0xdfff) AM_READWRITE( MRA8_BANK7, MWA8_BANK15 )
	AM_RANGE( 0xe000, 0xffff) AM_READWRITE( MRA8_BANK8, MWA8_BANK16 )
ADDRESS_MAP_END

static ADDRESS_MAP_START( mtx_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) ) 
	AM_RANGE( 0x00, 0x00) AM_READWRITE( mtx_strobe_r, mtx_bankswitch_w )
	AM_RANGE( 0x01, 0x02) AM_READWRITE( mtx_vdp_r, mtx_vdp_w )
	AM_RANGE( 0x03, 0x03) AM_READWRITE( mtx_psg_r, mtx_cst_w )
	AM_RANGE( 0x04, 0x04) AM_READWRITE( mtx_prt_r, mtx_prt_w )
	AM_RANGE( 0x05, 0x05) AM_READWRITE( mtx_key_lo_r, mtx_sense_w )
	AM_RANGE( 0x06, 0x06) AM_READWRITE( mtx_key_hi_r, mtx_psg_w )
	AM_RANGE( 0x08, 0x0b) AM_READWRITE( mtx_ctc_r, mtx_ctc_w )
ADDRESS_MAP_END

static INPUT_PORTS_START( mtx512 )
 PORT_START /* 0 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 !") PORT_CODE(KEYCODE_1)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 #") PORT_CODE(KEYCODE_3)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 %") PORT_CODE(KEYCODE_5)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 '") PORT_CODE(KEYCODE_7)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 )") PORT_CODE(KEYCODE_9)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("- =") PORT_CODE(KEYCODE_MINUS)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\ |") PORT_CODE(KEYCODE_BACKSLASH)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMERICPAD7 PAGE )") PORT_CODE(KEYCODE_7_PAD)

 PORT_START /* 1 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESCAPE") PORT_CODE(KEYCODE_ESC)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 \"") PORT_CODE(KEYCODE_2)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 $") PORT_CODE(KEYCODE_4)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 &") PORT_CODE(KEYCODE_6)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 (") PORT_CODE(KEYCODE_8)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 0") PORT_CODE(KEYCODE_0)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("^ ~") PORT_CODE(KEYCODE_EQUALS)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMERICPAD8 EOL") PORT_CODE(KEYCODE_8_PAD)

 PORT_START /* 2 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("w W") PORT_CODE(KEYCODE_W)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("r R") PORT_CODE(KEYCODE_R)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("y Y") PORT_CODE(KEYCODE_Y)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("i I") PORT_CODE(KEYCODE_I)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("p P") PORT_CODE(KEYCODE_P)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[ {") PORT_CODE(KEYCODE_CLOSEBRACE)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMERICPAD5 CURSORUP") PORT_CODE(KEYCODE_5_PAD)

 PORT_START /* 3 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("q Q") PORT_CODE(KEYCODE_Q)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("e E") PORT_CODE(KEYCODE_E)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("t T") PORT_CODE(KEYCODE_T)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("u U") PORT_CODE(KEYCODE_U)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("o O") PORT_CODE(KEYCODE_O)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@ `") PORT_CODE(KEYCODE_OPENBRACE)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LINEFEED") PORT_CODE(KEYCODE_DOWN)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMERICPAD1 CURSORLEFT") PORT_CODE(KEYCODE_1_PAD)

 PORT_START /* 4 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ALPHALOCK") PORT_CODE(KEYCODE_CAPSLOCK)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("s S") PORT_CODE(KEYCODE_S)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("f F") PORT_CODE(KEYCODE_F)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("h H") PORT_CODE(KEYCODE_H)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("k K") PORT_CODE(KEYCODE_K)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("; +") PORT_CODE(KEYCODE_COLON)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("] }") PORT_CODE(KEYCODE_BACKSLASH2)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMERICPAD3 CURSORRIGHT") PORT_CODE(KEYCODE_3_PAD)

 PORT_START /* 5 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("a A") PORT_CODE(KEYCODE_A)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("d D") PORT_CODE(KEYCODE_D)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("g G") PORT_CODE(KEYCODE_G)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("j J") PORT_CODE(KEYCODE_J)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("l L") PORT_CODE(KEYCODE_L)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(": *") PORT_CODE(KEYCODE_QUOTE)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMERICPAD2 HOME") PORT_CODE(KEYCODE_2_PAD)

 PORT_START /* 6 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LEFTSHIFT") PORT_CODE(KEYCODE_LSHIFT)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("x X") PORT_CODE(KEYCODE_X)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("v V") PORT_CODE(KEYCODE_V)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("n N") PORT_CODE(KEYCODE_N)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RIGHTSHIFT") PORT_CODE(KEYCODE_RSHIFT)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMERICPAD(.) CURSORDOWN") PORT_CODE(KEYCODE_DEL_PAD)

 PORT_START /* 7 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("z Z") PORT_CODE(KEYCODE_Z)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("c C") PORT_CODE(KEYCODE_C)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("b B") PORT_CODE(KEYCODE_B)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("m M") PORT_CODE(KEYCODE_M)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(.) >") PORT_CODE(KEYCODE_STOP)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("_ _") PORT_CODE(KEYCODE_MINUS_PAD)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMERICPAD0 INS") PORT_CODE(KEYCODE_0_PAD)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMERICPADENTER CLS") PORT_CODE(KEYCODE_ENTER_PAD)

/* Hi Bits */

 PORT_START /* 8 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMERICPAD9 BRK") PORT_CODE(KEYCODE_9_PAD)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BS") PORT_CODE(KEYCODE_BACKSPACE)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMERICPAD4 TAB") PORT_CODE(KEYCODE_4_PAD)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMERICPAD6 DEL") PORT_CODE(KEYCODE_6_PAD)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F6") PORT_CODE(KEYCODE_F6)

 PORT_START /* 9 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(none)")
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F7") PORT_CODE(KEYCODE_F7)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(none)")
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(none)")
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F8") PORT_CODE(KEYCODE_F8)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)

 PORT_START /* 10 */
  PORT_DIPNAME(0x02, 0x00, "Country Code Switch 1")
  PORT_DIPSETTING(0x02, DEF_STR(Off) )
  PORT_DIPSETTING(0x00, DEF_STR(On) )
  PORT_DIPNAME(0x01, 0x00, "Country Code Switch 0")
  PORT_DIPSETTING(0x01, DEF_STR(Off) )
  PORT_DIPSETTING(0x00, DEF_STR(On) )
  PORT_BIT(0xfc, IP_ACTIVE_LOW, IPT_UNUSED)

INPUT_PORTS_END


static struct z80_irq_daisy_chain mtx_daisy_chain[] =
{
	{z80ctc_reset, z80ctc_irq_state, z80ctc_irq_ack, z80ctc_irq_reti, 0},
	{0,0,0,0,-1}
};

static MACHINE_DRIVER_START( mtx512 )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, MTX_SYSTEM_CLOCK)
	MDRV_CPU_PROGRAM_MAP(mtx_mem, 0)
	MDRV_CPU_IO_MAP(mtx_io, 0)
	MDRV_CPU_VBLANK_INT(mtx_interrupt, 1)
	MDRV_CPU_CONFIG(mtx_daisy_chain)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START( mtx512 )

	/* video hardware */
	MDRV_IMPORT_FROM(tms9928a)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76489A, MTX_SYSTEM_CLOCK)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

ROM_START (mtx512)
	ROM_REGION (0x12000, REGION_CPU1, 0)
	ROM_LOAD ("osrom", 0x0, 0x2000, CRC(9ca858cc) SHA1(3804503a58f0bcdea96bb6488833782ebd03976d))
	ROM_LOAD ("basicrom", 0x2000, 0x2000, CRC(87b4e59c) SHA1(c49782a82a7f068c1195cd967882ba9edd546eaf))
	ROM_LOAD ("assemrom", 0x4000, 0x2000, CRC(9d7538c3) SHA1(d1882c4ea61a68b1715bd634ded5603e18a99c5f))
ROM_END

SYSTEM_CONFIG_START(mtx512)
	CONFIG_RAM(32 * 1024)
	CONFIG_RAM_DEFAULT(64 * 1024)
	CONFIG_RAM(96 * 1024)
	CONFIG_RAM(128 * 1024)
	CONFIG_RAM(160 * 1024)
	CONFIG_RAM(192 * 1024)
	CONFIG_RAM(224 * 1024)
	CONFIG_RAM(256 * 1024)
	CONFIG_RAM(288 * 1024)
	CONFIG_RAM(320 * 1024)
	CONFIG_RAM(352 * 1024)
	CONFIG_RAM(384 * 1024)
	CONFIG_RAM(416 * 1024)
	CONFIG_RAM(448 * 1024)
	CONFIG_RAM(480 * 1024)
	CONFIG_RAM(512 * 1024)
	CONFIG_DEVICE(mtx_printer_getinfo)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT	COMPAT	MACHINE   INPUT     INIT     CONFIG,  COMPANY          FULLNAME */
COMP( 1983, mtx512,   0,		0,		mtx512,   mtx512,   0,       mtx512,  "Memotech Ltd.", "MTX 512" , 0)
