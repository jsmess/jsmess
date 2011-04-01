/***************************************************************************

    ACT Apricot F1 series

    preliminary driver by Angelo Salese

****************************************************************************/

#include "emu.h"
#include "cpu/i86/i86.h"
#include "machine/wd17xx.h"
#include "imagedev/flopdrv.h"
#include "cpu/z80/z80daisy.h"
#include "machine/z80ctc.h"
#include "imagedev/flopdrv.h"
#include "formats/basicdsk.h"

class act_state : public driver_device
{
public:
	act_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT16 *m_paletteram;
	UINT16 *m_vram;
	UINT16 *m_scrollram;

	UINT8 m_fdrv_num;
};

static VIDEO_START( act_f1 )
{
}

static SCREEN_UPDATE( act_f1 )
{
	act_state *state = screen->machine().driver_data<act_state>();
	int x,y,i;
	int x_count;

	{
		for(y=0;y<256;y++)
		{
			x_count = 0;
			for(x=0;x<640;x+=8)
			{
				int pen[2],color;

				for (i=0;i<8;i++)
				{
					pen[0] = (state->m_vram[state->m_scrollram[y]+x_count])>>(7-i) & 1;
					pen[1] = (state->m_vram[state->m_scrollram[y]+x_count])>>(15-i) & 1;

					color = pen[0]|pen[1]<<1;

					const rectangle &visarea = screen->visible_area();
					if((x+i)<=visarea.max_x && ((y)+0)<visarea.max_y)
						*BITMAP_ADDR16(bitmap, y, x+i) = screen->machine().pens[color];
				}

				x_count++;
			}
		}
	}

	return 0;
}

//static UINT8 fdc_irq_flag;
//static UINT8 fdc_drq_flag;
//static UINT8 fdc_side;
//static UINT8 fdc_drive;

static READ8_HANDLER( act_fdc_r )
{
	act_state *state = space->machine().driver_data<act_state>();
	device_t* dev = space->machine().device("fdc");

//  printf("%02x\n",offset);

	floppy_mon_w(floppy_get_device(space->machine(), state->m_fdrv_num), CLEAR_LINE);
	floppy_drive_set_ready_state(floppy_get_device(space->machine(), state->m_fdrv_num), 1,0);

	switch(offset)
	{
		case 0:
			return wd17xx_status_r(dev,offset);
		case 1:
			return wd17xx_track_r(dev,offset);
		case 2:
			return wd17xx_sector_r(dev,offset);
		case 3:
			return wd17xx_data_r(dev,offset);
		default:
			logerror("FDC: read from %04x\n",offset);
			return 0xff;
	}

	return 0x00;
}

static WRITE8_HANDLER( act_fdc_w )
{
	act_state *state = space->machine().driver_data<act_state>();
	device_t* dev = space->machine().device("fdc");

//  printf("%02x %02x\n",offset,data);

	floppy_mon_w(floppy_get_device(space->machine(), state->m_fdrv_num), CLEAR_LINE);
	floppy_drive_set_ready_state(floppy_get_device(space->machine(), state->m_fdrv_num), 1,0);

	switch(offset)
	{
		case 0:
			wd17xx_command_w(dev,offset,data);
			break;
		case 1:
			wd17xx_track_w(dev,offset,data);
			break;
		case 2:
			wd17xx_sector_w(dev,offset,data);
			break;
		case 3:
			wd17xx_data_w(dev,offset,data);
			break;
		default:
			logerror("FDC: write to %04x = %02x\n",offset,data);
			break;
	}
}

static READ16_HANDLER( act_pal_r )
{
	act_state *state = space->machine().driver_data<act_state>();

	return state->m_paletteram[offset];
}

