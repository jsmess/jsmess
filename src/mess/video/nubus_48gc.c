/***************************************************************************

  Apple 4*8 Graphics Card (model 630-0400) emulation

***************************************************************************/

#include "emu.h"
#include "video/nubus_48gc.h"

#define GC48_SCREEN_NAME	"48gc_screen"
#define GC48_ROM_REGION		"48gc_rom"

#define VRAM_SIZE	(0x200000)	// 2 megs, maxed out

static SCREEN_UPDATE( mac_48gc );

MACHINE_CONFIG_FRAGMENT( macvideo_48gc )
	MCFG_SCREEN_ADD( GC48_SCREEN_NAME, RASTER)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MCFG_SCREEN_UPDATE( mac_48gc )
	MCFG_SCREEN_RAW_PARAMS(25175000, 800, 0, 640, 525, 0, 480)
//	MCFG_SCREEN_SIZE(1152, 870)
//	MCFG_SCREEN_VISIBLE_AREA(0, 1152-1, 0, 870-1)
//	MCFG_SCREEN_REFRESH_RATE(75)
//	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(1260))
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
	install_declaration_rom(this, GC48_ROM_REGION);

	slotspace = get_slotspace();

//	printf("[4*8gc %p] slotspace = %x\n", this, slotspace);

	m_vram = auto_alloc_array(machine(), UINT8, VRAM_SIZE);
	install_bank(slotspace, slotspace+VRAM_SIZE-1, 0, 0, "bank_48gc", m_vram);

	m_nubus->install_device(slotspace+0x200000, slotspace+0x2003ff, read32_delegate(FUNC(nubus_48gc_device::mac_48gc_r), this), write32_delegate(FUNC(nubus_48gc_device::mac_48gc_w), this));
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void nubus_48gc_device::device_reset()
{
	m_toggle = 0;
	m_clutoffs = 0;
	m_count = 0;
	m_vbl_disable = 1;
	m_stride = 80;
	m_base = 0;
	m_xres = 640;
	m_yres = 480;
	memset(m_vram, 0, VRAM_SIZE);
}

/***************************************************************************

  Apple 4*8 Graphics Card section

***************************************************************************/

static SCREEN_UPDATE( mac_48gc )
{
	nubus_48gc_device *card = downcast<nubus_48gc_device *>(screen->owner());
	UINT32 *scanline, *base;
	int x, y;
	UINT8 *vram8 = (UINT8 *)card->m_vram;
	UINT8 pixels;

	if (!card->m_vbl_disable)
	{
		card->m_nubus->set_irq_line(card->m_slot, ASSERT_LINE);
	}

	vram8 += 0xa00;

	switch (card->m_mode)
	{
		case 0:	// 1bpp
			for (y = 0; y < card->m_yres; y++)
			{
				scanline = BITMAP_ADDR32(bitmap, y, 0);
				for (x = 0; x < card->m_xres/8; x++)
				{
					pixels = vram8[(y * card->m_stride) + (BYTE4_XOR_BE(x))];

					*scanline++ = card->m_palette[(pixels>>7)&1];
					*scanline++ = card->m_palette[(pixels>>6)&1];
					*scanline++ = card->m_palette[(pixels>>5)&1];
					*scanline++ = card->m_palette[(pixels>>4)&1];
					*scanline++ = card->m_palette[(pixels>>3)&1];
					*scanline++ = card->m_palette[(pixels>>2)&1];
					*scanline++ = card->m_palette[(pixels>>1)&1];
					*scanline++ = card->m_palette[pixels&1];
				}
			}
			break;

		case 1:	// 2bpp
			for (y = 0; y < card->m_yres; y++)
			{
				scanline = BITMAP_ADDR32(bitmap, y, 0);
				for (x = 0; x < card->m_xres/4; x++)
				{
					pixels = vram8[(y * card->m_stride) + (BYTE4_XOR_BE(x))];

					*scanline++ = card->m_palette[(pixels>>6)&0x3];
					*scanline++ = card->m_palette[(pixels>>4)&0x3];
					*scanline++ = card->m_palette[(pixels>>2)&0x3];
					*scanline++ = card->m_palette[pixels&3];
				}
			}
			break;

		case 2: // 4 bpp
			for (y = 0; y < card->m_yres; y++)
			{
				scanline = BITMAP_ADDR32(bitmap, y, 0);

				for (x = 0; x < card->m_xres/2; x++)
				{
					pixels = vram8[(y * card->m_stride) + (BYTE4_XOR_BE(x))];

					*scanline++ = card->m_palette[(pixels>>4)&0xf];
					*scanline++ = card->m_palette[pixels&0xf];
				}
			}
			break;

		case 3: // 8 bpp
			for (y = 0; y < card->m_yres; y++)
			{
				scanline = BITMAP_ADDR32(bitmap, y, 0);

				for (x = 0; x < card->m_xres; x++)
				{
					pixels = vram8[(y * card->m_stride) + (BYTE4_XOR_BE(x))];
					*scanline++ = card->m_palette[pixels];
				}
			}
			break;

		case 4: // 24 bpp
			for (y = 0; y < card->m_yres; y++)
			{
				scanline = BITMAP_ADDR32(bitmap, y, 0);
				base = (UINT32 *)&card->m_vram[y * card->m_stride];
				for (x = 0; x < card->m_xres; x++)
				{
					*scanline++ = *base++;
				}
			}
			break;
	}

	return 0;
}

