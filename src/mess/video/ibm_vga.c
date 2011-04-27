/***************************************************************************

    Video Graphics Adapter (VGA) section

***************************************************************************/

#include "emu.h"
#include "ibm_vga.h"
#include "video/mc6845.h"

#define VGA_SCREEN_NAME "vga_screen"
#define VGA_MC6845_NAME "vga_mc6845"

#define LOG_ACCESSES (1)

#define CRTC_PORT_ADDR ((m_misc_out_reg & 1) ? 0x3d0 : 0x3b0 )

UINT8 ibm_vga_device::vga_crtc_r(int offset)
{
	int data = 0xff;
	switch( offset )
	{
		case 4:
			/* return last written mc6845 address value here? */
			break;
		case 5:
			data = mc6845_register_r( m_crtc, offset );
			break;
    }
	return data;

}

void ibm_vga_device::vga_crtc_w(int offset, UINT8 data)
{
	switch(offset)
	{
		case 4:
			mc6845_address_w( m_crtc, offset, data );
			break;
		case 5:
			mc6845_register_w( m_crtc, offset, data );
			break;
	}
}

READ8_MEMBER(ibm_vga_device::vga_port_03b0_r)
{
	UINT8 retVal = 0xff;
	if (CRTC_PORT_ADDR == 0x3b0)
		retVal = vga_crtc_r(offset);
	//if (LOG_ACCESSES)
		//logerror("vga_port_03b0_r(): port=0x%04x data=0x%02x\n", offset + 0x3b0, retVal);
	return retVal;
}

WRITE8_MEMBER( ibm_vga_device::vga_port_03b0_w )
{
	//if (LOG_ACCESSES)
		//logerror("vga_port_03b0_w(): port=0x%04x data=0x%02x\n", offset + 0x3b0, data);
	if (CRTC_PORT_ADDR == 0x3b0)
		vga_crtc_w(offset, data);
}

READ8_MEMBER( ibm_vga_device::vga_port_03c0_r)
{
	UINT8 retVal = 0xff;

	switch(offset) {
		case 0x01:  // Attribute controller
					if (m_attr_state==0)
					{
						retVal = m_attr_idx;
					}
					else
					{
						retVal = m_attr_data[m_attr_idx & 0x1f];
					}
					break;
		case 0x04 : // Sequencer Index Register
					retVal = m_seq_idx;
					break;
		case 0x05 : // Sequencer Data Register
					retVal = m_seq_data[m_seq_idx & 0x1f];
					break;
		case 0x06 : // Video DAC Pel Mask Register
					retVal = m_dac_mask;
					break;
		case 0x07 : // Video DAC Index (CLUT Reads)
					if (m_dac_read)
						retVal = 0;
					else
						retVal = 3;
					break;
		case 0x08 : // Video DAC Index (CLUT Writes)
					retVal = m_dac_write_idx;
					break;
		case 0x09 : // Video DAC Data Register
					if (m_dac_read)
					{
						retVal = m_dac_color[m_dac_state++][m_dac_read_idx];
						if (m_dac_state==3)
						{
							m_dac_state = 0;
							m_dac_read_idx++;
						}
					}
					break;
		case 0x0a : //
					retVal = m_feature_ctrl;
		case 0x0c : // Miscellaneous Output Register
					retVal = m_misc_out_reg;
					break;
		case 0x0e : // Graphics Index Register
					retVal = m_gc_idx;
					break;
		case 0x0f : // Graphics Data Register
					retVal = m_gc_data[m_gc_idx & 0x1f];
					break;
	}

	if (LOG_ACCESSES)
		logerror("vga_port_03c0_r(): port=0x%04x data=0x%02x\n", offset + 0x3c0, retVal);
	return retVal;
}

