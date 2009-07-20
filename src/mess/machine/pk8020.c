/***************************************************************************

        PK-8020 driver by Miodrag Milanovic

        18/07/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "includes/pk8020.h"

static UINT8 vpage = 0;
static UINT8 attr = 0;
static UINT8 takt = 0;
UINT8 pk8020_color = 0;

static void pk8020_set_bank(running_machine *machine,UINT8 data);

/* Driver initialization */
DRIVER_INIT(pk8020)
{
	memset(mess_ram,0,129*1024);
}

static READ8_HANDLER(keyboard_r)
{
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7", 
											"LINE8", "LINE9", "LINE10", "LINE11", "LINE12", "LINE13", "LINE14", "LINE15" };
	UINT8 retVal=0x00;	
	UINT8 line = 0;
	if (offset & 0x100)  line=8;
		
	if (offset & 0x0001) retVal|=input_port_read(space->machine,keynames[line]);
	line++;
	if (offset & 0x0002) retVal|=input_port_read(space->machine,keynames[line]);
	line++;
	if (offset & 0x0004) retVal|=input_port_read(space->machine,keynames[line]);
	line++;
	if (offset & 0x0008) retVal|=input_port_read(space->machine,keynames[line]);
	line++;
	if (offset & 0x0010) retVal|=input_port_read(space->machine,keynames[line]);
	line++;
	if (offset & 0x0020) retVal|=input_port_read(space->machine,keynames[line]);
	line++;
	if (offset & 0x0040) retVal|=input_port_read(space->machine,keynames[line]);
	line++;
	if (offset & 0x0080) retVal|=input_port_read(space->machine,keynames[line]);
	line++;
	return retVal;
}

static READ8_HANDLER(sysreg_r)
{
	logerror("sysreg_r %02x\n",offset);
	return 0xff;
}
static WRITE8_HANDLER(sysreg_w)
{
	logerror("sysreg_w %02x %02x\n",offset,data);
	if (BIT(offset,7)==0) {
		pk8020_set_bank(space->machine,data >> 2);
	} else if (BIT(offset,6)==0) {
		// Color
		pk8020_color = data;
	} else if (BIT(offset,2)==0) {
		// Palette set
		UINT8 number = data & 0x0f;
		UINT8 color = data >> 4;
		UINT8 i = (color & 0x08) ?  0x3F : 0;
		UINT8 r = ((color & 0x04) ? 0xC0 : 0) + i;
		UINT8 g = ((color & 0x02) ? 0xC0 : 0) + i;
		UINT8 b = ((color & 0x01) ? 0xC0 : 0) + i;					
		palette_set_color( space->machine, number, MAKE_RGB(r,g,b) );
	}
}

static READ8_HANDLER(text_r)
{
	return mess_ram[0x20000+offset];	
}

static WRITE8_HANDLER(text_w)
{
	mess_ram[0x20000+offset] = data;
}

static READ8_HANDLER(devices_r)
{
	const device_config *ppi1 = devtag_get_device(space->machine, "ppi8255_1");
	const device_config *ppi2 = devtag_get_device(space->machine, "ppi8255_2");
	const device_config *ppi3 = devtag_get_device(space->machine, "ppi8255_3");
	const device_config *pit = devtag_get_device(space->machine, "pit8253");
	const device_config *pic = devtag_get_device(space->machine, "pic8259");
	const device_config *rs232 = devtag_get_device(space->machine, "rs232");
	const device_config *lan = devtag_get_device(space->machine, "lan");
	const device_config *fdc = devtag_get_device(space->machine, "wd1793");	
	switch(offset & 0x38)
	{
		case 0x00: 
		case 0x18: 
		case 0x20: 
		case 0x28: 
		case 0x38: 
			//logerror("devices_r %02x %04x\n",offset,cpu_get_pc(space->cpu));
			break;
	}
	
	switch(offset & 0x38)
	{
		case 0x00: return pit8253_r(pit,offset & 3);
		case 0x08: return i8255a_r(ppi3,offset & 3);
		case 0x10: switch(offset & 1) {
						case 0 : return msm8251_data_r(rs232,0);
				   		case 1 : return msm8251_status_r(rs232,0);
				   }
				   break;		
		case 0x18: switch(offset & 3) {
						case 0 : return wd17xx_status_r(fdc,0);
						case 1 : return wd17xx_track_r(fdc,0);
						case 2 : return wd17xx_sector_r(fdc,0);
						case 3 : return wd17xx_data_r(fdc,0);
					} 
					break;
		case 0x20: switch(offset & 1) {
						case 0 : return msm8251_data_r(lan,0);
				   		case 1 : return msm8251_status_r(lan,0);
				   }
				   break;		
		case 0x28: return pic8259_r(pic,offset & 1);
		case 0x30: return i8255a_r(ppi2,offset & 3);
		case 0x38: return i8255a_r(ppi1,offset & 3);
	}
	return 0xff;
}

