/***************************************************************************

    ACT Apricot F1 series

    preliminary driver by Angelo Salese


11/09/2011 - modernised. The portable doesn't seem to have
             scroll registers, and it sets the palette to black.
             I've added a temporary video output so that you can get
             an idea of what the screen should look like. [Robbbert]

****************************************************************************/

/*

	TODO:
	
	- CTC/SIO interrupt acknowledge
	- CTC clocks
	- sound
	- portable has wrong devices

*/

#include "includes/apricotp.h"



//**************************************************************************
//  VIDEO
//**************************************************************************

bool fp_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	m_crtc->update(&bitmap, &cliprect);

	return false;
}


static const gfx_layout charset_8x8 =
{
	8,8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static GFXDECODE_START( act_f1 )
	GFXDECODE_ENTRY( I8086_TAG, 0x0800, charset_8x8, 0, 1 )
GFXDECODE_END



//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( fp_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( fp_mem, AS_PROGRAM, 16, fp_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0xeffff) AM_RAM// AM_BASE(m_p_videoram)
	AM_RANGE(0xf0000, 0xf7fff) AM_RAM
	AM_RANGE(0xf8000, 0xfffff) AM_ROM AM_REGION(I8086_TAG, 0)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( fp_io )
//-------------------------------------------------

static ADDRESS_MAP_START( fp_io, AS_IO, 16, fp_state )
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( sound_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( sound_mem, AS_PROGRAM, 8, fp_state )
	AM_RANGE(0xf000, 0xffff) AM_ROM AM_REGION(HD63B01V1_TAG, 0)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( sound_io )
//-------------------------------------------------

static ADDRESS_MAP_START( sound_io, AS_IO, 8, fp_state )
	AM_RANGE(M6801_PORT1, M6801_PORT1)
	AM_RANGE(M6801_PORT2, M6801_PORT2)
	AM_RANGE(M6801_PORT3, M6801_PORT3)
	AM_RANGE(M6801_PORT4, M6801_PORT4)
ADDRESS_MAP_END



//**************************************************************************
//  INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_PORTS( act )
//-------------------------------------------------

static INPUT_PORTS_START( fp )
	// defined in machine/apricotkb.c
INPUT_PORTS_END



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  APRICOT_KEYBOARD_INTERFACE( kb_intf )
//-------------------------------------------------

static APRICOT_KEYBOARD_INTERFACE( kb_intf )
{
	DEVCB_NULL
};


//-------------------------------------------------
//  pic8259_interface pic_intf
//-------------------------------------------------

static IRQ_CALLBACK( fp_irq_callback )
{
	fp_state *state = device->machine().driver_data<fp_state>();

	return pic8259_acknowledge(state->m_pic);
}

/*

	INT0	TIMER
	INT1	FDC
	INT2	6301
	INT3	COMS
	INT4	USART
	INT5	COMS
	INT6	PRINT
	INT7	EOP

*/

static const struct pic8259_interface pic_intf =
{
	DEVCB_CPU_INPUT_LINE(I8086_TAG, INPUT_LINE_IRQ0),
	DEVCB_LINE_VCC,
	DEVCB_NULL
};


//-------------------------------------------------
//  I8237_INTERFACE( dmac_intf )
//-------------------------------------------------

static I8237_INTERFACE( dmac_intf )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	{ DEVCB_NULL, DEVCB_DEVICE_HANDLER(WD2797_TAG, wd17xx_data_r), DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_NULL, DEVCB_DEVICE_HANDLER(WD2797_TAG, wd17xx_data_w), DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL }
};


//-------------------------------------------------
//  Z80DART_INTERFACE( sio_intf )
//-------------------------------------------------

static Z80DART_INTERFACE( sio_intf )
{
	0, 0, 0, 0,

	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_DEVICE_LINE(I8259A_TAG, pic8259_ir4_w)
};


//-------------------------------------------------
//  wd17xx_interface fdc_intf
//-------------------------------------------------

static LEGACY_FLOPPY_OPTIONS_START( act )
	LEGACY_FLOPPY_OPTION( img2hd, "dsk", "2HD disk image", basicdsk_identify_default, basicdsk_construct_default, NULL,
		HEADS([2])
		TRACKS([80])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
LEGACY_FLOPPY_OPTIONS_END

static const floppy_interface act_floppy_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_3_5_DSDD, // Sony OA-D32W
	LEGACY_FLOPPY_OPTIONS_NAME(act),
	"floppy_3_5",
	NULL
};

static const wd17xx_interface fdc_intf =
{
	DEVCB_LINE_GND,
	DEVCB_DEVICE_LINE(I8259A_TAG, pic8259_ir1_w),
	DEVCB_DEVICE_LINE(I8237_TAG, i8237_dreq1_w),
	{ FLOPPY_0, NULL, NULL, NULL }
};


//-------------------------------------------------
//  centronics_interface centronics_intf
//-------------------------------------------------

static const centronics_interface centronics_intf =
{
	0,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  mc6845_interface crtc_intf
//-------------------------------------------------

static MC6845_UPDATE_ROW( fp_update_row )
{
}

static const mc6845_interface crtc_intf =
{
	SCREEN_TAG,
	8,
	NULL,
	fp_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};



//**************************************************************************
//  MACHINE INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_START( pc1512 )
//-------------------------------------------------

void fp_state::machine_start()
{
	// register CPU IRQ callback
	device_set_irq_callback(m_maincpu, fp_irq_callback);
}



//**************************************************************************
//  MACHINE DRIVERS
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( fp )
//-------------------------------------------------

static MACHINE_CONFIG_START( fp, fp_state )
	/* basic machine hardware */
	MCFG_CPU_ADD(I8086_TAG, I8086, XTAL_15MHz/3)
	MCFG_CPU_PROGRAM_MAP(fp_mem)
	MCFG_CPU_IO_MAP(fp_io)

	MCFG_CPU_ADD(HD63B01V1_TAG, HD6301, 2000000)
	MCFG_CPU_PROGRAM_MAP(sound_mem)
	MCFG_CPU_IO_MAP(sound_io)
	MCFG_DEVICE_DISABLE()

	/* video hardware */
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 256)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 256-1)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)
	MCFG_GFXDECODE(act_f1)
	MCFG_DEFAULT_LAYOUT( layout_lcd )
	
	MCFG_MC6845_ADD(MC6845_TAG, MC6845, 4000000, crtc_intf)

	/* Devices */
	MCFG_APRICOT_KEYBOARD_ADD(kb_intf)
	MCFG_I8237_ADD(I8237_TAG, 250000, dmac_intf)
	MCFG_PIC8259_ADD(I8259A_TAG, pic_intf)
	MCFG_Z80DART_ADD(Z80SIO0_TAG, 2500000, sio_intf)
	MCFG_WD2797_ADD(WD2797_TAG, fdc_intf)
	MCFG_LEGACY_FLOPPY_2_DRIVES_ADD(act_floppy_interface)
	MCFG_CENTRONICS_ADD(CENTRONICS_TAG, centronics_intf)
