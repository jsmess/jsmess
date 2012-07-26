/***************************************************************************

        DEC Rainbow 100

        Driver-in-progress by R. Belmont and Miodrag Milanovic

****************************************************************************/

#include "emu.h"
#include "cpu/i86/i86.h"
#include "cpu/z80/z80.h"
#include "video/vtvideo.h"
#include "machine/wd17xx.h"
#include "imagedev/flopdrv.h"
#include "machine/i8251.h"

class rainbow_state : public driver_device
{
public:
	rainbow_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
        m_crtc(*this, "vt100_video"),
        m_i8088(*this, "maincpu"),
        m_z80(*this, "subcpu"),
        m_fdc(*this, "wd1793"),
        m_kbd8251(*this, "kbdser"),
		m_p_ram(*this, "p_ram"),
        m_shared(*this, "sh_ram")
    { }

	required_device<device_t> m_crtc;
    required_device<device_t> m_i8088;
    required_device<device_t> m_z80;
    required_device<device_t> m_fdc;
    required_device<i8251_device> m_kbd8251;
	required_shared_ptr<UINT8> m_p_ram;
	required_shared_ptr<UINT8> m_shared;
	UINT8 m_diagnostic;

    virtual void machine_start();

	DECLARE_READ8_MEMBER(read_video_ram_r);
	DECLARE_WRITE8_MEMBER(clear_video_interrupt);

	DECLARE_READ8_MEMBER(diagnostic_r);
	DECLARE_WRITE8_MEMBER(diagnostic_w);

	DECLARE_READ8_MEMBER(share_z80_r);
	DECLARE_WRITE8_MEMBER(share_z80_w);

	DECLARE_READ8_MEMBER(floating_bus_r);

	DECLARE_READ8_MEMBER(i8088_latch_r);
	DECLARE_WRITE8_MEMBER(i8088_latch_w);
	DECLARE_READ8_MEMBER(z80_latch_r);
	DECLARE_WRITE8_MEMBER(z80_latch_w);

	DECLARE_WRITE8_MEMBER(z80_diskdiag_read_w);
	DECLARE_WRITE8_MEMBER(z80_diskdiag_write_w);

	DECLARE_READ_LINE_MEMBER(kbd_rx);
	DECLARE_WRITE_LINE_MEMBER(kbd_tx);
    DECLARE_WRITE_LINE_MEMBER(kbd_rxready_w);
    DECLARE_WRITE_LINE_MEMBER(kbd_txready_w);

    bool m_zflip;                   // Z80 alternate memory map with A15 inverted
    bool m_z80_halted;
    bool m_kbd_tx_ready, m_kbd_rx_ready;

private:
    UINT8 m_z80_private[0x800];     // Z80 private 2K
    UINT8 m_z80_mailbox, m_8088_mailbox;

    void update_kbd_irq();
};

void rainbow_state::machine_start()
{
    save_item(NAME(m_z80_private));
    save_item(NAME(m_z80_mailbox));
    save_item(NAME(m_8088_mailbox));
    save_item(NAME(m_zflip));
    save_item(NAME(m_kbd_tx_ready));
    save_item(NAME(m_kbd_rx_ready));
}

