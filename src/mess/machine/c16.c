/***************************************************************************

	commodore c16 home computer

	peter.trauner@jk.uni-linz.ac.at
    documentation
 	 www.funet.fi

***************************************************************************/

#include "driver.h"
#include "image.h"
#include "cpu/m6502/m6502.h"
#include "sound/sid6581.h"

#include "includes/cbmserb.h"
#include "includes/vc1541.h"
#include "machine/tpi6525.h"
#include "video/ted7360.h"

#include "includes/c16.h"

#include "devices/cassette.h"
#include "devices/cartslot.h"


#define VERBOSE_LEVEL 0
#define DBG_LOG(N,M,A) \
	{ \
		if(VERBOSE_LEVEL >= N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s", attotime_to_double(timer_get_time()), (char*) M ); \
			logerror A; \
		} \
	}

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

static UINT8 read_cfg0(running_machine *machine)
{
	UINT8 result;
	switch(mame_get_phase(machine))
	{
		case MAME_PHASE_RESET:
		case MAME_PHASE_RUNNING:
			result = input_port_read(machine, "CFG0");
			break;

		default:
			result = 0x00;
			break;
	}
	return result;
}

static UINT8 read_cfg1(running_machine *machine)
{
	UINT8 result;
	switch(mame_get_phase(machine))
	{
		case MAME_PHASE_RESET:
		case MAME_PHASE_RUNNING:
			result = input_port_read(machine, "CFG1");
			break;

		default:
			result = 0x00;
			break;
	}
	return result;
}

/*
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
	cbm_serial_atn_write (atn = !(data & 0x04));
	cbm_serial_clock_write (clk = !(data & 0x02));
	cbm_serial_data_write (dat = !(data & 0x01));

//	vc20_tape_write (!(data & 0x02));		// CASSETTE_RECORD not implemented yet
}

UINT8 c16_m7501_port_read(void)
{
	UINT8 data = 0xff;
	UINT8 c16_port7501 = (UINT8) cpunum_get_info_int(0, CPUINFO_INT_M6510_PORT);

	if ((c16_port7501 & 0x01) || !cbm_serial_data_read())
		data &= ~0x80;

	if ((c16_port7501 & 0x02) || !cbm_serial_clock_read())
		data &= ~0x40;

//	data &= ~0x20; // port bit not in pinout

	if (cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0)) > +0.0)
		data |=  0x10;
	else
		data &= ~0x10;

	cassette_change_state(image_from_devtype_and_index(IO_CASSETTE, 0), (c16_port7501 & 0x08) ? CASSETTE_MOTOR_DISABLED : CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);

	return data;
}

static void c16_bankswitch (running_machine *machine)
{
	UINT8 *rom = memory_region(machine, "main");
	memory_set_bankptr(9, mess_ram);

	switch (lowrom)
	{
	case 0:
		memory_set_bankptr (2, rom + 0x10000);
		break;
	case 1:
		memory_set_bankptr (2, rom + 0x18000);
		break;
	case 2:
		memory_set_bankptr (2, rom + 0x20000);
		break;
	case 3:
		memory_set_bankptr (2, rom + 0x28000);
		break;
	}

	switch (highrom)
	{
	case 0:
		memory_set_bankptr (3, rom + 0x14000);
		memory_set_bankptr (8, rom + 0x17f20);
		break;
	case 1:
		memory_set_bankptr (3, rom + 0x1c000);
		memory_set_bankptr (8, rom + 0x1ff20);
		break;
	case 2:
		memory_set_bankptr (3, rom + 0x24000);
		memory_set_bankptr (8, rom + 0x27f20);
		break;
	case 3:
		memory_set_bankptr (3, rom + 0x2c000);
		memory_set_bankptr (8, rom + 0x2ff20);
		break;
	}
	memory_set_bankptr (4, rom + 0x17c00);
}

WRITE8_HANDLER(c16_switch_to_rom)
{
	ted7360_rom = 1;
	c16_bankswitch(machine);
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
	lowrom = offset & 0x03;
	highrom = (offset & 0x0c) >> 2;
	if (ted7360_rom)
		c16_bankswitch(machine);
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

	if (!(port6529 & 0x01))
		value &= keyline[0];

	if (!(port6529 & 0x02))
		value &= keyline[1];

	if (!(port6529 & 0x04))
		value &= keyline[2];

	if (!(port6529 & 0x08))
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
	if (!(databus & 0x04))
		value &= keyline[8];

	if (!(databus & 0x02))
		value &= keyline[9];

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
	int data = 0x00;

	if (!((cassette_get_state(image_from_devtype_and_index(IO_CASSETTE, 0)) & CASSETTE_MASK_UISTATE) == CASSETTE_PLAY))
		data |= 0x04;
	return data;
}

 READ8_HANDLER(c16_fd1x_r)
{
	int data = 0x00;

	if (!((cassette_get_state(image_from_devtype_and_index(IO_CASSETTE, 0)) & CASSETTE_MASK_UISTATE) == CASSETTE_PLAY))
		data |= 0x04;
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
	offset &= 0x03;
	DBG_LOG (3, "6551", ("port write %.2x %.2x\n", offset, data));
	port6529 = data;
}

 READ8_HANDLER(c16_6551_port_r)
{
	int data = 0x00;

	offset &= 0x03;
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

void c16_interrupt (running_machine *machine, int level)
{
	static int old_level;

	if (level != old_level)
	{
		DBG_LOG (3, "mos7501", ("irq %s\n", level ? "start" : "end"));
		cpunum_set_input_line(machine, 0, M6510_IRQ_LINE, level);
		old_level = level;
	}
}

static void c16_common_driver_init (running_machine *machine)
{
#ifdef VC1541
	VC1541_CONFIG vc1541= { 1, 8 };
#endif
	C1551_CONFIG config= { 1 };
	UINT8 *rom;

	/* configure the M7501 port */
	cpunum_set_info_fct(0, CPUINFO_PTR_M6510_PORTREAD, (genf *) c16_m7501_port_read);
	cpunum_set_info_fct(0, CPUINFO_PTR_M6510_PORTWRITE, (genf *) c16_m7501_port_write);

	c16_select_roms (machine, 0, 0);
	c16_switch_to_rom (machine, 0, 0);

	if ((read_cfg0(machine) & 0xc0 ) == 0x40)		/* C1551 */
	 {
		tpi6525[2].a.read   = c1551x_0_read_data;
		tpi6525[2].a.output = c1551x_0_write_data;
		tpi6525[2].b.read   = c1551x_0_read_status;
		tpi6525[2].c.read   = c1551x_0_read_handshake;
		tpi6525[2].c.output = c1551x_0_write_handshake;
	} 
	else 
	{
		tpi6525[2].a.read   = c1551_0_read_data;
		tpi6525[2].a.output = c1551_0_write_data;
		tpi6525[2].b.read   = c1551_0_read_status;
		tpi6525[2].c.read   = c1551_0_read_handshake;
		tpi6525[2].c.output = c1551_0_write_handshake;
	}

	tpi6525[3].a.read   = c1551_1_read_data;
	tpi6525[3].a.output = c1551_1_write_data;
	tpi6525[3].b.read   = c1551_1_read_status;
	tpi6525[3].c.read   = c1551_1_read_handshake;
	tpi6525[3].c.output = c1551_1_write_handshake;

	rom = memory_region(machine, "main");

	c16_memory_10000 = rom + 0x10000;
	c16_memory_14000 = rom + 0x14000;
	c16_memory_18000 = rom + 0x18000;
	c16_memory_1c000 = rom + 0x1c000;
	c16_memory_20000 = rom + 0x20000;
	c16_memory_24000 = rom + 0x24000;
	c16_memory_28000 = rom + 0x28000;
	c16_memory_2c000 = rom + 0x2c000;

	/* need to recognice non available tia6523's (iec8/9) */
	memset(mess_ram + (0xfdc0 % mess_ram_size), 0xff, 0x40);

	memset(mess_ram + (0xfd40 % mess_ram_size), 0xff, 0x20);

	if ((read_cfg0(machine) & 0xc0 ) == 0x40)		/* C1551 */
		c1551_config (0, 0, &config);

#ifdef VC1541
	if ((read_cfg0(machine) & 0xc0 ) == 0x80)		/* VC1541 */
		vc1541_config (0, 0, &vc1541);
#endif
}

void c16_driver_init(running_machine *machine)
{
	c16_common_driver_init (machine);
	ted7360_init ((read_cfg1(machine) & 0x10 ) == 0x00);		/* is it PAL? */
	ted7360_set_dma (ted7360_dma_read, ted7360_dma_read_rom);
}

MACHINE_RESET( c16 )
{
	tpi6525_2_reset();
	tpi6525_3_reset();
	c364_speech_init();

	sndti_reset(SOUND_SID8580, 0);

	if (read_cfg1(machine) & 0x80)  /* SID card present */
	{
		memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xfd40, 0xfd5f, 0, 0, sid6581_0_port_r);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM,  0xfd40, 0xfd5f, 0, 0, sid6581_0_port_w);
		memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xfe80, 0xfe9f, 0, 0, sid6581_0_port_r);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM,  0xfe80, 0xfe9f, 0, 0, sid6581_0_port_w);
	}
	else
	{
		memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xfd40, 0xfd5f, 0, 0, SMH_NOP);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM,  0xfd40, 0xfd5f, 0, 0, SMH_NOP);
		memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xfe80, 0xfe9f, 0, 0, SMH_NOP);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM,  0xfe80, 0xfe9f, 0, 0, SMH_NOP);
	}

