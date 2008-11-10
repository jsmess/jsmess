/***************************************************************************

        Pecom driver by Miodrag Milanovic

        08/11/2008 Preliminary driver.

****************************************************************************/

#include "driver.h"
#include "cpu/cdp1802/cdp1802.h"
#include "devices/cassette.h"
#include "includes/pecom.h"
 
  
/* Driver initialization */
DRIVER_INIT(pecom)
{
	memset(mess_ram,0,32*1024);
}

MACHINE_START( pecom )
{
}

MACHINE_RESET( pecom )
{
	cpunum_set_input_line(machine, 0, INPUT_LINE_RESET, PULSE_LINE);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, SMH_UNMAP);
	memory_set_bankptr(1, memory_region(machine, "main") + 0x8000);	
	memory_set_bankptr(2, mess_ram + 0x4000);
}

WRITE8_HANDLER( pecom_bank_w )
{
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, SMH_BANK1);
	memory_set_bankptr(1, mess_ram + 0x0000);
}

UINT8 key_val = 0;
READ8_HANDLER (pecom_keyboard_r)
{
	static const char *keynames[] = { 	"LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7",
										"LINE8", "LINE9", "LINE10", "LINE11", "LINE12", "LINE13", "LINE14", "LINE15", "LINE16",
										"LINE17", "LINE18", "LINE19", "LINE20", "LINE21", "LINE22", "LINE23", "LINE24","LINE25" };
	UINT8 reg = cpunum_get_reg(0,CDP1802_D);
	switch(reg) {
		case 0xca : key_val = 0;break;
		case 0x29 : key_val = 1;break;
		case 0x28 : key_val = 2;break;
		case 0x2f : key_val = 3;break;
		case 0x2e : key_val = 4;break;
		case 0x2d : key_val = 5;break;
		case 0x2c : key_val = 6;break;
		case 0x33 : key_val = 7;break;
		case 0x32 : key_val = 8;break;
		case 0x31 : key_val = 9;break;
		case 0x30 : key_val = 10;break;
		case 0x37 : key_val = 11;break;
		case 0x36 : key_val = 12;break;
		case 0x35 : key_val = 13;break;
		case 0x34 : key_val = 14;break;
		case 0x3b : key_val = 15;break;
		case 0x3a : key_val = 16;break;
		case 0x39 : key_val = 17;break;
		case 0x38 : key_val = 18;break;
		case 0x3f : key_val = 19;break;
		case 0x3e : key_val = 20;break;
		case 0x3d : key_val = 21;break;
		case 0x3c : key_val = 22;break;
		case 0x03 : key_val = 23;break;
		case 0x02 : key_val = 24;break;
		case 0x01 : key_val = 25;break;		
	}			
	return input_port_read(machine, keynames[key_val]) & 0x03;
}

/* CDP1802 Interface */
static CDP1802_MODE_READ( pecom64_mode_r )
{
	return CDP1802_MODE_RUN;
}

static const device_config *cassette_device_image(running_machine *machine)
{
	return devtag_get_device(machine, CASSETTE, "cassette");
}

static CDP1802_EF_READ( pecom64_ef_r )
{
	int flags = 0x0f;
	flags -= input_port_read(machine, "CNT");

	if (cassette_input(cassette_device_image(machine)) > +0.5) flags -= EF2;
	
	return flags;
}

static CDP1802_Q_WRITE( pecom64_q_w )
{	
	cassette_output(cassette_device_image(machine), level ? +1.0 : -1.0);
}

CDP1802_INTERFACE( pecom64_cdp1802_config )
{
	pecom64_mode_r,
	pecom64_ef_r,
	NULL,
	pecom64_q_w,
	NULL,
	NULL
};


