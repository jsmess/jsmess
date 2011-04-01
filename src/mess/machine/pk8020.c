/***************************************************************************

        PK-8020 driver by Miodrag Milanovic

        18/07/2008 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "includes/pk8020.h"
#include "cpu/i8085/i8085.h"
#include "machine/wd17xx.h"
#include "machine/ram.h"
#include "imagedev/flopdrv.h"

static void pk8020_set_bank(running_machine &machine,UINT8 data);


static READ8_HANDLER( keyboard_r )
{
	static const char *const keynames[] = {
		"LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7",
		"LINE8", "LINE9", "LINE10", "LINE11", "LINE12", "LINE13", "LINE14", "LINE15"
	};
	UINT8 retVal=0x00;
	UINT8 line = 0;
	if (offset & 0x100)  line=8;

	if (offset & 0x0001) retVal|=input_port_read(space->machine(),keynames[line]);
	line++;
	if (offset & 0x0002) retVal|=input_port_read(space->machine(),keynames[line]);
	line++;
	if (offset & 0x0004) retVal|=input_port_read(space->machine(),keynames[line]);
	line++;
	if (offset & 0x0008) retVal|=input_port_read(space->machine(),keynames[line]);
	line++;
	if (offset & 0x0010) retVal|=input_port_read(space->machine(),keynames[line]);
	line++;
	if (offset & 0x0020) retVal|=input_port_read(space->machine(),keynames[line]);
	line++;
	if (offset & 0x0040) retVal|=input_port_read(space->machine(),keynames[line]);
	line++;
	if (offset & 0x0080) retVal|=input_port_read(space->machine(),keynames[line]);
	line++;

	return retVal;
}

static READ8_HANDLER(sysreg_r)
{
	return ram_get_ptr(space->machine().device(RAM_TAG))[offset];
}
static WRITE8_HANDLER(sysreg_w)
{
	pk8020_state *state = space->machine().driver_data<pk8020_state>();
	if (BIT(offset,7)==0) {
		pk8020_set_bank(space->machine(),data >> 2);
	} else if (BIT(offset,6)==0) {
		// Color
		state->m_color = data;
	} else if (BIT(offset,2)==0) {
		// Palette set
		UINT8 number = data & 0x0f;
		UINT8 color = data >> 4;
		UINT8 i = (color & 0x08) ?  0x3F : 0;
		UINT8 r = ((color & 0x04) ? 0xC0 : 0) + i;
		UINT8 g = ((color & 0x02) ? 0xC0 : 0) + i;
		UINT8 b = ((color & 0x01) ? 0xC0 : 0) + i;
		palette_set_color( space->machine(), number, MAKE_RGB(r,g,b) );
	}
}

static READ8_HANDLER(text_r)
{
	pk8020_state *state = space->machine().driver_data<pk8020_state>();
	if (state->m_attr == 3) state->m_text_attr=ram_get_ptr(space->machine().device(RAM_TAG))[0x40400+offset];
	return ram_get_ptr(space->machine().device(RAM_TAG))[0x40000+offset];
}

static WRITE8_HANDLER(text_w)
{
	pk8020_state *state = space->machine().driver_data<pk8020_state>();
	UINT8 *ram = ram_get_ptr(space->machine().device(RAM_TAG));
	ram[0x40000+offset] = data;
	switch (state->m_attr) {
		case 0: break;
		case 1: ram[0x40400+offset]=0x01;break;
		case 2: ram[0x40400+offset]=0x00;break;
		case 3: ram[0x40400+offset]=state->m_text_attr;break;
	}
}

static READ8_HANDLER(gzu_r)
{
	pk8020_state *state = space->machine().driver_data<pk8020_state>();
	UINT8 *addr = ram_get_ptr(space->machine().device(RAM_TAG)) + 0x10000 + (state->m_video_page_access * 0xC000);
	UINT8 p0 = addr[offset];
	UINT8 p1 = addr[offset + 0x4000];
	UINT8 p2 = addr[offset + 0x8000];
	UINT8 retVal = 0;
	if(state->m_color & 0x80) {
		// Color mode
		if (!(state->m_color & 0x10)) {
			p0 ^= 0xff;
		}
		if (!(state->m_color & 0x20)) {
			p1 ^= 0xff;
		}
		if (!(state->m_color & 0x40)) {
			p2 ^= 0xff;
		}
		retVal = (p0 & p1 & p2) ^ 0xff;
	} else {
		// Plane mode
		if (state->m_color & 0x10) {
			retVal |= p0;
		}
		if (state->m_color & 0x20) {
			retVal |= p1;
		}
		if (state->m_color & 0x40) {
			retVal |= p2;
		}
	}
	return retVal;
}

static WRITE8_HANDLER(gzu_w)
{
	pk8020_state *state = space->machine().driver_data<pk8020_state>();
	UINT8 *addr = ram_get_ptr(space->machine().device(RAM_TAG)) + 0x10000 + (state->m_video_page_access * 0xC000);
	UINT8 *plane_0 = addr;
	UINT8 *plane_1 = addr + 0x4000;
	UINT8 *plane_2 = addr + 0x8000;

	if(state->m_color & 0x80)
	{
		// Color mode
		plane_0[offset] = (plane_0[offset] & ~data) | ((state->m_color & 2) ? data : 0);
		plane_1[offset] = (plane_1[offset] & ~data) | ((state->m_color & 4) ? data : 0);
		plane_2[offset] = (plane_2[offset] & ~data) | ((state->m_color & 8) ? data : 0);
	} else {
		// Plane mode
		UINT8 mask = (state->m_color & 1) ? data : 0;
		if (!(state->m_color & 0x02)) {
			plane_0[offset] = (plane_0[offset] & ~data) | mask;
		}
		if (!(state->m_color & 0x04)) {
			plane_1[offset] = (plane_1[offset] & ~data) | mask;
		}
		if (!(state->m_color & 0x08)) {
			plane_2[offset] = (plane_2[offset] & ~data) | mask;
		}
	}
}

static READ8_HANDLER(devices_r)
{
	device_t *ppi1 = space->machine().device("ppi8255_1");
	device_t *ppi2 = space->machine().device("ppi8255_2");
	device_t *ppi3 = space->machine().device("ppi8255_3");
	device_t *pit = space->machine().device("pit8253");
	device_t *pic = space->machine().device("pic8259");
	device_t *rs232 = space->machine().device("rs232");
	device_t *lan = space->machine().device("lan");
	device_t *fdc = space->machine().device("wd1793");

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
	device_t *ppi1 = space->machine().device("ppi8255_1");
	device_t *ppi2 = space->machine().device("ppi8255_2");
	device_t *ppi3 = space->machine().device("ppi8255_3");
	device_t *pit = space->machine().device("pit8253");
	device_t *pic = space->machine().device("pic8259");
	device_t *rs232 = space->machine().device("rs232");
	device_t *lan = space->machine().device("lan");
	device_t *fdc = space->machine().device("wd1793");

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

static void pk8020_set_bank(running_machine &machine,UINT8 data)
{
	address_space *space = machine.device("maincpu")->memory().space(AS_PROGRAM);
	UINT8 *mem = machine.region("maincpu")->base();
	UINT8 *ram = ram_get_ptr(machine.device(RAM_TAG));

	switch(data & 0x1F) {
		case 0x00 :
					{
						// ROM
						space->install_read_bank (0x0000, 0x37ff, "bank1");
						space->install_write_bank(0x0000, 0x37ff, "bank2");
						memory_set_bankptr(machine, "bank1", mem + 0x10000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// Keyboard
						space->install_legacy_read_handler (0x3800, 0x39ff, FUNC(keyboard_r));
						space->install_write_bank(0x3800, 0x39ff, "bank3");
						memory_set_bankptr(machine, "bank3", ram + 0x3800);
						// System reg
						space->install_legacy_read_handler (0x3a00, 0x3aff, FUNC(sysreg_r));
						space->install_legacy_write_handler(0x3a00, 0x3aff, FUNC(sysreg_w));
						// Devices
						space->install_legacy_read_handler (0x3b00, 0x3bff, FUNC(devices_r));
						space->install_legacy_write_handler(0x3b00, 0x3bff, FUNC(devices_w));
						// Text Video Memory
						space->install_legacy_read_handler (0x3c00, 0x3fff, FUNC(text_r));
						space->install_legacy_write_handler(0x3c00, 0x3fff, FUNC(text_w));
						// RAM
						space->install_read_bank (0x4000, 0xffff, "bank4");
						space->install_write_bank(0x4000, 0xffff, "bank5");
						memory_set_bankptr(machine, "bank4", ram + 0x4000);
						memory_set_bankptr(machine, "bank5", ram + 0x4000);
					}
					break;
		case 0x01 : {
						// ROM
						space->install_read_bank (0x0000, 0x1fff, "bank1");
						space->install_write_bank(0x0000, 0x1fff, "bank2");
						memory_set_bankptr(machine, "bank1", mem + 0x10000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// RAM
						space->install_read_bank (0x2000, 0xffff, "bank3");
						space->install_write_bank(0x2000, 0xffff, "bank4");
						memory_set_bankptr(machine, "bank3", ram + 0x2000);
						memory_set_bankptr(machine, "bank4", ram + 0x2000);
					}
					break;
		case 0x02 : {
						// ROM
						space->install_read_bank (0x0000, 0x3fff, "bank1");
						space->install_write_bank(0x0000, 0x3fff, "bank2");
						memory_set_bankptr(machine, "bank1", mem + 0x10000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// RAM
						space->install_read_bank (0x4000, 0xffff, "bank3");
						space->install_write_bank(0x4000, 0xffff, "bank4");
						memory_set_bankptr(machine, "bank3", ram + 0x4000);
						memory_set_bankptr(machine, "bank4", ram + 0x4000);
					}
					break;
		case 0x03 : {
						// RAM
						space->install_read_bank (0x0000, 0xffff, "bank1");
						space->install_write_bank(0x0000, 0xffff, "bank2");
						memory_set_bankptr(machine, "bank1", ram);
						memory_set_bankptr(machine, "bank2", ram);
					}
					break;
		case 0x04 :
		case 0x05 :
					{
						// ROM
						space->install_read_bank (0x0000, 0x1fff, "bank1");
						space->install_write_bank(0x0000, 0x1fff, "bank2");
						memory_set_bankptr(machine, "bank1", mem + 0x10000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// RAM
						space->install_read_bank (0x2000, 0xf7ff, "bank3");
						space->install_write_bank(0x2000, 0xf7ff, "bank4");
						memory_set_bankptr(machine, "bank3", ram + 0x2000);
						memory_set_bankptr(machine, "bank4", ram + 0x2000);
						// Keyboard
						space->install_legacy_read_handler (0xf800, 0xf9ff, FUNC(keyboard_r));
						space->install_write_bank(0xf800, 0xf9ff, "bank5");
						memory_set_bankptr(machine, "bank5", ram + 0xf800);
						// System reg
						space->install_legacy_read_handler (0xfa00, 0xfaff, FUNC(sysreg_r));
						space->install_legacy_write_handler(0xfa00, 0xfaff, FUNC(sysreg_w));
						// Devices
						space->install_legacy_read_handler (0xfb00, 0xfbff, FUNC(devices_r));
						space->install_legacy_write_handler(0xfb00, 0xfbff, FUNC(devices_w));
						// Text Video Memory
						space->install_legacy_read_handler (0xfc00, 0xffff, FUNC(text_r));
						space->install_legacy_write_handler(0xfc00, 0xffff, FUNC(text_w));
					}
					break;
		case 0x06 :
					{
						// ROM
						space->install_read_bank (0x0000, 0x3fff, "bank1");
						space->install_write_bank(0x0000, 0x3fff, "bank2");
						memory_set_bankptr(machine, "bank1", mem + 0x10000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// RAM
						space->install_read_bank (0x4000, 0xf7ff, "bank3");
						space->install_write_bank(0x4000, 0xf7ff, "bank4");
						memory_set_bankptr(machine, "bank3", ram + 0x4000);
						memory_set_bankptr(machine, "bank4", ram + 0x4000);
						// Keyboard
						space->install_legacy_read_handler (0xf800, 0xf9ff, FUNC(keyboard_r));
						space->install_write_bank(0xf800, 0xf9ff, "bank5");
						memory_set_bankptr(machine, "bank5", ram + 0xf800);
						// System reg
						space->install_legacy_read_handler (0xfa00, 0xfaff, FUNC(sysreg_r));
						space->install_legacy_write_handler(0xfa00, 0xfaff, FUNC(sysreg_w));
						// Devices
						space->install_legacy_read_handler (0xfb00, 0xfbff, FUNC(devices_r));
						space->install_legacy_write_handler(0xfb00, 0xfbff, FUNC(devices_w));
						// Text Video Memory
						space->install_legacy_read_handler (0xfc00, 0xffff, FUNC(text_r));
						space->install_legacy_write_handler(0xfc00, 0xffff, FUNC(text_w));
					}
					break;
		case 0x07 :
					{
						// RAM
						space->install_read_bank (0x0000, 0xf7ff, "bank1");
						space->install_write_bank(0x0000, 0xf7ff, "bank2");
						memory_set_bankptr(machine, "bank1", ram);
						memory_set_bankptr(machine, "bank2", ram);
						// Keyboard
						space->install_legacy_read_handler (0xf800, 0xf9ff, FUNC(keyboard_r));
						space->install_write_bank(0xf800, 0xf9ff, "bank3");
						memory_set_bankptr(machine, "bank3", ram + 0xf800);
						// System reg
						space->install_legacy_read_handler (0xfa00, 0xfaff, FUNC(sysreg_r));
						space->install_legacy_write_handler(0xfa00, 0xfaff, FUNC(sysreg_w));
						// Devices
						space->install_legacy_read_handler (0xfb00, 0xfbff, FUNC(devices_r));
						space->install_legacy_write_handler(0xfb00, 0xfbff, FUNC(devices_w));
						// Text Video Memory
						space->install_legacy_read_handler (0xfc00, 0xffff, FUNC(text_r));
						space->install_legacy_write_handler(0xfc00, 0xffff, FUNC(text_w));
					}
					break;
		case 0x08 :
					{
						// ROM
						space->install_read_bank (0x0000, 0x3fff, "bank1");
						space->install_write_bank(0x0000, 0x3fff, "bank2");
						memory_set_bankptr(machine, "bank1", mem + 0x10000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// Keyboard
						space->install_legacy_read_handler (0x3800, 0x39ff, FUNC(keyboard_r));
						space->install_write_bank(0x3800, 0x39ff, "bank3");
						memory_set_bankptr(machine, "bank3", ram + 0x3800);
						// System reg
						space->install_legacy_read_handler (0x3a00, 0x3aff, FUNC(sysreg_r));
						space->install_legacy_write_handler(0x3a00, 0x3aff, FUNC(sysreg_w));
						// Devices
						space->install_legacy_read_handler (0x3b00, 0x3bff, FUNC(devices_r));
						space->install_legacy_write_handler(0x3b00, 0x3bff, FUNC(devices_w));
						// Text Video Memory
						space->install_legacy_read_handler (0x3c00, 0x3fff, FUNC(text_r));
						space->install_legacy_write_handler(0x3c00, 0x3fff, FUNC(text_w));
						// RAM
						space->install_read_bank (0x4000, 0xbfff, "bank4");
						space->install_write_bank(0x4000, 0xbfff, "bank5");
						memory_set_bankptr(machine, "bank4", ram + 0x4000);
						memory_set_bankptr(machine, "bank5", ram + 0x4000);
						// Video RAM
						space->install_legacy_read_handler (0xc000, 0xffff, FUNC(gzu_r));
						space->install_legacy_write_handler(0xc000, 0xffff, FUNC(gzu_w));

					}
					break;
		case 0x09 :
					{
						// ROM
						space->install_read_bank (0x0000, 0x1fff, "bank1");
						space->install_write_bank(0x0000, 0x1fff, "bank2");
						memory_set_bankptr(machine, "bank1", mem + 0x10000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// RAM
						space->install_read_bank (0x2000, 0xbfff, "bank3");
						space->install_write_bank(0x2000, 0xbfff, "bank4");
						memory_set_bankptr(machine, "bank3", ram + 0x2000);
						memory_set_bankptr(machine, "bank4", ram + 0x2000);
						// Video RAM
						space->install_legacy_read_handler (0xc000, 0xffff, FUNC(gzu_r));
						space->install_legacy_write_handler(0xc000, 0xffff, FUNC(gzu_w));
					}
					break;
		case 0x0A :
					{
						// ROM
						space->install_read_bank (0x0000, 0x3fff, "bank1");
						space->install_write_bank(0x0000, 0x3fff, "bank2");
						memory_set_bankptr(machine, "bank1", mem + 0x10000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// RAM
						space->install_read_bank (0x4000, 0xbfff, "bank3");
						space->install_write_bank(0x4000, 0xbfff, "bank4");
						memory_set_bankptr(machine, "bank3", ram + 0x4000);
						memory_set_bankptr(machine, "bank4", ram + 0x4000);
						// Video RAM
						space->install_legacy_read_handler (0xc000, 0xffff, FUNC(gzu_r));
						space->install_legacy_write_handler(0xc000, 0xffff, FUNC(gzu_w));
					}
					break;
		case 0x0B :
					{
						// RAM
						space->install_read_bank (0x0000, 0xbfff, "bank1");
						space->install_write_bank(0x0000, 0xbfff, "bank2");
						memory_set_bankptr(machine, "bank1", ram + 0x0000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// Video RAM
						space->install_legacy_read_handler (0xc000, 0xffff, FUNC(gzu_r));
						space->install_legacy_write_handler(0xc000, 0xffff, FUNC(gzu_w));
					}
					break;
		case 0x0C :
		case 0x0D :
					{
						// ROM
						space->install_read_bank (0x0000, 0x1fff, "bank1");
						space->install_write_bank(0x0000, 0x1fff, "bank2");
						memory_set_bankptr(machine, "bank1", mem + 0x10000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// RAM
						space->install_read_bank (0x2000, 0x3fff, "bank3");
						space->install_write_bank(0x2000, 0x3fff, "bank4");
						memory_set_bankptr(machine, "bank3", ram + 0x2000);
						memory_set_bankptr(machine, "bank4", ram + 0x2000);
						// Video RAM
						space->install_legacy_read_handler (0x4000, 0x7fff, FUNC(gzu_r));
						space->install_legacy_write_handler(0x4000, 0x7fff, FUNC(gzu_w));
						// RAM
						space->install_read_bank (0x8000, 0xfdff, "bank5");
						space->install_write_bank(0x8000, 0xfdff, "bank6");
						memory_set_bankptr(machine, "bank5", ram + 0x8000);
						memory_set_bankptr(machine, "bank6", ram + 0x8000);
						// Devices
						space->install_legacy_read_handler (0xfe00, 0xfeff, FUNC(devices_r));
						space->install_legacy_write_handler(0xfe00, 0xfeff, FUNC(devices_w));
						// System reg
						space->install_legacy_read_handler (0xff00, 0xffff, FUNC(sysreg_r));
						space->install_legacy_write_handler(0xff00, 0xffff, FUNC(sysreg_w));
					}
					break;
		case 0x0E :
					{
						// ROM
						space->install_read_bank (0x0000, 0x3fff, "bank1");
						space->install_write_bank(0x0000, 0x3fff, "bank2");
						memory_set_bankptr(machine, "bank1", mem + 0x10000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// Video RAM
						space->install_legacy_read_handler (0x4000, 0x7fff, FUNC(gzu_r));
						space->install_legacy_write_handler(0x4000, 0x7fff, FUNC(gzu_w));
						// RAM
						space->install_read_bank (0x8000, 0xfdff, "bank5");
						space->install_write_bank(0x8000, 0xfdff, "bank6");
						memory_set_bankptr(machine, "bank5", ram + 0x8000);
						memory_set_bankptr(machine, "bank6", ram + 0x8000);
						// Devices
						space->install_legacy_read_handler (0xfe00, 0xfeff, FUNC(devices_r));
						space->install_legacy_write_handler(0xfe00, 0xfeff, FUNC(devices_w));
						// System reg
						space->install_legacy_read_handler (0xff00, 0xffff, FUNC(sysreg_r));
						space->install_legacy_write_handler(0xff00, 0xffff, FUNC(sysreg_w));
					}
					break;
		case 0x0F :
					{
						// RAM
						space->install_read_bank (0x0000, 0x3fff, "bank1");
						space->install_write_bank(0x0000, 0x3fff, "bank2");
						memory_set_bankptr(machine, "bank1", ram + 0x0000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// Video RAM
						space->install_legacy_read_handler (0x4000, 0x7fff, FUNC(gzu_r));
						space->install_legacy_write_handler(0x4000, 0x7fff, FUNC(gzu_w));
						// RAM
						space->install_read_bank (0x8000, 0xfdff, "bank3");
						space->install_write_bank(0x8000, 0xfdff, "bank4");
						memory_set_bankptr(machine, "bank3", ram + 0x8000);
						memory_set_bankptr(machine, "bank4", ram + 0x8000);
						// Devices
						space->install_legacy_read_handler (0xfe00, 0xfeff, FUNC(devices_r));
						space->install_legacy_write_handler(0xfe00, 0xfeff, FUNC(devices_w));
						// System reg
						space->install_legacy_read_handler (0xff00, 0xffff, FUNC(sysreg_r));
						space->install_legacy_write_handler(0xff00, 0xffff, FUNC(sysreg_w));
					}
					break;
		case 0x10 :
					{
						// ROM
						space->install_read_bank (0x0000, 0x5fff, "bank1");
						space->install_write_bank(0x0000, 0x5fff, "bank2");
						memory_set_bankptr(machine, "bank1", mem + 0x10000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// RAM
						space->install_read_bank (0x6000, 0xf7ff, "bank3");
						space->install_write_bank(0x6000, 0xf7ff, "bank4");
						memory_set_bankptr(machine, "bank3", ram + 0x6000);
						memory_set_bankptr(machine, "bank4", ram + 0x6000);
						// Keyboard
						space->install_legacy_read_handler (0xf800, 0xf9ff, FUNC(keyboard_r));
						space->install_write_bank(0xf800, 0xf9ff, "bank5");
						memory_set_bankptr(machine, "bank5", ram + 0xf800);
						// System reg
						space->install_legacy_read_handler (0xfa00, 0xfaff, FUNC(sysreg_r));
						space->install_legacy_write_handler(0xfa00, 0xfaff, FUNC(sysreg_w));
						// Devices
						space->install_legacy_read_handler (0xfb00, 0xfbff, FUNC(devices_r));
						space->install_legacy_write_handler(0xfb00, 0xfbff, FUNC(devices_w));
						// Text Video Memory
						space->install_legacy_read_handler (0xfc00, 0xffff, FUNC(text_r));
						space->install_legacy_write_handler(0xfc00, 0xffff, FUNC(text_w));
					}
					break;
		case 0x11 :
					{
						// ROM
						space->install_read_bank (0x0000, 0x1fff, "bank1");
						space->install_write_bank(0x0000, 0x1fff, "bank2");
						memory_set_bankptr(machine, "bank1", mem + 0x10000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// RAM
						space->install_read_bank (0x2000, 0xf7ff, "bank3");
						space->install_write_bank(0x2000, 0xf7ff, "bank4");
						memory_set_bankptr(machine, "bank3", ram + 0x2000);
						memory_set_bankptr(machine, "bank4", ram + 0x2000);
						// Keyboard
						space->install_legacy_read_handler (0xf800, 0xf9ff, FUNC(keyboard_r));
						space->install_write_bank(0xf800, 0xf9ff, "bank5");
						memory_set_bankptr(machine, "bank5", ram + 0xf800);
						// System reg
						space->install_legacy_read_handler (0xfa00, 0xfaff, FUNC(sysreg_r));
						space->install_legacy_write_handler(0xfa00, 0xfaff, FUNC(sysreg_w));
						// Devices
						space->install_legacy_read_handler (0xfb00, 0xfbff, FUNC(devices_r));
						space->install_legacy_write_handler(0xfb00, 0xfbff, FUNC(devices_w));
						// Text Video Memory
						space->install_legacy_read_handler (0xfc00, 0xffff, FUNC(text_r));
						space->install_legacy_write_handler(0xfc00, 0xffff, FUNC(text_w));
					}
					break;
		case 0x12 :
					{
						// ROM
						space->install_read_bank (0x0000, 0x3fff, "bank1");
						space->install_write_bank(0x0000, 0x3fff, "bank2");
						memory_set_bankptr(machine, "bank1", mem + 0x10000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// RAM
						space->install_read_bank (0x4000, 0xf7ff, "bank3");
						space->install_write_bank(0x4000, 0xf7ff, "bank4");
						memory_set_bankptr(machine, "bank3", ram + 0x4000);
						memory_set_bankptr(machine, "bank4", ram + 0x4000);
						// Keyboard
						space->install_legacy_read_handler (0xf800, 0xf9ff, FUNC(keyboard_r));
						space->install_write_bank(0xf800, 0xf9ff, "bank5");
						memory_set_bankptr(machine, "bank5", ram + 0xf800);
						// System reg
						space->install_legacy_read_handler (0xfa00, 0xfaff, FUNC(sysreg_r));
						space->install_legacy_write_handler(0xfa00, 0xfaff, FUNC(sysreg_w));
						// Devices
						space->install_legacy_read_handler (0xfb00, 0xfbff, FUNC(devices_r));
						space->install_legacy_write_handler(0xfb00, 0xfbff, FUNC(devices_w));
						// Text Video Memory
						space->install_legacy_read_handler (0xfc00, 0xffff, FUNC(text_r));
						space->install_legacy_write_handler(0xfc00, 0xffff, FUNC(text_w));
					}
					break;
		case 0x13 :
					{
						// RAM
						space->install_read_bank (0x0000, 0xf7ff, "bank1");
						space->install_write_bank(0x0000, 0xf7ff, "bank2");
						memory_set_bankptr(machine, "bank1", ram + 0x0000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// Keyboard
						space->install_legacy_read_handler (0xf800, 0xf9ff, FUNC(keyboard_r));
						space->install_write_bank(0xf800, 0xf9ff, "bank3");
						memory_set_bankptr(machine, "bank3", ram + 0xf800);
						// System reg
						space->install_legacy_read_handler (0xfa00, 0xfaff, FUNC(sysreg_r));
						space->install_legacy_write_handler(0xfa00, 0xfaff, FUNC(sysreg_w));
						// Devices
						space->install_legacy_read_handler (0xfb00, 0xfbff, FUNC(devices_r));
						space->install_legacy_write_handler(0xfb00, 0xfbff, FUNC(devices_w));
						// Text Video Memory
						space->install_legacy_read_handler (0xfc00, 0xffff, FUNC(text_r));
						space->install_legacy_write_handler(0xfc00, 0xffff, FUNC(text_w));
					}
					break;
		case 0x14 :
					{
						// ROM
						space->install_read_bank (0x0000, 0x5fff, "bank1");
						space->install_write_bank(0x0000, 0x5fff, "bank2");
						memory_set_bankptr(machine, "bank1", mem + 0x10000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// RAM
						space->install_read_bank (0x6000, 0xfdff, "bank3");
						space->install_write_bank(0x6000, 0xfdff, "bank4");
						memory_set_bankptr(machine, "bank3", ram + 0x6000);
						memory_set_bankptr(machine, "bank4", ram + 0x6000);
						// Devices
						space->install_legacy_read_handler (0xfe00, 0xfeff, FUNC(devices_r));
						space->install_legacy_write_handler(0xfe00, 0xfeff, FUNC(devices_w));
						// System reg
						space->install_legacy_read_handler (0xff00, 0xffff, FUNC(sysreg_r));
						space->install_legacy_write_handler(0xff00, 0xffff, FUNC(sysreg_w));
					}
					break;
		case 0x15 :
					{
						// ROM
						space->install_read_bank (0x0000, 0x1fff, "bank1");
						space->install_write_bank(0x0000, 0x1fff, "bank2");
						memory_set_bankptr(machine, "bank1", mem + 0x10000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// RAM
						space->install_read_bank (0x2000, 0xfdff, "bank3");
						space->install_write_bank(0x2000, 0xfdff, "bank4");
						memory_set_bankptr(machine, "bank3", ram + 0x2000);
						memory_set_bankptr(machine, "bank4", ram + 0x2000);
						// Devices
						space->install_legacy_read_handler (0xfe00, 0xfeff, FUNC(devices_r));
						space->install_legacy_write_handler(0xfe00, 0xfeff, FUNC(devices_w));
						// System reg
						space->install_legacy_read_handler (0xff00, 0xffff, FUNC(sysreg_r));
						space->install_legacy_write_handler(0xff00, 0xffff, FUNC(sysreg_w));
					}
					break;
		case 0x16 :
					{
						// ROM
						space->install_read_bank (0x0000, 0x3fff, "bank1");
						space->install_write_bank(0x0000, 0x3fff, "bank2");
						memory_set_bankptr(machine, "bank1", mem + 0x10000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// RAM
						space->install_read_bank (0x4000, 0xfdff, "bank3");
						space->install_write_bank(0x4000, 0xfdff, "bank4");
						memory_set_bankptr(machine, "bank3", ram + 0x4000);
						memory_set_bankptr(machine, "bank4", ram + 0x4000);
						// Devices
						space->install_legacy_read_handler (0xfe00, 0xfeff, FUNC(devices_r));
						space->install_legacy_write_handler(0xfe00, 0xfeff, FUNC(devices_w));
						// System reg
						space->install_legacy_read_handler (0xff00, 0xffff, FUNC(sysreg_r));
						space->install_legacy_write_handler(0xff00, 0xffff, FUNC(sysreg_w));
					}
					break;
		case 0x17 :
					{
						// RAM
						space->install_read_bank (0x0000, 0xfdff, "bank1");
						space->install_write_bank(0x0000, 0xfdff, "bank2");
						memory_set_bankptr(machine, "bank1", ram);
						memory_set_bankptr(machine, "bank2", ram);
						// Devices
						space->install_legacy_read_handler (0xfe00, 0xfeff, FUNC(devices_r));
						space->install_legacy_write_handler(0xfe00, 0xfeff, FUNC(devices_w));
						// System reg
						space->install_legacy_read_handler (0xff00, 0xffff, FUNC(sysreg_r));
						space->install_legacy_write_handler(0xff00, 0xffff, FUNC(sysreg_w));
					}
					break;
		case 0x18 :
					{
						// ROM
						space->install_read_bank (0x0000, 0x5fff, "bank1");
						space->install_write_bank(0x0000, 0x5fff, "bank2");
						memory_set_bankptr(machine, "bank1", mem + 0x10000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// RAM
						space->install_read_bank (0x6000, 0xbeff, "bank3");
						space->install_write_bank(0x6000, 0xbeff, "bank4");
						memory_set_bankptr(machine, "bank3", ram + 0x6000);
						memory_set_bankptr(machine, "bank4", ram + 0x6000);
						// System reg
						space->install_legacy_read_handler (0xbf00, 0xbfff, FUNC(sysreg_r));
						space->install_legacy_write_handler(0xbf00, 0xbfff, FUNC(sysreg_w));
						// Video RAM
						space->install_legacy_read_handler (0xc000, 0xffff, FUNC(gzu_r));
						space->install_legacy_write_handler(0xc000, 0xffff, FUNC(gzu_w));
					}
					break;
		case 0x19 :
					{
						// ROM
						space->install_read_bank (0x0000, 0x1fff, "bank1");
						space->install_write_bank(0x0000, 0x1fff, "bank2");
						memory_set_bankptr(machine, "bank1", mem + 0x10000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// RAM
						space->install_read_bank (0x2000, 0xbeff, "bank3");
						space->install_write_bank(0x2000, 0xbeff, "bank4");
						memory_set_bankptr(machine, "bank3", ram + 0x2000);
						memory_set_bankptr(machine, "bank4", ram + 0x2000);
						// System reg
						space->install_legacy_read_handler (0xbf00, 0xbfff, FUNC(sysreg_r));
						space->install_legacy_write_handler(0xbf00, 0xbfff, FUNC(sysreg_w));
						// Video RAM
						space->install_legacy_read_handler (0xc000, 0xffff, FUNC(gzu_r));
						space->install_legacy_write_handler(0xc000, 0xffff, FUNC(gzu_w));
					}
					break;
		case 0x1A :
					{
						// ROM
						space->install_read_bank (0x0000, 0x3fff, "bank1");
						space->install_write_bank(0x0000, 0x3fff, "bank2");
						memory_set_bankptr(machine, "bank1", mem + 0x10000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// RAM
						space->install_read_bank (0x4000, 0xbeff, "bank3");
						space->install_write_bank(0x4000, 0xbeff, "bank4");
						memory_set_bankptr(machine, "bank3", ram + 0x4000);
						memory_set_bankptr(machine, "bank4", ram + 0x4000);
						// System reg
						space->install_legacy_read_handler (0xbf00, 0xbfff, FUNC(sysreg_r));
						space->install_legacy_write_handler(0xbf00, 0xbfff, FUNC(sysreg_w));
						// Video RAM
						space->install_legacy_read_handler (0xc000, 0xffff, FUNC(gzu_r));
						space->install_legacy_write_handler(0xc000, 0xffff, FUNC(gzu_w));
					}
					break;
		case 0x1B :
					{
						// RAM
						space->install_read_bank (0x0000, 0xbeff, "bank1");
						space->install_write_bank(0x0000, 0xbeff, "bank2");
						memory_set_bankptr(machine, "bank1", ram);
						memory_set_bankptr(machine, "bank2", ram);
						// System reg
						space->install_legacy_read_handler (0xbf00, 0xbfff, FUNC(sysreg_r));
						space->install_legacy_write_handler(0xbf00, 0xbfff, FUNC(sysreg_w));
						// Video RAM
						space->install_legacy_read_handler (0xc000, 0xffff, FUNC(gzu_r));
						space->install_legacy_write_handler(0xc000, 0xffff, FUNC(gzu_w));
					}
					break;
		case 0x1C :
					{
						// ROM
						space->install_read_bank (0x0000, 0x5fff, "bank1");
						space->install_write_bank(0x0000, 0x5fff, "bank2");
						memory_set_bankptr(machine, "bank1", mem + 0x10000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// RAM
						space->install_read_bank (0x6000, 0xbfff, "bank3");
						space->install_write_bank(0x6000, 0xbfff, "bank4");
						memory_set_bankptr(machine, "bank3", ram + 0x6000);
						memory_set_bankptr(machine, "bank4", ram + 0x6000);
						// Video RAM
						space->install_legacy_read_handler (0xc000, 0xffff, FUNC(gzu_r));
						space->install_legacy_write_handler(0xc000, 0xffff, FUNC(gzu_w));
					}
					break;
		case 0x1D :
					{
						// ROM
						space->install_read_bank (0x0000, 0x1fff, "bank1");
						space->install_write_bank(0x0000, 0x1fff, "bank2");
						memory_set_bankptr(machine, "bank1", mem + 0x10000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// RAM
						space->install_read_bank (0x2000, 0xbfff, "bank3");
						space->install_write_bank(0x2000, 0xbfff, "bank4");
						memory_set_bankptr(machine, "bank3", ram + 0x2000);
						memory_set_bankptr(machine, "bank4", ram + 0x2000);
						// Video RAM
						space->install_legacy_read_handler (0xc000, 0xffff, FUNC(gzu_r));
						space->install_legacy_write_handler(0xc000, 0xffff, FUNC(gzu_w));
					}
					break;
		case 0x1E :
					{
						// ROM
						space->install_read_bank (0x0000, 0x3fff, "bank1");
						space->install_write_bank(0x0000, 0x3fff, "bank2");
						memory_set_bankptr(machine, "bank1", mem + 0x10000);
						memory_set_bankptr(machine, "bank2", ram + 0x0000);
						// RAM
						space->install_read_bank (0x4000, 0xbfff, "bank3");
						space->install_write_bank(0x4000, 0xbfff, "bank4");
						memory_set_bankptr(machine, "bank3", ram + 0x4000);
						memory_set_bankptr(machine, "bank4", ram + 0x4000);
						// Video RAM
						space->install_legacy_read_handler (0xc000, 0xffff, FUNC(gzu_r));
						space->install_legacy_write_handler(0xc000, 0xffff, FUNC(gzu_w));
					}
					break;
		case 0x1F :
					{
						// RAM
						space->install_read_bank (0x0000, 0xbfff, "bank1");
						space->install_write_bank(0x0000, 0xbfff, "bank2");
						memory_set_bankptr(machine, "bank1", ram);
						memory_set_bankptr(machine, "bank2", ram);
						// Video RAM
						space->install_legacy_read_handler (0xc000, 0xffff, FUNC(gzu_r));
						space->install_legacy_write_handler(0xc000, 0xffff, FUNC(gzu_w));
					}
					break;

	}
}

static READ8_DEVICE_HANDLER(pk8020_porta_r)
{
	pk8020_state *state = device->machine().driver_data<pk8020_state>();
	return 0xf0 | (state->m_takt <<1) | (state->m_text_attr)<<3;
}

static WRITE8_DEVICE_HANDLER(pk8020_portc_w)
{
	pk8020_state *state = device->machine().driver_data<pk8020_state>();
	state->m_video_page_access =(data>>6) & 3;
	state->m_attr = (data >> 4) & 3;
	state->m_wide = (data >> 3) & 1;
	state->m_font = (data >> 2) & 1;
	state->m_video_page = (data & 3);


	state->m_portc_data = data;
}

static WRITE8_DEVICE_HANDLER(pk8020_portb_w)
{
	device_t *fdc = device->machine().device("wd1793");
	// Turn all motors off
	floppy_mon_w(floppy_get_device(device->machine(), 0), 1);
	floppy_mon_w(floppy_get_device(device->machine(), 1), 1);
	floppy_mon_w(floppy_get_device(device->machine(), 2), 1);
	floppy_mon_w(floppy_get_device(device->machine(), 3), 1);
	wd17xx_set_side(fdc,BIT(data,4));
	if (BIT(data,0)) {
		wd17xx_set_drive(fdc,0);
		floppy_mon_w(floppy_get_device(device->machine(), 0), 0);
		floppy_drive_set_ready_state(floppy_get_device(device->machine(), 0), 1, 1);
	} else if (BIT(data,1)) {
		wd17xx_set_drive(fdc,1);
		floppy_mon_w(floppy_get_device(device->machine(), 1), 0);
		floppy_drive_set_ready_state(floppy_get_device(device->machine(), 1), 1, 1);
	} else if (BIT(data,2)) {
		wd17xx_set_drive(fdc,2);
		floppy_mon_w(floppy_get_device(device->machine(), 2), 0);
		floppy_drive_set_ready_state(floppy_get_device(device->machine(), 2), 1, 1);
	} else if (BIT(data,3)) {
		wd17xx_set_drive(fdc,3);
		floppy_mon_w(floppy_get_device(device->machine(), 3), 0);
		floppy_drive_set_ready_state(floppy_get_device(device->machine(), 3), 1, 1);
	}
}

static READ8_DEVICE_HANDLER(pk8020_portc_r)
{
	pk8020_state *state = device->machine().driver_data<pk8020_state>();
	return state->m_portc_data;
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
	pk8020_state *state = device->machine().driver_data<pk8020_state>();
	device_t *speaker = device->machine().device("speaker");

	state->m_sound_gate = BIT(data,3);

	speaker_level_w(speaker, state->m_sound_gate ? state->m_sound_level : 0);
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

static WRITE_LINE_DEVICE_HANDLER( pk8020_pit_out0 )
{
	pk8020_state *drvstate = device->machine().driver_data<pk8020_state>();
	device_t *speaker = device->machine().device("speaker");

	drvstate->m_sound_level = state;

	speaker_level_w(speaker, drvstate->m_sound_gate ? drvstate->m_sound_level : 0);
}


static WRITE_LINE_DEVICE_HANDLER(pk8020_pit_out1)
{
}


const struct pit8253_config pk8020_pit8253_intf =
{
	{
		{
			XTAL_20MHz / 10,
			DEVCB_NULL,
			DEVCB_LINE(pk8020_pit_out0)
		},
		{
			XTAL_20MHz / 10,
			DEVCB_NULL,
			DEVCB_LINE(pk8020_pit_out1)
		},
		{
			(XTAL_20MHz / 8) / 164,
			DEVCB_NULL,
			DEVCB_DEVICE_LINE("pic8259", pic8259_ir5_w)
		}
	}
};

static WRITE_LINE_DEVICE_HANDLER( pk8020_pic_set_int_line )
{
	cputag_set_input_line(device->machine(), "maincpu", 0, state ? HOLD_LINE : CLEAR_LINE);
}

const struct pic8259_interface pk8020_pic8259_config =
{
	DEVCB_LINE(pk8020_pic_set_int_line)
};

static IRQ_CALLBACK( pk8020_irq_callback )
{
	return pic8259_acknowledge(device->machine().device("pic8259"));
}

MACHINE_RESET( pk8020 )
{
	pk8020_state *state = machine.driver_data<pk8020_state>();
	pk8020_set_bank(machine,0);
	device_set_irq_callback(machine.device("maincpu"), pk8020_irq_callback);

	state->m_sound_gate = 0;
	state->m_sound_level = 0;
}

INTERRUPT_GEN( pk8020_interrupt )
{
	pk8020_state *state = device->machine().driver_data<pk8020_state>();
	state->m_takt ^= 1;
	pic8259_ir4_w(device->machine().device("pic8259"), 1);
}