static ADDRESS_MAP_START( rainbow8088_map, AS_PROGRAM, 8, rainbow_state)
	ADDRESS_MAP_UNMAP_HIGH
    AM_RANGE(0x00000, 0x0ffff) AM_RAM AM_SHARE("sh_ram")
    AM_RANGE(0x10000, 0x1ffff) AM_RAM
    AM_RANGE(0x20000, 0xdffff) AM_READ(floating_bus_r)  // test at f4e1c
    AM_RANGE(0x20000, 0x3ffff) AM_RAM
    AM_RANGE(0xec000, 0xedfff) AM_RAM
	AM_RANGE(0xee000, 0xeffff) AM_RAM AM_SHARE("p_ram")
	AM_RANGE(0xf0000, 0xfffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( rainbow8088_io , AS_IO, 8, rainbow_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
    AM_RANGE (0x00, 0x00) AM_READWRITE(i8088_latch_r, i8088_latch_w)
	// 0x04 Video processor DC011
	AM_RANGE (0x04, 0x04) AM_DEVWRITE_LEGACY("vt100_video", vt_video_dc011_w)

	AM_RANGE (0x0a, 0x0a) AM_READWRITE(diagnostic_r, diagnostic_w)
	// 0x0C Video processor DC012
	AM_RANGE (0x0c, 0x0c) AM_DEVWRITE_LEGACY("vt100_video", vt_video_dc012_w)

	AM_RANGE(0x10, 0x10) AM_DEVREADWRITE("kbdser", i8251_device, data_r, data_w)
	AM_RANGE(0x11, 0x11) AM_DEVREADWRITE("kbdser", i8251_device, status_r, control_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(rainbowz80_mem, AS_PROGRAM, 8, rainbow_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0xffff ) AM_READWRITE(share_z80_r, share_z80_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( rainbowz80_io, AS_IO, 8, rainbow_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
    AM_RANGE(0x00, 0x00) AM_READWRITE(z80_latch_r, z80_latch_w)
    AM_RANGE(0x20, 0x20) AM_WRITE(z80_diskdiag_read_w)
    AM_RANGE(0x21, 0x21) AM_WRITE(z80_diskdiag_write_w)

    AM_RANGE(0x60, 0x60) AM_DEVREADWRITE_LEGACY("wd1793", wd17xx_status_r, wd17xx_command_w)
    AM_RANGE(0x61, 0x61) AM_DEVREADWRITE_LEGACY("wd1793", wd17xx_track_r, wd17xx_track_w)
    AM_RANGE(0x62, 0x62) AM_DEVREADWRITE_LEGACY("wd1793", wd17xx_sector_r, wd17xx_sector_w)
    AM_RANGE(0x63, 0x63) AM_DEVREADWRITE_LEGACY("wd1793", wd17xx_data_r, wd17xx_data_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( rainbow )
INPUT_PORTS_END


static MACHINE_RESET( rainbow )
{
    rainbow_state *state = machine.driver_data<rainbow_state>();

    device_set_input_line(state->m_z80, INPUT_LINE_HALT, ASSERT_LINE);

    state->m_zflip = true;
    state->m_z80_halted = true;
    state->m_kbd_tx_ready = state->m_kbd_rx_ready = false;

    state->m_kbd8251->input_callback(SERIAL_STATE_CTS); // raise clear to send
}

static SCREEN_UPDATE_IND16( rainbow )
{
	device_t *devconf = screen.machine().device("vt100_video");
	rainbow_video_update( devconf, bitmap, cliprect);
	return 0;
}

READ8_MEMBER(rainbow_state::floating_bus_r)
{
    return (offset>>16) + 2;
}

READ8_MEMBER(rainbow_state::share_z80_r)
{
    if (m_zflip)
    {
        if (offset < 0x8000)
        {
            return m_shared[offset + 0x8000];
        }
        else if (offset < 0x8800)
        {
            return m_z80_private[offset & 0x7ff];
        }

        return m_shared[offset ^ 0x8000];
    }
    else
    {
        if (offset < 0x800)
        {
            return m_z80_private[offset];
        }

        return m_shared[offset];
    }

    return 0xff;
}

WRITE8_MEMBER(rainbow_state::share_z80_w)
{
    if (m_zflip)
    {
        if (offset < 0x8000)
        {
            m_shared[offset + 0x8000] = data;
        }
        else if (offset < 0x8800)
        {
            m_z80_private[offset & 0x7ff] = data;
        }

        m_shared[offset ^ 0x8000] = data;
    }
    else
    {
        if (offset < 0x800)
        {
            m_z80_private[offset] = data;
        }
        else
        {
            m_shared[offset] = data;
        }
    }
}

READ8_MEMBER(rainbow_state::i8088_latch_r)
{
//    printf("Read %02x from 8088 mailbox\n", m_8088_mailbox);
    device_set_input_line(m_i8088, INPUT_LINE_INT1, CLEAR_LINE);
    return m_8088_mailbox;
}

WRITE8_MEMBER(rainbow_state::i8088_latch_w)
{
//    printf("%02x to Z80 mailbox\n", data);
    device_set_input_line_and_vector(m_z80, 0, ASSERT_LINE, 0xf7);
    m_z80_mailbox = data;
}

READ8_MEMBER(rainbow_state::z80_latch_r)
{
//    printf("Read %02x from Z80 mailbox\n", m_z80_mailbox);
    device_set_input_line(m_z80, 0, CLEAR_LINE);
    return m_z80_mailbox;
}

WRITE8_MEMBER(rainbow_state::z80_latch_w)
{
//    printf("%02x to 8088 mailbox\n", data);
    device_set_input_line_and_vector(m_i8088, INPUT_LINE_INT1, ASSERT_LINE, 0x27);
    m_8088_mailbox = data;
}

WRITE8_MEMBER(rainbow_state::z80_diskdiag_read_w)
{
    m_zflip = true;
}

WRITE8_MEMBER(rainbow_state::z80_diskdiag_write_w)
{
    m_zflip = false;
}

READ8_MEMBER( rainbow_state::read_video_ram_r )
{
	return m_p_ram[offset];
}

static INTERRUPT_GEN( vblank_irq )
{
    device_set_input_line_and_vector(device, INPUT_LINE_INT0, ASSERT_LINE, 0x20);
}

WRITE8_MEMBER( rainbow_state::clear_video_interrupt )
{
    device_set_input_line(m_i8088, INPUT_LINE_INT0, CLEAR_LINE);
}

READ8_MEMBER( rainbow_state::diagnostic_r )
{
	return m_diagnostic | 0x0e;
}

WRITE8_MEMBER( rainbow_state::diagnostic_w )
{
//    printf("%02x to diag port (PC=%x)\n", data, cpu_get_pc(&space.device()));

    if (!(data & 1))
    {
        device_set_input_line(m_z80, INPUT_LINE_HALT, ASSERT_LINE);
        m_z80_halted = true;
    }

    if ((data & 1) && (m_z80_halted))
    {
        m_zflip = true;
        m_z80_halted = false;
        device_set_input_line(m_z80, INPUT_LINE_HALT, CLEAR_LINE);
        m_z80->reset();
    }

	m_diagnostic = data;
}

void rainbow_state::update_kbd_irq()
{
    if ((m_kbd_rx_ready) || (m_kbd_tx_ready))
    {
        device_set_input_line_and_vector(m_i8088, INPUT_LINE_INT2, ASSERT_LINE, 0x26);
    }
    else
    {
        device_set_input_line(m_i8088, INPUT_LINE_INT2, CLEAR_LINE);
    }
}

READ_LINE_MEMBER(rainbow_state::kbd_rx)
{
//    printf("read keyboard\n");
    return 0x00;
}

WRITE_LINE_MEMBER(rainbow_state::kbd_tx)
{
//    printf("%02x to keyboard\n", state);
}

WRITE_LINE_MEMBER(rainbow_state::kbd_rxready_w)
{
    m_kbd_rx_ready = (state == 1) ? true : false;
    update_kbd_irq();
}

WRITE_LINE_MEMBER(rainbow_state::kbd_txready_w)
{
    m_kbd_tx_ready = (state == 1) ? true : false;
    update_kbd_irq();
}

static const vt_video_interface video_interface =
{
	"screen",
	"chargen",
	DEVCB_DRIVER_MEMBER(rainbow_state, read_video_ram_r),
	DEVCB_DRIVER_MEMBER(rainbow_state, clear_video_interrupt)
};

/* F4 Character Displayer */
static const gfx_layout rainbow_charlayout =
{
	8, 10,					/* 8 x 16 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 15*8, 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8 },
	8*16					/* every char takes 16 bytes */
};

static GFXDECODE_START( rainbow )
	GFXDECODE_ENTRY( "chargen", 0x0000, rainbow_charlayout, 0, 1 )
GFXDECODE_END

// Rainbow Z80 polls only, no IRQ/DRQ are connected
const wd17xx_interface rainbow_wd17xx_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	{FLOPPY_0, FLOPPY_1}
};

static const floppy_interface floppy_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_SSDD,
	LEGACY_FLOPPY_OPTIONS_NAME(default),
	"floppy_5_25",
	NULL
};

static const i8251_interface i8251_intf =
{
	DEVCB_DRIVER_LINE_MEMBER(rainbow_state, kbd_rx),         // rxd in
	DEVCB_DRIVER_LINE_MEMBER(rainbow_state, kbd_tx),         // txd out
	DEVCB_NULL,         // dsr
	DEVCB_NULL,         // dtr
	DEVCB_NULL,         // rts
	DEVCB_DRIVER_LINE_MEMBER(rainbow_state, kbd_rxready_w),
	DEVCB_DRIVER_LINE_MEMBER(rainbow_state, kbd_txready_w),
	DEVCB_NULL,         // tx empty
	DEVCB_NULL          // syndet
};

static MACHINE_CONFIG_START( rainbow, rainbow_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",I8088, XTAL_24_0734MHz / 5)
	MCFG_CPU_PROGRAM_MAP(rainbow8088_map)
	MCFG_CPU_IO_MAP(rainbow8088_io)
	MCFG_CPU_VBLANK_INT("screen", vblank_irq)

	MCFG_CPU_ADD("subcpu",Z80, XTAL_24_0734MHz / 6)
	MCFG_CPU_PROGRAM_MAP(rainbowz80_mem)
	MCFG_CPU_IO_MAP(rainbowz80_io)

	MCFG_MACHINE_RESET(rainbow)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(80*10, 25*10)
	MCFG_SCREEN_VISIBLE_AREA(0, 80*10-1, 0, 25*10-1)
	MCFG_SCREEN_UPDATE_STATIC(rainbow)
	MCFG_GFXDECODE(rainbow)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(monochrome_green)
	MCFG_VT100_VIDEO_ADD("vt100_video", video_interface)

	MCFG_FD1793_ADD("wd1793", rainbow_wd17xx_interface )
	MCFG_LEGACY_FLOPPY_2_DRIVES_ADD(floppy_intf)

	MCFG_I8251_ADD("kbdser", i8251_intf)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( rainbow )
	ROM_REGION(0x100000,"maincpu", 0)
	ROM_LOAD( "23-022e5-00.bin",  0xf0000, 0x4000, CRC(9d1332b4) SHA1(736306d2a36bd44f95a39b36ebbab211cc8fea6e))
	ROM_RELOAD(0xf4000,0x4000)
	ROM_LOAD( "23-020e5-00.bin", 0xf8000, 0x4000, CRC(8638712f) SHA1(8269b0d95dc6efbe67d500dac3999df4838625d8)) // German, French, English
	//ROM_LOAD( "23-015e5-00.bin", 0xf8000, 0x4000, NO_DUMP) // Dutch, French, English
	//ROM_LOAD( "23-016e5-00.bin", 0xf8000, 0x4000, NO_DUMP) // Finish, Swedish, English
	//ROM_LOAD( "23-017e5-00.bin", 0xf8000, 0x4000, NO_DUMP) // Danish, Norwegian, English
	//ROM_LOAD( "23-018e5-00.bin", 0xf8000, 0x4000, NO_DUMP) // Spanish, Italian, English
	ROM_RELOAD(0xfc000,0x4000)
	ROM_REGION(0x1000, "chargen", 0)
	ROM_LOAD( "chargen.bin", 0x0000, 0x1000, CRC(1685e452) SHA1(bc299ff1cb74afcededf1a7beb9001188fdcf02f))
ROM_END

/* Driver */

/*    YEAR  NAME     PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY                         FULLNAME       FLAGS */
COMP( 1982, rainbow, 0,      0,       rainbow,   rainbow, 0,  "Digital Equipment Corporation", "Rainbow 100B", GAME_NOT_WORKING | GAME_NO_SOUND)
