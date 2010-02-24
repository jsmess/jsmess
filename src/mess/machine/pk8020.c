/***************************************************************************

        PK-8020 driver by Miodrag Milanovic

        18/07/2008 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "includes/pk8020.h"
#include "cpu/i8085/i8085.h"
#include "machine/wd17xx.h"
#include "devices/messram.h"

static UINT8 attr = 0;
static UINT8 text_attr = 0;
static UINT8 takt = 0;
UINT8 pk8020_color = 0;
UINT8 pk8020_video_page = 0;
UINT8 pk8020_wide = 0;
UINT8 pk8020_font = 0;
static UINT8 pk8020_video_page_access = 0;
static UINT8 portc_data;
static void pk8020_set_bank(running_machine *machine,UINT8 data);
static UINT8 sound_gate = 0;
static UINT8 sound_level = 0;


static READ8_HANDLER( keyboard_r )
{
	static const char *const keynames[] = {
		"LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7",
		"LINE8", "LINE9", "LINE10", "LINE11", "LINE12", "LINE13", "LINE14", "LINE15"
	};
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
	return messram_get_ptr(devtag_get_device(space->machine, "messram"))[offset];
}
static WRITE8_HANDLER(sysreg_w)
{
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
    if (attr == 3) text_attr=messram_get_ptr(devtag_get_device(space->machine, "messram"))[0x40400+offset];
	return messram_get_ptr(devtag_get_device(space->machine, "messram"))[0x40000+offset];
}

static WRITE8_HANDLER(text_w)
{
	messram_get_ptr(devtag_get_device(space->machine, "messram"))[0x40000+offset] = data;
	switch (attr) {
        case 0: break;
        case 1: messram_get_ptr(devtag_get_device(space->machine, "messram"))[0x40400+offset]=0x01;break;
        case 2: messram_get_ptr(devtag_get_device(space->machine, "messram"))[0x40400+offset]=0x00;break;
        case 3: messram_get_ptr(devtag_get_device(space->machine, "messram"))[0x40400+offset]=text_attr;break;
    }
}

static READ8_HANDLER(gzu_r)
{
	UINT8 *addr = messram_get_ptr(devtag_get_device(space->machine, "messram")) + 0x10000 + (pk8020_video_page_access * 0xC000);
	UINT8 p0 = addr[offset];
	UINT8 p1 = addr[offset + 0x4000];
	UINT8 p2 = addr[offset + 0x8000];
	UINT8 retVal = 0;
	if(pk8020_color & 0x80) {
		// Color mode
		if (!(pk8020_color & 0x10)) {
			p0 ^= 0xff;
		}
		if (!(pk8020_color & 0x20)) {
			p1 ^= 0xff;
		}
		if (!(pk8020_color & 0x40)) {
			p2 ^= 0xff;
		}
		retVal = (p0 & p1 & p2) ^ 0xff;
	} else {
		// Plane mode
		if (pk8020_color & 0x10) {
			retVal |= p0;
		}
		if (pk8020_color & 0x20) {
			retVal |= p1;
		}
		if (pk8020_color & 0x40) {
			retVal |= p2;
		}
	}
	return retVal;
}

static WRITE8_HANDLER(gzu_w)
{
	UINT8 *addr = messram_get_ptr(devtag_get_device(space->machine, "messram")) + 0x10000 + (pk8020_video_page_access * 0xC000);
	UINT8 *plane_0 = addr;
	UINT8 *plane_1 = addr + 0x4000;
	UINT8 *plane_2 = addr + 0x8000;

	if(pk8020_color & 0x80)
	{
		// Color mode
		plane_0[offset] = (plane_0[offset] & ~data) | ((pk8020_color & 2) ? data : 0);
		plane_1[offset] = (plane_1[offset] & ~data) | ((pk8020_color & 4) ? data : 0);
		plane_2[offset] = (plane_2[offset] & ~data) | ((pk8020_color & 8) ? data : 0);
	} else {
		// Plane mode
		UINT8 mask = (pk8020_color & 1) ? data : 0;
		if (!(pk8020_color & 0x02)) {
			plane_0[offset] = (plane_0[offset] & ~data) | mask;
		}
		if (!(pk8020_color & 0x04)) {
			plane_1[offset] = (plane_1[offset] & ~data) | mask;
		}
		if (!(pk8020_color & 0x08)) {
			plane_2[offset] = (plane_2[offset] & ~data) | mask;
		}
	}
}

static READ8_HANDLER(devices_r)
{
	running_device *ppi1 = devtag_get_device(space->machine, "ppi8255_1");
	running_device *ppi2 = devtag_get_device(space->machine, "ppi8255_2");
	running_device *ppi3 = devtag_get_device(space->machine, "ppi8255_3");
	running_device *pit = devtag_get_device(space->machine, "pit8253");
	running_device *pic = devtag_get_device(space->machine, "pic8259");
	running_device *rs232 = devtag_get_device(space->machine, "rs232");
	running_device *lan = devtag_get_device(space->machine, "lan");
	running_device *fdc = devtag_get_device(space->machine, "wd1793");

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
	running_device *ppi1 = devtag_get_device(space->machine, "ppi8255_1");
	running_device *ppi2 = devtag_get_device(space->machine, "ppi8255_2");
	running_device *ppi3 = devtag_get_device(space->machine, "ppi8255_3");
	running_device *pit = devtag_get_device(space->machine, "pit8253");
	running_device *pic = devtag_get_device(space->machine, "pic8259");
	running_device *rs232 = devtag_get_device(space->machine, "rs232");
	running_device *lan = devtag_get_device(space->machine, "lan");
	running_device *fdc = devtag_get_device(space->machine, "wd1793");

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
						case 0 : wd17xx_command_w(fdc,0,data);break;
						case 1 : wd17xx_track_w(fdc,0,data);break;
						case 2 : wd17xx_sector_w(fdc,0,data);break;
						case 3 : wd17xx_data_w(fdc,0,data);break;
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
						memory_install_read_bank (space, 0x0000, 0x37ff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0x37ff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// Keyboard
						memory_install_read8_handler (space, 0x3800, 0x39ff, 0, 0, keyboard_r);
						memory_install_write_bank(space, 0x3800, 0x39ff, 0, 0, "bank3");
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x3800);
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
						memory_install_read_bank (space, 0x4000, 0xffff, 0, 0, "bank4");
						memory_install_write_bank(space, 0x4000, 0xffff, 0, 0, "bank5");
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x4000);
						memory_set_bankptr(machine, "bank5", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x4000);
					}
					break;
		case 0x01 : {
						// ROM
						memory_install_read_bank (space, 0x0000, 0x1fff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0x1fff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// RAM
						memory_install_read_bank (space, 0x2000, 0xffff, 0, 0, "bank3");
						memory_install_write_bank(space, 0x2000, 0xffff, 0, 0, "bank4");
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x2000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x2000);
					}
					break;
		case 0x02 : {
						// ROM
						memory_install_read_bank (space, 0x0000, 0x3fff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0x3fff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// RAM
						memory_install_read_bank (space, 0x4000, 0xffff, 0, 0, "bank3");
						memory_install_write_bank(space, 0x4000, 0xffff, 0, 0, "bank4");
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x4000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x4000);
					}
					break;
		case 0x03 : {
						// RAM
						memory_install_read_bank (space, 0x0000, 0xffff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0xffff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", messram_get_ptr(devtag_get_device(machine, "messram")));
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")));
					}
					break;
		case 0x04 :
		case 0x05 :
					{
						// ROM
						memory_install_read_bank (space, 0x0000, 0x1fff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0x1fff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// RAM
						memory_install_read_bank (space, 0x2000, 0xf7ff, 0, 0, "bank3");
						memory_install_write_bank(space, 0x2000, 0xf7ff, 0, 0, "bank4");
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x2000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x2000);
						// Keyboard
						memory_install_read8_handler (space, 0xf800, 0xf9ff, 0, 0, keyboard_r);
						memory_install_write_bank(space, 0xf800, 0xf9ff, 0, 0, "bank5");
						memory_set_bankptr(machine, "bank5", messram_get_ptr(devtag_get_device(machine, "messram")) + 0xf800);
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
						memory_install_read_bank (space, 0x0000, 0x3fff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0x3fff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// RAM
						memory_install_read_bank (space, 0x4000, 0xf7ff, 0, 0, "bank3");
						memory_install_write_bank(space, 0x4000, 0xf7ff, 0, 0, "bank4");
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x4000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x4000);
						// Keyboard
						memory_install_read8_handler (space, 0xf800, 0xf9ff, 0, 0, keyboard_r);
						memory_install_write_bank(space, 0xf800, 0xf9ff, 0, 0, "bank5");
						memory_set_bankptr(machine, "bank5", messram_get_ptr(devtag_get_device(machine, "messram")) + 0xf800);
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
						memory_install_read_bank (space, 0x0000, 0xf7ff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0xf7ff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", messram_get_ptr(devtag_get_device(machine, "messram")));
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")));
						// Keyboard
						memory_install_read8_handler (space, 0xf800, 0xf9ff, 0, 0, keyboard_r);
						memory_install_write_bank(space, 0xf800, 0xf9ff, 0, 0, "bank3");
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0xf800);
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
						memory_install_read_bank (space, 0x0000, 0x3fff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0x3fff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// Keyboard
						memory_install_read8_handler (space, 0x3800, 0x39ff, 0, 0, keyboard_r);
						memory_install_write_bank(space, 0x3800, 0x39ff, 0, 0, "bank3");
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x3800);
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
						memory_install_read_bank (space, 0x4000, 0xbfff, 0, 0, "bank4");
						memory_install_write_bank(space, 0x4000, 0xbfff, 0, 0, "bank5");
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x4000);
						memory_set_bankptr(machine, "bank5", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x4000);
						// Video RAM
						memory_install_read8_handler (space, 0xc000, 0xffff, 0, 0, gzu_r);
						memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, gzu_w);

					}
					break;
		case 0x09 :
					{
						// ROM
						memory_install_read_bank (space, 0x0000, 0x1fff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0x1fff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// RAM
						memory_install_read_bank (space, 0x2000, 0xbfff, 0, 0, "bank3");
						memory_install_write_bank(space, 0x2000, 0xbfff, 0, 0, "bank4");
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x2000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x2000);
						// Video RAM
						memory_install_read8_handler (space, 0xc000, 0xffff, 0, 0, gzu_r);
						memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, gzu_w);
					}
					break;
		case 0x0A :
					{
						// ROM
						memory_install_read_bank (space, 0x0000, 0x3fff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0x3fff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// RAM
						memory_install_read_bank (space, 0x4000, 0xbfff, 0, 0, "bank3");
						memory_install_write_bank(space, 0x4000, 0xbfff, 0, 0, "bank4");
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x4000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x4000);
						// Video RAM
						memory_install_read8_handler (space, 0xc000, 0xffff, 0, 0, gzu_r);
						memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, gzu_w);
					}
					break;
		case 0x0B :
					{
						// RAM
						memory_install_read_bank (space, 0x0000, 0xbfff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0xbfff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// Video RAM
						memory_install_read8_handler (space, 0xc000, 0xffff, 0, 0, gzu_r);
						memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, gzu_w);
					}
					break;
		case 0x0C :
		case 0x0D :
					{
						// ROM
						memory_install_read_bank (space, 0x0000, 0x1fff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0x1fff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// RAM
						memory_install_read_bank (space, 0x2000, 0x3fff, 0, 0, "bank3");
						memory_install_write_bank(space, 0x2000, 0x3fff, 0, 0, "bank4");
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x2000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x2000);
						// Video RAM
						memory_install_read8_handler (space, 0x4000, 0x7fff, 0, 0, gzu_r);
						memory_install_write8_handler(space, 0x4000, 0x7fff, 0, 0, gzu_w);
						// RAM
						memory_install_read_bank (space, 0x8000, 0xfdff, 0, 0, "bank5");
						memory_install_write_bank(space, 0x8000, 0xfdff, 0, 0, "bank6");
						memory_set_bankptr(machine, "bank5", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x8000);
						memory_set_bankptr(machine, "bank6", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x8000);
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
						memory_install_read_bank (space, 0x0000, 0x3fff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0x3fff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// Video RAM
						memory_install_read8_handler (space, 0x4000, 0x7fff, 0, 0, gzu_r);
						memory_install_write8_handler(space, 0x4000, 0x7fff, 0, 0, gzu_w);
						// RAM
						memory_install_read_bank (space, 0x8000, 0xfdff, 0, 0, "bank5");
						memory_install_write_bank(space, 0x8000, 0xfdff, 0, 0, "bank6");
						memory_set_bankptr(machine, "bank5", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x8000);
						memory_set_bankptr(machine, "bank6", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x8000);
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
						memory_install_read_bank (space, 0x0000, 0x3fff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0x3fff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// Video RAM
						memory_install_read8_handler (space, 0x4000, 0x7fff, 0, 0, gzu_r);
						memory_install_write8_handler(space, 0x4000, 0x7fff, 0, 0, gzu_w);
						// RAM
						memory_install_read_bank (space, 0x8000, 0xfdff, 0, 0, "bank3");
						memory_install_write_bank(space, 0x8000, 0xfdff, 0, 0, "bank4");
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x8000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x8000);
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
						memory_install_read_bank (space, 0x0000, 0x5fff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0x5fff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// RAM
						memory_install_read_bank (space, 0x6000, 0xf7ff, 0, 0, "bank3");
						memory_install_write_bank(space, 0x6000, 0xf7ff, 0, 0, "bank4");
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x6000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x6000);
						// Keyboard
						memory_install_read8_handler (space, 0xf800, 0xf9ff, 0, 0, keyboard_r);
						memory_install_write_bank(space, 0xf800, 0xf9ff, 0, 0, "bank5");
						memory_set_bankptr(machine, "bank5", messram_get_ptr(devtag_get_device(machine, "messram")) + 0xf800);
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
						memory_install_read_bank (space, 0x0000, 0x1fff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0x1fff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// RAM
						memory_install_read_bank (space, 0x2000, 0xf7ff, 0, 0, "bank3");
						memory_install_write_bank(space, 0x2000, 0xf7ff, 0, 0, "bank4");
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x2000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x2000);
						// Keyboard
						memory_install_read8_handler (space, 0xf800, 0xf9ff, 0, 0, keyboard_r);
						memory_install_write_bank(space, 0xf800, 0xf9ff, 0, 0, "bank5");
						memory_set_bankptr(machine, "bank5", messram_get_ptr(devtag_get_device(machine, "messram")) + 0xf800);
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
						memory_install_read_bank (space, 0x0000, 0x3fff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0x3fff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// RAM
						memory_install_read_bank (space, 0x4000, 0xf7ff, 0, 0, "bank3");
						memory_install_write_bank(space, 0x4000, 0xf7ff, 0, 0, "bank4");
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x4000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x4000);
						// Keyboard
						memory_install_read8_handler (space, 0xf800, 0xf9ff, 0, 0, keyboard_r);
						memory_install_write_bank(space, 0xf800, 0xf9ff, 0, 0, "bank5");
						memory_set_bankptr(machine, "bank5", messram_get_ptr(devtag_get_device(machine, "messram")) + 0xf800);
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
						memory_install_read_bank (space, 0x0000, 0xf7ff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0xf7ff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// Keyboard
						memory_install_read8_handler (space, 0xf800, 0xf9ff, 0, 0, keyboard_r);
						memory_install_write_bank(space, 0xf800, 0xf9ff, 0, 0, "bank3");
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0xf800);
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
						memory_install_read_bank (space, 0x0000, 0x5fff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0x5fff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// RAM
						memory_install_read_bank (space, 0x6000, 0xfdff, 0, 0, "bank3");
						memory_install_write_bank(space, 0x6000, 0xfdff, 0, 0, "bank4");
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x6000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x6000);
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
						memory_install_read_bank (space, 0x0000, 0x1fff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0x1fff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// RAM
						memory_install_read_bank (space, 0x2000, 0xfdff, 0, 0, "bank3");
						memory_install_write_bank(space, 0x2000, 0xfdff, 0, 0, "bank4");
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x2000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x2000);
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
						memory_install_read_bank (space, 0x0000, 0x3fff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0x3fff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// RAM
						memory_install_read_bank (space, 0x4000, 0xfdff, 0, 0, "bank3");
						memory_install_write_bank(space, 0x4000, 0xfdff, 0, 0, "bank4");
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x4000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x4000);
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
						memory_install_read_bank (space, 0x0000, 0xfdff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0xfdff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", messram_get_ptr(devtag_get_device(machine, "messram")));
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")));
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
						memory_install_read_bank (space, 0x0000, 0x5fff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0x5fff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// RAM
						memory_install_read_bank (space, 0x6000, 0xbeff, 0, 0, "bank3");
						memory_install_write_bank(space, 0x6000, 0xbeff, 0, 0, "bank4");
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x6000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x6000);
						// System reg
						memory_install_read8_handler (space, 0xbf00, 0xbfff, 0, 0, sysreg_r);
						memory_install_write8_handler(space, 0xbf00, 0xbfff, 0, 0, sysreg_w);
						// Video RAM
						memory_install_read8_handler (space, 0xc000, 0xffff, 0, 0, gzu_r);
						memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, gzu_w);
					}
					break;
		case 0x19 :
					{
						// ROM
						memory_install_read_bank (space, 0x0000, 0x1fff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0x1fff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// RAM
						memory_install_read_bank (space, 0x2000, 0xbeff, 0, 0, "bank3");
						memory_install_write_bank(space, 0x2000, 0xbeff, 0, 0, "bank4");
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x2000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x2000);
						// System reg
						memory_install_read8_handler (space, 0xbf00, 0xbfff, 0, 0, sysreg_r);
						memory_install_write8_handler(space, 0xbf00, 0xbfff, 0, 0, sysreg_w);
						// Video RAM
						memory_install_read8_handler (space, 0xc000, 0xffff, 0, 0, gzu_r);
						memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, gzu_w);
					}
					break;
		case 0x1A :
					{
						// ROM
						memory_install_read_bank (space, 0x0000, 0x3fff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0x3fff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// RAM
						memory_install_read_bank (space, 0x4000, 0xbeff, 0, 0, "bank3");
						memory_install_write_bank(space, 0x4000, 0xbeff, 0, 0, "bank4");
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x4000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x4000);
						// System reg
						memory_install_read8_handler (space, 0xbf00, 0xbfff, 0, 0, sysreg_r);
						memory_install_write8_handler(space, 0xbf00, 0xbfff, 0, 0, sysreg_w);
						// Video RAM
						memory_install_read8_handler (space, 0xc000, 0xffff, 0, 0, gzu_r);
						memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, gzu_w);
					}
					break;
		case 0x1B :
					{
						// RAM
						memory_install_read_bank (space, 0x0000, 0xbeff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0xbeff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", messram_get_ptr(devtag_get_device(machine, "messram")));
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")));
						// System reg
						memory_install_read8_handler (space, 0xbf00, 0xbfff, 0, 0, sysreg_r);
						memory_install_write8_handler(space, 0xbf00, 0xbfff, 0, 0, sysreg_w);
						// Video RAM
						memory_install_read8_handler (space, 0xc000, 0xffff, 0, 0, gzu_r);
						memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, gzu_w);
					}
					break;
		case 0x1C :
					{
						// ROM
						memory_install_read_bank (space, 0x0000, 0x5fff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0x5fff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// RAM
						memory_install_read_bank (space, 0x6000, 0xbfff, 0, 0, "bank3");
						memory_install_write_bank(space, 0x6000, 0xbfff, 0, 0, "bank4");
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x6000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x6000);
						// Video RAM
						memory_install_read8_handler (space, 0xc000, 0xffff, 0, 0, gzu_r);
						memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, gzu_w);
					}
					break;
		case 0x1D :
					{
						// ROM
						memory_install_read_bank (space, 0x0000, 0x1fff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0x1fff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// RAM
						memory_install_read_bank (space, 0x2000, 0xbfff, 0, 0, "bank3");
						memory_install_write_bank(space, 0x2000, 0xbfff, 0, 0, "bank4");
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x2000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x2000);
						// Video RAM
						memory_install_read8_handler (space, 0xc000, 0xffff, 0, 0, gzu_r);
						memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, gzu_w);
					}
					break;
		case 0x1E :
					{
						// ROM
						memory_install_read_bank (space, 0x0000, 0x3fff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0x3fff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", memory_region(machine,"maincpu") + 0x10000);
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
						// RAM
						memory_install_read_bank (space, 0x4000, 0xbfff, 0, 0, "bank3");
						memory_install_write_bank(space, 0x4000, 0xbfff, 0, 0, "bank4");
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x4000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x4000);
						// Video RAM
						memory_install_read8_handler (space, 0xc000, 0xffff, 0, 0, gzu_r);
						memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, gzu_w);
					}
					break;
		case 0x1F :
					{
						// RAM
						memory_install_read_bank (space, 0x0000, 0xbfff, 0, 0, "bank1");
						memory_install_write_bank(space, 0x0000, 0xbfff, 0, 0, "bank2");
						memory_set_bankptr(machine, "bank1", messram_get_ptr(devtag_get_device(machine, "messram")));
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")));
						// Video RAM
						memory_install_read8_handler (space, 0xc000, 0xffff, 0, 0, gzu_r);
						memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, gzu_w);
					}
					break;

	}
}

static READ8_DEVICE_HANDLER(pk8020_porta_r)
{
	return 0xf0 | (takt <<1) | (text_attr)<<3;
}

static WRITE8_DEVICE_HANDLER(pk8020_portc_w)
{
   pk8020_video_page_access =(data>>6) & 3;
   attr = (data >> 4) & 3;
   pk8020_wide = (data >> 3) & 1;
   pk8020_font = (data >> 2) & 1;
   pk8020_video_page = (data & 3);


   portc_data = data;
}

static WRITE8_DEVICE_HANDLER(pk8020_portb_w)
{
	running_device *fdc = devtag_get_device(device->machine, "wd1793");
	wd17xx_set_side(fdc,BIT(data,4));
	if (BIT(data,0)) {
		wd17xx_set_drive(fdc,0);
	} else if (BIT(data,1)) {
		wd17xx_set_drive(fdc,1);
	} else if (BIT(data,2)) {
		wd17xx_set_drive(fdc,2);
	} else if (BIT(data,3)) {
		wd17xx_set_drive(fdc,3);
	}
}

static READ8_DEVICE_HANDLER(pk8020_portc_r)
{
	return portc_data;
}


I8255A_INTERFACE( pk8020_ppi8255_interface_1 )
{
	DEVCB_HANDLER(pk8020_porta_r),
	DEVCB_NULL,
	DEVCB_HANDLER(pk8020_portc_r),
	DEVCB_NULL,
	DEVCB_HANDLER(pk8020_portb_w),
	DEVCB_HANDLER(pk8020_portc_w)
};

static WRITE8_DEVICE_HANDLER(pk8020_2_portc_w)
{
	running_device *speaker = devtag_get_device(device->machine, "speaker");

	sound_gate = BIT(data,3);

	speaker_level_w(speaker, sound_gate ? sound_level : 0);
}

I8255A_INTERFACE( pk8020_ppi8255_interface_2 )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(pk8020_2_portc_w)
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
	running_device *speaker = devtag_get_device(device->machine, "speaker");

	sound_level = state;

	speaker_level_w(speaker, sound_gate ? sound_level : 0);
}


static PIT8253_OUTPUT_CHANGED(pk8020_pit_out1)
{
}

static PIT8253_OUTPUT_CHANGED(pk8020_pit_out2)
{
	pic8259_ir5_w(devtag_get_device(device->machine, "pic8259"), state);
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

static WRITE_LINE_DEVICE_HANDLER( pk8020_pic_set_int_line )
{
	cputag_set_input_line(device->machine, "maincpu", 0, state ? HOLD_LINE : CLEAR_LINE);
}

const struct pic8259_interface pk8020_pic8259_config =
{
	DEVCB_LINE(pk8020_pic_set_int_line)
};

static IRQ_CALLBACK( pk8020_irq_callback )
{
	return pic8259_acknowledge(devtag_get_device(device->machine, "pic8259"));
}

MACHINE_RESET( pk8020 )
{
	pk8020_set_bank(machine,0);
	cpu_set_irq_callback(devtag_get_device(machine, "maincpu"), pk8020_irq_callback);

	sound_gate = 0;
	sound_level = 0;
}

INTERRUPT_GEN( pk8020_interrupt )
{
	takt ^= 1;
	pic8259_ir4_w(devtag_get_device(device->machine, "pic8259"), 1);
}
