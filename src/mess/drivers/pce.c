/****************************************************************************

 PC-Engine / Turbo Grafx 16 driver
 by Charles Mac Donald
 E-Mail: cgfm2@hooked.net

 Thanks to David Shadoff and Brian McPhail for help with the driver.

****************************************************************************/

/**********************************************************************
          To-Do List:
- convert h6280-based drivers to internal memory map for the I/O region
- test sprite collision and overflow interrupts
- sprite precaching
- fix RCR interrupt
- rewrite the base renderer loop
- Add CD support
- SuperGrafix Driver
- Banking for SF2 (and others?)
- Add 6 button joystick support
- Add 263 line mode
- Sprite DMA should use vdc VRAM functions
**********************************************************************/

/**********************************************************************
                          Known Bugs
***********************************************************************
- Afterburner 2 crashes: RTI to BRK, BRK vector -> BRK
- TV Sports games freeze.
- Adventure Island black screens: v-blank as pulse, not assert?
- Street Fighter 2: missing letter on title screen
- Deep Blue: missing graphics
- Aero Blasters: no title screen
- Darius Plus: locks up because of PC wrapping?
- Cyber Knight: bad graphics
- Rastan Saga 2: bad graphics
- Ankuku Densetsu: graphics flake out during intro
- Racing Damashii: windows lose borders, track misaligned
- Violent Soldier: corruption on title screen
**********************************************************************/

#include <assert.h>

#include "driver.h"
#include "video/generic.h"
#include "video/vdc.h"
#include "cpu/h6280/h6280.h"
#include "includes/pce.h"
#include "devices/cartslot.h"
#include "sound/c6280.h"
#include "hash.h"

