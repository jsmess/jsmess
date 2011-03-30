/*

    TODO:

    - X order
    - freeze (light pen)
    - screen size
    - scanline based update

    http://zone.ni.com/devzone/cda/tut/p/id/4750

*/

#include "emu.h"
#include "includes/vidbrain.h"
#include "cpu/f8/f8.h"



//**************************************************************************
//  CONSTANTS / MACROS
//**************************************************************************

#define LOG 0


// write-only registers
#define REGISTER_COMMAND			0xf7
#define REGISTER_BACKGROUND			0xf5
#define REGISTER_FINAL_MODIFIER		0xf2
#define REGISTER_Y_INTERRUPT		0xf0


// read-only registers
#define REGISTER_X_FREEZE			0xf8
#define REGISTER_Y_FREEZE_LOW		0xf9
#define REGISTER_Y_FREEZE_HIGH		0xfa
#define REGISTER_CURRENT_Y_LOW		0xfb


// read/write registers - RAM memory
#define RAM_RP_LO					0x00	// cartridge pointer low order
#define RAM_RP_HI					0x10	// cartridge pointer high order and color
#define RAM_DX						0x20	// dX, intensity, X-copy
#define RAM_DY						0x30	// dY
#define RAM_X						0x40	// X value
#define RAM_Y_LO_A					0x50	// Y value low order list A
#define RAM_Y_LO_B					0x60	// Y value low order list B
#define RAM_XY_HI_A					0x70	// Y value high order and X order list A
#define RAM_XY_HI_B					0x80	// Y value high order and X order list B


// command register bits
#define COMMAND_YINT_H_O			0x80
#define COMMAND_A_B					0x40
#define COMMAND_Y_ZM				0x20
#define COMMAND_KBD					0x10
#define COMMAND_INT					0x08
#define COMMAND_ENB					0x04
#define COMMAND_FRZ					0x02
#define COMMAND_X_ZM				0x01


#define IS_CHANGED(_bit) \
	((m_cmd & _bit) != (data & _bit))

#define RAM(_offset) \
	m_vlsi_ram[_offset + i]

#define IS_VISIBLE(_y) \
	((_y >= cliprect.min_y) && (_y <= cliprect.max_y))

#define DRAW_PIXEL(_scanline, _dot) \
	if (IS_VISIBLE(_scanline)) *BITMAP_ADDR16(&bitmap, (_scanline), _dot) = pixel;



//**************************************************************************
//  READ/WRITE HANDLERS
//**************************************************************************

//-------------------------------------------------
//  get_field_vpos - get scanline within field
//-------------------------------------------------

int vidbrain_state::get_field_vpos()
{
	int vpos = m_screen->vpos();

	if (vpos >= 262)
	{
		// even field
		vpos -= 262;
	}

	return vpos;
}


//-------------------------------------------------
//  get_field - get video field
//-------------------------------------------------

int vidbrain_state::get_field()
{
	return m_screen->vpos() < 262;
}


//-------------------------------------------------
//  vlsi_r - video VLSI read
//-------------------------------------------------

READ8_MEMBER( vidbrain_state::vlsi_r )
{
	UINT8 data = 0xff;

	switch (offset)
	{
	case REGISTER_X_FREEZE:
		data = m_freeze_x;

		if (LOG) logerror("X-Freeze %02x\n", data);
		break;

	case REGISTER_Y_FREEZE_LOW:
		data = m_freeze_y & 0xff;

		if (LOG) logerror("Y-Freeze Low %02x\n", data);
		break;

	case REGISTER_Y_FREEZE_HIGH:
		/*

            bit     signal      description

            0       Y-F8        Y freeze high order (MSB) bit
            1       Y-C8        current Y counter high order (MSB) bit
            2
            3
            4
            5
            6
            7       O/_E        odd/even field

        */

		data = (get_field() << 7) | (BIT(get_field_vpos(), 8) << 1) | BIT(m_freeze_y, 8);

		if (LOG) logerror("Y-Freeze High %02x\n", data);
		break;

	case REGISTER_CURRENT_Y_LOW:
		data = get_field_vpos() & 0xff;

		if (LOG) logerror("Current-Y Low %02x\n", data);
		break;

	default:
		if (offset < 0x90)
			data = m_vlsi_ram[offset];
		else
			if (LOG) logerror("Unknown VLSI read from %02x!\n", offset);
	}

	return data;
}


//-------------------------------------------------
//  set_y_interrupt - set Y interrupt timer
//-------------------------------------------------

void vidbrain_state::set_y_interrupt()
{
	int scanline = ((m_cmd & COMMAND_YINT_H_O) << 1) | m_y_int;

	m_timer_y_odd->adjust(m_screen->time_until_pos(scanline), 0, m_screen->frame_period());
	m_timer_y_even->adjust(m_screen->time_until_pos(scanline + 262), 0, m_screen->frame_period());
}


