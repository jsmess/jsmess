/*

    TODO:

	- X order
	- freeze (light pen)
	- screen size
	- scanline based update
	- partial updates to allow mid-screen zoom change

*/

#include "emu.h"
#include "includes/vidbrain.h"
#include "cpu/f8/f8.h"

/***************************************************************************
    CONSTANTS
***************************************************************************/

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
#define RAM_XY_HI_B					0x80	// Y value high order and Y order list B

// command register bits
#define COMMAND_YINT_H_O			0x80
#define COMMAND_A_B					0x40
#define COMMAND_Y_ZM				0x20
#define COMMAND_KBD					0x10
#define COMMAND_INT					0x08
#define COMMAND_ENB					0x04
#define COMMAND_FRZ					0x02
#define COMMAND_X_ZM				0x01

/***************************************************************************
    READ/WRITE HANDLERS
***************************************************************************/

/*-------------------------------------------------
    vidbrain_vlsi_r - video VLSI read
-------------------------------------------------*/

READ8_HANDLER( vidbrain_vlsi_r )
{
	vidbrain_state *state = space->machine->driver_data<vidbrain_state>();

	UINT8 data = 0xff;

	switch (offset)
	{
	case REGISTER_X_FREEZE:
		data = state->freeze_x;
		if (LOG) logerror("X-Freeze %02x\n", data);
		break;

	case REGISTER_Y_FREEZE_LOW:
		data = state->freeze_y & 0xff;
		if (LOG) logerror("Y-Freeze Low %02x\n", data);
		break;

	case REGISTER_Y_FREEZE_HIGH:
		/*

			bit		signal		description

			0		Y-F8		Y freeze high order (MSB) bit
			1		Y-C8		current Y counter high order (MSB) bit
			2
			3
			4
			5
			6
			7		O/_E		odd/even field

		*/

		data = (state->field << 7) | (BIT(state->screen->vpos(), 8) << 1) | BIT(state->freeze_y, 8);
		if (LOG) logerror("Y-Freeze High %02x\n", data);
		break;

	case REGISTER_CURRENT_Y_LOW:
		data = state->screen->vpos() & 0xff;
		if (LOG) logerror("Current-Y Low %02x\n", data);
		break;

	default:
		if (offset < 0x90)
			data = state->vlsi_ram[offset];
		else
			if (LOG) logerror("Unknown VLSI read from %02x!\n", offset);
	}

	return data;
}

/*-------------------------------------------------
    set_y_interrupt - set Y interrupt timer
-------------------------------------------------*/

static void set_y_interrupt(vidbrain_state *state)
{
	int scanline = ((state->cmd & COMMAND_YINT_H_O) << 1) | state->y_int;

	state->timer_y_int->adjust(state->screen->time_until_pos(scanline, 0), 0, state->screen->frame_period());
}

/*-------------------------------------------------
    vidbrain_vlsi_w - video VLSI write
-------------------------------------------------*/

WRITE8_HANDLER( vidbrain_vlsi_w )
{
	vidbrain_state *state = space->machine->driver_data<vidbrain_state>();

	switch (offset)
	{
	case REGISTER_Y_INTERRUPT:
		if (LOG) logerror("Y-Interrupt %02x\n", data);

		if (state->y_int != data)
		{
			state->y_int = data;
			set_y_interrupt(state);
		}
		break;

	case REGISTER_FINAL_MODIFIER:
		/*

			bit		signal		description

			0		RED			red
			1		GREEN		green
			2		BLUE		blue
			3		INT 0		intensity 0
			4		INT 1		intensity 1
			5		not used
			6		not used
			7		not used

		*/

		if (LOG) logerror("Final Modifier %02x\n", data);

		state->fmod = data & 0x1f;
		break;

	case REGISTER_BACKGROUND:
		/*

			bit		signal		description

			0		RED			red
			1		GREEN		green
			2		BLUE		blue
			3		INT 0		intensity 0
			4		INT 1		intensity 1
			5		not used
			6		not used
			7		not used

		*/

		if (LOG) logerror("Background %02x\n", data);

		state->bg = data & 0x1f;
		break;

	case REGISTER_COMMAND:
		/*

			bit		signal		description

			0		X-ZM		X zoom
			1		FRZ			freeze
			2		ENB			COMMAND_ENB
			3		INT			COMMAND_INT
			4		KBD			general purpose output
			5		Y-ZM		Y zoom
			6		A/_B		list selection
			7		YINT H.O.	Y COMMAND_INT register high order bit

		*/

		if (LOG) logerror("Command %02x\n", data);
		
		if (BIT(state->cmd, 7) != BIT(data, 7))
		{
			set_y_interrupt(state);
		}

		state->cmd = data;
		break;

	default:
		if (offset < 0x90)
			state->vlsi_ram[offset] = data;
		else
			logerror("Unknown VLSI write %02x to %02x!\n", data, offset);
	}
}

/***************************************************************************
    VIDEO
***************************************************************************/

/*-------------------------------------------------
    PALETTE_INIT( vidbrain )
-------------------------------------------------*/

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

/*-------------------------------------------------
    VIDEO_START( vidbrain )
-------------------------------------------------*/

static VIDEO_START( vidbrain )
{
	vidbrain_state *state = machine->driver_data<vidbrain_state>();
	
	/* register for state saving */
	state_save_register_global_array(machine, state->vlsi_ram);
	state_save_register_global(machine, state->y_int);
	state_save_register_global(machine, state->fmod);
	state_save_register_global(machine, state->bg);
	state_save_register_global(machine, state->cmd);
	state_save_register_global(machine, state->freeze_x);
	state_save_register_global(machine, state->freeze_y);
	state_save_register_global(machine, state->field);

	/* get the devices */
	state->screen = machine->device<screen_device>(SCREEN_TAG);
	state->timer_y_int = machine->device<timer_device>(TIMER_Y_INT_TAG);
}

