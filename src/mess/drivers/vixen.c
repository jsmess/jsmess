/*

Osborne 4 Vixen

Main PCB Layout
---------------

TODO

Notes:
    Relevant IC's shown.

    CPU     - Zilog Z8400APS Z80A CPU
    FDC     - SMC FDC1797
    8155    - Intel P8155H
    ROM0    -
    ROM1,2  - AMD AM2732-1DC 4Kx8 EPROM
    CN1     - keyboard connector
    CN2     -
    CN3     -
    CN4     - floppy connector
    CN5     - power connector
    CN6     - composite video connector
    SW1     - reset switch
    SW2     -


I/O PCB Layout
--------------

TODO

Notes:
    Relevant IC's shown.

    8155    - Intel P8155H
    8251    - AMD P8251A
    CN1     - IEEE488 connector
    CN2     - RS232 connector
    CN3     -

*/

/*

    TODO:

    - video line buffer
    - floppy
    - keyboard
    - RS232 RI interrupt
    - PCB layouts

*/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "imagedev/flopdrv.h"
#include "machine/ram.h"
#include "machine/i8155.h"
#include "machine/ieee488.h"
#include "machine/msm8251.h"
#include "machine/wd17xx.h"
#include "sound/discrete.h"
#include "includes/vixen.h"



//**************************************************************************
//  INTERRUPTS
//**************************************************************************

//-------------------------------------------------
//  update_interrupt -
//-------------------------------------------------

void vixen_state::update_interrupt()
{
	int state = (m_cmd_d1 & m_fdint) | m_vsync | (!m_enb_srq_int & !m_srq) | (!m_enb_atn_int & !m_atn) | (!m_enb_xmt_int & m_txrdy) | (!m_enb_rcv_int & m_rxrdy);

	device_set_input_line(m_maincpu, INPUT_LINE_IRQ0, state ? ASSERT_LINE : CLEAR_LINE);
}


//-------------------------------------------------
//  ctl_w - command write
//-------------------------------------------------

WRITE8_MEMBER( vixen_state::ctl_w )
{
	logerror("CTL %u\n", data);

	memory_set_bank(m_machine, "bank3", BIT(data, 0));
}


//-------------------------------------------------
//  status_r - status read
//-------------------------------------------------

READ8_MEMBER( vixen_state::status_r )
{
	/*

        bit     description

        0       VSYNC enable
        1       FDINT enable
        2       VSYNC
        3       1
        4       1
        5       1
        6       1
        7       1

    */

	UINT8 data = 0xf8;

	// vertical sync interrupt enable
	data |= m_cmd_d0;

	// floppy interrupt enable
	data |= m_cmd_d1 << 1;

	// vertical sync
	data |= m_vsync << 2;

	return data;
}


//-------------------------------------------------
//  cmd_w - command write
//-------------------------------------------------

WRITE8_MEMBER( vixen_state::cmd_w )
{
	/*

        bit     description

        0       VSYNC enable
        1       FDINT enable
        2
        3
        4
        5
        6
        7

    */

//  logerror("CMD %u\n", data);

	// vertical sync interrupt enable
	m_cmd_d0 = BIT(data, 0);

	if (!m_cmd_d0)
	{
		// clear vertical sync
		m_vsync = 0;
	}

	// floppy interrupt enable
	m_cmd_d1 = BIT(data, 1);

	update_interrupt();
}


//-------------------------------------------------
//  port3_r - serial status read
//-------------------------------------------------

READ8_MEMBER( vixen_state::port3_r )
{
	/*

        bit     description

        0       RI
        1       DCD
        2       1
        3       1
        4       1
        5       1
        6       1
        7       1

    */

	UINT8 data = 0xff; //0xfc;

	// TODO ring indicator
	//data |= rs232_ri_r(m_rs232);

	// TODO data carrier detect
	//data |= rs232_dcd_r(m_rs232) << 1;

	return data;
}



