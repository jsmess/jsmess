/***************************************************************************

	commodore c16 home computer

	peter.trauner@jk.uni-linz.ac.at
    documentation
 	 www.funet.fi

***************************************************************************/

#include <ctype.h>
#include "driver.h"
#include "image.h"
#include "cpu/m6502/m6502.h"
#include "sound/sid6581.h"

#define VERBOSE_DBG 1
#include "includes/cbm.h"
#include "includes/tpi6525.h"
#include "includes/cbmserb.h"
#include "includes/vc1541.h"
#include "includes/vc20tape.h"
#include "includes/ted7360.h"

#include "includes/c16.h"

static UINT8 keyline[10] =
{
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

/*
 * tia6523
 *
 * connector to floppy c1551 (delivered with c1551 as c16 expansion)
 * port a for data read/write
 * port b
 * 0 status 0
 * 1 status 1
 * port c
 * 6 dav output edge data on port a available
 * 7 ack input edge ready for next datum
 */

static UINT8 port6529;

static int lowrom = 0, highrom = 0;

static UINT8 *c16_memory_10000;
static UINT8 *c16_memory_14000;
static UINT8 *c16_memory_18000;
static UINT8 *c16_memory_1c000;
static UINT8 *c16_memory_20000;
static UINT8 *c16_memory_24000;
static UINT8 *c16_memory_28000;
static UINT8 *c16_memory_2c000;

static int c16_rom_load(mess_image *img);

/**
  ddr bit 1 port line is output
  port bit 1 port line is high

  serial bus
  1 serial srq in (ignored)
  2 gnd
  3 atn out (pull up)
  4 clock in/out (pull up)
  5 data in/out (pull up)
  6 /reset (pull up) hardware


  p0 negated serial bus pin 5 /data out
  p1 negated serial bus pin 4 /clock out, cassette write
  p2 negated serial bus pin 3 /atn out
  p3 cassette motor out

  p4 cassette read
  p5 not connected (or not available on MOS7501?)
  p6 serial clock in
  p7 serial data in, serial bus 5
 */
void c16_m7501_port_write(UINT8 data)
{
	int dat, atn, clk;

	/* bit zero then output 0 */
	cbm_serial_atn_write (atn = !(data & 4));
	cbm_serial_clock_write (clk = !(data & 2));
	cbm_serial_data_write (dat = !(data & 1));
	vc20_tape_write (!(data & 2));
	vc20_tape_motor (data & 8);
}

UINT8 c16_m7501_port_read(void)
{
	UINT8 data = 0xFF;
	UINT8 c16_port7501 = (UINT8) cpunum_get_info_int(0, CPUINFO_INT_M6510_PORT);

	if ((c16_port7501 & 0x01) || !cbm_serial_data_read())
		data &= ~0x80;

	if ((c16_port7501 & 0x02) || !cbm_serial_clock_read())
		data &= ~0x40;

	if (!vc20_tape_read())
		data &= ~0x10;

/*	data &= ~0x20; //port bit not in pinout */
	return data;
}

static void c16_bankswitch (void)
{
	memory_set_bankptr(9, mess_ram);

	switch (lowrom)
	{
	case 0:
		memory_set_bankptr (2, memory_region(REGION_CPU1) + 0x10000);
		break;
	case 1:
		memory_set_bankptr (2, memory_region(REGION_CPU1) + 0x18000);
		break;
	case 2:
		memory_set_bankptr (2, memory_region(REGION_CPU1) + 0x20000);
		break;
	case 3:
		memory_set_bankptr (2, memory_region(REGION_CPU1) + 0x28000);
		break;
	}
	switch (highrom)
	{
	case 0:
		memory_set_bankptr (3, memory_region(REGION_CPU1) + 0x14000);
		memory_set_bankptr (8, memory_region(REGION_CPU1) + 0x17f20);
		break;
	case 1:
		memory_set_bankptr (3, memory_region(REGION_CPU1) + 0x1c000);
		memory_set_bankptr (8, memory_region(REGION_CPU1) + 0x1ff20);
		break;
	case 2:
		memory_set_bankptr (3, memory_region(REGION_CPU1) + 0x24000);
		memory_set_bankptr (8, memory_region(REGION_CPU1) + 0x27f20);
		break;
	case 3:
		memory_set_bankptr (3, memory_region(REGION_CPU1) + 0x2c000);
		memory_set_bankptr (8, memory_region(REGION_CPU1) + 0x2ff20);
		break;
	}
	memory_set_bankptr (4, memory_region(REGION_CPU1) + 0x17c00);
}

WRITE8_HANDLER(c16_switch_to_rom)
{
	ted7360_rom = 1;
	c16_bankswitch ();
}

/* write access to fddX load data flipflop
 * and selects roms
 * a0 a1
 * 0  0  basic
 * 0  1  plus4 low
 * 1  0  c1 low
 * 1  1  c2 low
 *
 * a2 a3
 * 0  0  kernal
 * 0  1  plus4 hi
 * 1  0  c1 high
 * 1  1  c2 high */
WRITE8_HANDLER(c16_select_roms)
{
	lowrom = offset & 3;
	highrom = (offset & 0xc) >> 2;
	if (ted7360_rom)
		c16_bankswitch ();
}

WRITE8_HANDLER(c16_switch_to_ram)
{
	ted7360_rom = 0;
	memory_set_bankptr (2, mess_ram + (0x8000 % mess_ram_size));
	memory_set_bankptr (3, mess_ram + (0xc000 % mess_ram_size));
	memory_set_bankptr (4, mess_ram + (0xfc00 % mess_ram_size));
	memory_set_bankptr (8, mess_ram + (0xff20 % mess_ram_size));
}

int c16_read_keyboard (int databus)
{
	int value = 0xff;

	if (!(port6529 & 1))
		value &= keyline[0];
	if (!(port6529 & 2))
		value &= keyline[1];
	if (!(port6529 & 4))
		value &= keyline[2];
	if (!(port6529 & 8))
		value &= keyline[3];
	if (!(port6529 & 0x10))
		value &= keyline[4];
	if (!(port6529 & 0x20))
		value &= keyline[5];
	if (!(port6529 & 0x40))
		value &= keyline[6];
	if (!(port6529 & 0x80))
		value &= keyline[7];

	/* looks like joy 0 needs dataline2 low
	 * and joy 1 needs dataline1 low
	 * write to 0xff08 (value on databus) reloads latches */
	if (!(databus & 4)) {
		value &= keyline[8];
	}
	if (!(databus & 2)) {
		value &= keyline[9];
	}
	return value;
}

/*
 * mos 6529
 * simple 1 port 8bit input output
 * output with pull up resistors, 0 means low
 * input, 0 means low
 */
/*
 * ic used as output,
 * output low means keyboard line selected
 * keyboard line is then read into the ted7360 latch
 */
WRITE8_HANDLER(c16_6529_port_w)
{
	port6529 = data;
}

 READ8_HANDLER(c16_6529_port_r)
{
	return port6529 & (c16_read_keyboard (0xff /*databus */ ) | (port6529 ^ 0xff));
}

/*
 * p0 Userport b
 * p1 Userport k
 * p2 Userport 4, cassette sense
 * p3 Userport 5
 * p4 Userport 6
 * p5 Userport 7
 * p6 Userport j
 * p7 Userport f
 */
WRITE8_HANDLER(plus4_6529_port_w)
{
}

 READ8_HANDLER(plus4_6529_port_r)
{
	int data = 0;

	if (vc20_tape_switch ())
		data |= 4;
	return data;
}

 READ8_HANDLER(c16_fd1x_r)
{
	int data = 0;

	if (vc20_tape_switch ())
		data |= 4;
	return data;
}

/**
 0 write: transmit data
 0 read: receiver data
 1 write: programmed rest (data is dont care)
 1 read: status register
 2 command register
 3 control register
 control register (offset 3)
  cleared by hardware reset, not changed by programmed reset
  7: 2 stop bits (0 1 stop bit)
  6,5: data word length
   00 8 bits
   01 7
   10 6
   11 5
  4: ?? clock source
   0 external receiver clock
   1 baud rate generator
  3-0: baud rate generator
   0000 use external clock
   0001 60
   0010 75
   0011
   0100
   0101
   0110 300
   0111 600
   1000 1200
   1001
   1010 2400
   1011 3600
   1100 4800
   1101 7200
   1110 9600
   1111 19200
 control register
  */
WRITE8_HANDLER(c16_6551_port_w)
{
	offset &= 3;
	DBG_LOG (3, "6551", ("port write %.2x %.2x\n", offset, data));
	port6529 = data;
}

 READ8_HANDLER(c16_6551_port_r)
{
	int data = 0;

	offset &= 3;
	DBG_LOG (3, "6551", ("port read %.2x %.2x\n", offset, data));
	return data;
}

static READ8_HANDLER(ted7360_dma_read)
{
	return mess_ram[offset % mess_ram_size];
}

static  READ8_HANDLER(ted7360_dma_read_rom)
{
	/* should read real c16 system bus from 0xfd00 -ff1f */
	if (offset >= 0xc000)
	{								   /* rom address in rom */
		if ((offset >= 0xfc00) && (offset < 0xfd00))
			return c16_memory_10000[offset];
		switch (highrom)
		{
		case 0:
			return c16_memory_10000[offset & 0x7fff];
		case 1:
			return c16_memory_18000[offset & 0x7fff];
		case 2:
			return c16_memory_20000[offset & 0x7fff];
		case 3:
			return c16_memory_28000[offset & 0x7fff];
		}
	}
	if (offset >= 0x8000)
	{								   /* rom address in rom */
		switch (lowrom)
		{
		case 0:
			return c16_memory_10000[offset & 0x7fff];
		case 1:
			return c16_memory_18000[offset & 0x7fff];
		case 2:
			return c16_memory_20000[offset & 0x7fff];
		case 3:
			return c16_memory_28000[offset & 0x7fff];
		}
	}

	return mess_ram[offset % mess_ram_size];
}

void c16_interrupt (int level)
{
	static int old_level;

	if (level != old_level)
	{
		DBG_LOG (3, "mos7501", ("irq %s\n", level ? "start" : "end"));
		cpunum_set_input_line (0, M6510_IRQ_LINE, level);
		old_level = level;
	}
}

static void c16_common_driver_init (void)
{
#ifdef VC1541
	VC1541_CONFIG vc1541= { 1, 8 };
#endif
	C1551_CONFIG config= { 1 };

	/* configure the M7501 port */
	cpunum_set_info_fct(0, CPUINFO_PTR_M6510_PORTREAD, (genf *) c16_m7501_port_read);
	cpunum_set_info_fct(0, CPUINFO_PTR_M6510_PORTWRITE, (genf *) c16_m7501_port_write);

	c16_select_roms (0, 0);
	c16_switch_to_rom (0, 0);

	if (REAL_C1551) {
		tpi6525[2].a.read=c1551x_0_read_data;
		tpi6525[2].a.output=c1551x_0_write_data;
		tpi6525[2].b.read=c1551x_0_read_status;
		tpi6525[2].c.read=c1551x_0_read_handshake;
		tpi6525[2].c.output=c1551x_0_write_handshake;
	} else {
		tpi6525[2].a.read=c1551_0_read_data;
		tpi6525[2].a.output=c1551_0_write_data;
		tpi6525[2].b.read=c1551_0_read_status;
		tpi6525[2].c.read=c1551_0_read_handshake;
		tpi6525[2].c.output=c1551_0_write_handshake;
	}

	tpi6525[3].a.read=c1551_1_read_data;
	tpi6525[3].a.output=c1551_1_write_data;
	tpi6525[3].b.read=c1551_1_read_status;
	tpi6525[3].c.read=c1551_1_read_handshake;
	tpi6525[3].c.output=c1551_1_write_handshake;

	c16_memory_10000 = memory_region(REGION_CPU1) + 0x10000;
	c16_memory_14000 = memory_region(REGION_CPU1) + 0x14000;
	c16_memory_18000 = memory_region(REGION_CPU1) + 0x18000;
	c16_memory_1c000 = memory_region(REGION_CPU1) + 0x1c000;
	c16_memory_20000 = memory_region(REGION_CPU1) + 0x20000;
	c16_memory_24000 = memory_region(REGION_CPU1) + 0x24000;
	c16_memory_28000 = memory_region(REGION_CPU1) + 0x28000;
	c16_memory_2c000 = memory_region(REGION_CPU1) + 0x2c000;

	/* need to recognice non available tia6523's (iec8/9) */
	memset(mess_ram + (0xfdc0 % mess_ram_size), 0xff, 0x40);


	memset(mess_ram + (0xfd40 % mess_ram_size), 0xff, 0x20);

	c16_tape_open ();

	if (REAL_C1551)
		c1551_config (0, 0, &config);

#ifdef VC1541
	if (REAL_VC1541)
		vc1541_config (0, 0, &vc1541);
#endif
}

void c16_driver_init(void)
{
	c16_common_driver_init ();
	ted7360_init (C16_PAL);
	ted7360_set_dma (ted7360_dma_read, ted7360_dma_read_rom);
}

static WRITE8_HANDLER(c16_sidcart_16k)
{
	mess_ram[(0x1400 + offset) % mess_ram_size] = data;
	mess_ram[(0x5400 + offset) % mess_ram_size] = data;
	mess_ram[(0x9400 + offset) % mess_ram_size] = data;
	mess_ram[(0xd400 + offset) % mess_ram_size] = data;
	sid6581_0_port_w(offset,data);
}

static WRITE8_HANDLER(c16_sidcart_64k)
{
	mess_ram[(0xd400 + offset) % mess_ram_size] = data;
	sid6581_0_port_w(offset,data);
}

MACHINE_RESET( c16 )
{
	int i;

	tpi6525_2_reset();
	tpi6525_3_reset();
	c364_speech_init();

	sndti_reset(SOUND_SID8580, 0);

	if (SIDCARD)
	{
		memory_install_read8_handler (0, ADDRESS_SPACE_PROGRAM, 0xfd40, 0xfd5f, 0, 0, sid6581_0_port_r);
		memory_install_write8_handler (0, ADDRESS_SPACE_PROGRAM,  0xfd40, 0xfd5f, 0, 0, sid6581_0_port_w);
		memory_install_read8_handler (0, ADDRESS_SPACE_PROGRAM, 0xfe80, 0xfe9f, 0, 0, sid6581_0_port_r);
		memory_install_write8_handler (0, ADDRESS_SPACE_PROGRAM,  0xfe80, 0xfe9f, 0, 0, sid6581_0_port_w);
	}
	else
	{
		memory_install_read8_handler (0, ADDRESS_SPACE_PROGRAM, 0xfd40, 0xfd5f, 0, 0, MRA8_NOP);
		memory_install_write8_handler (0, ADDRESS_SPACE_PROGRAM,  0xfd40, 0xfd5f, 0, 0, MWA8_NOP);
		memory_install_read8_handler (0, ADDRESS_SPACE_PROGRAM, 0xfe80, 0xfe9f, 0, 0, MRA8_NOP);
		memory_install_write8_handler (0, ADDRESS_SPACE_PROGRAM,  0xfe80, 0xfe9f, 0, 0, MWA8_NOP);
	}

#if 0
	c16_switch_to_rom (0, 0);
	c16_select_roms (0, 0);
#endif
	if (TYPE_C16)
	{
		memory_set_bankptr(1, mess_ram + (0x4000 % mess_ram_size));

		memory_set_bankptr(5, mess_ram + (0x4000 % mess_ram_size));
		memory_set_bankptr(6, mess_ram + (0x8000 % mess_ram_size));
		memory_set_bankptr(7, mess_ram + (0xc000 % mess_ram_size));

		memory_install_write8_handler (0, ADDRESS_SPACE_PROGRAM, 0xff20, 0xff3d, 0, 0, MWA8_BANK10);
		memory_install_write8_handler (0, ADDRESS_SPACE_PROGRAM, 0xff40, 0xffff, 0, 0, MWA8_BANK11);
		memory_set_bankptr(10, mess_ram + (0xff20 % mess_ram_size)); 
		memory_set_bankptr(11, mess_ram + (0xff40 % mess_ram_size)); 

		if (SIDCARD_HACK)
			memory_install_write8_handler (0, ADDRESS_SPACE_PROGRAM,  0xd400, 0xd41f, 0, 0, c16_sidcart_16k);

		ted7360_set_dma (ted7360_dma_read, ted7360_dma_read_rom);
	}
	else
	{
		memory_install_write8_handler (0, ADDRESS_SPACE_PROGRAM, 0x4000, 0xfcff, 0, 0, MWA8_BANK10);
		memory_set_bankptr(10, mess_ram + (0x4000 % mess_ram_size));

		if (SIDCARD_HACK)
			memory_install_write8_handler (0, ADDRESS_SPACE_PROGRAM,  0xd400, 0xd41f, 0, 0, c16_sidcart_64k);

		ted7360_set_dma (ted7360_dma_read, ted7360_dma_read_rom);
	}

	if (IEC8ON || REAL_C1551)
	{
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM,  0xfee0, 0xfeff, 0, 0, tpi6525_2_port_w);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xfee0, 0xfeff, 0, 0, tpi6525_2_port_r);
	}
	else
	{
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM,  0xfee0, 0xfeff, 0, 0, MWA8_NOP);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xfee0, 0xfeff, 0, 0, MRA8_NOP);
	}
	if (IEC9ON)
	{
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM,  0xfec0, 0xfedf, 0, 0, tpi6525_3_port_w);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xfec0, 0xfedf, 0, 0, tpi6525_3_port_r);
	}
	else
	{
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM,  0xfec0, 0xfedf, 0, 0, MWA8_NOP);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xfec0, 0xfedf, 0, 0, MRA8_NOP);
	}

	cbm_drive_0_config (SERIAL, 8);
	cbm_drive_1_config (SERIAL, 9);

	if (REAL_C1551)
		c1551_reset ();