static INTERRUPT_GEN( pce_interrupt )
{
    int ret = 0;

    /* bump current scanline */ 
    vdc.curline = (vdc.curline + 1) % VDC_LPF;
	vdc.current_segment_line++;

	if(vdc.curline==0)
	{
		//state is now in the VDS region, nothing even tries to draw here.
		vdc.top_blanking=(vdc.vdc_data[VPR].b.l&0x1F)+1;
		vdc.top_overscan=(vdc.vdc_data[VPR].b.h)+2;
		vdc.current_bitmap_line=0;
		vdc.current_segment=STATE_TOPBLANK;
		vdc.current_segment_line=0;
		vdc.active_lines = vdc.physical_height = 1 + ( vdc.vdc_data[VDW].w & 0x01FF );
		vdc.bottomfill=vdc.vdc_data[VCR].b.l;
	}

	switch(vdc.current_segment)
	{
	case STATE_TOPBLANK:
		{
			if(vdc.current_segment_line == vdc.top_blanking)
			{
				vdc.current_segment=STATE_TOPFILL;
				vdc.current_segment_line=0;
			}
			break;
		}
	case STATE_TOPFILL:
		{
			if(vdc.current_segment_line == vdc.top_overscan)
			{
				vdc.current_segment=STATE_ACTIVE;
				vdc.current_segment_line=0;
				vdc.y_scroll = vdc.vdc_data[BYR].w;
			}
			break;
		}
	case STATE_ACTIVE:
		{
			if(vdc.current_segment_line == vdc.active_lines)
			{
				vdc.current_segment_line=0;
				if(vdc.bottomfill != 0)
				{
					vdc.current_segment=STATE_BOTTOMFILL;
					vdc.status |= VDC_VD;
					if(vdc.vdc_data[CR].w & CR_VR)
					{
						
						ret = 1;
					}				
					
					/* do VRAM > SATB DMA if the enable bit is set or the DVSSR reg. was written to */ 
					if((vdc.vdc_data[DCR].w & DCR_DSR) || vdc.dvssr_write)
					{
						if(vdc.dvssr_write) vdc.dvssr_write = 0;
						memcpy(&vdc.sprite_ram, &vdc.vram[vdc.vdc_data[DVSSR].w<<1], 512);
						vdc.status |= VDC_DS;   /* set satb done flag */ 
						
						/* generate interrupt if needed */ 
						if(vdc.vdc_data[DCR].w & DCR_DSC)
							ret = 1;
					}				
					
				}
				else vdc.current_segment=STATE_TOPBLANK;
			}
			break;
		}
	case STATE_BOTTOMFILL:
		{
			if(vdc.current_segment_line == vdc.bottomfill)
			{
				vdc.current_segment_line=0;
				vdc.current_segment=STATE_TOPBLANK;
			}
			break;
		}

	default:break;
	}

    /* draw a line of the display */ 
    if(vdc.curline >= FIRST_VISIBLE && vdc.curline < (LAST_VISIBLE))
    {
        	switch(vdc.current_segment)
			{
			default:
				draw_black_line(vdc.current_bitmap_line);
				break;
			case STATE_TOPFILL:
			case STATE_BOTTOMFILL:
				draw_overscan_line(vdc.current_bitmap_line);
				break;
			case STATE_ACTIVE:
				pce_refresh_line(vdc.current_bitmap_line, vdc.current_segment_line);
				break;
			}
			vdc.current_bitmap_line++;
    }
	else
    {
		//rendering can take place here, but it's never on screen!
        	switch(vdc.current_segment)
			{
			default:
				draw_black_line(243);
				break;
			case STATE_TOPFILL:
			case STATE_BOTTOMFILL:
				draw_overscan_line(243);
				break;
			case STATE_ACTIVE:
				pce_refresh_line(243, vdc.current_segment_line);
				break;
			}
    }


    /* generate interrupt on line compare if necessary */
    if(vdc.vdc_data[CR].w & CR_RC)
		if(vdc.curline == (((vdc.vdc_data[RCR].w-64)+vdc.top_blanking+vdc.top_overscan)%263))
    {
        vdc.status |= VDC_RR;
        ret = 1;
    }

    /* handle frame events */ 
    if(vdc.curline == 261 && vdc.current_segment != STATE_BOTTOMFILL)
    {
        vdc.status |= VDC_VD;   /* set vblank flag */ 

        /* do VRAM > SATB DMA if the enable bit is set or the DVSSR reg. was written to */ 
        if((vdc.vdc_data[DCR].w & DCR_DSR) || vdc.dvssr_write)
        {
            if(vdc.dvssr_write) vdc.dvssr_write = 0;
#ifdef MAME_DEBUG
			assert(((vdc.vdc_data[DVSSR].w<<1) + 512) <= 0x10000);
#endif
            memcpy(&vdc.sprite_ram, &vdc.vram[vdc.vdc_data[DVSSR].w<<1], 512);
            vdc.status |= VDC_DS;   /* set satb done flag */ 

            /* generate interrupt if needed */ 
            if(vdc.vdc_data[DCR].w & DCR_DSC)
            {
                ret = 1;
            }
        }

        if(vdc.vdc_data[CR].w & CR_VR)  /* generate IRQ1 if enabled */ 
        {
            ret = 1;
        }
    }
	if (ret)
		cpunum_set_input_line(0, 0, HOLD_LINE);
}

ADDRESS_MAP_START( pce_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x000000, 0x1EDFFF) AM_ROM
	AM_RANGE( 0x1EE000, 0x1EFFFF) AM_RAM
	AM_RANGE( 0x1F0000, 0x1F1FFF) AM_RAM AM_MIRROR(0x6000) AM_BASE( &pce_user_ram )
	AM_RANGE( 0x1FE000, 0x1FE3FF) AM_READWRITE( vdc_r, vdc_w )
	AM_RANGE( 0x1FE400, 0x1FE7FF) AM_READWRITE( vce_r, vce_w )
	AM_RANGE( 0x1FE800, 0x1FEBFF) AM_READWRITE( C6280_r, C6280_0_w )
	AM_RANGE( 0x1FEC00, 0x1FEFFF) AM_READWRITE( H6280_timer_r, H6280_timer_w )
	AM_RANGE( 0x1FF000, 0x1FF3FF) AM_READWRITE( pce_joystick_r, pce_joystick_w )
	AM_RANGE( 0x1FF400, 0x1FF7FF) AM_READWRITE( H6280_irq_status_r, H6280_irq_status_w )
