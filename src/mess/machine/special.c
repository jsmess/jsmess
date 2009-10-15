/***************************************************************************

        Specialist machine driver by Miodrag Milanovic

        20/03/2008 Cassette support
        15/03/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "sound/dac.h"
#include "devices/cassette.h"
#include "machine/i8255a.h"
#include "machine/pit8253.h"
#include "machine/wd17xx.h"
#include "includes/special.h"
#include "devices/messram.h"

static UINT8 specimx_color;
UINT8 *specimx_colorram;

static int specialist_8255_porta;
static int specialist_8255_portb;
static int specialist_8255_portc;

/* Driver initialization */
DRIVER_INIT(special)
{
	/* set initialy ROM to be visible on first bank */
	UINT8 *RAM = memory_region(machine, "maincpu");
	memset(RAM,0x0000,0x3000); // make frist page empty by default
	memory_configure_bank(machine, 1, 1, 2, RAM, 0x0000);
	memory_configure_bank(machine, 1, 0, 2, RAM, 0xc000);
}

static READ8_DEVICE_HANDLER (specialist_8255_porta_r )
{
	if (input_port_read(device->machine, "LINE0")!=0xff) return 0xfe;
	if (input_port_read(device->machine, "LINE1")!=0xff) return 0xfd;
	if (input_port_read(device->machine, "LINE2")!=0xff) return 0xfb;
	if (input_port_read(device->machine, "LINE3")!=0xff) return 0xf7;
	if (input_port_read(device->machine, "LINE4")!=0xff) return 0xef;
	if (input_port_read(device->machine, "LINE5")!=0xff) return 0xdf;
	if (input_port_read(device->machine, "LINE6")!=0xff) return 0xbf;
	if (input_port_read(device->machine, "LINE7")!=0xff) return 0x7f;
	return 0xff;
}

static READ8_DEVICE_HANDLER (specialist_8255_portb_r )
{

	int dat = 0;
	double level;

	if ((specialist_8255_porta & 0x01)==0) dat ^= (input_port_read(device->machine, "LINE0") ^ 0xff);
	if ((specialist_8255_porta & 0x02)==0) dat ^= (input_port_read(device->machine, "LINE1") ^ 0xff);
	if ((specialist_8255_porta & 0x04)==0) dat ^= (input_port_read(device->machine, "LINE2") ^ 0xff);
	if ((specialist_8255_porta & 0x08)==0) dat ^= (input_port_read(device->machine, "LINE3") ^ 0xff);
	if ((specialist_8255_porta & 0x10)==0) dat ^= (input_port_read(device->machine, "LINE4") ^ 0xff);
	if ((specialist_8255_porta & 0x20)==0) dat ^= (input_port_read(device->machine, "LINE5") ^ 0xff);
	if ((specialist_8255_porta & 0x40)==0) dat ^= (input_port_read(device->machine, "LINE6") ^ 0xff);
	if ((specialist_8255_porta & 0x80)==0) dat ^= (input_port_read(device->machine, "LINE7") ^ 0xff);
	if ((specialist_8255_portc & 0x01)==0) dat ^= (input_port_read(device->machine, "LINE8") ^ 0xff);
	if ((specialist_8255_portc & 0x02)==0) dat ^= (input_port_read(device->machine, "LINE9") ^ 0xff);
	if ((specialist_8255_portc & 0x04)==0) dat ^= (input_port_read(device->machine, "LINE10") ^ 0xff);
	if ((specialist_8255_portc & 0x08)==0) dat ^= (input_port_read(device->machine, "LINE11") ^ 0xff);

	dat = (dat  << 2) ^0xff;
	if (input_port_read(device->machine, "LINE12")!=0xff) dat ^= 0x02;

	level = cassette_input(devtag_get_device(device->machine, "cassette"));
	if (level >=  0)
	{
			dat ^= 0x01;
 	}
	return dat & 0xff;
}

