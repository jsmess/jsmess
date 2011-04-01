/***************************************************************************

    Sharp PC-E220

    preliminary driver by Angelo Salese

    Notes:
    - checks for a "machine language" string at some point, nothing in the
      current dump match it

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/ram.h"


class pce220_state : public driver_device
{
public:
	pce220_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_bank_num;
	UINT16 m_lcd_index_row;
	UINT16 m_lcd_index_col;
	UINT8 m_lcd_timer;
};



static VIDEO_START( pce220 )
{
}

static SCREEN_UPDATE( pce220 )
{
	int x, y, xi,yi;
	int count = 0;
//  static int test_x,test_y;
	UINT8 *vram = screen->machine().region("lcd_vram")->base();

	for (y = 0; y < 4; y++)
	{
		for (x = 0; x < 24; x++)
		{
			int color;

			for (xi = 0; xi < 5; xi++)
			{
				for (yi = 0; yi < 8; yi++)
				{
					color = (vram[count]) >> (yi) & 1;

					//if ((x + xi) <= screen->visible_area()->max_x && (y + 0) < screen->visible_area()->max_y)
						*BITMAP_ADDR32(bitmap, y*8 + yi, x*5 + xi) = screen->machine().pens[color ^ 1];
				}

				count++;
			}
		}
	}
    return 0;
}

static READ8_HANDLER( lcd_status_r )
{
	return 0; //bit 7 lcd busy
}

static WRITE8_HANDLER( lcd_control_w )
{
	pce220_state *state = space->machine().driver_data<pce220_state>();
	if((data & 0xb8) == 0xb8)
		state->m_lcd_index_row = (data & 0x07);
	if((data & 0x40) == 0x40)
		state->m_lcd_index_col = (data & 0x3f);
}

static WRITE8_HANDLER( lcd_data_w )
{
	pce220_state *state = space->machine().driver_data<pce220_state>();
	UINT8 *vram = space->machine().region("lcd_vram")->base();

	vram[state->m_lcd_index_row*0x40|state->m_lcd_index_col] = data;

	gfx_element_mark_dirty(space->machine().gfx[0], (state->m_lcd_index_row*0x40|state->m_lcd_index_col)/5);
	state->m_lcd_index_col++;
}

static READ8_HANDLER( rom_bank_r )
{
	pce220_state *state = space->machine().driver_data<pce220_state>();
	return state->m_bank_num;
}

static WRITE8_HANDLER( rom_bank_w )
{
	pce220_state *state = space->machine().driver_data<pce220_state>();
	UINT8 bank2 = data & 0x07; // bits 0,1,2
	UINT8 bank1 = (data & 0x70) >> 4; // bits 4,5,6

	state->m_bank_num = data;

	memory_set_bankptr(space->machine(), "bank3", space->machine().region("user1")->base() + 0x4000 * bank1);
	memory_set_bankptr(space->machine(), "bank4", space->machine().region("user1")->base() + 0x4000 * bank2);
}

static WRITE8_HANDLER( ram_bank_w )
{
	address_space *space_prg = space->machine().device("maincpu")->memory().space(AS_PROGRAM);
	UINT8 bank = BIT(data,2);
	space_prg->install_write_bank(0x0000, 0x3fff, "bank1");

	memory_set_bankptr(space->machine(), "bank1", ram_get_ptr(space->machine().device(RAM_TAG))+0x0000+bank*0x8000);
	memory_set_bankptr(space->machine(), "bank2", ram_get_ptr(space->machine().device(RAM_TAG))+0x4000+bank*0x8000);
}

static ADDRESS_MAP_START(pce220_mem, AS_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_RAMBANK("bank1")
	AM_RANGE(0x4000, 0x7fff) AM_RAMBANK("bank2")
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK("bank3")
	AM_RANGE(0xc000, 0xffff) AM_ROMBANK("bank4")
ADDRESS_MAP_END

static READ8_HANDLER( battery_status_r )
{
	return 0; //if bit 0 is active, battery is low
}

/* TODO */

static READ8_HANDLER( timer_lcd_r )
{
	return space->machine().rand() & 1;
}

static WRITE8_HANDLER( timer_lcd_w )
{
	pce220_state *state = space->machine().driver_data<pce220_state>();
	state->m_lcd_timer = data & 1;
}

static READ8_HANDLER( port15_r )
{
	/*
    x--- ---- XIN input enabled
    ---- ---0
    */
	return 0;
}

static READ8_HANDLER( port18_r )
{
	/*
    x--- ---- XOUT/TXD
    ---- --x- DOUT
    ---- ---x BUSY/CTS
    */
	return 0;
}

static READ8_HANDLER( port1f_r )
{
	/*
    x--- ---- ON - resp. break key status (?)
    ---- -x-- XIN/RXD
    ---- --x- ACK/RTS
    ---- ---x DIN
    */

	return 0;
}