//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( vixen_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( vixen_mem, AS_PROGRAM, 8, vixen_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xefff) AM_READ_BANK("bank1") AM_WRITE_BANK("bank2")
	AM_RANGE(0xf000, 0xffff) AM_READ_BANK("bank3") AM_WRITE_BANK("bank4") AM_BASE(m_video_ram)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( vixen_io )
//-------------------------------------------------

static ADDRESS_MAP_START( vixen_io, AS_IO, 8, vixen_state )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x03) AM_DEVREADWRITE_LEGACY(FDC1797_TAG, wd17xx_r, wd17xx_w)
	AM_RANGE(0x04, 0x04) AM_MIRROR(0x03) AM_READWRITE(status_r, cmd_w)
	AM_RANGE(0x08, 0x08) AM_MIRROR(0x01) AM_DEVREADWRITE(P8155H_TAG, i8155_device, read, write)
	AM_RANGE(0x0c, 0x0d) AM_DEVWRITE(P8155H_TAG, i8155_device, ale_w)
	AM_RANGE(0x10, 0x10) AM_MIRROR(0x07) AM_DEVREAD_LEGACY(IEEE488_TAG, ieee488_dio_r)
	AM_RANGE(0x18, 0x18) AM_MIRROR(0x07) AM_READ_PORT("IEEE488")
	AM_RANGE(0x20, 0x21) AM_MIRROR(0x04) AM_DEVWRITE(P8155H_IO_TAG, i8155_device, ale_w)
	AM_RANGE(0x28, 0x28) AM_MIRROR(0x05) AM_DEVREADWRITE(P8155H_IO_TAG, i8155_device, read, write)
	AM_RANGE(0x30, 0x30) AM_MIRROR(0x06) AM_DEVREADWRITE_LEGACY(P8251A_TAG, msm8251_data_r, msm8251_data_w)
	AM_RANGE(0x31, 0x31) AM_MIRROR(0x06) AM_DEVREADWRITE_LEGACY(P8251A_TAG, msm8251_status_r, msm8251_control_w)
	AM_RANGE(0x38, 0x38) AM_MIRROR(0x07) AM_READ(port3_r)
//  AM_RANGE(0xf0, 0xff) Hard Disk?
ADDRESS_MAP_END



//**************************************************************************
//  INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_PORTS( vixen )
//-------------------------------------------------

