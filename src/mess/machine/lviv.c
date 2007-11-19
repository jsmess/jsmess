/***************************************************************************

  machine.c

  Functions to emulate general aspects of PK-01 Lviv (RAM, ROM, interrupts,
  I/O ports)

  Krzysztof Strzecha

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "devices/cassette.h"
#include "devices/snapquik.h"
#include "cpu/i8085/i8085.h"
#include "includes/lviv.h"
#include "machine/8255ppi.h"
#include "sound/speaker.h"
#include "image.h"

#define LVIV_SNAPSHOT_SIZE	82219

unsigned char * lviv_video_ram;

UINT8 lviv_ppi_port_outputs[2][3];

static UINT8 startup_mem_map;


static void lviv_update_memory (void)
{
	if (lviv_ppi_port_outputs[0][2] & 0x02)
	{
		memory_set_bankptr(1, mess_ram);
		memory_set_bankptr(2, mess_ram + 0x4000);
	}
	else
	{
		memory_set_bankptr(1, mess_ram + 0x8000);
		memory_set_bankptr(2, lviv_video_ram);
	}
}

static OPBASE_HANDLER(lviv_opbaseoverride)
{
	if (readinputport(12)&0x01)
		mame_schedule_soft_reset(Machine);
	return address;
}

static  READ8_HANDLER ( lviv_ppi_0_porta_r )
{
	return 0xff;
}

static  READ8_HANDLER ( lviv_ppi_0_portb_r )
{
	return 0xff;
}

static  READ8_HANDLER ( lviv_ppi_0_portc_r )
{
	UINT8 data = lviv_ppi_port_outputs[0][2] & 0x0f;
	if (cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0)) > 0.038)
		data |= 0x10;
	if (lviv_ppi_port_outputs[0][0] & readinputport(13))
		data |= 0x80;
	return data;
}

static WRITE8_HANDLER ( lviv_ppi_0_porta_w )
{
	lviv_ppi_port_outputs[0][0] = data;
}

static WRITE8_HANDLER ( lviv_ppi_0_portb_w )
{
	lviv_ppi_port_outputs[0][1] = data;
	lviv_update_palette (data&0x7f);
}

static WRITE8_HANDLER ( lviv_ppi_0_portc_w )	/* tape in/out, video memory on/off */
{
	lviv_ppi_port_outputs[0][2] = data;
	if (lviv_ppi_port_outputs[0][1]&0x80)
		speaker_level_w(0, data&0x01);
	cassette_output(image_from_devtype_and_index(IO_CASSETTE, 0), (data & 0x01) ? -1.0 : 1.0);
	lviv_update_memory();
}

static  READ8_HANDLER ( lviv_ppi_1_porta_r )
{
	return 0xff;
}

static  READ8_HANDLER ( lviv_ppi_1_portb_r )	/* keyboard reading */
{
	return	((lviv_ppi_port_outputs[1][0]&0x01) ? 0xff : readinputport(0)) &
		((lviv_ppi_port_outputs[1][0]&0x02) ? 0xff : readinputport(1)) &
		((lviv_ppi_port_outputs[1][0]&0x04) ? 0xff : readinputport(2)) &
		((lviv_ppi_port_outputs[1][0]&0x08) ? 0xff : readinputport(3)) &
		((lviv_ppi_port_outputs[1][0]&0x10) ? 0xff : readinputport(4)) &
		((lviv_ppi_port_outputs[1][0]&0x20) ? 0xff : readinputport(5)) &
		((lviv_ppi_port_outputs[1][0]&0x40) ? 0xff : readinputport(6)) &
		((lviv_ppi_port_outputs[1][0]&0x80) ? 0xff : readinputport(7));
}

static  READ8_HANDLER ( lviv_ppi_1_portc_r )     /* keyboard reading */
{
	return	((lviv_ppi_port_outputs[1][2]&0x01) ? 0xff : readinputport(8))  &
		((lviv_ppi_port_outputs[1][2]&0x02) ? 0xff : readinputport(9))  &
		((lviv_ppi_port_outputs[1][2]&0x04) ? 0xff : readinputport(10)) &
		((lviv_ppi_port_outputs[1][2]&0x08) ? 0xff : readinputport(11));
}

