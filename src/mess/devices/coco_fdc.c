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
#include "cococart.h"
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
#define FDCINFO_PTR_HWTYPE		DEVINFO_PTR_DEVICE_SPECIFIC



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _fdc_hardware_type fdc_hardware_type;
struct _fdc_hardware_type
{
	const char *name;
	void (*update_lines)(device_t *device);
	read8_device_func ff40_r;
	write8_device_func ff40_w;
	unsigned initial_drq : 1;
	device_type wdtype;
	machine_config_constructor wdmachine;
};


typedef struct _fdc_t fdc_t;
struct _fdc_t
{
	const fdc_hardware_type *hwtype;

	UINT8 dskreg;
	unsigned drq : 1;
	unsigned intrq : 1;

	device_t *cococart;			/* CoCo cart slot interface */
	device_t *wd17xx;			/* WD17xx */
	device_t *ds1315;			/* DS1315 */

	/* Disto RTC */
	device_t *disto_msm6242;		/* 6242 RTC on Disto interface */
	offs_t msm6242_rtc_address;
};


typedef enum
{
	RTC_DISTO	= 0x00,
	RTC_CLOUD9	= 0x01,

	RTC_NONE	= 0xFF
} rtc_type_t;


/***************************************************************************
    PROTOTYPES
***************************************************************************/

static WRITE_LINE_DEVICE_HANDLER( fdc_intrq_w );
static WRITE_LINE_DEVICE_HANDLER( fdc_drq_w );


/***************************************************************************
    LOCAL VARIABLES
***************************************************************************/

static const wd17xx_interface coco_wd17xx_interface =
{
	DEVCB_NULL,
	DEVCB_LINE(fdc_intrq_w),
	DEVCB_LINE(fdc_drq_w),
	{FLOPPY_0,FLOPPY_1,FLOPPY_2,FLOPPY_3}
};


/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    get_token
-------------------------------------------------*/

INLINE fdc_t *get_token(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == COCO_CARTRIDGE_PCB_FDC_COCO) || (device->type() == COCO_CARTRIDGE_PCB_FDC_DRAGON));
	return (fdc_t *) downcast<legacy_device_base *>(device)->token();
}


/***************************************************************************
    GENERAL FDC CODE
***************************************************************************/

/*-------------------------------------------------
    real_time_clock
-------------------------------------------------*/

INLINE rtc_type_t real_time_clock(device_t *device)
{
	rtc_type_t result;
	fdc_t *fdc = get_token(device);

	result = (rtc_type_t) input_port_read_safe(device->machine(), "real_time_clock", RTC_NONE);

	/* check to make sure we don't have any invalid values */
	if (((result == RTC_DISTO) && (fdc->disto_msm6242 == NULL))
		|| ((result == RTC_CLOUD9) && (fdc->ds1315 == NULL)))
	{
		result = RTC_NONE;
	}

	return result;
}


/*-------------------------------------------------
    fdc_intrq_w - callback from the FDC
-------------------------------------------------*/

static WRITE_LINE_DEVICE_HANDLER( fdc_intrq_w )
{
	fdc_t *fdc = get_token(device->owner());
	fdc->intrq = state;
	(*fdc->hwtype->update_lines)(device->owner());
}


/*-------------------------------------------------
    fdc_drq_w - callback from the FDC
-------------------------------------------------*/

static WRITE_LINE_DEVICE_HANDLER( fdc_drq_w )
{
	fdc_t *fdc = get_token(device->owner());
	fdc->drq = state;
	(*fdc->hwtype->update_lines)(device->owner());
}


/*-------------------------------------------------
    fdc_start - general function to initialize FDC
-------------------------------------------------*/

static DEVICE_START(fdc)
{
	fdc_t *fdc = get_token(device);
    const fdc_hardware_type *hwtype = (const fdc_hardware_type *)downcast<const legacy_cart_slot_device_config_base *>(&device->baseconfig())->get_config_ptr(FDCINFO_PTR_HWTYPE);

	/* initialize variables */
	memset(fdc, 0, sizeof(*fdc));
	fdc->hwtype         = hwtype;
	fdc->drq            = hwtype->initial_drq;
	fdc->cococart		= device->owner()->owner();
	fdc->disto_msm6242	= device->subdevice(DISTO_TAG);
	fdc->ds1315			= device->subdevice(CLOUD9_TAG);
	fdc->wd17xx			= device->subdevice(WD_TAG);

	assert(fdc->wd17xx != NULL);
}



