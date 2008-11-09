/***************************************************************************

        Pecom driver by Miodrag Milanovic

        08/11/2008 Preliminary driver.

****************************************************************************/

#include "driver.h"
#include "cpu/cdp1802/cdp1802.h"
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

READ8_HANDLER (pecom_keyboard_r)
{
	return input_port_read(machine, "LINE0");
}

/* CDP1802 Interface */
static CDP1802_MODE_READ( pecom64_mode_r )
{
	return CDP1802_MODE_RUN;
}

static CDP1802_EF_READ( pecom64_ef_r )
{
	int flags = 0x0f;
	return flags;
}

static CDP1802_SC_WRITE( pecom64_sc_w )
{
	//logerror("CDP1802_SC_WRITE %02x\n",state);
}

static CDP1802_Q_WRITE( pecom64_q_w )
{
	//logerror("CDP1802_Q_WRITE %d\n",level);
}

CDP1802_INTERFACE( pecom64_cdp1802_config )
{
	pecom64_mode_r,
	pecom64_ef_r,
	pecom64_sc_w,
	pecom64_q_w,
	NULL,
	NULL
};