MACHINE_CONFIG_END



//**************************************************************************
//  ROMS
//**************************************************************************

//-------------------------------------------------
//  ROM( fp )
//-------------------------------------------------

ROM_START( fp )
	ROM_REGION( 0x8000, I8086_TAG, 0 )
	ROM_LOAD16_BYTE( "lo_fp_3.1.ic20", 0x0000, 0x4000, CRC(0572add2) SHA1(c7ab0e5ced477802e37f9232b5673f276b8f5623) )	// Labelled 11212721 F97E PORT LO VR 3.1
	ROM_LOAD16_BYTE( "hi_fp_3.1.ic9",  0x0001, 0x4000, CRC(3903674b) SHA1(8418682dcc0c52416d7d851760fea44a3cf2f914) )	// Labelled 11212721 BD2D PORT HI VR 3.1

	ROM_REGION( 0x1000, HD63B01V1_TAG, 0 )
	ROM_LOAD( "voice interface hd63b01v01.ic29", 0x0000, 0x1000, NO_DUMP )
ROM_END



//**************************************************************************
//  SYSTEM DRIVERS
//**************************************************************************

//    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT   INIT     COMPANY             FULLNAME        FLAGS
COMP( 1984, fp,    0,      0,      fp,   fp,    0,     "ACT",   "Apricot Portable / FP", GAME_NOT_WORKING | GAME_NO_SOUND )
