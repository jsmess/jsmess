#include "driver.h"
#include "includes/spectrum.h"
#include "eventlst.h"
#include "devices/snapquik.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "sound/ay8910.h"
#include "sound/speaker.h"
#include "formats/tzx_cas.h"
#include "devices/basicdsk.h"
#include "machine/wd17xx.h"
#include "machine/beta.h"


static int ROMSelection;

static DIRECT_UPDATE_HANDLER( pentagon_direct )
{	
	UINT16 pc = cpu_get_reg(space->machine->cpu[0], REG_PREVIOUSPC);
	if (betadisk_is_active()) {
		if (pc >= 0x4000) {
			ROMSelection = ((spectrum_128_port_7ffd_data>>4) & 0x01) ? 1 : 0;
			betadisk_disable();
			memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_UNMAP);
			memory_set_bankptr(space->machine, 1, memory_region(space->machine, "main") + 0x010000 + (ROMSelection<<14));
		} 	
	} else if (((pc & 0xff00) == 0x3d00) && (ROMSelection==1))
	{
		ROMSelection = 3;
		betadisk_enable();
		
	} 
	if((address>=0x0000) && (address<=0x3fff)) {
		memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_UNMAP);
		direct->raw = direct->decrypted =  memory_region(space->machine, "main") + 0x010000 + (ROMSelection<<14);
		memory_set_bankptr(space->machine, 1, direct->raw);
		return ~0;
	}
	return address;
}

static void pentagon_update_memory(running_machine *machine)
{	
	spectrum_128_screen_location = mess_ram + ((spectrum_128_port_7ffd_data & 8) ? (7<<14) : (5<<14));

	memory_set_bankptr(machine, 4, mess_ram + ((spectrum_128_port_7ffd_data & 0x07) * 0x4000));

	if( betadisk_is_active() && !( spectrum_128_port_7ffd_data & 0x10 ) ) {    
		/* GLUK */
		ROMSelection = 2;
	}
	else
	{
		/* ROM switching */
		ROMSelection = ((spectrum_128_port_7ffd_data>>4) & 0x01) ;
	}
	/* rom 0 is 128K rom, rom 1 is 48 BASIC */
	memory_set_bankptr(machine, 1, memory_region(machine, "main") + 0x010000 + (ROMSelection<<14));
}

static WRITE8_HANDLER(pentagon_port_7ffd_w)
{
	/* disable paging */
	if (spectrum_128_port_7ffd_data & 0x20)
		return;

	/* store new state */
	spectrum_128_port_7ffd_data = data;

	/* update memory */
	pentagon_update_memory(space->machine);
}

static ADDRESS_MAP_START (pentagon_io, ADDRESS_SPACE_IO, 8)	
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x001f, 0x001f) AM_READWRITE(betadisk_status_r,betadisk_command_w) AM_MIRROR(0xff00)
	AM_RANGE(0x003f, 0x003f) AM_READWRITE(betadisk_track_r,betadisk_track_w) AM_MIRROR(0xff00)
	AM_RANGE(0x005f, 0x005f) AM_READWRITE(betadisk_sector_r,betadisk_sector_w) AM_MIRROR(0xff00)
	AM_RANGE(0x007f, 0x007f) AM_READWRITE(betadisk_data_r,betadisk_data_w) AM_MIRROR(0xff00)
	AM_RANGE(0x00fe, 0x00fe) AM_READWRITE(spectrum_port_fe_r,spectrum_port_fe_w) AM_MIRROR(0xff00) AM_MASK(0xffff) 
	AM_RANGE(0x00ff, 0x00ff) AM_READWRITE(betadisk_state_r, betadisk_param_w) AM_MIRROR(0xff00)
	AM_RANGE(0x4000, 0x4000) AM_WRITE(pentagon_port_7ffd_w)  AM_MIRROR(0x3ffd)
	AM_RANGE(0x8000, 0x8000) AM_WRITE(ay8910_write_port_0_w) AM_MIRROR(0x3ffd)
	AM_RANGE(0xc000, 0xc000) AM_READWRITE(ay8910_read_port_0_r,ay8910_control_port_0_w) AM_MIRROR(0x3ffd)
ADDRESS_MAP_END

static MACHINE_RESET( pentagon )
{
	const address_space *space = cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM);

	memory_install_read8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_BANK1);
	memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_UNMAP);

	betadisk_enable();
	betadisk_clear_status();

	memory_set_direct_update_handler( space, pentagon_direct ); 
	
	memset(mess_ram,0,128*1024);
	
	/* Bank 5 is always in 0x4000 - 0x7fff */
	memory_set_bankptr(machine, 2, mess_ram + (5<<14));

	/* Bank 2 is always in 0x8000 - 0xbfff */
	memory_set_bankptr(machine, 3, mess_ram + (2<<14));

	spectrum_128_port_7ffd_data = 0;

	pentagon_update_memory(machine);	
}

static MACHINE_DRIVER_START( pentagon )
	MDRV_IMPORT_FROM( spectrum_128 )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(pentagon_io, 0)
	MDRV_MACHINE_RESET( pentagon )
		
	MDRV_WD179X_ADD("wd179x", beta_wd17xx_interface )
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(pentagon)
	ROM_REGION(0x020000, "main", 0)
	ROM_LOAD("128p-0.rom", 0x010000, 0x4000, CRC(124ad9e0) SHA1(d07fcdeca892ee80494d286ea9ea5bf3928a1aca) )
	ROM_LOAD("128p-1.rom", 0x014000, 0x4000, CRC(b96a36be) SHA1(80080644289ed93d71a1103992a154cc9802b2fa) )
	ROM_LOAD("gluck.rom",  0x018000, 0x4000, CRC(ca321d79) SHA1(015eb96dafb273d4f4512c467e9b43c305fd1bc4) )
	ROM_LOAD("trdos.rom",  0x01c000, 0x4000, CRC(10751aba) SHA1(21695e3f2a8f796386ce66eea8a246b0ac44810c) )	
	ROM_CART_LOAD(0, "rom", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

SYSTEM_CONFIG_EXTERN(spectrum)

static SYSTEM_CONFIG_START(pentagon)
	CONFIG_IMPORT_FROM(spectrum)
	CONFIG_RAM_DEFAULT(128 * 1024)
	CONFIG_DEVICE(beta_floppy_getinfo)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT    COMPAT  MACHINE     INPUT       INIT    CONFIG      COMPANY     FULLNAME */
COMP( ????, pentagon, spec128,	0,		pentagon,	spectrum,	0,		pentagon,	"???",		"Pentagon", GAME_NOT_WORKING)
