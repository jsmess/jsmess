/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "includes/galaxy.h"
#include "imagedev/cassette.h"
#include "machine/ram.h"

/***************************************************************************
  I/O devices
***************************************************************************/

READ8_HANDLER( galaxy_keyboard_r )
{
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7" };

	if (offset == 0)
	{
		double level = cassette_input(space->machine().device("cassette"));
		return (level >  0) ? 0xfe : 0xff;
	}
	else
	{
		return input_port_read(space->machine(), keynames[(offset>>3) & 0x07]) & (0x01<<(offset & 0x07)) ? 0xfe : 0xff;
	}
}

WRITE8_HANDLER( galaxy_latch_w )
{
	galaxy_state *state = space->machine().driver_data<galaxy_state>();
	double val = (((data >>6) & 1 ) + ((data >> 2) & 1) - 1) * 32000;
	state->m_latch_value = data;
	cassette_output(space->machine().device("cassette"), val);
}



/***************************************************************************
  Interrupts
***************************************************************************/

INTERRUPT_GEN( galaxy_interrupt )
{
	device_set_input_line(device, 0, HOLD_LINE);
}

static IRQ_CALLBACK ( galaxy_irq_callback )
{
	galaxy_state *state = device->machine().driver_data<galaxy_state>();
	galaxy_set_timer(device->machine());
	state->m_interrupts_enabled = TRUE;
	return 0xff;
}

/***************************************************************************
  Snapshot files (GAL)
***************************************************************************/

#define GALAXY_SNAPSHOT_V1_SIZE	8268
#define GALAXY_SNAPSHOT_V2_SIZE	8244

static void galaxy_setup_snapshot (running_machine &machine, const UINT8 * data, UINT32 size)
{
	device_t *cpu = machine.device("maincpu");

	switch (size)
	{
		case GALAXY_SNAPSHOT_V1_SIZE:
			cpu_set_reg(cpu, Z80_AF,   data[0x00] | data[0x01] << 8);
			cpu_set_reg(cpu, Z80_BC,   data[0x04] | data[0x05] << 8);
			cpu_set_reg(cpu, Z80_DE,   data[0x08] | data[0x09] << 8);
			cpu_set_reg(cpu, Z80_HL,   data[0x0c] | data[0x0d] << 8);
			cpu_set_reg(cpu, Z80_IX,   data[0x10] | data[0x11] << 8);
			cpu_set_reg(cpu, Z80_IY,   data[0x14] | data[0x15] << 8);
			cpu_set_reg(cpu, Z80_PC,   data[0x18] | data[0x19] << 8);
			cpu_set_reg(cpu, Z80_SP,   data[0x1c] | data[0x1d] << 8);
			cpu_set_reg(cpu, Z80_AF2,  data[0x20] | data[0x21] << 8);
			cpu_set_reg(cpu, Z80_BC2,  data[0x24] | data[0x25] << 8);
			cpu_set_reg(cpu, Z80_DE2,  data[0x28] | data[0x29] << 8);
			cpu_set_reg(cpu, Z80_HL2,  data[0x2c] | data[0x2d] << 8);
			cpu_set_reg(cpu, Z80_IFF1, data[0x30]);
			cpu_set_reg(cpu, Z80_IFF2, data[0x34]);
			cpu_set_reg(cpu, Z80_HALT, data[0x38]);
			cpu_set_reg(cpu, Z80_IM,   data[0x3c]);
			cpu_set_reg(cpu, Z80_I,    data[0x40]);
			cpu_set_reg(cpu, Z80_R,    (data[0x44] & 0x7f) | (data[0x48] & 0x80));

			memcpy (ram_get_ptr(machine.device(RAM_TAG)), data + 0x084c, (ram_get_size(machine.device(RAM_TAG)) < 0x1800) ? ram_get_size(machine.device(RAM_TAG)) : 0x1800);

			break;
		case GALAXY_SNAPSHOT_V2_SIZE:
			cpu_set_reg(cpu, Z80_AF,   data[0x00] | data[0x01] << 8);
			cpu_set_reg(cpu, Z80_BC,   data[0x02] | data[0x03] << 8);
			cpu_set_reg(cpu, Z80_DE,   data[0x04] | data[0x05] << 8);
			cpu_set_reg(cpu, Z80_HL,   data[0x06] | data[0x07] << 8);
			cpu_set_reg(cpu, Z80_IX,   data[0x08] | data[0x09] << 8);
			cpu_set_reg(cpu, Z80_IY,   data[0x0a] | data[0x0b] << 8);
			cpu_set_reg(cpu, Z80_PC,   data[0x0c] | data[0x0d] << 8);
			cpu_set_reg(cpu, Z80_SP,   data[0x0e] | data[0x0f] << 8);
			cpu_set_reg(cpu, Z80_AF2,  data[0x10] | data[0x11] << 8);
			cpu_set_reg(cpu, Z80_BC2,  data[0x12] | data[0x13] << 8);
			cpu_set_reg(cpu, Z80_DE2,  data[0x14] | data[0x15] << 8);
			cpu_set_reg(cpu, Z80_HL2,  data[0x16] | data[0x17] << 8);

			cpu_set_reg(cpu, Z80_IFF1, data[0x18] & 0x01);
			cpu_set_reg(cpu, Z80_IFF2, (UINT64)0);

			cpu_set_reg(cpu, Z80_HALT, (UINT64)0);

			cpu_set_reg(cpu, Z80_IM,   (data[0x18] >> 1) & 0x03);

			cpu_set_reg(cpu, Z80_I,    data[0x19]);
			cpu_set_reg(cpu, Z80_R,    data[0x1a]);

			memcpy (ram_get_ptr(machine.device(RAM_TAG)), data + 0x0834, (ram_get_size(machine.device(RAM_TAG)) < 0x1800) ? ram_get_size(machine.device(RAM_TAG)) : 0x1800);

			break;
	}

	device_set_input_line(cpu, INPUT_LINE_NMI, CLEAR_LINE);
	device_set_input_line(cpu, INPUT_LINE_IRQ0, CLEAR_LINE);
}

