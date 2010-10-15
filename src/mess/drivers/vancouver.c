/******************************************************************************
 Mephisto Vancouver Chess Computer
 2010 R. Schaefer

 CPU 68000 
 Clock 12 MHz
 IRQ CLK 600 Hz

 RAM	0x00400000, 0x0087ffff
 ROM	0x00000000, 0x0003ffff 

 I/O	0xc00000	  read_board   
		0xc80000	  write_board  
		0xd00000 	  write_led    
		0xf00000	  Buttons   
		0xd00004	  Buttons   
		0xd00008	  Buttons   
		0xd80000	  LCD Data  
		0xd80008	  LCD Enable + Beeper  
		0xe80002	  Unknown   
		0xe80004	  Unknown   
		0xe80006	  Unknown   
		 

******************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "sound/beep.h"
#include "debugger.h"

#include "video/hd44780.h"

#include "render.h"
#include "rendlay.h"
#include "uiinput.h"

#include "machine/mboard.h"

#define VERBOSE 0
#define LOG(x)	do { if (VERBOSE) logerror x; } while (0)

#define MAIN_CLOCK 12000000 /* 12 MHz */
 
#define FALSE	0
#define TRUE	1

static int own_char=FALSE;

static emu_timer *start_timer = NULL;
static emu_timer *irq_timer = NULL;

static const hd44780_interface van16_display =
{
    2,                  /* number of lines */
    16,                 /* chars for line */
    NULL                /* custom display layout */
};

static void get_bin(char* b_str, UINT8 b)
{
	int i,k;
	k=8;
	for (i=0;i<8;i++)
	{
		if (k==4)
			b_str[k--]=' ';

		if ( BIT(b,i) )
			b_str[k--]='1';
		else
			b_str[k--]='0';
	}
	b_str[9]=0;
}

static int get_char(UINT8 *data)
{
	switch (*data)
	{
	case 0xef: *data=0x6f;	/* oe = o */
		return TRUE;
	case 0xe1: *data=0x61;	/* ae = a */
		return TRUE;
	case 0xf5: *data=0x75;	/* ue = u */
		return TRUE;

	case 0x08: *data=161;	/* King */
		return TRUE;
	case 0x09: *data=162;	/* Queen */
		return TRUE;
	case 0x0a: *data=163;	/* Rook */
		return TRUE;
	case 0x0b: *data=164;	/* Bishop */
		return TRUE;
	case 0x0c: *data=165;	/* Knight */
		return TRUE; 
	case 0x0d: *data=166;	/* Pawn */
		return TRUE;
	case 0x0e: *data=167;	/* symbol for white */
		return TRUE;
	case 0x0f: *data=168;	/* symbol for black */
		return TRUE;
	default:
		return FALSE;
	}
}


/* Read port */

static READ16_HANDLER( read_button_f00000 )
{
	UINT16 data=0;
	LOG(("Read from %06x offset: %x\n",0xf00000,offset));

	data=input_port_read(space->machine, "BUTTON_1");

	data=data<<8;

    if (data)
		return data;

   return 0x0000;
}

static READ16_HANDLER( read_button_f00004 )
{
	UINT16 data=0;
	LOG(("Read from %06x offset: %x\n",0xf00004,offset));

	data=input_port_read(space->machine, "BUTTON_2");
	data=data<<8;

    if (data)
		return data;

    return 0x0000;
}

static READ16_HANDLER( read_button_f00008 )
{
	UINT16 data=0;
	LOG(("Read from %06x offset: %x\n",0xf00008,offset));

	data=input_port_read(space->machine, "BUTTON_3");

	data=data<<8;
	
    if (data)
		return data;

    return 0x0000;
}

/* Write Port */

/* LCD  - DATA */

static WRITE16_HANDLER( write_lcd_data_d80000 )
{
	UINT8 data_8;
	char bin_str[10];
	int is_data;

	hd44780_device * hd44780 = space->machine->device<hd44780_device>("hd44780");

	data_8=data>>8;
	get_bin(bin_str,data_8);

	//logerror("Write DATA %06x data: %02x  %s\n",0xd80000,data_8,bin_str);

	LOG(("Write from %06x data: %04x\n",0xd80000,data));
	
	if (!own_char)							/* skip certain ascii control commands - only if they are not used for symbols */
	{
		if (data_8 == 0x0d ||  
			data_8 == 0x0f || data_8 == 0x0b )
			return;
	}

	is_data=get_char(&data_8);
  
	if (BIT(data_8, 7) && !is_data)  
		hd44780->control_write(128, data_8);		/* Set Cursor */
	else
		hd44780->data_write(128, data_8);


	if ( data_8 == 0xc1 || data_8 == 0xc2 )		/* Control command for display  self defined ascii char */
		own_char = TRUE;
	else
		own_char =FALSE;

}

