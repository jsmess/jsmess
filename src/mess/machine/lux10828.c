/*

Luxor Conkort

PCB Layout
----------

55 10900-01

|-----------------------------------|
|   LD1 SW1 LS132       CON2        |
|                               S   |
|   4MHz        S240    N8T97N      |
|   S                               |
|   7404            MC1458  4024    |
|       S                           |
|   74276   S                       |
|           S   S                   |
|   LS32    S   C140E       LS273   |
|                                   |
|                                   |
|       MB8876          ROM         |
|                                   |
|                                   |
|   S   LS32    LS156       TC5514  |
|                                   |
|                           TC5514  |
|       Z80                         |
|                           LS273   |
|                                   |
|                           LS373   |
|       Z80PIO                      |
|                           LS245   |
|   S1                              |
|   LS125   LS124     S20   DM8131  |
|                                   |
|--|-----------------------------|--|
   |------------CON1-------------|

Notes:
    All IC's shown.

    ROM     - Hitachi HN462716 2Kx8 EPROM "MPI02"
    Z80     - Sharp LH0080A Z80A CPU
    Z80PIO  - SGS Z8420AB1 Z80A PIO
    MB8876  - Mitsubishi MB8876 Floppy Disc Controller (FD1791 compatible)
    TC5514  - Toshiba TC5514AP-2 1Kx4 bit Static RAM
    DM8131  - National Semiconductor DM8131N 6-Bit Unified Bus Comparator
    C140E   - Ferranti 2C140E "copy protection device"
    N8T97N  - SA N8T97N ?
    CON1    - ABC bus connector
    CON2    - 25-pin D sub floppy connector (AMP4284)
    SW1     - Disk drive type (SS/DS, SD/DD)
    S1      - ABC bus card address bit 0
    S2      -
    S3      -
    S4      -
    S5      -
    S6      -
    S7      -
    S8      -
    S9      -
    LD1     - LED

*/

/*

    TODO:

    - Z80 IN instruction needs to halt in mid air for this controller to ever work (the first data byte of disk sector is read too early)

        wd17xx_command_w $88 READ_SEC
        wd17xx_data_r: (no new data) $00 (data_count 0)
        WAIT
        wd179x: Read Sector callback.
        sector found! C:$00 H:$00 R:$0b N:$01
        wd17xx_data_r: $FF (data_count 256)
        WAIT

    - copy protection device (sends sector header bytes to CPU? DDEN is serial clock? code checks for either $b6 or $f7)

        06F8: ld   a,$2F                    ; SEEK
        06FA: out  ($BC),a
        06FC: push af
        06FD: push bc
        06FE: ld   bc,$0724
        0701: push bc
        0702: ld   b,$07
        0704: rr   a
        0706: call $073F
        073F: DB 7E         in   a,($7E)    ; PIO PORT B
        0741: EE 08         xor  $08        ; DDEN
        0743: D3 7E         out  ($7E),a
        0745: EE 08         xor  $08
        0747: D3 7E         out  ($7E),a
        0749: DB 7E         in   a,($7E)
        074B: 1F            rra
        074C: 1F            rra
        074D: 1F            rra
        074E: CB 11         rl   c
        0750: 79            ld   a,c
        0751: C9            ret
        0709: djnz $0703            <-- jumps to middle of instruction!
        0703: rlca
        0704: rr   a
        0706: call $073F

    - S2-S5 jumpers
    - ABC80 ERR 48 on boot
    - side select makes controller go crazy and try to WRITE_TRK

*/

#include "lux10828.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define Z80_TAG		"5a"
#define Z80PIO_TAG	"3a"
#define FD1791_TAG	"7a"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type LUXOR_55_10828 = luxor_55_10828_device_config::static_alloc_device_config;



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  luxor_55_10828_device_config - constructor
//-------------------------------------------------

luxor_55_10828_device_config::luxor_55_10828_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
	: device_config(mconfig, static_alloc_device_config, "Luxor 55 10828", tag, owner, clock),
	  device_config_abcbus_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  static_alloc_device_config - allocate a new
//  configuration object
//-------------------------------------------------

device_config *luxor_55_10828_device_config::static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
{
	return global_alloc(luxor_55_10828_device_config(mconfig, tag, owner, clock));
}


//-------------------------------------------------
//  alloc_device - allocate a new device object
//-------------------------------------------------

