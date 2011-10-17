/***************************************************************************

    coco_multi.c

    Code for emulating CoCo's Multi-Pak Interface

     The Multi-Pak interface multiplexes all I/O lines from the Color
     Computer's expansion port to four identical ports. All I/O lines
     are continuously multiplexed except:

     Pin 36 - *SCS
     Pin 32 - *CTS
     Pin  8 - *CART

     These I/O lines are switched in one of two ways. First, is the front
     panel, four position switch. When adjusted the switch will direct the
     MPI to target these three I/O lines to the selected slot.

     Second, the MPI will listen to writes to 0xff7f and respond accordingly:

     bit 0 --\___ Target *SCS to this slot number
     bit 1 --/
     bit 2 ------ Ignore
     bit 3 ------ Ignore
     bit 4 --\___ Target *CTS and *CART to this slot number
     bit 5 --/
     bit 6 ------ Ignore
     bit 7 ------ Ignore

     After writing to 0xff7f, the position of the physical switch has no
     effect until reset.

     Reading is supported on 0xff7f. It will reflect the position of the
     physical switch. Until data is written to 0xff7f, then it will only
     reflect what has been written until a reset.

     A common modification users of the OS-9 operating system made was to
     tie all of the *CART pins together on the MPI motherboard. The *CART
     line is connected to the 6809's IRQ line. This allowed any hardware
     device in any slot to signal an IRQ to the CPU, no matter what the
     switch position was. OS-9 was designed from the very start to poll
     each device attached on every IQR signal.

     Because of sloppy address decoding the original MPI also responds to
     0xff9f. No software is known to take advantage of this. After the
     introduction of the CoCo 3, which uses 0xff9f internally, Tandy provided
     free upgrades to any MPI to fix this problem.


***************************************************************************/

#include "emu.h"
#include "coco_multi.h"
#include "coco_232.h"
#include "coco_orch90.h"
#include "coco_pak.h"
#include "coco_fdc.h"

#define SLOT1_TAG			"slot1"
#define SLOT2_TAG			"slot2"
#define SLOT3_TAG			"slot3"
#define SLOT4_TAG			"slot4"

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

static SLOT_INTERFACE_START(coco_cart_slot1_3)
	SLOT_INTERFACE("rs232", COCO_232)
	SLOT_INTERFACE("orch90", COCO_ORCH90)
	SLOT_INTERFACE("banked_16k", COCO_PAK_BANKED)
	SLOT_INTERFACE("pak", COCO_PAK)
SLOT_INTERFACE_END
static SLOT_INTERFACE_START(coco_cart_slot4)
	SLOT_INTERFACE("fdcv11", COCO_FDC_V11)
	SLOT_INTERFACE("rs232", COCO_232)
	SLOT_INTERFACE("orch90", COCO_ORCH90)
	SLOT_INTERFACE("banked_16k", COCO_PAK_BANKED)
	SLOT_INTERFACE("pak", COCO_PAK)
SLOT_INTERFACE_END

WRITE_LINE_DEVICE_HANDLER(multi_cart_w)
{
	cococart_slot_device *cart = dynamic_cast<cococart_slot_device *>(device->owner()->owner());
	cart->m_cart_callback.writeline(device,state);
}

WRITE_LINE_DEVICE_HANDLER(multi_nmi_w)
{
	cococart_slot_device *cart = dynamic_cast<cococart_slot_device *>(device->owner()->owner());
	cart->m_nmi_callback.writeline(device,state);
}

WRITE_LINE_DEVICE_HANDLER(multi_halt_w)
{
	cococart_slot_device *cart = dynamic_cast<cococart_slot_device *>(device->owner()->owner());
	cart->m_halt_callback.writeline(device,state);
}

static const cococart_interface multi_cococart_interface =
{
	DEVCB_LINE(multi_cart_w),
	DEVCB_LINE(multi_nmi_w),
	DEVCB_LINE(multi_halt_w)
};

static MACHINE_CONFIG_FRAGMENT(coco_multi)
	MCFG_COCO_CARTRIDGE_ADD(SLOT1_TAG, multi_cococart_interface, coco_cart_slot1_3, NULL, NULL)
	MCFG_COCO_CARTRIDGE_ADD(SLOT2_TAG, multi_cococart_interface, coco_cart_slot1_3, NULL, NULL)
	MCFG_COCO_CARTRIDGE_ADD(SLOT3_TAG, multi_cococart_interface, coco_cart_slot1_3, NULL, NULL)
	MCFG_COCO_CARTRIDGE_ADD(SLOT4_TAG, multi_cococart_interface, coco_cart_slot4, "fdcv11", NULL)
MACHINE_CONFIG_END

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type COCO_MULTIPAK = &device_creator<coco_multipak_device>;

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  coco_multipak_device - constructor
//-------------------------------------------------
coco_multipak_device::coco_multipak_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
      : device_t(mconfig, COCO_MULTIPAK, "CoCo Multi-Pak Interface", tag, owner, clock),
		device_cococart_interface( mconfig, *this )
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void coco_multipak_device::device_start()
{
	m_slot1 = dynamic_cast<cococart_slot_device *>(subdevice(SLOT1_TAG));
	m_slot2 = dynamic_cast<cococart_slot_device *>(subdevice(SLOT2_TAG));
	m_slot3 = dynamic_cast<cococart_slot_device *>(subdevice(SLOT3_TAG));
	m_slot4 = dynamic_cast<cococart_slot_device *>(subdevice(SLOT4_TAG));
	m_owner = dynamic_cast<cococart_slot_device *>(owner());

	m_active = m_slot4;
}

//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor coco_multipak_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( coco_multi );
}

/*-------------------------------------------------
    get_cart_base
-------------------------------------------------*/

UINT8* coco_multipak_device::get_cart_base()
{
	return m_active->get_cart_base();
}

/*-------------------------------------------------
    read
-------------------------------------------------*/

READ8_MEMBER(coco_multipak_device::read)
{
	return m_active->read(space,offset);
}

/*-------------------------------------------------
    write
-------------------------------------------------*/

WRITE8_MEMBER(coco_multipak_device::write)
{
	m_active->write(space,offset,data);
}
