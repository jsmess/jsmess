/***************************************************************************

  snes.c

  Driver file to handle emulation of the Nintendo Super NES.

  R. Belmont
  Anthony Kruize
  Angelo Salese
  Fabio Priuli
  Based on the original MESS driver by Lee Hammerton (aka Savoury Snax)

  Driver is preliminary right now.

  The memory map included below is setup in a way to make it easier to handle
  Mode 20 and Mode 21 ROMs.

  Todo (in no particular order):
    - Fix additional sound bugs
    - Emulate extra chips - superfx, dsp2, sa-1 etc.
    - Add horizontal mosaic, hi-res. interlaced etc to video emulation.
    - Fix support for Mode 7. (In Progress)
    - Handle interleaved roms (maybe even multi-part roms, but how?)
    - Add support for running at 3.58 MHz at the appropriate time.
    - I'm sure there's lots more ...

***************************************************************************/

#include "emu.h"
#include "video/generic.h"
#include "cpu/spc700/spc700.h"
#include "cpu/superfx/superfx.h"
#include "cpu/g65816/g65816.h"
#include "includes/snes.h"
#include "machine/snescart.h"
#include "crsshair.h"

#define MAX_SNES_CART_SIZE 0x600000

/*************************************
 *
 *  Memory handlers
 *
 *************************************/

static READ8_HANDLER( spc_ram_100_r )
{
	return spc_ram_r(space, offset + 0x100);
}

static WRITE8_HANDLER( spc_ram_100_w )
{
	spc_ram_w(space, offset + 0x100, data);
}

/*************************************
 *
 *  Address maps
 *
 *************************************/

static ADDRESS_MAP_START( snes_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x000000, 0x2fffff) AM_READWRITE(snes_r_bank1, snes_w_bank1)	/* I/O and ROM (repeats for each bank) */
	AM_RANGE(0x300000, 0x3fffff) AM_READWRITE(snes_r_bank2, snes_w_bank2)	/* I/O and ROM (repeats for each bank) */
	AM_RANGE(0x400000, 0x5fffff) AM_READ(snes_r_bank3)		/* ROM (and reserved in Mode 20) */
	AM_RANGE(0x600000, 0x6fffff) AM_READWRITE(snes_r_bank4, snes_w_bank4)	/* used by Mode 20 DSP-1 */
	AM_RANGE(0x700000, 0x7dffff) AM_READWRITE(snes_r_bank5, snes_w_bank5)
	AM_RANGE(0x7e0000, 0x7fffff) AM_RAM					/* 8KB Low RAM, 24KB High RAM, 96KB Expanded RAM */
	AM_RANGE(0x800000, 0xbfffff) AM_READWRITE(snes_r_bank6, snes_w_bank6)	/* Mirror and ROM */
	AM_RANGE(0xc00000, 0xffffff) AM_READWRITE(snes_r_bank7, snes_w_bank7)	/* Mirror and ROM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( superfx_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x000000, 0x3fffff) AM_READWRITE(superfx_r_bank1, superfx_w_bank1)
	AM_RANGE(0x400000, 0x5fffff) AM_READWRITE(superfx_r_bank2, superfx_w_bank2)
	AM_RANGE(0x600000, 0x7dffff) AM_READWRITE(superfx_r_bank3, superfx_w_bank3)
	AM_RANGE(0x800000, 0xbfffff) AM_READWRITE(superfx_r_bank1, superfx_w_bank1)
	AM_RANGE(0xc00000, 0xdfffff) AM_READWRITE(superfx_r_bank2, superfx_w_bank2)
	AM_RANGE(0xe00000, 0xffffff) AM_READWRITE(superfx_r_bank3, superfx_w_bank3)
ADDRESS_MAP_END

static ADDRESS_MAP_START( spc_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x00ef) AM_READWRITE(spc_ram_r, spc_ram_w) AM_BASE(&spc_ram)   	/* lower 32k ram */
	AM_RANGE(0x00f0, 0x00ff) AM_READWRITE(spc_io_r, spc_io_w)   	/* spc io */
	AM_RANGE(0x0100, 0xffff) AM_WRITE(spc_ram_100_w)
	AM_RANGE(0x0100, 0xffbf) AM_READ(spc_ram_100_r)
	AM_RANGE(0xffc0, 0xffff) AM_READ(spc_ipl_r)
ADDRESS_MAP_END



/*************************************
 *
 *  Input ports
 *
 *************************************/

static CUSTOM_INPUT( snes_mouse_speed_input )
{
	snes_state *state = (snes_state *)field->port->machine->driver_data;
	int port = (FPTR)param;

	if (snes_ram[OLDJOY1] & 0x1)
	{
		state->mouse[port].speed++;
		if ((state->mouse[port].speed & 0x03) == 0x03)
			state->mouse[port].speed = 0;
	}

	return state->mouse[port].speed;
}