#if 0
	c16_switch_to_rom (0, 0);
	c16_select_roms (0, 0);
#endif
	if ((read_cfg1(machine) & 0x0c ) == 0x00)		/* is it C16? */
	{
		memory_set_bankptr(1, mess_ram + (0x4000 % mess_ram_size));

		memory_set_bankptr(5, mess_ram + (0x4000 % mess_ram_size));
		memory_set_bankptr(6, mess_ram + (0x8000 % mess_ram_size));
		memory_set_bankptr(7, mess_ram + (0xc000 % mess_ram_size));

		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xff20, 0xff3d, 0, 0, SMH_BANK10);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xff40, 0xffff, 0, 0, SMH_BANK11);
		memory_set_bankptr(10, mess_ram + (0xff20 % mess_ram_size));
		memory_set_bankptr(11, mess_ram + (0xff40 % mess_ram_size));

		ted7360_set_dma (ted7360_dma_read, ted7360_dma_read_rom);
	}
	else
	{
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x4000, 0xfcff, 0, 0, SMH_BANK10);
		memory_set_bankptr(10, mess_ram + (0x4000 % mess_ram_size));

		ted7360_set_dma (ted7360_dma_read, ted7360_dma_read_rom);
	}

	if ((read_cfg0(machine) & 0x38 ) == 0x08 || (read_cfg0(machine) & 0xc0 ) == 0x40)  /* IEC8 on || C1551 */
	{
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM,  0xfee0, 0xfeff, 0, 0, tpi6525_2_port_w);
		memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xfee0, 0xfeff, 0, 0, tpi6525_2_port_r);
	}
	else
	{
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM,  0xfee0, 0xfeff, 0, 0, SMH_NOP);
		memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xfee0, 0xfeff, 0, 0, SMH_NOP);
	}
	if ((read_cfg0(machine) & 0x07 ) == 0x01)		/* IEC9 on */
	{
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM,  0xfec0, 0xfedf, 0, 0, tpi6525_3_port_w);
		memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xfec0, 0xfedf, 0, 0, tpi6525_3_port_r);
	}
	else
	{
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM,  0xfec0, 0xfedf, 0, 0, SMH_NOP);
		memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xfec0, 0xfedf, 0, 0, SMH_NOP);
	}

	cbm_drive_0_config (SERIAL, 8);
	cbm_drive_1_config (SERIAL, 9);

	if ((read_cfg0(machine) & 0xc0 ) == 0x40)		/* c1551 */
		c1551_reset ();