WRITE8_MEMBER( ibm_vga_device::vga_port_03c0_w )
{
	if (LOG_ACCESSES)
		logerror("vga_port_03c0_w(): port=0x%04x data=0x%02x\n", offset + 0x3c0, data);

	switch(offset) {
		case 0x00:  // Attribute Controller
					if (m_attr_state==0)
					{
						m_attr_idx = data;
					}
					else
					{
						m_attr_data[m_attr_idx & 0x1f] = data;
					}
					m_attr_state=!m_attr_state;
					break;
		case 0x02 : // Miscellaneous Output Register
					m_misc_out_reg = data;
					break;
		case 0x04 : // Sequencer Index Register
					m_seq_idx = data;
					break;
		case 0x05 : // Sequencer Data Register
					m_seq_data[m_seq_idx & 0x1f] = data;
					break;
		case 0x06 : // Video DAC Pel Mask Register
					m_dac_mask = data;
					break;
		case 0x07 : // Video DAC Index (CLUT Reads)
					m_dac_read_idx = data;
					m_dac_state = 0;
					m_dac_read = 1;
					break;
		case 0x08 : // Video DAC Index (CLUT Writes)
					m_dac_write_idx = data;
					m_dac_state = 0;
					m_dac_read = 0;
					break;
		case 0x09 : // Video DAC Data Register
					if (!m_dac_read)
					{
						m_dac_color[m_dac_state++][m_dac_write_idx] = data;
						if (m_dac_state==3) {
							m_dac_state=0;
							m_dac_write_idx++;
						}
					}
					break;
		case 0x0e : // Graphics Index Register
					m_gc_idx = data;
					break;
		case 0x0f : // Graphics Data Register
					m_gc_data[m_gc_idx & 0x1f] = data;
					break;
	}
}

READ8_MEMBER( ibm_vga_device::vga_port_03d0_r)
{
	UINT8 retVal = 0xff;
	if (CRTC_PORT_ADDR == 0x3d0)
		retVal = vga_crtc_r(offset);
	//if (LOG_ACCESSES)
//      logerror("vga_port_03d0_r(): port=0x%04x data=0x%02x\n", offset + 0x3d0, retVal);
	return retVal;
}

WRITE8_MEMBER( ibm_vga_device::vga_port_03d0_w )
{
//  if (LOG_ACCESSES)
		//logerror("vga_port_03d0_w(): port=0x%04x data=0x%02x\n", offset + 0x3d0, data);
	if (CRTC_PORT_ADDR == 0x3d0)
		vga_crtc_w(offset, data);
}


static MC6845_UPDATE_ROW( vga_update_row )
{
}

static const mc6845_interface mc6845_vga_intf =
{
	VGA_SCREEN_NAME,	/* screen number */
	8,					/* numbers of pixels per video memory address */
	NULL,				/* begin_update */
	vga_update_row,		/* update_row */
	NULL,				/* end_update */
	DEVCB_NULL,			/* on_de_changed */
	DEVCB_NULL,			/* on_cur_changed */
	DEVCB_NULL,			/* on_hsync_changed */
	DEVCB_NULL,			/* on_vsync_changed */
	NULL
};

static SCREEN_UPDATE( mc6845_vga )
{
	device_t *devconf = screen->owner()->subdevice(VGA_MC6845_NAME);
	mc6845_update( devconf, bitmap, cliprect);
	return 0;
}

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type IBM_VGA = &device_creator<ibm_vga_device>;

static MACHINE_CONFIG_FRAGMENT( ibm_vga )
	MCFG_SCREEN_ADD(VGA_SCREEN_NAME, RASTER)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_RAW_PARAMS(XTAL_14_31818MHz,912,0,640,262,0,200)
	MCFG_SCREEN_UPDATE( mc6845_vga )

	MCFG_MC6845_ADD(VGA_MC6845_NAME, MC6845, XTAL_14_31818MHz/8, mc6845_vga_intf)
MACHINE_CONFIG_END

ROM_START( ibm_vga )
	ROM_REGION(0x10000, "vga", ROMREGION_LOADBYNAME)
	ROM_LOAD("8222554.bin", 0x00000, 0x6000, CRC(6c12d745) SHA1(c0156607100e377c6c9563f23967409dab3ab92b))
ROM_END


//-------------------------------------------------
//  rom_region - return a pointer to the device's
//  internal ROM region
//-------------------------------------------------