static WRITE16_HANDLER( act_pal_w )
{
	act_state *state = space->machine().driver_data<act_state>();
	UINT8 i,r,g,b;
	COMBINE_DATA(&state->m_paletteram[offset]);

	if(ACCESSING_BITS_0_7 && offset) //TODO: offset 0 looks bogus
	{
		i = state->m_paletteram[offset] & 1;
		r = ((state->m_paletteram[offset] & 2)>>0) | i;
		g = ((state->m_paletteram[offset] & 4)>>1) | i;
		b = ((state->m_paletteram[offset] & 8)>>2) | i;

		palette_set_color_rgb(space->machine(), offset, pal2bit(r), pal2bit(g), pal2bit(b));
	}
}

static ADDRESS_MAP_START(act_f1_mem, AS_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x01e00,0x01fff) AM_RAM AM_BASE_MEMBER(act_state,m_scrollram)
	AM_RANGE(0xe0000,0xe001f) AM_READWRITE(act_pal_r,act_pal_w) AM_BASE_MEMBER(act_state,m_paletteram)
	AM_RANGE(0x00000,0xeffff) AM_RAM AM_BASE_MEMBER(act_state,m_vram)
	AM_RANGE(0xf0000,0xf7fff) AM_RAM
	AM_RANGE(0xf8000,0xfffff) AM_ROM
ADDRESS_MAP_END

static WRITE8_HANDLER( actf1_sys_w )
{
//  static UINT8 cur_fdrv;
//  device_t* dev = space->machine().device("fdc");

	switch(offset)
	{
		case 0:
//          cur_fdrv = ~data & 1;
//          wd17xx_set_drive(dev,cur_fdrv);
			break;
		case 1:
//          wd17xx_set_side(dev,data ? 1 : 0);
			break;
		case 2:
//          floppy_drive_set_motor_state(floppy_get_device(space->machine(), cur_fdrv), data);
//          floppy_drive_set_ready_state(floppy_get_device(space->machine(), cur_fdrv), data,0);
			break;
		case 3:
//          data ? 256 : 200 line mode
//          data ? 50 : 60 Hz
			break;
		case 4:
//          data ? 80 : 40 columns mode
//          data ? 14 Mhz : 7 Mhz Pixel clock
			break;
		case 5:
//          caps lock LED
			break;
		case 6:
//          stop LED
			break;
	}
}

static ADDRESS_MAP_START( act_f1_io , AS_IO, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x000f) AM_WRITE8(actf1_sys_w,0x00ff)
	AM_RANGE(0x0010, 0x0017) AM_DEVREADWRITE8("ctc",z80ctc_r,z80ctc_w,0x00ff)
//  AM_RANGE(0x0020, 0x0027) z80 sio (!)
//  AM_RANGE(0x0030, 0x0031) AM_WRITE8(ctc_ack_w,0x00ff)
	AM_RANGE(0x0040, 0x0047) AM_READWRITE8(act_fdc_r, act_fdc_w,0x00ff)
//  AM_RANGE(0x01e0, 0x01ff) winchester
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( act )
INPUT_PORTS_END


static MACHINE_RESET(act)
{
}


static const gfx_layout charset_8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static GFXDECODE_START( act_f1 )
	GFXDECODE_ENTRY( "gfx",   0x00000, charset_8x8,    0x000, 1 )
GFXDECODE_END

static Z80CTC_INTERFACE( ctc_intf )
{
	0,					// timer disables
	DEVCB_NULL,		// interrupt handler
	DEVCB_NULL,		// ZC/TO0 callback
	DEVCB_NULL,		// ZC/TO1 callback
	DEVCB_NULL,		// ZC/TO2 callback
};

#if 0

static const z80sio_interface sio_intf =
{
	0,					/* interrupt handler */
	0,					/* DTR changed handler */
	0,					/* RTS changed handler */
	0,					/* BREAK changed handler */
	0,					/* transmit handler */
	0					/* receive handler */
};
#endif

#if 0
static const z80_daisy_config x1_daisy[] =
{
	{ "ctc" },
	{ NULL }
};
#endif

static INTERRUPT_GEN( act_f1_irq )
{
	//if(input_code_pressed(device->machine(), KEYCODE_C))
	//  device_set_input_line_and_vector(device,0,HOLD_LINE,0x60);
}

