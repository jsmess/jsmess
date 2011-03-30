/**********************************************************************

    Motorola 6883 SAM interface and emulation

    This function emulates all the functionality of one M6883
    synchronous address multiplexer.

    Note that the real SAM chip was intimately involved in things like
    memory and video addressing, which are things that the MAME core
    largely handles.  Thus, this code only takes care of a small part
    of the SAM's actual functionality; it simply tracks the SAM
    registers and handles things like save states.  It then delegates
    the bulk of the responsibilities back to the host.

    The Motorola 6883 SAM has 16 bits worth of state, but the state is changed
    by writing into a 32 byte address space; odd bytes set bits and even bytes
    clear bits.  Here is the layout:

        31  Set     TY  Map Type            0: RAM/ROM  1: All RAM
        30  Clear   TY  Map Type
        29  Set     M1  Memory Size         00: 4K      10: 64K Dynamic
        28  Clear   M1  Memory Size         01: 16K     11: 64K Static
        27  Set     M0  Memory Size
        26  Clear   M0  Memory Size
        25  Set     R1  MPU Rate            00: Slow    10: Fast
        24  Clear   R1  MPU Rate            01: Dual    11: Fast
        23  Set     R0  MPU Rate
        22  Clear   R0  MPU Rate
        21  Set     P1  Page #1             0: Low      1: High
        20  Clear   P1  Page #1
        19  Set     F6  Display Offset
        18  Clear   F6  Display Offset
        17  Set     F5  Display Offset
        16  Clear   F5  Display Offset
        15  Set     F4  Display Offset
        14  Clear   F4  Display Offset
        13  Set     F3  Display Offset
        12  Clear   F3  Display Offset
        11  Set     F2  Display Offset
        10  Clear   F2  Display Offset
         9  Set     F1  Display Offset
         8  Clear   F1  Display Offset
         7  Set     F0  Display Offset
         6  Clear   F0  Display Offset
         5  Set     V2  VDG Mode
         4  Clear   V2  VDG Mode
         3  Set     V1  VDG Mode
         2  Clear   V1  VDG Mode
         1  Set     V0  VDG Mode
         0  Clear   V0  VDG Mode

**********************************************************************/

#include "machine/6883sam.h"
#include "machine/ram.h"

#define LOG_VIDEO_POSITION	0

typedef enum
{
	TYPE_SAM6883 = 0,
	TYPE_SAM6883_GIME = 1,
} SAM6883_VERSION;


/*****************************************************************************
 Type definitions
*****************************************************************************/

typedef struct _sam6883_t sam6883_t;
struct _sam6883_t
{
	SAM6883_VERSION type;
	const sam6883_interface *intf;
	UINT16 state;
	UINT16 old_state;
	UINT16 video_position;
	int last_scanline;
};

static const UINT8 sam_video_mode_row_heights[] =
{
	12,	/* 0 - Text */
	3,	/* 1 - G1C/G1R */
	3,	/* 2 - G2C */
	2,	/* 3 - G2R */
	2,	/* 4 - G3C */
	1,	/* 5 - G3R */
	1,	/* 6 - G4C/G4R */
	1	/* 7 - Reserved/Invalid */
};

static const UINT8 sam_video_mode_row_pitches[] =
{
	32,	/* 0 - Text */
	16,	/* 1 - Graphics CG1/RG1 */
	32,	/* 2 - Graphics CG2 */
	16,	/* 3 - Graphics RG2 */
	32,	/* 4 - Graphics CG3 */
	16,	/* 5 - Graphics RG3 */
	32,	/* 6 - Graphics CG6/RG6 */
	32	/* 7 - Reserved/Invalid */
};


/*****************************************************************************
 Implementation
*****************************************************************************/

INLINE sam6883_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == SAM6883 || device->type() == SAM6883_GIME);

	return (sam6883_t *)downcast<legacy_device_base *>(device)->token();
}

static void update_sam(device_t *device)
{
	sam6883_t *sam = get_safe_token(device);
	UINT16 xorval;

	xorval = sam->old_state ^ sam->state;
	sam->old_state = sam->state;

	/* Check changes in Page #1 */
	if ((xorval & 0x0400) && sam->intf->set_pageonemode)
		sam->intf->set_pageonemode(device,(sam->state & 0x0400) / 0x0400);

	/* Check changes in MPU Rate */
	if ((xorval & 0x1800) && sam->intf->set_mpurate)
		sam->intf->set_mpurate(device,(sam->state & 0x1800) / 0x0800);

	/* Check changes in Memory Size */
	if ((xorval & 0x6000) && sam->intf->set_memorysize)
		sam->intf->set_memorysize(device,(sam->state & 0x6000) / 0x2000);

	/* Check changes in Map Type */
	if ((xorval & 0x8000) && sam->intf->set_maptype)
		sam->intf->set_maptype(device,(sam->state & 0x8000) / 0x8000);
}



