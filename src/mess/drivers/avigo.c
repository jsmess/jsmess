/******************************************************************************

        avigo.c

        TI "Avigo" PDA


        system driver

        Documentation:
                Hans B Pufal
                Avigo simulator


        MEMORY MAP:
                0x0000-0x03fff: flash 0 block 0
                0x4000-0x07fff: flash x block y
                0x8000-0x0bfff: ram block x, screen buffer, or flash x block y
                0xc000-0x0ffff: ram block 0

        Hardware:
            - Z80 CPU
            - 16c500c UART
            -  amd29f080 flash-file memory x 3 (3mb)
            - 128k ram
            - stylus pen
            - touch-pad screen
        TODO:
                Dissassemble the rom a bit and find out exactly
                how memory paging works!

            I don't have any documentation on the hardware, so a lot of this
            driver has been written using educated guesswork and a lot of help
            from an existing emulation written by Hans Pufal. Hans's emulator
            is also written from educated guesswork.

        Kevin Thacker [MESS driver]

 ******************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "includes/avigo.h"
#include "machine/intelfsh.h"
#include "machine/tc8521.h"
#include "machine/ins8250.h"
#include "sound/speaker.h"
#include "devices/messram.h"

static UINT8 avigo_key_line;
/*
    bit 7:                      ?? high priority. When it occurs, clear this bit.
    bit 6: pen int
     An interrupt when pen is pressed against screen.

    bit 5: real time clock


    bit 4:


    bit 3: uart int


    bit 2: synchronisation link interrupt???keyboard int            ;; check bit 5 of port 1,

    bit 1: ???      (cleared in nmi, and then set again)

*/

static UINT8 avigo_irq;

/* bit 3 = speaker state */
static UINT8 avigo_speaker_data;

/* bits 0-5 define bank index */
static unsigned long avigo_ram_bank_l;
static unsigned long avigo_ram_bank_h;
/* bits 0-5 define bank index */
static unsigned long avigo_rom_bank_l;
static unsigned long avigo_rom_bank_h;
static unsigned long avigo_ad_control_status;
static intelfsh8_device *avigo_flashes[3];
static int avigo_flash_at_0x4000;
static int avigo_flash_at_0x8000;
static void *avigo_banked_opbase[4];

static void avigo_setbank(running_machine *machine, int bank, void *address, read8_space_func rh, write8_space_func wh)
{
	address_space* space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	char bank_1[10];
	char bank_5[10];

	sprintf(bank_1,"bank%d",bank + 1);
	sprintf(bank_5,"bank%d",bank + 5);
	if (address)
	{
		memory_set_bankptr(machine, bank_1, address);
		memory_set_bankptr(machine, bank_5, address);
		avigo_banked_opbase[bank] = ((UINT8 *) address) - (bank * 0x4000);
	}
	if (rh)
	{
		memory_install_read8_handler(space, (bank * 0x4000),(bank * 0x4000) + 0x3FFF, 0, 0, rh);
	} else {
//      memory_nop_read(space, (bank * 0x4000),(bank * 0x4000) + 0x3FFF, 0, 0);
	}
	if (wh)
	{
		memory_install_write8_handler(space, (bank * 0x4000),(bank * 0x4000) + 0x3FFF, 0, 0, wh);
	} else {
//      memory_nop_write(space, (bank * 0x4000),(bank * 0x4000) + 0x3FFF, 0, 0);
	}
}

/* memory 0x0000-0x03fff */
static  READ8_HANDLER(avigo_flash_0x0000_read_handler)
{
	return avigo_flashes[0]->read(offset);
}

/* memory 0x04000-0x07fff */
static  READ8_HANDLER(avigo_flash_0x4000_read_handler)
{
	return avigo_flashes[avigo_flash_at_0x4000]->read((avigo_rom_bank_l<<14) | offset);
}

/* memory 0x0000-0x03fff */
static WRITE8_HANDLER(avigo_flash_0x0000_write_handler)
{
	avigo_flashes[0]->write(offset, data);
}

