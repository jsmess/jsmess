/***************************************************************************

    CIDCO MailStation

    22/10/2011 Skeleton driver.

    Hardware:
        - Z80 CPU
        - 29f080 8 Mbit flash
        - 28SF040 4 Mbit flash (for user data)
        - 128kb RAM
        - 320x128 LCD
        - RCV336ACFW 33.6kbps modem

    TODO:
    - Everything

    More info:
      http://www.fybertech.net/mailstation/info.php

****************************************************************************/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/intelfsh.h"
#include "machine/rp5c01.h"
#include "machine/ram.h"
#include "rendlay.h"

class mstation_state : public driver_device
{
public:
	mstation_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, "maincpu")
		{ }

	required_device<cpu_device> m_maincpu;

	intelfsh8_device *m_flashes[2];
	UINT8 m_bank1[2];
	UINT8 m_bank2[2];
	UINT8 *m_ram_base;
	int m_flash_at_0x4000;
	int m_flash_at_0x8000;

	DECLARE_READ8_MEMBER( flash_0x0000_read_handler );
	DECLARE_WRITE8_MEMBER( flash_0x0000_write_handler );
	DECLARE_READ8_MEMBER( flash_0x4000_read_handler );
	DECLARE_WRITE8_MEMBER( flash_0x4000_write_handler );
	DECLARE_READ8_MEMBER( flash_0x8000_read_handler );
	DECLARE_WRITE8_MEMBER( flash_0x8000_write_handler );

	void refresh_memory(UINT8 bank, UINT8 chip_select);
	DECLARE_READ8_MEMBER( bank1_r );
	DECLARE_WRITE8_MEMBER( bank1_w );
	DECLARE_READ8_MEMBER( bank2_r );
	DECLARE_WRITE8_MEMBER( bank2_w );

	virtual void machine_start();
	virtual void machine_reset();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
};

READ8_MEMBER( mstation_state::flash_0x0000_read_handler )
{
	return m_flashes[0]->read(offset);
}

WRITE8_MEMBER( mstation_state::flash_0x0000_write_handler )
{
	m_flashes[0]->write(offset, data);
}

READ8_MEMBER( mstation_state::flash_0x4000_read_handler )
{
	return m_flashes[m_flash_at_0x4000]->read(((m_bank1[0] & 0x3f)<<14) | offset);
}

WRITE8_MEMBER( mstation_state::flash_0x4000_write_handler )
{
	m_flashes[m_flash_at_0x4000]->write(((m_bank1[0] & 0x3f)<<14) | offset, data);
}

READ8_MEMBER( mstation_state::flash_0x8000_read_handler )
{
	return m_flashes[m_flash_at_0x8000]->read(((m_bank2[0] & 0x3f)<<14) | offset);
}

WRITE8_MEMBER( mstation_state::flash_0x8000_write_handler )
{
	m_flashes[m_flash_at_0x8000]->write(((m_bank2[0] & 0x3f)<<14) | offset, data);
}

bool mstation_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	return 0;
}

//***************************************************************************
//  Bankswitch
//***************************************************************************/

void mstation_state::refresh_memory(UINT8 bank, UINT8 chip_select)
{
	address_space* program = m_maincpu->memory().space(AS_PROGRAM);
	int &active_flash = (bank == 1 ? m_flash_at_0x4000 : m_flash_at_0x8000);
	char bank_tag[6];

	switch (chip_select)
	{
		case 0:	// flash 0
		case 3:	// flash 1
			if (active_flash < 0)
			{
				if (bank == 1)
					program->install_readwrite_handler(0x4000, 0x7fff, 0, 0, read8_delegate(FUNC(mstation_state::flash_0x4000_read_handler), this), write8_delegate(FUNC(mstation_state::flash_0x4000_write_handler), this));
				else
					program->install_readwrite_handler(0x8000, 0xbfff, 0, 0, read8_delegate(FUNC(mstation_state::flash_0x8000_read_handler), this), write8_delegate(FUNC(mstation_state::flash_0x8000_write_handler), this));
			}

			active_flash = chip_select ? 1 : 0;
			break;
		case 1: // banked RAM
			sprintf(bank_tag,"bank%d", bank);
			memory_set_bankptr(machine(), bank_tag, m_ram_base + (((bank == 1 ? m_bank1[0] : m_bank2[0]) & 0x07)<<14));
			program->install_readwrite_bank (bank * 0x4000, bank * 0x4000 + 0x3fff, bank_tag);
			active_flash = -1;
			break;
		default:
			program->unmap_readwrite(bank * 0x4000, bank * 0x4000 + 0x3fff);
			active_flash = -1;
			break;
	}
}

