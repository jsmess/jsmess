/******************************************************************************
 Peter.Trauner@jk.uni-linz.ac.at May 2001

 Paul Robson's Emulator at www.classicgaming.com/studio2 made it possible
******************************************************************************/
/*****************************************************************************
Additional Notes by Manfred Schneider
Memory Map
Memory mapping is done in two steps. The PVI(2636) provides the chip select signals
according to signals provided by the CPU address lines.
The PVI has 12 address line (A0-A11) which give him control over 4K. A11 of the PVI is not
connected to A11 of the CPU, but connected to the cartridge slot. On the cartridge it is
connected to A12 of the CPU which extends the addressable Range to 8K. This is also the
maximum usable space, because A13 and A14 of the CPU are not used.
With the above in mind address range will lock like this:
$0000 - $15FF  ROM, RAM
$1600 - $167F  unused
$1680 - $16FF  used for I/O Control on main PCB
$1700 - $17FF PVI internal Registers and RAM
$1800 - $1DFF ROM, RAM
$1E00 - $1E7F unused
$1E80 - $1EFF mirror of $1680 - $167F
$1F00 - $1FFF mirror of $1700 - $17FF
$2000 - $3FFF mirror of $0000 - $1FFF
$4000 - $5FFF mirror of $0000 - $1FFF
$6000 - $7FFF mirror of $0000 - $1FFF
On all cartridges for the Interton A11 from PVI is connected to A12 of the CPU.
There are four different types of Cartridges with the following memory mapping.
Type 1: 2K Rom or EPROM mapped from $0000 - $07FF
Type 2: 4K Rom or EPROM mapped from $0000 - $0FFF
Type 3: 4K Rom + 1K Ram
	Rom is mapped from $0000 - $0FFF
	Ram is mapped from $1000 - $13FF and mirrored from $1800 - $1BFF
Type 4: 6K Rom + 1K Ram
	Rom is mapped from $0000 - $15FF (only 5,5K ROM visible to the CPU)
	Ram is mapped from $1800 - $1BFF

One other type is known for Radofin (compatible to VC4000, but not the Cartridge Pins)
which consisted of 2K ROM and 2K RAM which are properly mapped as follows:
2K Rom mapped from $0000 - $07FF
2K Ram mapped from $0800 - $0FFF
The Cartridge is called Hobby Module and properly the Rom is the same as used in
elektor TV Game Computer which is a kind of developer machine for the VC4000.
******************************************************************************/
   
#include "driver.h"
#include "cpu/s2650/s2650.h"

#include "includes/vc4000.h"
#include "devices/cartslot.h"
#include "devices/snapquik.h" 

static  READ8_HANDLER(vc4000_key_r)
{
	UINT8 data=0;
	switch(offset & 0x0f) {
	case 0x08:
		data = readinputport(1);
		break;
	case 0x09:
		data = readinputport(2);
		break;
	case 0x0a:
		data = readinputport(3);
		break;
	case 0x0b:
		data = readinputport(0);
		break;
	case 0x0c:
		data = readinputport(4);
		break;
	case 0x0d:
		data = readinputport(5);
		break;
	case 0x0e:
		data = readinputport(6);
		break;
	}
	return data;
}

static WRITE8_HANDLER(vc4000_sound_ctl)
{
	logerror("Write to sound control register offset= %d value= %d\n", offset, data);
}


static ADDRESS_MAP_START( vc4000_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x07ff) AM_READWRITE( MRA8_BANK1, MWA8_BANK5 )
	AM_RANGE(0x0800, 0x0fff) AM_READWRITE( MRA8_BANK2, MWA8_BANK6 )
	AM_RANGE(0x1000, 0x15ff) AM_READWRITE( MRA8_BANK3, MWA8_BANK7 )
	AM_RANGE(0x1600, 0x167f) AM_NOP
	AM_RANGE(0x1680, 0x16ff) AM_READWRITE( vc4000_key_r, vc4000_sound_ctl ) AM_MIRROR(0x0800)
	AM_RANGE(0x1700, 0x17ff) AM_READWRITE( vc4000_video_r, vc4000_video_w ) AM_MIRROR(0x0800)
	AM_RANGE(0x1800, 0x1bff) AM_READWRITE( MRA8_BANK4, MWA8_BANK8 )
