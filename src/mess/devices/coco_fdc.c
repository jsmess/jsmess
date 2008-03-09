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
		CoCo:	Disk Basic Unravelled
		Dragon:	Inferences from the PC-Dragon source code
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

#include "driver.h"
#include "deprecat.h"
#include "cococart.h"
#include "coco_vhd.h"
#include "includes/coco.h"
#include "machine/wd17xx.h"
#include "machine/ds1315.h"
#include "machine/msm6242.h"

#define LOG_FDC		0


typedef struct _fdc_info fdc_info;
struct _fdc_info
{
	UINT8 dskreg;
	unsigned drq : 1;
	unsigned intrq : 1;

	void (*update_lines)(coco_cartridge *cartridge);
};



/***************************************************************************
    GENERAL FDC CODE
***************************************************************************/

/*-------------------------------------------------
    fdc_get_info
-------------------------------------------------*/

static fdc_info *fdc_get_info(coco_cartridge *cartridge)
{
	return cococart_get_extra_data(cartridge);
}



/*-------------------------------------------------
    fdc_callback - callback from the FDC
-------------------------------------------------*/

static void fdc_callback(wd17xx_state_t state, void *param)
{
	coco_cartridge *cartridge = (coco_cartridge *) param;
	fdc_info *info = fdc_get_info(cartridge);

	switch(state)
	{
		case WD17XX_IRQ_CLR:
			info->intrq = 0;
			break;

		case WD17XX_IRQ_SET:
			info->intrq = 1;
			break;

		case WD17XX_DRQ_CLR:
			info->drq = 0;
			break;

		case WD17XX_DRQ_SET:
			info->drq = 1;
			break;
	}

	(*info->update_lines)(cartridge);
}



/*-------------------------------------------------
    fdc_init - general function to initialize FDC
-------------------------------------------------*/

static void fdc_init(coco_cartridge *cartridge,
	wd17xx_type_t type,
	void (*update_lines)(coco_cartridge *cartridge),
	int initial_drq)
{
	fdc_info *info = fdc_get_info(cartridge);

	/* initialize variables */
	memset(info, 0, sizeof(*info));
	info->update_lines = update_lines;
	info->drq = initial_drq ? 1 : 0;

	/* initialize FDC */
	wd17xx_init(type, fdc_callback, (void *) cartridge);
}



/*-------------------------------------------------
    cococart_fdc - general FDC function
-------------------------------------------------*/

static void cococart_fdc(UINT32 state, cococartinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case COCOCARTINFO_INT_DATASIZE:						info->i = sizeof(fdc_info);	break;
	}
}



/***************************************************************************
    COCO-SPECIFIC FDC CODE
***************************************************************************/

/*-------------------------------------------------
    fdc_coco_update_lines - CoCo specific disk
	controller lines
-------------------------------------------------*/

static void fdc_coco_update_lines(coco_cartridge *cartridge)
{
	fdc_info *info = fdc_get_info(cartridge);

	/* clear HALT enable under certain circumstances */
	if ((info->intrq != 0) && (info->dskreg & 0x20))
		info->dskreg &= ~0x80;	/* clear halt enable */

	/* set the NMI line */
	cococart_set_line(
		cartridge,
		COCOCART_LINE_NMI,
		((info->intrq != 0) && (info->dskreg & 0x20)) ? COCOCART_LINE_VALUE_ASSERT : COCOCART_LINE_VALUE_CLEAR);

	/* set the HALT line */
	cococart_set_line(
		cartridge,
		COCOCART_LINE_HALT,
		((info->drq == 0) && (info->dskreg & 0x80)) ? COCOCART_LINE_VALUE_ASSERT : COCOCART_LINE_VALUE_CLEAR);
}



/*-------------------------------------------------
    fdc_coco_init - function to initialize CoCo FDC
-------------------------------------------------*/

static void fdc_coco_init(coco_cartridge *cartridge)
{
	fdc_init(cartridge, WD_TYPE_1773, fdc_coco_update_lines, 1);
}



/*-------------------------------------------------
    fdc_coco_dskreg_w - function to write to CoCo
	dskreg
-------------------------------------------------*/

static void fdc_coco_dskreg_w(coco_cartridge *cartridge, UINT8 data)
{
	fdc_info *info = fdc_get_info(cartridge);
	UINT8 drive = 0;
	UINT8 head = 0;
	int motor_mask = 0;

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
	motor_mask = 0x08;

	if (data & 0x04)
		drive = 2;
	else if (data & 0x02)
		drive = 1;
	else if (data & 0x01)
		drive = 0;
	else if (data & 0x40)
		drive = 3;
	else
		motor_mask = 0;

	head = ((data & 0x40) && (drive != 3)) ? 1 : 0;

	info->dskreg = data;

	(*info->update_lines)(cartridge);

	wd17xx_set_drive(drive);
	wd17xx_set_side(head);
	wd17xx_set_density((info->dskreg & 0x20) ? DEN_MFM_LO : DEN_FM_LO);
}



