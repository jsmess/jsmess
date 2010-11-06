/***************************************************************************
   
        PHUNSY (Philipse Universal System)

        04/11/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/s2650/s2650.h"


#define LOG	1


class phunsy_state : public driver_device
{
public:
	phunsy_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *video_ram;
	UINT8	data_out;
	UINT8	keyboard_input;
	emu_timer	*kb_timer;
};


static ADDRESS_MAP_START(phunsy_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x07ff) AM_ROM
	AM_RANGE( 0x0800, 0x0fff) AM_RAM
	AM_RANGE( 0x1000, 0x17ff) AM_RAM	AM_BASE_MEMBER( phunsy_state, video_ram ) // Video RAM
	AM_RANGE( 0x1800, 0x1fff) AM_RAM // Banked ROM
	AM_RANGE( 0x4000, 0xffff) AM_RAMBANK("bank2") // Banked RAM
ADDRESS_MAP_END


static WRITE8_HANDLER( phunsy_data_w )
{
	phunsy_state *state = space->machine->driver_data<phunsy_state>();

	if (LOG)
		logerror("%s: phunsy_data_w %02x\n", cpuexec_describe_context(space->machine), data);

	state->data_out = data;

	/* b0 - TTY out */
	/* b1 - select MDCR / keyboard */
	/* b2 - Clear keyboard strobe signal */
	if ( data & 0x04 )
	{
		state->keyboard_input |= 0x80;
	}
	/* b3 - speaker output */
	/* b4 - -REV MDCR output */
	/* b5 - -FWD MDCR output */
	/* b6 - -WCD MDCR output */
	/* b7 - WDA MDCR output */
}


static READ8_HANDLER( phunsy_data_r )
{
	phunsy_state *state = space->machine->driver_data<phunsy_state>();
	UINT8 data;

	if (LOG)
		logerror("%s: phunsy_data_r\n", cpuexec_describe_context(space->machine));

	if ( state->data_out & 0x02 )
	{
		/* MDCR selected */
		/* b0 - TTY input */
		/* b1 - SK1 switch input */
		/* b2 - SK2 switch input */
		/* b3 - -WEN MDCR input */
		/* b4 - -CIP MDCR input */
		/* b5 - -BET MDCR input */
		/* b6 - RDA MDCR input */
		/* b7 - RDC MDCR input */
		data = 0xFF;
	}
	else
	{
		/* Keyboard selected */
		/* b0-b6 - ASCII code from keyboard */
		/* b7    - strobe signal */
		data = state->keyboard_input;
	}

	return data;
}


static READ8_HANDLER( phunsy_sense_r )
{
	return 0;
}