/*-------------------------------------------------
    DEVICE_GET_INFO( fdc ) - general FDC get info func
-------------------------------------------------*/

static void general_fdc_get_info(const device_config *device, UINT32 state, deviceinfo *info,
	const fdc_hardware_type *hwtype)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(fdc_t);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data --- */
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = hwtype->wdmachine;	break;
		case FDCINFO_PTR_HWTYPE:						info->p = (void *) hwtype;					break;

		/* --- the following bits of info are returned as pointers to functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(fdc);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;
		case COCOCARTINFO_FCT_FF40_R:					info->f = (genf *) hwtype->ff40_r;			break;
		case COCOCARTINFO_FCT_FF40_W:					info->f = (genf *) hwtype->ff40_w;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, hwtype->name);				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "CoCo FDC");				break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
	}
}



/***************************************************************************
    COCO-SPECIFIC FDC CODE
***************************************************************************/

/*-------------------------------------------------
    fdc_coco_update_lines - CoCo specific disk
    controller lines
-------------------------------------------------*/

static void fdc_coco_update_lines(device_t *device)
{
	fdc_t *fdc = get_token(device);

	/* clear HALT enable under certain circumstances */
	if ((fdc->intrq != 0) && (fdc->dskreg & 0x20))
		fdc->dskreg &= ~0x80;	/* clear halt enable */

	/* set the NMI line */
	coco_cartridge_set_line(
		fdc->cococart,
		COCOCART_LINE_NMI,
		((fdc->intrq != 0) && (fdc->dskreg & 0x20)) ? COCOCART_LINE_VALUE_ASSERT : COCOCART_LINE_VALUE_CLEAR);

	/* set the HALT line */
	coco_cartridge_set_line(
		fdc->cococart,
		COCOCART_LINE_HALT,
		((fdc->drq == 0) && (fdc->dskreg & 0x80)) ? COCOCART_LINE_VALUE_ASSERT : COCOCART_LINE_VALUE_CLEAR);
}



/*-------------------------------------------------
    fdc_coco_dskreg_w - function to write to CoCo
    dskreg
-------------------------------------------------*/

static void fdc_coco_dskreg_w(device_t *device, UINT8 data)
{
	fdc_t *fdc = get_token(device);
	UINT8 drive = 0;
	UINT8 head = 0;

	if (LOG_FDC)
	{
		logerror("fdc_coco_dskreg_w(): %c%c%c%c%c%c%c%c ($%02x)\n",
			data & 0x80 ? 'H' : 'h',
			data & 0x40 ? '3' : '.',
			data & 0x20 ? 'D' : 'S',
			data & 0x10 ? 'P' : 'p',
			data & 0x08 ? 'M' : 'm',
			data & 0x04 ? '2' : '.',
			data & 0x02 ? '1' : '.',
			data & 0x01 ? '0' : '.',
			data);
	}

	/* An email from John Kowalski informed me that if the DS3 is
     * high, and one of the other drive bits is selected (DS0-DS2), then the
     * second side of DS0, DS1, or DS2 is selected.  If multiple bits are
     * selected in other situations, then both drives are selected, and any
     * read signals get yucky.
     */

	if (data & 0x04)
		drive = 2;
	else if (data & 0x02)
		drive = 1;
	else if (data & 0x01)
		drive = 0;
	else if (data & 0x40)
		drive = 3;

	head = ((data & 0x40) && (drive != 3)) ? 1 : 0;

	fdc->dskreg = data;

	(*fdc->hwtype->update_lines)(device);

	wd17xx_set_drive(fdc->wd17xx, drive);
	wd17xx_set_side(fdc->wd17xx, head);
	wd17xx_dden_w(fdc->wd17xx, !BIT(fdc->dskreg, 5));
}



/*-------------------------------------------------
    fdc_coco_r - function to read from CoCo FDC
-------------------------------------------------*/