//-------------------------------------------------
//  do_partial_update - update screen
//-------------------------------------------------

void vidbrain_state::do_partial_update()
{
	int vpos = m_screen->vpos();

	if (LOG) logerror("Partial screen update at scanline %u\n", vpos);

	m_screen->update_partial(vpos);
}


//-------------------------------------------------
//  vlsi_w - video VLSI write
//-------------------------------------------------

WRITE8_MEMBER( vidbrain_state::vlsi_w )
{
	switch (offset)
	{
	case REGISTER_Y_INTERRUPT:
		if (LOG) logerror("Y-Interrupt %02x\n", data);

		if (m_y_int != data)
		{
			m_y_int = data;
			set_y_interrupt();
		}
		break;

	case REGISTER_FINAL_MODIFIER:
		/*

            bit     signal      description

            0       RED         red
            1       GREEN       green
            2       BLUE        blue
            3       INT 0       intensity 0
            4       INT 1       intensity 1
            5       not used
            6       not used
            7       not used

        */

		if (LOG) logerror("Final Modifier %02x\n", data);

		do_partial_update();
		m_fmod = data & 0x1f;
		break;

	case REGISTER_BACKGROUND:
		/*

            bit     signal      description

            0       RED         red
            1       GREEN       green
            2       BLUE        blue
            3       INT 0       intensity 0
            4       INT 1       intensity 1
            5       not used
            6       not used
            7       not used

        */

		if (LOG) logerror("Background %02x\n", data);

		do_partial_update();
		m_bg = data & 0x1f;
		break;

	case REGISTER_COMMAND:
		/*

            bit     signal      description

            0       X-ZM        X zoom
            1       FRZ         freeze
            2       ENB         COMMAND_ENB
            3       INT         COMMAND_INT
            4       KBD         general purpose output
            5       Y-ZM        Y zoom
            6       A/_B        list selection
            7       YINT H.O.   Y COMMAND_INT register high order bit

        */

		if (LOG) logerror("Command %02x\n", data);

		if (IS_CHANGED(COMMAND_YINT_H_O))
		{
			set_y_interrupt();
		}

		if (IS_CHANGED(COMMAND_A_B) || IS_CHANGED(COMMAND_Y_ZM) || IS_CHANGED(COMMAND_X_ZM))
		{
			do_partial_update();
		}

		m_cmd = data;
		break;

	default:
		if (offset < 0x90)
			m_vlsi_ram[offset] = data;
		else
			logerror("Unknown VLSI write %02x to %02x!\n", data, offset);
	}
}



//**************************************************************************
//  VIDEO
//**************************************************************************

//-------------------------------------------------
//  PALETTE_INIT( vidbrain )
//-------------------------------------------------

static PALETTE_INIT( vidbrain )
{
	static const UINT8 INTENSITY[] = { 0x90, 0xb0, 0xd0, 0xff };

	for (int i = 0; i < 4; i++)
	{
		int offset = i * 8;
		UINT8 value = INTENSITY[i];

		palette_set_color_rgb(machine, offset + 0, 0,		0,		0);		// black
		palette_set_color_rgb(machine, offset + 1, value,	0,		0);		// red
		palette_set_color_rgb(machine, offset + 2, 0,		value,	0);		// green
		palette_set_color_rgb(machine, offset + 3, value,	value,	0);		// red-green
		palette_set_color_rgb(machine, offset + 4, 0,		0,		value); // blue
		palette_set_color_rgb(machine, offset + 5, value,	0,		value); // red-blue
		palette_set_color_rgb(machine, offset + 6, 0,		value,	value); // green-blue
		palette_set_color_rgb(machine, offset + 7, value,	value,	value); // white
	}
}


//-------------------------------------------------
//  VIDEO_START( vidbrain )
//-------------------------------------------------

void vidbrain_state::video_start()
{
	// register for state saving
	state_save_register_global_array(m_machine, m_vlsi_ram);
	state_save_register_global(m_machine, m_y_int);
	state_save_register_global(m_machine, m_fmod);
	state_save_register_global(m_machine, m_bg);
	state_save_register_global(m_machine, m_cmd);
	state_save_register_global(m_machine, m_freeze_x);
	state_save_register_global(m_machine, m_freeze_y);
	state_save_register_global(m_machine, m_field);
}


//-------------------------------------------------
//  SCREEN_UPDATE( vidbrain )
//-------------------------------------------------

