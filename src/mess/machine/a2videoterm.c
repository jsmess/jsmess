/*********************************************************************

    a2videoterm.c

    Implementation of the Videx VideoTerm 80-column card
 
    Notes (from Videoterm user's manual, which contains
           schematics and firmware source listings).
 
    C0nX: C0n0 is 6845 register address,
          C0n1 is 6845 register data.
 
          Bits 2 & 3 on any access to C0nX set the VRAM page at CC00.
 
    C800-CBFF: ROM
    CC00-CDFF: VRAM window
 
    TODO:
    Cursor is probably not completely right.
    Add font ROM select.
 
*********************************************************************/

#include "a2videoterm.h"
#include "includes/apple2.h"


/***************************************************************************
    PARAMETERS
***************************************************************************/

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type A2BUS_VIDEOTERM = &device_creator<a2bus_videoterm_device>;

#define VIDEOTERM_ROM_REGION  "vterm_rom"
#define VIDEOTERM_GFX_REGION  "vterm_gfx"
#define VIDEOTERM_SCREEN_NAME "vterm_screen"
#define VIDEOTERM_MC6845_NAME "mc6845_vterm"

#define MDA_CLOCK	16257000

static MC6845_UPDATE_ROW( videoterm_update_row );

static const mc6845_interface mc6845_mda_intf =
{
	VIDEOTERM_SCREEN_NAME, /* screen number */
	8,					/* number of pixels per video memory address */
	NULL,				/* begin_update */
	videoterm_update_row,		/* update_row */
	NULL,				/* end_update */
	DEVCB_NULL,				/* on_de_changed */
	DEVCB_NULL,				/* on_cur_changed */
    DEVCB_NULL,         /* on hsync changed */
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, a2bus_videoterm_device, vsync_changed),	/* on_vsync_changed */
	NULL
};

MACHINE_CONFIG_FRAGMENT( a2videoterm )
	MCFG_SCREEN_ADD( VIDEOTERM_SCREEN_NAME, RASTER) // 560x216?  (80x24 7x9 characters)
	MCFG_SCREEN_RAW_PARAMS(MDA_CLOCK, 882, 0, 720, 370, 0, 350 )
	MCFG_SCREEN_UPDATE_DEVICE( VIDEOTERM_MC6845_NAME, mc6845_device, screen_update )

	MCFG_MC6845_ADD( VIDEOTERM_MC6845_NAME, MC6845, MDA_CLOCK/9, mc6845_mda_intf)
MACHINE_CONFIG_END

ROM_START( a2videoterm )
	ROM_REGION(0x400, VIDEOTERM_ROM_REGION, 0)
    ROM_LOAD( "videx videoterm rom 2.4.bin", 0x000000, 0x000400, CRC(bbe3bb28) SHA1(bb653836e84850ce3197f461d4e19355f738cfbf) ) 

	ROM_REGION(0x5000, VIDEOTERM_GFX_REGION, 0)
    ROM_LOAD( "videx videoterm character rom normal.bin", 0x000000, 0x000800, CRC(87f89f08) SHA1(410b54f33d13c82e3857f1be906d93a8c5b8d321) ) 
    ROM_LOAD( "videx videoterm character rom normal uppercase.bin", 0x000800, 0x000800, CRC(3d94a7a4) SHA1(5518254f24bc945aab13bc71ecc9526d6dd8e033) ) 
    ROM_LOAD( "videx videoterm character rom apl.bin", 0x001000, 0x000800, CRC(1adb704e) SHA1(a95df910eca33188cacee333b1325aa47edbcc25) ) 
    ROM_LOAD( "videx videoterm character rom epson.bin", 0x001800, 0x000800, CRC(0c6ef8d0) SHA1(db72c0c120086f1aa4a87120c5d7993c4a9d3a18) ) 
    ROM_LOAD( "videx videoterm character rom french.bin", 0x002000, 0x000800, CRC(266aa837) SHA1(2c6b4e9d342dbb2de8e278740f11925a9d8c6616) ) 
    ROM_LOAD( "videx videoterm character rom german.bin", 0x002800, 0x000800, CRC(df7324fa) SHA1(0ce58d2ffadbebc8db9f85bbb9a08a4f142af682) ) 
    ROM_LOAD( "videx videoterm character rom katakana.bin", 0x003000, 0x000800, CRC(b728690e) SHA1(e018fa66b0ff560313bb35757c9ce7adecae0c3a) ) 
    ROM_LOAD( "videx videoterm character rom spanish.bin", 0x003800, 0x000800, CRC(439eac08) SHA1(d6f9f8eb7702440d9ae39129ea4f480b80fc4608) ) 
    ROM_LOAD( "videx videoterm character rom super and subscript.bin", 0x004000, 0x000800, CRC(08b7c538) SHA1(7f4029d97be05680fe695debe07cea07666419e0) ) 
    ROM_LOAD( "videx videoterm character rom symbol.bin", 0x004800, 0x000800, CRC(82bce582) SHA1(29dfa8c5257dbf25651c6bffa9cdb453482aa70e) ) 
ROM_END

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor a2bus_videoterm_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( a2videoterm );
}

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *a2bus_videoterm_device::device_rom_region() const
{
	return ROM_NAME( a2videoterm );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

a2bus_videoterm_device::a2bus_videoterm_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock) :
    device_t(mconfig, type, name, tag, owner, clock),
    device_a2bus_card_interface(mconfig, *this),
    m_crtc(*this, VIDEOTERM_MC6845_NAME)
{
	m_shortname = "a2vidtrm";
}

