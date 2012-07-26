/*

    Wang Professional Computer

    http://www.seasip.info/VintagePC/wangpc.html

    chdman -createblankhd q540.chd 512 8 17 512

*/

#include "includes/wangpc.h"



//**************************************************************************
//  MACROS/CONSTANTS
//**************************************************************************

enum
{
	LED_DIAGNOSTIC = 0
};



//**************************************************************************
//  DIRECT MEMORY ACCESS
//**************************************************************************

WRITE8_MEMBER( wangpc_state::fdc_ctrl_w )
{
	/*

        bit     description

        0       Enable /EOP
        1       Disable /DREQ2
        2       Clear drive 1 door disturbed interrupt
        3       Clear drive 2 door disturbed interrupt
        4
        5
        6
        7

    */

	m_fdc_tc_enable = BIT(data, 0);
	m_fdc_dma_enable = BIT(data, 1);

	update_fdc_tc();
	update_fdc_drq();
}


READ8_MEMBER( wangpc_state::deselect_drive1_r )
{
	m_ds1 = 1;

	return 0;
}


WRITE8_MEMBER( wangpc_state::deselect_drive1_w )
{
	m_ds1 = 1;
}


READ8_MEMBER( wangpc_state::select_drive1_r )
{
	m_ds1 = 0;

	return 0;
}


WRITE8_MEMBER( wangpc_state::select_drive1_w )
{
	m_ds1 = 0;
}


READ8_MEMBER( wangpc_state::deselect_drive2_r )
{
	m_ds2 = 1;

	return 0;
}


WRITE8_MEMBER( wangpc_state::deselect_drive2_w )
{
	m_ds2 = 1;
}


READ8_MEMBER( wangpc_state::select_drive2_r )
{
	m_ds2 = 0;

	return 0;
}


WRITE8_MEMBER( wangpc_state::select_drive2_w )
{
	m_ds2 = 0;
}


READ8_MEMBER( wangpc_state::motor1_off_r )
{
	floppy_mon_w(m_floppy0, 1);

	return 0;
}


WRITE8_MEMBER( wangpc_state::motor1_off_w )
{
	floppy_mon_w(m_floppy0, 1);
}


READ8_MEMBER( wangpc_state::motor1_on_r )
{
	floppy_mon_w(m_floppy0, 0);

	return 0;
}


WRITE8_MEMBER( wangpc_state::motor1_on_w )
{
	floppy_mon_w(m_floppy0, 0);
}


READ8_MEMBER( wangpc_state::motor2_off_r )
{
	floppy_mon_w(m_floppy1, 1);

	return 0;
}


WRITE8_MEMBER( wangpc_state::motor2_off_w )
{
	floppy_mon_w(m_floppy1, 1);
}


READ8_MEMBER( wangpc_state::motor2_on_r )
{
	floppy_mon_w(m_floppy1, 0);

	return 0;
}


WRITE8_MEMBER( wangpc_state::motor2_on_w )
{
	floppy_mon_w(m_floppy1, 0);
}


READ8_MEMBER( wangpc_state::fdc_reset_r )
{
	upd765_reset_w(m_fdc, 1);
	upd765_reset_w(m_fdc, 0);

	return 0;
}


WRITE8_MEMBER( wangpc_state::fdc_reset_w )
{
	upd765_reset_w(m_fdc, 1);
	upd765_reset_w(m_fdc, 0);
}


READ8_MEMBER( wangpc_state::fdc_tc_r )
{
	upd765_tc_w(m_fdc, 1);
	upd765_tc_w(m_fdc, 0);

	return 0;
}


WRITE8_MEMBER( wangpc_state::fdc_tc_w )
{
	upd765_tc_w(m_fdc, 1);
	upd765_tc_w(m_fdc, 0);
}



//-------------------------------------------------
//  dma_page_w -
//-------------------------------------------------