#ifdef VC1541
	if (REAL_VC1541)
		vc1541_reset ();
#endif

	cbm_serial_reset_write (0);

	for (i = 0; i < 2; i++)
	{
		mess_image *image = image_from_devtype_and_index(IO_CARTSLOT, i);
		if (image_exists(image))
			c16_rom_load(image);
	}
}

static int c16_rom_id(mess_image *image)
{
    /* magic lowrom at offset 7: $43 $42 $4d */
	/* if at offset 6 stands 1 it will immediatly jumped to offset 0 (0x8000) */
	int retval = 0;
	char magic[] = {0x43, 0x42, 0x4d}, buffer[sizeof (magic)];
	const char *name = image_filename(image);
	char *cp;

	logerror("c16_rom_id %s\n", name);
	retval = 0;

	image_fseek (image, 7, SEEK_SET);
	image_fread (image, buffer, sizeof (magic));

	if (memcmp (magic, buffer, sizeof (magic)) == 0)
	{
		retval = 1;
	}
	else if ((cp = strrchr (name, '.')) != NULL)
	{
		if ((mame_stricmp (cp + 1, "rom") == 0) || (mame_stricmp (cp + 1, "prg") == 0)
			|| (mame_stricmp (cp + 1, "bin") == 0)
			|| (mame_stricmp (cp + 1, "lo") == 0) || (mame_stricmp (cp + 1, "hi") == 0))
			retval = 1;
	}

		if (retval)
			logerror("rom %s recognized\n", name);
		else
			logerror("rom %s not recognized\n", name);
	return retval;
}