/*-------------------------------------------------
    VIDEO_UPDATE( vidbrain )
-------------------------------------------------*/

#define RAM(_offset) state->vlsi_ram[_offset + i]

static VIDEO_UPDATE( vidbrain )
{
	vidbrain_state *state = screen->machine->driver_data<vidbrain_state>();
	address_space *program = cputag_get_address_space(screen->machine, F3850_TAG, ADDRESS_SPACE_PROGRAM);

	if (!(state->cmd & COMMAND_ENB))
	{
		bitmap_fill(bitmap, cliprect, get_black_pen(screen->machine));
		return 0;
	}

	bitmap_fill(bitmap, cliprect, state->bg);

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
		int y = (state->cmd & COMMAND_A_B) ? y_a : y_b;
		int xord = (state->cmd & COMMAND_A_B) ? xord_a : xord_b;

		if (LOG) logerror("Object %u rp %04x color %u dx %u dy %u xcopy %u x %u y %u xord %u\n", i, rp, color, dx, dy, xcopy, x, y, xord);

		if (rp == 0) continue;
		if (y > 255) continue;

		for (int sy = 0; sy < dy; sy++)
		{
			for (int sx = 0; sx < dx; sx++)
			{
				UINT8 data = program->read_byte(rp);

				for (int bit = 0; bit < 8; bit++)
				{
					int pixel = ((BIT(data, 7) ? color : state->bg) ^ state->fmod) & 0x1f;

					if (state->cmd & COMMAND_Y_ZM)
					{
						if (state->cmd & COMMAND_X_ZM)
						{
							*BITMAP_ADDR16(bitmap, y + (sy * 2), x + (sx * 8) + (bit * 2)) = pixel;
							*BITMAP_ADDR16(bitmap, y + (sy * 2) + 1, x + (sx * 8) + (bit * 2)) = pixel;
							*BITMAP_ADDR16(bitmap, y + (sy * 2), x + (sx * 8) + (bit * 2) + 1) = pixel;
							*BITMAP_ADDR16(bitmap, y + (sy * 2) + 1, x + (sx * 8) + (bit * 2) + 1) = pixel;
						}
						else
						{
							*BITMAP_ADDR16(bitmap, y + (sy * 2), x + (sx * 8) + bit) = pixel;
							*BITMAP_ADDR16(bitmap, y + (sy * 2) + 1, x + (sx * 8) + bit) = pixel;
						}
					}
					else
					{
						if (state->cmd & COMMAND_X_ZM)
						{
							*BITMAP_ADDR16(bitmap, y + sy, x + (sx * 8) + (bit * 2)) = pixel;
							*BITMAP_ADDR16(bitmap, y + sy, x + (sx * 8) + (bit * 2) + 1) = pixel;
						}
						else
						{
							*BITMAP_ADDR16(bitmap, y + sy, x + (sx * 8) + bit) = pixel;
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

/*-------------------------------------------------
    gfx_layout vidbrain_charlayout
-------------------------------------------------*/

static const gfx_layout vidbrain_charlayout =
{
	8, 7,					/* 8 x 7 characters */
	59,						/* 59 characters */
	1,						/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ STEP8(0,8) },
	8*7					/* every char takes 7 bytes */
};

/*-------------------------------------------------
    GFXDECODE( vidbrain )
-------------------------------------------------*/

static GFXDECODE_START( vidbrain )
	GFXDECODE_ENTRY( F3850_TAG, 0x2010, vidbrain_charlayout, 0, 1 )
GFXDECODE_END

/*-------------------------------------------------
    TIMER_DEVICE_CALLBACK( scanline_tick )
-------------------------------------------------*/

static TIMER_DEVICE_CALLBACK( y_int_tick )
{
	vidbrain_state *state = timer.machine->driver_data<vidbrain_state>();

	if ((state->cmd & COMMAND_INT) && !(state->cmd & COMMAND_FRZ))
	{
//		f3853_set_external_interrupt_in_line(state->smi, 1);
		state->ext_int_latch = 1;
		vidbrain_interrupt_check(timer.machine);
	}
}

/*-------------------------------------------------
    TIMER_DEVICE_CALLBACK( field_tick )
-------------------------------------------------*/

static TIMER_DEVICE_CALLBACK( field_tick )
{
	vidbrain_state *state = timer.machine->driver_data<vidbrain_state>();

	state->field = !state->field;
}

/***************************************************************************
    MACHINE CONFIGURATION
***************************************************************************/

/*-------------------------------------------------
    MACHINE_CONFIG_FRAGMENT( vidbrain_video )
-------------------------------------------------*/

MACHINE_CONFIG_FRAGMENT( vidbrain_video )
    /* video hardware */
    MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
    MDRV_SCREEN_REFRESH_RATE(30)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(320, 525)
    MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 0, 262-1)
	MDRV_GFXDECODE(vidbrain)
    MDRV_PALETTE_LENGTH(32)
    MDRV_PALETTE_INIT(vidbrain)

    MDRV_VIDEO_START(vidbrain)
    MDRV_VIDEO_UPDATE(vidbrain)

	MDRV_TIMER_ADD(TIMER_Y_INT_TAG, y_int_tick)
	MDRV_TIMER_ADD_PERIODIC(TIMER_FIELD_TAG, field_tick, HZ(60))
MACHINE_CONFIG_END
