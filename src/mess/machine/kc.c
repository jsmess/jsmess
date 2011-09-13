/* Core includes */
#include "emu.h"
#include "includes/kc.h"

/* Components */
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "cpu/z80/z80.h"
#include "sound/speaker.h"

/* Devices */
#include "imagedev/cassette.h"
#include "machine/ram.h"

#define KC_DEBUG 0
#define LOG(x) do { if (KC_DEBUG) logerror x; } while (0)


static void kc85_4_update_0x0c000(running_machine &machine);
static void kc85_4_update_0x0e000(running_machine &machine);
static void kc85_4_update_0x08000(running_machine &machine);

/* PIO PORT A: port 0x088:

bit 7: ROM C (BASIC)
bit 6: Tape Motor on
bit 5: LED
bit 4: K OUT
bit 3: WRITE PROTECT RAM 0
bit 2: IRM
bit 1: ACCESS RAM 0
bit 0: CAOS ROM E
*/

/* PIO PORT B: port 0x089:
bit 7: BLINK ENABLE (1=blink, 0=no blink)
bit 6: WRITE PROTECT RAM 8
bit 5: ACCESS RAM 8
bit 4: TONE 4
bit 3: TONE 3
bit 2: TONE 2
bit 1: TONE 1
bit 0: TRUCK */


/* load image */
static int kc_load(device_image_interface &image, unsigned char **ptr)
{
	int datasize;
	unsigned char *data;

	/* get file size */
	datasize = image.length();

	if (datasize!=0)
	{
		/* malloc memory for this data */
		data = (unsigned char *)auto_alloc_array(image.device().machine(), unsigned char, datasize);

		if (data!=NULL)
		{
			/* read whole file */
			image.fread( data, datasize);

			*ptr = data;

			logerror("File loaded!\r\n");

			/* ok! */
			return 1;
		}
	}

	return 0;
}

struct kcc_header
{
	unsigned char	name[10];
	unsigned char	reserved[6];
	unsigned char	anzahl;
	unsigned char	load_address_l;
	unsigned char	load_address_h;
	unsigned char	end_address_l;
	unsigned char	end_address_h;
	unsigned char	execution_address_l;
	unsigned char	execution_address_h;
	unsigned char	pad[128-2-2-2-1-16];
};

/* appears to work a bit.. */
/* load file, then type: MENU and it should now be displayed. */
/* now type name that has appeared! */

/* load snapshot */
QUICKLOAD_LOAD(kc)
{
	unsigned char *data;
	struct kcc_header *header;
	int addr;
	int datasize;
	int execution_address;
	int i;

	if (!kc_load(image, &data))
		return IMAGE_INIT_FAIL;

	header = (struct kcc_header *) data;
	addr = (header->load_address_l & 0x0ff) | ((header->load_address_h & 0x0ff)<<8);
	datasize = ((header->end_address_l & 0x0ff) | ((header->end_address_h & 0x0ff)<<8)) - addr;
	execution_address = (header->execution_address_l & 0x0ff) | ((header->execution_address_h & 0x0ff)<<8);

	if (datasize + 128 > image.length())
	{
		mame_printf_info("Invalid snapshot size: expected 0x%04x, found 0x%04x\n", datasize, (UINT32)image.length() - 128);
		datasize = image.length() - 128;
	}

	for (i=0; i<datasize; i++)
		ram_get_ptr(image.device().machine().device(RAM_TAG))[(addr+i) & 0x0ffff] = data[i+128];

	if (execution_address != 0 && header->anzahl >= 3 )
	{
		// if specified, jumps to the quickload start address
		cpu_set_reg(image.device().machine().device("maincpu"), STATE_GENPC, execution_address);
	}

	auto_free(image.device().machine(), data);

	logerror("Snapshot loaded at: 0x%04x-0x%04x, execution address: 0x%04x\n", addr, addr + datasize - 1, execution_address);

	return IMAGE_INIT_PASS;
}


/*****************/
/* MODULE SYSTEM */
/*****************/

/* The KC85/4 and KC85/3 are "modular systems". These computers can be expanded with modules.
*/

/*
    Module ID       Module Name         Module Description


                    D001                Basis Device
                    D002                Bus Driver device
    a7              D004                Floppy Disk Interface Device


    ef              M001                Digital IN/OUT
    ee              M003                V24
                    M005                User (empty)
                    M007                Adapter (empty)
    e7              M010                ADU1
    f6              M011                64k RAM
    fb              M012                Texor
    f4              M022                Expander RAM (16k)
    f7              M025                User PROM (8k)
    fb              M026                Forth
    fb              M027                Development
    e3              M029                DAU1
*/

static void kc85_module_system_init(running_machine &machine)
{
	kc_state *state = machine.driver_data<kc_state>();

	state->m_expansions[0] = machine.device<kcexp_slot_device>("m1");
	state->m_expansions[1] = machine.device<kcexp_slot_device>("m2");
	state->m_expansions[2] = machine.device<kcexp_slot_device>("exp");
}

