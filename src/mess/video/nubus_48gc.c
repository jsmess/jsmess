/***************************************************************************

  Apple 4*8 Graphics Card (model 630-0400) emulation

***************************************************************************/

#include "emu.h"
#include "video/nubus_48gc.h"

#define GC48_SCREEN_NAME	"48gc_screen"
#define GC48_ROM_REGION		"48gc_rom"

static SCREEN_UPDATE( mac_48gc );

MACHINE_CONFIG_FRAGMENT( macvideo_48gc )
	MCFG_SCREEN_ADD( GC48_SCREEN_NAME, RASTER)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MCFG_SCREEN_UPDATE( mac_48gc )
	MCFG_SCREEN_RAW_PARAMS(25175000, 800, 0, 640, 525, 0, 480)
MACHINE_CONFIG_END

ROM_START( gc48 )
	ROM_REGION(0x8000, GC48_ROM_REGION, 0)
	ROM_LOAD( "3410801.bin",  0x0000, 0x8000, CRC(e283da91) SHA1(4ae21d6d7bbaa6fc7aa301bee2b791ed33b1dcf9) )
ROM_END

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type NUBUS_48GC = &device_creator<nubus_48gc_device>;


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor nubus_48gc_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( macvideo_48gc );
}

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *nubus_48gc_device::device_rom_region() const
{
	return ROM_NAME( gc48 );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  nubus_48gc_device - constructor
//-------------------------------------------------

nubus_48gc_device::nubus_48gc_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, NUBUS_48GC, "NUBUS_48GC", tag, owner, clock),
		device_nubus_card_interface(mconfig, *this),
		device_slot_card_interface(mconfig, *this)
{
	m_shortname = "nb_48gc";
}

nubus_48gc_device::nubus_48gc_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, type, name, tag, owner, clock),
		device_nubus_card_interface(mconfig, *this),
		device_slot_card_interface(mconfig, *this)
{
	m_shortname = "nb_48gc";
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void nubus_48gc_device::device_start()
{
	UINT32 slotspace;

	// set_nubus_device makes m_slot valid
	set_nubus_device();
	install_declaration_rom(GC48_ROM_REGION);

	slotspace = get_slotspace();

	m_vram = auto_alloc_array(machine(), UINT8, 0x7ffff);
	m_nubus->install_device(slotspace+0x200000, slotspace+0x2003ff, read32_delegate(FUNC(nubus_48gc_device::mac_48gc_r), this), write32_delegate(FUNC(nubus_48gc_device::mac_48gc_w), this));
	m_nubus->install_bank(slotspace, slotspace+0x7ffff, 0, 0x07000, "bank_48gc", m_vram);

	m_toggle = 0;
	m_clutoffs = 0;
	m_count = 0;
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void nubus_48gc_device::device_reset()
{
}

/***************************************************************************

  Monochrome Display Adapter (MDA) section

***************************************************************************/

static SCREEN_UPDATE( mac_48gc )
{
	nubus_48gc_device *card = downcast<nubus_48gc_device *>(screen->owner());
	UINT32 *scanline;
	int x, y;
	UINT8 *vram8 = (UINT8 *)card->m_vram;
	UINT8 pixels;
//	UINT32 stride = card->m_registers[0xc/4]/4;

	vram8 += 0xa00;

	for (y = 0; y < 480; y++)
	{
		scanline = BITMAP_ADDR32(bitmap, y, 0);
		for (x = 0; x < 640/8; x++)
		{
			pixels = vram8[(y * 0x50) + (BYTE4_XOR_BE(x))];

			*scanline++ = card->m_palette[pixels&0x80];
			*scanline++ = card->m_palette[(pixels<<1)&0x80];
			*scanline++ = card->m_palette[(pixels<<2)&0x80];
			*scanline++ = card->m_palette[(pixels<<3)&0x80];
			*scanline++ = card->m_palette[(pixels<<4)&0x80];
			*scanline++ = card->m_palette[(pixels<<5)&0x80];
			*scanline++ = card->m_palette[(pixels<<6)&0x80];
			*scanline++ = card->m_palette[(pixels<<7)&0x80];
		}
	}

	return 0;
}

WRITE32_MEMBER( nubus_48gc_device::mac_48gc_w )
{
	printf("48gc_w: %08x to %x, mask %08x\n", data, offset*4, mem_mask);

	COMBINE_DATA(&m_registers[offset&0xff]);

	switch (offset)
	{
		case 0x80:	// DAC control
			m_clutoffs = data>>24;
			m_count = 0;
			break;

		case 0x81:	// DAC data
			m_colors[m_count++] = data>>24;

			if (m_count == 3)
			{
				m_palette[m_clutoffs] = MAKE_RGB(m_colors[0], m_colors[1], m_colors[2]);
				m_clutoffs++;
				m_count = 0;
			}
			break;

		default:
			break;
	}
}

READ32_MEMBER( nubus_48gc_device::mac_48gc_r )
{
	printf("48gc_r: @ %x, mask %08x [PC=%x]\n", offset, mem_mask, cpu_get_pc(machine().device("maincpu")));

	switch (offset)
	{
		case 0:
			return 0x0c00;	// sense 13" RGB for now
			break;

		case 0x1c0/4:
			m_toggle ^= 0xffffffff;
			return m_toggle;
			break;
	}

	return 0;
}
