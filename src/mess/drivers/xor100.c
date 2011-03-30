/****************************************************************************************************

        XOR S-100-12

*****************************************************************************************************

        Summary of Monitor commands:

        D xxxx yyyy = dump memory to screen
        F xxxx yyyy zz = fill memory from xxxx to yyyy-1 with zz
        G xxxx         = execute program at xxxx
        H xxxx yyyy aa bb...  = unknown
        L xxxx         = edit memory (. to exit)
        M xxxx yyyy zzzz = Move (copy) memory
        V xxxx           = unknown
    X n     = Select a bank (0 works, others freeze)

        Note some of the commands are a bit buggy, eg F doesn't fill the last byte

*****************************************************************************************************/

/*

    TODO:

    - floppy images are bad
    - honor jumper settings
    - CTC signal header
    - serial printer
    - cassette?

*/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "formats/basicdsk.h"
#include "imagedev/flopdrv.h"
#include "machine/ram.h"
#include "machine/com8116.h"
#include "machine/ctronics.h"
#include "machine/i8255a.h"
#include "machine/msm8251.h"
#include "machine/terminal.h"
#include "machine/wd17xx.h"
#include "machine/z80ctc.h"
#include "includes/xor100.h"

/* Read/Write Handlers */

enum
{
	EPROM_0000 = 0,
	EPROM_F800,
	EPROM_OFF
};

void xor100_state::bankswitch()
{
	address_space *program = m_maincpu->memory().space(AS_PROGRAM);
	int banks = ram_get_size(m_ram) / 0x10000;

	switch (m_mode)
	{
	case EPROM_0000:
		if (m_bank < banks)
		{
			program->install_write_bank(0x0000, 0xffff, "bank1");
			memory_set_bank(m_machine, "bank1", 1 + m_bank);
		}
		else
		{
			program->unmap_write(0x0000, 0xffff);
		}

		program->install_read_bank(0x0000, 0xf7ff, 0x07ff, 0, "bank2");
		program->install_read_bank(0xf800, 0xffff, "bank3");
		memory_set_bank(m_machine, "bank2", 0);
		memory_set_bank(m_machine, "bank3", 0);
		break;

	case EPROM_F800:
		if (m_bank < banks)
		{
			program->install_write_bank(0x0000, 0xffff, "bank1");
			program->install_read_bank(0x0000, 0xf7ff, "bank2");
			memory_set_bank(m_machine, "bank1", 1 + m_bank);
			memory_set_bank(m_machine, "bank2", 1 + m_bank);
		}
		else
		{
			program->unmap_write(0x0000, 0xffff);
			program->unmap_read(0x0000, 0xf7ff);
		}

		program->install_read_bank(0xf800, 0xffff, "bank3");
		memory_set_bank(m_machine, "bank3", 0);
		break;

	case EPROM_OFF:
		if (m_bank < banks)
		{
			program->install_write_bank(0x0000, 0xffff, "bank1");
			program->install_read_bank(0x0000, 0xf7ff, "bank2");
			program->install_read_bank(0xf800, 0xffff, "bank3");
			memory_set_bank(m_machine, "bank1", 1 + m_bank);
			memory_set_bank(m_machine, "bank2", 1 + m_bank);
			memory_set_bank(m_machine, "bank3", 1 + m_bank);
		}
		else
		{
			program->unmap_write(0x0000, 0xffff);
			program->unmap_read(0x0000, 0xf7ff);
			program->unmap_read(0xf800, 0xffff);
		}
		break;
	}
}

WRITE8_MEMBER( xor100_state::mmu_w )
{
	/*

        bit     description

        0       A16
        1       A17
        2       A18
        3       A19
        4
        5
        6
        7

    */

	m_bank = data & 0x07;

	bankswitch();
}

WRITE8_MEMBER( xor100_state::prom_toggle_w )
{
	switch (m_mode)
	{
	case EPROM_OFF: m_mode = EPROM_F800; break;
	case EPROM_F800: m_mode = EPROM_OFF; break;
	}

	bankswitch();
}

READ8_MEMBER( xor100_state::prom_disable_r )
{
	m_mode = EPROM_F800;

	bankswitch();

	return 0xff;
}