static READ8_HANDLER ( kc_expansion_read )
{
	kc_state *state = space->machine().driver_data<kc_state>();
	UINT8 result = 0xff;

	// assert MEI line of first slot
	state->m_expansions[0]->mei_w(ASSERT_LINE);

	for (int i=0; i<3; i++)
		state->m_expansions[i]->read(offset, result);

	return result;
}

static WRITE8_HANDLER ( kc_expansion_write )
{
	kc_state *state = space->machine().driver_data<kc_state>();

	// assert MEI line of first slot
	state->m_expansions[0]->mei_w(ASSERT_LINE);

	for (int i=0; i<3; i++)
		state->m_expansions[i]->write(offset, data);
}

/*
    port xx80

    - xx is module id.

    Only addressess divisible by 4 are checked.
    If module does not exist, 0x0ff is returned.

    When xx80 is read, if a module exists a id will be returned.
    Id's for known modules are listed above.
*/

READ8_HANDLER ( kc_expansion_io_read )
{
	kc_state *state = space->machine().driver_data<kc_state>();
	UINT8 result = 0xff;

	// assert MEI line of first slot
	state->m_expansions[0]->mei_w(ASSERT_LINE);

	if ((offset & 0xff) == 0x80)
	{
		UINT8 slot_id = (offset>>8) & 0xff;

		if (slot_id == 0x08 || slot_id == 0x0c)
			result = state->m_expansions[(slot_id - 8) >> 2]->module_id_r();
		else
			state->m_expansions[2]->io_read(offset, result);
	}
	else
	{
		for (int i=0; i<3; i++)
			state->m_expansions[i]->io_read(offset, result);
	}

	return result;
}

WRITE8_HANDLER ( kc_expansion_io_write )
{
	kc_state *state = space->machine().driver_data<kc_state>();

	// assert MEI line of first slot
	state->m_expansions[0]->mei_w(ASSERT_LINE);

	if ((offset & 0xff) == 0x80)
	{
		UINT8 slot_id = (offset>>8) & 0xff;

		if (slot_id == 0x08 || slot_id == 0x0c)
			state->m_expansions[(slot_id - 8) >> 2]->control_w(data);
		else
			state->m_expansions[2]->io_write(offset, data);
	}
	else
	{
		for (int i=0; i<3; i++)
			state->m_expansions[i]->io_write(offset, data);
	}
}

// module read/write handlers
static READ8_HANDLER ( kc_expansion_4000_r ){ return kc_expansion_read(space, offset + 0x4000); }
static WRITE8_HANDLER( kc_expansion_4000_w ){ kc_expansion_write(space, offset + 0x4000, data); }
static READ8_HANDLER ( kc_expansion_8000_r ){ return kc_expansion_read(space, offset + 0x8000); }
static WRITE8_HANDLER( kc_expansion_8000_w ){ kc_expansion_write(space, offset + 0x8000, data); }
static READ8_HANDLER ( kc_expansion_c000_r ){ return kc_expansion_read(space, offset + 0xc000); }
static WRITE8_HANDLER( kc_expansion_c000_w ){ kc_expansion_write(space, offset + 0xc000, data); }
static READ8_HANDLER ( kc_expansion_e000_r ){ return kc_expansion_read(space, offset + 0xe000); }
static WRITE8_HANDLER( kc_expansion_e000_w ){ kc_expansion_write(space, offset + 0xe000, data); }


/**********************/
/* CASSETTE EMULATION */
/**********************/
/* used by KC85/4 and KC85/3 */
/*
    The cassette motor is controlled by bit 6 of PIO port A.
    The cassette read data is connected to /ASTB input of the PIO.
    A edge from the cassette therefore will trigger a interrupt
    from the PIO.
    The duration between two edges can be timed and the data-bit
    identified.

    I have used a timer to feed data into /ASTB. The timer is only
    active when the cassette motor is on.
*/


#define KC_CASSETTE_TIMER_FREQUENCY attotime::from_hz(4800)

/* this timer is used to update the cassette */
/* this is the current state of the cassette motor */
/* ardy output from pio */

static TIMER_CALLBACK(kc_cassette_timer_callback)
{
	kc_state *state = machine.driver_data<kc_state>();
	int bit;

	/* the cassette data is linked to /astb input of
    the pio. */

	bit = 0;

	/* get data from cassette */
	if ((machine.device<cassette_image_device>(CASSETTE_TAG))->input() > 0.0038)
		bit = 1;

	/* update astb with bit */
	z80pio_astb_w(state->m_kc85_z80pio,bit & state->m_ardy);
}

static void	kc_cassette_init(running_machine &machine)
{
	kc_state *state = machine.driver_data<kc_state>();
	state->m_cassette_timer = machine.scheduler().timer_alloc(FUNC(kc_cassette_timer_callback));
}