WRITE32_MEMBER( nubus_48gc_device::mac_48gc_w )
{
//	printf("48gc_w: %08x to %x, mask %08x\n", data, offset*4, mem_mask);

	COMBINE_DATA(&m_registers[offset&0xff]);

	switch (offset)
	{
		case 0x8/4:	// base
//			printf("%x to base\n", data);
			m_base = (data*2)<<4;
			break;

		case 0xc/4:	// stride
//			printf("%x to stride\n", data);
			// this value is in DWORDs for 1-8 bpp and, uhh, strange for 24bpp
			if (m_mode < 4)
			{
				m_stride = data*4;
			}
			else
			{
				m_stride = (data*32)/3;
			}
			break;

		case 0x200/4:	// DAC control
			m_clutoffs = data>>24;
			m_count = 0;
			break;

		case 0x204/4:	// DAC data
			m_colors[m_count++] = data>>24;

			if (m_count == 3)
			{
				m_palette[m_clutoffs] = MAKE_RGB(m_colors[0], m_colors[1], m_colors[2]);
				m_clutoffs++;
				m_count = 0;
			}
			break;

		case 0x208/4:	// mode control
			m_mode = (data>>3)&3;
			if (m_mode == 3)	// this can be 8 or 24 bpp
			{
				// check pixel format for 24bpp
				if (((data>>5)&3) == 0)
				{
					m_mode = 4;	// 24bpp
				}
			}
//			printf("%02x to mode (m_mode = %d)\n", data, m_mode);
			break;

		case 0x13c/4:	// bit 1 = VBL disable (1=no interrupts)
			m_vbl_disable = (data & 2) ? 1 : 0;
			break;

		case 0x148/4:	// write 1 here to clear interrupt
			if (data == 1)
			{
				m_nubus->set_irq_line(m_slot, CLEAR_LINE);
			}
			break;

		default:
			break;
	}
}

READ32_MEMBER( nubus_48gc_device::mac_48gc_r )
{
//	printf("48gc_r: @ %x, mask %08x [PC=%x]\n", offset, mem_mask, cpu_get_pc(machine().device("maincpu")));

	switch (offset)
	{
		case 0:
			return 0x0c00;	// sense 13" RGB for now
//			return 0x0000;	// sense "RGB Kong" monitor
			break;

		case 0x1c0/4:
			m_toggle ^= 0xffffffff;
			return m_toggle;
			break;
	}

	return 0;
}
