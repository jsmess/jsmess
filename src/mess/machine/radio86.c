/***************************************************************************

        Radio-86RK machine driver by Miodrag Milanovic

        06/03/2008 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "imagedev/cassette.h"
#include "machine/i8255a.h"
#include "machine/i8257.h"
#include "video/i8275.h"
#include "includes/radio86.h"



void radio86_init_keyboard(running_machine &machine)
{
	radio86_state *state = machine.driver_data<radio86_state>();
	state->m_keyboard_mask = 0;
	state->m_tape_value = 0x10;
}

/* Driver initialization */
DRIVER_INIT(radio86)
{
	/* set initialy ROM to be visible on first bank */
	UINT8 *RAM = machine.region("maincpu")->base();
	memset(RAM,0x0000,0x1000); // make frist page empty by default
	memory_configure_bank(machine, "bank1", 1, 2, RAM, 0x0000);
	memory_configure_bank(machine, "bank1", 0, 2, RAM, 0xf800);
	radio86_init_keyboard(machine);
}

DRIVER_INIT(radioram)
{
	radio86_state *state = machine.driver_data<radio86_state>();
	DRIVER_INIT_CALL(radio86);
	state->m_radio_ram_disk = auto_alloc_array(machine, UINT8, 0x20000);
	memset(state->m_radio_ram_disk,0,0x20000);
}
static READ8_DEVICE_HANDLER (radio86_8255_portb_r2 )
{
	radio86_state *state = device->machine().driver_data<radio86_state>();
	UINT8 key = 0xff;
	if ((state->m_keyboard_mask & 0x01)!=0) { key &= input_port_read(device->machine(),"LINE0"); }
	if ((state->m_keyboard_mask & 0x02)!=0) { key &= input_port_read(device->machine(),"LINE1"); }
	if ((state->m_keyboard_mask & 0x04)!=0) { key &= input_port_read(device->machine(),"LINE2"); }
	if ((state->m_keyboard_mask & 0x08)!=0) { key &= input_port_read(device->machine(),"LINE3"); }
	if ((state->m_keyboard_mask & 0x10)!=0) { key &= input_port_read(device->machine(),"LINE4"); }
	if ((state->m_keyboard_mask & 0x20)!=0) { key &= input_port_read(device->machine(),"LINE5"); }
	if ((state->m_keyboard_mask & 0x40)!=0) { key &= input_port_read(device->machine(),"LINE6"); }
	if ((state->m_keyboard_mask & 0x80)!=0) { key &= input_port_read(device->machine(),"LINE7"); }
	return key;
}

static READ8_DEVICE_HANDLER (radio86_8255_portc_r2 )
{
	radio86_state *state = device->machine().driver_data<radio86_state>();
	double level = cassette_input(device->machine().device("cassette"));
	UINT8 dat = input_port_read(device->machine(), "LINE8");
	if (level <  0) {
		dat ^= state->m_tape_value;
	}
	return dat;
}

static WRITE8_DEVICE_HANDLER (radio86_8255_porta_w2 )
{
	radio86_state *state = device->machine().driver_data<radio86_state>();
	state->m_keyboard_mask = data ^ 0xff;
}

static WRITE8_DEVICE_HANDLER (radio86_8255_portc_w2 )
{
	cassette_output(device->machine().device("cassette"),data & 0x01 ? 1 : -1);
}


I8255A_INTERFACE( radio86_ppi8255_interface_1 )
{
	DEVCB_NULL,
	DEVCB_HANDLER(radio86_8255_portb_r2),
	DEVCB_HANDLER(radio86_8255_portc_r2),
	DEVCB_HANDLER(radio86_8255_porta_w2),
	DEVCB_NULL,
	DEVCB_HANDLER(radio86_8255_portc_w2),
};

I8255A_INTERFACE( mikrosha_ppi8255_interface_1 )
{
	DEVCB_HANDLER(radio86_8255_portb_r2),
	DEVCB_NULL,
	DEVCB_HANDLER(radio86_8255_portc_r2),
	DEVCB_NULL,
	DEVCB_HANDLER(radio86_8255_porta_w2),
	DEVCB_NULL,
};



static READ8_DEVICE_HANDLER (rk7007_8255_portc_r )
{
	radio86_state *state = device->machine().driver_data<radio86_state>();
	double level = cassette_input(device->machine().device("cassette"));
	UINT8 key = 0xff;
	if ((state->m_keyboard_mask & 0x01)!=0) { key &= input_port_read(device->machine(),"CLINE0"); }
	if ((state->m_keyboard_mask & 0x02)!=0) { key &= input_port_read(device->machine(),"CLINE1"); }
	if ((state->m_keyboard_mask & 0x04)!=0) { key &= input_port_read(device->machine(),"CLINE2"); }
	if ((state->m_keyboard_mask & 0x08)!=0) { key &= input_port_read(device->machine(),"CLINE3"); }
	if ((state->m_keyboard_mask & 0x10)!=0) { key &= input_port_read(device->machine(),"CLINE4"); }
	if ((state->m_keyboard_mask & 0x20)!=0) { key &= input_port_read(device->machine(),"CLINE5"); }
	if ((state->m_keyboard_mask & 0x40)!=0) { key &= input_port_read(device->machine(),"CLINE6"); }
	if ((state->m_keyboard_mask & 0x80)!=0) { key &= input_port_read(device->machine(),"CLINE7"); }
	key &= 0xe0;
	if (level <  0) {
		key ^= state->m_tape_value;
	}
	return key;
}