static WRITE8_HANDLER(devices_w)
{
	const device_config *ppi1 = devtag_get_device(space->machine, "ppi8255_1");
	const device_config *ppi2 = devtag_get_device(space->machine, "ppi8255_2");
	const device_config *ppi3 = devtag_get_device(space->machine, "ppi8255_3");
	const device_config *pit = devtag_get_device(space->machine, "pit8253");
	const device_config *pic = devtag_get_device(space->machine, "pic8259");
	const device_config *rs232 = devtag_get_device(space->machine, "rs232");
	const device_config *lan = devtag_get_device(space->machine, "lan");
	const device_config *fdc = devtag_get_device(space->machine, "wd1793");
	switch(offset & 0x38)
	{
		case 0x00: 
		case 0x18: 
		case 0x20: 
		case 0x28: 
		case 0x38: 
			//logerror("devices_w %02x %02x %04x\n",offset,data,cpu_get_pc(space->cpu));
			break;
	}
	
	switch(offset & 0x38)
	{
		case 0x00: pit8253_w(pit,offset & 3,data); break;
		case 0x08: i8255a_w(ppi3,offset & 3,data); break;
		case 0x10: switch(offset & 1) {
						case 0 : msm8251_data_w(rs232,0,data); break;
				   		case 1 : msm8251_control_w(rs232,0,data); break;
				   }
				   break;			
		case 0x18: switch(offset & 3) {
						case 0 : wd17xx_command_w(fdc,0,data);
						case 1 : wd17xx_track_w(fdc,0,data);
						case 2 : wd17xx_sector_w(fdc,0,data);
						case 3 : wd17xx_data_w(fdc,0,data);
					} 
					break;				   	
		case 0x20: switch(offset & 1) {
						case 0 : msm8251_data_w(lan,0,data); break;
				   		case 1 : msm8251_control_w(lan,0,data); break;
				   }
				   break;			
		case 0x28: pic8259_w(pic,offset & 1,data);break;
		case 0x30: i8255a_w(ppi2,offset & 3,data); break;
		case 0x38: i8255a_w(ppi1,offset & 3,data); break;
	}
}

