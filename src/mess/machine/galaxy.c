/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/galaxy.h"
#include "devices/cassette.h"

/***************************************************************************
  I/O devices
***************************************************************************/

READ8_HANDLER( galaxy_keyboard_r )
{
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7" };

	if (offset == 0)
	{
		double level = cassette_input(devtag_get_device(space->machine, "cassette"));
		return (level >  0) ? 0xfe : 0xff;
	}
	else
	{
		return input_port_read(space->machine, keynames[(offset>>3) & 0x07]) & (0x01<<(offset & 0x07)) ? 0xfe : 0xff;
	}
}

UINT8 gal_latch_value = 0;

WRITE8_HANDLER( galaxy_latch_w )
{
	double val = (((data >>6) & 1 ) + ((data >> 2) & 1) - 1) * 32000;
	gal_latch_value = data;
	cassette_output(devtag_get_device(space->machine, "cassette"), val);
}



/***************************************************************************
  Interrupts
***************************************************************************/
int galaxy_interrupts_enabled = TRUE;

INTERRUPT_GEN( galaxy_interrupt )
{
	cpu_set_input_line(device, 0, HOLD_LINE);
}

static IRQ_CALLBACK ( galaxy_irq_callback )
{
	gal_cnt = 0;
	galaxy_interrupts_enabled = TRUE;
	timer_adjust_periodic(gal_video_timer, attotime_zero, 0, ATTOTIME_IN_HZ(6144000 / 8));
	return 0xff;
}

/***************************************************************************
  Snapshot files (GAL)
***************************************************************************/

#define GALAXY_SNAPSHOT_V1_SIZE	8268
#define GALAXY_SNAPSHOT_V2_SIZE	8244

static void galaxy_setup_snapshot (running_machine *machine, const UINT8 * data, UINT32 size)
{
	const device_config *cpu = cputag_get_cpu(machine, "maincpu");

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

			memcpy (mess_ram, data + 0x084c, (mess_ram_size < 0x1800) ? mess_ram_size : 0x1800);

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
			cpu_set_reg(cpu, Z80_IFF2, 0);

			cpu_set_reg(cpu, Z80_HALT, 0);

			cpu_set_reg(cpu, Z80_IM,   (data[0x18] >> 1) & 0x03);

			cpu_set_reg(cpu, Z80_I,    data[0x19]);
			cpu_set_reg(cpu, Z80_R,    data[0x1a]);

			memcpy (mess_ram, data + 0x0834, (mess_ram_size < 0x1800) ? mess_ram_size : 0x1800);

			break;
	}

	cpu_set_input_line(cpu, INPUT_LINE_NMI, CLEAR_LINE);
	cpu_set_input_line(cpu, INPUT_LINE_IRQ0, CLEAR_LINE);
}

SNAPSHOT_LOAD( galaxy )
{
	UINT8* snapshot_data;

	switch (snapshot_size)
	{
		case GALAXY_SNAPSHOT_V1_SIZE:
		case GALAXY_SNAPSHOT_V2_SIZE:
			snapshot_data = auto_alloc_array(image->machine, UINT8, snapshot_size);
			break;
		default:
			return INIT_FAIL;
	}

	image_fread(image, snapshot_data, snapshot_size);

	galaxy_setup_snapshot(image->machine, snapshot_data, snapshot_size);

	return INIT_PASS;
}


/***************************************************************************
  Driver Initialization
***************************************************************************/

DRIVER_INIT( galaxy )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	memory_install_read8_handler( space, 0x2800, 0x2800 + mess_ram_size - 1, 0, 0, SMH_BANK(1));
	memory_install_write8_handler(space, 0x2800, 0x2800 + mess_ram_size - 1, 0, 0, SMH_BANK(1));
	memory_set_bankptr(machine, 1, mess_ram);

	if (mess_ram_size < (6 + 48) * 1024)
	{
		memory_install_read8_handler( space, 0x2800 + mess_ram_size, 0xffff, 0, 0, SMH_NOP);
		memory_install_write8_handler(space, 0x2800 + mess_ram_size, 0xffff, 0, 0, SMH_NOP);
	}
}

/***************************************************************************
  Machine Initialization
***************************************************************************/

MACHINE_RESET( galaxy )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	/* ROM 2 enable/disable */
	memory_install_read8_handler(space, 0x1000, 0x1fff, 0, 0, input_port_read(machine, "ROM2") ? SMH_BANK(10) : SMH_NOP);
	memory_install_write8_handler(space, 0x1000, 0x1fff, 0, 0, SMH_NOP);
	memory_set_bankptr(machine,10, memory_region(machine, "maincpu") + 0x1000);

	cpu_set_irq_callback(cputag_get_cpu(machine, "maincpu"), galaxy_irq_callback);
	galaxy_interrupts_enabled = TRUE;

	gal_video_timer = timer_alloc(machine, gal_video, NULL);
	timer_adjust_periodic(gal_video_timer, attotime_zero, 0,attotime_never);
}

DRIVER_INIT( galaxyp )
{
	DRIVER_INIT_CALL(galaxy);
}

MACHINE_RESET( galaxyp )
{
	UINT8 *ROM = memory_region(machine, "maincpu");
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	cpu_set_irq_callback(cputag_get_cpu(machine, "maincpu"), galaxy_irq_callback);

	ROM[0x0037] = 0x29;
	ROM[0x03f9] = 0xcd;
	ROM[0x03fa] = 0x00;
	ROM[0x03fb] = 0xe0;

	memory_install_read8_handler(space, 0xe000, 0xefff, 0, 0, SMH_BANK(11));
	memory_install_write8_handler(space, 0xe000, 0xefff, 0, 0, SMH_NOP);
	memory_set_bankptr(machine,11, memory_region(machine, "maincpu") + 0xe000);
	galaxy_interrupts_enabled = TRUE;

	gal_video_timer = timer_alloc(machine, gal_video, NULL);
	timer_adjust_periodic(gal_video_timer, attotime_zero, 0, attotime_never);

}