/*-------------------------------------------------
    fdc_coco_r - function to read from CoCo FDC
-------------------------------------------------*/

static UINT8 fdc_coco_r(coco_cartridge *cartridge, UINT16 addr)
{
	running_machine *machine = Machine;
	UINT8 result = 0;
	switch(addr & 0xEF)
	{
		case 8:
			result = wd17xx_status_r(machine, 0);
			break;
		case 9:
			result = wd17xx_track_r(machine, 0);
			break;
		case 10:
			result = wd17xx_sector_r(machine, 0);
			break;
		case 11:
			result = wd17xx_data_r(machine, 0);
			break;
	}
	return result;
}



/*-------------------------------------------------
    fdc_coco_w - function to write to CoCo FDC
-------------------------------------------------*/

static void fdc_coco_w(coco_cartridge *cartridge, UINT16 addr, UINT8 data)
{
	running_machine *machine = Machine;
	switch(addr & 0xEF)
	{
		case 0: case 1: case 2: case 3:
		case 4: case 5: case 6: case 7:
			fdc_coco_dskreg_w(cartridge, data);
			break;
		case 8:
			wd17xx_command_w(machine, 0, data);
			break;
		case 9:
			wd17xx_track_w(machine, 0, data);
			break;
		case 10:
			wd17xx_sector_w(machine, 0, data);
			break;
		case 11:
			wd17xx_data_w(machine, 0, data);
			break;
	};
}



/*-------------------------------------------------
    cococart_fdc_coco - get info function for the
	CoCo FDC
-------------------------------------------------*/

void cococart_fdc_coco(UINT32 state, cococartinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case COCOCARTINFO_INT_DATASIZE:						info->i = sizeof(fdc_info);	break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case COCOCARTINFO_PTR_INIT:							info->init = fdc_coco_init;	break;
		case COCOCARTINFO_PTR_FF40_R:						info->rh = fdc_coco_r;	break;
		case COCOCARTINFO_PTR_FF40_W:						info->wh = fdc_coco_w;	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case COCOCARTINFO_STR_NAME:							strcpy(info->s, "coco_fdc"); break;

		default: cococart_fdc(state, info); break;
	}
}



/***************************************************************************
    COCO3 EXTRA FEATURES/HACKS
***************************************************************************/

typedef enum
{
	RTC_DISTO	= 0x00,
	RTC_CLOUD9	= 0x01
} rtc_type_t;

static int msm6242_rtc_address;

/*-------------------------------------------------
    real_time_clock
-------------------------------------------------*/

static rtc_type_t real_time_clock(void)
{
	return (rtc_type_t) (int) readinputportbytag_safe("real_time_clock", 0x00);
}



/*-------------------------------------------------
    fdc_coco3plus_r
-------------------------------------------------*/

static UINT8 fdc_coco3plus_r(coco_cartridge *cartridge, UINT16 addr)
{
	running_machine *machine = Machine;
	UINT8 result = fdc_coco_r(cartridge, addr);

	switch(addr)
	{
		case 0x10:	/* FF50 */
			if (real_time_clock() == RTC_DISTO)
				result = msm6242_r(machine, msm6242_rtc_address);
			break;

		case 0x38:	/* FF78 */
			if (real_time_clock() == RTC_CLOUD9)
				ds1315_r_0(machine, addr);
			break;

		case 0x39:	/* FF79 */
			if (real_time_clock() == RTC_CLOUD9)
				ds1315_r_1(machine, addr);
			break;

		case 0x3C:	/* FF7C */
			if (real_time_clock() == RTC_CLOUD9)
				result = ds1315_r_data(machine, addr);
			break;

		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43:
		case 0x44:
		case 0x45:
			if (device_count(IO_VHD) > 0)
				result = coco_vhd_io_r(machine, addr);
			break;
	}
	return result;
}



/*-------------------------------------------------
    fdc_coco3plus_w
-------------------------------------------------*/

static void fdc_coco3plus_w(coco_cartridge *cartridge, UINT16 addr, UINT8 data)
{
	running_machine *machine = Machine;

	fdc_coco_w(cartridge, addr, data);

	switch(addr)
	{
		case 0x10:	/* FF50 */
			if (real_time_clock() == RTC_DISTO)
				msm6242_w(machine, msm6242_rtc_address, data);
			break;

		case 0x11:	/* FF51 */
			if (real_time_clock() == RTC_DISTO)
				msm6242_rtc_address = data & 0x0f;
			break;

		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43:
		case 0x44:
		case 0x45:
			if (device_count(IO_VHD) > 0)
				coco_vhd_io_w(machine, addr, data);
			break;
	}
}