WRITE8_MEMBER( wangpc_state::dma_page_w )
{
	m_dma_page[offset] = data & 0x0f;
}


//-------------------------------------------------
//  status_r -
//-------------------------------------------------

READ8_MEMBER( wangpc_state::status_r )
{
	/*

        bit     description

        0       Memory Parity Flag
        1       I/O Error Flag
        2       Unassigned
        3       FDC Interrupt Flag
        4       Door disturbed on drive 1
        5       Door disturbed on drive 2
        6       Door open on drive 1
        7       Door open on drive 2

    */

	UINT8 data = 0x03;

	// FDC interrupt
	data |= upd765_int_r(m_fdc) << 3;

	return data;
}


//-------------------------------------------------
//  timer0_int_clr_w -
//-------------------------------------------------

WRITE8_MEMBER( wangpc_state::timer0_irq_clr_w )
{
	pic8259_ir0_w(m_pic, CLEAR_LINE);
}


//-------------------------------------------------
//  timer2_irq_clr_r -
//-------------------------------------------------

READ8_MEMBER( wangpc_state::timer2_irq_clr_r )
{
	m_timer2_irq = 1;
	check_level1_interrupts();

	return 0;
}


//-------------------------------------------------
//  nmi_mask_w -
//-------------------------------------------------

WRITE8_MEMBER( wangpc_state::nmi_mask_w )
{
}


//-------------------------------------------------
//  led_on_r -
//-------------------------------------------------

READ8_MEMBER( wangpc_state::led_on_r )
{
	output_set_led_value(LED_DIAGNOSTIC, 1);

	return 0;
}


//-------------------------------------------------
//  fpu_mask_w -
//-------------------------------------------------

WRITE8_MEMBER( wangpc_state::fpu_mask_w )
{
}


//-------------------------------------------------
//  uart_tbre_clr_w -
//-------------------------------------------------

WRITE8_MEMBER( wangpc_state::uart_tbre_clr_w  )
{
	m_uart_tbre = 0;
	check_level2_interrupts();
}


//-------------------------------------------------
//  uart_r -
//-------------------------------------------------

READ8_MEMBER( wangpc_state::uart_r )
{
	m_uart_dr = 0;
	check_level2_interrupts();

	return m_uart->read(space, 0);
}


//-------------------------------------------------
//  uart_w -
//-------------------------------------------------

WRITE8_MEMBER( wangpc_state::uart_w  )
{
	m_uart_tbre = 0;
	check_level2_interrupts();

	m_uart->write(space, 0, data);
}


//-------------------------------------------------
//  centronics_r -
//-------------------------------------------------

READ8_MEMBER( wangpc_state::centronics_r )
{
	m_dav = 1;
	check_level1_interrupts();

	return m_centronics->read(space, 0);
}


//-------------------------------------------------
//  centronics_w -
//-------------------------------------------------

WRITE8_MEMBER( wangpc_state::centronics_w )
{
	m_acknlg = 1;
	check_level1_interrupts();

	m_centronics->write(space, 0, data);

	m_centronics->strobe_w(0);
	m_centronics->strobe_w(1);
}


//-------------------------------------------------
//  busy_clr_r -
//-------------------------------------------------

READ8_MEMBER( wangpc_state::busy_clr_r )
{
	m_busy = 1;
	check_level1_interrupts();

	return 0;
}


//-------------------------------------------------
//  acknlg_clr_w -
//-------------------------------------------------

WRITE8_MEMBER( wangpc_state::acknlg_clr_w )
{
	m_acknlg = 1;
	check_level1_interrupts();
}


//-------------------------------------------------
//  led_off_r -
//-------------------------------------------------

READ8_MEMBER( wangpc_state::led_off_r )
{
	output_set_led_value(LED_DIAGNOSTIC, 0);

	return 0;
}


//-------------------------------------------------
//  parity_nmi_clr_w -
//-------------------------------------------------

