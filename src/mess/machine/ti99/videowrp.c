/*
    TI-99/4(A) and /8 Video subsystem
    This device actually wraps the naked video chip implementation

    EVPC (Enhanced Video Processor Card) from SNUG
    based on v9938 (may also be equipped with v9958)
    Can be used with TI-99/4A as an add-on card; internal VDP must be removed

    The SGCPU ("TI-99/4P") only runs with EVPC.

    Michael Zapf, October 2010
*/

#include "emu.h"
#include "videowrp.h"

typedef struct _ti99_video_state
{
	address_space	*space;
	device_t	*cpu;
	int				chip;
} ti99_video_state;

INLINE ti99_video_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == TIVIDEO);

	return (ti99_video_state *)downcast<legacy_device_base *>(device)->token();
}

INLINE const ti99_video_config *get_config(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == TIVIDEO);

	return (const ti99_video_config *) downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
}


/*****************************************************************************/

/*
    Memory read (TI-99/4(A))
*/
READ16_DEVICE_HANDLER( ti_tms991x_r16 )
{
	ti99_video_state *video = get_safe_token(device);
//  device_adjust_icount(video->cpu, -4);

	if (offset & 1)
	{	/* read VDP status */
		return ((int) TMS9928A_register_r(video->space, 0)) << 8;
	}
	else
	{	/* read VDP RAM */
		return ((int) TMS9928A_vram_r(video->space, 0)) << 8;
	}
}

/*
    Memory read (TI-99/8). Makes use of the Z memory handler.
*/
READ8Z_DEVICE_HANDLER( ti8_tms991x_rz )
{
	ti99_video_state *video = get_safe_token(device);
//  device_adjust_icount(video->cpu, -4);

	if (offset & 2)
	{	/* read VDP status */
		*value = TMS9928A_register_r(video->space, 0);
	}
	else
	{	/* read VDP RAM */
		*value = TMS9928A_vram_r(video->space, 0);
	}
}

/*
    Memory read (EVPC)
*/
READ16_DEVICE_HANDLER( ti_v9938_r16 )
{
	ti99_video_state *video = get_safe_token(device);
//  device_adjust_icount(video->cpu, -4);

	if (offset & 1)
	{	/* read VDP status */
		return ((int) v9938_0_status_r(video->space, 0)) << 8;
	}
	else
	{	/* read VDP RAM */
		return ((int) v9938_0_vram_r(video->space, 0)) << 8;
	}
}

/*
    Memory write
*/
WRITE16_DEVICE_HANDLER( ti_tms991x_w16 )
{
	ti99_video_state *video = get_safe_token(device);
//  device_adjust_icount(video->cpu, -4);

	if (offset & 1)
	{	/* write VDP address */
		TMS9928A_register_w(video->space, 0, (data >> 8) & 0xff);
	}
	else
	{	/* write VDP data */
		TMS9928A_vram_w(video->space, 0, (data >> 8) & 0xff);
	}
}

/*
    Memory write (TI-99/8)
*/
WRITE8_DEVICE_HANDLER( ti8_tms991x_w )
{
	ti99_video_state *video = get_safe_token(device);
//  device_adjust_icount(video->cpu, -4);

	if (offset & 2)
	{	/* write VDP address */
		TMS9928A_register_w(video->space, 0, data);
	}
	else
	{	/* write VDP data */
		TMS9928A_vram_w(video->space, 0, data);
	}
}

/*
    Memory write (EVPC)
*/
WRITE16_DEVICE_HANDLER ( ti_v9938_w16 )
{
	ti99_video_state *video = get_safe_token(device);
//  device_adjust_icount(video->cpu, -4);

	switch (offset & 3)
	{
	case 0:
		/* write VDP data */
		v9938_0_vram_w(video->space, 0, (data >> 8) & 0xff);
		break;
	case 1:
		/* write VDP address */
		v9938_0_command_w(video->space, 0, (data >> 8) & 0xff);
		break;
	case 2:
		/* write VDP palette */
		v9938_0_palette_w(video->space, 0, (data >> 8) & 0xff);
		break;
	case 3:
		/* write VDP register pointer (indirect access) */
		v9938_0_register_w(video->space, 0, (data >> 8) & 0xff);
		break;
	}
}

/*
    Video write (Geneve)
*/
WRITE8_DEVICE_HANDLER ( gen_v9938_w )
{
	ti99_video_state *video = get_safe_token(device);
//  device_adjust_icount(video->cpu, -4);

	switch (offset & 6)
	{
	case 0:
		/* write VDP data */
		v9938_0_vram_w(video->space, 0, data);
		break;
	case 2:
		/* write VDP address */
		v9938_0_command_w(video->space, 0, data);
		break;
	case 4:
		/* write VDP palette */
		v9938_0_palette_w(video->space, 0, data);
		break;
	case 6:
		/* write VDP register pointer (indirect access) */
		v9938_0_register_w(video->space, 0, data);
		break;
	}
}

/*
    Video read (Geneve).
*/
READ8Z_DEVICE_HANDLER( gen_v9938_rz )
{
	ti99_video_state *video = get_safe_token(device);
//  device_adjust_icount(video->cpu, -4);

	if (offset & 2)
	{	/* read VDP status */
		*value = v9938_0_status_r(video->space, 0);
	}
	else
	{	/* read VDP RAM */
		*value = v9938_0_vram_r(video->space, 0);
	}
}

READ16_DEVICE_HANDLER ( ti_video_rnop )
{
	ti99_video_state *video = get_safe_token(device);
	device_adjust_icount(video->cpu, -4);
	return 0;
}

WRITE16_DEVICE_HANDLER ( ti_video_wnop )
{
	ti99_video_state *video = get_safe_token(device);
	device_adjust_icount(video->cpu, -4);
}
/**************************************************************************/
// Interfacing to mouse attached to v9938

void video_update_mouse( device_t *device, int delta_x, int delta_y, int buttons)
{
	ti99_video_state *video = get_safe_token(device);
	// TODO: V9938 to be devicified
	if (video->chip==TI_V9938)
		v9938_update_mouse_state(0, delta_x, delta_y, buttons & 3);
}


/**************************************************************************/

static DEVICE_START( ti99_video )
{
	ti99_video_state *video = get_safe_token(device);
	const ti99_video_config* conf = (const ti99_video_config*)get_config(device);

	video->cpu = device->machine().device("maincpu");
	video->space = device->machine().device("maincpu")->memory().space(AS_PROGRAM);

	if (conf->chip==TI_TMS991X)
	{
		TMS9928A_configure(conf->tmsparam);
	}
	else
	{
		running_machine &machine = device->machine();
		VIDEO_START_CALL(generic_bitmapped);
	}
}

static DEVICE_STOP( ti99_video )
{
}

static DEVICE_RESET( ti99_video )
{
	const ti99_video_config* conf = (const ti99_video_config*)get_config(device);
	if (conf->chip==TI_TMS991X)
	{
		TMS9928A_reset();
	}
	else
	{
		running_machine &machine = device->machine();
		int memsize = (input_port_read(machine, "V9938-MEM")==0)? 0x20000 : 0x30000;

		v9938_init(machine, 0, *machine.primary_screen, machine.generic.tmpbitmap,
			MODEL_V9938, memsize, conf->callback);
		v9938_reset(0);
	}
}

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##ti99_video##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET | DT_HAS_INLINE_CONFIG
#define DEVTEMPLATE_NAME                "TI-99/x Video subsystem"
#define DEVTEMPLATE_FAMILY              "Internal device"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( TIVIDEO, ti99_video );

