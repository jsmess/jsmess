/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/galaxy.h"

#define DEBUG_GALAXY_LATCH	0

#if DEBUG_GALAXY_LATCH
	#define LOG_GALAXY_LATCH_R(_port, _data) logerror ("Galaxy latch read : %04x, Data: %02x\n", _port, _data)
	#define LOG_GALAXY_LATCH_W(_port, _data) logerror ("Galaxy latch write: %04x, Data: %02x\n", _port, _data)
#else
	#define LOG_GALAXY_LATCH_R(_port, _data)
	#define LOG_GALAXY_LATCH_W(_port, _data)
#endif

int galaxy_interrupts_enabled = TRUE;

/***************************************************************************
  I/O devices
***************************************************************************/

READ8_HANDLER( galaxy_keyboard_r )
{
	return readinputport((offset>>3)&0x07) & (0x01<<(offset&0x07)) ? 0xfe : 0xff;
}

READ8_HANDLER( galaxy_latch_r )
{
	UINT8 data = 0xff;
	LOG_GALAXY_LATCH_R(offset, data);
	return data;
}

WRITE8_HANDLER( galaxy_latch_w )
{
	LOG_GALAXY_LATCH_W(offset, data);
}

/***************************************************************************
  Interrupts
***************************************************************************/

INTERRUPT_GEN( galaxy_interrupt )
{
	cpunum_set_input_line(0, 0, HOLD_LINE);
}

static int galaxy_irq_callback (int cpu)
{
	galaxy_interrupts_enabled = TRUE;
	return 1;
}

/***************************************************************************
  Snapshot files (GAL)
***************************************************************************/

#define GALAXY_SNAPSHOT_V1_SIZE	8268
#define GALAXY_SNAPSHOT_V2_SIZE	8244

static void galaxy_setup_snapshot (const UINT8 * data, UINT32 size)
{
	switch (size)
	{
		case GALAXY_SNAPSHOT_V1_SIZE:
			cpunum_set_reg(0, Z80_AF,   data[0x00] | data[0x01]<<8);
			cpunum_set_reg(0, Z80_BC,   data[0x04] | data[0x05]<<8);
			cpunum_set_reg(0, Z80_DE,   data[0x08] | data[0x09]<<8);
			cpunum_set_reg(0, Z80_HL,   data[0x0c] | data[0x0d]<<8);
			cpunum_set_reg(0, Z80_IX,   data[0x10] | data[0x11]<<8);
			cpunum_set_reg(0, Z80_IY,   data[0x14] | data[0x15]<<8);
			cpunum_set_reg(0, Z80_PC,   data[0x18] | data[0x19]<<8);
			cpunum_set_reg(0, Z80_SP,   data[0x1c] | data[0x1d]<<8);
			cpunum_set_reg(0, Z80_AF2,  data[0x20] | data[0x21]<<8);
			cpunum_set_reg(0, Z80_BC2,  data[0x24] | data[0x25]<<8);
			cpunum_set_reg(0, Z80_DE2,  data[0x28] | data[0x29]<<8);
			cpunum_set_reg(0, Z80_HL2,  data[0x2c] | data[0x2d]<<8);
			cpunum_set_reg(0, Z80_IFF1, data[0x30]);
			cpunum_set_reg(0, Z80_IFF2, data[0x34]);
			cpunum_set_reg(0, Z80_HALT, data[0x38]);
			cpunum_set_reg(0, Z80_IM,   data[0x3c]);
			cpunum_set_reg(0, Z80_I,    data[0x40]);
			cpunum_set_reg(0, Z80_R,    (data[0x44]&0x7f) | (data[0x48]&0x80));

			memcpy (mess_ram, data+0x084c, (mess_ram_size < 0x1800) ? mess_ram_size : 0x1800);

			break;
		case GALAXY_SNAPSHOT_V2_SIZE:
			cpunum_set_reg(0, Z80_AF,   data[0x00] | data[0x01]<<8);
			cpunum_set_reg(0, Z80_BC,   data[0x02] | data[0x03]<<8);
			cpunum_set_reg(0, Z80_DE,   data[0x04] | data[0x05]<<8);
			cpunum_set_reg(0, Z80_HL,   data[0x06] | data[0x07]<<8);
			cpunum_set_reg(0, Z80_IX,   data[0x08] | data[0x09]<<8);
			cpunum_set_reg(0, Z80_IY,   data[0x0a] | data[0x0b]<<8);
			cpunum_set_reg(0, Z80_PC,   data[0x0c] | data[0x0d]<<8);
			cpunum_set_reg(0, Z80_SP,   data[0x0e] | data[0x0f]<<8);
			cpunum_set_reg(0, Z80_AF2,  data[0x10] | data[0x11]<<8);
			cpunum_set_reg(0, Z80_BC2,  data[0x12] | data[0x13]<<8);
			cpunum_set_reg(0, Z80_DE2,  data[0x14] | data[0x15]<<8);
			cpunum_set_reg(0, Z80_HL2,  data[0x16] | data[0x17]<<8);

			cpunum_set_reg(0, Z80_IFF1, data[0x18]&0x01);
			cpunum_set_reg(0, Z80_IFF2, 0);

			cpunum_set_reg(0, Z80_HALT, 0);

			cpunum_set_reg(0, Z80_IM,   (data[0x18]>>1)&0x03);

			cpunum_set_reg(0, Z80_I,    data[0x19]);
			cpunum_set_reg(0, Z80_R,    data[0x1a]);

			memcpy (mess_ram, data+0x0834, (mess_ram_size < 0x1800) ? mess_ram_size : 0x1800);

			break;
	}

	cpunum_set_input_line(0, INPUT_LINE_NMI, CLEAR_LINE);
	cpunum_set_input_line(0, INPUT_LINE_IRQ0, CLEAR_LINE);
}

SNAPSHOT_LOAD( galaxy )
{
	UINT8* snapshot_data;

	switch (snapshot_size)
	{
		case GALAXY_SNAPSHOT_V1_SIZE:
		case GALAXY_SNAPSHOT_V2_SIZE:
			snapshot_data = auto_malloc(snapshot_size);
			break;
		default:
			return INIT_FAIL;
	}

	image_fread(image, snapshot_data, snapshot_size);
	
	galaxy_setup_snapshot(snapshot_data, snapshot_size);

	return INIT_PASS;
}


/***************************************************************************
  Driver Initialization
***************************************************************************/

DRIVER_INIT( galaxy )
{
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2800+mess_ram_size-1, 0, 0, MRA8_BANK1);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2800+mess_ram_size-1, 0, 0, MWA8_BANK1);
	memory_set_bankptr(1, mess_ram);

	if (mess_ram_size < (6+48)*1024)
	{
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2800+mess_ram_size, 0xffff, 0, 0, MRA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2800+mess_ram_size, 0xffff, 0, 0, MWA8_NOP);
	}
}

/***************************************************************************
  Machine Initialization
***************************************************************************/

MACHINE_RESET( galaxy )
{
	/* ROM 2 enable/disable */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1000, 0x1fff, 0, 0, readinputport(7) ? MRA8_ROM : MRA8_NOP);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1000, 0x1fff, 0, 0, readinputport(7) ? MWA8_ROM : MWA8_NOP);

	cpunum_set_irq_callback(0, galaxy_irq_callback);

	galaxy_interrupts_enabled = TRUE;
}