device_t *luxor_55_10828_device_config::alloc_device(running_machine &machine) const
{
	return auto_alloc(machine, luxor_55_10828_device(machine, *this));
}


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void luxor_55_10828_device_config::device_config_complete()
{
	// inherit a copy of the static data
	const luxor_55_10828_interface *intf = reinterpret_cast<const luxor_55_10828_interface *>(static_config());
	if (intf != NULL)
		*static_cast<luxor_55_10828_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		fatalerror("Interface not provided!");
	}

	m_sw1 = intf->m_sw1;
	m_drive_type = intf->m_drive_type;
	m_s1 = intf->m_s1;
	
	m_shortname = "lux10828";
}


//-------------------------------------------------
//  ROM( luxor_55_10828 )
//-------------------------------------------------

ROM_START( luxor_55_10828 )
	ROM_REGION( 0x800, "abc830", 0 )
	ROM_LOAD_OPTIONAL( "basf .02.7c", 0x0000, 0x0800, CRC(5daba200) SHA1(7881933760bed3b94f27585c0a6fc43e5d5153f5) ) // BASF 6106/08
	ROM_LOAD_OPTIONAL( "mpi .02.7c",  0x0000, 0x0800, CRC(2aac9296) SHA1(c01a62e7933186bdf7068d2e9a5bc36590544349) ) // MPI 51 (55 10760-01)
	ROM_LOAD( "new mpi .02.7c",       0x0000, 0x0800, CRC(ab788171) SHA1(c8e29965c04c85f2f2648496ea10c9c7ff95392f) ) // MPI 51 (55 10760-01)

	ROM_REGION( 0x800, "abc832", 0 )
	ROM_LOAD_OPTIONAL( "micr 1.4.7c", 0x0000, 0x0800, CRC(a7bc05fa) SHA1(6ac3e202b7ce802c70d89728695f1cb52ac80307) ) // Micropolis 1015 (v1.4)
	ROM_LOAD_OPTIONAL( "micr 2.3.7c", 0x0000, 0x0800, CRC(f2fc5ccc) SHA1(86d6baadf6bf1d07d0577dc1e092850b5ff6dd1b) ) // Micropolis 1115 (v2.3)
	ROM_LOAD_OPTIONAL( "basf 1.2.7c", 0x0000, 0x0800, CRC(9ca1a1eb) SHA1(04973ad69de8da403739caaebe0b0f6757e4a6b1) ) // BASF 6118 (v1.2)

	ROM_REGION( 0x800, "abc838", 0 )
	ROM_LOAD_OPTIONAL( "basf 8 1.0.7c", 0x0000, 0x0800, NO_DUMP ) // BASF 6104, BASF 6115 (v1.0)
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *luxor_55_10828_device_config::device_rom_region() const
{
	return ROM_NAME( luxor_55_10828 );
}


//-------------------------------------------------
//  ADDRESS_MAP( luxor_55_10828_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( luxor_55_10828_mem, AS_PROGRAM, 8, luxor_55_10828_device )
	ADDRESS_MAP_GLOBAL_MASK(0x1fff)
	AM_RANGE(0x0000, 0x07ff) AM_MIRROR(0x0800) AM_ROM AM_REGION("luxor_55_10828:abc830", 0)
	AM_RANGE(0x1000, 0x13ff) AM_MIRROR(0x0c00) AM_RAM
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( luxor_55_10828_io )
//-------------------------------------------------

static ADDRESS_MAP_START( luxor_55_10828_io, AS_IO, 8, luxor_55_10828_device )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x70, 0x73) AM_MIRROR(0x0c) AM_DEVREADWRITE_LEGACY(Z80PIO_TAG, z80pio_ba_cd_r, z80pio_ba_cd_w)
	AM_RANGE(0xb0, 0xb3) AM_MIRROR(0x0c) AM_DEVREADWRITE(DEVICE_SELF_OWNER, luxor_55_10828_device, fdc_r, fdc_w)
	AM_RANGE(0xd0, 0xd0) AM_MIRROR(0x0f) AM_DEVWRITE(DEVICE_SELF_OWNER, luxor_55_10828_device, status_w)
	AM_RANGE(0xe0, 0xe0) AM_MIRROR(0x0f) AM_DEVWRITE(DEVICE_SELF_OWNER, luxor_55_10828_device, ctrl_w)
ADDRESS_MAP_END


//-------------------------------------------------
//  Z80PIO_INTERFACE( pio_intf )
//-------------------------------------------------

READ8_MEMBER( luxor_55_10828_device::pio_pa_r )
{
	return m_data;
}