static CUSTOM_INPUT( snes_superscope_offscreen_input )
{
	snes_state *state = (snes_state *)field->port->machine->driver_data;
	int port = (FPTR)param;
	static const char *const portnames[2][3] =
			{
				{ "SUPERSCOPE1", "SUPERSCOPE1_X", "SUPERSCOPE1_Y" },
				{ "SUPERSCOPE2", "SUPERSCOPE2_X", "SUPERSCOPE2_Y" },
			};

	UINT16 x = input_port_read(field->port->machine, portnames[port][1]);
	UINT16 y = input_port_read(field->port->machine, portnames[port][2]);

	/* these are the theoretical boundaries */
	if (x < 0 || x >= SNES_SCR_WIDTH || y < 0 || y >= snes_ppu.beam.last_visible_line)
		state->scope[port].offscreen = 1;
	else
		state->scope[port].offscreen = 0;

	return state->scope[port].offscreen;
}

static TIMER_CALLBACK( lightgun_tick )
{
	if ((input_port_read(machine, "CTRLSEL") & 0x0f) == 0x03 || (input_port_read(machine, "CTRLSEL") & 0x0f) == 0x04)
	{
		/* enable lightpen crosshair */
		crosshair_set_screen(machine, 0, CROSSHAIR_SCREEN_ALL);
	}
	else
	{
		/* disable lightpen crosshair */
		crosshair_set_screen(machine, 0, CROSSHAIR_SCREEN_NONE);
	}

	if ((input_port_read(machine, "CTRLSEL") & 0xf0) == 0x30 || (input_port_read(machine, "CTRLSEL") & 0xf0) == 0x40)
	{
		/* enable lightpen crosshair */
		crosshair_set_screen(machine, 1, CROSSHAIR_SCREEN_ALL);
	}
	else
	{
		/* disable lightpen crosshair */
		crosshair_set_screen(machine, 1, CROSSHAIR_SCREEN_NONE);
	}
}


static INPUT_PORTS_START( snes_joypads )
	PORT_START("SERIAL1_DATA1_L")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("P1 Button A") PORT_PLAYER(1) PORT_CATEGORY(11)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("P1 Button X") PORT_PLAYER(1) PORT_CATEGORY(11)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_NAME("P1 Button L") PORT_PLAYER(1) PORT_CATEGORY(11)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_NAME("P1 Button R") PORT_PLAYER(1) PORT_CATEGORY(11)
	PORT_START("SERIAL1_DATA1_H")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P1 Button B") PORT_PLAYER(1) PORT_CATEGORY(11)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("P1 Button Y") PORT_PLAYER(1) PORT_CATEGORY(11)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SELECT ) PORT_PLAYER(1) PORT_CATEGORY(11)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START1 ) PORT_PLAYER(1) PORT_CATEGORY(11)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1) PORT_CATEGORY(11)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1) PORT_CATEGORY(11)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1) PORT_CATEGORY(11)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_CATEGORY(11)

	PORT_START("SERIAL2_DATA1_L")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("P2 Button A") PORT_PLAYER(2) PORT_CATEGORY(21)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("P2 Button X") PORT_PLAYER(2) PORT_CATEGORY(21)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_NAME("P2 Button L") PORT_PLAYER(2) PORT_CATEGORY(21)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_NAME("P2 Button R") PORT_PLAYER(2) PORT_CATEGORY(21)
	PORT_START("SERIAL2_DATA1_H")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P2 Button B") PORT_PLAYER(2) PORT_CATEGORY(21)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("P2 Button Y") PORT_PLAYER(2) PORT_CATEGORY(21)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SELECT ) PORT_PLAYER(2) PORT_CATEGORY(21)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 ) PORT_PLAYER(2) PORT_CATEGORY(21)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2) PORT_CATEGORY(21)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2) PORT_CATEGORY(21)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2) PORT_CATEGORY(21)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_CATEGORY(21)

	PORT_START("SERIAL1_DATA2_L")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_START("SERIAL1_DATA2_H")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("SERIAL2_DATA2_L")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_START("SERIAL2_DATA2_H")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