WRITE8_MEMBER( wangpc_state::parity_nmi_clr_w )
{
}


//-------------------------------------------------
//  option_id_r -
//-------------------------------------------------

READ8_MEMBER( wangpc_state::option_id_r )
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
        7       FDC Interrupt Flag

    */

	UINT8 data = 0;

	// FDC interrupt
	data |= upd765_int_r(m_fdc) << 7;

	return data;
}



//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( wangpc_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( wangpc_mem, AS_PROGRAM, 16, wangpc_state )
//  AM_RANGE(0x00000, 0xfffff) AM_READWRITE(mrdc_r, amwc_c)
	AM_RANGE(0x00000, 0x1ffff) AM_RAM
	AM_RANGE(0xfc000, 0xfffff) AM_ROM AM_REGION(I8086_TAG, 0)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( wangpc_io )
//-------------------------------------------------

static ADDRESS_MAP_START( wangpc_io, AS_IO, 16, wangpc_state )
	AM_RANGE(0x1000, 0x1001) AM_WRITE8(fdc_ctrl_w, 0x00ff)
	AM_RANGE(0x1004, 0x1005) AM_READWRITE8(deselect_drive1_r, deselect_drive1_w, 0x00ff)
	AM_RANGE(0x1006, 0x1007) AM_READWRITE8(select_drive1_r, select_drive1_w, 0x00ff)
	AM_RANGE(0x1008, 0x1009) AM_READWRITE8(deselect_drive2_r, deselect_drive2_w, 0x00ff)
	AM_RANGE(0x100a, 0x100b) AM_READWRITE8(select_drive2_r, select_drive2_w, 0x00ff)
	AM_RANGE(0x100c, 0x100d) AM_READWRITE8(motor1_off_r, motor1_off_w, 0x00ff)
	AM_RANGE(0x100e, 0x100f) AM_READWRITE8(motor1_on_r, motor1_on_w, 0x00ff)
	AM_RANGE(0x1010, 0x1011) AM_READWRITE8(motor1_off_r, motor1_off_w, 0x00ff)
	AM_RANGE(0x1012, 0x1013) AM_READWRITE8(motor1_on_r, motor1_on_w, 0x00ff)
	AM_RANGE(0x1014, 0x1015) AM_DEVREAD8_LEGACY(UPD765_TAG, upd765_status_r, 0x00ff)
	AM_RANGE(0x1016, 0x1017) AM_DEVREADWRITE8_LEGACY(UPD765_TAG, upd765_data_r, upd765_data_w, 0x00ff)
	AM_RANGE(0x1018, 0x1019) AM_MIRROR(0x0002) AM_READWRITE8(fdc_reset_r, fdc_reset_w, 0x00ff)
	AM_RANGE(0x101c, 0x101d) AM_MIRROR(0x0002) AM_READWRITE8(fdc_tc_r, fdc_tc_w, 0x00ff)
	AM_RANGE(0x1020, 0x1027) AM_DEVREADWRITE8(I8255A_TAG, i8255_device, read, write, 0x00ff)
	AM_RANGE(0x1040, 0x1047) AM_DEVREADWRITE8_LEGACY(I8253_TAG, pit8253_r, pit8253_w, 0x00ff)
	AM_RANGE(0x1060, 0x1063) AM_DEVREADWRITE8_LEGACY(I8259A_TAG, pic8259_r, pic8259_w, 0x00ff)