static FLOPPY_OPTIONS_START( act )
	FLOPPY_OPTION( img2hd, "dsk", "2HD disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static const floppy_config act_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(act),
	NULL
};

static MACHINE_CONFIG_START( act_f1, act_state )

	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", I8086, 4670000)
	MCFG_CPU_PROGRAM_MAP(act_f1_mem)
	MCFG_CPU_IO_MAP(act_f1_io)
	MCFG_CPU_VBLANK_INT("screen",act_f1_irq )
//  MCFG_CPU_CONFIG(x1_daisy)

	MCFG_Z80CTC_ADD( "ctc", 4670000 , ctc_intf )

	MCFG_MACHINE_RESET(act)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 256)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 256-1)
	MCFG_SCREEN_UPDATE(act_f1)

	MCFG_PALETTE_LENGTH(16)
//  MCFG_PALETTE_INIT(black_and_white)

	MCFG_WD2793_ADD("fdc", default_wd17xx_interface_2_drives )

	MCFG_GFXDECODE(act_f1)

	MCFG_VIDEO_START(act_f1)

	MCFG_FLOPPY_2_DRIVES_ADD(act_floppy_config)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( aprif1 )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "lo_f1_1.6.8f",  0xf8000, 0x4000, CRC(be018be2) SHA1(80b97f5b2111daf112c69b3f58d1541a4ba69da0) )	// Labelled F1 - LO Vr. 1.6
	ROM_LOAD16_BYTE( "hi_f1_1.6.10f", 0xf8001, 0x4000, CRC(bbba77e2) SHA1(e62bed409eb3198f4848f85fccd171cd0745c7c0) )	// Labelled F1 - HI Vr. 1.6

	ROM_REGION( 0x00800, "gfx", ROMREGION_ERASEFF )
	ROM_COPY( "maincpu", 0xf8800, 0x00000, 0x00800 )
ROM_END

ROM_START( aprif10 )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "lo_f10_3.1.1.8f",  0xf8000, 0x4000, CRC(bfd46ada) SHA1(0a36ef379fa9af7af9744b40c167ce6e12093485) )	// Labelled LO-FRange Vr3.1.1
	ROM_LOAD16_BYTE( "hi_f10_3.1.1.10f", 0xf8001, 0x4000, CRC(67ad5b3a) SHA1(a5ececb87476a30167cf2a4eb35c03aeb6766601) )	// Labelled HI-FRange Vr3.1.1

	ROM_REGION( 0x00800, "gfx", ROMREGION_ERASEFF )
	ROM_COPY( "maincpu", 0xf8800, 0x00000, 0x00800 )
ROM_END

ROM_START( aprifp )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "lo_fp_3.1.ic20", 0xf8000, 0x4000, CRC(0572add2) SHA1(c7ab0e5ced477802e37f9232b5673f276b8f5623) )	// Labelled 11212721 F97E PORT LO VR 3.1
	ROM_LOAD16_BYTE( "hi_fp_3.1.ic9",  0xf8001, 0x4000, CRC(3903674b) SHA1(8418682dcc0c52416d7d851760fea44a3cf2f914) )	// Labelled 11212721 BD2D PORT HI VR 3.1

	ROM_REGION( 0x00800, "gfx", ROMREGION_ERASEFF )
	ROM_COPY( "maincpu", 0xf8800, 0x00000, 0x00800 )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT MACHINE INPUT   INIT   COMPANY  FULLNAME                 FLAGS */
COMP( 1984, aprif1,    0,    0,     act_f1,    act,    0, "ACT",   "Apricot F1",            GAME_NOT_WORKING | GAME_NO_SOUND )
COMP( 1985, aprif10,   0,    0,     act_f1,    act,    0, "ACT",   "Apricot F10",           GAME_NOT_WORKING | GAME_NO_SOUND )
COMP( 1984, aprifp,    0,    0,     act_f1,    act,    0, "ACT",   "Apricot Portable / FP", GAME_NOT_WORKING | GAME_NO_SOUND )
