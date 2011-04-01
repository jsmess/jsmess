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
#include "machine/tc8521.h"
#include "machine/ins8250.h"
#include "sound/speaker.h"
#include "machine/ram.h"

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


/* bit 3 = speaker state */

/* bits 0-5 define bank index */
/* bits 0-5 define bank index */

static void avigo_setbank(running_machine &machine, int bank, void *address, read8_space_func rh, write8_space_func wh)
{
	avigo_state *state = machine.driver_data<avigo_state>();
	address_space* space = machine.device("maincpu")->memory().space(AS_PROGRAM);
	char bank_1[10];
	char bank_5[10];

	sprintf(bank_1,"bank%d",bank + 1);
	sprintf(bank_5,"bank%d",bank + 5);
	if (address)
	{
		memory_set_bankptr(machine, bank_1, address);
		memory_set_bankptr(machine, bank_5, address);
		state->m_banked_opbase[bank] = ((UINT8 *) address) - (bank * 0x4000);
	}
	if (rh)
	{
		space->install_legacy_read_handler((bank * 0x4000),(bank * 0x4000) + 0x3FFF, FUNC(rh));
	} else {
//      space->nop_read((bank * 0x4000),(bank * 0x4000) + 0x3FFF);
	}
	if (wh)
	{
		space->install_legacy_write_handler((bank * 0x4000),(bank * 0x4000) + 0x3FFF, FUNC(wh));
	} else {
//      space->nop_write((bank * 0x4000),(bank * 0x4000) + 0x3FFF);
	}
}

/* memory 0x0000-0x03fff */
static  READ8_HANDLER(avigo_flash_0x0000_read_handler)
{
	avigo_state *state = space->machine().driver_data<avigo_state>();
	return state->m_flashes[0]->read(offset);
}

/* memory 0x04000-0x07fff */
static  READ8_HANDLER(avigo_flash_0x4000_read_handler)
{
	avigo_state *state = space->machine().driver_data<avigo_state>();
	return state->m_flashes[state->m_flash_at_0x4000]->read((state->m_rom_bank_l<<14) | offset);
}

/* memory 0x0000-0x03fff */
static WRITE8_HANDLER(avigo_flash_0x0000_write_handler)
{
	avigo_state *state = space->machine().driver_data<avigo_state>();
	state->m_flashes[0]->write(offset, data);
}

/* memory 0x04000-0x07fff */
static WRITE8_HANDLER(avigo_flash_0x4000_write_handler)
{
	avigo_state *state = space->machine().driver_data<avigo_state>();
	state->m_flashes[state->m_flash_at_0x4000]->write((state->m_rom_bank_l<<14) | offset, data);
}

/* memory 0x08000-0x0bfff */
static  READ8_HANDLER(avigo_flash_0x8000_read_handler)
{
	avigo_state *state = space->machine().driver_data<avigo_state>();
	return state->m_flashes[state->m_flash_at_0x8000]->read((state->m_ram_bank_l<<14) | offset);
}

#ifdef UNUSED_FUNCTION
/* memory 0x08000-0x0bfff */
static WRITE8_HANDLER(avigo_flash_0x8000_write_handler)
{
	avigo_state *state = space->machine().driver_data<avigo_state>();
	state->m_flashes[state->m_flash_at_0x8000]->write((state->m_rom_bank_l<<14) | offset, data);
}
#endif

static void avigo_refresh_ints(running_machine &machine)
{
	avigo_state *state = machine.driver_data<avigo_state>();
	if (state->m_irq!=0)
		cputag_set_input_line(machine, "maincpu", 0, HOLD_LINE);
	else
		cputag_set_input_line(machine, "maincpu", 0, CLEAR_LINE);
}


/* previous input port data */

