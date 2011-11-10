/***************************************************************************

    IQ151 Aritma Minigraf 0507 module emulation

***************************************************************************/

#include "emu.h"
#include "iq151_minigraf.h"


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

ROM_START( iq151_minigraf )
	ROM_REGION(0x1000, "minigraf", 0)
	ROM_LOAD( "minigraf_010787.rom", 0x0000, 0x0800, CRC(d854d203) SHA1(ae19c2859f8d78fda227a74ab50c6eb095d14014))
	ROM_LOAD( "minigraf_050986.rom", 0x0800, 0x0800, CRC(e0559e9e) SHA1(475d294e4976f88ad13e77a39b1c607827c791dc))
ROM_END

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type IQ151_MINIGRAF = &device_creator<iq151_minigraf_device>;

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  iq151_minigraf_device - constructor
//-------------------------------------------------

iq151_minigraf_device::iq151_minigraf_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
      : device_t(mconfig, IQ151_MINIGRAF, "IQ151 Minigraf", tag, owner, clock),
		device_iq151cart_interface( mconfig, *this )
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void iq151_minigraf_device::device_start()
{
	m_rom = (UINT8*)subregion("minigraf")->base();
}

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *iq151_minigraf_device::device_rom_region() const
{
	return ROM_NAME( iq151_minigraf );
}

//-------------------------------------------------
//  read
//-------------------------------------------------

void iq151_minigraf_device::read(offs_t offset, UINT8 &data)
{
	// interal ROM is mapped at 0xc000-0xc7ff
	if (offset >= 0xc000 && offset < 0xc800)
		data = m_rom[offset & 0x7ff];
}

//-------------------------------------------------
//  IO write
//-------------------------------------------------

void iq151_minigraf_device::io_write(offs_t offset, UINT8 data)
{
	if (offset >= 0xf0 && offset < 0xf4)
	{
		// Plotter control lines
	}
}