/* LCD - ENABLE */

static WRITE16_HANDLER( write_lcd_enable_d80008 )
{
	UINT8 data_8;
	char bin_str[10];
 
	running_device *speaker = space->machine->device("beep");

	data_8=data>>8;
	get_bin(bin_str,data_8);

	LOG(("Write from %06x data: %04x\n",0xd80008,data));
  
//	beep_set_state(speaker,BIT(data_8, 3)?1:0);
	beep_set_state(speaker,BIT(data_8, 2)?1:0);
	
}

/* Unknown read/write */

static READ16_HANDLER( read_e80002 )
{
	LOG(("Read from %06x offset: %x\n",0xe80002,offset));
    return 0xff00;
}

static WRITE16_HANDLER( write_e80004 )
{
	LOG(("Write from %06x data: %04x\n",0xe80004,data));
}

static READ16_HANDLER( read_e80006 )
{
	LOG(("Read from %06x offset: %x\n",0xe80006,offset));
	return 0x0000;

}

static TIMER_CALLBACK( timer_update_irq )
{
		UINT32 A3=0;
		UINT32 PC=0;

		PC=cpu_get_reg(machine->device("maincpu"), M68K_PC);
		A3=cpu_get_reg(machine->device("maincpu"), M68K_A3);		//HACK for Overclocking
		if (A3 == 0x00407f00)
		{
			cputag_set_input_line_and_vector(machine, "maincpu",  M68K_IRQ_7,ASSERT_LINE, M68K_INT_ACK_AUTOVECTOR);
			cputag_set_input_line_and_vector(machine, "maincpu",  M68K_IRQ_7,CLEAR_LINE, M68K_INT_ACK_AUTOVECTOR);			 
		}else
		{
//			debugger_break(machine);
//			logerror("A3: %08x PC: %08x\n",A3.PC);
		}
}


static TIMER_CALLBACK( start_timer_callback)
{
	timer_adjust_periodic( irq_timer, attotime_zero, 0, ATTOTIME_IN_HZ(605) );  
	timer_reset(start_timer, attotime_never);
}

static MACHINE_START( van16 )
{
	
	start_timer = timer_alloc(machine, start_timer_callback, NULL);
	irq_timer = timer_alloc(machine, timer_update_irq, NULL);

	timer_adjust_oneshot(start_timer, ATTOTIME_IN_MSEC(500), 0);
	timer_pulse(machine, ATTOTIME_IN_HZ(100), NULL, 0, update_artwork);

	mboard_savestate_register(machine);

}

static MACHINE_RESET( van16 )
{
	hd44780_device * hd44780 = machine->device<hd44780_device>("hd44780");

	hd44780->control_write(128, 15);		/* Initialize LCD */

	set_boarder_pieces();
	set_board();
}

static VIDEO_START( van16 )
{

}

static VIDEO_UPDATE( van16 )
{
    hd44780_device * hd44780 = screen->machine->device<hd44780_device>("hd44780");

    return hd44780->video_update( bitmap, cliprect );
}

static PALETTE_INIT( van16 )
{
	palette_set_color(machine, 0, MAKE_RGB(255, 255, 255));
    palette_set_color(machine, 1, MAKE_RGB(0, 0, 0));
}