/* this is not a real interrupt. This timer updates the stylus position from mouse
movements, and checks if the mouse button is pressed to emulate a press of the stylus to the screen.
*/
static TIMER_DEVICE_CALLBACK(avigo_dummy_timer_callback)
{
	avigo_state *state = timer.machine().driver_data<avigo_state>();
	int i;
	int current_input_port_data[4];
	int changed;
	int nx,ny;
	int dx, dy;
	static const char *const linenames[] = { "LINE0", "LINE1", "LINE2", "LINE3" };

	for (i = 0; i < 4; i++)
	{
		current_input_port_data[i] = input_port_read(timer.machine(), linenames[i]);
	}

	changed = current_input_port_data[3]^state->m_previous_input_port_data[3];

	if ((changed & 0x01)!=0)
	{
		if ((current_input_port_data[3] & 0x01)!=0)
		{
			/* pen pressed to screen */

			logerror("pen pressed interrupt\n");
			state->m_stylus_press_x = state->m_stylus_marker_x;
			state->m_stylus_press_y = state->m_stylus_marker_y;
			/* set pen interrupt */
			state->m_irq |= (1<<6);
		}
		else
		{
			state->m_stylus_press_x = 0;
			state->m_stylus_press_y = 0;
		}
	}

	if ((changed & 0x02)!=0)
	{
		if ((current_input_port_data[3] & 0x02)!=0)
		{
			/* ????? causes a NMI */
			cputag_set_input_line(timer.machine(), "maincpu", INPUT_LINE_NMI, PULSE_LINE);
		}
	}

	for (i=0; i<4; i++)
	{
		state->m_previous_input_port_data[i] = current_input_port_data[i];
	}

	nx = input_port_read(timer.machine(), "POSX");
	if (nx>=0x800) nx-=0x1000;
	else if (nx<=-0x800) nx+=0x1000;

	dx = nx - state->m_ox;
	state->m_ox = nx;

	ny = input_port_read(timer.machine(), "POSY");
	if (ny>=0x800) ny-=0x1000;
	else if (ny<=-0x800) ny+=0x1000;

	dy = ny - state->m_oy;
	state->m_oy = ny;

	state->m_stylus_marker_x +=dx;
	state->m_stylus_marker_y +=dy;

	avigo_vh_set_stylus_marker_position(timer.machine(), state->m_stylus_marker_x, state->m_stylus_marker_y);
#if 0
	/* not sure if keyboard generates an interrupt, or if something
    is plugged in for synchronisation! */
	/* not sure if this is correct! */
	for (i=0; i<2; i++)
	{
		int changed;
		int current;
		current = ~current_input_port_data[i];

		changed = ((current^(~state->m_previous_input_port_data[i])) & 0x07);

		if (changed!=0)
		{
			/* if there are 1 bits remaining, it means there is a bit
            that has changed, the old state was off and new state is on */
			if (current & changed)
			{
				state->m_irq |= (1<<2);
				break;
			}
		}
	}
#endif
	/* copy current to previous */
	memcpy(state->m_previous_input_port_data, current_input_port_data, sizeof(int)*4);

	/* refresh status of interrupts */
	avigo_refresh_ints(timer.machine());
}

/* does not do anything yet */
static void avigo_tc8521_alarm_int(device_t *device, int state)
{
	avigo_state *drvstate = device->machine().driver_data<avigo_state>();
//#if 0
	drvstate->m_irq &=~(1<<5);

	if (state)
	{
		drvstate->m_irq |= (1<<5);
	}

	avigo_refresh_ints(device->machine());
//#endif
}


static const tc8521_interface avigo_tc8521_interface =
{
	avigo_tc8521_alarm_int
};