ADDRESS_MAP_END

static ADDRESS_MAP_START( vc4000_io , ADDRESS_SPACE_IO, 8)
	AM_RANGE( S2650_SENSE_PORT,S2650_SENSE_PORT) AM_READ( vc4000_vsync_r)
ADDRESS_MAP_END

static INPUT_PORTS_START( vc4000 )
	PORT_START
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Start") PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Game Select") PORT_CODE(KEYCODE_F2)
	PORT_START
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 1") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 4") PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 7") PORT_CODE(KEYCODE_7) PORT_CODE(KEYCODE_A)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left Enter") PORT_CODE(KEYCODE_ENTER) PORT_CODE(KEYCODE_Z)
	PORT_START
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 2/Button") PORT_CODE(KEYCODE_2) PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 5") PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_W)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 8") PORT_CODE(KEYCODE_8) PORT_CODE(KEYCODE_S)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 0") PORT_CODE(KEYCODE_0) PORT_CODE(KEYCODE_X)
	PORT_START
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 3") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 6") PORT_CODE(KEYCODE_6) PORT_CODE(KEYCODE_E)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 9") PORT_CODE(KEYCODE_9) PORT_CODE(KEYCODE_D)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left Clear") PORT_CODE(KEYCODE_C) PORT_CODE(KEYCODE_ESC)
	PORT_START
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 7") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right ENTER") PORT_CODE(KEYCODE_ENTER_PAD)
	PORT_START
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 2/Button") PORT_CODE(KEYCODE_2_PAD) PORT_CODE(KEYCODE_LALT)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 8") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 0") PORT_CODE(KEYCODE_0_PAD)
	PORT_START
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 9") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right Clear") PORT_CODE(KEYCODE_DEL_PAD)
#ifndef ANALOG_HACK
    // shit, auto centering too slow, so only using 5 bits, and scaling at videoside
    PORT_START
PORT_BIT(0xff,0x70,IPT_AD_STICK_X) PORT_SENSITIVITY(70) PORT_KEYDELTA(5) PORT_CENTERDELTA(0) PORT_MINMAX(20,225) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH) PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH) PORT_PLAYER(1)
    PORT_START
PORT_BIT(0xff,0x70,IPT_AD_STICK_Y) PORT_SENSITIVITY(70) PORT_KEYDELTA(5) PORT_CENTERDELTA(0) PORT_MINMAX(20,225) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH) PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH) PORT_PLAYER(1)
    PORT_START
PORT_BIT(0xff,0x70,IPT_AD_STICK_X) PORT_SENSITIVITY(70) PORT_KEYDELTA(5) PORT_CENTERDELTA(0) PORT_MINMAX(20,225) PORT_CODE_DEC(KEYCODE_DEL) PORT_CODE_INC(KEYCODE_PGDN) PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH) PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH) PORT_PLAYER(2)
    PORT_START
PORT_BIT(0xff,0x70,IPT_AD_STICK_Y) PORT_SENSITIVITY(70) PORT_KEYDELTA(5) PORT_CENTERDELTA(0) PORT_MINMAX(20,225) PORT_CODE_DEC(KEYCODE_HOME) PORT_CODE_INC(KEYCODE_END) PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH) PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH) PORT_PLAYER(2)
#else
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/left") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/right") PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/down") PORT_CODE(KEYCODE_DOWN)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/up") PORT_CODE(KEYCODE_UP)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/left") PORT_CODE(KEYCODE_DEL)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/right") PORT_CODE(KEYCODE_PGDN)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/down") PORT_CODE(KEYCODE_END)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/up") PORT_CODE(KEYCODE_HOME)
#endif
INPUT_PORTS_END

