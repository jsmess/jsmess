/***************************************************************************

        PP-01 driver by Miodrag Milanovic

        08/09/2008 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "includes/pp01.h"
#include "devices/messram.h"

static UINT8 memory_block[16];
UINT8 pp01_video_scroll;
static UINT8 pp01_video_write_mode;

WRITE8_HANDLER( pp01_video_write_mode_w )
{
	pp01_video_write_mode = data & 0x0f;
}

static void pp01_video_w(running_machine *machine,UINT8 block,UINT16 offset,UINT8 data,UINT8 part)
{
	UINT16 addroffset = part ? 0x1000  : 0x0000;

	if (BIT(pp01_video_write_mode,3)) {
		// Copy mode
		if(BIT(pp01_video_write_mode,0)) {
			messram_get_ptr(devtag_get_device(machine, "messram"))[0x6000+offset+addroffset] = data;
		} else {
			messram_get_ptr(devtag_get_device(machine, "messram"))[0x6000+offset+addroffset] = 0;
		}
		if(BIT(pp01_video_write_mode,1)) {
			messram_get_ptr(devtag_get_device(machine, "messram"))[0xa000+offset+addroffset] = data;
		} else {
			messram_get_ptr(devtag_get_device(machine, "messram"))[0xa000+offset+addroffset] = 0;
		}
		if(BIT(pp01_video_write_mode,2)) {
			messram_get_ptr(devtag_get_device(machine, "messram"))[0xe000+offset+addroffset] = data;
		} else {
			messram_get_ptr(devtag_get_device(machine, "messram"))[0xe000+offset+addroffset] = 0;
		}
	} else {
		if (block==0) {
			messram_get_ptr(devtag_get_device(machine, "messram"))[0x6000+offset+addroffset] = data;
		}
		if (block==1) {
			messram_get_ptr(devtag_get_device(machine, "messram"))[0xa000+offset+addroffset] = data;
		}
		if (block==2) {
			messram_get_ptr(devtag_get_device(machine, "messram"))[0xe000+offset+addroffset] = data;
		}
	}
}

static WRITE8_HANDLER (pp01_video_r_1_w)
{
	pp01_video_w(space->machine,0,offset,data,0);
}
static WRITE8_HANDLER (pp01_video_g_1_w)
{
	pp01_video_w(space->machine,1,offset,data,0);
}
static WRITE8_HANDLER (pp01_video_b_1_w)
{
	pp01_video_w(space->machine,2,offset,data,0);
}

static WRITE8_HANDLER (pp01_video_r_2_w)
{
	pp01_video_w(space->machine,0,offset,data,1);
}
static WRITE8_HANDLER (pp01_video_g_2_w)
{
	pp01_video_w(space->machine,1,offset,data,1);
}
static WRITE8_HANDLER (pp01_video_b_2_w)
{
	pp01_video_w(space->machine,2,offset,data,1);
}


static void pp01_set_memory(running_machine *machine,UINT8 block, UINT8 data)
{
	UINT8 *mem = memory_region(machine, "maincpu");
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT16 startaddr = block*0x1000;
	UINT16 endaddr   = ((block+1)*0x1000)-1;
	UINT8  blocknum  = block + 1;
	char bank[10];
	sprintf(bank,"bank%d",blocknum);
	if (data>=0xE0 && data<=0xEF) {
		// This is RAM
		memory_install_read_bank (space, startaddr, endaddr, 0, 0, bank);
		switch(data) {
			case 0xe6 :
					memory_install_write8_handler(space, startaddr, endaddr, 0, 0, pp01_video_r_1_w);
					break;
			case 0xe7 :
					memory_install_write8_handler(space, startaddr, endaddr, 0, 0, pp01_video_r_2_w);
					break;
			case 0xea :
					memory_install_write8_handler(space, startaddr, endaddr, 0, 0, pp01_video_g_1_w);
					break;
			case 0xeb :
					memory_install_write8_handler(space, startaddr, endaddr, 0, 0, pp01_video_g_2_w);
					break;
			case 0xee :
					memory_install_write8_handler(space, startaddr, endaddr, 0, 0, pp01_video_b_1_w);
					break;
			case 0xef :
					memory_install_write8_handler(space, startaddr, endaddr, 0, 0, pp01_video_b_2_w);
					break;

			default :
					memory_install_write_bank(space, startaddr, endaddr, 0, 0, bank);
					break;
		}

		memory_set_bankptr(machine, bank, messram_get_ptr(devtag_get_device(machine, "messram")) + (data & 0x0F)* 0x1000);
	} else if (data>=0xF8) {
		memory_install_read_bank (space, startaddr, endaddr, 0, 0, bank);
		memory_unmap_write(space, startaddr, endaddr, 0, 0);
		memory_set_bankptr(machine, bank, mem + ((data & 0x0F)-8)* 0x1000+0x10000);
	} else {
		logerror("%02x %02x\n",block,data);
		memory_unmap_readwrite (space, startaddr, endaddr, 0, 0);
	}
}

MACHINE_RESET( pp01 )
{
	int i;
	memset(memory_block,0xff,16);
	for(i=0;i<16;i++) {
		memory_block[i] = 0xff;
		pp01_set_memory(machine, i, 0xff);
	}
}

WRITE8_HANDLER (pp01_mem_block_w)
{
	memory_block[offset] = data;
	pp01_set_memory(space->machine, offset, data);
}

READ8_HANDLER (pp01_mem_block_r)
{
	return	memory_block[offset];
}

MACHINE_START(pp01)
{
}


static WRITE_LINE_DEVICE_HANDLER( pp01_pit_out0 )
{
}

static WRITE_LINE_DEVICE_HANDLER( pp01_pit_out1 )
{
}

const struct pit8253_config pp01_pit8253_intf =
{
	{
		{
			0,
			DEVCB_NULL,
			DEVCB_LINE(pp01_pit_out0)
		},
		{
			2000000,
			DEVCB_NULL,
			DEVCB_LINE(pp01_pit_out1)
		},
		{
			2000000,
			DEVCB_NULL,
			DEVCB_LINE(pit8253_clk0_w)
		}
	}
};

static READ8_DEVICE_HANDLER (pp01_8255_porta_r )
{
	return pp01_video_scroll;
}
static WRITE8_DEVICE_HANDLER (pp01_8255_porta_w )
{
	pp01_video_scroll = data;
}

static UINT8 pp01_key_line = 0;
static READ8_DEVICE_HANDLER (pp01_8255_portb_r )
{
	static const char *const keynames[] = {
		"LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7",
		"LINE8", "LINE9", "LINEA", "LINEB", "LINEC", "LINED", "LINEE", "LINEF"
	};

	return (input_port_read(device->machine,keynames[pp01_key_line]) & 0x3F) | (input_port_read(device->machine,"LINEALL") & 0xC0);
}
static WRITE8_DEVICE_HANDLER (pp01_8255_portb_w )
{
	//logerror("pp01_8255_portb_w %02x\n",data);

}

static WRITE8_DEVICE_HANDLER (pp01_8255_portc_w )
{
	pp01_key_line = data & 0x0f;
}

static READ8_DEVICE_HANDLER (pp01_8255_portc_r )
{
	return 0xff;
}



I8255A_INTERFACE( pp01_ppi8255_interface )
{
	DEVCB_HANDLER(pp01_8255_porta_r),
	DEVCB_HANDLER(pp01_8255_portb_r),
	DEVCB_HANDLER(pp01_8255_portc_r),
	DEVCB_HANDLER(pp01_8255_porta_w),
	DEVCB_HANDLER(pp01_8255_portb_w),
	DEVCB_HANDLER(pp01_8255_portc_w)
};

