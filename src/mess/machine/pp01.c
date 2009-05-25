/***************************************************************************

        PP-01 driver by Miodrag Milanovic

        08/09/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "includes/pp01.h"

static UINT8 memory_block[16];
UINT8 pp01_video_scroll;

/* Driver initialization */
DRIVER_INIT(pp01)
{
	memset(mess_ram, 0, 64 * 1024);
}

static void pp01_set_memory(running_machine *machine)
{	
	UINT8 *mem = memory_region(machine, "maincpu");	
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	int i;
	UINT8 map;
	
	for(i=0;i<16;i++) {
		map = memory_block[i];
		if (map>=0xE0 && map<=0xEF) {
			// This is RAM
			memory_install_read8_handler (space, i*0x1000, ((i+1)*0x1000)-1, 0, 0, SMH_BANK(i+1));
			memory_install_write8_handler(space, i*0x1000, ((i+1)*0x1000)-1, 0, 0, SMH_BANK(i+1));	
			memory_set_bankptr(machine, i+1, mess_ram + (map & 0x0F)* 0x1000);
		} else if (map>=0xF8) {
			memory_install_read8_handler (space, i*0x1000, ((i+1)*0x1000)-1, 0, 0, SMH_BANK(i+1));
			memory_install_write8_handler(space, i*0x1000, ((i+1)*0x1000)-1, 0, 0, SMH_UNMAP);	
			memory_set_bankptr(machine, i+1, mem + ((map & 0x0F)-8)* 0x1000+0x10000);
		} else {
			memory_install_read8_handler(space, i*0x1000, ((i+1)*0x1000)-1, 0, 0, SMH_UNMAP);	
			memory_install_write8_handler(space, i*0x1000, ((i+1)*0x1000)-1, 0, 0, SMH_UNMAP);	
		}
	}	
}
	
MACHINE_RESET( pp01 )
{
	memset(memory_block,0xff,16);
	pp01_set_memory(machine);
}

WRITE8_HANDLER (pp01_mem_block_w) 
{
	memory_block[offset] = data;
	pp01_set_memory(space->machine);
}

MACHINE_START(pp01)
{
}


static PIT8253_OUTPUT_CHANGED(pp01_pit_out0)
{
}

static PIT8253_OUTPUT_CHANGED(pp01_pit_out1)
{
}

static PIT8253_OUTPUT_CHANGED(pp01_pit_out2)
{
	pit8253_set_clock_signal( device, 0, state );
}


const struct pit8253_config pp01_pit8253_intf =
{
	{
		{
			0,
			pp01_pit_out0
		},
		{
			2000000,
			pp01_pit_out1
		},
		{
			2000000,
			pp01_pit_out2
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

static READ8_DEVICE_HANDLER (pp01_8255_portb_r )
{
	return 0xff;
}
static WRITE8_DEVICE_HANDLER (pp01_8255_portb_w )
{	
	logerror("pp01_8255_portb_w %02x\n",data);
}

static WRITE8_DEVICE_HANDLER (pp01_8255_portc_w )
{	
	logerror("pp01_8255_portb_w %02x\n",data);
}

static READ8_DEVICE_HANDLER (pp01_8255_portc_r )
{	
	return 0xff;
}



const ppi8255_interface pp01_ppi8255_interface =
{
	DEVCB_HANDLER(pp01_8255_porta_r),
	DEVCB_HANDLER(pp01_8255_portb_r),
	DEVCB_HANDLER(pp01_8255_portc_r),
	DEVCB_HANDLER(pp01_8255_porta_w),
	DEVCB_HANDLER(pp01_8255_portb_w),
	DEVCB_HANDLER(pp01_8255_portc_w)
};