static const rgb_t vc4000_palette[] =
{
	// background colors
	MAKE_RGB(0, 0, 0), // black
	MAKE_RGB(0, 0, 175), // blue
	MAKE_RGB(0, 175, 0), // green
	MAKE_RGB(0, 255, 255), // cyan
	MAKE_RGB(255, 0, 0), // red
	MAKE_RGB(255, 0, 255), // magenta
	MAKE_RGB(200, 200, 0), // yellow
	MAKE_RGB(200, 200, 200), // white
	// sprite colors
	// simplier to add another 8 colors else using colormapping
	// xor 7, bit 2 not green, bit 1 not blue, bit 0 not red
//	MAKE_RGB(255, 255, 255), // white
//	MAKE_RGB(255, 255, 0), // yellow
//	MAKE_RGB(255, 0, 255), // magenta
//	MAKE_RGB(175, 0, 0), // red
//	MAKE_RGB(0, 255, 255), // cyan
//	MAKE_RGB(0, 175, 0), // green
//	MAKE_RGB(0, 0, 175), // blue
//	MAKE_RGB(0, 0, 0) // black
};

static PALETTE_INIT( vc4000 )
{
	palette_set_colors(machine, 0, vc4000_palette, ARRAY_LENGTH(vc4000_palette));
}