static void avigo_refresh_memory(running_machine &machine)
{
	avigo_state *state = machine.driver_data<avigo_state>();
	unsigned char *addr;
	address_space* space = machine.device("maincpu")->memory().space(AS_PROGRAM);

	switch (state->m_rom_bank_h)
	{
		/* 011 */
		case 0x03:
		{
			state->m_flash_at_0x4000 = 1;
		}
		break;

		/* 101 */
		case 0x05:
		{
			state->m_flash_at_0x4000 = 2;
		}
		break;

		default:
			state->m_flash_at_0x4000 = 0;
			break;
	}

	addr = (unsigned char *)state->m_flashes[state->m_flash_at_0x4000]->space()->get_read_ptr(0);
	addr = addr + (state->m_rom_bank_l<<14);
	avigo_setbank(machine, 1, addr, avigo_flash_0x4000_read_handler, avigo_flash_0x4000_write_handler);

	switch (state->m_ram_bank_h)
	{
		/* %101 */
		/* screen */
		case 0x06:
			avigo_setbank(machine, 2, NULL, avigo_vid_memory_r, avigo_vid_memory_w);
			break;

		/* %001 */
		/* ram */
		case 0x01:
			addr = ram_get_ptr(machine.device(RAM_TAG)) + ((state->m_ram_bank_l & 0x07)<<14);
			memory_set_bankptr(machine, "bank3", addr);
			memory_set_bankptr(machine, "bank7", addr);
			state->m_banked_opbase[2] = ((UINT8 *) addr) - (2 * 0x4000);
			space->install_read_bank ((2 * 0x4000),(2 * 0x4000) + 0x3FFF, "bank3");
			space->install_write_bank((2 * 0x4000),(2 * 0x4000) + 0x3FFF, "bank7");
			break;

		/* %111 */
		case 0x03:
			state->m_flash_at_0x8000 = 1;


			addr = (unsigned char *)state->m_flashes[state->m_flash_at_0x8000]->space()->get_read_ptr(0);
			addr = addr + (state->m_ram_bank_l<<14);
			avigo_setbank(machine, 2, addr, avigo_flash_0x8000_read_handler, NULL /* avigo_flash_0x8000_write_handler */);
			break;

		case 0x07:
			state->m_flash_at_0x8000 = 0;

			addr = (unsigned char *)state->m_flashes[state->m_flash_at_0x8000]->space()->get_read_ptr(0);
			addr = addr + (state->m_ram_bank_l<<14);
			avigo_setbank(machine, 2, addr, avigo_flash_0x8000_read_handler, NULL /* avigo_flash_0x8000_write_handler */);
			break;
	}
}



static INS8250_INTERRUPT( avigo_com_interrupt )
{
	avigo_state *drvstate = device->machine().driver_data<avigo_state>();
	logerror("com int\r\n");

	drvstate->m_irq &= ~(1<<3);

	if (state)
	{
		drvstate->m_irq |= (1<<3);
	}

	avigo_refresh_ints(device->machine());
}



static const ins8250_interface avigo_com_interface =
{
	1843200,
	avigo_com_interrupt,
	NULL,
	NULL,
	NULL
};