static ADDRESS_MAP_START( pce220_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x10, 0x10) AM_READNOP //key matrix r
	AM_RANGE(0x11, 0x12) AM_WRITENOP //key matrix mux
	AM_RANGE(0x13, 0x13) AM_READNOP //bit 0 = shift key
	AM_RANGE(0x14, 0x14) AM_READ(timer_lcd_r) AM_WRITE(timer_lcd_w)
	AM_RANGE(0x15, 0x15) AM_READ(port15_r) AM_WRITENOP
	AM_RANGE(0x16, 0x16) AM_NOP // irq status / acks
	AM_RANGE(0x17, 0x17) AM_WRITENOP // irq mask
	AM_RANGE(0x18, 0x18) AM_READ(port18_r) AM_WRITENOP
	AM_RANGE(0x19, 0x19) AM_READ(rom_bank_r) AM_WRITE(rom_bank_w)
	AM_RANGE(0x1a, 0x1a) AM_WRITENOP //cleared on BASIC init
	AM_RANGE(0x1b, 0x1b) AM_WRITE(ram_bank_w)
	AM_RANGE(0x1c, 0x1c) AM_WRITENOP //peripheral reset
	AM_RANGE(0x1d, 0x1d) AM_READ(battery_status_r)
	AM_RANGE(0x1e, 0x1e) AM_WRITENOP //???
	AM_RANGE(0x1f, 0x1f) AM_READ(port1f_r) AM_WRITENOP
	AM_RANGE(0x58, 0x58) AM_WRITE(lcd_control_w)
	AM_RANGE(0x59, 0x59) AM_READ(lcd_status_r)
	AM_RANGE(0x5a, 0x5a) AM_WRITE(lcd_data_w)

ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pce220 )
INPUT_PORTS_END


static MACHINE_RESET(pce220)
{
	address_space *space = machine.device("maincpu")->memory().space(AS_PROGRAM);
	space->unmap_write(0x0000, 0x3fff);
	memory_set_bankptr(machine, "bank1", machine.region("user1")->base() + 0x0000);
}

static const gfx_layout test_decode =
{
	5,7,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8 },
	{ 7, 6, 5, 4, 3, 2, 1 },
	5*8
};

/* decoded for debugging purpose, this will be nuked in the end... */
static GFXDECODE_START( pce220 )
	GFXDECODE_ENTRY( "lcd_vram",   0x00000, test_decode,    0, 1 )
GFXDECODE_END

static MACHINE_CONFIG_START( pce220, pce220_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",Z80, 3072000 ) // CMOS-SC7852
    MCFG_CPU_PROGRAM_MAP(pce220_mem)
    MCFG_CPU_IO_MAP(pce220_io)

    MCFG_MACHINE_RESET(pce220)

    /* video hardware */
	// 4 lines x 24 characters, resp. 144 x 32 pixel
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
    MCFG_SCREEN_SIZE(32*8, 32*8)
    MCFG_SCREEN_VISIBLE_AREA(0, 32*8-1, 0, 32*8-1)
    MCFG_SCREEN_UPDATE(pce220)

    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)
	MCFG_GFXDECODE(pce220)

    MCFG_VIDEO_START(pce220)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("64K") // 32K internal + 32K external card
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( pce220 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_REGION( 0x20000, "user1", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "v1", "v 0.1")
	ROM_SYSTEM_BIOS( 1, "v2", "v 0.2")
	ROM_LOAD( "bank0.bin",      0x0000, 0x4000, CRC(1fa94d11) SHA1(24c54347dbb1423388360a359aa09db47d2057b7))
	ROM_LOAD( "bank1.bin",      0x4000, 0x4000, CRC(0f9864b0) SHA1(6b7301c96f1a865e1931d82872a1ed5d1f80644e))
	ROM_LOAD( "bank2.bin",      0x8000, 0x4000, CRC(1625e958) SHA1(090440600d461aa7efe4adbf6e975aa802aabeec))
	ROM_LOAD( "bank3.bin",      0xc000, 0x4000, CRC(ed9a57f8) SHA1(58087dc64103786a40325c0a1e04bd88bfd6da57))
	ROM_LOAD( "bank4.bin",     0x10000, 0x4000, CRC(e37665ae) SHA1(85f5c84f69f79e7ac83b30397b2a1d9629f9eafa))
	ROMX_LOAD( "bank5.bin",     0x14000, 0x4000, CRC(6b116e7a) SHA1(b29f5a070e846541bddc88b5ee9862cc36b88eee),ROM_BIOS(2))
	ROMX_LOAD( "bank5_0.1.bin", 0x14000, 0x4000, CRC(13c26eb4) SHA1(b9cd0efd6b195653b9610e20ad8aab541824a689),ROM_BIOS(1))
	ROMX_LOAD( "bank6.bin",     0x18000, 0x4000, CRC(4fbfbd18) SHA1(e5aab1df172dcb94aa90e7d898eacfc61157ff15),ROM_BIOS(2))
	ROMX_LOAD( "bank6_0.1.bin", 0x18000, 0x4000, CRC(e2cda7a6) SHA1(01b1796d9485fde6994cb5afbe97514b54cfbb3a),ROM_BIOS(1))
	ROMX_LOAD( "bank7.bin",     0x1c000, 0x4000, CRC(5e98b5b6) SHA1(f22d74d6a24f5929efaf2983caabd33859232a94),ROM_BIOS(2))
	ROMX_LOAD( "bank7_0.1.bin", 0x1c000, 0x4000, CRC(d8e821b2) SHA1(18245a75529d2f496cdbdc28cdf40def157b20c0),ROM_BIOS(1))

	ROM_REGION( 0x1000, "lcd_vram", ROMREGION_ERASE00)
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT COMPANY   FULLNAME       FLAGS */
COMP( 1991, pce220,  0,       0,	pce220, 	pce220,  0,   "Sharp",   "PC-E220",		GAME_NOT_WORKING | GAME_NO_SOUND)