static MACHINE_DRIVER_START( vc4000 )
	/* basic machine hardware */
	MDRV_CPU_ADD(S2650, 865000)        /* 3550000/4, 3580000/3, 4430000/3 */
	MDRV_CPU_PROGRAM_MAP(vc4000_mem, 0)
	MDRV_CPU_IO_MAP(vc4000_io, 0)
	MDRV_CPU_PERIODIC_INT(vc4000_video_line, 312*50)
	MDRV_INTERLEAVE(1)

	/* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(226, 312)
	MDRV_SCREEN_VISIBLE_AREA(10, 182, 0, 269)
	MDRV_PALETTE_LENGTH(ARRAY_LENGTH(vc4000_palette))
	MDRV_PALETTE_INIT( vc4000 )

	MDRV_VIDEO_START( vc4000 )
	MDRV_VIDEO_UPDATE( vc4000 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(vc4000_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

ROM_START(vc4000)
	ROM_REGION(0x2000,REGION_CPU1, 0)
	ROM_FILL( 0x0000, 0x2000, 0xFF )

ROM_END

static DEVICE_LOAD( vc4000_cart )
{
	int size = image_length(image);

	switch (size)
	{
	case 0x0800: // 2K
		image_fread(image, memory_region(REGION_CPU1) + 0x0000, 0x0800);
		memory_install_read8_handler (0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x07FF, 0, 0, MRA8_BANK1); 
		memory_install_write8_handler (0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x07FF, 0, 0, MWA8_BANK5);
		memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x0000);
		memory_install_read8_handler (0, ADDRESS_SPACE_PROGRAM, 0x0800, 0x0FFF, 0, 0, MRA8_BANK2); 
		memory_install_write8_handler (0, ADDRESS_SPACE_PROGRAM, 0x0800, 0x0FFF, 0, 0, MWA8_BANK6);
		memory_set_bankptr(2, mess_ram);
		memory_set_bankptr(6, mess_ram);
		break;

	case 0x1000: // 4K
		image_fread(image, memory_region(REGION_CPU1) + 0x0000, 0x1000);
		memory_install_read8_handler (0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x07FF, 0, 0, MRA8_BANK1); 
		memory_install_write8_handler (0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x07FF, 0, 0, MWA8_BANK5);
		memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x0000);
		memory_install_read8_handler (0, ADDRESS_SPACE_PROGRAM, 0x0800, 0x0FFF, 0, 0, MRA8_BANK2); 
		memory_install_write8_handler (0, ADDRESS_SPACE_PROGRAM, 0x0800, 0x0FFF, 0, 0, MWA8_BANK6);
		memory_set_bankptr(2, memory_region(REGION_CPU1) + 0x0800);
		memory_install_read8_handler (0, ADDRESS_SPACE_PROGRAM, 0x1000, 0x15FF, 0, 0, MRA8_BANK3); 
		memory_install_write8_handler (0, ADDRESS_SPACE_PROGRAM, 0x1000, 0x15FF, 0, 0, MWA8_BANK7);
		memory_set_bankptr(3, mess_ram);
		memory_set_bankptr(7, mess_ram);
		break;

	case 0x1800: // 6K
		image_fread(image, memory_region(REGION_CPU1) + 0x0000, 0x15FF);
		memory_install_read8_handler (0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x07FF, 0, 0, MRA8_BANK1); 
		memory_install_write8_handler (0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x07FF, 0, 0, MWA8_BANK5);
		memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x0000);
		memory_install_read8_handler (0, ADDRESS_SPACE_PROGRAM, 0x0800, 0x0FFF, 0, 0, MRA8_BANK2); 
		memory_install_write8_handler (0, ADDRESS_SPACE_PROGRAM, 0x0800, 0x0FFF, 0, 0, MWA8_BANK6);
		memory_set_bankptr(2, memory_region(REGION_CPU1) + 0x0800);
		memory_install_read8_handler (0, ADDRESS_SPACE_PROGRAM, 0x1000, 0x15FF, 0, 0, MRA8_BANK3); 
		memory_install_write8_handler (0, ADDRESS_SPACE_PROGRAM, 0x0800, 0x15FF, 0, 0, MWA8_BANK7);
		memory_set_bankptr(3, memory_region(REGION_CPU1) + 0x1000);
		memory_install_read8_handler (0, ADDRESS_SPACE_PROGRAM, 0x1800, 0x1BFF, 0, 0, MRA8_BANK4); 
		memory_install_write8_handler (0, ADDRESS_SPACE_PROGRAM, 0x1800, 0x1BFF, 0, 0, MWA8_BANK8);
		memory_set_bankptr(4, mess_ram);
		memory_set_bankptr(8, mess_ram);
		break;


	default:
		return INIT_FAIL;
	}
 
	return INIT_PASS;
}

static void vc4000_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;
		case MESS_DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = device_load_vc4000_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "rom,bin"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

QUICKLOAD_LOAD(vc4000)
{
	int i;
	int quick_addr = 0x08c0;
	int quick_length;
	UINT8 *quick_data;
	int read_;

	quick_length = image_length(image);
	quick_data = malloc(quick_length);
	if (!quick_data)
		return INIT_FAIL;

	read_ = image_fread(image, quick_data, quick_length);
	if (read_ != quick_length)
		return INIT_FAIL;
	
	if (quick_data[0] != 2)
		return INIT_FAIL;
		
	quick_addr = quick_data[1] * 256 + quick_data[2];	
	
	//if ((quick_addr + quick_length - 5) > 0x1000)
	//	return INIT_FAIL;
	
	program_write_byte(0x08be, quick_data[3]);	
	program_write_byte(0x08bf, quick_data[4]);	
	
	for (i = 0; i < quick_length - 5; i++)
	{	if ((quick_addr + i) < 0x1000)
			program_write_byte(i + quick_addr, quick_data[i+5]);
	}
	
	logerror("quick loading at %.4x size:%.4x\n", quick_addr, (quick_length-5));
	return INIT_PASS;
}

static void vc4000_quickload_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* quickload */
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "tvc"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_QUICKLOAD_LOAD:				info->f = (genf *) quickload_load_vc4000; break;

		default:										quickload_device_getinfo(devclass, state, info); break;
	}
}
  

SYSTEM_CONFIG_START(vc4000)
	CONFIG_RAM_DEFAULT(5 * 1024) 
	CONFIG_DEVICE(vc4000_cartslot_getinfo)
	CONFIG_DEVICE(vc4000_quickload_getinfo)
SYSTEM_CONFIG_END

/*    	YEAR		NAME	PARENT	COMPAT	MACHINE	INPUT	INIT		CONFIG	COMPANY		FULLNAME */
CONS(1978,	vc4000,	0,		0,		vc4000,	vc4000,	0,	vc4000,	"Interton",	"VC4000", GAME_IMPERFECT_GRAPHICS )