static void	kc_cassette_set_motor(running_machine &machine, int motor_state)
{
	kc_state *state = machine.driver_data<kc_state>();
	/* state changed? */
	if (((state->m_cassette_motor_state^motor_state)&0x01)!=0)
	{
		/* set new motor state in cassette device */
		machine.device<cassette_image_device>(CASSETTE_TAG)->change_state(
			motor_state ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED,
			CASSETTE_MASK_MOTOR);

		if (motor_state)
		{
			/* start timer */
			state->m_cassette_timer->adjust(attotime::zero, 0, KC_CASSETTE_TIMER_FREQUENCY);
		}
		else
		{
			/* stop timer */
			state->m_cassette_timer->reset();
		}
	}

	/* store new state */
	state->m_cassette_motor_state = motor_state;
}

/*
  pin 2 = gnd
  pin 3 = read
  pin 1 = k1        ?? modulating signal
  pin 4 = k0        ?? signal??
  pin 5 = motor on


    Tape signals:
        K0, K1      ??
        MOTON       motor control
        ASTB        read?

        T1-T4 give 4 bit a/d tone sound?
        K1, K0 are mixed with tone.

    Cassette read goes into ASTB of PIO.
    From this, KC must be able to detect the length
    of the pulses and can read the data.


    Tape write: clock comes from CTC?
    truck signal resets, 5v signal for set.
    output gives k0 and k1.




*/



/*********************************************************************/

/* port 0x084/0x085:

bit 7: RAF3
bit 6: RAF2
bit 5: RAF1
bit 4: RAF0
bit 3: FPIX. high resolution
bit 2: BLA1 .access screen
bit 1: BLA0 .pixel/color
bit 0: BILD .display screen 0 or 1
*/

/* port 0x086/0x087:

bit 7: ROCC
bit 6: ROF1
bit 5: ROF0
bit 4-2 are not connected
bit 1: WRITE PROTECT RAM 4
bit 0: ACCESS RAM 4
*/


/* KC85/4 specific */




/* port 0x086:

bit 7: CAOS ROM C
bit 6:
bit 5:
bit 4:
bit 3:
bit 2:
bit 1: write protect ram 4
bit 0: ram 4
*/

static void kc85_4_update_0x08000(running_machine &machine)
{
	kc_state *state = machine.driver_data<kc_state>();
	address_space *space = machine.device( "maincpu")->memory().space( AS_PROGRAM );
	unsigned char *ram_page;

    if (state->m_kc85_pio_data[1] & (1<<5))
    {
		int ram8_block;
		unsigned char *mem_ptr;

		/* ram8 block chosen */
		ram8_block = ((state->m_kc85_84_data)>>4) & 0x01;

		mem_ptr = ram_get_ptr(machine.device(RAM_TAG))+0x08000+(ram8_block<<14);

		memory_set_bankptr(machine, "bank3", mem_ptr);
		memory_set_bankptr(machine, "bank4", mem_ptr+0x02800);
		space->install_read_bank(0x8000, 0xa7ff, "bank3");
		space->install_read_bank(0xa800, 0xbfff, "bank4");

		/* write protect RAM8 ? */
		if ((state->m_kc85_pio_data[1] & (1<<6))==0)
		{
			/* ram8 is enabled and write protected */
			space->nop_write(0x8000, 0xa7ff);
			space->nop_write(0xa800, 0xbfff);
		}
		else
		{
			LOG(("RAM8 write enabled\n"));

			/* ram8 is enabled and write enabled */
			space->install_write_bank(0x8000, 0xa7ff, "bank9");
			space->install_write_bank(0xa800, 0xbfff, "bank10");
			memory_set_bankptr(machine, "bank9", mem_ptr);
			memory_set_bankptr(machine, "bank10", mem_ptr+0x02800);
		}
    }
    else
    {
		LOG(("Module at ram8\n"));

		space->install_legacy_read_handler (0x8000, 0xbfff, FUNC(kc_expansion_8000_r));
		space->install_legacy_write_handler(0x8000, 0xbfff, FUNC(kc_expansion_8000_w));
    }

	/* if IRM is enabled override block 3/9 settings */
	if (state->m_kc85_pio_data[0] & (1<<2))
	{
		/* IRM enabled - has priority over RAM8 enabled */
		ram_page = kc85_4_get_video_ram_base(machine, (state->m_kc85_84_data & 0x04), (state->m_kc85_84_data & 0x02));

		memory_set_bankptr(machine, "bank3", ram_page);
		memory_set_bankptr(machine, "bank9", ram_page);
		space->install_read_bank(0x8000, 0xa7ff, "bank3");
		space->install_write_bank(0x8000, 0xa7ff, "bank9");

		ram_page = kc85_4_get_video_ram_base(machine, 0, 0);

		memory_set_bankptr(machine, "bank4", ram_page + 0x2800);
		memory_set_bankptr(machine, "bank10", ram_page + 0x2800);
		space->install_read_bank(0xa800, 0xbfff, "bank4");
		space->install_write_bank(0xa800, 0xbfff, "bank10");
	}
}