static READ8_DEVICE_HANDLER (specialist_8255_portc_r )
{
	if (input_port_read(device->machine, "LINE8")!=0xff) return 0x0e;
	if (input_port_read(device->machine, "LINE9")!=0xff) return 0x0d;
	if (input_port_read(device->machine, "LINE10")!=0xff) return 0x0b;
	if (input_port_read(device->machine, "LINE11")!=0xff) return 0x07;
	return 0x0f;
}

static WRITE8_DEVICE_HANDLER (specialist_8255_porta_w )
{
	specialist_8255_porta = data;
}

static WRITE8_DEVICE_HANDLER (specialist_8255_portb_w )
{
	specialist_8255_portb = data;
}
static WRITE8_DEVICE_HANDLER (specialist_8255_portc_w )
{
	const device_config *dac_device = devtag_get_device(device->machine, "dac");

	specialist_8255_portc = data;

	cassette_output(devtag_get_device(device->machine, "cassette"),data & 0x80 ? 1 : -1);

	dac_data_w(dac_device, data & 0x20); //beeper

}

I8255A_INTERFACE( specialist_ppi8255_interface )
{
	DEVCB_HANDLER(specialist_8255_porta_r),
	DEVCB_HANDLER(specialist_8255_portb_r),
	DEVCB_HANDLER(specialist_8255_portc_r),
	DEVCB_HANDLER(specialist_8255_porta_w),
	DEVCB_HANDLER(specialist_8255_portb_w),
	DEVCB_HANDLER(specialist_8255_portc_w)
};

static TIMER_CALLBACK( special_reset )
{
	memory_set_bank(machine, 1, 0);
}


MACHINE_RESET( special )
{
	timer_set(machine, ATTOTIME_IN_USEC(10), NULL, 0, special_reset);
	memory_set_bank(machine, 1, 1);
}

READ8_HANDLER( specialist_keyboard_r )
{
	return i8255a_r(devtag_get_device(space->machine, "ppi8255"), (offset & 3));
}

WRITE8_HANDLER( specialist_keyboard_w )
{
	i8255a_w(devtag_get_device(space->machine, "ppi8255"), (offset & 3) , data );
}


/*
     Specialist MX
*/

static WRITE8_HANDLER( video_memory_w )
{
	messram_get_ptr(devtag_get_device(space->machine, "messram"))[0x9000 + offset] = data;
	specimx_colorram[offset]  = specimx_color;
}

WRITE8_HANDLER (specimx_video_color_w )
{
	specimx_color = data;
}

READ8_HANDLER (specimx_video_color_r )
{
	return specimx_color;
}

static void specimx_set_bank(running_machine *machine, int i,int data)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	memory_install_write8_handler(space, 0xc000, 0xffbf, 0, 0, SMH_BANK(3));
	memory_install_write8_handler(space, 0xffc0, 0xffdf, 0, 0, SMH_BANK(4));
	memory_set_bankptr(machine, 4, messram_get_ptr(devtag_get_device(machine, "messram")) + 0xffc0);
	switch(i)
	{
		case 0 :
				memory_install_write8_handler(space, 0x0000, 0x8fff, 0, 0, SMH_BANK(1));
				memory_install_write8_handler(space, 0x9000, 0xbfff, 0, 0, video_memory_w);

				memory_set_bankptr(machine, 1, messram_get_ptr(devtag_get_device(machine, "messram")));
				memory_set_bankptr(machine, 2, messram_get_ptr(devtag_get_device(machine, "messram")) + 0x9000);
				memory_set_bankptr(machine, 3, messram_get_ptr(devtag_get_device(machine, "messram")) + 0xc000);
				break;
		case 1 :
				memory_install_write8_handler(space, 0x0000, 0x8fff, 0, 0, SMH_BANK(1));
				memory_install_write8_handler(space, 0x9000, 0xbfff, 0, 0, SMH_BANK(2));

				memory_set_bankptr(machine, 1, messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000);
				memory_set_bankptr(machine, 2, messram_get_ptr(devtag_get_device(machine, "messram")) + 0x19000);
				memory_set_bankptr(machine, 3, messram_get_ptr(devtag_get_device(machine, "messram")) + 0x1c000);
				break;
		case 2 :
				memory_install_write8_handler(space, 0x0000, 0x8fff, 0, 0, SMH_UNMAP);
				memory_install_write8_handler(space, 0x9000, 0xbfff, 0, 0, SMH_UNMAP);

				memory_set_bankptr(machine, 1, memory_region(machine, "maincpu") + 0x10000);
				memory_set_bankptr(machine, 2, memory_region(machine, "maincpu") + 0x19000);
				if (data & 0x80)
				{
					memory_set_bankptr(machine, 3, messram_get_ptr(devtag_get_device(machine, "messram")) + 0x1c000);
				}
				else
				{
					memory_set_bankptr(machine, 3, messram_get_ptr(devtag_get_device(machine, "messram")) + 0xc000);
				}
				break;
	}
}
WRITE8_HANDLER( specimx_select_bank )
{
	specimx_set_bank(space->machine, offset, data);
}