static ADDRESS_MAP_START( phunsy_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( S2650_DATA_PORT,S2650_DATA_PORT) AM_READWRITE( phunsy_data_r, phunsy_data_w )
	AM_RANGE( S2650_SENSE_PORT,S2650_SENSE_PORT) AM_READ( phunsy_sense_r)
ADDRESS_MAP_END


/* Input ports */
INPUT_PORTS_START( phunsy )
	PORT_START( "IN0" )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_NAME("A")
INPUT_PORTS_END


static TIMER_CALLBACK( phunsy_kb_scan )
{
	phunsy_state *state = machine->driver_data<phunsy_state>();
	UINT8 data;

	data = input_port_read( machine, "IN0" );

	if ( data & 0x01 )
	{
		state->keyboard_input = 0x61;
	}
}


static DRIVER_INIT( phunsy )
{
	phunsy_state *state = machine->driver_data<phunsy_state>();

	state->kb_timer = timer_alloc( machine, phunsy_kb_scan, NULL );
}


static MACHINE_RESET(phunsy) 
{
	phunsy_state *state = machine->driver_data<phunsy_state>();

	memory_set_bankptr( machine, "bank2", memory_region(machine, "ram_4000") );
	state->keyboard_input = 0xFF;

	timer_adjust_periodic( state->kb_timer, machine->primary_screen->frame_period(), 0, machine->primary_screen->frame_period() );
}


static PALETTE_INIT( phunsy )
{
	for ( int i = 0; i < 8; i++ )
	{
		int j = ( i << 5 ) | ( i << 2 ) | ( i >> 1 );

		palette_set_color_rgb( machine, i, j, j, j );
	}
}


static VIDEO_START( phunsy )
{
}


static VIDEO_UPDATE( phunsy )
{
	phunsy_state *state = screen->machine->driver_data<phunsy_state>();
	UINT8	*gfx = memory_region( screen->machine, "gfx" );
	UINT8	*v = state->video_ram;

	for ( int h = 0; h < 32; h++ )
	{
		for ( int w = 0; w < 64; w++ )
		{
			UINT8 c = *v;

			if ( c & 0x80 )
			{
				UINT8 grey = ( c >> 4 ) & 0x07;
				/* Graphics mode */
				for ( int i = 0; i < 4; i++ )
				{
					UINT16 *p = BITMAP_ADDR16( bitmap, h * 8 + i, w * 6 );

					p[0] = ( c & 0x01 ) ? grey : 0;
					p[1] = ( c & 0x01 ) ? grey : 0;
					p[2] = ( c & 0x01 ) ? grey : 0;
					p[3] = ( c & 0x02 ) ? grey : 0;
					p[4] = ( c & 0x02 ) ? grey : 0;
					p[5] = ( c & 0x02 ) ? grey : 0;
				}
				for ( int i = 0; i < 4; i++ )
				{
					UINT16 *p = BITMAP_ADDR16( bitmap, h * 8 + 4 + i, w * 6 );

					p[0] = ( c & 0x04 ) ? grey : 0;
					p[1] = ( c & 0x04 ) ? grey : 0;
					p[2] = ( c & 0x04 ) ? grey : 0;
					p[3] = ( c & 0x08 ) ? grey : 0;
					p[4] = ( c & 0x08 ) ? grey : 0;
					p[5] = ( c & 0x08 ) ? grey : 0;
				}
			}
			else
			{
				/* ASCII mode */
				for ( int i = 0; i < 8; i++ )
				{
					UINT16 *p = BITMAP_ADDR16( bitmap, h * 8 + i, w * 6 );
					UINT8 pat = gfx[ c * 8 + i];

					p[0] = ( pat & 0x20 ) ? 7 : 0;
					p[1] = ( pat & 0x10 ) ? 7 : 0;
					p[2] = ( pat & 0x08 ) ? 7 : 0;
					p[3] = ( pat & 0x04 ) ? 7 : 0;
					p[4] = ( pat & 0x02 ) ? 7 : 0;
					p[5] = ( pat & 0x01 ) ? 7 : 0;
				}
			}

			v++;
		}
	}
    return 0;
}


static MACHINE_CONFIG_START( phunsy, phunsy_state )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",S2650, XTAL_1MHz)
    MDRV_CPU_PROGRAM_MAP(phunsy_mem)
    MDRV_CPU_IO_MAP(phunsy_io)	

    MDRV_MACHINE_RESET(phunsy)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
	/* Display (page 12 of pdf)
	   - 8Mhz clock
	   - 64 6 pixel characters on a line.
	   - 16us not active, 48us active: ( 64 * 6 ) * 60 / 48 => 480 pixels wide
	   - 313 line display of which 256 are displayed.
	*/
	MDRV_SCREEN_RAW_PARAMS(XTAL_8MHz, 480, 0, 64*6, 313, 0, 256)
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_PALETTE_LENGTH(8)
    MDRV_PALETTE_INIT(phunsy)

    MDRV_VIDEO_START(phunsy)
    MDRV_VIDEO_UPDATE(phunsy)
MACHINE_CONFIG_END


/* ROM definition */
ROM_START( phunsy )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "phunsy_bios.bin", 0x0000, 0x0800, CRC(a789e82e) SHA1(b1c130ab2b3c139fd16ddc5dc7bdcaf7a9957d02))
	ROM_LOAD( "dass.bin",        0x0800, 0x0800, CRC(13380140) SHA1(a999201cb414abbf1e10a7fcc1789e3e000a5ef1))
	ROM_LOAD( "pdcr.bin",        0x1000, 0x0800, CRC(74bf9d0a) SHA1(8d2f673615215947f033571f1221c6aa99c537e9))
	ROM_LOAD( "labhnd.bin",      0x1800, 0x0800, CRC(1d5a106b) SHA1(a20d09e32e21cf14db8254cbdd1d691556b473f0))
	ROM_REGION( 0x0400, "gfx", ROMREGION_ERASEFF )
	ROM_LOAD( "ph_char1.bin", 0x0000, 0x0200, CRC(a7e567fc) SHA1(b18aae0a2d4f92f5a7e22640719bbc4652f3f4ee))
	ROM_LOAD( "ph_char2.bin", 0x0200, 0x0200, CRC(3d5786d3) SHA1(8cf87d83be0b5e4becfa9fd6e05b01250a2feb3b))
	/* 16 x 16KB RAM banks */
	ROM_REGION( 0x40000, "ram_4000", ROMREGION_ERASEFF )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1980, phunsy,  0,       0, 	phunsy, 	phunsy, 	 phunsy,  	   	 "J.F.P. Philipse",   "PHUNSY",		GAME_NOT_WORKING | GAME_NO_SOUND)