static INPUT_PORTS_START( snes_mouse )
	PORT_START("MOUSE1")
	/* bits 0,3 = mouse signature (must be 1) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	/* bits 4,5 = mouse speed: 0 = slow, 1 = normal, 2 = fast, 3 = unused */
	PORT_BIT( 0x30, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(snes_mouse_speed_input, (void *)0)
	/* bits 6,7 = mouse buttons */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P1 Mouse Button Left") PORT_PLAYER(1) PORT_CATEGORY(12)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("P1 Mouse Button Right") PORT_PLAYER(1) PORT_CATEGORY(12)

	PORT_START("MOUSE1_X")
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1) PORT_CATEGORY(12)

	PORT_START("MOUSE1_Y")
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1) PORT_CATEGORY(12)

	PORT_START("MOUSE2")
	/* bits 0,3 = mouse signature (must be 1) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	/* bits 4,5 = mouse speed: 0 = slow, 1 = normal, 2 = fast, 3 = unused */
	PORT_BIT( 0x30, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(snes_mouse_speed_input, (void *)1)
	/* bits 6,7 = mouse buttons */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P2 Mouse Button Left") PORT_PLAYER(2) PORT_CATEGORY(22)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("P2 Mouse Button Right") PORT_PLAYER(2) PORT_CATEGORY(22)

	PORT_START("MOUSE2_X")
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(2) PORT_CATEGORY(22)

	PORT_START("MOUSE2_Y")
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(2) PORT_CATEGORY(22)
INPUT_PORTS_END

static INPUT_PORTS_START( snes_superscope )
	PORT_START("SUPERSCOPE1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )	// Noise
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(snes_superscope_offscreen_input, (void *)0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("Port1 Superscope Pause") PORT_PLAYER(1) PORT_CATEGORY(13)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("Port1 Superscope Turbo") PORT_TOGGLE PORT_PLAYER(1) PORT_CATEGORY(13)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("Port1 Superscope Cursor") PORT_PLAYER(1) PORT_CATEGORY(13)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Port1 Superscope Fire") PORT_PLAYER(1) PORT_CATEGORY(13)

	PORT_START("SUPERSCOPE1_X")
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_NAME("Port1 Superscope X Axis") PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(1) PORT_CATEGORY(13)

	PORT_START("SUPERSCOPE1_Y")
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y) PORT_NAME("Port1 Superscope Y Axis") PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(1) PORT_CATEGORY(13)

	PORT_START("SUPERSCOPE2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )	// Noise
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(snes_superscope_offscreen_input, (void *)1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("Port2 Superscope Pause") PORT_PLAYER(2) PORT_CATEGORY(23)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("Port2 Superscope Turbo") PORT_PLAYER(2) PORT_CATEGORY(23)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("Port2 Superscope Cursor") PORT_PLAYER(2) PORT_CATEGORY(23)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Port2 Superscope Fire") PORT_PLAYER(2) PORT_CATEGORY(23)

	PORT_START("SUPERSCOPE2_X")
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_NAME("Port2 Superscope X Axis") PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(2) PORT_CATEGORY(23)

	PORT_START("SUPERSCOPE2_Y")
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y) PORT_NAME("Port2 Superscope Y Axis") PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(2) PORT_CATEGORY(23)
INPUT_PORTS_END

static INPUT_PORTS_START( snes )
	PORT_START("CTRLSEL")  /* Select Controller Type */
	PORT_CATEGORY_CLASS( 0x0f, 0x01, "P1 Controller")
	PORT_CATEGORY_ITEM(  0x00, "Unconnected",		10 )
	PORT_CATEGORY_ITEM(  0x01, "Gamepad",		11 )
	PORT_CATEGORY_ITEM(  0x02, "Mouse",			12 )
	PORT_CATEGORY_ITEM(  0x03, "Superscope",		13 )
//	PORT_CATEGORY_ITEM(  0x04, "Justfier",		14 )
//	PORT_CATEGORY_ITEM(  0x05, "Multitap",		15 )
	PORT_CATEGORY_CLASS( 0xf0, 0x10, "P2 Controller")
	PORT_CATEGORY_ITEM(  0x00, "Unconnected",		20 )
	PORT_CATEGORY_ITEM(  0x10, "Gamepad",		21 )
	PORT_CATEGORY_ITEM(  0x20, "Mouse",			22 )
	PORT_CATEGORY_ITEM(  0x30, "Superscope",		23 )
//	PORT_CATEGORY_ITEM(  0x40, "Justfier",		24 )
//	PORT_CATEGORY_ITEM(  0x50, "Multitap",		25 )

	PORT_INCLUDE(snes_joypads)
	PORT_INCLUDE(snes_mouse)
	PORT_INCLUDE(snes_superscope)


#ifdef SNES_LAYER_DEBUG
	PORT_START("INTERNAL")
	PORT_CONFNAME( 0x1, 0x1, "Enforce 32 sprites/line" )
	PORT_CONFSETTING(   0x0, DEF_STR( No ) )
	PORT_CONFSETTING(   0x1, DEF_STR( Yes ) )

	PORT_START("DEBUG1")
	PORT_CONFNAME( 0x3, 0x0, "Browse tiles" )
	PORT_CONFSETTING(   0x0, DEF_STR( Off ) )
	PORT_CONFSETTING(   0x1, "2bpl" )
	PORT_CONFSETTING(   0x2, "4bpl" )
	PORT_CONFSETTING(   0x3, "8bpl" )
	PORT_CONFNAME( 0xc, 0x0, "Browse maps" )
	PORT_CONFSETTING(   0x0, DEF_STR( Off ) )
	PORT_CONFSETTING(   0x4, "2bpl" )
	PORT_CONFSETTING(   0x8, "4bpl" )
	PORT_CONFSETTING(   0xc, "8bpl" )

	PORT_START("DEBUG2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle BG 1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle BG 2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle BG 3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle BG 4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Objects") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Main/Sub") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Back col") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Windows") PORT_CODE(KEYCODE_8_PAD)

	PORT_START("DEBUG3")
	PORT_BIT( 0x1, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Pal prev") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x2, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Pal next") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x4, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Transparency") PORT_CODE(KEYCODE_9_PAD)

	PORT_START("DEBUG4")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Mode 0 draw")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Mode 1 draw")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Mode 2 draw")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Mode 3 draw")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Mode 4 draw")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Mode 5 draw")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Mode 6 draw")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Mode 7 draw")
