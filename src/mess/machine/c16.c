/***************************************************************************

    commodore c16 home computer

    peter.trauner@jk.uni-linz.ac.at
    documentation
     www.funet.fi

***************************************************************************/

#include "emu.h"
#include "image.h"
#include "audio/ted7360.h"
#include "cpu/m6502/m6502.h"
#include "devices/cassette.h"
#include "devices/cartslot.h"
#include "devices/messram.h"
#include "includes/c16.h"
#include "machine/cbmiec.h"
#include "sound/sid6581.h"

#define VERBOSE_LEVEL 0
#define DBG_LOG( MACHINE, N, M, A ) \
	do { \
		if(VERBOSE_LEVEL >= N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s", attotime_to_double(timer_get_time(MACHINE)), (char*) M ); \
			logerror A; \
		} \
	} while (0)


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

void c16_m7501_port_write( running_device *device, UINT8 direction, UINT8 data )
{
	c16_state *state = (c16_state *)device->machine->driver_data;

	/* bit zero then output 0 */
	cbm_iec_atn_w(state->serbus, device, !BIT(data, 2));
	cbm_iec_clk_w(state->serbus, device, !BIT(data, 1));
	cbm_iec_data_w(state->serbus, device, !BIT(data, 0));

	cassette_output(state->cassette, !BIT(data, 1) ? -(0x5a9e >> 1) : +(0x5a9e >> 1));

	cassette_change_state(state->cassette, BIT(data, 7) ? CASSETTE_MOTOR_DISABLED : CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);
}

UINT8 c16_m7501_port_read( running_device *device, UINT8 direction )
{
	c16_state *state = (c16_state *)device->machine->driver_data;
	UINT8 data = 0xff;
	UINT8 c16_port7501 = m6510_get_port(state->maincpu);

	if (BIT(c16_port7501, 0) || !cbm_iec_data_r(state->serbus))
		data &= ~0x80;

	if (BIT(c16_port7501, 1) || !cbm_iec_clk_r(state->serbus))
		data &= ~0x40;

//  data &= ~0x20; // port bit not in pinout

	if (cassette_input(state->cassette) > +0.0)
		data |=  0x10;
	else
		data &= ~0x10;

	return data;
}

static void c16_bankswitch( running_machine *machine )
{
	c16_state *state = (c16_state *)machine->driver_data;
	UINT8 *rom = memory_region(machine, "maincpu");
	memory_set_bankptr(machine, "bank9", messram_get_ptr(state->messram));

	switch (state->lowrom)
	{
	case 0:
		memory_set_bankptr(machine, "bank2", rom + 0x10000);
		break;
	case 1:
		memory_set_bankptr(machine, "bank2", rom + 0x18000);
		break;
	case 2:
		memory_set_bankptr(machine, "bank2", rom + 0x20000);
		break;
	case 3:
		memory_set_bankptr(machine, "bank2", rom + 0x28000);
		break;
	}

	switch (state->highrom)
	{
	case 0:
		memory_set_bankptr(machine, "bank3", rom + 0x14000);
		memory_set_bankptr(machine, "bank8", rom + 0x17f20);
		break;
	case 1:
		memory_set_bankptr(machine, "bank3", rom + 0x1c000);
		memory_set_bankptr(machine, "bank8", rom + 0x1ff20);
		break;
	case 2:
		memory_set_bankptr(machine, "bank3", rom + 0x24000);
		memory_set_bankptr(machine, "bank8", rom + 0x27f20);
		break;
	case 3:
		memory_set_bankptr(machine, "bank3", rom + 0x2c000);
		memory_set_bankptr(machine, "bank8", rom + 0x2ff20);
		break;
	}
	memory_set_bankptr(machine, "bank4", rom + 0x17c00);
}

WRITE8_HANDLER( c16_switch_to_rom )
{
	c16_state *state = (c16_state *)space->machine->driver_data;

	ted7360_rom_switch_w(state->ted7360, 1);
	c16_bankswitch(space->machine);
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
WRITE8_HANDLER( c16_select_roms )
{
	c16_state *state = (c16_state *)space->machine->driver_data;

	state->lowrom = offset & 0x03;
	state->highrom = (offset & 0x0c) >> 2;
	if (ted7360_rom_switch_r(state->ted7360))
		c16_bankswitch(space->machine);
}

WRITE8_HANDLER( c16_switch_to_ram )
{
	c16_state *state = (c16_state *)space->machine->driver_data;

	ted7360_rom_switch_w(state->ted7360, 0);

	memory_set_bankptr(space->machine, "bank2", messram_get_ptr(state->messram) + (0x8000 % messram_get_size(state->messram)));
	memory_set_bankptr(space->machine, "bank3", messram_get_ptr(state->messram) + (0xc000 % messram_get_size(state->messram)));
	memory_set_bankptr(space->machine, "bank4", messram_get_ptr(state->messram) + (0xfc00 % messram_get_size(state->messram)));
	memory_set_bankptr(space->machine, "bank8", messram_get_ptr(state->messram) + (0xff20 % messram_get_size(state->messram)));
}

UINT8 c16_read_keyboard( running_machine *machine, int databus )
{
	c16_state *state = (c16_state *)machine->driver_data;
	UINT8 value = 0xff;
	int i;

	for (i = 0; i < 8; i++)
	{
		if (!BIT(state->port6529, i))
			value &= state->keyline[i];
	}

	/* looks like joy 0 needs dataline2 low
     * and joy 1 needs dataline1 low
     * write to 0xff08 (value on databus) reloads latches */
	if (!BIT(databus, 2))
		value &= state->keyline[8];

	if (!BIT(databus, 1))
		value &= state->keyline[9];

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
WRITE8_HANDLER( c16_6529_port_w )
{
	c16_state *state = (c16_state *)space->machine->driver_data;
	state->port6529 = data;
}

READ8_HANDLER( c16_6529_port_r )
{
	c16_state *state = (c16_state *)space->machine->driver_data;
	return state->port6529 & (c16_read_keyboard (space->machine, 0xff /*databus */ ) | (state->port6529 ^ 0xff));
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
WRITE8_HANDLER( plus4_6529_port_w )
{
}

READ8_HANDLER( plus4_6529_port_r )
{
	c16_state *state = (c16_state *)space->machine->driver_data;
	int data = 0x00;

	if ((cassette_get_state(state->cassette) & CASSETTE_MASK_UISTATE) != CASSETTE_STOPPED)
		data &= ~0x04;
	else
		data |=  0x04;

	return data;
}

READ8_HANDLER( c16_fd1x_r )
{
	c16_state *state = (c16_state *)space->machine->driver_data;
	int data = 0x00;

	if ((cassette_get_state(state->cassette) & CASSETTE_MASK_UISTATE) != CASSETTE_STOPPED)
		data &= ~0x04;
	else
		data |=  0x04;

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
WRITE8_HANDLER( c16_6551_port_w )
{
	c16_state *state = (c16_state *)space->machine->driver_data;

	offset &= 0x03;
	DBG_LOG(space->machine, 3, "6551", ("port write %.2x %.2x\n", offset, data));
	state->port6529 = data;
}

READ8_HANDLER( c16_6551_port_r )
{
	int data = 0x00;

	offset &= 0x03;
	DBG_LOG(space->machine, 3, "6551", ("port read %.2x %.2x\n", offset, data));
	return data;
}

int c16_dma_read( running_machine *machine, int offset )
{
	c16_state *state = (c16_state *)machine->driver_data;
	return messram_get_ptr(state->messram)[offset % messram_get_size(state->messram)];
}

int c16_dma_read_rom( running_machine *machine, int offset )
{
	c16_state *state = (c16_state *)machine->driver_data;

	/* should read real c16 system bus from 0xfd00 -ff1f */
	if (offset >= 0xc000)
	{								   /* rom address in rom */
		if ((offset >= 0xfc00) && (offset < 0xfd00))
			return state->mem10000[offset];

		switch (state->highrom)
		{
			case 0:
				return state->mem10000[offset & 0x7fff];
			case 1:
				return state->mem18000[offset & 0x7fff];
			case 2:
				return state->mem20000[offset & 0x7fff];
			case 3:
				return state->mem28000[offset & 0x7fff];
		}
	}

	if (offset >= 0x8000)
	{								   /* rom address in rom */
		switch (state->lowrom)
		{
			case 0:
				return state->mem10000[offset & 0x7fff];
			case 1:
				return state->mem18000[offset & 0x7fff];
			case 2:
				return state->mem20000[offset & 0x7fff];
			case 3:
				return state->mem28000[offset & 0x7fff];
		}
	}

	return messram_get_ptr(state->messram)[offset % messram_get_size(state->messram)];
}

void c16_interrupt( running_machine *machine, int level )
{
	c16_state *state = (c16_state *)machine->driver_data;

	if (level != state->old_level)
	{
		DBG_LOG(machine, 3, "mos7501", ("irq %s\n", level ? "start" : "end"));
		cpu_set_input_line(state->maincpu, M6510_IRQ_LINE, level);
		state->old_level = level;
	}
}

static void c16_common_driver_init( running_machine *machine )
{
	c16_state *state = (c16_state *)machine->driver_data;
	UINT8 *rom = memory_region(machine, "maincpu");

	/* initial bankswitch (notice that TED7360 is init to ROM) */
	memory_set_bankptr(machine, "bank2", rom + 0x10000);
	memory_set_bankptr(machine, "bank3", rom + 0x14000);
	memory_set_bankptr(machine, "bank4", rom + 0x17c00);
	memory_set_bankptr(machine, "bank8", rom + 0x17f20);

	state->mem10000 = rom + 0x10000;
	state->mem14000 = rom + 0x14000;
	state->mem18000 = rom + 0x18000;
	state->mem1c000 = rom + 0x1c000;
	state->mem20000 = rom + 0x20000;
	state->mem24000 = rom + 0x24000;
	state->mem28000 = rom + 0x28000;
	state->mem2c000 = rom + 0x2c000;
}

DRIVER_INIT( c16 )
{
	c16_state *state = (c16_state *)machine->driver_data;
	c16_common_driver_init(machine);

	state->sidcard = 0;
	state->pal = 1;
}

DRIVER_INIT( plus4 )
{
	c16_state *state = (c16_state *)machine->driver_data;
	c16_common_driver_init(machine);

	state->sidcard = 0;
	state->pal = 0;
}

DRIVER_INIT( c16sid )
{
	c16_state *state = (c16_state *)machine->driver_data;
	c16_common_driver_init(machine);

	state->sidcard = 1;
	state->pal = 1;
}

DRIVER_INIT( plus4sid )
{
	c16_state *state = (c16_state *)machine->driver_data;
	c16_common_driver_init(machine);

	state->sidcard = 1;
	state->pal = 0;
}

MACHINE_RESET( c16 )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	c16_state *state = (c16_state *)machine->driver_data;

	memset(state->keyline, 0xff, ARRAY_LENGTH(state->keyline));

	state->lowrom = 0;
	state->highrom = 0;
	state->old_level = 0;
	state->port6529 = 0;

	if (state->pal)
	{
		memory_set_bankptr(machine, "bank1", messram_get_ptr(state->messram) + (0x4000 % messram_get_size(state->messram)));

		memory_set_bankptr(machine, "bank5", messram_get_ptr(state->messram) + (0x4000 % messram_get_size(state->messram)));
		memory_set_bankptr(machine, "bank6", messram_get_ptr(state->messram) + (0x8000 % messram_get_size(state->messram)));
		memory_set_bankptr(machine, "bank7", messram_get_ptr(state->messram) + (0xc000 % messram_get_size(state->messram)));

		memory_install_write_bank(space, 0xff20, 0xff3d, 0, 0,"bank10");
		memory_install_write_bank(space, 0xff40, 0xffff, 0, 0, "bank11");
		memory_set_bankptr(machine, "bank10", messram_get_ptr(state->messram) + (0xff20 % messram_get_size(state->messram)));
		memory_set_bankptr(machine, "bank11", messram_get_ptr(state->messram) + (0xff40 % messram_get_size(state->messram)));
	}
	else
	{
		memory_install_write_bank(space, 0x4000, 0xfcff, 0, 0, "bank10");
		memory_set_bankptr(machine, "bank10", messram_get_ptr(state->messram) + (0x4000 % messram_get_size(state->messram)));
	}
}

#if 0
// FIXME
// in very old MESS versions, we had these handlers to enable SID writes to 0xd400.
// would a real SID Card allow for this? If not, this should be removed completely
static WRITE8_HANDLER( c16_sidcart_16k )
{
	c16_state *state = (c16_state *)space->machine->driver_data;

	messram_get_ptr(state->messram)[0x1400 + offset] = data;
	messram_get_ptr(state->messram)[0x5400 + offset] = data;
	messram_get_ptr(state->messram)[0x9400 + offset] = data;
	messram_get_ptr(state->messram)[0xd400 + offset] = data;

	sid6581_w(state->sid, offset, data);
}

static WRITE8_HANDLER( c16_sidcart_64k )
{
	c16_state *state = (c16_state *)space->machine->driver_data;

	messram_get_ptr(state->messram)[0xd400 + offset] = data;

	sid6581_w(state->sid, offset, data);
}

static TIMER_CALLBACK( c16_sidhack_tick )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	c16_state *state = (c16_state *)space->machine->driver_data;

	if (input_port_read_safe(machine, "SID", 0x00) & 0x02)
	{
		if (state->pal)
			memory_install_write8_handler(space, 0xd400, 0xd41f, 0, 0, c16_sidcart_16k);
		else
			memory_install_write8_handler(space, 0xd400, 0xd41f, 0, 0, c16_sidcart_64k);
	}
	else
	{
		memory_unmap_write(space, 0xd400, 0xd41f, 0, 0);
	}
}
#endif

static TIMER_CALLBACK( c16_sidcard_tick )
{
	c16_state *state = (c16_state *)machine->driver_data;
	const address_space *space = cpu_get_address_space(state->maincpu, ADDRESS_SPACE_PROGRAM);

	if (input_port_read_safe(machine, "SID", 0x00) & 0x01)
		memory_install_readwrite8_device_handler(space, state->sid, 0xfe80, 0xfe9f, 0, 0, sid6581_r, sid6581_w);
	else
		memory_install_readwrite8_device_handler(space, state->sid, 0xfd40, 0xfd5f, 0, 0, sid6581_r, sid6581_w);
}

INTERRUPT_GEN( c16_frame_interrupt )
{
	c16_state *state = (c16_state *)device->machine->driver_data;
	int value, i;
	static const char *const c16ports[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4", "ROW5", "ROW6", "ROW7" };

	/* Lines 0-7 : common keyboard */
	for (i = 0; i < 8; i++)
	{
		value = 0xff;
		value &= ~input_port_read(device->machine, c16ports[i]);

		/* Shift Lock is mapped on Left/Right Shift */
		if ((i == 1) && (input_port_read(device->machine, "SPECIAL") & 0x80))
			value &= ~0x80;

		state->keyline[i] = value;
	}

	if (input_port_read(device->machine, "CTRLSEL") & 0x01)
	{
		value = 0xff;
		if (input_port_read(device->machine, "JOY0") & 0x10)			/* Joypad1_Button */
			{
				if (input_port_read(device->machine, "SPECIAL") & 0x40)
					value &= ~0x80;
				else
					value &= ~0x40;
			}

		value &= ~(input_port_read(device->machine, "JOY0") & 0x0f);	/* Other Inputs Joypad1 */

		if (input_port_read(device->machine, "SPECIAL") & 0x40)
			state->keyline[9] = value;
		else
			state->keyline[8] = value;
	}

	if (input_port_read(device->machine, "CTRLSEL") & 0x10)
	{
		value = 0xff;
		if (input_port_read(device->machine, "JOY1") & 0x10)			/* Joypad2_Button */
			{
				if (input_port_read(device->machine, "SPECIAL") & 0x40)
					value &= ~0x40;
				else
					value &= ~0x80;
			}

		value &= ~(input_port_read(device->machine, "JOY1") & 0x0f);	/* Other Inputs Joypad2 */

		if (input_port_read(device->machine, "SPECIAL") & 0x40)
			state->keyline[8] = value;
		else
			state->keyline[9] = value;
	}

	ted7360_frame_interrupt_gen(state->ted7360);

	if (state->sidcard)
	{
		/* if we are emulating the SID card, check which memory area should be accessed */
		timer_set(device->machine, attotime_zero, NULL, 0, c16_sidcard_tick);
#if 0
		/* if we are emulating the SID card, check if writes to 0xd400 have been enabled */
		timer_set(device->machine, attotime_zero, NULL, 0, c16_sidhack_tick);
#endif
	}

	set_led_status(device->machine, 1, input_port_read(device->machine, "SPECIAL") & 0x80 ? 1 : 0);		/* Shift Lock */
	set_led_status(device->machine, 0, input_port_read(device->machine, "SPECIAL") & 0x40 ? 1 : 0);		/* Joystick Swap */
}


/***********************************************

    C16 Cartridges

***********************************************/

static DEVICE_IMAGE_LOAD( c16_cart )
{
	UINT8 *mem = memory_region(image.device().machine, "maincpu");
	int size = image.length(), test;
	const char *filetype;
	int address = 0;

	/* magic lowrom at offset 7: $43 $42 $4d */
	/* if at offset 6 stands 1 it will immediatly jumped to offset 0 (0x8000) */
	static const unsigned char magic[] = {0x43, 0x42, 0x4d};
	unsigned char buffer[sizeof (magic)];

	image.fseek(7, SEEK_SET);
	image.fread( buffer, sizeof (magic));
	image.fseek(0, SEEK_SET);

	/* Check if our cart has the magic string, and set its loading address */
	if (!memcmp(buffer, magic, sizeof (magic)))
		address = 0x20000;

	/* Give a loading address to non .bin / non .rom carts as well */
	filetype = image.filetype();

	/* We would support .hi and .lo files, but currently I'm not sure where to load them.
       We simply load them at 0x20000 at this stage, even if it's probably wrong!
       It could also well be that they both need to be loaded at the same time, but this
       is now impossible since I reduced to 1 the number of cart slots.
       More investigations are in order if any .hi, .lo dump would surface!              */
	if (!mame_stricmp(filetype, "hi"))
		address = 0x20000;	/* FIX ME! */

	else if (!mame_stricmp(filetype, "lo"))
		address = 0x20000;	/* FIX ME! */

	/* As a last try, give a reasonable loading address also to .bin/.rom without the magic string */
	else if (!address)
	{
		logerror("Cart %s does not contain the magic string: it may be loaded at the wrong memory address!\n", image.filename());
		address = 0x20000;
	}

	logerror("Loading cart %s at %.5x size:%.4x\n", image.filename(), address, size);

	/* Finally load the cart */
	test = image.fread( mem + address, size);

	if (test != size)
		return INIT_FAIL;

	return INIT_PASS;
}

MACHINE_DRIVER_START( c16_cartslot )
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("bin,rom,hi,lo")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(c16_cart)
MACHINE_DRIVER_END
