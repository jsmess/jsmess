/***************************************************************************

        PK-8020 driver by Miodrag Milanovic

        18/07/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "machine/i8255a.h"

#include "includes/pk8020.h"

static UINT8 vpage =0;

static void pk8020_set_bank(running_machine *machine,UINT8 data);

/* Driver initialization */
DRIVER_INIT(pk8020)
{
	memset(mess_ram,0,130*1024);
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
	logerror("keyboard_r :%04x %02x\n",offset,retVal);
	return retVal;
}

static READ8_HANDLER(sysreg_r)
{
	return 0xff;
}
static WRITE8_HANDLER(sysreg_w)
{
	logerror("sysreg_w %02x %02x\n",offset,data);
	if (BIT(offset,7)==0) {
		pk8020_set_bank(space->machine,data >> 2);
	} else if (BIT(offset,6)==0) {
		// Color
	} else if (BIT(offset,2)==0) {
		// LED
	}
}

static READ8_HANDLER(text_r)
{
	return mess_ram[0x20000+offset];
}

static WRITE8_HANDLER(text_w)
{
//	logerror("text_w %02x %02x\n",offset,data);
	mess_ram[0x20000+offset] = data;
}

static READ8_HANDLER(devices_r)
{
	const device_config *ppi1 = devtag_get_device(space->machine, "ppi8255_1");
	const device_config *ppi2 = devtag_get_device(space->machine, "ppi8255_2");
	const device_config *ppi3 = devtag_get_device(space->machine, "ppi8255_3");
	logerror("devices_r %02x\n",offset);
	switch(offset & 0x38)
	{
		case 0x08: return i8255a_r(ppi3,offset & 3);
		case 0x30: return i8255a_r(ppi2,offset & 3);
		case 0x38: return i8255a_r(ppi1,offset & 3);
				
/*		case 0x00:	Timer_Write(Addres,Value);break; //xx00..03
        case 0x08:	PPI3_Write (Addres,Value);break; //xx08..0B
        case 0x10:	RS232_Write(Addres,Value);break; //xx10..11
        case 0x18:	FDC_Write  (Addres,Value);break; //xx18..1B
        case 0x20:	LAN_Write  (Addres,Value);break; //xx20..21
        case 0x28:	PIC_Write  (Addres,Value);break; //xx28..29
        case 0x30:	PPI2_Write (Addres,Value);break; //xx30..33
        case 0x38:	PPI1_Write (Addres,Value);break; //xx38..3B*/
	}
	return 0xff;
}

static WRITE8_HANDLER(devices_w)
{
	const device_config *ppi1 = devtag_get_device(space->machine, "ppi8255_1");
	const device_config *ppi2 = devtag_get_device(space->machine, "ppi8255_2");
	const device_config *ppi3 = devtag_get_device(space->machine, "ppi8255_3");
	logerror("devices_w %02x %02x\n",offset,data);
	switch(offset & 0x38)
	{
		case 0x08: i8255a_w(ppi3,offset & 3,data); return;
		case 0x30: i8255a_w(ppi2,offset & 3,data); return;
		case 0x38: i8255a_w(ppi1,offset & 3,data); return;
				
/*		case 0x00:	Timer_Write(Addres,Value);break; //xx00..03
        case 0x08:	PPI3_Write (Addres,Value);break; //xx08..0B
        case 0x10:	RS232_Write(Addres,Value);break; //xx10..11
        case 0x18:	FDC_Write  (Addres,Value);break; //xx18..1B
        case 0x20:	LAN_Write  (Addres,Value);break; //xx20..21
        case 0x28:	PIC_Write  (Addres,Value);break; //xx28..29
        case 0x30:	PPI2_Write (Addres,Value);break; //xx30..33
        case 0x38:	PPI1_Write (Addres,Value);break; //xx38..3B*/
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
MACHINE_RESET( pk8020 )
{
	pk8020_set_bank(machine,0);
}