static WRITE8_DEVICE_HANDLER( baud_w )
{
	com8116_str_w(device, 0, data & 0x0f);
	com8116_stt_w(device, 0, data >> 4);
}

WRITE8_MEMBER( xor100_state::i8251_b_data_w )
{
	msm8251_data_w(m_uart_b, 0, data);
	terminal_write(m_terminal, 0, data);
}

READ8_MEMBER( xor100_state::fdc_wait_r )
{
	/*

        bit     description

        0
        1
        2
        3
        4
        5
        6
        7       FDC IRQ

    */

	if (!m_fdc_irq && !m_fdc_drq)
	{
		/* TODO: this is really connected to the Z80 _RDY line */
		device_set_input_line(m_maincpu, INPUT_LINE_HALT, ASSERT_LINE);
	}

	return !m_fdc_irq << 7;
}

WRITE8_MEMBER( xor100_state::fdc_dcont_w )
{
	/*

        bit     description

        0       DS0
        1       DS1
        2       DS2
        3       DS3
        4
        5
        6
        7       _HLSTB

    */

	/* drive select */
	if (BIT(data, 0)) wd17xx_set_drive(m_fdc, 0);
	if (BIT(data, 1)) wd17xx_set_drive(m_fdc, 1);

	floppy_mon_w(m_floppy0, CLEAR_LINE);
	floppy_mon_w(m_floppy1, CLEAR_LINE);
	floppy_drive_set_ready_state(m_floppy0, 1, 1);
	floppy_drive_set_ready_state(m_floppy1, 1, 1);
}

WRITE8_MEMBER( xor100_state::fdc_dsel_w )
{
	/*

        bit     description

        0       J
        1       K
        2
        3
        4
        5
        6
        7

    */

	switch (data & 0x03)
	{
	case 0: break;
	case 1: m_fdc_dden = 1; break;
	case 2: m_fdc_dden = 0; break;
	case 3: m_fdc_dden = !m_fdc_dden; break;
	}

	wd17xx_dden_w(m_fdc, m_fdc_dden);
}

/* Memory Maps */

static ADDRESS_MAP_START( xor100_mem, AS_PROGRAM, 8, xor100_state )
	AM_RANGE(0x0000, 0xffff) AM_WRITE_BANK("bank1")
	AM_RANGE(0x0000, 0xf7ff) AM_READ_BANK("bank2")
	AM_RANGE(0xf800, 0xffff) AM_READ_BANK("bank3")
ADDRESS_MAP_END