//  AM_RANGE(0x1080, 0x1087) AM_DEVREAD8(SCN2661_TAG, scn2661_device, read, 0x00ff)
//  AM_RANGE(0x1088, 0x108f) AM_DEVWRITE8(SCN2661_TAG, scn2661_device, write, 0x00ff)
	AM_RANGE(0x10a0, 0x10bf) AM_DEVREADWRITE8_LEGACY(AM9517A_TAG, i8237_r, i8237_w, 0x00ff)
	AM_RANGE(0x10c2, 0x10c7) AM_WRITE8(dma_page_w, 0x00ff)
	AM_RANGE(0x10e0, 0x10e1) AM_READWRITE8(status_r, timer0_irq_clr_w, 0x00ff)
	AM_RANGE(0x10e2, 0x10e3) AM_READWRITE8(timer2_irq_clr_r, nmi_mask_w, 0x00ff)
	AM_RANGE(0x10e4, 0x10e5) AM_READWRITE8(led_on_r, fpu_mask_w, 0x00ff)
	AM_RANGE(0x10e6, 0x10e7) AM_WRITE8(uart_tbre_clr_w, 0x00ff)
	AM_RANGE(0x10e8, 0x10e9) AM_READWRITE8(uart_r, uart_w, 0x00ff)
	AM_RANGE(0x10ea, 0x10eb) AM_READWRITE8(centronics_r, centronics_w, 0x00ff)
	AM_RANGE(0x10ec, 0x10ed) AM_READWRITE8(busy_clr_r, acknlg_clr_w, 0x00ff)
	AM_RANGE(0x10ee, 0x10ef) AM_READWRITE8(led_off_r, parity_nmi_clr_w, 0x00ff)
	AM_RANGE(0x10fe, 0x10ff) AM_READ8(option_id_r, 0x00ff)
	AM_RANGE(0x1100, 0x1fff) AM_DEVREADWRITE(WANGPC_BUS_TAG, wangpcbus_device, sad_r, sad_w)
ADDRESS_MAP_END



//**************************************************************************
//  INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_PORTS( wangpc )
//-------------------------------------------------

static INPUT_PORTS_START( wangpc )
	// keyboard defined in machine/wangpckb.c

	PORT_START("SW")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  I8237_INTERFACE( dmac_intf )
//-------------------------------------------------

void wangpc_state::update_fdc_tc()
{
	if (m_fdc_tc_enable)
		upd765_tc_w(m_fdc, m_dma_eop);
	else
		upd765_tc_w(m_fdc, 1);
}

WRITE_LINE_MEMBER( wangpc_state::hrq_w )
{
	m_maincpu->set_input_line(INPUT_LINE_HALT, state ? ASSERT_LINE : CLEAR_LINE);

	i8237_hlda_w(m_dmac, state);
}

WRITE_LINE_MEMBER( wangpc_state::eop_w )
{
	m_dma_eop = !state;

	update_fdc_tc();
	check_level2_interrupts();
}

READ8_MEMBER( wangpc_state::memr_r )
{
	address_space *program = m_maincpu->memory().space(AS_PROGRAM);
	offs_t page_offset = m_dma_page[m_dma_channel] << 16;

	return program->read_byte(page_offset + offset);
}

WRITE8_MEMBER( wangpc_state::memw_w )
{
	address_space *program = m_maincpu->memory().space(AS_PROGRAM);
	offs_t page_offset = m_dma_page[m_dma_channel] << 16;

	program->write_byte(page_offset + offset, data);
}

READ8_MEMBER( wangpc_state::ior1_r )
{
	return m_bus->dack_r(1);
}

READ8_MEMBER( wangpc_state::ior2_r )
{
	if (m_fdc_dma_enable)
		return m_bus->dack_r(2);
	else
		return upd765_dack_r(m_fdc, 0);
}

READ8_MEMBER( wangpc_state::ior3_r )
{
	return m_bus->dack_r(3);
}

WRITE8_MEMBER( wangpc_state::iow1_w )
{
	m_bus->dack_w(1, data);
}

WRITE8_MEMBER( wangpc_state::iow2_w )
{
	if (m_fdc_dma_enable)
		m_bus->dack_w(2, data);
	else
		upd765_dack_w(m_fdc, 0, data);
}

WRITE8_MEMBER( wangpc_state::iow3_w )
{
	m_bus->dack_w(3, data);
}