static void pk8020_set_bank(running_machine *machine,UINT8 data) 
{ 
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);	
	
	switch(data & 0x1F) {
		case 0x00 : 
					{
						// ROM
						memory_install_read8_handler (space, 0x0000, 0x37ff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0x37ff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);
						// Keyboard
						memory_install_read8_handler (space, 0x3800, 0x39ff, 0, 0, keyboard_r);
						memory_install_write8_handler(space, 0x3800, 0x39ff, 0, 0, SMH_BANK(3));
						memory_set_bankptr(machine, 3, mess_ram + 0x3800);
						// System reg
						memory_install_read8_handler (space, 0x3a00, 0x3aff, 0, 0, sysreg_r);
						memory_install_write8_handler(space, 0x3a00, 0x3aff, 0, 0, sysreg_w);
						// Devices
						memory_install_read8_handler (space, 0x3b00, 0x3bff, 0, 0, devices_r);
						memory_install_write8_handler(space, 0x3b00, 0x3bff, 0, 0, devices_w);
						// Text Video Memory
						memory_install_read8_handler (space, 0x3c00, 0x3fff, 0, 0, text_r);
						memory_install_write8_handler(space, 0x3c00, 0x3fff, 0, 0, text_w);
						// RAM
						memory_install_read8_handler (space, 0x4000, 0xffff, 0, 0, SMH_BANK(4));
						memory_install_write8_handler(space, 0x4000, 0xffff, 0, 0, SMH_BANK(5));
						memory_set_bankptr(machine, 4, mess_ram + 0x4000);
						memory_set_bankptr(machine, 5, mess_ram + 0x4000);
					}
					break;
		case 0x01 : {
						// ROM
						memory_install_read8_handler (space, 0x0000, 0x1fff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0x1fff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);
						// RAM
						memory_install_read8_handler (space, 0x2000, 0xffff, 0, 0, SMH_BANK(3));
						memory_install_write8_handler(space, 0x2000, 0xffff, 0, 0, SMH_BANK(4));
						memory_set_bankptr(machine, 3, mess_ram + 0x2000);
						memory_set_bankptr(machine, 4, mess_ram + 0x2000);
					}					
					break;
		case 0x02 : {
						// ROM
						memory_install_read8_handler (space, 0x0000, 0x3fff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);
						// RAM
						memory_install_read8_handler (space, 0x4000, 0xffff, 0, 0, SMH_BANK(3));
						memory_install_write8_handler(space, 0x4000, 0xffff, 0, 0, SMH_BANK(4));
						memory_set_bankptr(machine, 3, mess_ram + 0x4000);
						memory_set_bankptr(machine, 4, mess_ram + 0x4000);
					}					
					break;
		case 0x03 : {
						// RAM
						memory_install_read8_handler (space, 0x0000, 0xffff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0xffff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, mess_ram);
						memory_set_bankptr(machine, 2, mess_ram);
					}					
					break;
		case 0x04 : 
		case 0x05 : 
					{
						// ROM
						memory_install_read8_handler (space, 0x0000, 0x1fff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0x1fff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);
						// RAM
						memory_install_read8_handler (space, 0x2000, 0xf7ff, 0, 0, SMH_BANK(3));
						memory_install_write8_handler(space, 0x2000, 0xf7ff, 0, 0, SMH_BANK(4));
						memory_set_bankptr(machine, 3, mess_ram + 0x2000);
						memory_set_bankptr(machine, 4, mess_ram + 0x2000);
						// Keyboard
						memory_install_read8_handler (space, 0xf800, 0xf9ff, 0, 0, keyboard_r);
						memory_install_write8_handler(space, 0xf800, 0xf9ff, 0, 0, SMH_BANK(5));
						memory_set_bankptr(machine, 5, mess_ram + 0xf800);
						// System reg
						memory_install_read8_handler (space, 0xfa00, 0xfaff, 0, 0, sysreg_r);
						memory_install_write8_handler(space, 0xfa00, 0xfaff, 0, 0, sysreg_w);
						// Devices
						memory_install_read8_handler (space, 0xfb00, 0xfbff, 0, 0, devices_r);
						memory_install_write8_handler(space, 0xfb00, 0xfbff, 0, 0, devices_w);
						// Text Video Memory
						memory_install_read8_handler (space, 0xfc00, 0xffff, 0, 0, text_r);
						memory_install_write8_handler(space, 0xfc00, 0xffff, 0, 0, text_w);
					}
					break;
		case 0x06 : 
					{
						// ROM
						memory_install_read8_handler (space, 0x0000, 0x3fff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);
						// RAM
						memory_install_read8_handler (space, 0x4000, 0xf7ff, 0, 0, SMH_BANK(3));
						memory_install_write8_handler(space, 0x4000, 0xf7ff, 0, 0, SMH_BANK(4));
						memory_set_bankptr(machine, 3, mess_ram + 0x4000);
						memory_set_bankptr(machine, 4, mess_ram + 0x4000);
						// Keyboard
						memory_install_read8_handler (space, 0xf800, 0xf9ff, 0, 0, keyboard_r);
						memory_install_write8_handler(space, 0xf800, 0xf9ff, 0, 0, SMH_BANK(5));
						memory_set_bankptr(machine, 5, mess_ram + 0xf800);
						// System reg
						memory_install_read8_handler (space, 0xfa00, 0xfaff, 0, 0, sysreg_r);
						memory_install_write8_handler(space, 0xfa00, 0xfaff, 0, 0, sysreg_w);
						// Devices
						memory_install_read8_handler (space, 0xfb00, 0xfbff, 0, 0, devices_r);
						memory_install_write8_handler(space, 0xfb00, 0xfbff, 0, 0, devices_w);
						// Text Video Memory
						memory_install_read8_handler (space, 0xfc00, 0xffff, 0, 0, text_r);
						memory_install_write8_handler(space, 0xfc00, 0xffff, 0, 0, text_w);
					}
					break;	
		case 0x07 : 
					{
						// RAM
						memory_install_read8_handler (space, 0x0000, 0xf7ff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0xf7ff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, mess_ram + 0x4000);
						memory_set_bankptr(machine, 2, mess_ram + 0x4000);
						// Keyboard
						memory_install_read8_handler (space, 0xf800, 0xf9ff, 0, 0, keyboard_r);
						memory_install_write8_handler(space, 0xf800, 0xf9ff, 0, 0, SMH_BANK(3));
						memory_set_bankptr(machine, 3, mess_ram + 0xf800);
						// System reg
						memory_install_read8_handler (space, 0xfa00, 0xfaff, 0, 0, sysreg_r);
						memory_install_write8_handler(space, 0xfa00, 0xfaff, 0, 0, sysreg_w);
						// Devices
						memory_install_read8_handler (space, 0xfb00, 0xfbff, 0, 0, devices_r);
						memory_install_write8_handler(space, 0xfb00, 0xfbff, 0, 0, devices_w);
						// Text Video Memory
						memory_install_read8_handler (space, 0xfc00, 0xffff, 0, 0, text_r);
						memory_install_write8_handler(space, 0xfc00, 0xffff, 0, 0, text_w);
					}
					break;	
		case 0x08 : 
					{
						// ROM
						memory_install_read8_handler (space, 0x0000, 0x3fff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);
						// Keyboard
						memory_install_read8_handler (space, 0x3800, 0x39ff, 0, 0, keyboard_r);
						memory_install_write8_handler(space, 0x3800, 0x39ff, 0, 0, SMH_BANK(3));
						memory_set_bankptr(machine, 3, mess_ram + 0x3800);
						// System reg
						memory_install_read8_handler (space, 0x3a00, 0x3aff, 0, 0, sysreg_r);
						memory_install_write8_handler(space, 0x3a00, 0x3aff, 0, 0, sysreg_w);
						// Devices
						memory_install_read8_handler (space, 0x3b00, 0x3bff, 0, 0, devices_r);
						memory_install_write8_handler(space, 0x3b00, 0x3bff, 0, 0, devices_w);
						// Text Video Memory
						memory_install_read8_handler (space, 0x3c00, 0x3fff, 0, 0, text_r);
						memory_install_write8_handler(space, 0x3c00, 0x3fff, 0, 0, text_w);
						// RAM
						memory_install_read8_handler (space, 0x4000, 0xbfff, 0, 0, SMH_BANK(4));
						memory_install_write8_handler(space, 0x4000, 0xbfff, 0, 0, SMH_BANK(5));
						memory_set_bankptr(machine, 4, mess_ram + 0x4000);
						memory_set_bankptr(machine, 5, mess_ram + 0x4000);
						// Video RAM
						memory_install_read8_handler (space, 0xc000, 0xffff, 0, 0, SMH_BANK(6));
						memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, SMH_BANK(7));
						memory_set_bankptr(machine, 6, mess_ram + 0x10000 + vpage * 0x4000);
						memory_set_bankptr(machine, 7, mess_ram + 0x10000 + vpage * 0x4000);
						
					}
					break;
		case 0x09 : 
					{
						// ROM
						memory_install_read8_handler (space, 0x0000, 0x1fff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0x1fff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);
						// RAM
						memory_install_read8_handler (space, 0x2000, 0xbfff, 0, 0, SMH_BANK(3));
						memory_install_write8_handler(space, 0x2000, 0xbfff, 0, 0, SMH_BANK(4));
						memory_set_bankptr(machine, 3, mess_ram + 0x2000);
						memory_set_bankptr(machine, 4, mess_ram + 0x2000);
						// Video RAM
						memory_install_read8_handler (space, 0xc000, 0xffff, 0, 0, SMH_BANK(5));
						memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, SMH_BANK(6));
						memory_set_bankptr(machine, 5, mess_ram + 0x10000 + vpage * 0x4000);
						memory_set_bankptr(machine, 6, mess_ram + 0x10000 + vpage * 0x4000);
						
					}
					break;
		case 0x0A : 
					{
						// ROM
						memory_install_read8_handler (space, 0x0000, 0x3fff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);
						// RAM
						memory_install_read8_handler (space, 0x4000, 0xbfff, 0, 0, SMH_BANK(3));
						memory_install_write8_handler(space, 0x4000, 0xbfff, 0, 0, SMH_BANK(4));
						memory_set_bankptr(machine, 3, mess_ram + 0x4000);
						memory_set_bankptr(machine, 4, mess_ram + 0x4000);
						// Video RAM
						memory_install_read8_handler (space, 0xc000, 0xffff, 0, 0, SMH_BANK(5));
						memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, SMH_BANK(6));
						memory_set_bankptr(machine, 5, mess_ram + 0x10000 + vpage * 0x4000);
						memory_set_bankptr(machine, 6, mess_ram + 0x10000 + vpage * 0x4000);
						
					}
					break;
		case 0x0B : 
					{
						// RAM
						memory_install_read8_handler (space, 0x0000, 0xbfff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0xbfff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, mess_ram + 0x0000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);
						// Video RAM
						memory_install_read8_handler (space, 0xc000, 0xffff, 0, 0, SMH_BANK(3));
						memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, SMH_BANK(4));
						memory_set_bankptr(machine, 3, mess_ram + 0x10000 + vpage * 0x4000);
						memory_set_bankptr(machine, 4, mess_ram + 0x10000 + vpage * 0x4000);
						
					}
					break;
		case 0x0C : 
		case 0x0D : 
					{
						// ROM
						memory_install_read8_handler (space, 0x0000, 0x1fff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0x1fff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);
						// RAM
						memory_install_read8_handler (space, 0x2000, 0x3fff, 0, 0, SMH_BANK(3));
						memory_install_write8_handler(space, 0x2000, 0x3fff, 0, 0, SMH_BANK(4));
						memory_set_bankptr(machine, 3, mess_ram + 0x2000);
						memory_set_bankptr(machine, 4, mess_ram + 0x2000);
						// Video RAM
						memory_install_read8_handler (space, 0x4000, 0x7fff, 0, 0, SMH_BANK(5));
						memory_install_write8_handler(space, 0x4000, 0x7fff, 0, 0, SMH_BANK(6));
						memory_set_bankptr(machine, 5, mess_ram + 0x10000 + vpage * 0x4000);
						memory_set_bankptr(machine, 6, mess_ram + 0x10000 + vpage * 0x4000);
						// RAM
						memory_install_read8_handler (space, 0x8000, 0xfdff, 0, 0, SMH_BANK(7));
						memory_install_write8_handler(space, 0x8000, 0xfdff, 0, 0, SMH_BANK(8));
						memory_set_bankptr(machine, 7, mess_ram + 0x8000);
						memory_set_bankptr(machine, 8, mess_ram + 0x8000);
						// Devices
						memory_install_read8_handler (space, 0xfe00, 0xfeff, 0, 0, devices_r);
						memory_install_write8_handler(space, 0xfe00, 0xfeff, 0, 0, devices_w);
						// System reg
						memory_install_read8_handler (space, 0xff00, 0xffff, 0, 0, sysreg_r);
						memory_install_write8_handler(space, 0xff00, 0xffff, 0, 0, sysreg_w);						
					}
					break;
		case 0x0E : 
					{
						// ROM
						memory_install_read8_handler (space, 0x0000, 0x3fff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);
						// Video RAM
						memory_install_read8_handler (space, 0x4000, 0x7fff, 0, 0, SMH_BANK(3));
						memory_install_write8_handler(space, 0x4000, 0x7fff, 0, 0, SMH_BANK(4));
						memory_set_bankptr(machine, 3, mess_ram + 0x10000 + vpage * 0x4000);
						memory_set_bankptr(machine, 4, mess_ram + 0x10000 + vpage * 0x4000);
						// RAM
						memory_install_read8_handler (space, 0x8000, 0xfdff, 0, 0, SMH_BANK(5));
						memory_install_write8_handler(space, 0x8000, 0xfdff, 0, 0, SMH_BANK(6));
						memory_set_bankptr(machine, 5, mess_ram + 0x8000);
						memory_set_bankptr(machine, 6, mess_ram + 0x8000);
						// Devices
						memory_install_read8_handler (space, 0xfe00, 0xfeff, 0, 0, devices_r);
						memory_install_write8_handler(space, 0xfe00, 0xfeff, 0, 0, devices_w);
						// System reg
						memory_install_read8_handler (space, 0xff00, 0xffff, 0, 0, sysreg_r);
						memory_install_write8_handler(space, 0xff00, 0xffff, 0, 0, sysreg_w);						
					}
					break;
		case 0x0F : 
					{
						// RAM
						memory_install_read8_handler (space, 0x0000, 0x3fff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, mess_ram + 0x0000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);
						// Video RAM
						memory_install_read8_handler (space, 0x4000, 0x7fff, 0, 0, SMH_BANK(3));
						memory_install_write8_handler(space, 0x4000, 0x7fff, 0, 0, SMH_BANK(4));
						memory_set_bankptr(machine, 3, mess_ram + 0x10000 + vpage * 0x4000);
						memory_set_bankptr(machine, 4, mess_ram + 0x10000 + vpage * 0x4000);
						// RAM
						memory_install_read8_handler (space, 0x8000, 0xfdff, 0, 0, SMH_BANK(5));
						memory_install_write8_handler(space, 0x8000, 0xfdff, 0, 0, SMH_BANK(6));
						memory_set_bankptr(machine, 5, mess_ram + 0x8000);
						memory_set_bankptr(machine, 6, mess_ram + 0x8000);
						// Devices
						memory_install_read8_handler (space, 0xfe00, 0xfeff, 0, 0, devices_r);
						memory_install_write8_handler(space, 0xfe00, 0xfeff, 0, 0, devices_w);
						// System reg
						memory_install_read8_handler (space, 0xff00, 0xffff, 0, 0, sysreg_r);
						memory_install_write8_handler(space, 0xff00, 0xffff, 0, 0, sysreg_w);						
					}
					break;					
		case 0x10 : 
					{
						// ROM
						memory_install_read8_handler (space, 0x0000, 0x5fff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0x5fff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);
						// RAM
						memory_install_read8_handler (space, 0x6000, 0xf7ff, 0, 0, SMH_BANK(3));
						memory_install_write8_handler(space, 0x6000, 0xf7ff, 0, 0, SMH_BANK(4));
						memory_set_bankptr(machine, 3, mess_ram + 0x6000);
						memory_set_bankptr(machine, 4, mess_ram + 0x6000);						
						// Keyboard
						memory_install_read8_handler (space, 0xf800, 0xf9ff, 0, 0, keyboard_r);
						memory_install_write8_handler(space, 0xf800, 0xf9ff, 0, 0, SMH_BANK(5));
						memory_set_bankptr(machine, 5, mess_ram + 0xf800);
						// System reg
						memory_install_read8_handler (space, 0xfa00, 0xfaff, 0, 0, sysreg_r);
						memory_install_write8_handler(space, 0xfa00, 0xfaff, 0, 0, sysreg_w);
						// Devices
						memory_install_read8_handler (space, 0xfb00, 0xfbff, 0, 0, devices_r);
						memory_install_write8_handler(space, 0xfb00, 0xfbff, 0, 0, devices_w);
						// Text Video Memory
						memory_install_read8_handler (space, 0xfc00, 0xffff, 0, 0, text_r);
						memory_install_write8_handler(space, 0xfc00, 0xffff, 0, 0, text_w);
					}
					break;					
		case 0x11 : 
					{
						// ROM
						memory_install_read8_handler (space, 0x0000, 0x1fff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0x1fff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);
						// RAM
						memory_install_read8_handler (space, 0x2000, 0xf7ff, 0, 0, SMH_BANK(3));
						memory_install_write8_handler(space, 0x2000, 0xf7ff, 0, 0, SMH_BANK(4));
						memory_set_bankptr(machine, 3, mess_ram + 0x2000);
						memory_set_bankptr(machine, 4, mess_ram + 0x2000);						
						// Keyboard
						memory_install_read8_handler (space, 0xf800, 0xf9ff, 0, 0, keyboard_r);
						memory_install_write8_handler(space, 0xf800, 0xf9ff, 0, 0, SMH_BANK(5));
						memory_set_bankptr(machine, 5, mess_ram + 0xf800);
						// System reg
						memory_install_read8_handler (space, 0xfa00, 0xfaff, 0, 0, sysreg_r);
						memory_install_write8_handler(space, 0xfa00, 0xfaff, 0, 0, sysreg_w);
						// Devices
						memory_install_read8_handler (space, 0xfb00, 0xfbff, 0, 0, devices_r);
						memory_install_write8_handler(space, 0xfb00, 0xfbff, 0, 0, devices_w);
						// Text Video Memory
						memory_install_read8_handler (space, 0xfc00, 0xffff, 0, 0, text_r);
						memory_install_write8_handler(space, 0xfc00, 0xffff, 0, 0, text_w);
					}
					break;						
		case 0x12 : 
					{
						// ROM
						memory_install_read8_handler (space, 0x0000, 0x3fff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);
						// RAM
						memory_install_read8_handler (space, 0x4000, 0xf7ff, 0, 0, SMH_BANK(3));
						memory_install_write8_handler(space, 0x4000, 0xf7ff, 0, 0, SMH_BANK(4));
						memory_set_bankptr(machine, 3, mess_ram + 0x4000);
						memory_set_bankptr(machine, 4, mess_ram + 0x4000);						
						// Keyboard
						memory_install_read8_handler (space, 0xf800, 0xf9ff, 0, 0, keyboard_r);
						memory_install_write8_handler(space, 0xf800, 0xf9ff, 0, 0, SMH_BANK(5));
						memory_set_bankptr(machine, 5, mess_ram + 0xf800);
						// System reg
						memory_install_read8_handler (space, 0xfa00, 0xfaff, 0, 0, sysreg_r);
						memory_install_write8_handler(space, 0xfa00, 0xfaff, 0, 0, sysreg_w);
						// Devices
						memory_install_read8_handler (space, 0xfb00, 0xfbff, 0, 0, devices_r);
						memory_install_write8_handler(space, 0xfb00, 0xfbff, 0, 0, devices_w);
						// Text Video Memory
						memory_install_read8_handler (space, 0xfc00, 0xffff, 0, 0, text_r);
						memory_install_write8_handler(space, 0xfc00, 0xffff, 0, 0, text_w);
					}
					break;						
		case 0x13 : 
					{
						// RAM
						memory_install_read8_handler (space, 0x0000, 0xf7ff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0xf7ff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, mess_ram + 0x0000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);						
						// Keyboard
						memory_install_read8_handler (space, 0xf800, 0xf9ff, 0, 0, keyboard_r);
						memory_install_write8_handler(space, 0xf800, 0xf9ff, 0, 0, SMH_BANK(3));
						memory_set_bankptr(machine, 3, mess_ram + 0xf800);
						// System reg
						memory_install_read8_handler (space, 0xfa00, 0xfaff, 0, 0, sysreg_r);
						memory_install_write8_handler(space, 0xfa00, 0xfaff, 0, 0, sysreg_w);
						// Devices
						memory_install_read8_handler (space, 0xfb00, 0xfbff, 0, 0, devices_r);
						memory_install_write8_handler(space, 0xfb00, 0xfbff, 0, 0, devices_w);
						// Text Video Memory
						memory_install_read8_handler (space, 0xfc00, 0xffff, 0, 0, text_r);
						memory_install_write8_handler(space, 0xfc00, 0xffff, 0, 0, text_w);
					}
					break;											
		case 0x14 : 
					{
						// ROM
						memory_install_read8_handler (space, 0x0000, 0x5fff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0x5fff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);
						// RAM
						memory_install_read8_handler (space, 0x6000, 0xfdff, 0, 0, SMH_BANK(3));
						memory_install_write8_handler(space, 0x6000, 0xfdff, 0, 0, SMH_BANK(4));
						memory_set_bankptr(machine, 3, mess_ram + 0x6000);
						memory_set_bankptr(machine, 4, mess_ram + 0x6000);
						// Devices
						memory_install_read8_handler (space, 0xfe00, 0xfeff, 0, 0, devices_r);
						memory_install_write8_handler(space, 0xfe00, 0xfeff, 0, 0, devices_w);
						// System reg
						memory_install_read8_handler (space, 0xff00, 0xffff, 0, 0, sysreg_r);
						memory_install_write8_handler(space, 0xff00, 0xffff, 0, 0, sysreg_w);						
					}
					break;
		case 0x15 : 
					{
						// ROM
						memory_install_read8_handler (space, 0x0000, 0x1fff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0x1fff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);
						// RAM
						memory_install_read8_handler (space, 0x2000, 0xfdff, 0, 0, SMH_BANK(3));
						memory_install_write8_handler(space, 0x2000, 0xfdff, 0, 0, SMH_BANK(4));
						memory_set_bankptr(machine, 3, mess_ram + 0x2000);
						memory_set_bankptr(machine, 4, mess_ram + 0x2000);
						// Devices
						memory_install_read8_handler (space, 0xfe00, 0xfeff, 0, 0, devices_r);
						memory_install_write8_handler(space, 0xfe00, 0xfeff, 0, 0, devices_w);
						// System reg
						memory_install_read8_handler (space, 0xff00, 0xffff, 0, 0, sysreg_r);
						memory_install_write8_handler(space, 0xff00, 0xffff, 0, 0, sysreg_w);						
					}
					break;
		case 0x16 : 
					{
						// ROM
						memory_install_read8_handler (space, 0x0000, 0x3fff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);
						// RAM
						memory_install_read8_handler (space, 0x4000, 0xfdff, 0, 0, SMH_BANK(3));
						memory_install_write8_handler(space, 0x4000, 0xfdff, 0, 0, SMH_BANK(4));
						memory_set_bankptr(machine, 3, mess_ram + 0x4000);
						memory_set_bankptr(machine, 4, mess_ram + 0x4000);
						// Devices
						memory_install_read8_handler (space, 0xfe00, 0xfeff, 0, 0, devices_r);
						memory_install_write8_handler(space, 0xfe00, 0xfeff, 0, 0, devices_w);
						// System reg
						memory_install_read8_handler (space, 0xff00, 0xffff, 0, 0, sysreg_r);
						memory_install_write8_handler(space, 0xff00, 0xffff, 0, 0, sysreg_w);						
					}
					break;
		case 0x17 : 
					{
						// RAM
						memory_install_read8_handler (space, 0x0000, 0xfdff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0xfdff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, mess_ram);
						memory_set_bankptr(machine, 2, mess_ram);
						// Devices
						memory_install_read8_handler (space, 0xfe00, 0xfeff, 0, 0, devices_r);
						memory_install_write8_handler(space, 0xfe00, 0xfeff, 0, 0, devices_w);
						// System reg
						memory_install_read8_handler (space, 0xff00, 0xffff, 0, 0, sysreg_r);
						memory_install_write8_handler(space, 0xff00, 0xffff, 0, 0, sysreg_w);						
					}
					break;
		case 0x18 : 
					{
						// ROM
						memory_install_read8_handler (space, 0x0000, 0x5fff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0x5fff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);
						// RAM
						memory_install_read8_handler (space, 0x6000, 0xbeff, 0, 0, SMH_BANK(3));
						memory_install_write8_handler(space, 0x6000, 0xbeff, 0, 0, SMH_BANK(4));
						memory_set_bankptr(machine, 3, mess_ram + 0x6000);
						memory_set_bankptr(machine, 4, mess_ram + 0x6000);
						// System reg
						memory_install_read8_handler (space, 0xbf00, 0xbfff, 0, 0, sysreg_r);
						memory_install_write8_handler(space, 0xbf00, 0xbfff, 0, 0, sysreg_w);						
						// Video RAM
						memory_install_read8_handler (space, 0xc000, 0xffff, 0, 0, SMH_BANK(5));
						memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, SMH_BANK(6));
						memory_set_bankptr(machine, 5, mess_ram + 0x10000 + vpage * 0x4000);
						memory_set_bankptr(machine, 6, mess_ram + 0x10000 + vpage * 0x4000);
					}
					break;
		case 0x19 : 
					{
						// ROM
						memory_install_read8_handler (space, 0x0000, 0x1fff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0x1fff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);
						// RAM
						memory_install_read8_handler (space, 0x2000, 0xbeff, 0, 0, SMH_BANK(3));
						memory_install_write8_handler(space, 0x2000, 0xbeff, 0, 0, SMH_BANK(4));
						memory_set_bankptr(machine, 3, mess_ram + 0x2000);
						memory_set_bankptr(machine, 4, mess_ram + 0x2000);
						// System reg
						memory_install_read8_handler (space, 0xbf00, 0xbfff, 0, 0, sysreg_r);
						memory_install_write8_handler(space, 0xbf00, 0xbfff, 0, 0, sysreg_w);						
						// Video RAM
						memory_install_read8_handler (space, 0xc000, 0xffff, 0, 0, SMH_BANK(5));
						memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, SMH_BANK(6));
						memory_set_bankptr(machine, 5, mess_ram + 0x10000 + vpage * 0x4000);
						memory_set_bankptr(machine, 6, mess_ram + 0x10000 + vpage * 0x4000);
					}
					break;
		case 0x1A : 
					{
						// ROM
						memory_install_read8_handler (space, 0x0000, 0x3fff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);
						// RAM
						memory_install_read8_handler (space, 0x4000, 0xbeff, 0, 0, SMH_BANK(3));
						memory_install_write8_handler(space, 0x4000, 0xbeff, 0, 0, SMH_BANK(4));
						memory_set_bankptr(machine, 3, mess_ram + 0x4000);
						memory_set_bankptr(machine, 4, mess_ram + 0x4000);
						// System reg
						memory_install_read8_handler (space, 0xbf00, 0xbfff, 0, 0, sysreg_r);
						memory_install_write8_handler(space, 0xbf00, 0xbfff, 0, 0, sysreg_w);						
						// Video RAM
						memory_install_read8_handler (space, 0xc000, 0xffff, 0, 0, SMH_BANK(5));
						memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, SMH_BANK(6));
						memory_set_bankptr(machine, 5, mess_ram + 0x10000 + vpage * 0x4000);
						memory_set_bankptr(machine, 6, mess_ram + 0x10000 + vpage * 0x4000);
					}
					break;
		case 0x1B : 
					{
						// RAM
						memory_install_read8_handler (space, 0x0000, 0xbeff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0xbeff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, mess_ram);
						memory_set_bankptr(machine, 2, mess_ram);
						// System reg
						memory_install_read8_handler (space, 0xbf00, 0xbfff, 0, 0, sysreg_r);
						memory_install_write8_handler(space, 0xbf00, 0xbfff, 0, 0, sysreg_w);						
						// Video RAM
						memory_install_read8_handler (space, 0xc000, 0xffff, 0, 0, SMH_BANK(3));
						memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, SMH_BANK(4));
						memory_set_bankptr(machine, 3, mess_ram + 0x10000 + vpage * 0x4000);
						memory_set_bankptr(machine, 4, mess_ram + 0x10000 + vpage * 0x4000);
					}
					break;
		case 0x1C : 
					{
						// ROM
						memory_install_read8_handler (space, 0x0000, 0x5fff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0x5fff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);
						// RAM
						memory_install_read8_handler (space, 0x6000, 0xbfff, 0, 0, SMH_BANK(3));
						memory_install_write8_handler(space, 0x6000, 0xbfff, 0, 0, SMH_BANK(4));
						memory_set_bankptr(machine, 3, mess_ram + 0x6000);
						memory_set_bankptr(machine, 4, mess_ram + 0x6000);
						// Video RAM
						memory_install_read8_handler (space, 0xc000, 0xffff, 0, 0, SMH_BANK(5));
						memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, SMH_BANK(6));
						memory_set_bankptr(machine, 5, mess_ram + 0x10000 + vpage * 0x4000);
						memory_set_bankptr(machine, 6, mess_ram + 0x10000 + vpage * 0x4000);
					}
					break;
		case 0x1D : 
					{
						// ROM
						memory_install_read8_handler (space, 0x0000, 0x1fff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0x1fff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);
						// RAM
						memory_install_read8_handler (space, 0x2000, 0xbfff, 0, 0, SMH_BANK(3));
						memory_install_write8_handler(space, 0x2000, 0xbfff, 0, 0, SMH_BANK(4));
						memory_set_bankptr(machine, 3, mess_ram + 0x2000);
						memory_set_bankptr(machine, 4, mess_ram + 0x2000);
						// Video RAM
						memory_install_read8_handler (space, 0xc000, 0xffff, 0, 0, SMH_BANK(5));
						memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, SMH_BANK(6));
						memory_set_bankptr(machine, 5, mess_ram + 0x10000 + vpage * 0x4000);
						memory_set_bankptr(machine, 6, mess_ram + 0x10000 + vpage * 0x4000);
					}
					break;
		case 0x1E : 
					{
						// ROM
						memory_install_read8_handler (space, 0x0000, 0x3fff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, 2, mess_ram + 0x0000);
						// RAM
						memory_install_read8_handler (space, 0x4000, 0xbfff, 0, 0, SMH_BANK(3));
						memory_install_write8_handler(space, 0x4000, 0xbfff, 0, 0, SMH_BANK(4));
						memory_set_bankptr(machine, 3, mess_ram + 0x4000);
						memory_set_bankptr(machine, 4, mess_ram + 0x4000);
						// Video RAM
						memory_install_read8_handler (space, 0xc000, 0xffff, 0, 0, SMH_BANK(5));
						memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, SMH_BANK(6));
						memory_set_bankptr(machine, 5, mess_ram + 0x10000 + vpage * 0x4000);
						memory_set_bankptr(machine, 6, mess_ram + 0x10000 + vpage * 0x4000);
					}
					break;
		case 0x1F : 
					{
						// RAM
						memory_install_read8_handler (space, 0x0000, 0xbfff, 0, 0, SMH_BANK(1));
						memory_install_write8_handler(space, 0x0000, 0xbfff, 0, 0, SMH_BANK(2));
						memory_set_bankptr(machine, 1, mess_ram);
						memory_set_bankptr(machine, 2, mess_ram);
						// Video RAM
						memory_install_read8_handler (space, 0xc000, 0xffff, 0, 0, SMH_BANK(3));
						memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, SMH_BANK(4));
						memory_set_bankptr(machine, 3, mess_ram + 0x10000 + vpage * 0x4000);
						memory_set_bankptr(machine, 4, mess_ram + 0x10000 + vpage * 0x4000);
					}
					break;
					
	}
}