#endif
INPUT_PORTS_END


/*************************************
 *
 *  Input callbacks
 *
 *************************************/

static void snes_gun_latch( running_machine *machine, INT16 x, INT16 y )
{
	/* these are the theoretical boundaries */
	if (x < 0)
		x = 0;
	if (x > (SNES_SCR_WIDTH - 1))
		x = SNES_SCR_WIDTH - 1;

	if (y < 0)
		y = 0;
	if (y > (snes_ppu.beam.last_visible_line - 1))
		y = snes_ppu.beam.last_visible_line - 1;

	snes_ppu.beam.latch_horz = x;
	snes_ppu.beam.latch_vert = y;
	snes_ram[STAT78] |= 0x40;
}

static void snes_input_read_joy( running_machine *machine, int port )
{
	snes_state *state = (snes_state *)machine->driver_data;
	static const char *const portnames[2][4] =
			{
				{ "SERIAL1_DATA1_L", "SERIAL1_DATA1_H", "SERIAL1_DATA2_L", "SERIAL1_DATA2_H" },
				{ "SERIAL2_DATA1_L", "SERIAL2_DATA1_H", "SERIAL2_DATA2_L", "SERIAL2_DATA2_H" },
			};

	state->joypad[port + 0].low  = input_port_read(machine, portnames[port][0]);
	state->joypad[port + 0].high = input_port_read(machine, portnames[port][1]);
	state->joypad[port + 2].low  = input_port_read(machine, portnames[port][2]);
	state->joypad[port + 2].high = input_port_read(machine, portnames[port][3]);
}

static void snes_input_read_mouse( running_machine *machine, int port )
{
	snes_state *state = (snes_state *)machine->driver_data;
	INT16 var;
	static const char *const portnames[2][3] =
			{
				{ "MOUSE1", "MOUSE1_X", "MOUSE1_Y" },
				{ "MOUSE2", "MOUSE2_X", "MOUSE2_Y" },
			};

	state->mouse[port].buttons = input_port_read(machine, portnames[port][0]);
	state->mouse[port].x = input_port_read(machine, portnames[port][1]);
	state->mouse[port].y = input_port_read(machine, portnames[port][2]);
	var = state->mouse[port].x - state->mouse[port].oldx;

	if (var < -127)
	{
		state->mouse[port].deltax = 0xff;
		state->mouse[port].oldx -= 127;
	}
	else if (var < 0)
	{
		state->mouse[port].deltax = 0x80 | (-var);
		state->mouse[port].oldx = state->mouse[port].x;
	}
	else if (var > 127)
	{
		state->mouse[port].deltax = 0x7f;
		state->mouse[port].oldx += 127;
	}
	else
	{
		state->mouse[port].deltax = var & 0xff;
		state->mouse[port].oldx = state->mouse[port].x;
	}

	var = state->mouse[port].y - state->mouse[port].oldy;

	if (var < -127)
	{
		state->mouse[port].deltay = 0xff;
		state->mouse[port].oldy -= 127;
	}
	else if (var < 0)
	{
		state->mouse[port].deltay = 0x80 | (-var);
		state->mouse[port].oldy = state->mouse[port].y;
	}
	else if (var > 127)
	{
		state->mouse[port].deltay = 0x7f;
		state->mouse[port].oldy += 127;
	}
	else
	{
		state->mouse[port].deltay = var & 0xff;
		state->mouse[port].oldy = state->mouse[port].y;
	}
}

