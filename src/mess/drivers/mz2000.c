/***************************************************************************

	Sharp MZ-2000

	skeleton driver

	Basically a simpler version of Sharp MZ-2500

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "machine/i8255a.h"
#include "machine/wd17xx.h"
#include "machine/pit8253.h"
#include "sound/2203intf.h"
#include "sound/beep.h"
#include "machine/rp5c15.h"

//#include "imagedev/cassette.h"
#include "imagedev/flopdrv.h"
#include "imagedev/flopimg.h"
#include "formats/basicdsk.h"



class mz2000_state : public driver_device
{
public:
	mz2000_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 ipl_enable;
	UINT8 tvram_enable;
	UINT8 gvram_enable;
};

static VIDEO_START( mz2000 )
{
}

static SCREEN_UPDATE( mz2000 )
{
	UINT8 *tvram = screen->machine->region("tvram")->base();
	UINT8 *gfx_data = screen->machine->region("chargen")->base();
	int x,y,xi,yi;

	for(y=0;y<25;y++)
	{
		for(x=0;x<40;x++)
		{
			UINT8 tile = tvram[y*40+x];
			UINT8 color = 7;

			for(yi=0;yi<8;yi++)
			{
				for(xi=0;xi<8;xi++)
				{
					int pen;
					int res_x,res_y;

					res_x = x * 8 + xi;
					res_y = y * 8 + yi;

					if(res_x > 640-1 || res_y > 200-1)
						continue;

					pen = ((gfx_data[tile*8+yi] >> (7-xi)) & 1) ? color : 0;

					*BITMAP_ADDR16(bitmap, res_y, res_x) = screen->machine->pens[pen];
				}
			}
		}
	}

    return 0;
}

static READ8_HANDLER( mz2000_ipl_r )
{
	UINT8 *ipl = space->machine->region("ipl")->base();

	return ipl[offset];
}

static READ8_HANDLER( mz2000_wram_r )
{
	UINT8 *wram = space->machine->region("wram")->base();

	return wram[offset];
}

static WRITE8_HANDLER( mz2000_wram_w )
{
	UINT8 *wram = space->machine->region("wram")->base();

	wram[offset] = data;
}

static READ8_HANDLER( mz2000_tvram_r )
{
	UINT8 *tvram = space->machine->region("tvram")->base();

	return tvram[offset];
}

static WRITE8_HANDLER( mz2000_tvram_w )
{
	UINT8 *tvram = space->machine->region("tvram")->base();

	tvram[offset] = data;
}


static READ8_HANDLER( mz2000_mem_r )
{
	mz2000_state *state = space->machine->driver_data<mz2000_state>();
	UINT8 page_mem;

	page_mem = (offset & 0xf000) >> 12;

	if(page_mem == 0 && state->ipl_enable)
		return mz2000_ipl_r(space,offset & 0xfff);

	if(page_mem >= 8)
	{
		if(page_mem == 0xd && state->tvram_enable)
			return mz2000_tvram_r(space,offset & 0xfff);
		else
			return mz2000_wram_r(space,offset);
	}

	return 0xff;
}

static WRITE8_HANDLER( mz2000_mem_w )
{
	mz2000_state *state = space->machine->driver_data<mz2000_state>();
	UINT8 page_mem;

	page_mem = (offset & 0xf000) >> 12;

	if(page_mem >= 8)
	{
		if(page_mem == 0xd && state->tvram_enable)
			mz2000_tvram_w(space,offset & 0xfff,data);
		else
			mz2000_wram_w(space,offset,data);
	}
}



static ADDRESS_MAP_START(mz2000_map, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0xffff ) AM_READWRITE(mz2000_mem_r,mz2000_mem_w)
	// RAM is 0x10000
	// TVRAM is 0x1000
	// GVRAM is 0x4000
ADDRESS_MAP_END

static ADDRESS_MAP_START(mz2000_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
//	AM_RANGE(0xd8, 0xdd) disk i/o identical to Sharp MZ-2500
	AM_RANGE(0xe0, 0xe3) AM_DEVREADWRITE("i8255_0", i8255a_r, i8255a_w)
//	AM_RANGE(0xe4, 0xe7) i8253
	AM_RANGE(0xe8, 0xeb) AM_DEVREADWRITE("z80pio_1", z80pio_ba_cd_r, z80pio_ba_cd_w)
//	AM_RANGE(0xf0, 0xf3) clear 8253, same as Sharp MZ-2500
//	AM_RANGE(0xf4, 0xf7) CRTC
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( mz2000 )
INPUT_PORTS_END


static MACHINE_RESET(mz2000)
{
	mz2000_state *state = machine->driver_data<mz2000_state>();

	state->ipl_enable = 1;
	state->tvram_enable = 0;
	state->gvram_enable = 0;
}


static const gfx_layout mz2000_charlayout =
{
	8, 8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static GFXDECODE_START( mz2000 )
	GFXDECODE_ENTRY( "chargen", 0x0000, mz2000_charlayout, 0, 1 )
GFXDECODE_END

static READ8_DEVICE_HANDLER( mz2000_porta_r )
{
	printf("A R\n");
	return 0xff;
}

static READ8_DEVICE_HANDLER( mz2000_portb_r )
{
	/*
	x--- ---- break key
	---- ---x "blank" control
	*/
	return 0xff;
}

