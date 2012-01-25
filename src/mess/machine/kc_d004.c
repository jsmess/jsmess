/***************************************************************************

    kc_d004.c

    KC85 D004 Floppy Disk Interface

***************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "kc_d004.h"

#define Z80_TAG			"disk"
#define Z80CTC_TAG		"z80ctc"
#define UPD765_TAG		"upd765"

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

static ADDRESS_MAP_START(kc_d004_mem, AS_PROGRAM, 8, kc_d004_device)
	AM_RANGE(0x0000, 0xffff) AM_RAM		// 64kb RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START(kc_d004_io, AS_IO, 8, kc_d004_device)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xf0, 0xf0) AM_DEVREAD_LEGACY(UPD765_TAG, upd765_status_r)
	AM_RANGE(0xf1, 0xf1) AM_DEVREADWRITE_LEGACY(UPD765_TAG, upd765_data_r, upd765_data_w)
	AM_RANGE(0xf2, 0xf3) AM_DEVREADWRITE_LEGACY(UPD765_TAG, upd765_dack_r, upd765_dack_w)
	AM_RANGE(0xf4, 0xf4) AM_READ(hw_input_gate_r)
	AM_RANGE(0xf6, 0xf7) AM_WRITE(fdd_select_w)
	AM_RANGE(0xf8, 0xf9) AM_WRITE(hw_terminal_count_w)
	AM_RANGE(0xfc, 0xff) AM_DEVREADWRITE_LEGACY(Z80CTC_TAG, z80ctc_r, z80ctc_w)
ADDRESS_MAP_END

static LEGACY_FLOPPY_OPTIONS_START(kc_d004)
	LEGACY_FLOPPY_OPTION(kc85, "img", "KC85 disk image 800KB", basicdsk_identify_default, basicdsk_construct_default, NULL,
		HEADS([2])
		TRACKS([80])
		SECTORS([5])
		SECTOR_LENGTH([1024])
		FIRST_SECTOR_ID([1]))
	LEGACY_FLOPPY_OPTION(kc85, "img", "KC85 disk image 720KB", basicdsk_identify_default, basicdsk_construct_default, NULL,
		HEADS([2])
		TRACKS([80])
		SECTORS([9])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
	LEGACY_FLOPPY_OPTION(kc85, "img", "KC85 disk image 640KB", basicdsk_identify_default, basicdsk_construct_default, NULL,
		HEADS([2])
		TRACKS([80])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
LEGACY_FLOPPY_OPTIONS_END

static const floppy_interface kc_d004_floppy_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	LEGACY_FLOPPY_OPTIONS_NAME(kc_d004),
	"floppy_5_25",
	NULL
};

static device_t *kc_d004_get_fdd(device_t *device, int floppy_index)
{
	kc_d004_device* owner = dynamic_cast<kc_d004_device *>(device->owner());
	return owner->get_active_fdd();
}

static const upd765_interface kc_d004_interface =
{
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF, kc_d004_device, fdc_interrupt),
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF, kc_d004_device, fdc_dma_request),
	kc_d004_get_fdd,
	UPD765_RDY_PIN_NOT_CONNECTED,
	{FLOPPY_0, FLOPPY_1, FLOPPY_2, FLOPPY_3}
};

static const z80_daisy_config kc_d004_daisy_chain[] =
{
	{ Z80CTC_TAG },
	{ NULL }
};

static Z80CTC_INTERFACE( kc_d004_ctc_intf )
{
	0,												/* timer disablers */
	DEVCB_CPU_INPUT_LINE(Z80_TAG, 0),				/* interrupt callback */
	DEVCB_DEVICE_LINE(Z80CTC_TAG, z80ctc_trg1_w),	/* ZC/TO0 callback */
	DEVCB_DEVICE_LINE(Z80CTC_TAG, z80ctc_trg2_w),	/* ZC/TO1 callback */
	DEVCB_DEVICE_LINE(Z80CTC_TAG, z80ctc_trg3_w)	/* ZC/TO2 callback */
};

static MACHINE_CONFIG_FRAGMENT(kc_d004)
	MCFG_CPU_ADD(Z80_TAG, Z80, XTAL_8MHz/2)
	MCFG_CPU_PROGRAM_MAP(kc_d004_mem)
	MCFG_CPU_IO_MAP(kc_d004_io)
	MCFG_CPU_CONFIG(kc_d004_daisy_chain)

	MCFG_Z80CTC_ADD( Z80CTC_TAG, XTAL_8MHz/2, kc_d004_ctc_intf )

	MCFG_UPD765A_ADD(UPD765_TAG, kc_d004_interface)
	MCFG_LEGACY_FLOPPY_4_DRIVES_ADD(kc_d004_floppy_interface)
MACHINE_CONFIG_END

ROM_START( kc_d004 )
	ROM_REGION(0x2000, Z80_TAG, 0)
	ROM_LOAD_OPTIONAL(	"d004v20.bin",	0x0000, 0x2000, CRC(4f3494f1) SHA1(66f476de78fb474d9ac61c6eaffce3354fd66776))
ROM_END

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type KC_D004 = &device_creator<kc_d004_device>;

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  kc_d004_device - constructor
//-------------------------------------------------