/* memory 0x04000-0x07fff */
static WRITE8_HANDLER(avigo_flash_0x4000_write_handler)
{
	avigo_flashes[avigo_flash_at_0x4000]->write((avigo_rom_bank_l<<14) | offset, data);
}

/* memory 0x08000-0x0bfff */
static  READ8_HANDLER(avigo_flash_0x8000_read_handler)
{
	return avigo_flashes[avigo_flash_at_0x8000]->read((avigo_ram_bank_l<<14) | offset);
}

#ifdef UNUSED_FUNCTION
/* memory 0x08000-0x0bfff */
static WRITE8_HANDLER(avigo_flash_0x8000_write_handler)
{
	avigo_flashes[avigo_flash_at_0x8000]->write((avigo_rom_bank_l<<14) | offset, data);
}
#endif

static void avigo_refresh_ints(running_machine *machine)
{
	if (avigo_irq!=0)
		cputag_set_input_line(machine, "maincpu", 0, HOLD_LINE);
	else
		cputag_set_input_line(machine, "maincpu", 0, CLEAR_LINE);
}


/* previous input port data */
static int previous_input_port_data[4];
static int stylus_marker_x;
static int stylus_marker_y;
static int stylus_press_x;
static int stylus_press_y;

/* this is not a real interrupt. This timer updates the stylus position from mouse
movements, and checks if the mouse button is pressed to emulate a press of the stylus to the screen.
*/
static TIMER_CALLBACK(avigo_dummy_timer_callback)
{
	int i;
	int current_input_port_data[4];
	int changed;
	static int ox = 0, oy = 0;
	int nx,ny;
	int dx, dy;
	static const char *const linenames[] = { "LINE0", "LINE1", "LINE2", "LINE3" };

	for (i = 0; i < 4; i++)
	{
		current_input_port_data[i] = input_port_read(machine, linenames[i]);
	}

	changed = current_input_port_data[3]^previous_input_port_data[3];

	if ((changed & 0x01)!=0)
	{
		if ((current_input_port_data[3] & 0x01)!=0)
		{
			/* pen pressed to screen */

			logerror("pen pressed interrupt\n");
			stylus_press_x = stylus_marker_x;
			stylus_press_y = stylus_marker_y;
			/* set pen interrupt */
			avigo_irq |= (1<<6);
		}
		else
		{
			stylus_press_x = 0;
			stylus_press_y = 0;
		}
	}

	if ((changed & 0x02)!=0)
	{
		if ((current_input_port_data[3] & 0x02)!=0)
		{
			/* ????? causes a NMI */
			cputag_set_input_line(machine, "maincpu", INPUT_LINE_NMI, PULSE_LINE);
		}
	}

	for (i=0; i<4; i++)
	{
		previous_input_port_data[i] = current_input_port_data[i];
	}

	nx = input_port_read(machine, "POSX");
	if (nx>=0x800) nx-=0x1000;
	else if (nx<=-0x800) nx+=0x1000;

	dx = nx - ox;
	ox = nx;

	ny = input_port_read(machine, "POSY");
	if (ny>=0x800) ny-=0x1000;
	else if (ny<=-0x800) ny+=0x1000;

	dy = ny - oy;
	oy = ny;

	stylus_marker_x +=dx;
	stylus_marker_y +=dy;

	avigo_vh_set_stylus_marker_position(stylus_marker_x, stylus_marker_y);
#if 0
	/* not sure if keyboard generates an interrupt, or if something
    is plugged in for synchronisation! */
	/* not sure if this is correct! */
	for (i=0; i<2; i++)
	{
		int changed;
		int current;
		current = ~current_input_port_data[i];

		changed = ((current^(~previous_input_port_data[i])) & 0x07);

		if (changed!=0)
		{
			/* if there are 1 bits remaining, it means there is a bit
            that has changed, the old state was off and new state is on */
			if (current & changed)
			{
				avigo_irq |= (1<<2);
				break;
			}
		}
	}
#endif
	/* copy current to previous */
	memcpy(previous_input_port_data, current_input_port_data, sizeof(int)*4);

	/* refresh status of interrupts */
	avigo_refresh_ints(machine);
}

