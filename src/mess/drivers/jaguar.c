/***************************************************************************

	Atari Jaguar

	Nathan Woods
	based on MAME cojag driver by Aaron Giles

	TODO: Quite a bit; only a few games are playable.  Also, this driver
	lacks the speed up hacks in the MAME cojag driver

****************************************************************************

    Memory map (TBA)

    ========================================================================
    MAIN CPU
    ========================================================================

    ------------------------------------------------------------
    000000-3FFFFF   R/W   xxxxxxxx xxxxxxxx   DRAM 0
    400000-7FFFFF   R/W   xxxxxxxx xxxxxxxx   DRAM 1
    F00000-F000FF   R/W   xxxxxxxx xxxxxxxx   Tom Internal Registers
    F00400-F005FF   R/W   xxxxxxxx xxxxxxxx   CLUT - color lookup table A
    F00600-F007FF   R/W   xxxxxxxx xxxxxxxx   CLUT - color lookup table B
    F00800-F00D9F   R/W   xxxxxxxx xxxxxxxx   LBUF - line buffer A
    F01000-F0159F   R/W   xxxxxxxx xxxxxxxx   LBUF - line buffer B
    F01800-F01D9F   R/W   xxxxxxxx xxxxxxxx   LBUF - line buffer currently selected
    F02000-F021FF   R/W   xxxxxxxx xxxxxxxx   GPU control registers
    F02200-F022FF   R/W   xxxxxxxx xxxxxxxx   Blitter registers
    F03000-F03FFF   R/W   xxxxxxxx xxxxxxxx   Local GPU RAM
    F08800-F08D9F   R/W   xxxxxxxx xxxxxxxx   LBUF - 32-bit access to line buffer A
    F09000-F0959F   R/W   xxxxxxxx xxxxxxxx   LBUF - 32-bit access to line buffer B
    F09800-F09D9F   R/W   xxxxxxxx xxxxxxxx   LBUF - 32-bit access to line buffer currently selected
    F0B000-F0BFFF   R/W   xxxxxxxx xxxxxxxx   32-bit access to local GPU RAM
    F10000-F13FFF   R/W   xxxxxxxx xxxxxxxx   Jerry
    F14000-F17FFF   R/W   xxxxxxxx xxxxxxxx   Joysticks and GPIO0-5
    F18000-F1AFFF   R/W   xxxxxxxx xxxxxxxx   Jerry DSP
    F1B000-F1CFFF   R/W   xxxxxxxx xxxxxxxx   Local DSP RAM
    F1D000-F1DFFF   R     xxxxxxxx xxxxxxxx   Wavetable ROM
    ------------------------------------------------------------

***************************************************************************/


#include "driver.h"
#include "state.h"
#include "cpu/mips/r3000.h"
#include "cpu/jaguar/jaguar.h"
#include "includes/jaguar.h"
#include "devices/cartslot.h"
#include "devices/snapquik.h"



/*************************************
 *
 *	Global variables
 *
 *************************************/

UINT32 *jaguar_shared_ram;
UINT32 *jaguar_gpu_ram;
UINT32 *jaguar_gpu_clut;
UINT32 *jaguar_dsp_ram;
UINT32 *jaguar_wave_rom;
UINT8 cojag_is_r3000 = FALSE;



/*************************************
 *
 *	Local variables
 *
 *************************************/

static UINT32 joystick_data;
static UINT8 eeprom_enable;

static UINT32 *rom_base;
static size_t rom_size;

static UINT32 *cart_base;
static size_t cart_size;


static int jaguar_irq_callback(int level)
{
	return (level == 6) ? 0x40 : -1;
}

/*************************************
 *
 *	Machine init
 *
 *************************************/