static UINT8 fdc_coco_r(device_t *device, offs_t addr)
{
	fdc_t *fdc = get_token(device);
	UINT8 result = 0;

	switch(addr & 0xEF)
	{
		case 8:
			result = wd17xx_status_r(fdc->wd17xx, 0);
			break;
		case 9:
			result = wd17xx_track_r(fdc->wd17xx, 0);
			break;
		case 10:
			result = wd17xx_sector_r(fdc->wd17xx, 0);
			break;
		case 11:
			result = wd17xx_data_r(fdc->wd17xx, 0);
			break;
	}

	/* other stuff for RTCs */
	switch(addr)
	{
		case 0x10:	/* FF50 */
			if (real_time_clock(device) == RTC_DISTO)
				result = msm6242_r(fdc->disto_msm6242, fdc->msm6242_rtc_address);
			break;

		case 0x38:	/* FF78 */
			if (real_time_clock(device) == RTC_CLOUD9)
				ds1315_r_0(fdc->ds1315, addr);
			break;

		case 0x39:	/* FF79 */
			if (real_time_clock(device) == RTC_CLOUD9)
				ds1315_r_1(fdc->ds1315, addr);
			break;

		case 0x3C:	/* FF7C */
			if (real_time_clock(device) == RTC_CLOUD9)
				result = ds1315_r_data(fdc->ds1315, addr);
			break;
	}
	return result;
}



/*-------------------------------------------------
    fdc_coco_w - function to write to CoCo FDC
-------------------------------------------------*/

static void fdc_coco_w(device_t *device, offs_t addr, UINT8 data)
{
	fdc_t *fdc = get_token(device);

	switch(addr & 0x1F)
	{
		case 0: case 1: case 2: case 3:
		case 4: case 5: case 6: case 7:
			fdc_coco_dskreg_w(device, data);
			break;
		case 8:
			wd17xx_command_w(fdc->wd17xx, 0, data);
			break;
		case 9:
			wd17xx_track_w(fdc->wd17xx, 0, data);
			break;
		case 10:
			wd17xx_sector_w(fdc->wd17xx, 0, data);
			break;
		case 11:
			wd17xx_data_w(fdc->wd17xx, 0, data);
			break;
	};

	/* other stuff for RTCs */
	switch(addr)
	{
		case 0x10:	/* FF50 */
			if (real_time_clock(device) == RTC_DISTO)
				msm6242_w(fdc->disto_msm6242, fdc->msm6242_rtc_address, data);
			break;

		case 0x11:	/* FF51 */
			if (real_time_clock(device) == RTC_DISTO)
				fdc->msm6242_rtc_address = data & 0x0f;
			break;
	}
}


static const floppy_config coco_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(coco),
	NULL
};

/*-------------------------------------------------
    DEVICE_GET_INFO(coco_cartridge_pcb_fdc_coco) -
    get info function for the CoCo FDC
-------------------------------------------------*/
static MACHINE_CONFIG_FRAGMENT(coco_fdc)
	MCFG_WD1773_ADD(WD_TAG, coco_wd17xx_interface)
	MCFG_MSM6242_ADD(DISTO_TAG)
	MCFG_DS1315_ADD(CLOUD9_TAG)
MACHINE_CONFIG_END

DEVICE_GET_INFO(coco_cartridge_pcb_fdc_coco)
{
	static const fdc_hardware_type hwtype =
	{
		"CoCo FDC",
		fdc_coco_update_lines,
		fdc_coco_r,
		fdc_coco_w,
		1,
		WD1773,
		MACHINE_CONFIG_NAME(coco_fdc)
	};
	general_fdc_get_info(device, state, info, &hwtype);
}



/***************************************************************************
    DRAGON-SPECIFIC FDC CODE
***************************************************************************/

/*-------------------------------------------------
    fdc_dragon_update_lines - Dragon specific disk
    controller lines
-------------------------------------------------*/

static void fdc_dragon_update_lines(device_t *device)
{
	fdc_t *fdc = get_token(device);

	/* set the NMI line */
	coco_cartridge_set_line(
		fdc->cococart,
		COCOCART_LINE_NMI,
		((fdc->intrq != 0) && (fdc->dskreg & 0x20)) ? COCOCART_LINE_VALUE_ASSERT : COCOCART_LINE_VALUE_CLEAR);

	/* set the CART line */
	coco_cartridge_set_line(
		fdc->cococart,
		COCOCART_LINE_CART,
		(fdc->drq != 0) ? COCOCART_LINE_VALUE_ASSERT : COCOCART_LINE_VALUE_CLEAR);
}