/* update status of memory area 0x0000-0x03fff */
static void kc85_4_update_0x00000(running_machine &machine)
{
	kc_state *state = machine.driver_data<kc_state>();
	address_space *space = machine.device( "maincpu")->memory().space( AS_PROGRAM );

	/* access ram? */
	if (state->m_kc85_pio_data[0] & (1<<1))
	{
		LOG(("ram0 enabled\n"));

		/* yes; set address of bank */
		space->install_read_bank(0x0000, 0x3fff, "bank1");
		memory_set_bankptr(machine, "bank1", ram_get_ptr(machine.device(RAM_TAG)));

		/* write protect ram? */
		if ((state->m_kc85_pio_data[0] & (1<<3))==0)
		{
			/* yes */
			LOG(("ram0 write protected\n"));

			/* ram is enabled and write protected */
			space->unmap_write(0x0000, 0x3fff);
		}
		else
		{
			LOG(("ram0 write enabled\n"));

			/* ram is enabled and write enabled; and set address of bank */
			space->install_write_bank(0x0000, 0x3fff, "bank7");
			memory_set_bankptr(machine, "bank7", ram_get_ptr(machine.device(RAM_TAG)));
		}
	}
	else
	{
		LOG(("Module at ram0!\n"));

		space->install_legacy_read_handler (0x0000, 0x3fff, FUNC(kc_expansion_read));
		space->install_legacy_write_handler(0x0000, 0x3fff, FUNC(kc_expansion_write));
	}
}

/* update status of memory area 0x4000-0x07fff */
static void kc85_4_update_0x04000(running_machine &machine)
{
	kc_state *state = machine.driver_data<kc_state>();
	address_space *space = machine.device( "maincpu")->memory().space( AS_PROGRAM );

	/* access ram? */
	if (state->m_kc85_86_data & (1<<0))
	{
		UINT8 *mem_ptr;

		mem_ptr = ram_get_ptr(machine.device(RAM_TAG)) + 0x04000;

		/* yes */
		space->install_read_bank(0x4000, 0x7fff, "bank2");
		/* set address of bank */
		memory_set_bankptr(machine, "bank2", mem_ptr);

		/* write protect ram? */
		if ((state->m_kc85_86_data & (1<<1))==0)
		{
			/* yes */
			LOG(("ram4 write protected\n"));

			/* ram is enabled and write protected */
			space->nop_write(0x4000, 0x7fff);
		}
		else
		{
			LOG(("ram4 write enabled\n"));

			/* ram is enabled and write enabled */
			space->install_write_bank(0x4000, 0x7fff, "bank8");
			/* set address of bank */
			memory_set_bankptr(machine, "bank8", mem_ptr);
		}
	}
	else
	{
		LOG(("Module at ram4!\n"));

		space->install_legacy_read_handler (0x4000, 0x7fff, FUNC(kc_expansion_4000_r));
		space->install_legacy_write_handler(0x4000, 0x7fff, FUNC(kc_expansion_4000_w));
	}

}


/* update memory address 0x0c000-0x0e000 */
static void kc85_4_update_0x0c000(running_machine &machine)
{
	kc_state *state = machine.driver_data<kc_state>();
	address_space *space = machine.device( "maincpu")->memory().space( AS_PROGRAM );

	if (state->m_kc85_86_data & (1<<7))
	{
		/* CAOS rom takes priority */
		LOG(("CAOS rom 0x0c000\n"));

		memory_set_bankptr(machine, "bank5",machine.region("caos")->base());
		space->install_read_bank(0xc000, 0xdfff, "bank5");
		space->unmap_write(0xc000, 0xdfff);
	}
	else if (state->m_kc85_pio_data[0] & (1<<7))
	{
		/* BASIC takes next priority */
        	LOG(("BASIC rom 0x0c000\n"));

        memory_set_bankptr(machine, "bank5", machine.region("basic")->base());
		space->install_read_bank(0xc000, 0xdfff, "bank5");
		space->unmap_write(0xc000, 0xdfff);
	}
	else
	{
		LOG(("Module at 0x0c000\n"));

		space->install_legacy_read_handler (0xc000, 0xdfff, FUNC(kc_expansion_c000_r));
		space->install_legacy_write_handler(0xc000, 0xdfff, FUNC(kc_expansion_c000_w));
	}
}

/* update memory address 0x0e000-0x0ffff */
static void kc85_4_update_0x0e000(running_machine &machine)
{
	kc_state *state = machine.driver_data<kc_state>();
	address_space *space = machine.device( "maincpu")->memory().space( AS_PROGRAM );
	if (state->m_kc85_pio_data[0] & (1<<0))
	{
		/* enable CAOS rom in memory range 0x0e000-0x0ffff */
		LOG(("CAOS rom 0x0e000\n"));
		/* read will access the rom */
		memory_set_bankptr(machine, "bank6",machine.region("caos")->base() + 0x2000);
		space->install_read_bank(0xe000, 0xffff,"bank6");
		space->unmap_write(0xe000, 0xffff);
	}
	else
	{
		LOG(("Module at 0x0e000\n"));

		space->install_legacy_read_handler (0xe000, 0xffff, FUNC(kc_expansion_e000_r));
		space->install_legacy_write_handler(0xe000, 0xffff, FUNC(kc_expansion_e000_w));
	}
}