static const gfx_layout van16_charlayout =
{
    5, 8,                   /* 5 x 8 characters */
    224,                    /* 224 characters */
    1,                      /* 1 bits per pixel */
    { 0 },                  /* no bitplanes */
    { 3, 4, 5, 6, 7},
    { 0, 8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
    8*8                     /* 8 bytes */
};

static GFXDECODE_START( van16 )
    GFXDECODE_ENTRY( "chargen", 0x0000, van16_charlayout, 0, 1 )
GFXDECODE_END


/* Address maps */

static ADDRESS_MAP_START(van16_mem, ADDRESS_SPACE_PROGRAM, 16)

	AM_RANGE( 0x000000,  0x03ffff)  AM_ROM

	AM_RANGE( 0xc00000 , 0xc00001)  AM_READ     ( read_board_16 )				/* read_board  */
	AM_RANGE( 0xc80000 , 0xc80001)  AM_WRITE    ( write_board_16 )				/* write_board */
	AM_RANGE( 0xd00000 , 0xd00001)  AM_WRITE    ( write_LED_16 )				/* write_led   */
	AM_RANGE( 0xf00000 , 0xf00001)  AM_READ     ( read_button_f00000 )			/* 2 Buttons   */
	AM_RANGE( 0xf00004 , 0xf00005)  AM_READ     ( read_button_f00004 )			/* 2 Buttons   */
	AM_RANGE( 0xf00008 , 0xf00009)  AM_READ     ( read_button_f00008 )			/* 2 Buttons   */
	AM_RANGE( 0xd80000 , 0xd80001)  AM_WRITE    ( write_lcd_data_d80000 )		/* LCD Data    */
	AM_RANGE( 0xd80008 , 0xd80009)  AM_WRITE    ( write_lcd_enable_d80008 )		/* LCD Enable + Beeper */ 
	AM_RANGE( 0xe80004 , 0xe80005)  AM_WRITE    ( write_e80004 )				/* Unknown */
	AM_RANGE( 0xe80002 , 0xe80003)  AM_READ     ( read_e80002 )					/* Unknown */
	AM_RANGE( 0xe80006 , 0xe80007)  AM_READ     ( read_e80006 )					/* Unknown */

//	AM_RANGE( 0x00400000, 0x0047ffff)  AM_RAM      /* 512KB */ 
	AM_RANGE( 0x00400000, 0x0087ffff)  AM_RAM      /* 512KB ??? working */

ADDRESS_MAP_END
 
 
/* Input ports */

static INPUT_PORTS_START( board )
	PORT_START("LINE2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_START("LINE3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_START("LINE4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_START("LINE5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_START("LINE6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_START("LINE7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_START("LINE8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_START("LINE9")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)

	PORT_START("LINE10")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)

	PORT_START("B_WHITE") 
	PORT_BIT(0x01,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x02,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x04,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x08,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD)

	PORT_START("B_BLACK") 
	PORT_BIT(0x01,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x02,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x04,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x08,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD)

	PORT_START("B_BUTTONS") 
	PORT_BIT(0x01,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x02,  IP_ACTIVE_HIGH, IPT_KEYBOARD)

INPUT_PORTS_END

static INPUT_PORTS_START( van16 )

	PORT_INCLUDE( board )

	PORT_START("MOUSE_X")
	PORT_BIT( 0xffff, 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(100)    

	PORT_START("MOUSE_Y")
	PORT_BIT( 0xffff, 0x00, IPT_MOUSE_Y ) PORT_SENSITIVITY(100)   	 

	PORT_START("BUTTON_L")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_CODE(MOUSECODE_BUTTON1) PORT_NAME("left button")

	PORT_START("BUTTON_R")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CODE(MOUSECODE_BUTTON2) PORT_NAME("right button")

	PORT_START("BUTTON_1") 
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("LEFT") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("ENT") PORT_CODE(KEYCODE_5)

	PORT_START("BUTTON_2") 
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("RIGHT") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("UP") PORT_CODE(KEYCODE_7)

	PORT_START("BUTTON_3") 
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("DOWN") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("CLR") PORT_CODE(KEYCODE_9)

INPUT_PORTS_END
 
/* Machine driver */
static MACHINE_CONFIG_START( van16, driver_device )
    /* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M68000, MAIN_CLOCK)
	MDRV_CPU_PROGRAM_MAP(van16_mem)
	MDRV_MACHINE_START(van16)
	MDRV_MACHINE_RESET(van16)

    /* video hardware */

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(100, 22)
	MDRV_SCREEN_VISIBLE_AREA(0, 100-1, 0, 22-3)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(van16)

	MDRV_GFXDECODE(van16)

	MDRV_HD44780_ADD("hd44780", van16_display)

    MDRV_VIDEO_START(van16)
    MDRV_VIDEO_UPDATE(van16)

	MDRV_DEFAULT_LAYOUT(layout_lcd)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("beep", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

MACHINE_CONFIG_END
 
/* ROM definition */

ROM_START( van16 )
    ROM_REGION16_BE( 0x1000000, "maincpu", 0 )
    ROM_LOAD16_BYTE("va16even.bin", 0x00000, 0x20000,CRC(e87602d5) SHA1(90cb2767b4ae9e1b265951eb2569b9956b9f7f44))
    ROM_LOAD16_BYTE("va16odd.bin" , 0x00001, 0x20000,CRC(585f3bdd) SHA1(90bb94a12d3153a91e3760020e1ea2a9eaa7ec0a))

    ROM_REGION( 0x00700, "chargen", ROMREGION_ERASE )
    ROM_LOAD( "m_chargen.bin",    0x0000, 0x0700, CRC(b497af5c) SHA1(0bddebf91dac868ffbdd7f514253943635d7fe9a))

ROM_END

/* Driver */
 
/*    YEAR  NAME          PARENT  COMPAT  MACHINE    INPUT       INIT      COMPANY  FULLNAME                     FLAGS */
CONS(  1984, van16, 0,        0,    van16,       van16,   0,	   "Hegener & Glaser Muenchen",  "Mephisto Vancouver 16 Bit", GAME_SUPPORTS_SAVE)