static ADDRESS_MAP_START( xor100_io, AS_IO, 8, xor100_state )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_DEVREADWRITE_LEGACY(I8251_A_TAG, msm8251_data_r, msm8251_data_w)
	AM_RANGE(0x01, 0x01) AM_DEVREADWRITE_LEGACY(I8251_A_TAG, msm8251_status_r, msm8251_control_w)
	AM_RANGE(0x02, 0x02) AM_DEVREAD_LEGACY(I8251_B_TAG, msm8251_data_r) AM_WRITE(i8251_b_data_w)
	AM_RANGE(0x03, 0x03) AM_DEVREADWRITE_LEGACY(I8251_B_TAG, msm8251_status_r, msm8251_control_w)
	AM_RANGE(0x04, 0x07) AM_DEVREADWRITE_LEGACY(I8255A_TAG, i8255a_r, i8255a_w)
	AM_RANGE(0x08, 0x08) AM_WRITE(mmu_w)
	AM_RANGE(0x09, 0x09) AM_WRITE(prom_toggle_w)
	AM_RANGE(0x0a, 0x0a) AM_READ(prom_disable_r)
	AM_RANGE(0x0b, 0x0b) AM_READ_PORT("DSW0") AM_DEVWRITE_LEGACY(COM5016_TAG, baud_w)
	AM_RANGE(0x0c, 0x0f) AM_DEVREADWRITE_LEGACY(Z80CTC_TAG, z80ctc_r, z80ctc_w)
	AM_RANGE(0xf8, 0xfb) AM_DEVREADWRITE_LEGACY(WD1795_TAG, wd17xx_r, wd17xx_w)
	AM_RANGE(0xfc, 0xfc) AM_READWRITE(fdc_wait_r, fdc_dcont_w)
	AM_RANGE(0xfd, 0xfd) AM_WRITE(fdc_dsel_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( xor100 )

	PORT_START("DSW0")
	PORT_DIPNAME( 0x0f, 0x05, "Serial Port A" )
	PORT_DIPSETTING(    0x00, "50 baud" )
	PORT_DIPSETTING(    0x01, "75 baud" )
	PORT_DIPSETTING(    0x02, "110 baud" )
	PORT_DIPSETTING(    0x03, "134.5 baud" )
	PORT_DIPSETTING(    0x04, "150 baud" )
	PORT_DIPSETTING(    0x05, "300 baud" )
	PORT_DIPSETTING(    0x06, "600 baud" )
	PORT_DIPSETTING(    0x07, "1200 baud" )
	PORT_DIPSETTING(    0x08, "1800 baud" )
	PORT_DIPSETTING(    0x09, "2000 baud" )
	PORT_DIPSETTING(    0x0a, "2400 baud" )
	PORT_DIPSETTING(    0x0b, "3600 baud" )
	PORT_DIPSETTING(    0x0c, "4800 baud" )
	PORT_DIPSETTING(    0x0d, "7200 baud" )
	PORT_DIPSETTING(    0x0e, "9600 baud" )
	PORT_DIPSETTING(    0x0f, "19200 baud" )
	PORT_DIPNAME( 0xf0, 0xe0, "Serial Port B" )
	PORT_DIPSETTING(    0x00, "50 baud" )
	PORT_DIPSETTING(    0x10, "75 baud" )
	PORT_DIPSETTING(    0x20, "110 baud" )
	PORT_DIPSETTING(    0x30, "134.5 baud" )
	PORT_DIPSETTING(    0x40, "150 baud" )
	PORT_DIPSETTING(    0x50, "300 baud" )
	PORT_DIPSETTING(    0x60, "600 baud" )
	PORT_DIPSETTING(    0x70, "1200 baud" )
	PORT_DIPSETTING(    0x80, "1800 baud" )
	PORT_DIPSETTING(    0x90, "2000 baud" )
	PORT_DIPSETTING(    0xa0, "2400 baud" )
	PORT_DIPSETTING(    0xb0, "3600 baud" )
	PORT_DIPSETTING(    0xc0, "4800 baud" )
	PORT_DIPSETTING(    0xd0, "7200 baud" )
	PORT_DIPSETTING(    0xe0, "9600 baud" )
	PORT_DIPSETTING(    0xf0, "19200 baud" )

	PORT_START("J1")
	PORT_CONFNAME( 0x01, 0x01, "J1 Wait State")
	PORT_CONFSETTING( 0x00, "No Wait States" )
	PORT_CONFSETTING( 0x01, "1 M1 Wait State" )

	PORT_START("J2")
	PORT_CONFNAME( 0x01, 0x01, "J2 CPU Speed")
	PORT_CONFSETTING( 0x00, "2 MHz" )
	PORT_CONFSETTING( 0x01, "4 MHz" )

	PORT_START("J3")
	PORT_CONFNAME( 0x01, 0x00, "J3")
	PORT_CONFSETTING( 0x00, "" )
	PORT_CONFSETTING( 0x01, "" )

	PORT_START("J4-J5")
	PORT_CONFNAME( 0x01, 0x01, "J4/J5 EPROM Type")
	PORT_CONFSETTING( 0x00, "2708" )
	PORT_CONFSETTING( 0x01, "2716" )

	PORT_START("J6")
	PORT_CONFNAME( 0x01, 0x01, "J6 EPROM")
	PORT_CONFSETTING( 0x00, "Disabled" )
	PORT_CONFSETTING( 0x01, "Enabled" )

	PORT_START("J7")
	PORT_CONFNAME( 0x01, 0x00, "J7 I/O Port Addresses")
	PORT_CONFSETTING( 0x00, "00-0F" )
	PORT_CONFSETTING( 0x01, "10-1F" )

	PORT_START("J8")
	PORT_CONFNAME( 0x01, 0x00, "J8")
	PORT_CONFSETTING( 0x00, "" )
	PORT_CONFSETTING( 0x01, "" )

	PORT_START("J9")
	PORT_CONFNAME( 0x01, 0x01, "J9 Power On RAM")
	PORT_CONFSETTING( 0x00, "Enabled" )
	PORT_CONFSETTING( 0x01, "Disabled" )
INPUT_PORTS_END

/* COM5016 Interface */

static WRITE_LINE_DEVICE_HANDLER( com5016_fr_w )
{
	msm8251_transmit_clock(device);
	msm8251_receive_clock(device);
}

static WRITE_LINE_DEVICE_HANDLER( com5016_ft_w )
{
	msm8251_transmit_clock(device);
	msm8251_receive_clock(device);
}

static COM8116_INTERFACE( com5016_intf )
{
	DEVCB_NULL,					/* fX/4 output */
	DEVCB_DEVICE_LINE(I8251_A_TAG, com5016_fr_w),	/* fR output */
	DEVCB_DEVICE_LINE(I8251_B_TAG, com5016_ft_w),	/* fT output */
	{ 101376, 67584, 46080, 37686, 33792, 16896, 8448, 4224, 2816, 2534, 2112, 1408, 1056, 704, 528, 264 },	// WRONG?
	{ 101376, 67584, 46080, 37686, 33792, 16896, 8448, 4224, 2816, 2534, 2112, 1408, 1056, 704, 528, 264 },	// WRONG?
};

/* Printer 8251A Interface */

static const msm8251_interface printer_8251_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/* Terminal 8251A Interface */

static const msm8251_interface terminal_8251_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/* Printer 8255A Interface */

static READ8_DEVICE_HANDLER( i8255_pc_r )
{
	/*

        bit     description

        PC0
        PC1
        PC2
        PC3
        PC4     ON LINE
        PC5     BUSY
        PC6     _ACK
        PC7

    */

	UINT8 data = 0;

	/* on line */
	data |= centronics_vcc_r(device) << 4;

	/* busy */
	data |= centronics_busy_r(device) << 5;

	return data;
}

static I8255A_INTERFACE( printer_8255_intf )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_HANDLER(CENTRONICS_TAG, i8255_pc_r),
	DEVCB_DEVICE_HANDLER(CENTRONICS_TAG, centronics_data_w),
	DEVCB_DEVICE_LINE(CENTRONICS_TAG, centronics_strobe_w),
	DEVCB_NULL
};