DRIVER_INIT(specimx)
{
	memset(messram_get_ptr(devtag_get_device(machine, "messram")),0,128*1024);
}

static PIT8253_OUTPUT_CHANGED(specimx_pit8253_out0_changed)
{
	specimx_set_input( device->machine, 0, state );
}



static PIT8253_OUTPUT_CHANGED(specimx_pit8253_out1_changed)
{
	specimx_set_input( device->machine, 1, state );
}



static PIT8253_OUTPUT_CHANGED(specimx_pit8253_out2_changed)
{
	specimx_set_input( device->machine, 2, state );
}



const struct pit8253_config specimx_pit8253_intf =
{
	{
		{
			2000000,
			specimx_pit8253_out0_changed
		},
		{
			2000000,
			specimx_pit8253_out1_changed
		},
		{
			2000000,
			specimx_pit8253_out2_changed
		}
	}
};

MACHINE_START( specimx )
{
	const device_config *fdc = devtag_get_device(machine, "wd1793");
	wd17xx_set_density (fdc,DEN_FM_HI);
}

static TIMER_CALLBACK( setup_pit8253_gates )
{
	const device_config *pit8253 = devtag_get_device(machine, "pit8253");

	pit8253_gate_w(pit8253, 0, 0);
	pit8253_gate_w(pit8253, 1, 0);
	pit8253_gate_w(pit8253, 2, 0);
}

MACHINE_RESET( specimx )
{
	specimx_set_bank(machine, 2,0x00); // Initiali load ROM disk
	specimx_color = 0x70;
	timer_set(machine,  attotime_zero, NULL, 0, setup_pit8253_gates );
	wd17xx_set_pause_time(devtag_get_device(machine, "wd1793"),12);
}

READ8_HANDLER ( specimx_disk_ctrl_r )
{
  return 0xff;
}

WRITE8_HANDLER( specimx_disk_ctrl_w )
{
	const device_config *fdc = devtag_get_device(space->machine, "wd1793");

	switch(offset)
	{
		case 2 :
		 		wd17xx_set_side(fdc,data & 1);
				break;
		case 3 :
		 		wd17xx_set_drive(fdc,data & 1);
		 		break;

  }
}

/*
    Erik
*/
static UINT8 RR_register;
static UINT8 RC_register;