/* PIO PORT A: port 0x088:

bit 7: ROM C (BASIC)
bit 6: Tape Motor on
bit 5: LED
bit 4: K OUT
bit 3: WRITE PROTECT RAM 0
bit 2: IRM
bit 1: ACCESS RAM 0
bit 0: CAOS ROM E
*/

static READ8_DEVICE_HANDLER ( kc85_4_pio_porta_r )
{
	kc_state *state = device->machine().driver_data<kc_state>();
	return state->m_kc85_pio_data[0];
}

static WRITE8_DEVICE_HANDLER ( kc85_4_pio_porta_w )
{
	kc_state *state = device->machine().driver_data<kc_state>();
	state->m_kc85_pio_data[0] = data;

	kc85_4_update_0x0c000(device->machine());
	kc85_4_update_0x0e000(device->machine());
	kc85_4_update_0x08000(device->machine());
	kc85_4_update_0x00000(device->machine());

	kc_cassette_set_motor(device->machine(), (data>>6) & 0x01);
}

/* PIO PORT B: port 0x089:
bit 7: BLINK
bit 6: WRITE PROTECT RAM 8
bit 5: ACCESS RAM 8
bit 4: TONE 4
bit 3: TONE 3
bit 2: TONE 2
bit 1: TONE 1
bit 0: TRUCK */

static READ8_DEVICE_HANDLER ( kc85_4_pio_portb_r )
{
	kc_state *state = device->machine().driver_data<kc_state>();
	return state->m_kc85_pio_data[1];
}

static WRITE8_DEVICE_HANDLER ( kc85_4_pio_portb_w )
{
	kc_state *state = device->machine().driver_data<kc_state>();
	device_t *speaker = device->machine().device(SPEAKER_TAG);
	state->m_kc85_pio_data[1] = data;

	int speaker_level;

	kc85_4_update_0x08000(device->machine());

	/* 16 speaker levels */
	speaker_level = (data>>1) & 0x0f;

	/* this might not be correct, the range might
    be logarithmic and not linear! */
	speaker_level_w(speaker, (speaker_level<<4));
}


WRITE8_HANDLER ( kc85_4_86_w )
{
	kc_state *state = space->machine().driver_data<kc_state>();
	LOG(("0x086 W: %02x\n",data));

	state->m_kc85_86_data = data;

	kc85_4_update_0x0c000(space->machine());
	kc85_4_update_0x04000(space->machine());
}

 READ8_HANDLER ( kc85_4_86_r )
{
	kc_state *state = space->machine().driver_data<kc_state>();
	return state->m_kc85_86_data;
}


WRITE8_HANDLER ( kc85_4_84_w )
{
	kc_state *state = space->machine().driver_data<kc_state>();
	LOG(("0x084 W: %02x\n",data));

	state->m_kc85_84_data = data;

	kc85_4_video_ram_select_bank(space->machine(), data & 0x01);

	kc85_4_update_0x08000(space->machine());
}

 READ8_HANDLER ( kc85_4_84_r )
{
	kc_state *state = space->machine().driver_data<kc_state>();
	return state->m_kc85_84_data;
}
/*****************************************************************/

/* update memory region 0x0c000-0x0e000 */
static void kc85_3_update_0x0c000(running_machine &machine)
{
	kc_state *state = machine.driver_data<kc_state>();
	address_space *space = machine.device( "maincpu")->memory().space( AS_PROGRAM );

	if (state->m_kc85_pio_data[0] & (1<<7) && machine.region("basic")->base() != NULL)
	{
		/* BASIC takes next priority */
		LOG(("BASIC rom 0x0c000\n"));

		memory_set_bankptr(machine, "bank4", machine.region("basic")->base());
		space->install_read_bank(0xc000, 0xdfff, "bank4");
		space->unmap_write(0xc000, 0xdfff);
	}
	else
	{
		LOG(("Module at 0x0c000\n"));

		space->install_legacy_read_handler (0xc000, 0xdfff, FUNC(kc_expansion_c000_r));
		space->install_legacy_write_handler(0xc000, 0xdfff, FUNC(kc_expansion_c000_w));
	}
}

/* update memory address 0x0e000-0x0ffff */
static void kc85_3_update_0x0e000(running_machine &machine)
{
	kc_state *state = machine.driver_data<kc_state>();
	address_space *space = machine.device( "maincpu")->memory().space( AS_PROGRAM );

	if (state->m_kc85_pio_data[0] & (1<<0))
	{
		/* enable CAOS rom in memory range 0x0e000-0x0ffff */
		LOG(("CAOS rom 0x0e000\n"));

		memory_set_bankptr(machine, "bank5",machine.region("caos")->base());
		space->install_read_bank(0xe000, 0xffff, "bank5");
		space->unmap_write(0xe000, 0xffff);
	}
	else
	{
		LOG(("Module at 0x0e000\n"));

		space->install_legacy_read_handler (0xe000, 0xffff, FUNC(kc_expansion_e000_r));
		space->install_legacy_write_handler(0xe000, 0xffff, FUNC(kc_expansion_e000_w));
	}
}