ADDRESS_MAP_END

ADDRESS_MAP_START( pce_io , ADDRESS_SPACE_IO, 8)
	AM_RANGE( 0x00, 0x03) AM_READWRITE( vdc_r, vdc_w )
ADDRESS_MAP_END

/* todo: alternate forms of input (multitap, mouse, etc.) */
INPUT_PORTS_START( pce )

    PORT_START  /* Player 1 controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) /* button I */
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) /* button II */
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON4 ) /* select */
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 ) /* run */
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
#if 0
    PORT_START  /* Fake dipswitches for system config */
    PORT_DIPNAME( 0x01, 0x01, "Console type")
    PORT_DIPSETTING(    0x00, "Turbo-Grafx 16")
    PORT_DIPSETTING(    0x01, "PC-Engine")

    PORT_DIPNAME( 0x01, 0x01, "Joystick type")
    PORT_DIPSETTING(    0x00, "2 Button")
    PORT_DIPSETTING(    0x01, "6 Button")
#endif
INPUT_PORTS_END


#if 0
static gfx_layout pce_bg_layout =
{
        8, 8,
        2048,
        4,
        {0x00*8, 0x01*8, 0x10*8, 0x11*8 },
        {0, 1, 2, 3, 4, 5, 6, 7 },
        { 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
        32*8,
};

static gfx_layout pce_obj_layout =
{
        16, 16,
        512,
        4,
        {0x00*8, 0x20*8, 0x40*8, 0x60*8},
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
        { 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16, 8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
        128*8,
};

static gfx_decode pce_gfxdecodeinfo[] =
{
   { 1, 0x0000, &pce_bg_layout, 0, 0x10 },
   { 1, 0x0000, &pce_obj_layout, 0x100, 0x10 },
	{-1}
};
#endif


static MACHINE_DRIVER_START( pce )
	/* basic machine hardware */
	MDRV_CPU_ADD(H6280, 7195090)
	MDRV_CPU_PROGRAM_MAP(pce_mem, 0)
	MDRV_CPU_IO_MAP(pce_io, 0)
	MDRV_CPU_VBLANK_INT(pce_interrupt, VDC_LPF)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(45*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 45*8-1, 0*8, 32*8-1)
	/* MDRV_GFXDECODE( pce_gfxdecodeinfo ) */
	MDRV_PALETTE_LENGTH(512)
	MDRV_COLORTABLE_LENGTH(512)

	MDRV_VIDEO_START( pce )
	MDRV_VIDEO_UPDATE( pce )

	MDRV_NVRAM_HANDLER( pce )
	MDRV_SPEAKER_STANDARD_STEREO("left","right")
	MDRV_SOUND_ADD(C6280, 21477270/6)
	MDRV_SOUND_ROUTE(0, "left", 1.00)
	MDRV_SOUND_ROUTE(1, "right", 1.00)
MACHINE_DRIVER_END

static void pce_partialhash(char *dest, const unsigned char *data,
        unsigned long length, unsigned int functions)
{
        if ( ( length <= PCE_HEADER_SIZE ) || ( length & PCE_HEADER_SIZE ) ) {
	        hash_compute(dest, &data[PCE_HEADER_SIZE], length - PCE_HEADER_SIZE, functions);
	} else {
		hash_compute(dest, data, length, functions);
	}
}

static void pce_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;
		case DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_pce_cart; break;
		case DEVINFO_PTR_PARTIAL_HASH:					info->partialhash = pce_partialhash; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "pce"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(pce)
	CONFIG_DEVICE(pce_cartslot_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

#define rom_pce NULL
#define rom_tg16 NULL

/*	  YEAR  NAME    PARENT	COMPAT	MACHINE	INPUT	 INIT	CONFIG  COMPANY	 FULLNAME */
CONS( 1987, pce,    0,      0,      pce,    pce,     pce,   pce,	"Nippon Electronic Company", "PC Engine", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
CONS( 1989, tg16,   pce,    0,      pce,    pce,     tg16,  pce,	"Nippon Electronic Company", "TurboGrafx 16", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )

