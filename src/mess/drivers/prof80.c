/*

    PROF-80 (Prozessor RAM-Floppy Kontroller)
    GRIP-1/2/3/4/5 (Grafik-Interface-Prozessor)
    UNIO-1 (?)

    http://www.prof80.de/
    http://oldcomputers.dyndns.org/public/pub/rechner/conitec/info.html

*/

/*

    TODO:

    - GRIP does not ack display bytes sent by PROF
    - keyboard
    - NE555 timeout is 10x too high
    - convert GRIP models to devices
    - grip31 does not work
    - UNIO card (Z80-STI, Z80-SIO, 2x centronics)
    - GRIP-COLOR (192kB color RAM)
    - GRIP-5 (HD6345, 256KB RAM)
    - XR color card

*/

#include "includes/prof80.h"

/* Keyboard HACK */

static const UINT8 KEYCODES[3][9][8] =
{
	/* unshifted */
	{
	{ 0x1e, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37 },
	{ 0x38, 0x39, 0x30, 0x2d, 0x3d, 0x08, 0x7f, 0x2d },
	{ 0x37, 0x38, 0x39, 0x09, 0x71, 0x77, 0x65, 0x72 },
	{ 0x74, 0x79, 0x75, 0x69, 0x6f, 0x70, 0x5b, 0x5d },
	{ 0x1b, 0x2b, 0x34, 0x35, 0x36, 0x61, 0x73, 0x64 },
	{ 0x66, 0x67, 0x68, 0x6a, 0x6b, 0x6c, 0x3b, 0x27 },
	{ 0x0d, 0x0a, 0x01, 0x31, 0x32, 0x33, 0x7a, 0x78 },
	{ 0x63, 0x76, 0x62, 0x6e, 0x6d, 0x2c, 0x2e, 0x2f },
	{ 0x04, 0x02, 0x03, 0x30, 0x2e, 0x20, 0x00, 0x00 }
	},

	/* shifted */
	{
	{ 0x1e, 0x21, 0x40, 0x23, 0x24, 0x25, 0x5e, 0x26 },
	{ 0x2a, 0x28, 0x29, 0x5f, 0x2b, 0x08, 0x7f, 0x2d },
	{ 0x37, 0x38, 0x39, 0x09, 0x51, 0x57, 0x45, 0x52 },
	{ 0x54, 0x59, 0x55, 0x49, 0x4f, 0x50, 0x7b, 0x7d },
	{ 0x1b, 0x2b, 0x34, 0x35, 0x36, 0x41, 0x53, 0x44 },
	{ 0x46, 0x47, 0x48, 0x4a, 0x4b, 0x4c, 0x3a, 0x22 },
	{ 0x0d, 0x0a, 0x01, 0x31, 0x32, 0x33, 0x5a, 0x58 },
	{ 0x43, 0x56, 0x42, 0x4e, 0x4d, 0x3c, 0x3e, 0x3f },
	{ 0x04, 0x02, 0x03, 0x30, 0x2e, 0x20, 0x00, 0x00 }
	},

	/* control */
	{
	{ 0x9e, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97 },
	{ 0x98, 0x99, 0x90, 0x1f, 0x9a, 0x88, 0xff, 0xad },
	{ 0xb7, 0xb8, 0xb9, 0x89, 0x11, 0x17, 0x05, 0x12 },
	{ 0x14, 0x19, 0x15, 0x09, 0x0f, 0x10, 0x1b, 0x1d },
	{ 0x9b, 0xab, 0xb4, 0xb5, 0xb6, 0x01, 0x13, 0x04 },
	{ 0x06, 0x07, 0x08, 0x0a, 0x0b, 0x0c, 0x7e, 0x60 },
	{ 0x8d, 0x8a, 0x81, 0xb1, 0xb2, 0xb3, 0x1a, 0x18 },
	{ 0x03, 0x16, 0x02, 0x0e, 0x0d, 0x1c, 0x7c, 0x5c },
	{ 0x84, 0x82, 0x83, 0xb0, 0xae, 0x00, 0x00, 0x00 }
	}
};

void grip_state::scan_keyboard()
{
	static const char *const keynames[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4", "ROW5", "ROW6", "ROW7", "ROW8" };
	int table = 0, row, col;
	int keydata = -1;

	if (input_port_read(m_machine, "ROW9") & 0x07)
	{
		/* shift, upper case */
		table = 1;
	}

	if (input_port_read(m_machine, "ROW9") & 0x18)
	{
		/* ctrl */
		table = 2;
	}

	/* scan keyboard */
	for (row = 0; row < 9; row++)
	{
		UINT8 data = input_port_read(m_machine, keynames[row]);

		for (col = 0; col < 8; col++)
		{
			if (!BIT(data, col))
			{
				/* latch key data */
				keydata = KEYCODES[table][row][col];

				if (m_keydata != keydata)
				{
					m_keydata = keydata;

					/* trigger GRIP 8255 port C bit 2 (_STBB) */
					i8255a_pc2_w(m_ppi, 0);
					i8255a_pc2_w(m_ppi, 1);
					return;
				}
			}
		}
	}

	m_keydata = keydata;
}

static TIMER_DEVICE_CALLBACK( keyboard_tick )
{
	grip_state *state = timer.machine().driver_data<grip_state>();

	if (!state->m_kbf) state->scan_keyboard();
}

/* PROF-80 */

#define BLK_RAM1	0x0b
#define BLK_RAM2	0x0a
#define BLK_RAM3	0x03
#define BLK_RAM4	0x02
#define BLK_EPROM	0x00

void prof80_state::bankswitch()
{
	address_space *program = m_maincpu->memory().space(AS_PROGRAM);
	UINT8 *ram = ram_get_ptr(m_ram);
	UINT8 *rom = m_machine.region(Z80_TAG)->base();
	int bank;

	for (bank = 0; bank < 16; bank++)
	{
		UINT16 start_addr = bank * 0x1000;
		UINT16 end_addr = start_addr + 0xfff;
		int block = m_init ? m_mmu[bank] : BLK_EPROM;

		switch (block)
		{
		case BLK_RAM1:
			program->install_ram(start_addr, end_addr, ram + ((bank % 8) * 0x1000));
			break;

		case BLK_RAM2:
			program->install_ram(start_addr, end_addr, ram + 0x8000 + ((bank % 8) * 0x1000));
			break;

		case BLK_RAM3:
			program->install_ram(start_addr, end_addr, ram + 0x10000 + ((bank % 8) * 0x1000));
			break;

		case BLK_RAM4:
			program->install_ram(start_addr, end_addr, ram + 0x18000 + ((bank % 8) * 0x1000));
			break;

		case BLK_EPROM:
			program->install_rom(start_addr, end_addr, rom + ((bank % 2) * 0x1000));
			break;

		default:
			program->unmap_readwrite(start_addr, end_addr);
		}

		//logerror("Segment %u address %04x-%04x block %u\n", bank, start_addr, end_addr, block);
	}
}

void prof80_state::floppy_motor_off()
{
	floppy_mon_w(m_floppy0, 1);
	floppy_mon_w(m_floppy1, 1);
	floppy_drive_set_ready_state(m_floppy0, 0, 1);
	floppy_drive_set_ready_state(m_floppy1, 0, 1);

	m_motor = 0;
}

static TIMER_CALLBACK( floppy_motor_off_tick )
{
	prof80_state *state = machine.driver_data<prof80_state>();
	
	state->floppy_motor_off();
}

void prof80_state::ls259_w(int fa, int sa, int fb, int sb)
{
	switch (sa)
	{
	case 0: /* C0/TDI */
		m_rtc->data_in_w(fa);
		m_rtc->c0_w(fa);
		m_c0 = fa;
		break;

	case 1: /* C1 */
		m_rtc->c1_w(fa);
		m_c1 = fa;
		break;

	case 2: /* C2 */
		m_rtc->c2_w(fa);
		m_c2 = fa;
		break;

	case 3:	/* READY */
		break;

	case 4: /* TCK */
		m_rtc->clk_w(fa);
		break;

	case 5:	/* IN USE */
		output_set_led_value(0, fa);
		break;

	case 6:	/* _MOTOR */
		if (fa)
		{
			/* trigger floppy motor off NE555 timer */
			int t = 110 * RES_M(10) * CAP_U(6.8); // t = 1.1 * R8 * C6

			m_floppy_motor_off_timer->adjust(attotime::from_msec(t));
		}
		else
		{
			/* turn on floppy motor */
			floppy_mon_w(m_floppy0, 0);
			floppy_mon_w(m_floppy1, 0);
			floppy_drive_set_ready_state(m_floppy0, 1, 1);
			floppy_drive_set_ready_state(m_floppy1, 1, 1);

			m_motor = 1;

			/* reset floppy motor off NE555 timer */
			m_floppy_motor_off_timer->enable(0);
		}
		break;

	case 7:	/* SELECT */
		break;
	}

	switch (sb)
	{
	case 0: /* RESF */
		if (fb) upd765_reset(m_fdc, 0);
		break;

	case 1: /* MINI */
		break;

	case 2: /* _RTS */
		break;

	case 3: /* TX */
		if (m_terminal) terminal_serial_w(m_terminal, fb);
		break;

	case 4: /* _MSTOP */
		if (!fb)
		{
			/* immediately turn off floppy motor */
			m_floppy_motor_off_timer->adjust(attotime::zero);
		}
		break;

	case 5: /* TXP */
		break;

	case 6: /* TSTB */
		m_rtc->stb_w(fb);
		break;

	case 7: /* MME */
		//logerror("INIT %u\n", fb);
		m_init = fb;
		bankswitch();
		break;
	}
}

WRITE8_MEMBER( prof80_state::flr_w )
{
	/*

        bit     description

        0       FB
        1       SB0
        2       SB1
        3       SB2
        4       SA0
        5       SA1
        6       SA2
        7       FA

    */

	int fa = BIT(data, 7);
	int sa = (data >> 4) & 0x07;

	int fb = BIT(data, 0);
	int sb = (data >> 1) & 0x07;

	ls259_w(fa, sa, fb, sb);
}

READ8_MEMBER( prof80_state::status_r )
{
	/*

        bit     signal      description

        0       _RX
        1
        2
        3
        4       CTS
        5       _INDEX
        6
        7       CTSP

    */

	UINT8 data = 0;

	// serial receive
	if (m_terminal) data |= terminal_serial_r(m_terminal);

	// clear to send
	data |= 0x10;

	// floppy index
	data |= (m_fdc_index << 5);

	return data;
}

READ8_MEMBER( prof80_state::status2_r )
{
	/*

        bit     signal      description

        0       _MOTOR      floppy motor (0=on, 1=off)
        1
        2
        3
        4       JS4
        5       JS5
        6
        7       _TDO

    */

	UINT8 data = 0;
	int js4 = 0, js5 = 0;

	/* floppy motor */
	data |= !m_motor;

	/* JS4 */
	switch (input_port_read(m_machine, "J4"))
	{
	case 0: js4 = 0; break;
	case 1: js4 = 1; break;
	case 2: js4 = !m_c0; break;
	case 3: js4 = !m_c1; break;
	case 4: js4 = !m_c2; break;
	}

	data |= js4 << 4;

	/* JS5 */
	switch (input_port_read(m_machine, "J5"))
	{
	case 0: js5 = 0; break;
	case 1: js5 = 1; break;
	case 2: js5 = !m_c0; break;
	case 3: js5 = !m_c1; break;
	case 4: js5 = !m_c2; break;
	}

	data |= js5 << 4;

	/* RTC data */
	data |= !m_rtc->data_out_r() << 7;

	return data;
}

WRITE8_MEMBER( prof80_state::par_w )
{
	int bank = offset >> 12;

	m_mmu[bank] = data & 0x0f;

	//logerror("MMU bank %u block %u\n", bank, data & 0x0f);

	bankswitch();
}

READ8_MEMBER( prof80_state::gripc_r )
{
	//logerror("GRIP status read %02x\n", m_gripc);

	return m_gripc;
}

READ8_MEMBER( prof80_state::gripd_r )
{
	//logerror("GRIP data read %02x\n", m_gripd);

	/* trigger GRIP 8255 port C bit 6 (_ACKA) */
	i8255a_pc6_w(m_ppi, 0);
	i8255a_pc6_w(m_ppi, 1);

	return m_gripd;
}

WRITE8_MEMBER( prof80_state::gripd_w )
{
	m_gripd = data;
	//logerror("GRIP data write %02x\n", data);

	/* trigger GRIP 8255 port C bit 4 (_STBA) */
	i8255a_pc4_w(m_ppi, 0);
	i8255a_pc4_w(m_ppi, 1);
}

/* GRIP */

WRITE8_MEMBER( grip_state::vol0_w )
{
	m_vol0 = BIT(data, 7);
}

WRITE8_MEMBER( grip_state::vol1_w )
{
	m_vol1 = BIT(data, 7);
}

WRITE8_MEMBER( grip_state::flash_w )
{
	m_flash = BIT(data, 7);
}

WRITE8_MEMBER( grip_state::page_w )
{
	m_page = BIT(data, 7);

	memory_set_bank(m_machine, "videoram", m_page);
}

READ8_MEMBER( grip_state::stat_r )
{
	/*

        bit     signal      description

        0       LPA0
        1       LPA1
        2       LPA2
        3       SENSE
        4       JS0
        5       JS1
        6       _ERROR
        7       LPSTB       light pen strobe

    */

	UINT8 data = 0;
	int js0 = 0, js1 = 0;

	/* JS0 */
	switch (input_port_read(m_machine, "GRIP-J3A"))
	{
	case 0: js0 = 0; break;
	case 1: js0 = 1; break;
	case 2: js0 = m_vol0; break;
	case 3: js0 = m_vol1; break;
	case 4: js0 = m_page; break;
	}

	data |= js0 << 4;

	/* JS1 */
	switch (input_port_read(m_machine, "GRIP-J3B"))
	{
	case 0: js1 = 0; break;
	case 1: js1 = 1; break;
	case 2: js1 = m_vol0; break;
	case 3: js1 = m_vol1; break;
	case 4: js1 = m_page; break;
	}

	data |= js1 << 5;

	/* centronics fault */
	data |= centronics_fault_r(m_centronics) << 6;

	/* light pen strobe */
	data |= m_lps << 7;

	return data;
}

READ8_MEMBER( grip_state::lrs_r )
{
	m_lps = 0;

	return 0;
}

WRITE8_MEMBER( grip_state::lrs_w )
{
	m_lps = 0;
}

READ8_MEMBER( grip_state::cxstb_r )
{
	centronics_strobe_w(m_centronics, 0);
	centronics_strobe_w(m_centronics, 1);

	return 0;
}

WRITE8_MEMBER( grip_state::cxstb_w )
{
	centronics_strobe_w(m_centronics, 0);
	centronics_strobe_w(m_centronics, 1);
}

/* UNIO */

WRITE8_MEMBER( prof80_state::unio_ctrl_w )
{
//  int flag = BIT(data, 0);
	int flad = (data >> 1) & 0x07;

	switch (flad)
	{
	case 0: /* CG1 */
	case 1: /* CG2 */
	case 2: /* _STB1 */
	case 3: /* _STB2 */
	case 4: /* _INIT */
	case 5: /* JSO0 */
	case 6: /* JSO1 */
	case 7: /* JSO2 */
		break;
	}
}

/* Memory Maps */

static ADDRESS_MAP_START( prof80_mem, AS_PROGRAM, 8, prof80_state )
ADDRESS_MAP_END

static ADDRESS_MAP_START( prof80_io, AS_IO, 8, prof80_state )
//  AM_RANGE(0x80, 0x8f) AM_MIRROR(0xff00) AM_DEVREADWRITE_LEGACY(UNIO_Z80STI_TAG, z80sti_r, z80sti_w)
//  AM_RANGE(0x94, 0x95) AM_MIRROR(0xff00) AM_DEVREADWRITE_LEGACY(UNIO_Z80SIO_TAG, z80sio_d_r, z80sio_d_w)
//  AM_RANGE(0x96, 0x97) AM_MIRROR(0xff00) AM_DEVREADWRITE_LEGACY(UNIO_Z80SIO_TAG, z80sio_c_r, z80sio_c_w)
	AM_RANGE(0x9e, 0x9e) AM_MIRROR(0xff00) AM_WRITE(unio_ctrl_w)
//  AM_RANGE(0x9c, 0x9c) AM_MIRROR(0xff00) AM_DEVWRITE_LEGACY(UNIO_CENTRONICS1_TAG, centronics_data_w)
//  AM_RANGE(0x9d, 0x9d) AM_MIRROR(0xff00) AM_DEVWRITE_LEGACY(UNIO_CENTRONICS1_TAG, centronics_data_w)
//  AM_RANGE(0xc0, 0xc0) AM_MIRROR(0xff00) AM_READ(gripc_r)
//  AM_RANGE(0xc1, 0xc1) AM_MIRROR(0xff00) AM_READWRITE(gripd_r, gripd_w)
	AM_RANGE(0xd8, 0xd8) AM_MIRROR(0xff00) AM_WRITE(flr_w)
	AM_RANGE(0xda, 0xda) AM_MIRROR(0xff00) AM_READ(status_r)
	AM_RANGE(0xdb, 0xdb) AM_MIRROR(0xff00) AM_READ(status2_r)
	AM_RANGE(0xdc, 0xdc) AM_MIRROR(0xff00) AM_DEVREAD_LEGACY(UPD765_TAG, upd765_status_r)
	AM_RANGE(0xdd, 0xdd) AM_MIRROR(0xff00) AM_DEVREADWRITE_LEGACY(UPD765_TAG, upd765_data_r, upd765_data_w)
	AM_RANGE(0xde, 0xde) AM_MIRROR(0xff01) AM_MASK(0xff00) AM_WRITE(par_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( prof80_grip_io, AS_IO, 8, prof80_state )
//  AM_RANGE(0x80, 0x8f) AM_MIRROR(0xff00) AM_DEVREADWRITE_LEGACY(UNIO_Z80STI_TAG, z80sti_r, z80sti_w)
//  AM_RANGE(0x94, 0x95) AM_MIRROR(0xff00) AM_DEVREADWRITE_LEGACY(UNIO_Z80SIO_TAG, z80sio_d_r, z80sio_d_w)
//  AM_RANGE(0x96, 0x97) AM_MIRROR(0xff00) AM_DEVREADWRITE_LEGACY(UNIO_Z80SIO_TAG, z80sio_c_r, z80sio_c_w)
	AM_RANGE(0x9e, 0x9e) AM_MIRROR(0xff00) AM_WRITE(unio_ctrl_w)
//  AM_RANGE(0x9c, 0x9c) AM_MIRROR(0xff00) AM_DEVWRITE_LEGACY(UNIO_CENTRONICS1_TAG, centronics_data_w)
//  AM_RANGE(0x9d, 0x9d) AM_MIRROR(0xff00) AM_DEVWRITE_LEGACY(UNIO_CENTRONICS1_TAG, centronics_data_w)
	AM_RANGE(0xc0, 0xc0) AM_MIRROR(0xff00) AM_READ(gripc_r)
	AM_RANGE(0xc1, 0xc1) AM_MIRROR(0xff00) AM_READWRITE(gripd_r, gripd_w)
	AM_RANGE(0xd8, 0xd8) AM_MIRROR(0xff00) AM_WRITE(flr_w)
	AM_RANGE(0xda, 0xda) AM_MIRROR(0xff00) AM_READ(status_r)
	AM_RANGE(0xdb, 0xdb) AM_MIRROR(0xff00) AM_READ(status2_r)
	AM_RANGE(0xdc, 0xdc) AM_MIRROR(0xff00) AM_DEVREAD_LEGACY(UPD765_TAG, upd765_status_r)
	AM_RANGE(0xdd, 0xdd) AM_MIRROR(0xff00) AM_DEVREADWRITE_LEGACY(UPD765_TAG, upd765_data_r, upd765_data_w)
	AM_RANGE(0xde, 0xde) AM_MIRROR(0xff01) AM_MASK(0xff00) AM_WRITE(par_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( grip_mem, AS_PROGRAM, 8, grip_state )
    AM_RANGE(0x0000, 0x3fff) AM_ROM
    AM_RANGE(0x4000, 0x47ff) AM_RAM
    AM_RANGE(0x8000, 0xffff) AM_RAMBANK("videoram")
ADDRESS_MAP_END

static ADDRESS_MAP_START( grip_io, AS_IO, 8, grip_state )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_READWRITE(cxstb_r, cxstb_w)
//  AM_RANGE(0x10, 0x10) AM_WRITE(ccon_w)
	AM_RANGE(0x11, 0x11) AM_WRITE(vol0_w)
//  AM_RANGE(0x12, 0x12) AM_WRITE(rts_w)
	AM_RANGE(0x13, 0x13) AM_WRITE(page_w)
//  AM_RANGE(0x14, 0x14) AM_WRITE(cc1_w)
//  AM_RANGE(0x15, 0x15) AM_WRITE(cc2_w)
	AM_RANGE(0x16, 0x16) AM_WRITE(flash_w)
	AM_RANGE(0x17, 0x17) AM_WRITE(vol1_w)
	AM_RANGE(0x20, 0x2f) AM_DEVREADWRITE_LEGACY(Z80STI_TAG, z80sti_r, z80sti_w)
	AM_RANGE(0x30, 0x30) AM_READWRITE(lrs_r, lrs_w)
	AM_RANGE(0x40, 0x40) AM_READ(stat_r)
	AM_RANGE(0x50, 0x50) AM_DEVWRITE_LEGACY(MC6845_TAG, mc6845_address_w)
	AM_RANGE(0x52, 0x52) AM_DEVWRITE_LEGACY(MC6845_TAG, mc6845_register_w)
	AM_RANGE(0x53, 0x53) AM_DEVREAD_LEGACY(MC6845_TAG, mc6845_register_r)
	AM_RANGE(0x60, 0x60) AM_DEVWRITE_LEGACY(CENTRONICS_TAG, centronics_data_w)
	AM_RANGE(0x70, 0x73) AM_DEVREADWRITE_LEGACY(I8255A_TAG, i8255a_r, i8255a_w)
//  AM_RANGE(0x80, 0x80) AM_WRITE(bl2out_w)
//  AM_RANGE(0x90, 0x90) AM_WRITE(gr2out_w)
//  AM_RANGE(0xa0, 0xa0) AM_WRITE(rd2out_w)
//  AM_RANGE(0xb0, 0xb0) AM_WRITE(clrg2_w)
//  AM_RANGE(0xc0, 0xc0) AM_WRITE(bluout_w)
//  AM_RANGE(0xd0, 0xd0) AM_WRITE(grnout_w)
//  AM_RANGE(0xe0, 0xe0) AM_WRITE(redout_w)
//  AM_RANGE(0xf0, 0xf0) AM_WRITE(clrg1_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( grip5_io, AS_IO, 8, grip_state )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_READWRITE(cxstb_r, cxstb_w)
//  AM_RANGE(0x10, 0x10) AM_WRITE(eprom_w)
//  AM_RANGE(0x11, 0x11) AM_WRITE(vol0_w)
//  AM_RANGE(0x12, 0x12) AM_WRITE(rts_w)
	AM_RANGE(0x13, 0x13) AM_WRITE(page_w)
//  AM_RANGE(0x14, 0x14) AM_WRITE(str_w)
//  AM_RANGE(0x15, 0x15) AM_WRITE(intl_w)
//  AM_RANGE(0x16, 0x16) AM_WRITE(dpage_w)
//  AM_RANGE(0x17, 0x17) AM_WRITE(vol1_w)
	AM_RANGE(0x20, 0x2f) AM_DEVREADWRITE_LEGACY(Z80STI_TAG, z80sti_r, z80sti_w)
	AM_RANGE(0x30, 0x30) AM_READWRITE(lrs_r, lrs_w)
	AM_RANGE(0x40, 0x40) AM_READ(stat_r)
	AM_RANGE(0x50, 0x50) AM_DEVWRITE_LEGACY(MC6845_TAG, mc6845_address_w)
	AM_RANGE(0x52, 0x52) AM_DEVWRITE_LEGACY(MC6845_TAG, mc6845_register_w)
	AM_RANGE(0x53, 0x53) AM_DEVREAD_LEGACY(MC6845_TAG, mc6845_register_r)
	AM_RANGE(0x60, 0x60) AM_DEVWRITE_LEGACY(CENTRONICS_TAG, centronics_data_w)
	AM_RANGE(0x70, 0x73) AM_DEVREADWRITE_LEGACY(I8255A_TAG, i8255a_r, i8255a_w)

//  AM_RANGE(0x80, 0x80) AM_WRITE(xrflgs_w)
//  AM_RANGE(0xc0, 0xc0) AM_WRITE(xrclrg_w)
//  AM_RANGE(0xe0, 0xe0) AM_WRITE(xrclu0_w)
//  AM_RANGE(0xe1, 0xe1) AM_WRITE(xrclu1_w)
//  AM_RANGE(0xe2, 0xe2) AM_WRITE(xrclu2_w)

//  AM_RANGE(0x80, 0x80) AM_WRITE(bl2out_w)
//  AM_RANGE(0x90, 0x90) AM_WRITE(gr2out_w)
//  AM_RANGE(0xa0, 0xa0) AM_WRITE(rd2out_w)
//  AM_RANGE(0xb0, 0xb0) AM_WRITE(clrg2_w)
//  AM_RANGE(0xc0, 0xc0) AM_WRITE(bluout_w)
//  AM_RANGE(0xd0, 0xd0) AM_WRITE(grnout_w)
//  AM_RANGE(0xe0, 0xe0) AM_WRITE(redout_w)
//  AM_RANGE(0xf0, 0xf0) AM_WRITE(clrg1_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( ascii_keyboard )
	PORT_START("ROW0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("HELP") PORT_CODE(KEYCODE_TILDE)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('^')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('&')

	PORT_START("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("BACKSPACE") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad -") PORT_CODE(KEYCODE_MINUS_PAD)

	PORT_START("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 7") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 8") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 9") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB) PORT_CHAR('\t')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')

	PORT_START("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')

	PORT_START("ROW4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad +") PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')

	PORT_START("ROW5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('\'') PORT_CHAR('"')

	PORT_START("ROW6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR('\r')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("LINE FEED") PORT_CODE(KEYCODE_ENTER_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(UTF8_UP) PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')

	PORT_START("ROW7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')

	PORT_START("ROW8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(UTF8_LEFT) PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(UTF8_DOWN) PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(UTF8_RIGHT) PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 0") PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad .") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("ROW9")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("LEFT SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RIGHT SHIFT") PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("LEFT CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RIGHT CTRL") PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_MAMEKEY(RCONTROL))
INPUT_PORTS_END

static INPUT_PORTS_START( prof80 )
	PORT_START("J1")
	PORT_CONFNAME( 0x01, 0x00, "J1 RDY/HDLD")
	PORT_CONFSETTING( 0x00, "HDLD" )
	PORT_CONFSETTING( 0x01, "READY" )

	PORT_START("J2")
	PORT_CONFNAME( 0x01, 0x01, "J2 RDY/DCHG")
	PORT_CONFSETTING( 0x00, "DCHG" )
	PORT_CONFSETTING( 0x01, "READY" )

	PORT_START("J3")
	PORT_CONFNAME( 0x01, 0x00, "J3 Port Address")
	PORT_CONFSETTING( 0x00, "D8-DF" )
	PORT_CONFSETTING( 0x01, "E8-EF" )

	PORT_START("J4")
	PORT_CONFNAME( 0x07, 0x00, "J4 Console")
	PORT_CONFSETTING( 0x00, "GRIP-1" )
	PORT_CONFSETTING( 0x01, "V24 DUPLEX" )
	PORT_CONFSETTING( 0x02, "USER1" )
	PORT_CONFSETTING( 0x03, "USER2" )
	PORT_CONFSETTING( 0x04, "CP/M" )

	PORT_START("J5")
	PORT_CONFNAME( 0x07, 0x00, "J5 Baud")
	PORT_CONFSETTING( 0x00, "9600" )
	PORT_CONFSETTING( 0x01, "4800" )
	PORT_CONFSETTING( 0x02, "2400" )
	PORT_CONFSETTING( 0x03, "1200" )
	PORT_CONFSETTING( 0x04, "300" )

	PORT_START("J6")
	PORT_CONFNAME( 0x01, 0x01, "J6 Interrupt")
	PORT_CONFSETTING( 0x00, "Serial" )
	PORT_CONFSETTING( 0x01, "ECB" )

	PORT_START("J7")
	PORT_CONFNAME( 0x01, 0x01, "J7 DMA MMU")
	PORT_CONFSETTING( 0x00, "PROF" )
	PORT_CONFSETTING( 0x01, "DMA Card" )

	PORT_START("J8")
	PORT_CONFNAME( 0x01, 0x01, "J8 Active Mode")
	PORT_CONFSETTING( 0x00, DEF_STR( Off ) )
	PORT_CONFSETTING( 0x01, DEF_STR( On ) )

	PORT_START("J9")
	PORT_CONFNAME( 0x01, 0x00, "J9 EPROM Type")
	PORT_CONFSETTING( 0x00, "2732/2764" )
	PORT_CONFSETTING( 0x01, "27128" )

	PORT_START("J10")
	PORT_CONFNAME( 0x03, 0x00, "J10 Wait States")
	PORT_CONFSETTING( 0x00, "On all memory accesses" )
	PORT_CONFSETTING( 0x01, "On internal memory accesses" )
	PORT_CONFSETTING( 0x02, DEF_STR( None ) )

	PORT_START("L1")
	PORT_CONFNAME( 0x01, 0x00, "L1 Write Polarity")
	PORT_CONFSETTING( 0x00, "Inverted" )
	PORT_CONFSETTING( 0x01, "Normal" )
INPUT_PORTS_END

static INPUT_PORTS_START( grip )
	PORT_INCLUDE(ascii_keyboard)
	PORT_INCLUDE(prof80)

	PORT_START("GRIP-J1")
	PORT_CONFNAME( 0x01, 0x00, "J1 EPROM Type")
	PORT_CONFSETTING( 0x00, "2732" )
	PORT_CONFSETTING( 0x01, "2764/27128" )

	PORT_START("GRIP-J2")
	PORT_CONFNAME( 0x03, 0x00, "J2 Centronics Mode")
	PORT_CONFSETTING( 0x00, "Mode 1" )
	PORT_CONFSETTING( 0x01, "Mode 2" )
	PORT_CONFSETTING( 0x02, "Mode 3" )

	PORT_START("GRIP-J3A")
	PORT_CONFNAME( 0x07, 0x00, "J3 Host")
	PORT_CONFSETTING( 0x00, "ECB Bus" )
	PORT_CONFSETTING( 0x01, "V24 9600 Baud" )
	PORT_CONFSETTING( 0x02, "V24 4800 Baud" )
	PORT_CONFSETTING( 0x03, "V24 1200 Baud" )
	PORT_CONFSETTING( 0x04, "Keyboard" )

	PORT_START("GRIP-J3B")
	PORT_CONFNAME( 0x07, 0x00, "J3 Keyboard")
	PORT_CONFSETTING( 0x00, "Parallel" )
	PORT_CONFSETTING( 0x01, "Serial (1200 Baud, 8 Bits)" )
	PORT_CONFSETTING( 0x02, "Serial (1200 Baud, 7 Bits)" )
	PORT_CONFSETTING( 0x03, "Serial (600 Baud, 8 Bits)" )
	PORT_CONFSETTING( 0x04, "Serial (600 Baud, 7 Bits)" )

	PORT_START("GRIP-J4")
	PORT_CONFNAME( 0x01, 0x00, "J4 GRIP-COLOR")
	PORT_CONFSETTING( 0x00, DEF_STR( No ) )
	PORT_CONFSETTING( 0x01, DEF_STR( Yes ) )

	PORT_START("GRIP-J5")
	PORT_CONFNAME( 0x01, 0x01, "J5 Power On Reset")
	PORT_CONFSETTING( 0x00, "External" )
	PORT_CONFSETTING( 0x01, "Internal" )

	PORT_START("GRIP-J6")
	PORT_CONFNAME( 0x03, 0x00, "J6 Serial Clock")
	PORT_CONFSETTING( 0x00, "TC/16, TD/16, TD" )
	PORT_CONFSETTING( 0x01, "TD/16, TD/16, TD" )
	PORT_CONFSETTING( 0x02, "TC/16, BAUD/16, input" )
	PORT_CONFSETTING( 0x03, "BAUD/16, BAUD/16, input" )

	PORT_START("GRIP-J7")
	PORT_CONFNAME( 0x01, 0x00, "J7 ECB Bus Address")
	PORT_CONFSETTING( 0x00, "C0/C1" )
	PORT_CONFSETTING( 0x01, "A0/A1" )

	PORT_START("GRIP-J8")
	PORT_CONFNAME( 0x01, 0x00, "J8 Video RAM")
	PORT_CONFSETTING( 0x00, "32 KB" )
	PORT_CONFSETTING( 0x01, "64 KB" )

	PORT_START("GRIP-J9")
	PORT_CONFNAME( 0x01, 0x01, "J9 CPU Clock")
	PORT_CONFSETTING( 0x00, "2 MHz" )
	PORT_CONFSETTING( 0x01, "4 MHz" )

	PORT_START("GRIP-J10")
	PORT_CONFNAME( 0x01, 0x01, "J10 Pixel Clock")
	PORT_CONFSETTING( 0x00, "External" )
	PORT_CONFSETTING( 0x01, "Internal" )
INPUT_PORTS_END

/* Video */

static MC6845_UPDATE_ROW( grip_update_row )
{
	grip_state *state = device->machine().driver_data<grip_state>();
	int column, bit;

	for (column = 0; column < x_count; column++)
	{
		UINT16 address = (state->m_page << 12) | ((ma + column) << 3) | (ra & 0x07);
		UINT8 data = state->m_video_ram[address];

		for (bit = 0; bit < 8; bit++)
		{
			int x = (column * 8) + bit;
			int color = state->m_flash ? 0 : BIT(data, bit);

			*BITMAP_ADDR16(bitmap, y, x) = color;
		}
	}
}

static const mc6845_interface crtc_intf =
{
	SCREEN_TAG,
	8,
	NULL,
	grip_update_row,
	NULL,
	DEVCB_DEVICE_LINE(Z80STI_TAG, z80sti_i1_w),
	DEVCB_DEVICE_LINE(Z80STI_TAG, z80sti_i2_w),
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};

bool grip_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	mc6845_update(m_crtc, &bitmap, &cliprect);

	return 0;
}

/* uPD1990A Interface */

static UPD1990A_INTERFACE( prof80_upd1990a_intf )
{
	DEVCB_NULL,
	DEVCB_NULL
};

/* UPD765 Interface */

static void prof80_fdc_index_callback(device_t *controller, device_t *img, int state)
{
	prof80_state *driver_state = img->machine().driver_data<prof80_state>();

	driver_state->m_fdc_index = state;
}

static const struct upd765_interface prof80_upd765_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	NULL,
	UPD765_RDY_PIN_CONNECTED,
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};

/* PPI8255 Interface */

READ8_MEMBER( grip_state::ppi_pa_r )
{
	/*

        bit     description

        PA0     ECB bus
        PA1     ECB bus
        PA2     ECB bus
        PA3     ECB bus
        PA4     ECB bus
        PA5     ECB bus
        PA6     ECB bus
        PA7     ECB bus

    */

	return m_gripd;
}

WRITE8_MEMBER( grip_state::ppi_pa_w )
{
	/*

        bit     description

        PA0     ECB bus
        PA1     ECB bus
        PA2     ECB bus
        PA3     ECB bus
        PA4     ECB bus
        PA5     ECB bus
        PA6     ECB bus
        PA7     ECB bus

    */

	m_gripd = data;
}

READ8_MEMBER( grip_state::ppi_pb_r )
{
	/*

        bit     description

        PB0     Keyboard input
        PB1     Keyboard input
        PB2     Keyboard input
        PB3     Keyboard input
        PB4     Keyboard input
        PB5     Keyboard input
        PB6     Keyboard input
        PB7     Keyboard input

    */

	return m_keydata;
}

WRITE8_MEMBER( grip_state::ppi_pc_w )
{
	/*

        bit     signal      description

        PC0     INTRB       interrupt B output (keyboard)
        PC1     KBF         input buffer B full output (keyboard)
        PC2     _KBSTB      strobe B input (keyboard)
        PC3     INTRA       interrupt A output (PROF-80)
        PC4     _STBA       strobe A input (PROF-80)
        PC5     IBFA        input buffer A full output (PROF-80)
        PC6     _ACKA       acknowledge A input (PROF-80)
        PC7     _OBFA       output buffer full output (PROF-80)

    */

	/* keyboard interrupt */
	z80sti_i4_w(m_sti, BIT(data, 0));

	/* keyboard buffer full */
	m_kbf = BIT(data, 1);

	/* PROF-80 interrupt */
	z80sti_i7_w(m_sti, BIT(data, 3));

	/* PROF-80 handshaking */
	m_gripc = (!BIT(data, 7) << 7) | (!BIT(data, 5) << 6) | (i8255a_pa_r(m_ppi, 0) & 0x3f);
}

static I8255A_INTERFACE( ppi_intf )
{
	DEVCB_DRIVER_MEMBER(grip_state, ppi_pa_r),	// Port A read
	DEVCB_DRIVER_MEMBER(grip_state, ppi_pb_r),	// Port B read
	DEVCB_NULL,							// Port C read
	DEVCB_DRIVER_MEMBER(grip_state, ppi_pa_w),	// Port A write
	DEVCB_NULL,							// Port B write
	DEVCB_DRIVER_MEMBER(grip_state, ppi_pc_w)		// Port C write
};

/* Z80-STI Interface */

READ8_MEMBER( grip_state::sti_gpio_r )
{
	/*

        bit     signal      description

        I0      CTS         RS-232 clear to send input
        I1      DE          MC6845 display enable input
        I2      CURSOR      MC6845 cursor input
        I3      BUSY        Centronics busy input
        I4      PC0         PPI8255 PC0 input
        I5      SKBD        Serial keyboard input
        I6      EXIN        External interrupt input
        I7      PC3         PPI8255 PC3 input

    */

	return centronics_busy_r(m_centronics) << 3;
}

WRITE_LINE_MEMBER( grip_state::speaker_w )
{
	int level = state && ((m_vol1 << 1) | m_vol0);

	speaker_level_w(m_speaker, level);
}

static Z80STI_INTERFACE( sti_intf )
{
	0,														/* serial receive clock */
	0,														/* serial transmit clock */
	DEVCB_CPU_INPUT_LINE(GRIP_Z80_TAG, INPUT_LINE_IRQ0),	/* interrupt */
	DEVCB_DRIVER_MEMBER(grip_state, sti_gpio_r),			/* GPIO read */
	DEVCB_NULL,												/* GPIO write */
	DEVCB_NULL,												/* serial input */
	DEVCB_NULL,												/* serial output */
	DEVCB_NULL,												/* timer A output */
	DEVCB_DRIVER_LINE_MEMBER(grip_state, speaker_w),		/* timer B output */
	DEVCB_LINE(z80sti_tc_w),								/* timer C output */
	DEVCB_LINE(z80sti_rc_w)									/* timer D output */
};

/* Z80 Daisy Chain */

static const z80_daisy_config grip_daisy_chain[] =
{
	{ Z80STI_TAG },
	{ NULL }
};

/* Machine Initialization */

void prof80_state::machine_start()
{
	/* initialize RTC */
	m_rtc->cs_w(1);
	m_rtc->oe_w(1);

	/* configure FDC */
	floppy_drive_set_index_pulse_callback(m_floppy0, prof80_fdc_index_callback);

	/* allocate floppy motor off timer */
	m_floppy_motor_off_timer = m_machine.scheduler().timer_alloc(FUNC(floppy_motor_off_tick));

	/* bank switch */
	bankswitch();

	/* register for state saving */
	save_item(NAME(m_c0));
	save_item(NAME(m_c1));
	save_item(NAME(m_c2));
	save_item(NAME(m_mmu));
	save_item(NAME(m_init));
	save_item(NAME(m_fdc_index));
	save_item(NAME(m_gripd));
	save_item(NAME(m_gripc));
}

void prof80_state::machine_reset()
{
	int i;

	for (i = 0; i < 8; i++)
	{
		ls259_w(0, i, 0, i);
	}

	m_gripc = 0x40;
}

void grip_state::machine_start()
{
	prof80_state::machine_start();

	/* allocate video RAM */
	m_video_ram = auto_alloc_array(m_machine, UINT8, GRIP_VIDEORAM_SIZE);

	/* setup GRIP memory banking */
	memory_configure_bank(m_machine, "videoram", 0, 2, m_video_ram, 0x8000);
	memory_set_bank(m_machine, "videoram", 0);

	/* register for state saving */
	save_item(NAME(m_vol0));
	save_item(NAME(m_vol1));
	save_item(NAME(m_keydata));
	save_item(NAME(m_kbf));
	save_pointer(NAME(m_video_ram), GRIP_VIDEORAM_SIZE);
	save_item(NAME(m_lps));
	save_item(NAME(m_page));
	save_item(NAME(m_flash));
}

static const floppy_config prof80_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(default),
	NULL
};

static const INT16 speaker_levels[] = { -32768, 0, 32767, 0 };

static const speaker_interface speaker_intf =
{
	4,
	speaker_levels
};

static GENERIC_TERMINAL_INTERFACE( xor100_terminal_intf )
{
	DEVCB_NULL
};

/* Machine Drivers */

static MACHINE_CONFIG_START( common, prof80_state )
    /* basic machine hardware */
    MCFG_CPU_ADD(Z80_TAG, Z80, XTAL_6MHz)
    MCFG_CPU_PROGRAM_MAP(prof80_mem)
    MCFG_CPU_IO_MAP(prof80_io)

    /* video hardware */
    MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 480)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

	/* devices */
	MCFG_UPD1990A_ADD(UPD1990A_TAG, XTAL_32_768kHz, prof80_upd1990a_intf)
	MCFG_UPD765A_ADD(UPD765_TAG, prof80_upd765_interface)
	MCFG_FLOPPY_4_DRIVES_ADD(prof80_floppy_config)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("128K")
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( prof80, common )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, xor100_terminal_intf)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED_CLASS( grip, common, grip_state )
    /* basic machine hardware */
    MCFG_CPU_MODIFY(Z80_TAG)
    MCFG_CPU_IO_MAP(prof80_grip_io)

	MCFG_CPU_ADD(GRIP_Z80_TAG, Z80, XTAL_16MHz/4)
	MCFG_CPU_CONFIG(grip_daisy_chain)
    MCFG_CPU_PROGRAM_MAP(grip_mem)
    MCFG_CPU_IO_MAP(grip_io)

	/* keyboard hack */
	MCFG_TIMER_ADD_PERIODIC("keyboard", keyboard_tick, attotime::from_hz(50))

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(SPEAKER_TAG, SPEAKER_SOUND, 0)
	MCFG_SOUND_CONFIG(speaker_intf)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MCFG_MC6845_ADD(MC6845_TAG, MC6845, XTAL_16MHz/4, crtc_intf)
	MCFG_I8255A_ADD(I8255A_TAG, ppi_intf)
	MCFG_Z80STI_ADD(Z80STI_TAG, XTAL_16MHz/4, sti_intf)
	MCFG_CENTRONICS_ADD(CENTRONICS_TAG, standard_centronics)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( grip2, grip )
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( grip3, grip )
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( grip5, grip )
    /* basic machine hardware */
	MCFG_CPU_MODIFY(GRIP_Z80_TAG)
    MCFG_CPU_IO_MAP(grip5_io)
MACHINE_CONFIG_END

/* ROMs */

#define ROM_PROF80 \
	ROM_REGION( 0x2000, Z80_TAG, 0 ) \
	ROM_DEFAULT_BIOS( "v17" ) \
	ROM_SYSTEM_BIOS( 0, "v15", "v1.5" ) \
	ROMX_LOAD( "prof80v15.z7", 0x0000, 0x2000, CRC(8f74134c) SHA1(83f9dcdbbe1a2f50006b41d406364f4d580daa1f), ROM_BIOS(1) ) \
	ROM_SYSTEM_BIOS( 1, "v16", "v1.6" ) \
	ROMX_LOAD( "prof80v16.z7", 0x0000, 0x2000, CRC(7d3927b3) SHA1(bcc15fd04dbf1d6640115be595255c7b9d2a7281), ROM_BIOS(2) ) \
	ROM_SYSTEM_BIOS( 2, "v17", "v1.7" ) \
	ROMX_LOAD( "prof80v17.z7", 0x0000, 0x2000, CRC(53305ff4) SHA1(3ea209093ac5ac8a5db618a47d75b705965cdf44), ROM_BIOS(3) ) \

ROM_START( prof80 )
	ROM_PROF80
ROM_END

ROM_START( prof80g21 )
	ROM_PROF80

	ROM_REGION( 0x4000, GRIP_Z80_TAG, 0 )
	ROM_LOAD( "grip21.z2", 0x0000, 0x4000, CRC(7f6a37dd) SHA1(2e89f0b0c378257ff7e41c50d57d90865c6e214b) )
ROM_END

ROM_START( prof80g25 )
	ROM_PROF80

	ROM_REGION( 0x4000, GRIP_Z80_TAG, 0 )
	ROM_LOAD( "grip25.z2", 0x0000, 0x4000, CRC(49ebb284) SHA1(0a7eaaf89da6db2750f820146c8f480b7157c6c7) )
ROM_END

ROM_START( prof80g26 )
	ROM_PROF80

	ROM_REGION( 0x4000, GRIP_Z80_TAG, 0 )
	ROM_LOAD( "grip26.z2", 0x0000, 0x4000, CRC(a1c424f0) SHA1(83942bc75b9475f044f936b8d9d7540551d87db9) )
ROM_END

ROM_START( prof80g31 )
	ROM_PROF80

	ROM_REGION( 0x4000, GRIP_Z80_TAG, 0 )
	ROM_LOAD( "grip31.z2", 0x0000, 0x4000, CRC(e0e4e8ab) SHA1(73d3d14c9b06fed0c187fb0fffe5ec035d8dd256) )
ROM_END

ROM_START( prof80g562 )
	ROM_PROF80

	ROM_REGION( 0x8000, GRIP_Z80_TAG, 0 )
	ROM_LOAD( "grip562.z2", 0x0000, 0x8000, CRC(74be0455) SHA1(1c423ecca6363345a8690ddc45dbafdf277490d3) )
ROM_END

/* System Drivers */

/*    YEAR  NAME        PARENT  COMPAT  MACHINE INPUT   INIT    COMPANY                 FULLNAME                FLAGS */
COMP( 1984, prof80,     0,		0,		prof80,	prof80,	0,		"Conitec Datensysteme",	"PROF-80",				GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1984, prof80g21,	prof80,	0,		grip2,	grip,	0,		"Conitec Datensysteme",	"PROF-80 (GRIP-2.1)",	GAME_NOT_WORKING )
COMP( 1984, prof80g25,	prof80,	0,		grip2,	grip,	0,		"Conitec Datensysteme",	"PROF-80 (GRIP-2.5)",	GAME_NOT_WORKING )
COMP( 1984, prof80g26,	prof80,	0,		grip2,	grip,	0,		"Conitec Datensysteme",	"PROF-80 (GRIP-2.6)",	GAME_NOT_WORKING )
COMP( 1984, prof80g31,	prof80,	0,		grip3,	grip,	0,		"Conitec Datensysteme",	"PROF-80 (GRIP-3.1)",	GAME_NOT_WORKING )
COMP( 1984, prof80g562,	prof80,	0,		grip5,	grip,	0,		"Conitec Datensysteme",	"PROF-80 (GRIP-5.62)",	GAME_NOT_WORKING )