/* update status of memory area 0x0000-0x03fff */
/* SMH_BANK(1) is used for read operations and SMH_BANK(6) is used
for write operations */
static void kc85_3_update_0x00000(running_machine &machine)
{
	kc_state *state = machine.driver_data<kc_state>();
	address_space *space = machine.device( "maincpu")->memory().space( AS_PROGRAM );

	/* access ram? */
	if (state->m_kc85_pio_data[0] & (1<<1))
	{
		LOG(("ram0 enabled\n"));

		/* yes */
		space->install_read_bank(0x0000, 0x3fff, "bank1");
		/* set address of bank */
		memory_set_bankptr(machine, "bank1", ram_get_ptr(machine.device(RAM_TAG)));

		/* write protect ram? */
		if ((state->m_kc85_pio_data[0] & (1<<3))==0)
		{
			/* yes */
			LOG(("ram0 write protected\n"));

			/* ram is enabled and write protected */
			space->nop_write(0x0000, 0x3fff);
		}
		else
		{
			LOG(("ram0 write enabled\n"));

			/* ram is enabled and write enabled */
			space->install_write_bank(0x0000, 0x3fff, "bank6");
			/* set address of bank */
			memory_set_bankptr(machine, "bank6", ram_get_ptr(machine.device(RAM_TAG)));
		}
	}
	else
	{
		LOG(("Module at ram0!\n"));

		space->install_legacy_read_handler (0x0000, 0x3fff, FUNC(kc_expansion_read));
		space->install_legacy_write_handler(0x0000, 0x3fff, FUNC(kc_expansion_write));
	}
}

/* update status of memory area 0x08000-0x0ffff */
/* SMH_BANK(3) is used for read, SMH_BANK(8) is used for write */
static void kc85_3_update_0x08000(running_machine &machine)
{
	kc_state *state = machine.driver_data<kc_state>();
	address_space *space = machine.device( "maincpu")->memory().space( AS_PROGRAM );
	unsigned char *ram_page;

    if (state->m_kc85_pio_data[0] & (1<<2))
    {
        /* IRM enabled */
        LOG(("IRM enabled\n"));
		ram_page = ram_get_ptr(machine.device(RAM_TAG))+0x08000;

		memory_set_bankptr(machine, "bank3", ram_page);
		memory_set_bankptr(machine, "bank8", ram_page);
		space->install_read_bank(0x8000, 0xbfff, "bank3");
		space->install_write_bank(0x8000, 0xbfff, "bank8");
    }
    else if (state->m_kc85_pio_data[1] & (1<<5))
    {
		/* RAM8 ACCESS */
		LOG(("RAM8 enabled\n"));
		ram_page = ram_get_ptr(machine.device(RAM_TAG)) + 0x04000;

		memory_set_bankptr(machine, "bank3", ram_page);
		space->install_read_bank(0x8000, 0xbfff, "bank3");

		/* write protect RAM8 ? */
		if ((state->m_kc85_pio_data[1] & (1<<6))==0)
		{
			LOG(("RAM8 write protected\n"));
			/* ram8 is enabled and write protected */
			space->nop_write(0x8000, 0xbfff);
		}
		else
		{
			LOG(("RAM8 write enabled\n"));
			/* ram8 is enabled and write enabled */
			space->install_write_bank(0x8000, 0xbfff, "bank8");
			memory_set_bankptr(machine, "bank8",ram_page);
		}
    }
    else
    {
		LOG(("Module at ram8!\n"));

		space->install_legacy_read_handler (0x8000, 0xbfff, FUNC(kc_expansion_8000_r));
		space->install_legacy_write_handler(0x8000, 0xbfff, FUNC(kc_expansion_8000_w));
    }
}


/* PIO PORT A: port 0x088:

bit 7: ROM C (BASIC)
bit 6: Tape Motor on
bit 5: LED
bit 4: K OUT
bit 3: WRITE PROTECT RAM 0
bit 2: IRM
bit 1: ACCESS RAM 0
bit 0: CAOS ROM E
*/

static READ8_DEVICE_HANDLER ( kc85_3_pio_porta_r )
{
	kc_state *state = device->machine().driver_data<kc_state>();

	return state->m_kc85_pio_data[0];
}

static WRITE8_DEVICE_HANDLER ( kc85_3_pio_porta_w )
{
	kc_state *state = device->machine().driver_data<kc_state>();
	state->m_kc85_pio_data[0] = data;

	kc85_3_update_0x0c000(device->machine());
	kc85_3_update_0x0e000(device->machine());
	kc85_3_update_0x00000(device->machine());

	kc_cassette_set_motor(device->machine(), (data>>6) & 0x01);
}


