/*********************************************************************

    coco_fdc.c

    CoCo/Dragon FDC

    The CoCo and Dragon both use the Western Digital floppy disk controllers.
    The CoCo uses either the WD1793 or the WD1773, the Dragon uses the WD2797,
    which mostly uses the same command set with some subtle differences, most
    notably the 2797 handles disk side select internally. The Dragon Alpha also
    uses the WD2797, however as this is a built in interface and not an external
    cartrige, it is dealt with in the main coco.c file.

    The wd's variables are mapped to $FF48-$FF4B on the CoCo and on $FF40-$FF43
    on the Dragon.  In addition, there is another register
    called DSKREG that controls the interface with the wd1793.  DSKREG is
    detailed below:  But they appear to be

    References:
        CoCo:   Disk Basic Unravelled
        Dragon: Inferences from the PC-Dragon source code
                DragonDos Controller, Disk and File Formats by Graham E Kinns

    ---------------------------------------------------------------------------

    DSKREG - the control register
    CoCo ($FF40)                                    Dragon ($FF48)

    Bit                                              Bit
    7 halt enable flag                               7 not used
    6 drive select #3                                6 not used
    5 density (0=single, 1=double)                   5 NMI enable flag
        and NMI enable flag
    4 write precompensation                          4 write precompensation
    3 drive motor activation                         3 single density enable
    2 drive select #2                                2 drive motor activation
    1 drive select #1                                1 drive select high bit
    0 drive select #0                                0 drive select low bit

    Reading from $FF48-$FF4F clears bit 7 of DSKREG ($FF40)

    ---------------------------------------------------------------------------

    2007-02-22, P.Harvey-Smith

    Began implementing the Dragon Delta Dos controler, this was actually the first
    Dragon disk controler to market, beating Dragon Data's by a couple of months,
    it is based around the WD2791 FDC, which is compatible with the WD1793/WD2797 used
    by the standard CoCo and Dragon disk controlers except that it used an inverted
    data bus, which is the reason the read/write handlers invert the data. This
    controler like, the DragonDos WD2797 is mapped at $FF40-$FF43, in the normal
    register order.

    The Delta cart also has a register (74LS174 hex flipflop) at $FF44 encoded as
    follows :-

    Bit
    7 not used
    6 not used
    5 not used
    4 Single (0) / Double (1) density select
    3 5.25"(0) / 8"(1) Clock select
    2 Side select
    1 Drive select ms bit
    0 Drive select ls bit

*********************************************************************/

#include "emu.h"
#include "coco_fdc.h"
#include "imagedev/flopdrv.h"
#include "includes/coco.h"
#include "machine/wd17xx.h"
#include "machine/ds1315.h"
#include "machine/msm6242.h"
#include "imagedev/flopdrv.h"
#include "formats/coco_dsk.h"


/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG_FDC					0
#define WD_TAG					"wd17xx"
#define DISTO_TAG				"disto"
#define CLOUD9_TAG				"cloud9"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/***************************************************************************
    PROTOTYPES
***************************************************************************/

static WRITE_LINE_DEVICE_HANDLER( coco_fdc_intrq_w );
static WRITE_LINE_DEVICE_HANDLER( coco_fdc_drq_w );
static WRITE_LINE_DEVICE_HANDLER( dragon_fdc_intrq_w );
static WRITE_LINE_DEVICE_HANDLER( dragon_fdc_drq_w );

/***************************************************************************
    LOCAL VARIABLES
***************************************************************************/

static const floppy_interface coco_floppy_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(coco),
	NULL,
	NULL
};


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

//**************************************************************************
//  			COCO FDC
//**************************************************************************

static const wd17xx_interface coco_wd17xx_interface =
{
	DEVCB_NULL,
	DEVCB_LINE(coco_fdc_intrq_w),
	DEVCB_LINE(coco_fdc_drq_w),
	{FLOPPY_0,FLOPPY_1,FLOPPY_2,FLOPPY_3}
};