static READ8_DEVICE_HANDLER(pk8020_porta_r)
{	
	return 0xf0 | (takt <<1) | (attr &1)<<3;
}

static WRITE8_DEVICE_HANDLER(pk8020_portc_w)
{	
	if (((data & 0x30) >> 4)==3) {
		attr ^=1; 
	}
}

I8255A_INTERFACE( pk8020_ppi8255_interface_1 )
{
	DEVCB_HANDLER(pk8020_porta_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(pk8020_portc_w)
};
static READ8_DEVICE_HANDLER(pk8020_unk_r)
{	
	return 0x00;
}
I8255A_INTERFACE( pk8020_ppi8255_interface_2 )
{
	DEVCB_HANDLER(pk8020_unk_r),
	DEVCB_HANDLER(pk8020_unk_r),
	DEVCB_HANDLER(pk8020_unk_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

I8255A_INTERFACE( pk8020_ppi8255_interface_3 )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

static PIT8253_OUTPUT_CHANGED(pk8020_pit_out0)
{
	// beep	
}


static PIT8253_OUTPUT_CHANGED(pk8020_pit_out1)
{
}

static PIT8253_OUTPUT_CHANGED(pk8020_pit_out2)
{	
	pic8259_set_irq_line(devtag_get_device(device->machine, "pic8259"),5,state);
}


const struct pit8253_config pk8020_pit8253_intf =
{
	{
		{
			XTAL_20MHz / 10,
			pk8020_pit_out0
		},
		{
			XTAL_20MHz / 10,
			pk8020_pit_out1
		},
		{
			(XTAL_20MHz / 8) / 164,
			pk8020_pit_out2
		}
	}
};

static PIC8259_SET_INT_LINE( pk8020_pic_set_int_line )
{		
	cputag_set_input_line(device->machine, "maincpu", 0, interrupt ?  HOLD_LINE : CLEAR_LINE);  
} 

const struct pic8259_interface pk8020_pic8259_config = {
	pk8020_pic_set_int_line
};

static IRQ_CALLBACK(pk8020_irq_callback)
{	
	return pic8259_acknowledge(devtag_get_device(device->machine, "pic8259"));
} 

MACHINE_RESET( pk8020 )
{
	pk8020_set_bank(machine,0);
	cpu_set_irq_callback(cputag_get_cpu(machine, "maincpu"), pk8020_irq_callback);
}

DEVICE_IMAGE_LOAD( pk8020_floppy )
{
	int size;

	if (! image_has_been_created(image))
		{
		size = image_length(image);

		switch (size)
			{
			case 800*1024:
				break;
			default:
				return INIT_FAIL;
			}
		}
	else
		return INIT_FAIL;

	if (device_load_basicdsk_floppy (image) != INIT_PASS)
		return INIT_FAIL;

	basicdsk_set_geometry (image, 80, 2, 5, 1024, 1, 0, FALSE);	
	return INIT_PASS;
}
INTERRUPT_GEN( pk8020_interrupt )
{
	takt ^= 1;
}