static void snes_input_read_superscope( running_machine *machine, int port )
{
	snes_state *state = (snes_state *)machine->driver_data;
	static const char *const portnames[2][3] =
			{
				{ "SUPERSCOPE1", "SUPERSCOPE1_X", "SUPERSCOPE1_Y" },
				{ "SUPERSCOPE2", "SUPERSCOPE2_X", "SUPERSCOPE2_Y" },
			};
	UINT8 input;

	/* first read input bits */
	state->scope[port].x = input_port_read(machine, portnames[port][1]);
	state->scope[port].y = input_port_read(machine, portnames[port][2]);
	input = input_port_read(machine, portnames[port][0]);

	/* then start elaborating input bits: only keep old turbo value */
	state->scope[port].buttons &= 0x20;

	/* set onscreen/offscreen */
	state->scope[port].buttons |= BIT(input, 1);

	/* turbo is a switch; toggle is edge sensitive */
	if (BIT(input, 5) && !state->scope[port].turbo_lock)
	{
		state->scope[port].buttons ^= 0x20;
		state->scope[port].turbo_lock = 1;
	}
	else if (!BIT(input, 5))
		state->scope[port].turbo_lock = 0;

	/* fire is a button; if turbo is active, trigger is level sensitive; otherwise it is edge sensitive */
	if (BIT(input, 7) && (BIT(state->scope[port].buttons, 5) || !state->scope[port].fire_lock))
	{
		state->scope[port].buttons |= 0x80;
		state->scope[port].fire_lock = 1;
	}
	else if (!BIT(input, 7))
		state->scope[port].fire_lock = 0;

	/* cursor is a button; it is always level sensitive */
	state->scope[port].buttons |= BIT(input, 6);

	/* pause is a button; it is always edge sensitive */
	if (BIT(input, 4) && !state->scope[port].pause_lock)
	{
		state->scope[port].buttons |= 0x10;
		state->scope[port].pause_lock = 1;
	}
	else if (!BIT(input, 4))
		state->scope[port].pause_lock = 0;

	/* If we have pressed fire or cursor and we are on-screen and SuperScope is in Port2, then latch video signal.
	Notice that we only latch Port2 because its IOBit pin is connected to bit7 of the IO Port, while Port1 has 
	IOBit pin connected to bit6 of the IO Port, and the latter is not detected by the H/V Counters. In other
	words, you can connect SuperScope to Port1, but there is no way SNES could detect its on-screen position */
	if ((state->scope[port].buttons & 0xc0) && !(state->scope[port].buttons & 0x02) && port == 1)
		snes_gun_latch(machine, state->scope[port].x, state->scope[port].y);
}

static void snes_input_read( running_machine *machine )
{
	snes_state *state = (snes_state *)machine->driver_data;
	UINT8 ctrl1 = input_port_read(machine, "CTRLSEL") & 0x0f;
	UINT8 ctrl2 = (input_port_read(machine, "CTRLSEL") & 0xf0) >> 4;
	int i;

	/* Check if lightgun has been chosen as input: if so, enable crosshair */
	timer_set(machine, attotime_zero, NULL, 0, lightgun_tick);

	switch (ctrl1)
	{
	case 1:	/* SNES joypad */
		snes_input_read_joy(machine, 0);
		break;
	case 2:	/* SNES Mouse */
		snes_input_read_mouse(machine, 0);
		break;
	case 3:	/* SNES Superscope */
		snes_input_read_superscope(machine, 0);
		break;
	case 0:	/* no controller in port1 */
	default:
		break;
	}

	switch (ctrl2)
	{
	case 1:	/* SNES joypad */
		snes_input_read_joy(machine, 1);
		break;
	case 2:	/* SNES Mouse */
		snes_input_read_mouse(machine, 1);
		break;
	case 3:	/* SNES Superscope */
		snes_input_read_superscope(machine, 1);
		break;
	case 0:	/* no controller in port2 */
	default:
		break;
	}

	// avoid sending signals that could crash games
	for (i = 0; i < 4; i++)
	{
		// if left, no right
		if (state->joypad[i].high & 2)
			state->joypad[i].high &= ~1;
		// if up, no down
		if (state->joypad[i].high & 8)
			state->joypad[i].high &= ~4;
	}

	// is automatic reading on?
	if (snes_ram[NMITIMEN] & 1)
	{
		switch (ctrl1)
		{
		case 1:	/* SNES joypad */
			state->joy1l = state->joypad[0].low;
			state->joy1h = state->joypad[0].high;
			state->joy3l = state->joypad[2].low;
			state->joy3h = state->joypad[2].high;
			break;
		case 2:	/* SNES Mouse */
			state->joy1l = state->mouse[0].buttons;
			state->joy1h = 0;
			state->joy3l = 0;
			state->joy3h = 0;
			break;
		case 3:	/* SNES Superscope */
			state->joy1l = 0xff;
			state->joy1h = state->scope[0].buttons;
			state->joy3l = 0;
			state->joy3h = 0;
			break;
		case 0:	/* no controller in port2 */
		default:
			state->joy1l = 0;
			state->joy1h = 0;
			state->joy3l = 0;
			state->joy3h = 0;
			break;
		}

		switch (ctrl2)
		{
		case 1:	/* SNES joypad */
			state->joy2l = state->joypad[1].low;
			state->joy2h = state->joypad[1].high;
			state->joy4l = state->joypad[3].low;
			state->joy4h = state->joypad[3].high;
			break;
		case 2:	/* SNES Mouse */
			state->joy2l = state->mouse[1].buttons;
			state->joy2h = 0;
			state->joy4l = 0;
			state->joy4h = 0;
			break;
		case 3:	/* SNES Superscope */
			state->joy2l = 0xff;
			state->joy2h = state->scope[1].buttons;
			state->joy4l = 0;
			state->joy4h = 0;
			break;
		case 0:	/* no controller in port2 */
		default:
			state->joy2l = 0;
			state->joy2h = 0;
			state->joy4l = 0;
			state->joy4h = 0;
			break;
		}

		// make sure read_idx starts returning all 1s because the auto-read reads it :-)
		state->read_idx[0] = 16;
		state->read_idx[1] = 16;
	}

}