static MACHINE_CONFIG_FRAGMENT(coco_fdc)
	MCFG_WD1773_ADD(WD_TAG, coco_wd17xx_interface)
	MCFG_MSM6242_ADD(DISTO_TAG)
	MCFG_DS1315_ADD(CLOUD9_TAG)
	
	MCFG_FLOPPY_4_DRIVES_ADD(coco_floppy_interface)
MACHINE_CONFIG_END

ROM_START( coco_fdc )
	ROM_REGION(0x4000,"eprom",ROMREGION_ERASE00)
	ROM_LOAD_OPTIONAL(	"disk10.rom",	0x0000, 0x2000, CRC(b4f9968e) SHA1(04115be3f97952b9d9310b52f806d04f80b40d03))
ROM_END

const device_type COCO_FDC = &device_creator<coco_fdc_device>;

//-------------------------------------------------
//  coco_fdc_device - constructor
//-------------------------------------------------
coco_fdc_device::coco_fdc_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, type, name, tag, owner, clock),
	  device_cococart_interface( mconfig, *this ),
	  device_slot_card_interface(mconfig, *this)
{
}

coco_fdc_device::coco_fdc_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
      : device_t(mconfig, COCO_FDC, "CoCo FDC", tag, owner, clock),
		device_cococart_interface( mconfig, *this ),
		device_slot_card_interface(mconfig, *this)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void coco_fdc_device::device_start()
{
	m_owner = dynamic_cast<cococart_slot_device *>(owner());
	subtag(m_region_name, "eprom");
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void coco_fdc_device::device_config_complete()
{
	m_shortname = "coco_fdc";
}
//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor coco_fdc_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( coco_fdc );
}

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *coco_fdc_device::device_rom_region() const
{
	return ROM_NAME( coco_fdc );
}

/*-------------------------------------------------
    get_cart_base
-------------------------------------------------*/

UINT8* coco_fdc_device::get_cart_base()
{
	return machine().region(m_region_name.cstr())->base();
}


/*-------------------------------------------------
    coco_fdc_intrq_w - callback from the FDC
-------------------------------------------------*/

static WRITE_LINE_DEVICE_HANDLER( coco_fdc_intrq_w )
{
	//fdc->intrq = state;
	//(*fdc->hwtype->update_lines)(device->owner());
}


/*-------------------------------------------------
    coco_fdc_drq_w - callback from the FDC
-------------------------------------------------*/

static WRITE_LINE_DEVICE_HANDLER( coco_fdc_drq_w )
{
	//fdc->drq = state;
	//(*fdc->hwtype->update_lines)(device->owner());
}

//**************************************************************************
//  			DRAGON FDC
//**************************************************************************

static const wd17xx_interface dragon_wd17xx_interface =
{
	DEVCB_NULL,
	DEVCB_LINE(dragon_fdc_intrq_w),
	DEVCB_LINE(dragon_fdc_drq_w),
	{FLOPPY_0,FLOPPY_1,FLOPPY_2,FLOPPY_3}
};

static MACHINE_CONFIG_FRAGMENT(dragon_fdc)
	MCFG_WD2797_ADD(WD_TAG, dragon_wd17xx_interface)
	MCFG_MSM6242_ADD(DISTO_TAG)
	MCFG_DS1315_ADD(CLOUD9_TAG)
	
	MCFG_FLOPPY_4_DRIVES_ADD(coco_floppy_interface)
MACHINE_CONFIG_END


ROM_START( dragon_fdc )
	ROM_REGION(0x4000,"eprom",ROMREGION_ERASE00)
	ROM_LOAD_OPTIONAL(  "ddos10.rom",   0x0000,  0x2000, CRC(b44536f6) SHA1(a8918c71d319237c1e3155bb38620acb114a80bc))
ROM_END

const device_type DRAGON_FDC = &device_creator<dragon_fdc_device>;

//-------------------------------------------------
//  dragon_fdc_device - constructor
//-------------------------------------------------
dragon_fdc_device::dragon_fdc_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock)
	: coco_fdc_device(mconfig, type, name, tag, owner, clock)
{
}
dragon_fdc_device::dragon_fdc_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
      : coco_fdc_device(mconfig, DRAGON_FDC, "Dragon FDC", tag, owner, clock)
{
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void dragon_fdc_device::device_config_complete()
{
	m_shortname = "dragon_fdc";
}
//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor dragon_fdc_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( dragon_fdc );
}

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *dragon_fdc_device::device_rom_region() const
{
	return ROM_NAME( dragon_fdc );
}