WRITE_LINE_MEMBER( wangpc_state::dack0_w )
{
	if (!state) m_dma_channel = 0;
}

WRITE_LINE_MEMBER( wangpc_state::dack1_w )
{
	if (!state) m_dma_channel = 1;
}

WRITE_LINE_MEMBER( wangpc_state::dack2_w )
{
	if (!state) m_dma_channel = 2;
}

WRITE_LINE_MEMBER( wangpc_state::dack3_w )
{
	if (!state) m_dma_channel = 3;
}

static I8237_INTERFACE( dmac_intf )
{
	DEVCB_DRIVER_LINE_MEMBER(wangpc_state, hrq_w),
	DEVCB_DRIVER_LINE_MEMBER(wangpc_state, eop_w),
	DEVCB_DRIVER_MEMBER(wangpc_state, memr_r),
	DEVCB_DRIVER_MEMBER(wangpc_state, memw_w),
	{ DEVCB_NULL,
	  DEVCB_DRIVER_MEMBER(wangpc_state, ior1_r),
	  DEVCB_DRIVER_MEMBER(wangpc_state, ior2_r),
	  DEVCB_DRIVER_MEMBER(wangpc_state, ior3_r) },
	{ DEVCB_NULL,
	  DEVCB_DRIVER_MEMBER(wangpc_state, iow1_w),
	  DEVCB_DRIVER_MEMBER(wangpc_state, iow2_w),
	  DEVCB_DRIVER_MEMBER(wangpc_state, iow3_w) },
	{ DEVCB_DRIVER_LINE_MEMBER(wangpc_state, dack0_w),
	  DEVCB_DRIVER_LINE_MEMBER(wangpc_state, dack1_w),
	  DEVCB_DRIVER_LINE_MEMBER(wangpc_state, dack2_w),
	  DEVCB_DRIVER_LINE_MEMBER(wangpc_state, dack3_w) }
};


//-------------------------------------------------
//  pic8259_interface pic_intf
//-------------------------------------------------

void wangpc_state::check_level1_interrupts()
{
	int state = !m_timer2_irq | !m_ecpi_irq | !m_acknlg | !m_dav | !m_busy;

	pic8259_ir1_w(m_pic, state);
}

void wangpc_state::check_level2_interrupts()
{
	int state = !m_dma_eop | m_uart_dr | m_uart_tbre | upd765_int_r(m_fdc) | m_fpu_irq;

	pic8259_ir2_w(m_pic, state);
}

static IRQ_CALLBACK( wangpc_irq_callback )
{
	wangpc_state *state = device->machine().driver_data<wangpc_state>();

	return pic8259_acknowledge(state->m_pic);
}

static const struct pic8259_interface pic_intf =
{
	DEVCB_CPU_INPUT_LINE(I8086_TAG, INPUT_LINE_IRQ0),
	DEVCB_LINE_VCC,
	DEVCB_NULL
};


//-------------------------------------------------
//  I8255A_INTERFACE( ppi_intf )
//-------------------------------------------------

READ8_MEMBER( wangpc_state::ppi_pa_r )
{
	/*

        bit     description

        0       /POWER ON
        1       /SMART
        2       /DATA AVAILABLE
        3       SLCT
        4       BUSY
        5       /FAULT
        6       PE
        7       ACKNOWLEDGE

    */

	UINT8 data = 0x08 | 0x02 | 0x01;

	data |= m_dav << 2;
	data |= m_centronics->busy_r() << 4;
	data |= m_centronics->fault_r() << 5;
	data |= m_centronics->pe_r() << 6;
	data |= m_acknlg << 7;

	return data;
}