static MACHINE_RESET( jaguar )
{
	cpunum_set_irq_callback(0, jaguar_irq_callback);

	*((UINT32 *) jaguar_gpu_ram) = 0x3d0dead;

	memset(jaguar_shared_ram, 0, 0x200000);
	memcpy(jaguar_shared_ram, rom_base, 0x10);
	rom_base[0x53c / 4] = 0x67000002;

#if 0
	/* set up main CPU RAM/ROM banks */
	memory_set_bankptr(3, jaguar_gpu_ram);

	/* set up DSP RAM/ROM banks */
	memory_set_bankptr(10, jaguar_shared_ram);
	memory_set_bankptr(11, jaguar_gpu_clut);
	memory_set_bankptr(12, jaguar_gpu_ram);
	memory_set_bankptr(13, jaguar_dsp_ram);
	memory_set_bankptr(14, jaguar_shared_ram);
#endif
	memory_set_bankptr(15, cart_base);
	memory_set_bankptr(16, rom_base);
//	memory_set_bankptr(17, jaguar_gpu_ram);

	/* clear any spinuntil stuff */
	jaguar_gpu_resume();
	jaguar_dsp_resume();

	/* halt the CPUs */
	jaguargpu_ctrl_w(1, G_CTRL, 0, 0);
	jaguardsp_ctrl_w(2, D_CTRL, 0, 0);

	/* init the sound system */
	cojag_sound_reset();

	joystick_data = 0xffffffff;
}



/*************************************
 *
 *  32-bit access to the GPU
 *
 *************************************/

static READ32_HANDLER( gpuctrl_r )
{
	return jaguargpu_ctrl_r(1, offset);
}


static WRITE32_HANDLER( gpuctrl_w )
{
	jaguargpu_ctrl_w(1, offset, data, mem_mask);
}



/*************************************
 *
 *  32-bit access to the DSP
 *
 *************************************/

static READ32_HANDLER( dspctrl_r )
{
	return jaguardsp_ctrl_r(2, offset);
}


static WRITE32_HANDLER( dspctrl_w )
{
	jaguardsp_ctrl_w(2, offset, data, mem_mask);
}



/*************************************
 *
 *  Input ports
 *
 *	Information from "The Jaguar Underground Documentation"
 *	by Klaus and Nat!
 *
 *************************************/

static READ32_HANDLER( joystick_r )
{
	UINT16 joystick_result = 0xffff;
	UINT16 joybuts_result = 0xffff;
	UINT32 result;
	int i;

	/*
	 *   16        12        8         4         0
	 *   +---------+---------+---------^---------+
	 *   |  pad 1  |  pad 0  |      unused       |
	 *   +---------+---------+-------------------+
	 *     15...12   11...8          7...0
	 *
	 *   Reading this register gives you the output of the selected columns
	 *   of the pads.
	 *   The buttons pressed will appear as cleared bits. 
	 *   See the description of the column addressing to map the bits 
	 *   to the buttons.
	 */

	for (i = 0; i < 8; i++)
	{
		if ((joystick_data & (0x10000 << i)) == 0)
		{
			joystick_result &= readinputport(i);
			joybuts_result &= readinputport(i+8);
		}
	}

	result = joystick_result;
	result <<= 16;
	result |= joybuts_result;
	return result;
}