kc_d004_device::kc_d004_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
      : device_t(mconfig, KC_D004, "D004 Floppy Disk Interface", tag, owner, clock),
		device_kcexp_interface( mconfig, *this ),
		m_cpu(*this, Z80_TAG),
		m_fdc(*this, UPD765_TAG)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void kc_d004_device::device_start()
{
	m_rom  = subregion(Z80_TAG)->base();

	m_reset_timer = timer_alloc(TIMER_RESET);
	m_tc_clear_timer = timer_alloc(TIMER_TC_CLEAR);
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void kc_d004_device::device_reset()
{
	m_rom_base = 0xc000;
	m_enabled = m_connected = 0;
	m_floppy = downcast<device_t *>(subdevice(FLOPPY_0));

	// hold cpu at reset
	m_reset_timer->adjust(attotime::zero);
}

//-------------------------------------------------
//  device_mconfig_additions
//-------------------------------------------------

machine_config_constructor kc_d004_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( kc_d004 );
}

//-------------------------------------------------
//  device_rom_region
//-------------------------------------------------

const rom_entry *kc_d004_device::device_rom_region() const
{
	return ROM_NAME( kc_d004 );
}

//-------------------------------------------------
//  device_timer - handler timer events
//-------------------------------------------------

void kc_d004_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	switch(id)
	{
		case TIMER_RESET:
			device_set_input_line(m_cpu, INPUT_LINE_RESET, ASSERT_LINE);
			break;
		case TIMER_TC_CLEAR:
			upd765_tc_w(m_fdc, 0x00);
			break;
	}
}

/*-------------------------------------------------
    set module status
-------------------------------------------------*/

void kc_d004_device::control_w(UINT8 data)
{
	m_enabled = BIT(data, 0);
	m_connected = BIT(data, 2);
	m_rom_base = (data & 0x20) ? 0xe000 : 0xc000;
}

/*-------------------------------------------------
    read
-------------------------------------------------*/

void kc_d004_device::read(offs_t offset, UINT8 &data)
{
	if (offset >= m_rom_base && offset < (m_rom_base + 0x2000) && m_enabled)
		data = m_rom[offset & 0x1fff];
}

//-------------------------------------------------
//  IO read
//-------------------------------------------------

void kc_d004_device::io_read(offs_t offset, UINT8 &data)
{
	if ((offset & 0xff) == 0x80)
	{
		UINT8 slot_id = (offset>>8) & 0xff;

		if (slot_id == 0xfc)
			data = module_id_r();
	}
	else
	{
		if (m_connected)
		{
			switch(offset & 0xff)
			{
			case 0xf0:
			case 0xf1:
			case 0xf2:
			case 0xf3:
				data = m_cpu->memory().space(AS_PROGRAM)->read_byte(0xfc00 | ((offset & 0x03)<<8) | ((offset>>8) & 0xff));
				break;
			}
		}
	}
}

//-------------------------------------------------
//  IO write
//-------------------------------------------------

void kc_d004_device::io_write(offs_t offset, UINT8 data)
{
	if ((offset & 0xff) == 0x80)
	{
		UINT8 slot_id = (offset>>8) & 0xff;

		if (slot_id == 0xfc)
			control_w(data);
	}
	else
	{
		if (m_connected)
		{
			switch(offset & 0xff)
			{
			case 0xf0:
			case 0xf1:
			case 0xf2:
			case 0xf3:
				m_cpu->memory().space(AS_PROGRAM)->write_byte(0xfc00 | ((offset & 0x03)<<8) | ((offset>>8) & 0xff), data);
				break;
			case 0xf4:
				if (data & 0x01)
					device_set_input_line(m_cpu, INPUT_LINE_RESET, CLEAR_LINE);

				if (data & 0x02)
				{
					for (int i=0; i<0xfc00; i++)
						m_cpu->memory().space(AS_PROGRAM)->write_byte(i, 0);

					device_set_input_line(m_cpu, INPUT_LINE_RESET, ASSERT_LINE);
				}

				if (data & 0x04)
					device_set_input_line(m_cpu, INPUT_LINE_RESET, HOLD_LINE);

				if (data & 0x08)
					device_set_input_line(m_cpu, INPUT_LINE_NMI, HOLD_LINE);

				//printf("D004 CPU state: %x\n", data & 0x0f);
				break;
			}
		}
	}
}

//**************************************************************************
//  FDC emulation
//**************************************************************************

READ8_MEMBER(kc_d004_device::hw_input_gate_r)
{
	/*

        bit 7: DMA Request (DRQ from FDC)
        bit 6: Interrupt (INT from FDC)
        bit 5: Drive Ready
        bit 4: Index pulse from disc

    */

	if (floppy_ready_r(m_floppy))
		m_hw_input_gate |= 0x20;
	else
		m_hw_input_gate &= ~0x20;

	if (floppy_index_r(m_floppy))
		m_hw_input_gate &= ~0x10;
	else
		m_hw_input_gate |= 0x10;

	return m_hw_input_gate | 0x0f;
}

WRITE8_MEMBER(kc_d004_device::fdd_select_w)
{
	if (data & 0x01)
		m_floppy = downcast<device_t *>(subdevice(FLOPPY_0));
	else if (data & 0x02)
		m_floppy = downcast<device_t *>(subdevice(FLOPPY_1));
	else if (data & 0x04)
		m_floppy = downcast<device_t *>(subdevice(FLOPPY_2));
	else if (data & 0x08)
		m_floppy = downcast<device_t *>(subdevice(FLOPPY_3));
}

WRITE8_MEMBER(kc_d004_device::hw_terminal_count_w)
{
	upd765_tc_w(m_fdc, 1);

	m_tc_clear_timer->adjust(attotime::from_nsec(200));
}

WRITE_LINE_MEMBER( kc_d004_device::fdc_interrupt )
{
	if (state)
		m_hw_input_gate &= ~0x40;
	else
		m_hw_input_gate |= 0x40;
}

WRITE_LINE_MEMBER( kc_d004_device::fdc_dma_request )
{
	if (state)
		m_hw_input_gate &= ~0x80;
	else
		m_hw_input_gate |= 0x80;
}