static READ8_DEVICE_HANDLER( mz2000_portc_r )
{
	printf("C R\n");
	return 0xff;
}

static WRITE8_DEVICE_HANDLER( mz2000_porta_w )
{
	/*
	x--- ---- tape "APSS"
	-x-- ---- tape "APLAY"
	--x- ---- tape "AREW"
	---x ---- reverse video
	---- x--- tape stop
	---- -x-- tape play
	---- --x- tape ff
	---- ---x tape rewind
	*/
	//printf("A W %02x\n",data);

	// ...
}

static WRITE8_DEVICE_HANDLER( mz2000_portb_w )
{
	printf("B W %02x\n",data);

	// ...
}

static WRITE8_DEVICE_HANDLER( mz2000_portc_w )
{
	/*
		x--- ---- tape data write
		-x-- ---- tape rec
		--x- ---- tape ?
		---x ---- tape open
		---- x--- 0->1 transition = IPL reset
	    ---- -x-- beeper state
	    ---- --x- 0->1 transition = Work RAM reset
    */
	printf("C W %02x\n",data);

	// ...
}

static I8255A_INTERFACE( ppi8255_intf )
{
	DEVCB_HANDLER(mz2000_porta_r),						/* Port A read */
	DEVCB_HANDLER(mz2000_portb_r),						/* Port B read */
	DEVCB_HANDLER(mz2000_portc_r),						/* Port C read */
	DEVCB_HANDLER(mz2000_porta_w),						/* Port A write */
	DEVCB_HANDLER(mz2000_portb_w),						/* Port B write */
	DEVCB_HANDLER(mz2000_portc_w)						/* Port C write */
};

static WRITE8_DEVICE_HANDLER( mz2000_pio1_porta_w )
{
	mz2000_state *state = device->machine->driver_data<mz2000_state>();
	state->tvram_enable = ((data & 0xc0) == 0xc0);
	state->gvram_enable = ((data & 0xc0) == 0x80);

	//printf("%02x PIO\n",data);
}

static Z80PIO_INTERFACE( mz2000_pio1_intf )
{
	DEVCB_NULL,
	DEVCB_NULL, //HANDLER( mz2500_pio1_porta_r ),
	DEVCB_HANDLER( mz2000_pio1_porta_w ),
	DEVCB_NULL,
	DEVCB_NULL, //HANDLER( mz2500_pio1_porta_r ),
	DEVCB_NULL,
	DEVCB_NULL
};

static MACHINE_CONFIG_START( mz2000, mz2000_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)
	MCFG_CPU_PROGRAM_MAP(mz2000_map)
	MCFG_CPU_IO_MAP(mz2000_io)

	MCFG_MACHINE_RESET(mz2000)

	MCFG_I8255A_ADD( "i8255_0", ppi8255_intf )
	MCFG_Z80PIO_ADD( "z80pio_1", 4000000, mz2000_pio1_intf )

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 320-1, 0, 200-1)

	MCFG_GFXDECODE(mz2000)
	MCFG_PALETTE_LENGTH(8)
//	MCFG_PALETTE_INIT(black_and_white)

	MCFG_VIDEO_START(mz2000)
	MCFG_SCREEN_UPDATE(mz2000)

MACHINE_CONFIG_END




ROM_START( mz2000 )
	ROM_REGION( 0x1000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "mz20ipl.bin",0x0000, 0x0800, CRC(d7ccf37f) SHA1(692814ffc2cf50fa8bf9e30c96ebe4a9ee536a86))

	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )

	ROM_REGION( 0x10000, "wram", ROMREGION_ERASE00 )

	ROM_REGION( 0x1000, "tvram", ROMREGION_ERASE00 )

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "mzfont.rom", 0x0000, 0x0800, CRC(0631efc3) SHA1(99b206af5c9845995733d877e9e93e9681b982a8) )
ROM_END



/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE     INPUT    INIT    COMPANY           FULLNAME       FLAGS */
COMP( 1982, mz2000,   mz80b,    0,   mz2000,   mz2000,  0, "Sharp",   "MZ-2000", GAME_NOT_WORKING | GAME_NO_SOUND )