const rom_entry *ibm_vga_device::device_rom_region() const
{
	return ROM_NAME(ibm_vga);
}

//-------------------------------------------------
//  machine_config_additions - return a pointer to
//  the device's machine fragment
//-------------------------------------------------

machine_config_constructor ibm_vga_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME(ibm_vga);
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  ibm_vga_device - constructor
//-------------------------------------------------

ibm_vga_device::ibm_vga_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, IBM_VGA, "IBM VGA", tag, owner, clock),
	  m_crtc(*this, VGA_MC6845_NAME)
{
	m_shortname = "ibm_vga";
}

/*-------------------------------------------------
    device_start - device-specific startup
-------------------------------------------------*/

void ibm_vga_device::device_start()
{
	int buswidth;
	astring tempstring;
	address_space *spaceio = machine().firstcpu->memory().space(AS_IO);
	address_space *space = machine().firstcpu->memory().space(AS_PROGRAM);
	space->install_rom(0xc0000, 0xc5fff, machine().region(subtag(tempstring, "vga"))->base());
	buswidth = machine().firstcpu->memory().space_config(AS_PROGRAM)->m_databus_width;
	switch(buswidth)
	{
		case 8:
			spaceio->install_readwrite_handler(0x3b0, 0x3bf, read8_delegate(FUNC(ibm_vga_device::vga_port_03b0_r), this), write8_delegate(FUNC(ibm_vga_device::vga_port_03b0_w), this));
			spaceio->install_readwrite_handler(0x3c0, 0x3cf, read8_delegate(FUNC(ibm_vga_device::vga_port_03c0_r), this), write8_delegate(FUNC(ibm_vga_device::vga_port_03c0_w), this));
			spaceio->install_readwrite_handler(0x3d0, 0x3df, read8_delegate(FUNC(ibm_vga_device::vga_port_03d0_r), this), write8_delegate(FUNC(ibm_vga_device::vga_port_03d0_w), this));
			break;

		case 16:
			spaceio->install_readwrite_handler(0x3b0, 0x3bf, read8_delegate(FUNC(ibm_vga_device::vga_port_03b0_r), this), write8_delegate(FUNC(ibm_vga_device::vga_port_03b0_w), this), 0xffff);
			spaceio->install_readwrite_handler(0x3c0, 0x3cf, read8_delegate(FUNC(ibm_vga_device::vga_port_03c0_r), this), write8_delegate(FUNC(ibm_vga_device::vga_port_03c0_w), this), 0xffff);
			spaceio->install_readwrite_handler(0x3d0, 0x3df, read8_delegate(FUNC(ibm_vga_device::vga_port_03d0_r), this), write8_delegate(FUNC(ibm_vga_device::vga_port_03d0_w), this), 0xffff);
			break;

		case 32:
			break;

		case 64:
			break;

		default:
			fatalerror("VGA: Bus width %d not supported", buswidth);
			break;
	}
	m_videoram = auto_alloc_array(machine(), UINT8, 0x40000);
	space->install_readwrite_bank(0xa0000, 0xbffff, "vga_bank" );
	memory_set_bankptr(machine(),"vga_bank", m_videoram);
}

/*-------------------------------------------------
    device_reset - device-specific reset
-------------------------------------------------*/

void ibm_vga_device::device_reset()
{
	m_misc_out_reg = 0;
	m_feature_ctrl = 0;

	m_seq_idx = 0;
	memset(m_seq_data,0xff,0x1f);

	m_attr_idx = 0;
	memset(m_attr_data,0xff,0x1f);
	m_attr_state = 0;

	m_gc_idx = 0;
	memset(m_gc_data,0xff,0x1f);

	m_dac_mask = 0;
	m_dac_write_idx = 0;
	m_dac_read_idx = 0;
	m_dac_read = 0;
	m_dac_state = 0;
	memset(m_dac_color,0,sizeof(m_dac_color));

	for (int i = 0; i < 0x100; i++)
		palette_set_color_rgb(machine(), i, 0, 0, 0);
}