static MACHINE_RESET( avigo )
{
	avigo_state *state = machine.driver_data<avigo_state>();
	int i;
	unsigned char *addr;
	static const char *const linenames[] = { "LINE0", "LINE1", "LINE2", "LINE3" };

	memset(state->m_banked_opbase, 0, sizeof(state->m_banked_opbase));

	/* keep machine pointers to flash devices */
	state->m_flashes[0] = machine.device<intelfsh8_device>("flash0");
	state->m_flashes[1] = machine.device<intelfsh8_device>("flash1");
	state->m_flashes[2] = machine.device<intelfsh8_device>("flash2");

	/* initialize flash contents */
	memcpy(state->m_flashes[0]->space()->get_read_ptr(0), machine.region("maincpu")->base()+0x10000, 0x100000);
	memcpy(state->m_flashes[1]->space()->get_read_ptr(0), machine.region("maincpu")->base()+0x110000, 0x100000);

	state->m_stylus_marker_x = AVIGO_SCREEN_WIDTH>>1;
	state->m_stylus_marker_y = AVIGO_SCREEN_HEIGHT>>1;
	state->m_stylus_press_x = 0;
	state->m_stylus_press_y = 0;
	avigo_vh_set_stylus_marker_position(machine, state->m_stylus_marker_x, state->m_stylus_marker_y);

	/* initialise settings for port data */
	for (i = 0; i < 4; i++)
	{
		state->m_previous_input_port_data[i] = input_port_read(machine, linenames[i]);
	}

	state->m_irq = 0;
	state->m_rom_bank_l = 0;
	state->m_rom_bank_h = 0;
	state->m_ram_bank_l = 0;
	state->m_ram_bank_h = 0;
	state->m_flash_at_0x4000 = 0;
	state->m_flash_at_0x8000 = 0;

	/* clear */
	memset(ram_get_ptr(machine.device(RAM_TAG)), 0, 128*1024);

	addr = (unsigned char *)state->m_flashes[0]->space()->get_read_ptr(0);
	avigo_setbank(machine, 0, addr, avigo_flash_0x0000_read_handler, avigo_flash_0x0000_write_handler);

	avigo_setbank(machine, 3, ram_get_ptr(machine.device(RAM_TAG)), NULL, NULL);

	/* 0x08000 is specially banked! */
	avigo_refresh_memory(machine);
}

static MACHINE_START( avigo )
{
}