static void erik_set_bank(running_machine *machine)
{
	UINT8 bank1 = (RR_register & 3);
	UINT8 bank2 = ((RR_register >> 2) & 3);
	UINT8 bank3 = ((RR_register >> 4) & 3);
	UINT8 bank4 = ((RR_register >> 6) & 3);
	UINT8 *mem = memory_region(machine, "maincpu");
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_BANK(1));
	memory_install_write8_handler(space, 0x4000, 0x8fff, 0, 0, SMH_BANK(2));
	memory_install_write8_handler(space, 0x9000, 0xbfff, 0, 0, SMH_BANK(3));
	memory_install_write8_handler(space, 0xc000, 0xefff, 0, 0, SMH_BANK(4));
	memory_install_write8_handler(space, 0xf000, 0xf7ff, 0, 0, SMH_BANK(5));
	memory_install_write8_handler(space, 0xf800, 0xffff, 0, 0, SMH_BANK(6));

	switch(bank1)
	{
		case 	1:
		case 	2:
		case 	3:
						memory_set_bankptr(machine, 1, messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000*(bank1-1));
						break;
		case 	0:
						memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_UNMAP);
						memory_set_bankptr(machine, 1, mem + 0x10000);
						break;
	}
	switch(bank2)
	{
		case 	1:
		case 	2:
		case 	3:
						memory_set_bankptr(machine, 2, messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000*(bank2-1) + 0x4000);
						break;
		case 	0:
						memory_install_write8_handler(space, 0x4000, 0x8fff, 0, 0, SMH_UNMAP);
						memory_set_bankptr(machine, 2, mem + 0x14000);
						break;
	}
	switch(bank3)
	{
		case 	1:
		case 	2:
		case 	3:
						memory_set_bankptr(machine, 3, messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000*(bank3-1) + 0x9000);
						break;
		case 	0:
						memory_install_write8_handler(space, 0x9000, 0xbfff, 0, 0, SMH_UNMAP);
						memory_set_bankptr(machine, 3, mem + 0x19000);
						break;
	}
	switch(bank4)
	{
		case 	1:
		case 	2:
		case 	3:
						memory_set_bankptr(machine, 4, messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000*(bank4-1) + 0x0c000);
						memory_set_bankptr(machine, 5, messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000*(bank4-1) + 0x0f000);
						memory_set_bankptr(machine, 6, messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000*(bank4-1) + 0x0f800);
						break;
		case 	0:
						memory_install_write8_handler(space, 0xc000, 0xefff, 0, 0, SMH_UNMAP);
						memory_set_bankptr(machine, 4, mem + 0x1c000);
						memory_install_write8_handler(space, 0xf000, 0xf7ff, 0, 0, SMH_UNMAP);
						memory_install_read8_handler(space, 0xf000, 0xf7ff, 0, 0, SMH_NOP);
						memory_install_write8_handler(space, 0xf800, 0xffff, 0, 0, specialist_keyboard_w);
						memory_install_read8_handler(space, 0xf800, 0xffff, 0, 0, specialist_keyboard_r);
						break;
	}
}

DRIVER_INIT(erik)
{
	memset(messram_get_ptr(devtag_get_device(machine, "messram")),0,192*1024);
	erik_color_1 = 0;
	erik_color_2 = 0;
	erik_background = 0;
}

MACHINE_RESET( erik )
{
	RR_register = 0x00;
	RC_register = 0x00;
	erik_set_bank(machine);
}

READ8_HANDLER ( erik_rr_reg_r )
{
	return RR_register;
}
WRITE8_HANDLER( erik_rr_reg_w )
{
	RR_register = data;
	erik_set_bank(space->machine);
}

READ8_HANDLER ( erik_rc_reg_r )
{
	return RC_register;
}


WRITE8_HANDLER( erik_rc_reg_w )
{
	RC_register = data;
	erik_color_1 =  RC_register & 7;
	erik_color_2 =  (RC_register >> 3) & 7;
	erik_background = ((RC_register  >> 6 ) & 1) + ((RC_register  >> 7 ) & 1) * 4;
}

READ8_HANDLER ( erik_disk_reg_r )
{
	return 0xff;
}

WRITE8_HANDLER( erik_disk_reg_w )
{
	const device_config *fdc = devtag_get_device(space->machine, "wd1793");

	wd17xx_set_side (fdc,data & 1);
	wd17xx_set_drive(fdc,(data >> 1) & 1);
	if((data >>2) & 1)
	{
		wd17xx_set_density (fdc,DEN_FM_LO);
	}
	else
	{
		wd17xx_set_density (fdc,DEN_FM_HI);
  }
}