static WRITE32_HANDLER( joystick_w )
{
	/*
	 *   16        12         8         4         0
	 *   +-+-------^------+--+---------+---------+
	 *   |r|    unused    |mu|  col 1  |  col 0  |  
	 *   +-+--------------+--+---------+---------+
	 *    15                8   7...4     3...0
	 *
	 *   col 0:   column control of joypad 0 
	 *
	 *      Here you select which column of the joypad to poll. 
	 *      The columns are:
	 *
	 *                Joystick       Joybut 
	 *      col_bit|11 10  9  8     1    0  
	 *      -------+--+--+--+--    ---+------
	 *         0   | R  L  D  U     A  PAUSE       (RLDU = Joypad directions)
	 *         1   | *  7  4  1     B       
	 *         2   | 2  5  8  0     C       
	 *         3   | 3  6  9  #   OPTION
	 *
	 *      You select a column my clearing the appropriate bit and setting
	 *      all the other "column" bits. 
	 *
	 *
	 *   col1:    column control of joypad 1
	 *
	 *      This is pretty much the same as for joypad EXCEPT that the
	 *      column addressing is reversed (strange!!)
	 *            
	 *                Joystick      Joybut 
	 *      col_bit|15 14 13 12     3    2
	 *      -------+--+--+--+--    ---+------
	 *         4   | 3  6  9  #   OPTION
	 *         5   | 2  5  8  0     C
	 *         6   | *  7  4  1     B
	 *         7   | R  L  D  U     A  PAUSE     (RLDU = Joypad directions)
	 *
	 *   mute (mu):   sound control
	 *
	 *      You can turn off the sound by clearing this bit.
	 *
	 *   read enable (r):  
	 *
	 *      Set this bit to read from the joysticks, clear it to write
	 *      to them.
	 */
	COMBINE_DATA(&joystick_data);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/


static ADDRESS_MAP_START( jaguar_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x1fffff) AM_RAM AM_BASE(&jaguar_shared_ram) AM_SHARE(1) AM_MIRROR(0x200000)
	AM_RANGE(0x800000, 0xdfffff) AM_ROM AM_BASE(&cart_base) AM_SIZE(&cart_size)
	AM_RANGE(0xe00000, 0xe1ffff) AM_ROM AM_BASE(&rom_base) AM_SIZE(&rom_size)
	AM_RANGE(0xf00000, 0xf003ff) AM_READWRITE(jaguar_tom_regs32_r, jaguar_tom_regs32_w)
	AM_RANGE(0xf00400, 0xf007ff) AM_RAM AM_BASE(&jaguar_gpu_clut) AM_SHARE(2)
	AM_RANGE(0xf02100, 0xf021ff) AM_READWRITE(gpuctrl_r, gpuctrl_w)
	AM_RANGE(0xf02200, 0xf022ff) AM_READWRITE(jaguar_blitter_r, jaguar_blitter_w)
	AM_RANGE(0xf03000, 0xf03fff) AM_MIRROR(0x008000) AM_RAM AM_BASE(&jaguar_gpu_ram) AM_SHARE(3)
	AM_RANGE(0xf10000, 0xf103ff) AM_READWRITE(jaguar_jerry_regs32_r, jaguar_jerry_regs32_w)
	AM_RANGE(0xf14000, 0xf14003) AM_READWRITE(joystick_r, joystick_w)
	AM_RANGE(0xf16000, 0xf1600b) AM_READ(cojag_gun_input_r)	// GPI02
//	AM_RANGE(0xf17800, 0xf17803) AM_WRITE(latch_w)	// GPI04
	AM_RANGE(0xf1a100, 0xf1a13f) AM_READWRITE(dspctrl_r, dspctrl_w)
	AM_RANGE(0xf1a140, 0xf1a17f) AM_READWRITE(jaguar_serial_r, jaguar_serial_w)
	AM_RANGE(0xf1b000, 0xf1cfff) AM_RAM AM_BASE(&jaguar_dsp_ram) AM_SHARE(4)
ADDRESS_MAP_END



/*************************************
 *
 *  GPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( gpu_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x1fffff) AM_RAM AM_SHARE(1) AM_MIRROR(0x200000)
	AM_RANGE(0x800000, 0xdfffff) AM_ROMBANK(15)
	AM_RANGE(0xe00000, 0xe1ffff) AM_ROMBANK(16)

	AM_RANGE(0xf00000, 0xf003ff) AM_READWRITE(jaguar_tom_regs32_r, jaguar_tom_regs32_w)
	AM_RANGE(0xf00400, 0xf007ff) AM_RAM AM_SHARE(2)
	AM_RANGE(0xf02100, 0xf021ff) AM_READWRITE(gpuctrl_r, gpuctrl_w)
	AM_RANGE(0xf02200, 0xf022ff) AM_READWRITE(jaguar_blitter_r, jaguar_blitter_w)
	AM_RANGE(0xf03000, 0xf03fff) AM_RAM AM_SHARE(3) AM_MIRROR(0x008000)
	AM_RANGE(0xf10000, 0xf103ff) AM_READWRITE(jaguar_jerry_regs32_r, jaguar_jerry_regs32_w)
ADDRESS_MAP_END



/*************************************
 *
 *  DSP memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( dsp_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x1fffff) AM_RAM AM_SHARE(1) AM_MIRROR(0x200000)
	AM_RANGE(0x800000, 0xdfffff) AM_ROMBANK(15)
	AM_RANGE(0xe00000, 0xe1ffff) AM_ROMBANK(16)
	AM_RANGE(0xf10000, 0xf103ff) AM_READWRITE(jaguar_jerry_regs32_r, jaguar_jerry_regs32_w)
	AM_RANGE(0xf1a100, 0xf1a13f) AM_READWRITE(dspctrl_r, dspctrl_w)
	AM_RANGE(0xf1a140, 0xf1a17f) AM_READWRITE(jaguar_serial_r, jaguar_serial_w)
	AM_RANGE(0xf1b000, 0xf1cfff) AM_RAM AM_SHARE(4)
	AM_RANGE(0xf1d000, 0xf1dfff) AM_ROM AM_BASE(&jaguar_wave_rom)
ADDRESS_MAP_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( jaguar )
    PORT_START  /* [0] */
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT) PORT_CODE(JOYCODE_1_RIGHT ) PORT_PLAYER(1)
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT) PORT_CODE(JOYCODE_1_LEFT ) PORT_PLAYER(1)
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN) PORT_CODE(JOYCODE_1_DOWN ) PORT_PLAYER(1)
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_NAME("Up") PORT_CODE(KEYCODE_UP) PORT_CODE(JOYCODE_1_UP ) PORT_PLAYER(1)
	PORT_BIT ( 0xf0ff, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_START  /* [1] */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("*") PORT_CODE(KEYCODE_K) PORT_CODE(CODE_NONE ) PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CODE(CODE_NONE ) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CODE(CODE_NONE ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CODE(CODE_NONE ) PORT_PLAYER(1)
	PORT_BIT ( 0xf0ff, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_START  /* [2] */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CODE(CODE_NONE ) PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CODE(CODE_NONE ) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CODE(CODE_NONE ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CODE(CODE_NONE ) PORT_PLAYER(1)
	PORT_BIT ( 0xf0ff, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_START  /* [3] */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CODE(CODE_NONE ) PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CODE(CODE_NONE ) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CODE(CODE_NONE ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("#") PORT_CODE(KEYCODE_L) PORT_CODE(CODE_NONE ) PORT_PLAYER(1)
	PORT_BIT ( 0xf0ff, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_START  /* [4] */
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(CODE_NONE ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(CODE_NONE ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(CODE_NONE ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("#") PORT_CODE(CODE_NONE ) PORT_PLAYER(2)
	PORT_BIT ( 0x0fff, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_START  /* [5] */
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(CODE_NONE ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(CODE_NONE ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(CODE_NONE ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(CODE_NONE ) PORT_PLAYER(2)
	PORT_BIT ( 0x0fff, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_START  /* [6] */
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("*") PORT_CODE(CODE_NONE ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(CODE_NONE ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(CODE_NONE ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(CODE_NONE ) PORT_PLAYER(2)
	PORT_BIT ( 0x0fff, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_START  /* [7] */
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_NAME("Right") PORT_CODE(KEYCODE_6_PAD) PORT_CODE(JOYCODE_2_RIGHT ) PORT_PLAYER(2)
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_NAME("Left") PORT_CODE(KEYCODE_4_PAD) PORT_CODE(JOYCODE_2_LEFT ) PORT_PLAYER(2)
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_NAME("Down") PORT_CODE(KEYCODE_2_PAD) PORT_CODE(JOYCODE_2_DOWN ) PORT_PLAYER(2)
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_NAME("Up") PORT_CODE(KEYCODE_8_PAD) PORT_CODE(JOYCODE_2_UP ) PORT_PLAYER(2)
	PORT_BIT ( 0x0fff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START  /* [8] */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON5) PORT_NAME(DEF_STR(Pause)) PORT_CODE(KEYCODE_P) PORT_CODE(JOYCODE_1_BUTTON5 ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CODE(JOYCODE_1_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT ( 0xfffc, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START  /* [9] */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_NAME("B") PORT_CODE(KEYCODE_S) PORT_CODE(JOYCODE_1_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT ( 0xfffd, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START  /* [10] */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_NAME("C") PORT_CODE(KEYCODE_X) PORT_CODE(JOYCODE_1_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT ( 0xfffd, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START  /* [11] */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_NAME("Option") PORT_CODE(KEYCODE_O) PORT_CODE(JOYCODE_1_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT ( 0xfffd, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START  /* [12] */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_NAME("Option") PORT_CODE(KEYCODE_PLUS_PAD) PORT_CODE(JOYCODE_2_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT ( 0xfffd, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START  /* [13] */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_NAME("C") PORT_CODE(KEYCODE_MINUS_PAD) PORT_CODE(JOYCODE_2_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT ( 0xfffd, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START  /* [14] */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_NAME("B") PORT_CODE(KEYCODE_ASTERISK) PORT_CODE(JOYCODE_2_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT ( 0xfffd, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START  /* [15] */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON5) PORT_NAME(DEF_STR(Pause)) PORT_CODE(KEYCODE_ENTER_PAD) PORT_CODE(JOYCODE_2_BUTTON5 ) PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_NAME("A") PORT_CODE(KEYCODE_SLASH_PAD) PORT_CODE(JOYCODE_2_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT ( 0xfffc, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


/*************************************
 *
 *	Machine driver
 *
 *************************************/


static struct jaguar_config gpu_config =
{
	jaguar_gpu_cpu_int
};


static struct jaguar_config dsp_config =
{
	jaguar_dsp_cpu_int
};

MACHINE_DRIVER_START( jaguar )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68EC020, 13295000)
	MDRV_CPU_PROGRAM_MAP(jaguar_map,0)

	MDRV_CPU_ADD(JAGUARGPU, 52000000/2)
	MDRV_CPU_CONFIG(gpu_config)
	MDRV_CPU_PROGRAM_MAP(gpu_map,0)

	MDRV_CPU_ADD(JAGUARDSP, 52000000/2)
	MDRV_CPU_CONFIG(dsp_config)
	MDRV_CPU_PROGRAM_MAP(dsp_map,0)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(jaguar)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(40*8, 30*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 40*8-1, 0*8, 30*8-1)
	MDRV_PALETTE_LENGTH(65534)

	MDRV_VIDEO_START(cojag)
	MDRV_VIDEO_UPDATE(cojag)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( jaguar )
	ROM_REGION( 0xe20000, REGION_CPU1, 0 )  /* 4MB for RAM at 0 */
	ROM_LOAD16_WORD( "jagboot.rom",          0xe00000, 0x020000, CRC(fb731aaa) SHA1(f8991b0c385f4e5002fa2a7e2f5e61e8c5213356))
	ROM_CART_LOAD(0, "jag\0abs\0bin\0rom\0", 0x800000, 0x600000, ROM_NOMIRROR)
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( jaguar )
{
	state_save_register_global(joystick_data);
	state_save_register_global(eeprom_enable);

	/* init the sound system and install DSP speedups */
	cojag_sound_init();
}

static QUICKLOAD_LOAD( jaguar )
{
	offs_t quickload_begin = 0x4000;
	quickload_size = MIN(quickload_size, 0x200000 - quickload_begin);
	image_fread(image, &memory_region(REGION_CPU1)[quickload_begin], quickload_size);
	cpunum_set_reg(0, REG_PC, quickload_begin);
	return INIT_PASS;
}

static void jaguar_quickload_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* quickload */
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "bin"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_QUICKLOAD_LOAD:				info->f = (genf *) quickload_load_jaguar; break;

		default:										quickload_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(jaguar)
	CONFIG_DEVICE(cartslot_device_getinfo)
	CONFIG_DEVICE(jaguar_quickload_getinfo)
SYSTEM_CONFIG_END



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

/*    YEAR	NAME      PARENT    COMPAT	MACHINE   INPUT     INIT      CONFIG	COMPANY		FULLNAME */
CONS(1993,	jaguar,   0,        0,		jaguar,   jaguar,   jaguar,   jaguar,	"Atari",	"Atari Jaguar", GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND)