/* does not do anything yet */
static void avigo_tc8521_alarm_int(running_device *device, int state)
{
//#if 0
	avigo_irq &=~(1<<5);

	if (state)
	{
		avigo_irq |= (1<<5);
	}

	avigo_refresh_ints(device->machine);
//#endif
}


static const tc8521_interface avigo_tc8521_interface =
{
	avigo_tc8521_alarm_int
};

static void avigo_refresh_memory(running_machine *machine)
{
	unsigned char *addr;
	address_space* space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	switch (avigo_rom_bank_h)
	{
		/* 011 */
		case 0x03:
		{
			avigo_flash_at_0x4000 = 1;
		}
		break;

		/* 101 */
		case 0x05:
		{
			avigo_flash_at_0x4000 = 2;
		}
		break;

		default:
			avigo_flash_at_0x4000 = 0;
			break;
	}

	addr = (unsigned char *)avigo_flashes[avigo_flash_at_0x4000]->space()->get_read_ptr(0);
	addr = addr + (avigo_rom_bank_l<<14);
	avigo_setbank(machine, 1, addr, avigo_flash_0x4000_read_handler, avigo_flash_0x4000_write_handler);

	switch (avigo_ram_bank_h)
	{
		/* %101 */
		/* screen */
		case 0x06:
			avigo_setbank(machine, 2, NULL, avigo_vid_memory_r, avigo_vid_memory_w);
			break;

		/* %001 */
		/* ram */
		case 0x01:
			addr = messram_get_ptr(machine->device("messram")) + ((avigo_ram_bank_l & 0x07)<<14);
			memory_set_bankptr(machine, "bank3", addr);
			memory_set_bankptr(machine, "bank7", addr);
			avigo_banked_opbase[2] = ((UINT8 *) addr) - (2 * 0x4000);
			memory_install_read_bank (space, (2 * 0x4000),(2 * 0x4000) + 0x3FFF, 0, 0, "bank3");
			memory_install_write_bank(space, (2 * 0x4000),(2 * 0x4000) + 0x3FFF, 0, 0, "bank7");
			break;

		/* %111 */
		case 0x03:
			avigo_flash_at_0x8000 = 1;


			addr = (unsigned char *)avigo_flashes[avigo_flash_at_0x8000]->space()->get_read_ptr(0);
			addr = addr + (avigo_ram_bank_l<<14);
			avigo_setbank(machine, 2, addr, avigo_flash_0x8000_read_handler, NULL /* avigo_flash_0x8000_write_handler */);
			break;

		case 0x07:
			avigo_flash_at_0x8000 = 0;

			addr = (unsigned char *)avigo_flashes[avigo_flash_at_0x8000]->space()->get_read_ptr(0);
			addr = addr + (avigo_ram_bank_l<<14);
			avigo_setbank(machine, 2, addr, avigo_flash_0x8000_read_handler, NULL /* avigo_flash_0x8000_write_handler */);
			break;
	}
}



static INS8250_INTERRUPT( avigo_com_interrupt )
{
	logerror("com int\r\n");

	avigo_irq &= ~(1<<3);

	if (state)
	{
		avigo_irq |= (1<<3);
	}

	avigo_refresh_ints(device->machine);
}



static const ins8250_interface avigo_com_interface =
{
	1843200,
	avigo_com_interrupt,
	NULL,
	NULL,
	NULL
};

/* this is needed because this driver uses handlers in memory that gets executed */
DIRECT_UPDATE_HANDLER( avigo_opbase_handler )
{
	void *opbase_ptr;

	opbase_ptr = avigo_banked_opbase[address / 0x4000];
	if (opbase_ptr != NULL)
	{
		direct.explicit_configure(0x0000, 0xffff, 0xffff, (UINT8*)opbase_ptr);
		address = ~0;
	}
	return address;
}

