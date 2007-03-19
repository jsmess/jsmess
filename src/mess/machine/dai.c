/***************************************************************************

  machine/dai.c

  Functions to emulate general aspects of DAI (RAM, ROM, interrupts, I/O ports)

  Krzysztof Strzecha
  Nathan Woods

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "devices/cassette.h"
#include "cpu/i8085/i8085.h"
#include "includes/dai.h"
#include "machine/pit8253.h"
#include "machine/8255ppi.h"
#include "machine/tms5501.h"

#define DEBUG_DAI_PORTS	0

#if DEBUG_DAI_PORTS
	#define LOG_DAI_PORT_R(_port, _data, _comment) logerror ("DAI port read : %04x, Data: %02x (%s)\n", _port, _data, _comment)
	#define LOG_DAI_PORT_W(_port, _data, _comment) logerror ("DAI port write: %04x, Data: %02x (%s)\n", _port, _data, _comment)
#else
	#define LOG_DAI_PORT_R(_port, _data, _comment)
	#define LOG_DAI_PORT_W(_port, _data, _comment)
#endif

/* Discrete I/O devices */
UINT8 dai_noise_volume;
UINT8 dai_osc_volume[3];
static UINT8 dai_paddle_select;
static UINT8 dai_paddle_enable;
static UINT8 dai_cassette_motor[2];


static OPBASE_HANDLER(dai_opbaseoverride)
{
	tms5501_set_pio_bit_7 (0, (readinputport(8) & 0x04) ? 1:0);
	return address;
}

/* Memory */

WRITE8_HANDLER( dai_stack_interrupt_circuit_w )
{
	tms5501_sensor (0, 1);
	tms5501_sensor (0, 0);
}

static void dai_update_memory (int dai_rom_bank)
{
	memory_set_bank(2, dai_rom_bank);
}

static void dai_bootstrap_callback (int param)
{
	cpunum_set_reg(0, I8080_PC, 0xc000);
}

static UINT8 dai_keyboard_scan_mask = 0;

static UINT8 dai_keyboard_read (void)
{
	UINT8 data = 0x00;
	int i;

	for (i = 0; i < 8; i++)
	{
		if (dai_keyboard_scan_mask & (1 << i))
			data |= readinputport(i);
	}
	return data;
}

static void dai_keyboard_write (UINT8 data)
{
	dai_keyboard_scan_mask = data;
}

static void dai_interrupt_callback(int intreq, UINT8 vector)
{
	if (intreq)
		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, vector);
	else
		cpunum_set_input_line(0, 0, CLEAR_LINE);
}

static const tms5501_init_param dai_tms5501_init_param =
{
	dai_keyboard_read,
	dai_keyboard_write,
	dai_interrupt_callback,
	2000000.
};

static ppi8255_interface dai_ppi82555_intf =
{
	1, 			/* 1 chip */
	{ NULL, NULL },		/* Port A read */
	{ NULL, NULL },		/* Port B read */
	{ NULL, NULL },		/* Port C read */
};

static struct pit8253_config dai_pit8253_intf =
{
	TYPE8253,
	{
		{
			2000000,
			NULL,
			dai_sh_change_clock
		},
		{
			2000000,
			NULL,
			dai_sh_change_clock
		},
		{
			2000000,
			NULL,
			dai_sh_change_clock
		}
	}
};

MACHINE_START( dai )
{
	memory_set_opbase_handler(0, dai_opbaseoverride);

	memory_set_bankptr(1, mess_ram);
	memory_configure_bank(2, 0, 4, memory_region(REGION_CPU1) + 0x010000, 0x1000);

	tms5501_init(0, &dai_tms5501_init_param);
	ppi8255_init(&dai_ppi82555_intf);
	pit8253_init(1, &dai_pit8253_intf);

	timer_set(0, 0, dai_bootstrap_callback);
	return 0;
}

