/***************************************************************************

    IQ151 MS151A XY plotter module emulation

***************************************************************************/

#include "emu.h"
#include "iq151_ms151a.h"


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

ROM_START( iq151_ms151a )
	ROM_REGION(0x0800, "ms151a", 0)
	ROM_LOAD( "ms151a.rom", 0x0000, 0x0800, CRC(995c58d6) SHA1(ebdc4278cfe6d3cc7dafbaa05bc6c239e4e6c09b))
ROM_END

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type IQ151_MS151A = &device_creator<iq151_ms151a_device>;

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  iq151_ms151a_device - constructor
//-------------------------------------------------

iq151_ms151a_device::iq151_ms151a_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
      : device_t(mconfig, IQ151_MS151A, "IQ151 MS151A", tag, owner, clock),
		device_iq151cart_interface( mconfig, *this )
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void iq151_ms151a_device::device_start()
{
	m_rom = (UINT8*)subregion("ms151a")->base();
}

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *iq151_ms151a_device::device_rom_region() const
{
	return ROM_NAME( iq151_ms151a );
}

//-------------------------------------------------
//  read
//-------------------------------------------------

void iq151_ms151a_device::read(offs_t offset, UINT8 &data)
{
	// interal ROM is mapped at 0xc000-0xc7ff
	if (offset >= 0xc000 && offset < 0xc800)
		data = m_rom[offset & 0x7ff];
}

//-------------------------------------------------
//  IO read
//-------------------------------------------------

void iq151_ms151a_device::io_read(offs_t offset, UINT8 &data)
{

}

//-------------------------------------------------
//  IO write
//-------------------------------------------------

void iq151_ms151a_device::io_write(offs_t offset, UINT8 data)
{

}