static WRITE8_HANDLER ( lviv_ppi_1_porta_w )	/* kayboard scaning */
{
	lviv_ppi_port_outputs[1][0] = data;
}

static WRITE8_HANDLER ( lviv_ppi_1_portb_w )
{
	lviv_ppi_port_outputs[1][1] = data;
}

static WRITE8_HANDLER ( lviv_ppi_1_portc_w )	/* kayboard scaning */
{
	lviv_ppi_port_outputs[1][2] = data;
}


/* I/O */
 READ8_HANDLER ( lviv_io_r )
{
	if (startup_mem_map)
	{
		return 0;	/* ??? */
	}
	else
	{
		switch ((offset >> 4) & 0x3)
		{
		case 0:
			return ppi8255_0_r(offset & 3);
			break;

		case 1:
			return ppi8255_1_r(offset & 3);
			break;

		case 2:
		case 3:
		default:
			/* reserved for extension? */
			return 0;	/* ??? */
		}
	}
}

WRITE8_HANDLER ( lviv_io_w )
{
	if (startup_mem_map)
	{
		startup_mem_map = 0;

		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, MWA8_BANK1);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, MWA8_BANK2);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0, MWA8_BANK3);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xC000, 0xffff, 0, 0, MWA8_ROM);

		memory_set_bankptr(1, mess_ram);
		memory_set_bankptr(2, mess_ram + 0x4000);
		memory_set_bankptr(3, mess_ram + 0x8000);
		memory_set_bankptr(4, memory_region(REGION_CPU1) + 0x010000);
	}
	else
	{
		switch ((offset >> 4) & 0x3)
		{
		case 0:
			ppi8255_0_w(offset & 3, data);
			break;

		case 1:
			ppi8255_1_w(offset & 3, data);
			break;

		case 2:
		case 3:
			/* reserved for extension? */
			break;
		}
	}
}


static ppi8255_interface lviv_ppi8255_interface =
{
	2,
	{lviv_ppi_0_porta_r, lviv_ppi_1_porta_r},
	{lviv_ppi_0_portb_r, lviv_ppi_1_portb_r},
	{lviv_ppi_0_portc_r, lviv_ppi_1_portc_r},
	{lviv_ppi_0_porta_w, lviv_ppi_1_porta_w},
	{lviv_ppi_0_portb_w, lviv_ppi_1_portb_w},
	{lviv_ppi_0_portc_w, lviv_ppi_1_portc_w}
};

MACHINE_RESET( lviv )
{
	memory_set_opbase_handler(0, lviv_opbaseoverride);

	ppi8255_init(&lviv_ppi8255_interface);

	lviv_video_ram = mess_ram + 0xc000;

	startup_mem_map = 1;

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, MWA8_ROM);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, MWA8_ROM);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0, MWA8_ROM);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xC000, 0xffff, 0, 0, MWA8_ROM);

	memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x010000);
	memory_set_bankptr(2, memory_region(REGION_CPU1) + 0x010000);
	memory_set_bankptr(3, memory_region(REGION_CPU1) + 0x010000);
	memory_set_bankptr(4, memory_region(REGION_CPU1) + 0x010000);

	/*timer_pulse(TIME_IN_NSEC(200), 0, lviv_draw_pixel);*/

	/*memset(mess_ram, 0, sizeof(unsigned char)*0xffff);*/
}


/*******************************************************************************
Lviv snapshot files (SAV)
-------------------------

00000 - 0000D:  'LVOV/DUMP/2.0/' (like LVT-header)
0000E - 0000F:  'H+' (something additional)
00010           00h
00011 - 0C010:  RAM (0000 - BFFF)
0C011 - 10010:  ROM (C000 - FFFF)
10011 - 14010:  Video RAM (4000 - 7FFF)
14011 - 14110:  Ports map (00 - FF)
14111 - 1411C:  Registers (B,C,D,E,H,L,A,F,SP,PC)
1411D - 1412A:  ??? (something additional)
*******************************************************************************/

