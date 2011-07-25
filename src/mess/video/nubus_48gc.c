/***************************************************************************

  Apple 4*8 Graphics Card (model 630-0400) emulation

***************************************************************************/

#include "emu.h"
#include "video/nubus_48gc.h"

#define GC48_SCREEN_NAME	"48gc_screen"
#define GC48_ROM_REGION		"48gc_rom"

static SCREEN_UPDATE( mac_48gc );
static READ32_DEVICE_HANDLER( mac_48gc_r );
static WRITE32_DEVICE_HANDLER( mac_48gc_w );

MACHINE_CONFIG_FRAGMENT( macvideo_48gc )
	MCFG_SCREEN_ADD( GC48_SCREEN_NAME, RASTER)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_UPDATE( mac_48gc )
	MCFG_SCREEN_RAW_PARAMS(25175000, 800, 0, 640, 525, 0, 480)
MACHINE_CONFIG_END

ROM_START( gc48 )
	ROM_REGION(0x8000, GC48_ROM_REGION, 0)
	ROM_LOAD( "3410801.bin",  0x0000, 0x8000, CRC(e283da91) SHA1(4ae21d6d7bbaa6fc7aa301bee2b791ed33b1dcf9) ) 
ROM_END

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type NUBUS_48GC = &device_creator<nubus_48gc_device>;


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor nubus_48gc_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( macvideo_48gc );
}

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *nubus_48gc_device::device_rom_region() const
{
	return ROM_NAME( gc48 );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  nubus_48gc_device - constructor
//-------------------------------------------------

nubus_48gc_device::nubus_48gc_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, NUBUS_48GC, "NUBUS_48GC", tag, owner, clock),
		device_nubus_card_interface(mconfig, *this),
		device_slot_card_interface(mconfig, *this)
{
	m_shortname = "nb_48gc";
}

nubus_48gc_device::nubus_48gc_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, type, name, tag, owner, clock),
		device_nubus_card_interface(mconfig, *this),
		device_slot_card_interface(mconfig, *this)
{
	m_shortname = "nb_48gc";
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void nubus_48gc_device::device_start()
{
	UINT32 slotspace;

	// set_nubus_device makes m_slot valid
	set_nubus_device();
	install_declaration_rom(GC48_ROM_REGION);

	slotspace = get_slotspace();

	printf("slot %d, slotspace %08x\n", m_slot, slotspace);

	m_vram = auto_alloc_array(machine(), UINT8, 0x7ffff);
	m_nubus->install_device(this, slotspace+0x200000, slotspace+0x2003ff, 0, 0, FUNC(mac_48gc_r), FUNC(mac_48gc_w) );
	m_nubus->install_bank(slotspace, slotspace+0x7ffff, 0, 0x07000, "bank_48gc", m_vram);
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void nubus_48gc_device::device_reset()
{
}

/***************************************************************************

  Monochrome Display Adapter (MDA) section

***************************************************************************/

static SCREEN_UPDATE( mac_48gc )
{
	return 0;
}

WRITE32_DEVICE_HANDLER ( mac_48gc_w )
{
	printf("48gc_w: %08x to %x, mask %08x\n", data, offset, mem_mask);
}

READ32_DEVICE_HANDLER ( mac_48gc_r )
{
	return 0;
}