READ8_MEMBER( wangpc_state::ppi_pb_r )
{
	/*

        bit     description

        0       /TIMER 2 INTERRUPT
        1       /SERIAL INTERRUPT
        2       /PARALLEL PORT INTERRUPT
        3       /DMA INTERRUPT
        4       KBD INTERRUPT TRANSMIT
        5       KBD INTERRUPT RECEIVE
        6       FLOPPY DISK INTERRUPT
        7       8087 INTERRUPT

    */

	UINT8 data = 0;

	// timer 2 interrupt
	data |= m_timer2_irq;

	// serial interrupt
	data |= m_ecpi_irq << 1;

	// parallel port interrupt
	data |= m_acknlg << 2;

	// DMA interrupt
	data |= m_dma_eop << 3;

	// keyboard interrupt
	data |= m_uart_tbre << 4;
	data |= m_uart_dr << 5;

	// FDC interrupt
	data |= upd765_int_r(m_fdc) << 6;

	// 8087 interrupt
	data |= m_fpu_irq << 7;

	return data;
}

READ8_MEMBER( wangpc_state::ppi_pc_r )
{
	/*

        bit     description

        0
        1
        2
        3
        4       SW1
        5       SW2
        6       SW3
        7       SW4

    */

	return ioport("SW")->read() << 4;
}

WRITE8_MEMBER( wangpc_state::ppi_pc_w )
{
	/*

        bit     description

        0       /USR0 (pin 14)
        1       /USR1 (pin 36)
        2       /RESET (pin 31)
        3       Unassigned
        4
        5
        6
        7

    */

	m_centronics->autofeed_w(BIT(data, 0));
	m_centronics->init_prime_w(BIT(data, 2));
}

static I8255A_INTERFACE( ppi_intf )
{
	DEVCB_DRIVER_MEMBER(wangpc_state, ppi_pa_r),
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(wangpc_state, ppi_pb_r),
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(wangpc_state, ppi_pc_r),
	DEVCB_DRIVER_MEMBER(wangpc_state, ppi_pc_w)
};


//-------------------------------------------------
//  pit8253_config pit_intf
//-------------------------------------------------

WRITE_LINE_MEMBER( wangpc_state::pit2_w )
{
	if (state)
	{
		m_timer2_irq = 0;
		check_level1_interrupts();
	}
}

static const struct pit8253_config pit_intf =
{
	{
		{
			500000,
			DEVCB_LINE_VCC,
			DEVCB_DEVICE_LINE(I8259A_TAG, pic8259_ir0_w)
		}, {
			2000000,
			DEVCB_LINE_VCC,
			DEVCB_NULL
		}, {
			500000,
			DEVCB_LINE_VCC,
			DEVCB_DRIVER_LINE_MEMBER(wangpc_state, pit2_w)
		}
	}
};


//-------------------------------------------------
//  IM6402_INTERFACE( uart_intf )
//-------------------------------------------------

WRITE_LINE_MEMBER( wangpc_state::uart_dr_w )
{
	if (state)
	{
		m_uart_dr = 1;
		check_level2_interrupts();
	}
}

WRITE_LINE_MEMBER( wangpc_state::uart_tbre_w )
{
	if (state)
	{
		m_uart_tbre = 1;
		check_level2_interrupts();
	}
}

static IM6402_INTERFACE( uart_intf )
{
	62500,
	62500,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(wangpc_state, uart_dr_w),
	DEVCB_DRIVER_LINE_MEMBER(wangpc_state, uart_tbre_w),
	DEVCB_NULL
};


//-------------------------------------------------
//  upd765_interface fdc_intf
//-------------------------------------------------

static const floppy_interface floppy_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSDD,
	LEGACY_FLOPPY_OPTIONS_NAME(default),
	"floppy_5_25",
	NULL
};

WRITE_LINE_MEMBER( wangpc_state::fdc_int_w )
{
	check_level2_interrupts();
}

WRITE_LINE_MEMBER( wangpc_state::fdc_drq_w )
{
	m_fdc_drq = state;
	update_fdc_drq();
}