a2bus_videoterm_device::a2bus_videoterm_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
    device_t(mconfig, A2BUS_VIDEOTERM, "Videx VideoTerm", tag, owner, clock),
    device_a2bus_card_interface(mconfig, *this),
    m_crtc(*this, VIDEOTERM_MC6845_NAME)
{
	m_shortname = "a2vidtrm";
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void a2bus_videoterm_device::device_start()
{
	// set_a2bus_device makes m_slot valid
	set_a2bus_device();

	astring tempstring;
	m_rom = device().machine().root_device().memregion(this->subtag(tempstring, VIDEOTERM_ROM_REGION))->base();

	astring tempstring2;
	m_chrrom = device().machine().root_device().memregion(this->subtag(tempstring2, VIDEOTERM_GFX_REGION))->base();

    memset(m_ram, 0, 4*512);

    save_item(NAME(m_ram));
    save_item(NAME(m_framecnt));
    save_item(NAME(m_rambank));
}

void a2bus_videoterm_device::device_reset()
{
    m_rambank = 0;
    m_framecnt = 0;
}


/*-------------------------------------------------
    read_c0nx - called for reads from this card's c0nx space
-------------------------------------------------*/

UINT8 a2bus_videoterm_device::read_c0nx(address_space &space, UINT8 offset)
{
//    printf("Read c0n%x (PC=%x)\n", offset, cpu_get_pc(&space.device()));

    m_rambank = ((offset>>2) & 3) * 512;

    if (offset == 1)
    {
        return m_crtc->register_r(space, offset);   // status_r?
    }

	return 0xff;
}


/*-------------------------------------------------
    write_c0nx - called for writes to this card's c0nx space
-------------------------------------------------*/

void a2bus_videoterm_device::write_c0nx(address_space &space, UINT8 offset, UINT8 data)
{
//    printf("Write %02x to c0n%x (PC=%x)\n", data, offset, cpu_get_pc(&space.device()));

    if (offset == 0)
    {
        m_crtc->address_w(space, offset, data);
    }
    else if (offset == 1)
    {
        m_crtc->register_w(space, offset, data);
    }

    m_rambank = ((offset>>2) & 3) * 512;
}

/*-------------------------------------------------
    read_cnxx - called for reads from this card's cnxx space
-------------------------------------------------*/

UINT8 a2bus_videoterm_device::read_cnxx(address_space &space, UINT8 offset)
{
    // one slot image at the start of the ROM, it appears
    return m_rom[offset];
}

/*-------------------------------------------------
    write_cnxx - called for writes to this card's cnxx space
    the firmware writes here to switch in our $C800 a lot
-------------------------------------------------*/
void a2bus_videoterm_device::write_cnxx(address_space &space, UINT8 offset, UINT8 data)
{
}

/*-------------------------------------------------
    read_c800 - called for reads from this card's c800 space
-------------------------------------------------*/

UINT8 a2bus_videoterm_device::read_c800(address_space &space, UINT16 offset)
{
    // ROM at c800-cbff
    // bankswitched RAM at cc00-cdff
    if (offset < 0x400)
    {
//        printf("Read VRAM at %x = %02x\n", offset+m_rambank, m_ram[offset + m_rambank]);
        return m_rom[offset];
    }
    else
    {
        return m_ram[(offset-0x400) + m_rambank];
    }
}

/*-------------------------------------------------
    write_c800 - called for writes to this card's c800 space
-------------------------------------------------*/
void a2bus_videoterm_device::write_c800(address_space &space, UINT16 offset, UINT8 data)
{
    if (offset >= 0x400)
    {
//        printf("%02x to VRAM at %x\n", data, offset-0x400+m_rambank);
        m_ram[(offset-0x400) + m_rambank] = data;
    }
}

static MC6845_UPDATE_ROW( videoterm_update_row )
{
	a2bus_videoterm_device	*vterm  = downcast<a2bus_videoterm_device *>(device->owner());
	const rgb_t *palette = palette_entry_list_raw(bitmap.palette());
	UINT32	*p = &bitmap.pix32(y);
	UINT16	chr_base = ra; //( ra & 0x08 ) ? 0x800 | ( ra & 0x07 ) : ra;
	int i;

	for ( i = 0; i < x_count; i++ )
	{
		UINT16 offset = ( ma + i ) & 0x7ff;
		UINT8 chr = vterm->m_ram[ offset ];
		UINT8 data = vterm->m_chrrom[ chr_base + chr * 16 ];
		UINT8 fg = 15;
		UINT8 bg = 0;

		if ( i == cursor_x )
		{
			if ( vterm->m_framecnt & 0x08 )
			{
				data = 0xFF;
			}
		}

		*p = palette[( data & 0x80 ) ? fg : bg]; p++;
		*p = palette[( data & 0x40 ) ? fg : bg]; p++;
		*p = palette[( data & 0x20 ) ? fg : bg]; p++;
		*p = palette[( data & 0x10 ) ? fg : bg]; p++;
		*p = palette[( data & 0x08 ) ? fg : bg]; p++;
		*p = palette[( data & 0x04 ) ? fg : bg]; p++;
		*p = palette[( data & 0x02 ) ? fg : bg]; p++;
		*p = palette[( data & 0x01 ) ? fg : bg]; p++;
	}
}

WRITE_LINE_MEMBER( a2bus_videoterm_device::vsync_changed )
{
	if ( state )
	{
		m_framecnt++;
	}
}