INPUT_PORTS_START( vixen )
	PORT_START("ROW0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("ROW4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("ROW5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("ROW6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("ROW7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("IEEE488")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_READ_LINE_DEVICE(IEEE488_TAG, ieee488_atn_r)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_READ_LINE_DEVICE(IEEE488_TAG, ieee488_dav_r)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_READ_LINE_DEVICE(IEEE488_TAG, ieee488_ndac_r)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_READ_LINE_DEVICE(IEEE488_TAG, ieee488_nrfd_r)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_READ_LINE_DEVICE(IEEE488_TAG, ieee488_eoi_r)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_READ_LINE_DEVICE(IEEE488_TAG, ieee488_srq_r)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_READ_LINE_DEVICE(IEEE488_TAG, ieee488_ifc_r)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_READ_LINE_DEVICE(IEEE488_TAG, ieee488_ren_r)
INPUT_PORTS_END



//**************************************************************************
//  VIDEO
//**************************************************************************

//-------------------------------------------------
//  TIMER_DEVICE_CALLBACK( vsync_tick )
//-------------------------------------------------

static TIMER_DEVICE_CALLBACK( vsync_tick )
{
	vixen_state *state = timer.machine().driver_data<vixen_state>();

	if (state->m_cmd_d0)
	{
		state->m_vsync = 1;
		state->update_interrupt();
	}
}


//-------------------------------------------------
//  VIDEO_START( vixenc )
//-------------------------------------------------

void vixen_state::video_start()
{
	// find memory regions
	m_sync_rom = m_machine.region("video")->base();
	m_char_rom = m_machine.region("chargen")->base();

	// register for state saving
	state_save_register_global(m_machine, m_alt);
	state_save_register_global(m_machine, m_256);
	state_save_register_global(m_machine, m_vsync);
	state_save_register_global_pointer(m_machine, m_video_ram, 0x1000);
}


//-------------------------------------------------
//  SCREEN_UPDATE( vixen )
//-------------------------------------------------

bool vixen_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	for (int txadr = 0; txadr < 26; txadr++)
	{
		for (int scan = 0; scan < 10; scan++)
		{
			for (int chadr = 0; chadr < 128; chadr++)
			{
				UINT16 sync_addr = (txadr << 7) | chadr;
				UINT8 sync_data = m_sync_rom[sync_addr];
				int blank = BIT(sync_data, 4);
				/*
                int clrchadr = BIT(sync_data, 7);
                int hsync = BIT(sync_data, 6);
                int clrtxadr = BIT(sync_data, 5);
                int vsync = BIT(sync_data, 3);
                int comp_sync = BIT(sync_data, 2);

                logerror("SYNC %03x:%02x TXADR %u SCAN %u CHADR %u : COMPSYNC %u VSYNC %u BLANK %u CLRTXADR %u HSYNC %u CLRCHADR %u\n",
                    sync_addr,sync_data,txadr,scan,chadr,comp_sync,vsync,blank,clrtxadr,hsync,clrchadr);
                */

				int reverse = 0;

				UINT16 video_addr = (txadr << 7) | chadr;
				UINT8 video_data = m_video_ram[video_addr];
				UINT16 char_addr = 0;

				if (m_256)
				{
					char_addr = (BIT(video_data, 7) << 11) | (scan << 7) | (video_data & 0x7f);
					reverse = m_alt;
				}
				else
				{
					char_addr = (scan << 7) | (video_data & 0x7f);
					reverse = BIT(video_data, 7);
				}

				UINT8 char_data = m_char_rom[char_addr];

				for (int x = 0; x < 8; x++)
				{
					int color = (BIT(char_data, 7 - x) ^ reverse) & !blank;

					*BITMAP_ADDR16(&bitmap, (txadr * 10) + scan, (chadr * 8) + x) = color;
				}
			}
		}
	}

	return 0;
}



//**************************************************************************
//  SOUND
//**************************************************************************

//-------------------------------------------------
//  DISCRETE_SOUND( vixen )
//-------------------------------------------------

static DISCRETE_SOUND_START( vixen )
	DISCRETE_INPUT_LOGIC(NODE_01)
	DISCRETE_SQUAREWAVE(NODE_02, NODE_01, XTAL_23_9616MHz/15360, 100, 50, 0, 90)
	DISCRETE_OUTPUT(NODE_02, 5000)
DISCRETE_SOUND_END



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  I8155_INTERFACE( i8155_intf )
//-------------------------------------------------

READ8_MEMBER( vixen_state::i8155_pa_r )
{
	UINT8 data = 0xff;

	if (!BIT(m_col, 0)) data &= input_port_read(m_machine, "ROW0");
	if (!BIT(m_col, 1)) data &= input_port_read(m_machine, "ROW1");
	if (!BIT(m_col, 2)) data &= input_port_read(m_machine, "ROW2");
	if (!BIT(m_col, 3)) data &= input_port_read(m_machine, "ROW3");
	if (!BIT(m_col, 4)) data &= input_port_read(m_machine, "ROW4");
	if (!BIT(m_col, 5)) data &= input_port_read(m_machine, "ROW5");
	if (!BIT(m_col, 6)) data &= input_port_read(m_machine, "ROW6");
	if (!BIT(m_col, 7)) data &= input_port_read(m_machine, "ROW7");

	return data;
}

WRITE8_MEMBER( vixen_state::i8155_pb_w )
{
	m_col = data;
}

WRITE8_MEMBER( vixen_state::i8155_pc_w )
{
	/*

        bit     description

        0       DSEL1
        1       DSEL2
        2       DDEN
        3       ALT CHARSET
        4       256 CHARS
        5       BEEP ENB
        6
        7

    */

	// drive select
	if (!BIT(data, 0)) wd17xx_set_drive(m_fdc, 0);
	if (!BIT(data, 1)) wd17xx_set_drive(m_fdc, 1);

	// density select
	wd17xx_dden_w(m_fdc, BIT(data, 2));

	// charset
	m_alt = BIT(data, 3);
	m_256 = BIT(data, 4);

	// beep enable
	discrete_sound_w(m_discrete, NODE_01, BIT(data, 5));
}

static I8155_INTERFACE( i8155_intf )
{
	DEVCB_DRIVER_MEMBER(vixen_state, i8155_pa_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(vixen_state, i8155_pb_w),
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(vixen_state, i8155_pc_w),
	DEVCB_NULL
};


//-------------------------------------------------
//  I8155_INTERFACE( io_i8155_intf )
//-------------------------------------------------

WRITE8_MEMBER( vixen_state::io_i8155_pa_w )
{
	ieee488_dio_w(m_ieee488, m_io_i8155, data);
}

WRITE8_MEMBER( vixen_state::io_i8155_pb_w )
{
	/*

        bit     description

        PB0     ATN
        PB1     DAV
        PB2     NDAC
        PB3     NRFD
        PB4     EOI
        PB5     SRQ
        PB6     IFC
        PB7     REN

    */

	/* data valid */
	ieee488_atn_w(m_ieee488, m_io_i8155, BIT(data, 0));

	/* end or identify */
	ieee488_dav_w(m_ieee488, m_io_i8155, BIT(data, 1));

	/* remote enable */
	ieee488_ndac_w(m_ieee488, m_io_i8155, BIT(data, 2));

	/* attention */
	ieee488_nrfd_w(m_ieee488, m_io_i8155, BIT(data, 3));

	/* interface clear */
	ieee488_eoi_w(m_ieee488, m_io_i8155, BIT(data, 4));

	/* service request */
	ieee488_srq_w(m_ieee488, m_io_i8155, BIT(data, 5));

	/* not ready for data */
	ieee488_ifc_w(m_ieee488, m_io_i8155, BIT(data, 6));

	/* data not accepted */
	ieee488_ren_w(m_ieee488, m_io_i8155, BIT(data, 7));
}

WRITE8_MEMBER( vixen_state::io_i8155_pc_w )
{
	/*

        bit     description

        PC0     select internal clock
        PC1     ENB RING INT
        PC2     ENB RCV INT
        PC3     ENB XMT INT
        PC4     ENB ATN INT
        PC5     ENB SRQ INT
        PC6
        PC7

    */

	m_int_clk = BIT(data, 0);
	m_enb_ring_int = BIT(data, 1);
	m_enb_rcv_int = BIT(data, 2);
	m_enb_xmt_int = BIT(data, 3);
	m_enb_atn_int = BIT(data, 4);
	m_enb_srq_int = BIT(data, 5);
}

WRITE_LINE_MEMBER( vixen_state::io_i8155_to_w )
{
	if (m_int_clk && !state)
	{
		msm8251_transmit_clock(m_usart);
		msm8251_receive_clock(m_usart);
	}
}

static I8155_INTERFACE( io_i8155_intf )
{
    DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(vixen_state, io_i8155_pa_w),
    DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(vixen_state, io_i8155_pb_w),
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(vixen_state, io_i8155_pc_w),
	DEVCB_DRIVER_LINE_MEMBER(vixen_state, io_i8155_to_w)
};


//-------------------------------------------------
//  msm8251_interface usart_intf
//-------------------------------------------------

WRITE_LINE_MEMBER( vixen_state::rxrdy_w )
{
	m_rxrdy = state;
	update_interrupt();
}

WRITE_LINE_MEMBER( vixen_state::txrdy_w )
{
	m_txrdy = state;
	update_interrupt();
}

static const msm8251_interface usart_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(vixen_state, rxrdy_w),
	DEVCB_DRIVER_LINE_MEMBER(vixen_state, txrdy_w),
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  IEEE488_DAISY( ieee488_daisy )
//-------------------------------------------------

WRITE_LINE_MEMBER( vixen_state::srq_w )
{
	m_srq = state;
	update_interrupt();
}

WRITE_LINE_MEMBER( vixen_state::atn_w )
{
	m_atn = state;
	update_interrupt();
}

static IEEE488_DAISY( ieee488_daisy )
{
	{ P8155H_IO_TAG, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_DRIVER_LINE_MEMBER(vixen_state, srq_w), DEVCB_DRIVER_LINE_MEMBER(vixen_state, atn_w), DEVCB_NULL },
	{ NULL }
};


//-------------------------------------------------
//  wd17xx_interface fdc_intf
//-------------------------------------------------

static const floppy_config vixen_floppy_config =
{
    DEVCB_NULL,
	DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    FLOPPY_STANDARD_5_25_SSDD_40,
    FLOPPY_OPTIONS_NAME(default),
    "floppy_5_25"
};

WRITE_LINE_MEMBER( vixen_state::fdint_w )
{
	m_fdint = state;
	update_interrupt();
}

static const wd17xx_interface fdc_intf =
{
	DEVCB_NULL,
    DEVCB_DRIVER_LINE_MEMBER(vixen_state, fdint_w),
	DEVCB_NULL,
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};



//**************************************************************************
//  MACHINE INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  IRQ_CALLBACK( vixen )
//-------------------------------------------------

static IRQ_CALLBACK( vixen_int_ack )
{
	// D0 is pulled low
	return 0xfe;
}


//-------------------------------------------------
//  MACHINE_START( vixen )
//-------------------------------------------------

void vixen_state::machine_start()
{
	// interrupt callback
	device_set_irq_callback(m_maincpu, vixen_int_ack);

	// configure memory banking
	UINT8 *ram = ram_get_ptr(m_ram);

	memory_configure_bank(m_machine, "bank1", 0, 1, ram, 0);
	memory_configure_bank(m_machine, "bank1", 1, 1, m_machine.region(Z8400A_TAG)->base(), 0);

	memory_configure_bank(m_machine, "bank2", 0, 1, ram, 0);
	memory_configure_bank(m_machine, "bank2", 1, 1, m_video_ram, 0);

	memory_configure_bank(m_machine, "bank3", 0, 1, m_video_ram, 0);
	memory_configure_bank(m_machine, "bank3", 1, 1, m_machine.region(Z8400A_TAG)->base(), 0);

	memory_configure_bank(m_machine, "bank4", 0, 1, m_video_ram, 0);

	// register for state saving
	state_save_register_global(m_machine, m_reset);
	state_save_register_global(m_machine, m_col);
	state_save_register_global(m_machine, m_cmd_d0);
	state_save_register_global(m_machine, m_cmd_d1);
	state_save_register_global(m_machine, m_fdint);
}


//-------------------------------------------------
//  MACHINE_RESET( vixen )
//-------------------------------------------------

void vixen_state::machine_reset()
{
	address_space *program = m_maincpu->memory().space(AS_PROGRAM);

	program->install_read_bank(0x0000, 0xefff, 0xfff, 0, "bank1");
	program->install_write_bank(0x0000, 0xefff, 0xfff, 0, "bank2");

	memory_set_bank(m_machine, "bank1", 1);
	memory_set_bank(m_machine, "bank2", 1);
	memory_set_bank(m_machine, "bank3", 1);

	m_reset = 1;

	m_vsync = 0;
	m_cmd_d0 = 0;
	m_cmd_d1 = 0;
	update_interrupt();
}



//**************************************************************************
//  MACHINE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( vixen )
//-------------------------------------------------

static MACHINE_CONFIG_START( vixen, vixen_state )
    // basic machine hardware
    MCFG_CPU_ADD(Z8400A_TAG, Z80, XTAL_23_9616MHz/6)
    MCFG_CPU_PROGRAM_MAP(vixen_mem)
    MCFG_CPU_IO_MAP(vixen_io)

    // video hardware
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_RAW_PARAMS(XTAL_23_9616MHz/2, 96*8, 0*8, 81*8, 27*10, 0*10, 26*10)
	MCFG_TIMER_ADD_SCANLINE("vsync", vsync_tick, SCREEN_TAG, 26*10, 27*10)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(monochrome_amber)

	// sound hardware
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(DISCRETE_TAG, DISCRETE, 0)
	MCFG_SOUND_CONFIG_DISCRETE(vixen)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)

	// devices
	MCFG_I8155_ADD(P8155H_TAG, XTAL_23_9616MHz/6, i8155_intf)
	MCFG_I8155_ADD(P8155H_IO_TAG, XTAL_23_9616MHz/6, io_i8155_intf)
	MCFG_MSM8251_ADD(P8251A_TAG, usart_intf)
	MCFG_WD179X_ADD(FDC1797_TAG, fdc_intf)
	MCFG_FLOPPY_2_DRIVES_ADD(vixen_floppy_config)
	MCFG_IEEE488_ADD(IEEE488_TAG, ieee488_daisy)

	/* software lists */
	MCFG_SOFTWARE_LIST_ADD("disk_list","vixen")

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("64K")
MACHINE_CONFIG_END



//**************************************************************************
//  ROMS
//**************************************************************************

//-------------------------------------------------
//  ROM( vixen )
//-------------------------------------------------

ROM_START( vixen )
    ROM_REGION( 0x1000, Z8400A_TAG, 0 )
	ROM_LOAD( "osborne 4 mon rom v1.04 3p40082-03 a0a9.4c", 0x0000, 0x1000, CRC(5f1038ce) SHA1(e6809fac23650bbb4689e58edc768d917d80a2df) ) // OSBORNE 4 MON ROM / V1.04  3P40082-03 / A0A9 (c) OCC 1985

    ROM_REGION( 0x1000, "video", 0 )
	ROM_LOAD( "v1.10.3j", 0x0000, 0x1000, CRC(1f93e2d7) SHA1(0c479bfd3ac8d9959c285c020d0096930a9c6867) )

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "v1.00 l.1j", 0x0000, 0x1000, CRC(f97c50d9) SHA1(39f73afad68508c4b8a4d241c064f9978098d8f2) )
ROM_END



//**************************************************************************
//  DRIVER INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  DRIVER_INIT( vixen )
//-------------------------------------------------

DIRECT_UPDATE_HANDLER( vixen_direct_update_handler )
{
	vixen_state *state = machine->driver_data<vixen_state>();

	if (address >= 0xf000)
	{
		if (state->m_reset)
		{
			address_space *program = state->m_maincpu->memory().space(AS_PROGRAM);

			program->install_read_bank(0x0000, 0xefff, "bank1");
			program->install_write_bank(0x0000, 0xefff, "bank2");

			memory_set_bank(*machine, "bank1", 0);
			memory_set_bank(*machine, "bank2", 0);

			state->m_reset = 0;
		}

		direct.explicit_configure(0xf000, 0xffff, 0xfff, machine->region(Z8400A_TAG)->base());

		return ~0;
	}

	return address;
}

static DRIVER_INIT( vixen )
{
	address_space *program = machine.device<cpu_device>(Z8400A_TAG)->space(AS_PROGRAM);
	program->set_direct_update_handler(direct_update_delegate_create_static(vixen_direct_update_handler, machine));
}



//**************************************************************************
//  SYSTEM DRIVERS
//**************************************************************************

//    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS
COMP( 1984, vixen,  0,       0, 	vixen,	vixen,	 vixen,  "Osborne",   "Vixen",		GAME_NOT_WORKING )