/*-------------------------------------------------
    fdc_dragon_dskreg_w - function to write to
    Dragon dskreg
-------------------------------------------------*/

static void fdc_dragon_dskreg_w(device_t *device, UINT8 data)
{
	fdc_t *fdc = get_token(device);

	if (LOG_FDC)
	{
		logerror("fdc_dragon_dskreg_w(): %c%c%c%c%c%c%c%c ($%02x)\n",
			data & 0x80 ? 'X' : 'x',
			data & 0x40 ? 'X' : 'x',
			data & 0x20 ? 'N' : 'n',
			data & 0x10 ? 'P' : 'p',
			data & 0x08 ? 'S' : 'D',
			data & 0x04 ? 'M' : 'm',
			data & 0x02 ? '1' : '0',
			data & 0x01 ? '1' : '0',
			data);
	}

	if (data & 0x04)
		wd17xx_set_drive(fdc->wd17xx, data & 0x03);

	wd17xx_dden_w(fdc->wd17xx, BIT(data, 3));
	fdc->dskreg = data;
}



/*-------------------------------------------------
    fdc_dragon_r - function to read from Dragon FDC
-------------------------------------------------*/

static UINT8 fdc_dragon_r(device_t *device, offs_t addr)
{
	fdc_t *fdc = get_token(device);

	UINT8 result = 0;
	switch(addr & 0xEF)
	{
		case 0:
			result = wd17xx_status_r(fdc->wd17xx, 0);
			break;
		case 1:
			result = wd17xx_track_r(fdc->wd17xx, 0);
			break;
		case 2:
			result = wd17xx_sector_r(fdc->wd17xx, 0);
			break;
		case 3:
			result = wd17xx_data_r(fdc->wd17xx, 0);
			break;
	}
	return result;
}



/*-------------------------------------------------
    fdc_dragon_w - function to write to Dragon FDC
-------------------------------------------------*/

static void fdc_dragon_w(device_t *device, offs_t addr, UINT8 data)
{
	fdc_t *fdc = get_token(device);

	switch(addr & 0xEF)
	{
		case 0:
			wd17xx_command_w(fdc->wd17xx, 0, data);

			/* disk head is encoded in the command byte */
			/* Only for type 3 & 4 commands */
			if (data & 0x80)
				wd17xx_set_side(fdc->wd17xx, (data & 0x02) ? 1 : 0);
			break;
		case 1:
			wd17xx_track_w(fdc->wd17xx, 0, data);
			break;
		case 2:
			wd17xx_sector_w(fdc->wd17xx, 0, data);
			break;
		case 3:
			wd17xx_data_w(fdc->wd17xx, 0, data);
			break;
		case 8: case 9: case 10: case 11:
		case 12: case 13: case 14: case 15:
			fdc_dragon_dskreg_w(device, data);
			break;
	};
}



/*-------------------------------------------------
    DEVICE_GET_INFO(coco_cartridge_pcb_fdc_dragon) -
    get info function for the CoCo FDC
-------------------------------------------------*/
static MACHINE_CONFIG_FRAGMENT(dragon_fdc)
	MCFG_WD179X_ADD(WD_TAG, coco_wd17xx_interface)
MACHINE_CONFIG_END

DEVICE_GET_INFO(coco_cartridge_pcb_fdc_dragon)
{
	static const fdc_hardware_type hwtype =
	{
		"Dragon FDC",
		fdc_dragon_update_lines,
		fdc_dragon_r,
		fdc_dragon_w,
		0,
		WD179X,
		MACHINE_CONFIG_NAME(dragon_fdc)
	};
	general_fdc_get_info(device, state, info, &hwtype);
}


DEFINE_LEGACY_CART_SLOT_DEVICE(COCO_CARTRIDGE_PCB_FDC_COCO, coco_cartridge_pcb_fdc_coco);
DEFINE_LEGACY_CART_SLOT_DEVICE(COCO_CARTRIDGE_PCB_FDC_DRAGON, coco_cartridge_pcb_fdc_dragon);