static UINT8 snes_oldjoy1_read( running_machine *machine )
{
	snes_state *state = (snes_state *)machine->driver_data;
	UINT8 ctrl1 = input_port_read(machine, "CTRLSEL") & 0x0f;
	UINT8 res = 0;

	switch (ctrl1)
	{
	case 1:	/* SNES joypad */
		if (state->read_idx[0] >= 16)
			res = 0x01;
		else
			res = ((state->joypad[0].low | (state->joypad[0].high << 8)) >> (15 - state->read_idx[0]++)) & 0x01;
		break;
	case 2:	/* SNES Mouse */
		if (state->read_idx[0] >= 32)
			res = 0x01;
		else if (state->read_idx[0] >= 24)
			res = (state->mouse[0].deltax >> (31 - state->read_idx[0]++)) & 0x01;
		else if (state->read_idx[0] >= 16)
			res = (state->mouse[0].deltay >> (23 - state->read_idx[0]++)) & 0x01;
		else if (state->read_idx[0] >= 8)
			res = (state->mouse[0].buttons >> (15 - state->read_idx[0]++)) & 0x01;
		else
			res = 0;
		break;
	case 3:	/* SNES Superscope */
		if (state->read_idx[0] >= 8)
			res = 0x01;
		else
			res = (state->scope[0].buttons >> (7 - state->read_idx[0]++)) & 0x01;
		break;
	case 0:	/* no controller in port2 */
	default:
		break;
	}

	return res;
}

static UINT8 snes_oldjoy2_read( running_machine *machine )
{
	snes_state *state = (snes_state *)machine->driver_data;
	UINT8 ctrl2 = (input_port_read(machine, "CTRLSEL") & 0xf0) >> 4;
	UINT8 res = 0;

	switch (ctrl2)
	{
	case 1:	/* SNES joypad */
		if (state->read_idx[1] >= 16)
			res = 0x01;
		else
			res = ((state->joypad[1].low | (state->joypad[1].high << 8)) >> (15 - state->read_idx[1]++)) & 0x01;
		break;
	case 2:	/* SNES Mouse */
		if (state->read_idx[1] >= 32)
			res = 0x01;
		else if (state->read_idx[1] >= 24)
			res = (state->mouse[1].deltax >> (31 - state->read_idx[1]++)) & 0x01;
		else if (state->read_idx[1] >= 16)
			res = (state->mouse[1].deltay >> (23 - state->read_idx[1]++)) & 0x01;
		else if (state->read_idx[1] >= 8)
			res = (state->mouse[1].buttons >> (15 - state->read_idx[1]++)) & 0x01;
		else
			res = 0;
		break;
	case 3:	/* SNES Superscope */
		if (state->read_idx[1] >= 8)
			res = 0x01;
		else
			res = (state->scope[1].buttons >> (7 - state->read_idx[1]++)) & 0x01;
		break;
	case 0:	/* no controller in port2 */
	default:
		break;
	}

	return res;
}