/*-------------------------------------------------
    dragon_fdc_intrq_w - callback from the FDC
-------------------------------------------------*/

static WRITE_LINE_DEVICE_HANDLER( dragon_fdc_intrq_w )
{
	//fdc->intrq = state;
	//(*fdc->hwtype->update_lines)(device->owner());
}


/*-------------------------------------------------
    dragon_fdc_drq_w - callback from the FDC
-------------------------------------------------*/

static WRITE_LINE_DEVICE_HANDLER( dragon_fdc_drq_w )
{
	//fdc->drq = state;
	//(*fdc->hwtype->update_lines)(device->owner());
}

//**************************************************************************
//  			SDTANDY FDC
//**************************************************************************

ROM_START( sdtandy_fdc )
	ROM_REGION(0x4000,"eprom",ROMREGION_ERASE00)
	ROM_LOAD_OPTIONAL(  "sdtandy.rom",   0x0000,  0x2000, CRC(5d7779b7) SHA1(ca03942118f2deab2f6c8a89b8a4f41f2d0b94f1))
ROM_END

const device_type SDTANDY_FDC = &device_creator<sdtandy_fdc_device>;

//-------------------------------------------------
//  sdtandy_fdc_device - constructor
//-------------------------------------------------

sdtandy_fdc_device::sdtandy_fdc_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
      : dragon_fdc_device(mconfig, SDTANDY_FDC, "SDTANDY FDC", tag, owner, clock)
{
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void sdtandy_fdc_device::device_config_complete()
{
	m_shortname = "sdtandy_fdc";
}

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *sdtandy_fdc_device::device_rom_region() const
{
	return ROM_NAME( sdtandy_fdc );
}

//**************************************************************************
//  			COCO FDC v1.1
//**************************************************************************

ROM_START( coco_fdc_v11 )
	ROM_REGION(0x8000,"eprom",ROMREGION_ERASE00)
	ROM_LOAD_OPTIONAL(	"disk11.rom",	0x0000, 0x2000, CRC(0b9c5415) SHA1(10bdc5aa2d7d7f205f67b47b19003a4bd89defd1))
	ROM_RELOAD(0x2000, 0x2000)
	ROM_RELOAD(0x4000, 0x2000)
	ROM_RELOAD(0x6000, 0x2000)
ROM_END

const device_type COCO_FDC_V11 = &device_creator<coco_fdc_v11_device>;

//-------------------------------------------------
//  coco_fdc_v11_device - constructor
//-------------------------------------------------

coco_fdc_v11_device::coco_fdc_v11_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
      : coco_fdc_device(mconfig, COCO_FDC_V11, "CoCo FDC v1.1", tag, owner, clock)
{
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void coco_fdc_v11_device::device_config_complete()
{
	m_shortname = "coco_fdc_v11";
}

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *coco_fdc_v11_device::device_rom_region() const
{
	return ROM_NAME( coco_fdc_v11 );
}

//**************************************************************************
//  			CP400 FDC
//**************************************************************************

ROM_START( cp400_fdc )
	ROM_REGION(0x4000,"eprom",ROMREGION_ERASE00)
	ROM_LOAD("cp400dsk.rom",  0x0000, 0x2000, CRC(e9ad60a0) SHA1(827697fa5b755f5dc1efb054cdbbeb04e405405b))
ROM_END

const device_type CP400_FDC = &device_creator<cp400_fdc_device>;

//-------------------------------------------------
//  cp400_fdc_device - constructor
//-------------------------------------------------

cp400_fdc_device::cp400_fdc_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
      : coco_fdc_device(mconfig, CP400_FDC, "CP400 FDC", tag, owner, clock)
{
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void cp400_fdc_device::device_config_complete()
{
	m_shortname = "cp400_fdc";
}

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *cp400_fdc_device::device_rom_region() const
{
	return ROM_NAME( cp400_fdc );
}