SNAPSHOT_LOAD( galaxy )
{
	UINT8* snapshot_data;

	switch (snapshot_size)
	{
		case GALAXY_SNAPSHOT_V1_SIZE:
		case GALAXY_SNAPSHOT_V2_SIZE:
			snapshot_data = auto_alloc_array(image.device().machine(), UINT8, snapshot_size);
			break;
		default:
			return IMAGE_INIT_FAIL;
	}

	image.fread( snapshot_data, snapshot_size);

	galaxy_setup_snapshot(image.device().machine(), snapshot_data, snapshot_size);

	return IMAGE_INIT_PASS;
}


/***************************************************************************
  Driver Initialization
***************************************************************************/

DRIVER_INIT( galaxy )
{
	address_space *space = machine.device("maincpu")->memory().space(AS_PROGRAM);
	space->install_readwrite_bank( 0x2800, 0x2800 + ram_get_size(machine.device(RAM_TAG)) - 1, "bank1");
	memory_set_bankptr(machine, "bank1", ram_get_ptr(machine.device(RAM_TAG)));

	if (ram_get_size(machine.device(RAM_TAG)) < (6 + 48) * 1024)
	{
		space->nop_readwrite( 0x2800 + ram_get_size(machine.device(RAM_TAG)), 0xffff);
	}
}

/***************************************************************************
  Machine Initialization
***************************************************************************/

MACHINE_RESET( galaxy )
{
	galaxy_state *state = machine.driver_data<galaxy_state>();
	address_space *space = machine.device("maincpu")->memory().space(AS_PROGRAM);

	/* ROM 2 enable/disable */
	if (input_port_read(machine, "ROM2")) {
		space->install_read_bank(0x1000, 0x1fff, "bank10");
	} else {
		space->nop_read(0x1000, 0x1fff);
	}
	space->nop_write(0x1000, 0x1fff);

	if (input_port_read(machine, "ROM2"))
		memory_set_bankptr(machine,"bank10", machine.region("maincpu")->base() + 0x1000);

	device_set_irq_callback(machine.device("maincpu"), galaxy_irq_callback);
	state->m_interrupts_enabled = TRUE;
}

DRIVER_INIT( galaxyp )
{
	DRIVER_INIT_CALL(galaxy);
}

MACHINE_RESET( galaxyp )
{
	galaxy_state *state = machine.driver_data<galaxy_state>();
	UINT8 *ROM = machine.region("maincpu")->base();
	address_space *space = machine.device("maincpu")->memory().space(AS_PROGRAM);

	device_set_irq_callback(machine.device("maincpu"), galaxy_irq_callback);

	ROM[0x0037] = 0x29;
	ROM[0x03f9] = 0xcd;
	ROM[0x03fa] = 0x00;
	ROM[0x03fb] = 0xe0;

	space->install_read_bank(0xe000, 0xefff, "bank11");
	space->nop_write(0xe000, 0xefff);
	memory_set_bankptr(machine,"bank11", machine.region("maincpu")->base() + 0xe000);
	state->m_interrupts_enabled = TRUE;
}