/* PIO PORT B: port 0x089:
bit 7: BLINK ENABLE
bit 6: WRITE PROTECT RAM 8
bit 5: ACCESS RAM 8
bit 4: TONE 4
bit 3: TONE 3
bit 2: TONE 2
bit 1: TONE 1
bit 0: TRUCK */

static READ8_DEVICE_HANDLER ( kc85_3_pio_portb_r )
{
	kc_state *state = device->machine().driver_data<kc_state>();

	return state->m_kc85_pio_data[1];
}

static WRITE8_DEVICE_HANDLER ( kc85_3_pio_portb_w )
{
	kc_state *state = device->machine().driver_data<kc_state>();
	device_t *speaker = device->machine().device(SPEAKER_TAG);
	state->m_kc85_pio_data[1] = data;

	int speaker_level;

	kc85_3_update_0x08000(device->machine());

	/* 16 speaker levels */
	speaker_level = (data>>1) & 0x0f;

	/* this might not be correct, the range might
    be logarithmic and not linear! */
	speaker_level_w(speaker, (speaker_level<<4));
}


/*****************************************************************/

#if 0
DIRECT_UPDATE_HANDLER( kc85_3_opbaseoverride )
{
	memory_set_direct_update_handler(0,0);

	kc85_3_update_0x00000(machine);

	return (cpunum_get_reg(0, STATE_GENPC) & 0x0ffff);
}


DIRECT_UPDATE_HANDLER( kc85_4_opbaseoverride )
{
	memory_set_direct_update_handler(0,0);

	kc85_4_update_0x00000(machine);

	return (cpunum_get_reg(0, STATE_GENPC) & 0x0ffff);
}
#endif


static TIMER_CALLBACK(kc85_reset_timer_callback)
{
	cpu_set_reg(machine.device("maincpu"), STATE_GENPC, 0x0f000);
}


/* callback for ardy output from PIO */
/* used in KC85/4 & KC85/3 cassette interface */
static void kc85_pio_ardy_callback(device_t *device, int state)
{
	kc_state *drvstate = device->machine().driver_data<kc_state>();
	drvstate->m_ardy = state & 0x01;

	if (state)
	{
		LOG(("PIO A Ready\n"));
	}
}

/* callback for brdy output from PIO */
/* used in KC85/4 & KC85/3 keyboard interface */
static void kc85_pio_brdy_callback(device_t *device, int state)
{
	kc_state *drvstate = device->machine().driver_data<kc_state>();
	drvstate->m_brdy = state & 0x01;

	if (state)
	{
		LOG(("PIO B Ready\n"));
	}
}

Z80PIO_INTERFACE( kc85_2_pio_intf )
{
	DEVCB_CPU_INPUT_LINE("maincpu", 0),						/* callback when change interrupt status */
	DEVCB_HANDLER(kc85_3_pio_porta_r),						/* port A read callback */
	DEVCB_HANDLER(kc85_3_pio_porta_w),						/* port A write callback */
	DEVCB_LINE(kc85_pio_ardy_callback),						/* portA ready active callback */
	DEVCB_HANDLER(kc85_3_pio_portb_r),						/* port B read callback */
	DEVCB_HANDLER(kc85_3_pio_portb_w),						/* port B write callback */
	DEVCB_LINE(kc85_pio_brdy_callback)						/* portB ready active callback */
};

Z80PIO_INTERFACE( kc85_4_pio_intf )
{
	DEVCB_CPU_INPUT_LINE("maincpu", 0),						/* callback when change interrupt status */
	DEVCB_HANDLER(kc85_4_pio_porta_r),						/* port A read callback */
	DEVCB_HANDLER(kc85_4_pio_porta_w),						/* port A write callback */
	DEVCB_LINE(kc85_pio_ardy_callback),						/* portA ready active callback */
	DEVCB_HANDLER(kc85_4_pio_portb_r),						/* port B read callback */
	DEVCB_HANDLER(kc85_4_pio_portb_w),						/* port B write callback */
	DEVCB_LINE(kc85_pio_brdy_callback)						/* portB ready active callback */
};

/* used in cassette write -> K0 */
static WRITE_LINE_DEVICE_HANDLER(kc85_zc0_callback)
{


}

/* used in cassette write -> K1 */
static WRITE_LINE_DEVICE_HANDLER(kc85_zc1_callback)
{

}


static TIMER_CALLBACK(kc85_15khz_timer_callback)
{
	device_t *device = (device_t *)ptr;
	kc_state *state = device->machine().driver_data<kc_state>();

	/* toggle state of square wave */
	state->m_kc85_15khz_state^=1;

	/* set clock input for channel 2 and 3 to ctc */
	z80ctc_trg0_w(device, state->m_kc85_15khz_state);
	z80ctc_trg1_w(device, state->m_kc85_15khz_state);

	state->m_kc85_15khz_count++;

	if (state->m_kc85_15khz_count>=312)
	{
		state->m_kc85_15khz_count = 0;

		/* toggle state of square wave */
		state->m_kc85_50hz_state^=1;

		/* set clock input for channel 2 and 3 to ctc */
		z80ctc_trg2_w(device, state->m_kc85_50hz_state);
		z80ctc_trg3_w(device, state->m_kc85_50hz_state);
	}
}