WRITE8_MEMBER( luxor_55_10828_device::pio_pa_w )
{
	m_data = data;
}

READ8_MEMBER( luxor_55_10828_device::pio_pb_r )
{
	/*

        bit     description

        0       !(_DS0 & _DS1)  single/double sided (0=SS, 1=DS)
        1       !(_DD0 & _DD1)  single/double density (0=DS, 1=DD)
        2       8B pin 10
        3       FDC _DDEN       double density enable
        4       _R/BS           radial/binary drive select
        5       FDC HLT         head load timing
        6       FDC _HDLD       head loaded
        7       FDC IRQ         interrupt request

    */

	UINT8 data = 0x04;

	// single/double sided drive
	UINT8 sw1 = input_port_read(this, "SW1") & 0x0f;
	int ds0 = m_sel0 ? BIT(sw1, 0) : 1;
	int ds1 = m_sel1 ? BIT(sw1, 1) : 1;
	data |= !(ds0 & ds1);

	// single/double density drive
	int dd0 = m_sel0 ? BIT(sw1, 2) : 1;
	int dd1 = m_sel1 ? BIT(sw1, 3) : 1;
	data |= !(dd0 & dd1) << 1;

	// radial/binary drive select
	data |= 0x10;

	// head load
//  data |= wd17xx_hdld_r(device) << 6;
	data |= 0x40;

	// FDC interrupt request
	data |= m_fdc_irq << 7;

	return data;
}

WRITE8_MEMBER( luxor_55_10828_device::pio_pb_w )
{
	/*

        bit     signal          description

        0       !(_DS0 & _DS1)  single/double sided (0=SS, 1=DS)
        1       !(_DD0 & _DD1)  single/double density (0=DS, 1=DD)
        2       8B pin 10
        3       FDC _DDEN       double density enable
        4       _R/BS           radial/binary drive select
        5       FDC HLT         head load timing
        6       FDC _HDLD       head loaded
        7       FDC IRQ         interrupt request

    */

	// double density enable
	wd17xx_dden_w(m_fdc, BIT(data, 3));

	// head load timing
//  wd17xx_hlt_w(m_fdc, BIT(data, 5));
}

static Z80PIO_INTERFACE( pio_intf )
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, luxor_55_10828_device, pio_pa_r),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, luxor_55_10828_device, pio_pa_w),
	DEVCB_NULL,
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, luxor_55_10828_device, pio_pb_r),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, luxor_55_10828_device, pio_pb_w),
	DEVCB_NULL
};


//-------------------------------------------------
//  z80_daisy_config daisy_chain
//-------------------------------------------------

static const z80_daisy_config daisy_chain[] =
{
	{ Z80PIO_TAG },
	{ NULL }
};


//-------------------------------------------------
//  wd17xx_interface fdc_intf
//-------------------------------------------------

WRITE_LINE_MEMBER( luxor_55_10828_device::fdc_intrq_w )
{
	m_fdc_irq = state;
	z80pio_pb_w(m_pio, 0, state << 7);

	if (state)
	{
		// TODO: this is really connected to the Z80 WAIT line
		device_set_input_line(m_maincpu, INPUT_LINE_HALT, CLEAR_LINE);
	}
}

WRITE_LINE_MEMBER( luxor_55_10828_device::fdc_drq_w )
{
	m_fdc_drq = state;

	if (state)
	{
		// TODO: this is really connected to the Z80 WAIT line
		device_set_input_line(m_maincpu, INPUT_LINE_HALT, CLEAR_LINE);
	}
}

static const wd17xx_interface fdc_intf =
{
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, luxor_55_10828_device, fdc_intrq_w),
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, luxor_55_10828_device, fdc_drq_w),
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};


//-------------------------------------------------
//  MACHINE_DRIVER( luxor_55_10828 )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( luxor_55_10828 )
	// main CPU
	MCFG_CPU_ADD(Z80_TAG, Z80, XTAL_4MHz/2)
	MCFG_CPU_PROGRAM_MAP(luxor_55_10828_mem)
	MCFG_CPU_IO_MAP(luxor_55_10828_io)
	MCFG_CPU_CONFIG(daisy_chain)

	// devices
	MCFG_Z80PIO_ADD(Z80PIO_TAG, XTAL_4MHz/2, pio_intf)
	MCFG_WD179X_ADD(FD1791_TAG, fdc_intf)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor luxor_55_10828_device_config::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( luxor_55_10828 );
}


//-------------------------------------------------
//  INPUT_PORTS( luxor_55_10828 )
//-------------------------------------------------