I8255A_INTERFACE( rk7007_ppi8255_interface )
{
	DEVCB_NULL,
	DEVCB_HANDLER(radio86_8255_portb_r2),
	DEVCB_HANDLER(rk7007_8255_portc_r),
	DEVCB_HANDLER(radio86_8255_porta_w2),
	DEVCB_NULL,
	DEVCB_HANDLER(radio86_8255_portc_w2),
};

static WRITE_LINE_DEVICE_HANDLER( hrq_w )
{
	/* HACK - this should be connected to the BUSREQ line of Z80 */
	cputag_set_input_line(device->machine(), "maincpu", INPUT_LINE_HALT, state);

	/* HACK - this should be connected to the BUSACK line of Z80 */
	i8257_hlda_w(device, state);
}

static UINT8 memory_read_byte(address_space *space, offs_t address) { return space->read_byte(address); }
static void memory_write_byte(address_space *space, offs_t address, UINT8 data) { space->write_byte(address, data); }

I8257_INTERFACE( radio86_dma )
{
	DEVCB_LINE(hrq_w),
	DEVCB_NULL,
	DEVCB_NULL,
	I8257_MEMORY_HANDLER("maincpu", PROGRAM, memory_read_byte),
	I8257_MEMORY_HANDLER("maincpu", PROGRAM, memory_write_byte),
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_DEVICE_HANDLER("i8275", i8275_dack_w), DEVCB_NULL },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL }
};

static TIMER_CALLBACK( radio86_reset )
{
	memory_set_bank(machine, "bank1", 0);
}


READ8_HANDLER (radio_cpu_state_r )
{
	return cpu_get_reg(&space->device(), I8085_STATUS);
}

READ8_HANDLER (radio_io_r )
{
	return space->machine().device("maincpu")->memory().space(AS_PROGRAM)->read_byte((offset << 8) + offset);
}

WRITE8_HANDLER(radio_io_w )
{
	space->machine().device("maincpu")->memory().space(AS_PROGRAM)->write_byte((offset << 8) + offset,data);
}

MACHINE_RESET( radio86 )
{
	radio86_state *state = machine.driver_data<radio86_state>();
	machine.scheduler().timer_set(attotime::from_usec(10), FUNC(radio86_reset));
	memory_set_bank(machine, "bank1", 1);

	state->m_keyboard_mask = 0;
	state->m_disk_sel = 0;
}


WRITE8_HANDLER ( radio86_pagesel )
{
	radio86_state *state = space->machine().driver_data<radio86_state>();
	state->m_disk_sel = data;
}

static READ8_DEVICE_HANDLER (radio86_romdisk_porta_r )
{
	radio86_state *state = device->machine().driver_data<radio86_state>();
	UINT8 *romdisk = device->machine().region("maincpu")->base() + 0x10000;
	if ((state->m_disk_sel & 0x0f) ==0) {
		return romdisk[state->m_romdisk_msb*256+state->m_romdisk_lsb];
	} else {
		if (state->m_disk_sel==0xdf) {
			return state->m_radio_ram_disk[state->m_romdisk_msb*256+state->m_romdisk_lsb + 0x10000];
		} else {
			return state->m_radio_ram_disk[state->m_romdisk_msb*256+state->m_romdisk_lsb];
		}
	}
}

static WRITE8_DEVICE_HANDLER (radio86_romdisk_portb_w )
{
	radio86_state *state = device->machine().driver_data<radio86_state>();
	state->m_romdisk_lsb = data;
}

static WRITE8_DEVICE_HANDLER (radio86_romdisk_portc_w )
{
	radio86_state *state = device->machine().driver_data<radio86_state>();
	state->m_romdisk_msb = data;
}

I8255A_INTERFACE( radio86_ppi8255_interface_2 )
{
	DEVCB_HANDLER(radio86_romdisk_porta_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(radio86_romdisk_portb_w),
	DEVCB_HANDLER(radio86_romdisk_portc_w)
};

static WRITE8_DEVICE_HANDLER (mikrosha_8255_font_page_w )
{
	radio86_state *state = device->machine().driver_data<radio86_state>();
	state->m_mikrosha_font_page = (data  > 7) & 1;
}

I8255A_INTERFACE( mikrosha_ppi8255_interface_2 )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(mikrosha_8255_font_page_w),
	DEVCB_NULL
};

const i8275_interface radio86_i8275_interface = {
	"screen",
	6,
	0,
	DEVCB_DEVICE_LINE("dma8257", i8257_drq2_w),
	DEVCB_NULL,
	radio86_display_pixels
};

const i8275_interface mikrosha_i8275_interface = {
	"screen",
	6,
	0,
	DEVCB_DEVICE_LINE("dma8257", i8257_drq2_w),
	DEVCB_NULL,
	mikrosha_display_pixels
};

const i8275_interface apogee_i8275_interface = {
	"screen",
	6,
	0,
	DEVCB_DEVICE_LINE("dma8257", i8257_drq2_w),
	DEVCB_NULL,
	apogee_display_pixels
};

const i8275_interface partner_i8275_interface = {
	"screen",
	6,
	1,
	DEVCB_DEVICE_LINE("dma8257", i8257_drq2_w),
	DEVCB_NULL,
	partner_display_pixels
};