static MACHINE_RESET( avigo )
{
	int i;
	unsigned char *addr;
	static const char *const linenames[] = { "LINE0", "LINE1", "LINE2", "LINE3" };

	memset(avigo_banked_opbase, 0, sizeof(avigo_banked_opbase));

	/* keep machine pointers to flash devices */
	avigo_flashes[0] = machine->device<intelfsh8_device>("flash0");
	avigo_flashes[1] = machine->device<intelfsh8_device>("flash1");
	avigo_flashes[2] = machine->device<intelfsh8_device>("flash2");

	/* initialize flash contents */
	memcpy(memory_region(machine, "maincpu")+0x10000, avigo_flashes[0]->space()->get_read_ptr(0), 0x100000);
	memcpy(memory_region(machine, "maincpu")+0x110000, avigo_flashes[1]->space()->get_read_ptr(0), 0x100000);

	stylus_marker_x = AVIGO_SCREEN_WIDTH>>1;
	stylus_marker_y = AVIGO_SCREEN_HEIGHT>>1;
	stylus_press_x = 0;
	stylus_press_y = 0;
	avigo_vh_set_stylus_marker_position(stylus_marker_x, stylus_marker_y);

	/* initialise settings for port data */
	for (i = 0; i < 4; i++)
	{
		previous_input_port_data[i] = input_port_read(machine, linenames[i]);
	}

	avigo_irq = 0;
	avigo_rom_bank_l = 0;
	avigo_rom_bank_h = 0;
	avigo_ram_bank_l = 0;
	avigo_ram_bank_h = 0;
	avigo_flash_at_0x4000 = 0;
	avigo_flash_at_0x8000 = 0;

	/* clear */
	memset(messram_get_ptr(machine->device("messram")), 0, 128*1024);

	cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM)->set_direct_update_handler(direct_update_delegate_create_static(avigo_opbase_handler, *machine));

	addr = (unsigned char *)avigo_flashes[0]->space()->get_read_ptr(0);
	avigo_setbank(machine, 0, addr, avigo_flash_0x0000_read_handler, avigo_flash_0x0000_write_handler);

	avigo_setbank(machine, 3, messram_get_ptr(machine->device("messram")), NULL, NULL);

	/* 0x08000 is specially banked! */
	avigo_refresh_memory(machine);
}

static MACHINE_START( avigo )
{
	/* a timer used to check status of pen */
	/* an interrupt is generated when the pen is pressed to the screen */
	timer_pulse(machine, ATTOTIME_IN_HZ(50), NULL, 0, avigo_dummy_timer_callback);
}