INPUT_PORTS_START( luxor_55_10828 )
	PORT_START("SW1")
	PORT_DIPNAME( 0x01, 0x01, "Drive 0 Sided" ) PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Single ) )
	PORT_DIPSETTING(    0x00, "Double" )
	PORT_DIPNAME( 0x02, 0x02, "Drive 1 Sided" ) PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Single ) )
	PORT_DIPSETTING(    0x00, "Double" )
	PORT_DIPNAME( 0x04, 0x00, "Drive 0 Density" ) PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Single ) )
	PORT_DIPSETTING(    0x00, "Double" )
	PORT_DIPNAME( 0x08, 0x00, "Drive 1 Density" ) PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Single ) )
	PORT_DIPSETTING(    0x00, "Double" )

	PORT_START("S1")
	PORT_DIPNAME( 0x01, 0x01, "Card Address" ) PORT_DIPLOCATION("S1:1")
	PORT_DIPSETTING(    0x00, "44 (ABC 832/834/850)" )
	PORT_DIPSETTING(    0x01, "45 (ABC 830)" )
INPUT_PORTS_END


//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

const input_port_token *luxor_55_10828_device_config::device_input_ports() const
{
	return INPUT_PORTS_NAME( luxor_55_10828 );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  luxor_55_10828_device - constructor
//-------------------------------------------------

luxor_55_10828_device::luxor_55_10828_device(running_machine &_machine, const luxor_55_10828_device_config &_config)
    : device_t(_machine, _config),
	  device_abcbus_interface(_machine, _config, *this),
	  m_maincpu(*this, Z80_TAG),
	  m_pio(*this, Z80PIO_TAG),
	  m_fdc(*this, FD1791_TAG),
	  m_image0(m_machine.device(FLOPPY_0)),
	  m_image1(m_machine.device(FLOPPY_1)),
	  m_cs(false),
	  m_fdc_irq(0),
	  m_fdc_drq(0),
	  m_wait_enable(0),
	  m_sel0(0),
	  m_sel1(0),
      m_config(_config)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void luxor_55_10828_device::device_start()
{
	// state saving
	save_item(NAME(m_cs));
	save_item(NAME(m_status));
	save_item(NAME(m_data));
	save_item(NAME(m_fdc_irq));
	save_item(NAME(m_fdc_drq));
	save_item(NAME(m_wait_enable));
	save_item(NAME(m_sel0));
	save_item(NAME(m_sel1));

	// patch out protection checks
	UINT8 *rom = subregion("abc830")->base();
	rom[0x00fa] = 0xff;
	rom[0x0336] = 0xff;
	rom[0x0718] = 0xff;
	rom[0x072c] = 0xff;
	rom[0x0771] = 0xff;
	rom[0x0788] = 0xff;
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void luxor_55_10828_device::device_reset()
{
	m_cs = false;
}



//**************************************************************************
//  ABC BUS INTERFACE
//**************************************************************************

//-------------------------------------------------
//	abcbus_cs - 
//-------------------------------------------------

void luxor_55_10828_device::abcbus_cs(UINT8 data)
{
	UINT8 address = 0x2c | BIT(input_port_read(this, "S1"), 0);

	m_cs = (data == address);
}


//-------------------------------------------------
//  abcbus_rst - 
//-------------------------------------------------

void luxor_55_10828_device::abcbus_rst(int state)
{
	if (!state)
	{
		device_reset();
	}
}


//-------------------------------------------------
//  abcbus_stat - 
//-------------------------------------------------

UINT8 luxor_55_10828_device::abcbus_stat()
{
	UINT8 data = 0xff;

	if (m_cs)
	{
		data = (m_status & 0xfe) | z80pio_ardy_r(m_pio);
	}

	return data;
}


//-------------------------------------------------
//  abcbus_inp - 
//-------------------------------------------------

UINT8 luxor_55_10828_device::abcbus_inp()
{
	UINT8 data = 0xff;

	if (m_cs)
	{
		if (!BIT(m_status, 6))
		{
			data = m_data;
		}

		z80pio_astb_w(m_pio, 0);
		z80pio_astb_w(m_pio, 1);
	}

	return data;
}


//-------------------------------------------------
//  abcbus_utp - 
//-------------------------------------------------

void luxor_55_10828_device::abcbus_utp(UINT8 data)
{
	if (!m_cs) return;

	if (BIT(m_status, 6))
	{
		m_data = data;
	}

	z80pio_astb_w(m_pio, 0);
	z80pio_astb_w(m_pio, 1);
}


//-------------------------------------------------
//  abcbus_c1 - 
//-------------------------------------------------

void luxor_55_10828_device::abcbus_c1(UINT8 data)
{
	if (m_cs)
	{
		device_set_input_line(m_maincpu, INPUT_LINE_NMI, ASSERT_LINE);
		device_set_input_line(m_maincpu, INPUT_LINE_NMI, CLEAR_LINE);
	}
}


//-------------------------------------------------
//  abcbus_c3 - 
//-------------------------------------------------

void luxor_55_10828_device::abcbus_c3(UINT8 data)
{
	if (m_cs)
	{
		device_set_input_line(m_maincpu, INPUT_LINE_RESET, ASSERT_LINE);
		device_set_input_line(m_maincpu, INPUT_LINE_RESET, CLEAR_LINE);
	}
}



//**************************************************************************
//  IMPLEMENTATION
//**************************************************************************

//-------------------------------------------------
//  ctrl_w - 
//-------------------------------------------------

WRITE8_MEMBER( luxor_55_10828_device::ctrl_w )
{
	/*

        bit     signal          description

        0       SEL 0
        1       SEL 1
        2       SEL 2
        3       _MOT ON
        4       SIDE
        5       _PRECOMP ON
        6       _WAIT ENABLE
        7       FDC _MR

    */

	// drive selection
	if (BIT(data, 0)) wd17xx_set_drive(m_fdc, 0);
	if (BIT(data, 1)) wd17xx_set_drive(m_fdc, 1);
//  if (BIT(data, 2)) wd17xx_set_drive(m_fdc, 2);
	m_sel0 = BIT(data, 0);
	m_sel1 = BIT(data, 1);

	// motor enable
	int moton = BIT(data, 3);
	floppy_mon_w(m_image0, moton);
	floppy_mon_w(m_image1, moton);
	floppy_drive_set_ready_state(m_image0, !moton, 1);
	floppy_drive_set_ready_state(m_image1, !moton, 1);

	// side selection
	//wd17xx_set_side(m_fdc, BIT(data, 4));

	// wait enable
	m_wait_enable = BIT(data, 6);

	// FDC master reset
	wd17xx_mr_w(m_fdc, BIT(data, 7));
}


//-------------------------------------------------
//  status_w - 
//-------------------------------------------------

WRITE8_MEMBER( luxor_55_10828_device::status_w )
{
	/*

        bit     description

        0       _INT to main Z80
        1
        2
        3
        4
        5
        6       LS245 DIR
        7

    */

	// interrupt
//  abcbus_int_w(m_bus, BIT(data, 0) ? CLEAR_LINE : ASSERT_LINE);

	m_status = data & 0xfe;
}


//-------------------------------------------------
//  fdc_r - 
//-------------------------------------------------

READ8_MEMBER( luxor_55_10828_device::fdc_r )
{
	UINT8 data = 0xff;

	if (!m_wait_enable && !m_fdc_irq && !m_fdc_drq)
	{
		// TODO: this is really connected to the Z80 WAIT line
		device_set_input_line(m_maincpu, INPUT_LINE_HALT, ASSERT_LINE);
	}

	switch (offset & 0x03)
	{
	case 0: data = wd17xx_status_r(m_fdc, 0); break;
	case 1: data = wd17xx_track_r(m_fdc, 0);  break;
	case 2: data = wd17xx_sector_r(m_fdc, 0); break;
	case 3: data = wd17xx_data_r(m_fdc, 0);   break;
	}

	// FD1791 has inverted data lines
	return data ^ 0xff;
}


//-------------------------------------------------
//  fdc_w - 
//-------------------------------------------------

WRITE8_MEMBER( luxor_55_10828_device::fdc_w )
{
	if (!m_wait_enable && !m_fdc_irq && !m_fdc_drq)
	{
		logerror("WAIT\n");
		// TODO: this is really connected to the Z80 WAIT line
		device_set_input_line(m_maincpu, INPUT_LINE_HALT, ASSERT_LINE);
	}

	// FD1791 has inverted data lines
	data ^= 0xff;

	switch (offset & 0x03)
	{
	case 0: wd17xx_command_w(m_fdc, 0, data); break;
	case 1: wd17xx_track_w(m_fdc, 0, data);   break;
	case 2: wd17xx_sector_w(m_fdc, 0, data);  break;
	case 3: wd17xx_data_w(m_fdc, 0, data);    break;
	}
}