#ifdef VC1541
	if ((read_cfg0(machine) & 0xc0 ) == 0x80)		/* VC1541 */
		vc1541_reset ();
#endif

	cbm_serial_reset_write (0);
}


INTERRUPT_GEN( c16_frame_interrupt )
{
	int value, i;
	static const char *c16ports[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4", "ROW5", "ROW6", "ROW7" };

	/* Lines 0-7 : common keyboard */
	for (i = 0; i < 8; i++)
	{
		value = 0xff;
		value &= ~input_port_read(machine, c16ports[i]);

		/* Shift Lock is mapped on Left/Right Shift */
		if ((i == 1) && (input_port_read(machine, "SPECIAL") & 0x80))
			value &= ~0x80;			

		keyline[i] = value;
	}

	if (input_port_read(machine, "DSW0") & 0x80) 
	{
		value = 0xff;
		if (input_port_read(machine, "JOY0") & 0x10)			/* Joypad1_Button */
			{
				if (input_port_read(machine, "SPECIAL") & 0x40)
					value &= ~0x80;
				else
					value &= ~0x40;
			}

		value &= ~(input_port_read(machine, "JOY0") & 0x0f);	/* Other Inputs Joypad1 */

		if (input_port_read(machine, "SPECIAL") & 0x40)
			keyline[9] = value;
		else
			keyline[8] = value;
	}

	if (input_port_read(machine, "DSW0") & 0x40) 
	{
		value = 0xff;
		if (input_port_read(machine, "JOY1") & 0x10)			/* Joypad2_Button */
			{
				if (input_port_read(machine, "SPECIAL") & 0x40)
					value &= ~0x40;
				else
					value &= ~0x80;
			}

		value &= ~(input_port_read(machine, "JOY1") & 0x0f);	/* Other Inputs Joypad2 */
		
		if (input_port_read(machine, "SPECIAL") & 0x40)
			keyline[8] = value;
		else
			keyline[9] = value;
	}

	ted7360_frame_interrupt (machine, cpunum);

	set_led_status (1, input_port_read(machine, "SPECIAL") & 0x80 ? 1 : 0);		/* Shift Lock */
	set_led_status (0, input_port_read(machine, "SPECIAL") & 0x40 ? 1 : 0);		/* Joystick Swap */
}


/***********************************************

	C16 Cartridges

***********************************************/

DEVICE_IMAGE_LOAD(c16_cart)
{
	UINT8 *mem = memory_region(image->machine, "main");
	int size = image_length (image), test;
	const char *filetype;
	int address = 0;

    /* magic lowrom at offset 7: $43 $42 $4d */
	/* if at offset 6 stands 1 it will immediatly jumped to offset 0 (0x8000) */
	static const unsigned char magic[] = {0x43, 0x42, 0x4d}; 
	unsigned char buffer[sizeof (magic)];

	image_fseek (image, 7, SEEK_SET);
	image_fread (image, buffer, sizeof (magic));
	image_fseek (image, 0, SEEK_SET);

	/* Check if our cart has the magic string, and set its loading address */
	if (!memcmp (buffer, magic, sizeof (magic)))
		address = 0x20000;
	
	/* Give a loading address to non .bin / non .rom carts as well */
	filetype = image_filetype(image);

	/* We would support .hi and .lo files, but currently I'm not sure where to load them. 
	   We simply load them at 0x20000 at this stage, even if it's probably wrong!
	   It could also well be that they both need to be loaded at the same time, but this 
	   is now impossible since I reduced to 1 the number of cart slots. 
	   More investigations are in order if any .hi, .lo dump would surface!				 */
	if (!mame_stricmp (filetype, "hi"))
		address = 0x20000;	/* FIX ME! */

	else if (!mame_stricmp (filetype, "lo"))
		address = 0x20000;	/* FIX ME! */

	/* As a last try, give a reasonable loading address also to .bin/.rom without the magic string */		
	else if (!address)
	{
		logerror("Cart %s does not contain the magic string: it may be loaded at the wrong memory address!\n", image_filename(image));
		address = 0x20000;
	}

	logerror("Loading cart %s at %.5x size:%.4x\n", image_filename(image), address, size);

	/* Finally load the cart */
	test = image_fread (image, mem + address, size);

	if (test != size)
		return INIT_FAIL;

	return INIT_PASS;
}

void c16_cartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:					info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:						info->load = DEVICE_IMAGE_LOAD_NAME(c16_cart); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:			strcpy(info->s = device_temp_str(), "bin,rom,hi,lo"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}
