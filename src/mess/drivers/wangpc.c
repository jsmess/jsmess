#include "includes/wangpc.h"

//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( wangpc_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( wangpc_mem, AS_PROGRAM, 16, wangpc_state )
	AM_RANGE(0xfc000, 0xfffff) AM_ROM AM_REGION(I8086_TAG, 0)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( wangpc_io )
//-------------------------------------------------

static ADDRESS_MAP_START( wangpc_io, AS_IO, 16, wangpc_state )
	AM_RANGE(0x1014, 0x1015) AM_DEVREAD8_LEGACY(UPD765_TAG, upd765_status_r, 0x00ff)
	AM_RANGE(0x1016, 0x1017) AM_DEVREADWRITE8_LEGACY(UPD765_TAG, upd765_data_r, upd765_data_w, 0x00ff)
	AM_RANGE(0x1020, 0x1027) AM_DEVREADWRITE8(I8255A_TAG, i8255_device, read, write, 0x00ff)
	AM_RANGE(0x1040, 0x1047) AM_DEVREADWRITE8_LEGACY(I8253_TAG, pit8253_r, pit8253_w, 0x00ff)
	AM_RANGE(0x1060, 0x1063) AM_DEVREADWRITE8_LEGACY(I8259A_TAG, pic8259_r, pic8259_w, 0x00ff)
//	AM_RANGE(0x1080, 0x108f) AM_DEVREADWRITE8(SCN2661_TAG, scn2661_device, read, write, 0x00ff)
	AM_RANGE(0x10a0, 0x10bf) AM_DEVREADWRITE8_LEGACY(AM9517A_TAG, i8237_r, i8237_w, 0x00ff)
//	AM_RANGE(0x10e6, 0x10e9) AM_DEVREADWRITE8(IM6402_TAG, im6402_device, read, write, 0x00ff)
ADDRESS_MAP_END



//**************************************************************************
//  INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_PORTS( wangpc )
//-------------------------------------------------

static INPUT_PORTS_START( wangpc )
INPUT_PORTS_END



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  I8237_INTERFACE( dmac_intf )
//-------------------------------------------------

static I8237_INTERFACE( dmac_intf )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL }
};


//-------------------------------------------------
//  pic8259_interface pic_intf
//-------------------------------------------------

static IRQ_CALLBACK( wangpc_irq_callback )
{
	wangpc_state *state = device->machine().driver_data<wangpc_state>();

	return pic8259_acknowledge(state->m_pic);
}

static const struct pic8259_interface pic_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  I8255A_INTERFACE( ppi_intf )
//-------------------------------------------------

static I8255A_INTERFACE( ppi_intf )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  pit8253_config pit_intf
//-------------------------------------------------

static const struct pit8253_config pit_intf =
{
	{
		{
			0,
			DEVCB_NULL,
			DEVCB_NULL
		}, {
			0,
			DEVCB_NULL,
			DEVCB_NULL
		}, {
			0,
			DEVCB_NULL,
			DEVCB_NULL
		}
	}
};


//-------------------------------------------------
//  upd765_interface fdc_intf
//-------------------------------------------------

static const floppy_interface floppy_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSSD,
	LEGACY_FLOPPY_OPTIONS_NAME(default),
	"floppy_5_25",
	NULL
};

static const upd765_interface fdc_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	NULL,
	UPD765_RDY_PIN_NOT_CONNECTED,
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};


//-------------------------------------------------
//  WANGPC_KEYBOARD_INTERFACE( kb_intf )
//-------------------------------------------------

static WANGPC_KEYBOARD_INTERFACE( kb_intf )
{
	DEVCB_NULL
};



//**************************************************************************
//  MACHINE INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_START( wangpc )
//-------------------------------------------------

void wangpc_state::machine_start()
{
	// register CPU IRQ callback
	device_set_irq_callback(m_maincpu, wangpc_irq_callback);
}



//**************************************************************************
//  MACHINE DRIVERS
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( wangpc )
//-------------------------------------------------

static MACHINE_CONFIG_START( wangpc, wangpc_state )
	MCFG_CPU_ADD(I8086_TAG, I8086, 4000000)
	MCFG_CPU_PROGRAM_MAP(wangpc_mem)
	MCFG_CPU_IO_MAP(wangpc_io)

	// video hardware
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_UPDATE_DRIVER(wangpc_state, screen_update)
	MCFG_SCREEN_SIZE(640,480)
	MCFG_SCREEN_VISIBLE_AREA(0,639, 0,479)
	MCFG_SCREEN_REFRESH_RATE(60)
	
	// devices
	MCFG_I8237_ADD(AM9517A_TAG, 4000000, dmac_intf)
	MCFG_PIC8259_ADD(I8259A_TAG, pic_intf)
	MCFG_I8255A_ADD(I8255A_TAG, ppi_intf)
	MCFG_PIT8253_ADD(I8253_TAG, pit_intf)
	// IM6402 for keyboard
	// SCN2661 for RS-232
	MCFG_UPD765A_ADD(UPD765_TAG, fdc_intf)
	MCFG_LEGACY_FLOPPY_2_DRIVES_ADD(floppy_intf)
	MCFG_WANGPC_KEYBOARD_ADD(kb_intf)

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("128K")
MACHINE_CONFIG_END



//**************************************************************************
//  ROM DEFINITIONS
//**************************************************************************
	
//-------------------------------------------------
//  ROM( wangpc )
//-------------------------------------------------

ROM_START( wangpc )
	ROM_REGION16_LE( 0x4000, I8086_TAG, 0)
	ROM_LOAD16_BYTE( "motherboard-l94.bin", 0x0001, 0x2000, CRC(f9f41304) SHA1(1815295809ef11573d724ede47446f9ac7aee713) )
	ROM_LOAD16_BYTE( "motherboard-l115.bin", 0x0000, 0x2000, CRC(67b37684) SHA1(70d9f68eb88cc2bc9f53f949cc77411c09a4266e) )

	ROM_REGION( 0x1000, "remotecomms", 0 )
	ROM_LOAD( "remotecomms-l28.bin", 0x0000, 0x1000, CRC(c05a1bee) SHA1(6b3f0d787d014b1fd3925812c905ddb63c5055f1) )

	ROM_REGION( 0x1000, "network", 0 )
	ROM_LOAD( "network-l22.bin", 0x0000, 0x1000, CRC(487e5f04) SHA1(81e52e70e0c6e34715119b121ec19a7758cd6772) )

	ROM_REGION( 0x1000, "hdc1", 0 )
	ROM_LOAD( "hdc-l54.bin", 0x0000, 0x1000, CRC(94e9a17d) SHA1(060c576d70069ece2d0dbce86ffc448df2b169e7) )

	ROM_REGION( 0x1000, "hdc2", 0 )
	ROM_LOAD( "hdc-l19.bin", 0x0000, 0x1000, CRC(282770d2) SHA1(a0e3bad5041e0dfd6087907015b07a093b576bc0) )
ROM_END



//**************************************************************************
//  GAME DRIVERS
//**************************************************************************

COMP( 1985, wangpc, 0, 0, wangpc, wangpc, 0, "Wang Laboratories", "Wang Professional Computer", GAME_NOT_WORKING | GAME_NO_SOUND )
