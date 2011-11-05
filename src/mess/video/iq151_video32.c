/***************************************************************************

    IQ151 video32 cartridge emulation

***************************************************************************/

#include "emu.h"
#include "iq151_video32.h"


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

ROM_START( iq151_video32 )
	ROM_REGION(0x0400, "chargen", ROMREGION_INVERT)
	ROM_LOAD( "iq151_video32font.rom", 0x0000, 0x0400, CRC(395567a7) SHA1(18800543daf4daed3f048193c6ae923b4b0e87db))

	ROM_REGION(0x0400, "videoram", ROMREGION_ERASE)
ROM_END

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type IQ151_VIDEO32 = &device_creator<iq151_video32_device>;

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  iq151_video32_device - constructor
//-------------------------------------------------

iq151_video32_device::iq151_video32_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
      : device_t(mconfig, IQ151_VIDEO32, "IQ151 video32", tag, owner, clock),
		device_iq151cart_interface( mconfig, *this )
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void iq151_video32_device::device_start()
{
	m_videoram = (UINT8*)subregion("videoram")->base();
	m_chargen = (UINT8*)subregion("chargen")->base();
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void iq151_video32_device::device_reset()
{
	screen_device *screen = machine().primary_screen;

	// if required adjust screen size
	if (screen->visible_area().max_x < 32*8 - 1)
		screen->set_visible_area(0, 32*8-1, 0, 32*8-1);
}

//-------------------------------------------------
//  device_rom_region
//-------------------------------------------------

const rom_entry *iq151_video32_device::device_rom_region() const
{
	return ROM_NAME( iq151_video32 );
}

//-------------------------------------------------
//  read
//-------------------------------------------------

void iq151_video32_device::read(offs_t offset, UINT8 &data)
{
	// videoram is mapped at 0xec00-0xefff
	if (offset >= 0xec00 && offset < 0xf000)
		data = m_videoram[offset & 0x3ff];
}

//-------------------------------------------------
//  write
//-------------------------------------------------

void iq151_video32_device::write(offs_t offset, UINT8 data)
{
	if (offset >= 0xec00 && offset < 0xf000)
		m_videoram[offset & 0x3ff] = data;
}

//-------------------------------------------------
//  video update
//-------------------------------------------------

void iq151_video32_device::video_update(bitmap_t &bitmap, const rectangle &cliprect)
{
	UINT16 ma = 0, sy = 0;

	for (int y = 0; y < 32; y++)
	{
		for (int ra = 0; ra < 8; ra++)
		{
			UINT16 *p = BITMAP_ADDR16(&bitmap, sy++, 0);

			for (int x = ma; x < ma + 32; x++)
			{
				UINT8 chr = m_videoram[x] & 0x7f; // rom only has 128 characters
				UINT8 gfx = m_chargen[(chr<<3) | ra ];

				// chars above 0x7f have colors inverted
				if (m_videoram[x] > 0x7f)
					gfx = ~gfx;

				/* Display a scanline of a character */
				*p++ |= BIT(gfx, 7);
				*p++ |= BIT(gfx, 6);
				*p++ |= BIT(gfx, 5);
				*p++ |= BIT(gfx, 4);
				*p++ |= BIT(gfx, 3);
				*p++ |= BIT(gfx, 2);
				*p++ |= BIT(gfx, 1);
				*p++ |= BIT(gfx, 0);
			}
		}
		ma += 32;
	}
}