static STATE_POSTLOAD( update_sam_postload )
{
	device_t *device = (device_t *)param;
	update_sam(device);
}

void sam6883_set_state(device_t *device, UINT16 state, UINT16 mask)
{
	sam6883_t *sam = get_safe_token(device);
	sam->state &= ~mask;
	sam->state |= (state & mask);
	update_sam(device);
}

WRITE8_DEVICE_HANDLER(sam6883_w)
{
	UINT16 mask;
	sam6883_t *sam = get_safe_token(device);

	if (offset < 32)
	{
		mask = 1 << (offset / 2);
		if (offset & 1)
			sam->state |= mask;
		else
			sam->state &= ~mask;
		update_sam(device);
	}
}

#if 0
WRITE_LINE_DEVICE_HANDLER( sam6883_hs_w )
{
	sam6883_t *sam = get_safe_token(device);
}

WRITE8_DEVICE_HANDLER( sam6883_da_w )
{
	sam6883_t *sam = get_safe_token(device);
}
#endif

const UINT8 *sam6883_videoram(device_t *device,int scanline)
{
	sam6883_t *sam = get_safe_token(device);
	const UINT8 *ram_base;
	UINT16 video_position;

	if (scanline != sam->last_scanline)
	{
		/* first scanline? */
		if (scanline == 0)
		{
			/* reset video position */
			sam->video_position = (sam->state & 0x03F8) << 6;
		}
		else
		{
			/* time to advance a row? */
			if ((scanline % sam_video_mode_row_heights[sam->state & 0x0007]) == 0)
				sam->video_position += sam_video_mode_row_pitches[sam->state & 0x0007];
		}
		sam->last_scanline = scanline;
	}

	video_position = sam->video_position;

	if (sam->type != TYPE_SAM6883_GIME)
	{
		/* mask the video position according to the SAM's settings */
		switch(sam->state & 0x6000)
		{
			case 0x0000:	video_position &= 0x0FFF;	break;	/* 4k */
			case 0x2000:	video_position &= 0x3FFF;	break;	/* 16k */
			default:		video_position &= 0xFFFF;	break;	/* 64k/static RAM */
		}
	}

	if (LOG_VIDEO_POSITION)
		logerror("sam_m6847_get_video_ram(): scanline=%d video_position=0x%04X\n", scanline, video_position);

	/* return actual position */
	ram_base = sam->intf->get_rambase ? sam->intf->get_rambase(device) : ram_get_ptr(device->machine().device(RAM_TAG));
	return &ram_base[video_position];
}

UINT8 sam6883_memorysize(device_t *device)
{
	sam6883_t *sam = get_safe_token(device);
	return (sam->state & 0x6000) / 0x2000;
}

UINT8 sam6883_pagemode(device_t *device)
{
	sam6883_t *sam = get_safe_token(device);
	return (sam->state & 0x0400) / 0x0400;
}

UINT8 sam6883_maptype(device_t *device)
{
	sam6883_t *sam = get_safe_token(device);
	return (sam->state & 0x8000) / 0x8000;
}

/* Device Interface */
static void common_start(device_t *device, SAM6883_VERSION device_type)
{
	sam6883_t *sam = get_safe_token(device);
	// validate arguments
	assert(device != NULL);
	assert(device->tag() != NULL);
	assert(device->baseconfig().static_config() != NULL);

	sam->intf = (const sam6883_interface*)device->baseconfig().static_config();

	sam->type = device_type;

	sam->state = 0;
	sam->old_state = ~0;

	/* save state registration */
	state_save_register_item(device->machine(), "6883sam", NULL, 0, sam->state);
	state_save_register_item(device->machine(), "6883sam", NULL, 0, sam->video_position);
	device->machine().state().register_postload(update_sam_postload, (void*)device);
}

static DEVICE_START( sam6883 )
{
	common_start(device, TYPE_SAM6883);
}

static DEVICE_START( sam6883_gime )
{
	common_start(device, TYPE_SAM6883_GIME);
}

static DEVICE_RESET( sam6883 )
{
	sam6883_t *sam = get_safe_token(device);
	sam->state = 0;
	sam->old_state = ~0;
	update_sam(device);
}

DEVICE_GET_INFO( sam6883 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(sam6883_t);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(sam6883);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(sam6883);		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "6883 SAM");						break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "6883 SAM");						break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);							break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");			break;
	}
}

DEVICE_GET_INFO( sam6883_gime )
{
	switch (state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "6883 SAM GIME");				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(sam6883_gime);	break;

		default:										DEVICE_GET_INFO_CALL(sam6883);				break;
	}
}

DEFINE_LEGACY_DEVICE(SAM6883, sam6883);
DEFINE_LEGACY_DEVICE(SAM6883_GIME, sam6883_gime);