void wangpc_state::update_fdc_drq()
{
	if (m_fdc_dma_enable)
		i8237_dreq2_w(m_dmac, m_fdc_drq);
	else
		i8237_dreq2_w(m_dmac, CLEAR_LINE);
}

static UPD765_GET_IMAGE( wangpc_fdc_get_image )
{
	wangpc_state *state = device->machine().driver_data<wangpc_state>();

	if (!state->m_ds1) return state->m_floppy0;
	if (!state->m_ds2) return state->m_floppy1;

	return NULL;
}

static const upd765_interface fdc_intf =
{
	DEVCB_DRIVER_LINE_MEMBER(wangpc_state, fdc_int_w),
	DEVCB_DRIVER_LINE_MEMBER(wangpc_state, fdc_drq_w),
	wangpc_fdc_get_image,
	UPD765_RDY_PIN_NOT_CONNECTED,
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};


//-------------------------------------------------
//  centronics_interface centronics_intf
//-------------------------------------------------

WRITE_LINE_MEMBER( wangpc_state::ack_w )
{
	m_acknlg = state;
	check_level1_interrupts();
}

WRITE_LINE_MEMBER( wangpc_state::busy_w )
{
	m_busy = state;
	check_level1_interrupts();
}

static const centronics_interface centronics_intf =
{
	DEVCB_DRIVER_LINE_MEMBER(wangpc_state, ack_w),
	DEVCB_DRIVER_LINE_MEMBER(wangpc_state, busy_w),
	DEVCB_NULL
};


//-------------------------------------------------
//  WANGPC_KEYBOARD_INTERFACE( kb_intf )
//-------------------------------------------------

static WANGPC_KEYBOARD_INTERFACE( kb_intf )
{
	DEVCB_NULL
};


//-------------------------------------------------
//  WANGPC_BUS_INTERFACE( kb_intf )
//-------------------------------------------------

WRITE_LINE_MEMBER( wangpc_state::bus_irq2_w )
{
	check_level2_interrupts();
}

static WANGPC_BUS_INTERFACE( bus_intf )
{
	DEVCB_DRIVER_LINE_MEMBER(wangpc_state, bus_irq2_w),
	DEVCB_DEVICE_LINE(I8259A_TAG, pic8259_ir3_w),
	DEVCB_DEVICE_LINE(I8259A_TAG, pic8259_ir4_w),
	DEVCB_DEVICE_LINE(I8259A_TAG, pic8259_ir5_w),
	DEVCB_DEVICE_LINE(I8259A_TAG, pic8259_ir6_w),
	DEVCB_DEVICE_LINE(I8259A_TAG, pic8259_ir7_w),
	DEVCB_DEVICE_LINE(AM9517A_TAG, i8237_dreq1_w),
	DEVCB_DEVICE_LINE(AM9517A_TAG, i8237_dreq2_w),
	DEVCB_DEVICE_LINE(AM9517A_TAG, i8237_dreq3_w),
	DEVCB_CPU_INPUT_LINE(I8086_TAG, INPUT_LINE_NMI)
};

static SLOT_INTERFACE_START( wangpc_cards )
	SLOT_INTERFACE("lores", WANGPC_LORES_VIDEO)
SLOT_INTERFACE_END



//**************************************************************************
//  MACHINE INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_START( wangpc )
//-------------------------------------------------

void wangpc_state::machine_start()
{
	// register CPU IRQ callback
	device_set_irq_callback(m_maincpu, wangpc_irq_callback);

	// connect serial keyboard
	m_uart->connect(m_kb);
}


//-------------------------------------------------
//  MACHINE_RESET( wangpc )
//-------------------------------------------------

void wangpc_state::machine_reset()
{
	// initialize UART
	m_uart->cls1_w(1);
	m_uart->cls2_w(1);
	m_uart->pi_w(1);
	m_uart->sbs_w(1);
	m_uart->crl_w(1);
}



//**************************************************************************
//  MACHINE DRIVERS
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( wangpc )
//-------------------------------------------------