static void lviv_setup_snapshot (UINT8 * data)
{
	unsigned char lo,hi;

	/* Set registers */
	lo = data[0x14112] & 0x0ff;
	hi = data[0x14111] & 0x0ff;
	cpunum_set_reg(0, I8080_BC, (hi << 8) | lo);
	lo = data[0x14114] & 0x0ff;
	hi = data[0x14113] & 0x0ff;
	cpunum_set_reg(0, I8080_DE, (hi << 8) | lo);
	lo = data[0x14116] & 0x0ff;
	hi = data[0x14115] & 0x0ff;
	cpunum_set_reg(0, I8080_HL, (hi << 8) | lo);
	lo = data[0x14118] & 0x0ff;
	hi = data[0x14117] & 0x0ff;
	cpunum_set_reg(0, I8080_AF, (hi << 8) | lo);
	lo = data[0x14119] & 0x0ff;
	hi = data[0x1411a] & 0x0ff;
	cpunum_set_reg(0, I8080_SP, (hi << 8) | lo);
	lo = data[0x1411b] & 0x0ff;
	hi = data[0x1411c] & 0x0ff;
	cpunum_set_reg(0, I8080_PC, (hi << 8) | lo);

	/* Memory dump */
	memcpy (mess_ram, data+0x0011, 0xc000);
	memcpy (mess_ram+0xc000, data+0x10011, 0x4000);

	/* Ports */
	lviv_ppi_port_outputs[0][0] = data[0x14011+0xc0];
	lviv_ppi_port_outputs[0][1] = data[0x14011+0xc1];
	lviv_update_palette (lviv_ppi_port_outputs[0][1]&0x7f);
	lviv_ppi_port_outputs[0][2] = data[0x14011+0xc2];
	lviv_update_memory();
}

static void dump_registers(void)
{
	logerror("PC   = %04x\n", (unsigned) cpunum_get_reg(0, I8080_PC));
	logerror("SP   = %04x\n", (unsigned) cpunum_get_reg(0, I8080_SP));
	logerror("AF   = %04x\n", (unsigned) cpunum_get_reg(0, I8080_AF));
	logerror("BC   = %04x\n", (unsigned) cpunum_get_reg(0, I8080_BC));
	logerror("DE   = %04x\n", (unsigned) cpunum_get_reg(0, I8080_DE));
	logerror("HL   = %04x\n", (unsigned) cpunum_get_reg(0, I8080_HL));
}

static int lviv_verify_snapshot (UINT8 * data, UINT32 size)
{
	const char* tag = "LVOV/DUMP/2.0/";

	if( strncmp( tag, (char*)data, strlen(tag) ) )
	{
		logerror("Not a Lviv snapshot\n");
		return IMAGE_VERIFY_FAIL;
	}

	if (size != LVIV_SNAPSHOT_SIZE)
	{
		logerror ("Incomplete snapshot file\n");
		return IMAGE_VERIFY_FAIL;
	}

	logerror("returning ID_OK\n");
	return IMAGE_VERIFY_PASS;
}

SNAPSHOT_LOAD( lviv )
{
	UINT8 *lviv_snapshot_data;

	lviv_snapshot_data = malloc(LVIV_SNAPSHOT_SIZE);
	if (!lviv_snapshot_data)
	{
		logerror ("Unable to load snapshot file\n");
		return INIT_FAIL;
	}

	image_fread(image, lviv_snapshot_data, LVIV_SNAPSHOT_SIZE);

	if( lviv_verify_snapshot(lviv_snapshot_data, snapshot_size) == IMAGE_VERIFY_FAIL)
	{
		free(lviv_snapshot_data);
		return INIT_FAIL;
	}

	lviv_setup_snapshot (lviv_snapshot_data);

	dump_registers();	

	free(lviv_snapshot_data);

	logerror("Snapshot file loaded\n");
	return INIT_PASS;
}