/***************************************************************************

	Discrete Devices IO

	FD00	POR1:	IN	bit 0	-
				bit 1	-
				bit 2	PIPGE: Page signal
				bit 3	PIDTR: Serial output ready
				bit 4	PIBU1: Button on paddle 1 (1 = closed)
				bit 5	PIBU1: Button on paddle 2 (1 = closed)
				bit 6	PIRPI: Random data
				bit 7	PICAI: Cassette input data

	FD01	PDLST:	IN	Single pulse used to trigger paddle timer circuit

	FD04	POR1:	OUT	bit 0-3	Volume oscillator channel 0
				bit 4-7	Volume oscillator channel 1

	FD05	POR1:	OUT	bit 0-3	Volume oscillator channel 2
				bit 4-7	Volume random noise generator

	FD06	POR0:	OUT	bit 0	POCAS: Cassette data output
				bit 1-2	PDLMSK: Paddle select
				bit 3	PDPNA:	Paddle enable
				bit 4	POCM1:	Cassette 1 motor control (0 = run)
				bit 5	POCM2:	Cassette 2 motor control (0 = run)
				bit 6-7			ROM bank switching
***************************************************************************/

 READ8_HANDLER( dai_io_discrete_devices_r )
{
	UINT8 data = 0x00;

	switch(offset & 0x000f) {
	case 0x00:
		data = readinputport(8);
		data |= 0x08;			// serial ready
		if (mame_rand(Machine)&0x01)
			data |= 0x40;		// random number generator
		if (cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0)) > 0.01)
			data |= 0x80;		// tape input
		break;

	default:
		data = 0xff;
		LOG_DAI_PORT_R (offset, data, "discrete devices - unmapped");

		break;
	}
	return data;
}

WRITE8_HANDLER( dai_io_discrete_devices_w )
{
	switch(offset & 0x000f) {
	case 0x04:
		dai_osc_volume[0] = data&0x0f;
		dai_osc_volume[1] = (data&0xf0)>>4;
		LOG_DAI_PORT_W (offset, data&0x0f, "discrete devices - osc. 0 volume");
		LOG_DAI_PORT_W (offset, (data&0xf0)>>4, "discrete devices - osc. 1 volume");
		break;

	case 0x05:
		dai_osc_volume[2] = data&0x0f;
		dai_noise_volume = (data&0xf0)>>4;
		LOG_DAI_PORT_W (offset, data&0x0f, "discrete devices - osc. 2 volume");
		LOG_DAI_PORT_W (offset, (data&0xf0)>>4, "discrete devices - noise volume");
		break;

	case 0x06:
		dai_paddle_select = (data&0x06)>>2;
		dai_paddle_enable = (data&0x08)>>3;
		dai_cassette_motor[0] = (data&0x10)>>4;
		dai_cassette_motor[1] = (data&0x20)>>5;
		cassette_change_state(image_from_devtype_and_index(IO_CASSETTE, 0), dai_cassette_motor[0]?CASSETTE_MOTOR_DISABLED:CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);
		cassette_output(image_from_devtype_and_index(IO_CASSETTE, 0), (data & 0x01) ? -1.0 : 1.0);
		dai_update_memory ((data&0xc0)>>6);
		LOG_DAI_PORT_W (offset, (data&0x06)>>2, "discrete devices - paddle select");
		LOG_DAI_PORT_W (offset, (data&0x08)>>3, "discrete devices - paddle enable");
		LOG_DAI_PORT_W (offset, (data&0x10)>>4, "discrete devices - cassette motor 1");
		LOG_DAI_PORT_W (offset, (data&0x20)>>5, "discrete devices - cassette motor 2");
		LOG_DAI_PORT_W (offset, (data&0xc0)>>6, "discrete devices - ROM bank");
		break;

	default:
		LOG_DAI_PORT_W (offset, data, "discrete devices - unmapped");
		break;
	}
}

/***************************************************************************

	AMD 9911 mathematical coprocesor

***************************************************************************/

 READ8_HANDLER( amd9511_r )
{
	/* optional and no present at this moment */
	return 0xff;
}

WRITE8_HANDLER( amd9511_w )
{
	logerror ("Writing to AMD9511 math chip, %04x, %02x\n", offset, data);
}