DEVICE_LOAD(c16_rom)
{
	return (!c16_rom_id(image)) ? INIT_FAIL : INIT_PASS;
}

static int c16_rom_load(mess_image *image)
{
	UINT8 *mem = memory_region (REGION_CPU1);
	int size, read_;
	const char *filetype;
	static unsigned int addr = 0;

	if (!c16_rom_id(image))
		return 1;

	size = image_length (image);

	filetype = image_filetype(image);	
	if (filetype && !mame_stricmp (filetype, "prg"))
	{
		unsigned short in;

		image_fread(image, &in, 2);
		in = LITTLE_ENDIANIZE_INT16(in);
		logerror("rom prg %.4x\n", in);
		addr = in + 0x20000;
		size -= 2;
	}
	if (addr == 0)
	{
		addr = 0x20000;
	}
	logerror("loading rom at %.5x size:%.4x\n", addr, size);
	read_ = image_fread (image, mem + addr, size);
	addr += size;
	if (read_ != size)
		return 1;
	return 0;
}

INTERRUPT_GEN( c16_frame_interrupt )
{
	int value;

	value = 0xff;
	if (KEY_ATSIGN)
		value &= ~0x80;
	if (KEY_F3)
		value &= ~0x40;
	if (KEY_F2)
		value &= ~0x20;
	if (KEY_F1)
		value &= ~0x10;
	if (KEY_HELP)
		value &= ~8;
	if (KEY_POUND)
		value &= ~4;
	if (KEY_RETURN)
		value &= ~2;
	if (KEY_DEL)
		value &= ~1;
	keyline[0] = value;

	value = 0xff;
	if (KEY_SHIFT)
		value &= ~0x80;
	if (KEY_E)
		value &= ~0x40;
	if (KEY_S)
		value &= ~0x20;
	if (KEY_Z)
		value &= ~0x10;
	if (KEY_4)
		value &= ~8;
	if (KEY_A)
		value &= ~4;
	if (KEY_W)
		value &= ~2;
	if (KEY_3)
		value &= ~1;
	keyline[1] = value;

	value = 0xff;
	if (KEY_X)
		value &= ~0x80;
	if (KEY_T)
		value &= ~0x40;
	if (KEY_F)
		value &= ~0x20;
	if (KEY_C)
		value &= ~0x10;
	if (KEY_6)
		value &= ~8;
	if (KEY_D)
		value &= ~4;
	if (KEY_R)
		value &= ~2;
	if (KEY_5)
		value &= ~1;
	keyline[2] = value;

	value = 0xff;
	if (KEY_V)
		value &= ~0x80;
	if (KEY_U)
		value &= ~0x40;
	if (KEY_H)
		value &= ~0x20;
	if (KEY_B)
		value &= ~0x10;
	if (KEY_8)
		value &= ~8;
	if (KEY_G)
		value &= ~4;
	if (KEY_Y)
		value &= ~2;
	if (KEY_7)
		value &= ~1;
	keyline[3] = value;

	value = 0xff;
	if (KEY_N)
		value &= ~0x80;
	if (KEY_O)
		value &= ~0x40;
	if (KEY_K)
		value &= ~0x20;
	if (KEY_M)
		value &= ~0x10;
	if (KEY_0)
		value &= ~8;
	if (KEY_J)
		value &= ~4;
	if (KEY_I)
		value &= ~2;
	if (KEY_9)
		value &= ~1;
	keyline[4] = value;

	value = 0xff;
	if (KEY_COMMA)
		value &= ~0x80;
	if (KEY_MINUS)
		value &= ~0x40;
	if (KEY_SEMICOLON)
		value &= ~0x20;
	if (KEY_POINT)
		value &= ~0x10;
	if (KEY_UP)
		value &= ~8;
	if (KEY_L)
		value &= ~4;
	if (KEY_P)
		value &= ~2;
	if (KEY_DOWN)
		value &= ~1;
	keyline[5] = value;

	value = 0xff;
	if (KEY_SLASH)
		value &= ~0x80;
	if (KEY_PLUS)
		value &= ~0x40;
	if (KEY_EQUALS)
		value &= ~0x20;
	if (KEY_ESC)
		value &= ~0x10;
	if (KEY_RIGHT)
		value &= ~8;
	if (KEY_COLON)
		value &= ~4;
	if (KEY_ASTERIX)
		value &= ~2;
	if (KEY_LEFT)
		value &= ~1;
	keyline[6] = value;

	value = 0xff;
	if (KEY_STOP)
		value &= ~0x80;
	if (KEY_Q)
		value &= ~0x40;
	if (KEY_CBM)
		value &= ~0x20;
	if (KEY_SPACE)
		value &= ~0x10;
	if (KEY_2)
		value &= ~8;
	if (KEY_CTRL)
		value &= ~4;
	if (KEY_HOME)
		value &= ~2;
	if (KEY_1)
		value &= ~1;
	keyline[7] = value;

	if (JOYSTICK1_PORT) {
		value = 0xff;
		if (JOYSTICK_1_BUTTON)
			{
				if (JOYSTICK_SWAP)
					value &= ~0x80;
				else
					value &= ~0x40;
			}
		if (JOYSTICK_1_RIGHT)
			value &= ~8;
		if (JOYSTICK_1_LEFT)
			value &= ~4;
		if (JOYSTICK_1_DOWN)
			value &= ~2;
		if (JOYSTICK_1_UP)
			value &= ~1;
		if (JOYSTICK_SWAP)
			keyline[9] = value;
		else
			keyline[8] = value;
	}

	if (JOYSTICK2_PORT) {
		value = 0xff;
		if (JOYSTICK_2_BUTTON)
			{
				if (JOYSTICK_SWAP)
					value &= ~0x40;
				else
					value &= ~0x80;
			}
		if (JOYSTICK_2_RIGHT)
			value &= ~8;
		if (JOYSTICK_2_LEFT)
			value &= ~4;
		if (JOYSTICK_2_DOWN)
			value &= ~2;
		if (JOYSTICK_2_UP)
			value &= ~1;
		if (JOYSTICK_SWAP)
			keyline[8] = value;
		else
			keyline[9] = value;
	}

	ted7360_frame_interrupt ();

	vc20_tape_config (DATASSETTE, DATASSETTE_TONE);
	vc20_tape_buttons (DATASSETTE_PLAY, DATASSETTE_RECORD, DATASSETTE_STOP);
	set_led_status (1 /*KB_CAPSLOCK_FLAG */ , KEY_SHIFTLOCK ? 1 : 0);
	set_led_status (0 /*KB_NUMLOCK_FLAG */ , JOYSTICK_SWAP ? 1 : 0);
}