static ADDRESS_MAP_START( avigo_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_READ_BANK("bank1") AM_WRITE_BANK("bank5")
	AM_RANGE(0x4000, 0x7fff) AM_READ_BANK("bank2") AM_WRITE_BANK("bank6")
	AM_RANGE(0x8000, 0xbfff) AM_READ_BANK("bank3") AM_WRITE_BANK("bank7")
	AM_RANGE(0xc000, 0xffff) AM_READ_BANK("bank4") AM_WRITE_BANK("bank8")
ADDRESS_MAP_END


static  READ8_HANDLER(avigo_key_data_read_r)
{
	UINT8 data;

	data = 0x007;

	if (avigo_key_line & 0x01)
	{
		data &= input_port_read(space->machine, "LINE0");
	}

	if (avigo_key_line & 0x02)
	{
		data &= input_port_read(space->machine, "LINE1");
	}

	if (avigo_key_line & 0x04)
	{
		data &= input_port_read(space->machine, "LINE2");

	}

	/* if bit 5 is clear shows synchronisation logo! */
	/* bit 3 must be set, otherwise there is an infinite loop in startup */
	data |= (1<<3) | (1<<5);

	return data;
}


/* set key line(s) to read */
/* bit 0 set for line 0, bit 1 set for line 1, bit 2 set for line 2 */
static WRITE8_HANDLER(avigo_set_key_line_w)
{
	/* 5, 101, read back 3 */
	avigo_key_line = data;
}

static  READ8_HANDLER(avigo_irq_r)
{
	return avigo_irq;
}

static WRITE8_HANDLER(avigo_irq_w)
{
	avigo_irq &= ~data;

	avigo_refresh_ints(space->machine);
}

static  READ8_HANDLER(avigo_rom_bank_l_r)
{
	return avigo_rom_bank_l;
}

static  READ8_HANDLER(avigo_rom_bank_h_r)
{
	return avigo_rom_bank_h;
}

static  READ8_HANDLER(avigo_ram_bank_l_r)
{
	return avigo_ram_bank_l;
}

static  READ8_HANDLER(avigo_ram_bank_h_r)
{
	return avigo_ram_bank_h;
}



static WRITE8_HANDLER(avigo_rom_bank_l_w)
{
	logerror("rom bank l w: %04x\n", data);

        avigo_rom_bank_l = data & 0x03f;

        avigo_refresh_memory(space->machine);
}

static WRITE8_HANDLER(avigo_rom_bank_h_w)
{
	logerror("rom bank h w: %04x\n", data);


        /* 000 = flash 0
           001 = ram select
           011 = flash 1 (rom at ram - block 1 select)
           101 = flash 2
           110 = screen select?
           111 = flash 0 (rom at ram?)


        */
	avigo_rom_bank_h = data;


        avigo_refresh_memory(space->machine);
}

static WRITE8_HANDLER(avigo_ram_bank_l_w)
{
	logerror("ram bank l w: %04x\n", data);

        avigo_ram_bank_l = data & 0x03f;

        avigo_refresh_memory(space->machine);
}

static WRITE8_HANDLER(avigo_ram_bank_h_w)
{
	logerror("ram bank h w: %04x\n", data);

	avigo_ram_bank_h = data;

        avigo_refresh_memory(space->machine);
}

static  READ8_HANDLER(avigo_ad_control_status_r)
{
	logerror("avigo ad control read %02x\n", (int) avigo_ad_control_status);
	return avigo_ad_control_status;
}

static unsigned int avigo_ad_value;

static WRITE8_HANDLER(avigo_ad_control_status_w)
{
	logerror("avigo ad control w %02x\n",data);

	if ((data & 0x070)==0x070)
	{
		/* bit 3 appears to select between 1 = x coord, 0 = y coord */
		/* when 6,5,4 = 1 */
		if ((data & 0x08)!=0)
		{
			logerror("a/d select x coordinate\n");
			logerror("x coord: %d\n",stylus_press_x);

			/* on screen range 0x060->0x03a0 */
			/* 832 is on-screen range */
			/* 5.2 a/d units per pixel */

			if (stylus_press_x!=0)
			{
				/* this might not be totally accurate because hitable screen
                area may include the border around the screen! */
				avigo_ad_value = ((int)(stylus_press_x * 5.2f))+0x060;
				avigo_ad_value &= 0x03fc;
			}
			else
			{
				avigo_ad_value = 0;
			}

			logerror("ad value: %d\n",avigo_ad_value);
			stylus_press_x = 0;

		}
		else
		{
			/* in the avigo rom, the y coordinate is inverted! */
			/* therefore a low value would be near the bottom of the display,
            and a high value at the top */

			/* total valid range 0x044->0x036a */
			/* 0x0350 is also checked */

			/* assumption 0x044->0x0350 is screen area and
            0x0350->0x036a is panel at bottom */

			/* 780 is therefore on-screen range */
			/* 3.25 a/d units per pixel */
			/* a/d unit * a/d range = total height */
			/* 3.25 * 1024.00 = 315.07 */

			logerror("a/d select y coordinate\n");
			logerror("y coord: %d\n",stylus_press_y);

			if (stylus_press_y!=0)
			{
				avigo_ad_value = 1024 - (((stylus_press_y)*3.25f) + 0x040);
			}
			else
			{
				avigo_ad_value = 0;
			}

			logerror("ad value: %d\n",avigo_ad_value);
			stylus_press_y = 0;
		}
	}

	/* bit 0: 1 if a/d complete, 0 if a/d not complete */
	avigo_ad_control_status = data | 1;
}

static  READ8_HANDLER(avigo_ad_data_r)
{
	unsigned char data;

	data = 0;

	/* original */

	/* status AND   11110111 */
	/* status OR    01110000 -> C20F */

	switch (avigo_ad_control_status & 0x078)
	{
		/* x1110xxx */
		/* read upper 4 bits of 10 bit A/D number */
		case 0x070:
		case 0x078:
		{
			/* upper 4 bits of 10 bit A/D number in bits 7-4 of data */
			/* bit 0 must be 0, bit 1 must be 0 */
			/* bit 3 must be 1. bit 2 can have any value */

			logerror("a/d read upper 4 bits\n");
			data = ((avigo_ad_value>>6) & 0x0f)<<4;
			data |= 8;
		}
		break;

		/* x0111xxx */
		case 0x038:
		{
			/* lower 6 bits of 10-bit A/D number in bits 7-2 of data */
			/* bit 0 must be 1, bit 1 must be 0 */

			logerror("a/d lower 6-bits\n");
			data = ((avigo_ad_value & 0x03f)<<2);
			data |= 1;
		}
		break;

		default:
			break;
	}

	/* x coord? */
	/* wait for bit 0 of status to become 1 */
	/* read data -> d */


	/* C20f AND 10111111 */
	/* C20f OR  00001000 */
	/* x0111xxx */

	/* bit 1 must be 0, bit 0 must be 1 */
	/* read data -> e */

	/* upper 4 bits of d contain data */
	/* bits 0 and 1 do not contain data of e, but all other bits do */

	/* get bit 5 and 6 of d */
	/* and put into bit 0 and 1 of e */

	/* C20f OR  01000000 */
	/* x1111xxx */

	/* y coord? */
	/* bit 0 must be 0, bit 1 must be 0 */
	/* bit 3 must be 1. bit 2 can have any value */
	/* read data -> d */

	/* C20f AND  10111111 */
	/* x0111xxx */

	/* bit 1 must be 0, bit 0 must be 1 */
	/* read data -> e */


	/* original and 1111100 */
	/* original or  1111000 */
	/* 1111x00 */



	/* if fails! */
	/* original */
	/* AND 1001100 */
	/* OR  1001000 */
	/* 1001x00 */


	/* AND 1101100 */
	/* OR  1101000 */
	/* 1101x00 */

	/* 1111x00 */

	logerror("avigo ad read %02x\n",data);

	return data;
}


static WRITE8_HANDLER(avigo_speaker_w)
{
	running_device *speaker = space->machine->device("speaker");
	UINT8 previous_speaker;

	previous_speaker = avigo_speaker_data;
	avigo_speaker_data = data;

	/* changed state? */
	if (((data^avigo_speaker_data) & (1<<3))!=0)
	{
		/* DAC output state */
		speaker_level_w(speaker,(data>>3) & 0x01);
	}
}

static  READ8_HANDLER(avigo_unmapped_r)
{
	logerror("read unmapped port\n");
	return 0x0ff;
}

/* port 0x04:

  bit 7: ??? if set, does a write 0x00 to 0x02e */

  /* port 0x029:
    port 0x02e */
static  READ8_HANDLER(avigo_04_r)
{
	/* must be both 0 for it to boot! */
	return 0x0ff^((1<<7) | (1<<5));
}



static ADDRESS_MAP_START( avigo_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)

	AM_RANGE(0x000, 0x000) AM_READ( avigo_unmapped_r)
	AM_RANGE(0x001, 0x001) AM_READWRITE( avigo_key_data_read_r, avigo_set_key_line_w )
	AM_RANGE(0x002, 0x002) AM_READ( avigo_unmapped_r)
	AM_RANGE(0x003, 0x003) AM_READWRITE( avigo_irq_r, avigo_irq_w )
	AM_RANGE(0x004, 0x004) AM_READ( avigo_04_r)
	AM_RANGE(0x005, 0x005) AM_READWRITE( avigo_rom_bank_l_r, avigo_rom_bank_l_w )
	AM_RANGE(0x006, 0x006) AM_READWRITE( avigo_rom_bank_h_r, avigo_rom_bank_h_w )
	AM_RANGE(0x007, 0x007) AM_READWRITE( avigo_ram_bank_l_r, avigo_ram_bank_l_w )
	AM_RANGE(0x008, 0x008) AM_READWRITE( avigo_ram_bank_h_r, avigo_ram_bank_h_w )
	AM_RANGE(0x009, 0x009) AM_READWRITE( avigo_ad_control_status_r, avigo_ad_control_status_w )
	AM_RANGE(0x00a, 0x00f) AM_READ( avigo_unmapped_r)
	AM_RANGE(0x010, 0x01f) AM_DEVREADWRITE("rtc", tc8521_r, tc8521_w )
	AM_RANGE(0x020, 0x02c) AM_READ( avigo_unmapped_r)
	AM_RANGE(0x028, 0x028) AM_WRITE( avigo_speaker_w)
	AM_RANGE(0x02d, 0x02d) AM_READ( avigo_ad_data_r)
	AM_RANGE(0x02e, 0x02f) AM_READ( avigo_unmapped_r)
	AM_RANGE(0x030, 0x037) AM_DEVREADWRITE("ns16550", ins8250_r, ins8250_w )
	AM_RANGE(0x038, 0x0ff) AM_READ( avigo_unmapped_r)
ADDRESS_MAP_END



static INPUT_PORTS_START(avigo)
	PORT_START("LINE0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PAGE UP") PORT_CODE(KEYCODE_PGUP)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PAGE DOWN") PORT_CODE(KEYCODE_PGDN)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LIGHT") PORT_CODE(KEYCODE_L)
	PORT_BIT(0xf8, 0xf8, IPT_UNUSED)

	PORT_START("LINE1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TO DO") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ADDRESS") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SCHEDULE") PORT_CODE(KEYCODE_S)
	PORT_BIT(0xf8, 0xf8, IPT_UNUSED)

	PORT_START("LINE2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MEMO") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x0fe, 0xfe, IPT_UNUSED)

	PORT_START("LINE3")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Pen/Stylus pressed") PORT_CODE(KEYCODE_Q) PORT_CODE(JOYCODE_BUTTON1)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("?? Causes a NMI") PORT_CODE(KEYCODE_W) PORT_CODE(JOYCODE_BUTTON2)

	/* these two ports are used to emulate the position of the pen/stylus on the screen */
	/* a cursor is drawn to indicate the position, so when a click is done, it will occur in the correct place */
	/* To be converted to crosshair code? */
	PORT_START("POSX") /* Mouse - X AXIS */
	PORT_BIT(0xfff, 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

	PORT_START("POSY") /* Mouse - Y AXIS */
	PORT_BIT(0xfff, 0x00, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)
INPUT_PORTS_END



static MACHINE_CONFIG_START( avigo, driver_device )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(avigo_mem)
	MDRV_CPU_IO_MAP(avigo_io)
	MDRV_QUANTUM_TIME(HZ(60))

	MDRV_MACHINE_START( avigo )
	MDRV_MACHINE_RESET( avigo )

	MDRV_NS16550_ADD( "ns16550", avigo_com_interface )

    /* video hardware */
	MDRV_SCREEN_ADD("screen", LCD)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_PALETTE_LENGTH(256)
	MDRV_PALETTE_INIT( avigo )

	MDRV_VIDEO_START( avigo )
	MDRV_VIDEO_UPDATE( avigo )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* real time clock */
	MDRV_TC8521_ADD("rtc", avigo_tc8521_interface)

	/* flash ROMs */
	MDRV_INTEL_E28F008SA_ADD("flash0")
	MDRV_INTEL_E28F008SA_ADD("flash1")
	MDRV_INTEL_E28F008SA_ADD("flash2")

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("128K")
MACHINE_CONFIG_END


/***************************************************************************

  Game driver(s)

***************************************************************************/
ROM_START(avigo)
	ROM_REGION(0x210000, "maincpu",0)
	ROM_LOAD("avigo.rom", 0x010000, 0x0150000, CRC(160ee4a6) SHA1(4d09201a3876de16808bd92989f3d8d7182d72b3))
ROM_END

/*    YEAR  NAME    PARENT  COMPAT  MACHINE INPUT   INIT    COMPANY   FULLNAME */
COMP(1997,	avigo,	0,		0,		avigo,	avigo,	0,		"Texas Instruments", "TI Avigo 100 PDA",GAME_NOT_WORKING)