static MACHINE_CONFIG_START( wangpc, wangpc_state )
	MCFG_CPU_ADD(I8086_TAG, I8086, 8000000)
	MCFG_CPU_PROGRAM_MAP(wangpc_mem)
	MCFG_CPU_IO_MAP(wangpc_io)

	// devices
	MCFG_I8237_ADD(AM9517A_TAG, 4000000, dmac_intf)
	MCFG_PIC8259_ADD(I8259A_TAG, pic_intf)
	MCFG_I8255A_ADD(I8255A_TAG, ppi_intf)
	MCFG_PIT8253_ADD(I8253_TAG, pit_intf)
	MCFG_IM6402_ADD(IM6402_TAG, uart_intf)
	// SCN2661 for RS-232
	MCFG_UPD765A_ADD(UPD765_TAG, fdc_intf)
	MCFG_LEGACY_FLOPPY_2_DRIVES_ADD(floppy_intf)
	MCFG_CENTRONICS_PRINTER_ADD(CENTRONICS_TAG, centronics_intf)
	MCFG_WANGPC_KEYBOARD_ADD(kb_intf)

	// bus
	MCFG_WANGPC_BUS_ADD(bus_intf)
	MCFG_WANGPC_BUS_SLOT_ADD("slot1", 1, wangpc_cards, NULL, NULL)
	MCFG_WANGPC_BUS_SLOT_ADD("slot2", 2, wangpc_cards, "lores", NULL)
	MCFG_WANGPC_BUS_SLOT_ADD("slot3", 3, wangpc_cards, NULL, NULL)
	MCFG_WANGPC_BUS_SLOT_ADD("slot4", 4, wangpc_cards, NULL, NULL)
	MCFG_WANGPC_BUS_SLOT_ADD("slot5", 5, wangpc_cards, NULL, NULL)

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("128K")
MACHINE_CONFIG_END



//**************************************************************************
//  ROM DEFINITIONS
//**************************************************************************

//-------------------------------------------------
//  ROM( wangpc )
//-------------------------------------------------

ROM_START( wangpc )
	ROM_REGION16_LE( 0x4000, I8086_TAG, 0)
	ROM_LOAD16_BYTE( "motherboard-l94.bin", 0x0001, 0x2000, CRC(f9f41304) SHA1(1815295809ef11573d724ede47446f9ac7aee713) )
	ROM_LOAD16_BYTE( "motherboard-l115.bin", 0x0000, 0x2000, CRC(67b37684) SHA1(70d9f68eb88cc2bc9f53f949cc77411c09a4266e) )

	ROM_REGION( 0x1000, "remotecomms", 0 )
	ROM_LOAD( "remotecomms-l28.bin", 0x0000, 0x1000, CRC(c05a1bee) SHA1(6b3f0d787d014b1fd3925812c905ddb63c5055f1) )

	ROM_REGION( 0x1000, "network", 0 )
	ROM_LOAD( "network-l22.bin", 0x0000, 0x1000, CRC(487e5f04) SHA1(81e52e70e0c6e34715119b121ec19a7758cd6772) )

	ROM_REGION( 0x1000, "hdc1", 0 )
	ROM_LOAD( "hdc-l54.bin", 0x0000, 0x1000, CRC(94e9a17d) SHA1(060c576d70069ece2d0dbce86ffc448df2b169e7) )

	ROM_REGION( 0x1000, "hdc2", 0 )
	ROM_LOAD( "hdc-l19.bin", 0x0000, 0x1000, CRC(282770d2) SHA1(a0e3bad5041e0dfd6087907015b07a093b576bc0) )
ROM_END



//**************************************************************************
//  GAME DRIVERS
//**************************************************************************

COMP( 1985, wangpc, 0, 0, wangpc, wangpc, 0, "Wang Laboratories", "Wang Professional Computer", GAME_NOT_WORKING | GAME_NO_SOUND )