/*-------------------------------------------------
    cococart_fdc_coco3_plus - get info function for
	CoCo FDC, with extra CoCo 3 features
-------------------------------------------------*/

void cococart_fdc_coco3_plus(UINT32 state, cococartinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case COCOCARTINFO_PTR_FF40_R:						info->rh = fdc_coco3plus_r;	break;
		case COCOCARTINFO_PTR_FF40_W:						info->wh = fdc_coco3plus_w;	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case COCOCARTINFO_STR_NAME:							strcpy(info->s, "coco3_plus_fdc"); break;

		default: cococart_fdc_coco(state, info); break;
	}
}



/***************************************************************************
    DRAGON-SPECIFIC FDC CODE
***************************************************************************/

/*-------------------------------------------------
    fdc_dragon_update_lines - Dragon specific disk
	controller lines
-------------------------------------------------*/

static void fdc_dragon_update_lines(coco_cartridge *cartridge)
{
	fdc_info *info = fdc_get_info(cartridge);

	/* set the NMI line */
	cococart_set_line(
		cartridge,
		COCOCART_LINE_NMI,
		((info->intrq != 0) && (info->dskreg & 0x20)) ? COCOCART_LINE_VALUE_ASSERT : COCOCART_LINE_VALUE_CLEAR);

	/* set the CART line */
	cococart_set_line(
		cartridge,
		COCOCART_LINE_CART,
		(info->drq != 0) ? COCOCART_LINE_VALUE_ASSERT : COCOCART_LINE_VALUE_CLEAR);
}



/*-------------------------------------------------
    fdc_dragon_init - function to initialize Dragon
	FDC
-------------------------------------------------*/

static void fdc_dragon_init(coco_cartridge *cartridge)
{
	fdc_init(cartridge, WD_TYPE_179X, fdc_dragon_update_lines, 0);
}



/*-------------------------------------------------
    fdc_dragon_dskreg_w - function to write to
	Dragon dskreg
-------------------------------------------------*/

static void fdc_dragon_dskreg_w(coco_cartridge *cartridge, UINT8 data)
{
	fdc_info *info = fdc_get_info(cartridge);

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
		wd17xx_set_drive(data & 0x03);

	wd17xx_set_density((data & 0x08) ? DEN_FM_LO: DEN_MFM_LO);
	info->dskreg = data;
}



/*-------------------------------------------------
    fdc_dragon_r - function to read from Dragon FDC
-------------------------------------------------*/

static UINT8 fdc_dragon_r(coco_cartridge *cartridge, UINT16 addr)
{
	running_machine *machine = Machine;
	UINT8 result = 0;
	switch(addr & 0xEF)
	{
		case 0:
			result = wd17xx_status_r(machine, 0);
			break;
		case 1:
			result = wd17xx_track_r(machine, 0);
			break;
		case 2:
			result = wd17xx_sector_r(machine, 0);
			break;
		case 3:
			result = wd17xx_data_r(machine, 0);
			break;
	}
	return result;
}



/*-------------------------------------------------
    fdc_dragon_w - function to write to Dragon FDC
-------------------------------------------------*/

static void fdc_dragon_w(coco_cartridge *cartridge, UINT16 addr, UINT8 data)
{
	running_machine *machine = Machine;
	switch(addr & 0xEF)
	{
		case 0:
			wd17xx_command_w(machine, 0, data);

			/* disk head is encoded in the command byte */
			/* Only for type 3 & 4 commands */
			if (data & 0x80)
				wd17xx_set_side((data & 0x02) ? 1 : 0);
			break;
		case 1:
			wd17xx_track_w(machine, 0, data);
			break;
		case 2:
			wd17xx_sector_w(machine, 0, data);
			break;
		case 3:
			wd17xx_data_w(machine, 0, data);
			break;
		case 8: case 9: case 10: case 11:
		case 12: case 13: case 14: case 15:
			fdc_dragon_dskreg_w(cartridge, data);
			break;
	};
}



/*-------------------------------------------------
    cococart_fdc_dragon - get info function for the
	Dragon FDC
-------------------------------------------------*/

void cococart_fdc_dragon(UINT32 state, cococartinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case COCOCARTINFO_INT_DATASIZE:						info->i = sizeof(fdc_info);	break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case COCOCARTINFO_PTR_INIT:							info->init = fdc_dragon_init;	break;
		case COCOCARTINFO_PTR_FF40_R:						info->rh = fdc_dragon_r;	break;
		case COCOCARTINFO_PTR_FF40_W:						info->wh = fdc_dragon_w;	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case COCOCARTINFO_STR_NAME:							strcpy(info->s, "dragon_fdc"); break;

		default: cococart_fdc(state, info); break;
	}
}