/*************************************
 *
 *  Machine driver
 *
 *************************************/

static MACHINE_RESET( snes_mess )
{
	snes_state *state = (snes_state *)machine->driver_data;

	MACHINE_RESET_CALL(snes);

	state->io_read = snes_input_read;
	state->oldjoy1_read = snes_oldjoy1_read;
	state->oldjoy2_read = snes_oldjoy2_read;
}

static MACHINE_DRIVER_START( snes )

	/* driver data */
	MDRV_DRIVER_DATA(snes_state)

	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", G65816, MCLK_NTSC/6)	/* 2.68 MHz, also 3.58 MHz */
	MDRV_CPU_PROGRAM_MAP(snes_map)

	MDRV_CPU_ADD("soundcpu", SPC700, 1024000)	/* 1.024 MHz */
	MDRV_CPU_PROGRAM_MAP(spc_map)

	//MDRV_QUANTUM_TIME(HZ(48000))
	MDRV_QUANTUM_PERFECT_CPU("maincpu")

	MDRV_MACHINE_START(snes_mess)
	MDRV_MACHINE_RESET(snes_mess)

	/* video hardware */
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(snes)

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)

	MDRV_SCREEN_RAW_PARAMS(DOTCLK_NTSC, SNES_HTOTAL, 0, SNES_SCR_WIDTH, SNES_VTOTAL_NTSC, 0, SNES_SCR_HEIGHT_NTSC)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
	MDRV_SOUND_ADD("custom", SNES, 0)
	MDRV_SOUND_ROUTE(0, "lspeaker", 1.00)
	MDRV_SOUND_ROUTE(1, "rspeaker", 1.00)

	MDRV_IMPORT_FROM(snes_cartslot)
MACHINE_DRIVER_END

static SUPERFX_CONFIG( snes_superfx_config )
{
	DEVCB_LINE(snes_extern_irq_w)	/* IRQ line from cart */
};

static MACHINE_DRIVER_START( snessfx )
	MDRV_IMPORT_FROM(snes)

	MDRV_CPU_ADD("superfx", SUPERFX, 21480000)	/* 21.48MHz */
	MDRV_CPU_PROGRAM_MAP(superfx_map)
	MDRV_CPU_CONFIG(snes_superfx_config)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( snespal )
	MDRV_IMPORT_FROM(snes)
	MDRV_CPU_REPLACE("maincpu", G65816, MCLK_PAL/6)

	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_RAW_PARAMS(DOTCLK_PAL, SNES_HTOTAL, 0, SNES_SCR_WIDTH, SNES_VTOTAL_PAL, 0, SNES_SCR_HEIGHT_PAL)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( snespsfx )
	MDRV_IMPORT_FROM(snespal)

	MDRV_CPU_ADD("superfx", SUPERFX, 21480000)	/* 21.48MHz */
	MDRV_CPU_PROGRAM_MAP(superfx_map)
	MDRV_CPU_CONFIG(snes_superfx_config)
MACHINE_DRIVER_END

/*************************************
 *
 *  ROM definition(s)
 *
 *************************************/

ROM_START( snes )
	ROM_REGION( 0x1000000, "maincpu", ROMREGION_ERASE00 )

	ROM_REGION( 0x100, "user5", 0 )		/* IPL ROM */
	ROM_LOAD( "spc700.rom", 0, 0x40, CRC(44bb3a40) SHA1(97e352553e94242ae823547cd853eecda55c20f0) )	/* boot rom */

	ROM_REGION( 0x1000, "addons", 0 )		/* add-on chip ROMs (DSP, SFX, etc) */
	ROM_LOAD( "dsp1data.bin", 0x000000, 0x000800, CRC(4b02d66d) SHA1(1534f4403d2a0f68ba6e35186fe7595d33de34b1) )
	ROM_LOAD( "dsp3data.bin", 0x000800, 0x000800, CRC(4a1c5453) SHA1(2f69c652109938cde21df5eb89890bf090256dbb) )

	ROM_REGION( MAX_SNES_CART_SIZE, "cart", ROMREGION_ERASE00 )
ROM_END

ROM_START( snessfx )
	ROM_REGION( 0x1000000, "maincpu", ROMREGION_ERASE00 )

	ROM_REGION( 0x100, "user5", 0 )		/* IPL ROM */
	ROM_LOAD( "spc700.rom", 0, 0x40, CRC(44bb3a40) SHA1(97e352553e94242ae823547cd853eecda55c20f0) )	/* boot rom */

	ROM_REGION( 0x1000, "addons", 0 )		/* add-on chip ROMs (DSP, SFX, etc) */
	ROM_LOAD( "dsp1data.bin", 0x000000, 0x000800, CRC(4b02d66d) SHA1(1534f4403d2a0f68ba6e35186fe7595d33de34b1) )

	ROM_REGION( MAX_SNES_CART_SIZE, "cart", ROMREGION_ERASE00 )