READ8_MEMBER( mstation_state::bank1_r )
{
	return m_bank1[offset];
}

READ8_MEMBER( mstation_state::bank2_r )
{
	return m_bank2[offset];
}

WRITE8_MEMBER( mstation_state::bank1_w )
{
	m_bank1[offset] = data;

	refresh_memory(1, m_bank1[1] & 0x07);
}

WRITE8_MEMBER( mstation_state::bank2_w )
{
	m_bank2[offset] = data;

	refresh_memory(2, m_bank2[1] & 0x07);
}


static ADDRESS_MAP_START(mstation_mem, AS_PROGRAM, 8, mstation_state)
	AM_RANGE(0x0000, 0x3fff) AM_READWRITE(flash_0x0000_read_handler, flash_0x0000_write_handler)
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE_BANK("bank1")		// bank 1
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE_BANK("bank2")		// bank 2
	AM_RANGE(0xc000, 0xffff) AM_READWRITE_BANK("sysram")	// system ram always first RAM bank
ADDRESS_MAP_END

static ADDRESS_MAP_START(mstation_io , AS_IO, 8, mstation_state)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE( 0x05, 0x06 ) AM_READWRITE(bank1_r, bank1_w)
	AM_RANGE( 0x07, 0x08 ) AM_READWRITE(bank2_r, bank2_w)
	AM_RANGE( 0x10, 0x1f ) AM_DEVREADWRITE("rtc", rp5c01_device, read, write)
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( mstation )
INPUT_PORTS_END

void mstation_state::machine_start()
{
	m_flashes[0] = machine().device<intelfsh8_device>("flash0");
	m_flashes[1] = machine().device<intelfsh8_device>("flash1");

	m_ram_base = (UINT8*)ram_get_ptr(machine().device(RAM_TAG));

	// map firsh RAM bank at 0xc000-0xffff
	memory_set_bankptr(machine(), "sysram", m_ram_base);
}


void mstation_state::machine_reset()
{
	m_flash_at_0x4000 = 0;
	m_flash_at_0x8000 = 0;
	m_bank1[0] =  m_bank1[1] = 0;
	m_bank2[0] =  m_bank2[1] = 0;

	// reset banks
	refresh_memory(1, m_bank1[1]);
	refresh_memory(2, m_bank2[1]);
}

static RP5C01_INTERFACE( rtc_intf )
{
	DEVCB_NULL
};

static MACHINE_CONFIG_START( mstation, mstation_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)		//unknown clock
    MCFG_CPU_PROGRAM_MAP(mstation_mem)
    MCFG_CPU_IO_MAP(mstation_io)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(320, 128)
    MCFG_SCREEN_VISIBLE_AREA(0, 320-1, 0, 128-1)

    MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)
	MCFG_DEFAULT_LAYOUT(layout_lcd)

	MCFG_AMD_29F080_ADD("flash0")
	MCFG_AMD_29F080_ADD("flash1")	//SST-28SF040

	MCFG_RP5C01_ADD("rtc", XTAL_32_768kHz, rtc_intf)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("128K")
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( mstation )
	ROM_REGION( 0x100000, "flash0", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "v303a", "v3.03a" )
	ROMX_LOAD( "ms303a.bin", 0x000000, 0x100000, CRC(7a5cf752) SHA1(15629ccaecd8094dd883987bed94c16eee6de7c2), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "v253", "v2.53" )
	ROMX_LOAD( "ms253.bin",  0x000000, 0x0fc000, BAD_DUMP CRC(a27e7f8b) SHA1(ae5a0aa0f1e23f3b183c5c0bcf4d4c1ae54b1798), ROM_BIOS(2))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT COMPANY   FULLNAME       FLAGS */
COMP( 1999, mstation,  0,       0,	mstation,	mstation,  0,   "CIDCO",   "MailStation",		GAME_NOT_WORKING | GAME_NO_SOUND )