/* video blink */
static WRITE_LINE_DEVICE_HANDLER( kc85_zc2_callback )
{
	kc_state *drvstate = device->machine().driver_data<kc_state>();
	/* is blink enabled? */
	if (drvstate->m_kc85_pio_data[1] & (1<<7))
	{
		/* yes */
		/* toggle state of blink to video hardware */
		kc85_video_set_blink_state(device->machine(), state);
	}
}

Z80CTC_INTERFACE( kc85_ctc_intf )
{
	0,
    DEVCB_CPU_INPUT_LINE("maincpu", 0),
	DEVCB_LINE(kc85_zc0_callback),
	DEVCB_LINE(kc85_zc1_callback),
    DEVCB_LINE(kc85_zc2_callback)
};


/* keyboard callback */
WRITE_LINE_DEVICE_HANDLER(kc85_keyboard_cb)
{
	kc_state *drv_state = device->machine().driver_data<kc_state>();
	z80pio_bstb_w(device->machine().device("z80pio"), state & drv_state->m_brdy);

	/* FIXME: understand why the PIO fail to acknowledge the irq on kc85_2/3 */
	z80pio_d_w(device->machine().device("z80pio"), 1, drv_state->m_kc85_pio_data[1]);
}


const kc_keyb_interface kc85_keyboard_interface =
{
	DEVCB_LINE(kc85_keyboard_cb)
};

MACHINE_START(kc85)
{
	kc_cassette_init(machine);

	machine.scheduler().timer_pulse(attotime::from_hz(15625), FUNC(kc85_15khz_timer_callback), 0, (void *)machine.device("z80ctc"));
	machine.scheduler().timer_set(attotime::zero, FUNC(kc85_reset_timer_callback));
}

static void	kc85_common_init(running_machine &machine)
{
	kc_state *state = machine.driver_data<kc_state>();

	/* kc85 has a 50 Hz input to the ctc channel 2 and 3 */
	/* channel 2 this controls the video colour flash */
	/* kc85 has a 15 kHz (?) input to the ctc channel 0 and 1 */
	/* channel 0 and channel 1 are used for cassette write */
	state->m_kc85_50hz_state = 0;
	state->m_kc85_15khz_state = 0;
	state->m_kc85_15khz_count = 0;
	kc85_module_system_init(machine);
}

/*****************************************************************/

MACHINE_RESET( kc85_4 )
{
	kc_state *state = machine.driver_data<kc_state>();
	state->m_kc85_84_data = 0x0828;
	state->m_kc85_86_data = 0x063;
	/* enable CAOS rom in range 0x0e000-0x0ffff */
	/* ram0 enable, irm enable */
	state->m_kc85_pio_data[0] = 0x0f;
	state->m_kc85_pio_data[1] = 0x0f1;

	state->m_kc85_z80pio = machine.device("z80pio");

	kc85_4_update_0x04000(machine);
	kc85_4_update_0x08000(machine);
	kc85_4_update_0x0c000(machine);
	kc85_4_update_0x0e000(machine);

	kc85_common_init(machine);

	kc85_4_update_0x00000(machine);


#if 0
	/* this is temporary. Normally when a Z80 is reset, it will
    execute address 0. It appears the KC85 series pages the rom
    at address 0x0000-0x01000 which has a single jump in it,
    can't see yet where it disables it later!!!! so for now
    here will be a override */
	memory_set_direct_update_handler(0, kc85_4_opbaseoverride);
#endif
}

MACHINE_RESET( kc85_3 )
{
	kc_state *state = machine.driver_data<kc_state>();
	state->m_kc85_pio_data[0] = 0x0f;
	state->m_kc85_pio_data[1] = 0x0f1;

	memory_set_bankptr(machine, "bank2",ram_get_ptr(machine.device(RAM_TAG))+0x0c000);
	memory_set_bankptr(machine, "bank7",ram_get_ptr(machine.device(RAM_TAG))+0x0c000);

	state->m_kc85_z80pio = machine.device("z80pio");

	kc85_3_update_0x08000(machine);
	kc85_3_update_0x0c000(machine);
	kc85_3_update_0x0e000(machine);

	kc85_common_init(machine);

	kc85_3_update_0x00000(machine);

#if 0
	/* this is temporary. Normally when a Z80 is reset, it will
    execute address 0. It appears the KC85 series pages the rom
    at address 0x0000-0x01000 which has a single jump in it,
    can't see yet where it disables it later!!!! so for now
    here will be a override */
	memory_set_direct_update_handler(0, kc85_3_opbaseoverride);
#endif
}