static ADDRESS_MAP_START( avigo_mem , AS_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_READ_BANK("bank1") AM_WRITE_BANK("bank5")
	AM_RANGE(0x4000, 0x7fff) AM_READ_BANK("bank2") AM_WRITE_BANK("bank6")
	AM_RANGE(0x8000, 0xbfff) AM_READ_BANK("bank3") AM_WRITE_BANK("bank7")
	AM_RANGE(0xc000, 0xffff) AM_READ_BANK("bank4") AM_WRITE_BANK("bank8")
ADDRESS_MAP_END


static  READ8_HANDLER(avigo_key_data_read_r)
{
	avigo_state *state = space->machine().driver_data<avigo_state>();
	UINT8 data;

	data = 0x007;

	if (state->m_key_line & 0x01)
	{
		data &= input_port_read(space->machine(), "LINE0");
	}

	if (state->m_key_line & 0x02)
	{
		data &= input_port_read(space->machine(), "LINE1");
	}

	if (state->m_key_line & 0x04)
	{
		data &= input_port_read(space->machine(), "LINE2");

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
	avigo_state *state = space->machine().driver_data<avigo_state>();
	/* 5, 101, read back 3 */
	state->m_key_line = data;
}

static  READ8_HANDLER(avigo_irq_r)
{
	avigo_state *state = space->machine().driver_data<avigo_state>();
	return state->m_irq;
}

static WRITE8_HANDLER(avigo_irq_w)
{
	avigo_state *state = space->machine().driver_data<avigo_state>();
	state->m_irq &= ~data;

	avigo_refresh_ints(space->machine());
}

static  READ8_HANDLER(avigo_rom_bank_l_r)
{
	avigo_state *state = space->machine().driver_data<avigo_state>();
	return state->m_rom_bank_l;
}

static  READ8_HANDLER(avigo_rom_bank_h_r)
{
	avigo_state *state = space->machine().driver_data<avigo_state>();
	return state->m_rom_bank_h;
}

static  READ8_HANDLER(avigo_ram_bank_l_r)
{
	avigo_state *state = space->machine().driver_data<avigo_state>();
	return state->m_ram_bank_l;
}

static  READ8_HANDLER(avigo_ram_bank_h_r)
{
	avigo_state *state = space->machine().driver_data<avigo_state>();
	return state->m_ram_bank_h;
}



static WRITE8_HANDLER(avigo_rom_bank_l_w)
{
	avigo_state *state = space->machine().driver_data<avigo_state>();
	logerror("rom bank l w: %04x\n", data);

        state->m_rom_bank_l = data & 0x03f;

        avigo_refresh_memory(space->machine());
}

static WRITE8_HANDLER(avigo_rom_bank_h_w)
{
	avigo_state *state = space->machine().driver_data<avigo_state>();
	logerror("rom bank h w: %04x\n", data);


        /* 000 = flash 0
           001 = ram select
           011 = flash 1 (rom at ram - block 1 select)
           101 = flash 2
           110 = screen select?
           111 = flash 0 (rom at ram?)


        */
	state->m_rom_bank_h = data;


        avigo_refresh_memory(space->machine());
}

static WRITE8_HANDLER(avigo_ram_bank_l_w)
{
	avigo_state *state = space->machine().driver_data<avigo_state>();
	logerror("ram bank l w: %04x\n", data);

        state->m_ram_bank_l = data & 0x03f;

        avigo_refresh_memory(space->machine());
}

static WRITE8_HANDLER(avigo_ram_bank_h_w)
{
	avigo_state *state = space->machine().driver_data<avigo_state>();
	logerror("ram bank h w: %04x\n", data);

	state->m_ram_bank_h = data;

        avigo_refresh_memory(space->machine());
}

static  READ8_HANDLER(avigo_ad_control_status_r)
{
	avigo_state *state = space->machine().driver_data<avigo_state>();
	logerror("avigo ad control read %02x\n", (int) state->m_ad_control_status);
	return state->m_ad_control_status;
}


static WRITE8_HANDLER(avigo_ad_control_status_w)
{
	avigo_state *state = space->machine().driver_data<avigo_state>();
	logerror("avigo ad control w %02x\n",data);

	if ((data & 0x070)==0x070)
	{
		/* bit 3 appears to select between 1 = x coord, 0 = y coord */
		/* when 6,5,4 = 1 */
		if ((data & 0x08)!=0)
		{
			logerror("a/d select x coordinate\n");
			logerror("x coord: %d\n",state->m_stylus_press_x);

			/* on screen range 0x060->0x03a0 */
			/* 832 is on-screen range */
			/* 5.2 a/d units per pixel */

			if (state->m_stylus_press_x!=0)
			{
				/* this might not be totally accurate because hitable screen
                area may include the border around the screen! */
				state->m_ad_value = ((int)(state->m_stylus_press_x * 5.2f))+0x060;
				state->m_ad_value &= 0x03fc;
			}
			else
			{
				state->m_ad_value = 0;
			}

			logerror("ad value: %d\n",state->m_ad_value);
			state->m_stylus_press_x = 0;

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
			logerror("y coord: %d\n",state->m_stylus_press_y);

			if (state->m_stylus_press_y!=0)
			{
				state->m_ad_value = 1024 - (((state->m_stylus_press_y)*3.25f) + 0x040);
			}
			else
			{
				state->m_ad_value = 0;
			}

			logerror("ad value: %d\n",state->m_ad_value);
			state->m_stylus_press_y = 0;
		}
	}

	/* bit 0: 1 if a/d complete, 0 if a/d not complete */
	state->m_ad_control_status = data | 1;
}

static  READ8_HANDLER(avigo_ad_data_r)
{
	avigo_state *state = space->machine().driver_data<avigo_state>();
	unsigned char data;

	data = 0;

	/* original */

	/* status AND   11110111 */
	/* status OR    01110000 -> C20F */

	switch (state->m_ad_control_status & 0x078)
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
			data = ((state->m_ad_value>>6) & 0x0f)<<4;
			data |= 8;
		}
		break;

		/* x0111xxx */
		case 0x038:
		{
			/* lower 6 bits of 10-bit A/D number in bits 7-2 of data */
			/* bit 0 must be 1, bit 1 must be 0 */

			logerror("a/d lower 6-bits\n");
			data = ((state->m_ad_value & 0x03f)<<2);
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
	avigo_state *state = space->machine().driver_data<avigo_state>();
	device_t *speaker = space->machine().device("speaker");
	UINT8 previous_speaker;

	previous_speaker = state->m_speaker_data;
	state->m_speaker_data = data;

	/* changed state? */
	if (((data^state->m_speaker_data) & (1<<3))!=0)
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



static ADDRESS_MAP_START( avigo_io, AS_IO, 8)
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

/* F4 Character Displayer */
static const gfx_layout avigo_charlayout =
{
	8, 16,					/* 8 x 16 characters */
	96,					/* 96 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16					/* every char takes 16 bytes */
};

static const gfx_layout avigo_8_by_14 =
{
	8, 14,					/* 8 x 16 characters */
	1024,					/* 1024 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*32					/* every char takes 32 bytes */
};

static const gfx_layout avigo_16_by_15 =
{
	16, 15,					/* 8 x 16 characters */
	1024,					/* 1024 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	/* y offsets */
	{ 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16, 8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*16					/* every char takes 16 bytes */
};

static const gfx_layout avigo_15_by_16 =
{
	15, 16,					/* 8 x 16 characters */
	1024,					/* 1024 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16, 8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	/* y offsets */
	{ 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 },
	16*16					/* every char takes 16 bytes */
};

static const gfx_layout avigo_8_by_8 =
{
	8, 8,					/* 8 x 8 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	16*16					/* every char takes 16 bytes */
};

static const gfx_layout avigo_6_by_8 =
{
	6, 8,					/* 6 x 8 characters */
	255,					/* 255 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	/* y offsets */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	16*16					/* every char takes 16 bytes */
};

static GFXDECODE_START( avigo )
	GFXDECODE_ENTRY( "maincpu", 0x00000, avigo_charlayout, 0, 3 )
	GFXDECODE_ENTRY( "maincpu", 0x18992, avigo_charlayout, 0, 1 )
	GFXDECODE_ENTRY( "maincpu", 0x1c020, avigo_8_by_14, 0, 1 )
	GFXDECODE_ENTRY( "maincpu", 0x1c020, avigo_16_by_15, 0, 1 )
	GFXDECODE_ENTRY( "maincpu", 0x24020, avigo_15_by_16, 0, 1 )
	GFXDECODE_ENTRY( "maincpu", 0x2c020, avigo_8_by_8, 0, 1 )
	GFXDECODE_ENTRY( "maincpu", 0x2e020, avigo_6_by_8, 0, 1 )
GFXDECODE_END


static MACHINE_CONFIG_START( avigo, avigo_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, 4000000)
	MCFG_CPU_PROGRAM_MAP(avigo_mem)
	MCFG_CPU_IO_MAP(avigo_io)
	MCFG_QUANTUM_TIME(attotime::from_hz(60))

	MCFG_MACHINE_START( avigo )
	MCFG_MACHINE_RESET( avigo )

	MCFG_NS16550_ADD( "ns16550", avigo_com_interface )

	/* video hardware */
	MCFG_SCREEN_ADD("screen", LCD)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_SCREEN_UPDATE( avigo )

	MCFG_GFXDECODE(avigo)
	MCFG_PALETTE_LENGTH(16)
	MCFG_PALETTE_INIT( avigo )

	MCFG_VIDEO_START( avigo )

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* real time clock */
	MCFG_TC8521_ADD("rtc", avigo_tc8521_interface)

	/* flash ROMs */
	MCFG_INTEL_E28F008SA_ADD("flash0")
	MCFG_INTEL_E28F008SA_ADD("flash1")
	MCFG_INTEL_E28F008SA_ADD("flash2")

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("128K")

	/* a timer used to check status of pen */
	/* an interrupt is generated when the pen is pressed to the screen */
	MCFG_TIMER_ADD_PERIODIC("avigo_timer", avigo_dummy_timer_callback, attotime::from_hz(50))
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