static const centronics_interface xor100_centronics_intf =
{
	0,
	DEVCB_DEVICE_LINE(I8255A_TAG, i8255a_pc4_w),
	DEVCB_NULL,
	DEVCB_NULL
};

/* Z80-CTC Interface */

static WRITE_LINE_DEVICE_HANDLER( ctc_z0_w )
{
}

static WRITE_LINE_DEVICE_HANDLER( ctc_z1_w )
{
}

static WRITE_LINE_DEVICE_HANDLER( ctc_z2_w )
{
}

static Z80CTC_INTERFACE( ctc_intf )
{
	0,              	/* timer disables */
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	/* interrupt handler */
	DEVCB_LINE(ctc_z0_w),			/* ZC/TO0 callback */
	DEVCB_LINE(ctc_z1_w),			/* ZC/TO1 callback */
	DEVCB_LINE(ctc_z2_w)    		/* ZC/TO2 callback */
};

/* WD1795-02 Interface */

WRITE_LINE_MEMBER( xor100_state::fdc_irq_w )
{
	m_fdc_irq = state;
	z80ctc_trg0_w(m_ctc, state);

	if (state)
	{
		/* TODO: this is really connected to the Z80 _RDY line */
		device_set_input_line(m_maincpu, INPUT_LINE_HALT, CLEAR_LINE);
	}
}

WRITE_LINE_MEMBER( xor100_state::fdc_drq_w )
{
	m_fdc_drq = state;

	if (state)
	{
		/* TODO: this is really connected to the Z80 _RDY line */
		device_set_input_line(m_maincpu, INPUT_LINE_HALT, CLEAR_LINE);
	}
}