ROM_END

ROM_START( snespal )
	ROM_REGION( 0x1000000, "maincpu", ROMREGION_ERASE00 )

	ROM_REGION( 0x100, "user5", 0 )		/* IPL ROM */
	ROM_LOAD( "spc700.rom", 0, 0x40, CRC(44bb3a40) SHA1(97e352553e94242ae823547cd853eecda55c20f0) )	/* boot rom */

	ROM_REGION( 0x1000, "addons", 0 )		/* add-on chip ROMs (DSP, SFX, etc) */
	ROM_LOAD( "dsp1data.bin", 0x000000, 0x000800, CRC(4b02d66d) SHA1(1534f4403d2a0f68ba6e35186fe7595d33de34b1) )
	ROM_LOAD( "dsp3data.bin", 0x000800, 0x000800, CRC(4a1c5453) SHA1(2f69c652109938cde21df5eb89890bf090256dbb) )

	ROM_REGION( MAX_SNES_CART_SIZE, "cart", ROMREGION_ERASE00 )
ROM_END

ROM_START( snespsfx )
	ROM_REGION( 0x1000000, "maincpu", ROMREGION_ERASE00 )

	ROM_REGION( 0x100, "user5", 0 )		/* IPL ROM */
	ROM_LOAD( "spc700.rom", 0, 0x40, CRC(44bb3a40) SHA1(97e352553e94242ae823547cd853eecda55c20f0) )	/* boot rom */

	ROM_REGION( 0x1000, "addons", 0 )		/* add-on chip ROMs (DSP, SFX, etc) */
	ROM_LOAD( "dsp1data.bin", 0x000000, 0x000800, CRC(4b02d66d) SHA1(1534f4403d2a0f68ba6e35186fe7595d33de34b1) )

	ROM_REGION( MAX_SNES_CART_SIZE, "cart", ROMREGION_ERASE00 )
ROM_END

ROM_START( sfcbox )
	ROM_REGION( 0x1000000, "maincpu", ROMREGION_ERASE00 )

	/* atm, we only load these here, but we don't really emulate anything */
	ROM_REGION( 0x10000, "grom", 0 )
	ROM_LOAD( "grom1-1.bin", 0x0000, 0x8000, CRC(333bf9a7) SHA1(5d0cd9ca29e5580c3eebe9f136839987c879f979) )
	ROM_LOAD( "grom1-3.bin", 0x8000, 0x8000, CRC(ebec4c1c) SHA1(d638ef1486b4c0b3d4d5b666929ca7947e16efad) )

	ROM_REGION( 0x80000, "atrom", 0 )
	ROM_LOAD( "atrom-4s-0.smc", 0x00000, 0x80000, CRC(ad3ec05c) SHA1(a3d336db585fe02a37c323422d9db6a33fd489a6) )

	ROM_REGION( 0x100, "user5", 0 )		/* IPL ROM */
	ROM_LOAD( "spc700.rom", 0x00, 0x40, CRC(44bb3a40) SHA1(97e352553e94242ae823547cd853eecda55c20f0) )	/* boot rom */

	ROM_REGION( 0x1000, "addons", ROMREGION_ERASE00 )		/* add-on chip ROMs (DSP, SFX, etc) */
	ROM_REGION( MAX_SNES_CART_SIZE, "cart", ROMREGION_ERASE00 )
ROM_END

/*************************************
 *
 *  Game driver(s)
 *
 *************************************/

/*    YEAR  NAME      PARENT  COMPAT  MACHINE   INPUT  INIT  COMPANY     FULLNAME                                      FLAGS */
CONS( 1989, snes,     0,      0,      snes,     snes,  0,    "Nintendo", "Super Nintendo Entertainment System / Super Famicom (NTSC)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
CONS( 1989, snessfx,  snes,   0,      snessfx,  snes,  0,    "Nintendo", "Super Nintendo Entertainment System / Super Famicom (NTSC, w/SuperFX)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
CONS( 1991, snespal,  snes,   0,      snespal,  snes,  0,    "Nintendo", "Super Nintendo Entertainment System (PAL)",  GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
CONS( 1991, snespsfx, snes,   0,      snespsfx, snes,  0,    "Nintendo", "Super Nintendo Entertainment System (PAL, w/SuperFX)",  GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
CONS( 199?, sfcbox,   snes,   0,      snes,     snes,  0,    "Nintendo", "Super Famicom Box (NTSC)", GAME_NOT_WORKING )
