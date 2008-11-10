/***************************************************************************

        Pecom driver by Miodrag Milanovic

        08/11/2008 Preliminary driver.

****************************************************************************/

#include "driver.h"
#include "cpu/cdp1802/cdp1802.h"
#include "devices/cassette.h"
#include "includes/pecom.h"
 
UINT8 pecom_key_val = 0;
UINT8 pecom_key_cnt = 0;

  
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
	pecom_key_val = 0;
	pecom_key_cnt = 0;
}

WRITE8_HANDLER( pecom_bank_w )
{
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, SMH_BANK1);
	memory_set_bankptr(1, mess_ram + 0x0000);
}

READ8_HANDLER (pecom_keyboard_r)
{
	static const char *keynames[] = { 	"LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7",
										"LINE8", "LINE9", "LINE10", "LINE11", "LINE12", "LINE13", "LINE14", "LINE15", "LINE16",
										"LINE17", "LINE18", "LINE19", "LINE20", "LINE21", "LINE22", "LINE23", "LINE24","LINE25" };
	UINT8 val = input_port_read(machine, keynames[pecom_key_cnt]) & 0x03;
	if (val==0) {
		if (pecom_key_val!=0) {
			pecom_key_cnt = 0;
		} else {
			pecom_key_cnt++;
		}
		if (pecom_key_cnt==26) {
			pecom_key_cnt = 0;
		}
	} if ((pecom_key_val!=val) && (pecom_key_val!=0)) {
		val = 0;
		pecom_key_cnt = 0;
	}
	pecom_key_val=val;
	return val;	
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
	if (cassette_get_state(cassette_device_image(machine)) & CASSETTE_PLAY) {
		double val = cassette_input(cassette_device_image(machine));
		//logerror("%f\n",val);
		if ( val > +0.42) flags -= EF2;
	} else {
		flags -= input_port_read(machine, "CNT");
	}
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