static const wd17xx_interface fdc_intf =
{
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(xor100_state, fdc_irq_w),
	DEVCB_DRIVER_LINE_MEMBER(xor100_state, fdc_drq_w),
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};

/* Terminal Interface */

static WRITE8_DEVICE_HANDLER( xor100_kbd_put )
{
	msm8251_receive_character(device, data);
}

static GENERIC_TERMINAL_INTERFACE( xor100_terminal_intf )
{
	DEVCB_DEVICE_HANDLER(I8251_B_TAG, xor100_kbd_put)
};

/* Machine Initialization */

void xor100_state::machine_start()
{
	int banks = ram_get_size(m_ram) / 0x10000;
	UINT8 *ram = ram_get_ptr(m_ram);
	UINT8 *rom = m_machine.region(Z80_TAG)->base();

	/* setup memory banking */
	memory_configure_bank(m_machine, "bank1", 1, banks, ram, 0x10000);
	memory_configure_bank(m_machine, "bank2", 0, 1, rom, 0);
	memory_configure_bank(m_machine, "bank2", 1, banks, ram, 0x10000);
	memory_configure_bank(m_machine, "bank3", 0, 1, rom, 0);
	memory_configure_bank(m_machine, "bank3", 1, banks, ram + 0xf800, 0x10000);

	/* register for state saving */
	state_save_register_global(m_machine, m_mode);
	state_save_register_global(m_machine, m_bank);
	state_save_register_global(m_machine, m_fdc_irq);
	state_save_register_global(m_machine, m_fdc_drq);
	state_save_register_global(m_machine, m_fdc_dden);
}

void xor100_state::machine_reset()
{
	m_mode = EPROM_0000;

	bankswitch();
}

/* Machine Driver */

static const floppy_config xor100_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSDD,
	FLOPPY_OPTIONS_NAME(default),
	NULL
};

static MACHINE_CONFIG_START( xor100, xor100_state )
	/* basic machine hardware */
	MCFG_CPU_ADD(Z80_TAG, Z80, XTAL_8MHz/2)
	MCFG_CPU_PROGRAM_MAP(xor100_mem)
	MCFG_CPU_IO_MAP(xor100_io)

	/* video hardware */
	MCFG_FRAGMENT_ADD( generic_terminal )

	/* devices */
	MCFG_MSM8251_ADD(I8251_A_TAG, /*XTAL_8MHz/2,*/ printer_8251_intf)
	MCFG_MSM8251_ADD(I8251_B_TAG, /*XTAL_8MHz/2,*/ terminal_8251_intf)
	MCFG_I8255A_ADD(I8255A_TAG, printer_8255_intf)
	MCFG_Z80CTC_ADD(Z80CTC_TAG, XTAL_8MHz/2, ctc_intf)
	MCFG_COM8116_ADD(COM5016_TAG, 5000000, com5016_intf)
	MCFG_WD179X_ADD(WD1795_TAG, /*XTAL_8MHz/8,*/ fdc_intf)
	MCFG_FLOPPY_2_DRIVES_ADD(xor100_floppy_config)
	MCFG_CENTRONICS_ADD(CENTRONICS_TAG, xor100_centronics_intf)
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, xor100_terminal_intf)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("64K")
	MCFG_RAM_EXTRA_OPTIONS("128K,192K,256K,320K,384K,448K,512K")
MACHINE_CONFIG_END

/* ROMs */

ROM_START( xor100 )
	ROM_REGION( 0x800, Z80_TAG, 0 )
	ROM_SYSTEM_BIOS( 0, "v185", "v1.85" )
	ROMX_LOAD( "xp 185.8b", 0x000, 0x800, CRC(0d0bda8d) SHA1(11c83f7cd7e6a570641b44a2f2cc5737a7dd8ae3), ROM_BIOS(1) )
ROM_END

/* System Drivers */

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT    COMPANY                 FULLNAME        FLAGS */
COMP( 1980, xor100,		0,		0,		xor100,		xor100,		0,		"Xor Data Science",		"XOR S-100-12",	GAME_SUPPORTS_SAVE | GAME_NO_SOUND)