bool vidbrain_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	address_space *program = m_maincpu->memory().space(AS_PROGRAM);

	if (!(m_cmd & COMMAND_ENB))
	{
		bitmap_fill(&bitmap, &cliprect, get_black_pen(m_machine));
		return 0;
	}

	bitmap_fill(&bitmap, &cliprect, m_bg);

	for (int i = 0; i < 16; i++)
	{
		UINT16 rp = ((RAM(RAM_RP_HI) << 8) | RAM(RAM_RP_LO)) & 0x1fff;
		int color = ((RAM(RAM_DX) >> 2) | (BIT(RAM(RAM_RP_HI), 5) << 2) | (BIT(RAM(RAM_RP_HI), 6) << 1) | (BIT(RAM(RAM_RP_HI), 7))) & 0x1f;
		int dx = RAM(RAM_DX) & 0x1f;
		int dy = RAM(RAM_DY);
		int xcopy = BIT(RAM(RAM_DX), 7);
		int x = RAM(RAM_X);
		int xord_a = RAM(RAM_Y_LO_A) & 0x0f;
		int xord_b = RAM(RAM_Y_LO_B) & 0x0f;
		int y_a = ((RAM(RAM_XY_HI_A) & 0x80) << 1) | RAM(RAM_Y_LO_A);
		int y_b = ((RAM(RAM_XY_HI_B) & 0x80) << 1) | RAM(RAM_Y_LO_B);
		int y = (m_cmd & COMMAND_A_B) ? y_a : y_b;
		int xord = (m_cmd & COMMAND_A_B) ? xord_a : xord_b;

		if (LOG) logerror("Object %u rp %04x color %u dx %u dy %u xcopy %u x %u y %u xord %u\n", i, rp, color, dx, dy, xcopy, x, y, xord);

		if (rp == 0) continue;
		if (y > 262) continue;

		for (int sy = 0; sy < dy; sy++)
		{
			for (int sx = 0; sx < dx; sx++)
			{
				UINT8 data = program->read_byte(rp);

				for (int bit = 0; bit < 8; bit++)
				{
					int pixel = ((BIT(data, 7) ? color : m_bg) ^ m_fmod) & 0x1f;

					if (m_cmd & COMMAND_Y_ZM)
					{
						int scanline = y + (sy * 2);

						if (m_cmd & COMMAND_X_ZM)
						{
							int dot = (x * 2) + (sx * 16) + (bit * 2);

							DRAW_PIXEL(scanline, dot);
							DRAW_PIXEL(scanline, dot + 1);
							DRAW_PIXEL(scanline + 1, dot);
							DRAW_PIXEL(scanline + 1, dot + 1);
						}
						else
						{
							int dot = x + (sx * 8) + bit;

							DRAW_PIXEL(scanline, dot);
							DRAW_PIXEL(scanline + 1, dot);
						}
					}
					else
					{
						int scanline = y + sy;

						if (m_cmd & COMMAND_X_ZM)
						{
							int dot = (x * 2) + (sx * 16) + (bit * 2);

							DRAW_PIXEL(scanline, dot);
							DRAW_PIXEL(scanline, dot + 1);
						}
						else
						{
							int dot = x + (sx * 8) + bit;

							DRAW_PIXEL(scanline, dot);
						}
					}

					data <<= 1;
				}

				if (!xcopy) rp++;
			}

			if (xcopy) rp++;
		}
	}

	return 0;
}


//-------------------------------------------------
//  gfx_layout vidbrain_charlayout
//-------------------------------------------------

static const gfx_layout vidbrain_charlayout =
{
	8, 7,
	59,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ STEP8(0,8) },
	8*7
};


//-------------------------------------------------
//  GFXDECODE( vidbrain )
//-------------------------------------------------

static GFXDECODE_START( vidbrain )
	GFXDECODE_ENTRY( F3850_TAG, 0x2010, vidbrain_charlayout, 0, 1 )
GFXDECODE_END


//-------------------------------------------------
//  TIMER_DEVICE_CALLBACK( y_int_tick )
//-------------------------------------------------

static TIMER_DEVICE_CALLBACK( y_int_tick )
{
	vidbrain_state *state = timer.machine().driver_data<vidbrain_state>();

	if ((state->m_cmd & COMMAND_INT) && !(state->m_cmd & COMMAND_FRZ))
	{
		if (LOG) logerror("Y-Interrupt at scanline %u\n", state->m_screen->vpos());
//      f3853_set_external_interrupt_in_line(state->m_smi, 1);
		state->m_ext_int_latch = 1;
		state->interrupt_check();
	}
}



//**************************************************************************
//  MACHINE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( vidbrain_video )
//-------------------------------------------------

MACHINE_CONFIG_FRAGMENT( vidbrain_video )
    MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_RAW_PARAMS(XTAL_14_31818MHz, 455, 0, 320, 525, 0, 243)

	MCFG_GFXDECODE(vidbrain)

    MCFG_PALETTE_LENGTH(32)
    MCFG_PALETTE_INIT(vidbrain)

	MCFG_TIMER_ADD(TIMER_Y_ODD_TAG, y_int_tick)
	MCFG_TIMER_ADD(TIMER_Y_EVEN_TAG, y_int_tick)
MACHINE_CONFIG_END
