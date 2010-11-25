/******************************************************************************
 PeT mess@utanet.at march 2002
******************************************************************************/

#include "emu.h"
#include "cpu/upd7810/upd7810.h"
#include "devices/cartslot.h"

#include "includes/gmaster.h"


#define MAIN_XTAL	12000000


static READ8_HANDLER( gmaster_io_r )
{
	gmaster_state *state = space->machine->driver_data<gmaster_state>();
    UINT8 data = 0;
    if (state->machine.ports[2] & 1)
	{
		data = memory_region(space->machine, "maincpu")[0x4000 + offset];
		logerror("%.4x external memory %.4x read %.2x\n", (int)cpu_get_reg(space->cpu, CPUINFO_INT_PC), 0x4000 + offset, data);
    }
	else
	{
		switch (offset)
		{
		case 1:
			data=state->video.pixels[state->video.y][state->video.x];
			logerror("%.4x lcd x:%.2x y:%.2x %.4x read %.2x\n", (int)cpu_get_reg(space->cpu, CPUINFO_INT_PC), state->video.x, state->video.y, 0x4000 + offset, data);
			if (!(state->video.mode) && state->video.delayed)
				state->video.x++;
			state->video.delayed = TRUE;
			break;
		default:
			logerror("%.4x memory %.4x read %.2x\n", (int)cpu_get_reg(space->cpu, CPUINFO_INT_PC), 0x4000 + offset, data);
		}
    }
    return data;
}

#define BLITTER_Y ((state->machine.ports[2]&4)|(state->video.data[0]&3))

static WRITE8_HANDLER( gmaster_io_w )
{
	gmaster_state *state = space->machine->driver_data<gmaster_state>();
    if (state->machine.ports[2] & 1)
	{
		memory_region(space->machine, "maincpu")[0x4000 + offset] = data;
		logerror("%.4x external memory %.4x written %.2x\n", (int)cpu_get_reg(space->cpu, CPUINFO_INT_PC), 0x4000 + offset, data);
	}
	else
	{
		switch (offset)
		{
		case 0:
			state->video.delayed=FALSE;
			logerror("%.4x lcd %.4x written %.2x\n", (int)cpu_get_reg(space->cpu, CPUINFO_INT_PC), 0x4000 + offset, data);
			// e2 af a4 a0 a9 falling block init for both halves
			if ((data & 0xfc) == 0xb8)
			{
				state->video.index = 0;
				state->video.data[state->video.index] = data;
				state->video.y = BLITTER_Y;
			}
			else if ((data & 0xc0) == 0)
			{
				state->video.x = data;
			}
			else if ((data & 0xf0) == 0xe0)
			{
				state->video.mode = (data & 0xe) ? FALSE : TRUE;
			}
			state->video.data[state->video.index] = data;
			state->video.index = (state->video.index + 1) & 7;
			break;
		case 1:
			state->video.delayed = FALSE;
			if (state->video.x < ARRAY_LENGTH(state->video.pixels[0])) // continental galaxy flutlicht
				state->video.pixels[state->video.y][state->video.x] = data;
			logerror("%.4x lcd x:%.2x y:%.2x %.4x written %.2x\n",
				(int)cpu_get_reg(space->cpu, CPUINFO_INT_PC), state->video.x, state->video.y, 0x4000 + offset, data);
			state->video.x++;
/* 02 b8 1a
   02 bb 1a
   02 bb 22
   04 b8 12
   04 b8 1a
   04 b8 22
   04 b9 12
   04 b9 1a
   04 b9 22
   02 bb 12
    4000 e0
    rr w rr w rr w rr w rr w rr w rr w rr w
    4000 ee
*/
			break;
		default:
			logerror("%.4x memory %.4x written %.2x\n", (int)cpu_get_reg(space->cpu, CPUINFO_INT_PC), 0x4000 + offset, data);
		}
	}
}

static READ8_HANDLER( gmaster_port_r )
{
	//gmaster_state *state = space->machine->driver_data<gmaster_state>();
//  UINT8 data = state->machine.ports[offset];
    UINT8 data = 0xff;
    switch (offset)
	{
	case UPD7810_PORTA:
		data = input_port_read(space->machine, "JOY");
		break;
	default:
		logerror("%.4x port %d read %.2x\n", (int)cpu_get_reg(space->cpu, CPUINFO_INT_PC), offset, data);
    }
    return data;
}

static WRITE8_HANDLER( gmaster_port_w )
{
	gmaster_state *state = space->machine->driver_data<gmaster_state>();
    state->machine.ports[offset] = data;
    logerror("%.4x port %d written %.2x\n", (int)cpu_get_reg(space->cpu, CPUINFO_INT_PC), offset, data);
    switch (offset)
	{
		case UPD7810_PORTC:
			state->video.y = BLITTER_Y;
			break;
    }
}

static ADDRESS_MAP_START( gmaster_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE( 0x4000, 0x7fff) AM_READWRITE(gmaster_io_r, gmaster_io_w)
	AM_RANGE(0x8000, 0xfeff) AM_ROM
	AM_RANGE(0xff00, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START(gmaster_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(UPD7810_PORTA, UPD7810_PORTF) AM_READWRITE(gmaster_port_r, gmaster_port_w )
ADDRESS_MAP_END

static INPUT_PORTS_START( gmaster )
    PORT_START("JOY")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_NAME("B")
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_NAME("A")
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SELECT) PORT_NAME("Select")
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START) PORT_NAME("Start")
INPUT_PORTS_END

/* palette in red, green, blue tribles */
static const unsigned char gmaster_palette[2][3] =
{
#if 1
    { 130, 159, 166 },
    { 45,45,43 }
#else
    { 255,255,255 },
    { 0, 0, 0 }
#endif
};

static PALETTE_INIT( gmaster )
{
	int i;

	for (i = 0; i < 2; i++)
	{
		palette_set_color_rgb(machine, i, gmaster_palette[i][0], gmaster_palette[i][1], gmaster_palette[i][2]);
	}
}

static VIDEO_UPDATE( gmaster )
{
	gmaster_state *state = screen->machine->driver_data<gmaster_state>();
    int x,y;
//  plot_box(bitmap, 0, 0, 64/*bitmap->width*/, bitmap->height, 0); //xmess rounds up to 64 pixel
    for (y = 0; y < ARRAY_LENGTH(state->video.pixels); y++)
	{
		for (x = 0; x < ARRAY_LENGTH(state->video.pixels[0]); x++)
		{
			UINT8 d = state->video.pixels[y][x];
			UINT16 *line;

			line = BITMAP_ADDR16(bitmap, (y * 8), x);
			line[0] = BIT(d, 0);
			line = BITMAP_ADDR16(bitmap, (y * 8 + 1), x);
			line[0] = BIT(d, 1);
			line = BITMAP_ADDR16(bitmap, (y * 8 + 2), x);
			line[0] = BIT(d, 2);
			line = BITMAP_ADDR16(bitmap, (y * 8 + 3), x);
			line[0] = BIT(d, 3);
			line = BITMAP_ADDR16(bitmap, (y * 8 + 4), x);
			line[0] = BIT(d, 4);
			line = BITMAP_ADDR16(bitmap, (y * 8 + 5), x);
			line[0] = BIT(d, 5);
			line = BITMAP_ADDR16(bitmap, (y * 8 + 6), x);
			line[0] = BIT(d, 6);
			line = BITMAP_ADDR16(bitmap, (y * 8 + 7), x);
			line[0] = BIT(d, 7);
		}
    }
    return 0;
}

static DEVICE_IMAGE_LOAD( gmaster_cart )
{
	UINT32 size;

	if (image.software_entry() == NULL)
	{
		size = image.length();

		if (size > (memory_region_length(image.device().machine, "maincpu") - 0x8000))
		{
			image.seterror(IMAGE_ERROR_UNSPECIFIED, "Unsupported cartridge size");
			return IMAGE_INIT_FAIL;
		}

		if (image.fread( memory_region(image.device().machine, "maincpu") + 0x8000, size) != size)
		{
			image.seterror(IMAGE_ERROR_UNSPECIFIED, "Unable to fully read from file");
			return IMAGE_INIT_FAIL;
		}

	}
	else
	{
		size = image.get_software_region_length("rom");
		memcpy(memory_region(image.device().machine, "maincpu") + 0x8000, image.get_software_region("rom"), size);
	}

	return IMAGE_INIT_PASS;
}

static INTERRUPT_GEN( gmaster_interrupt )
{
	cputag_set_input_line(device->machine, "maincpu", UPD7810_INTFE1, ASSERT_LINE);
}

static const UPD7810_CONFIG config = {
//  TYPE_78C10, // 78c11 in handheld
	TYPE_7801, // temporarily until 7810 core fixes synchronized
	gmaster_io_callback
};

static MACHINE_CONFIG_START( gmaster, gmaster_state )
	MDRV_CPU_ADD("maincpu", UPD7810, MAIN_XTAL/2/*?*/)
	MDRV_CPU_PROGRAM_MAP(gmaster_mem)
	MDRV_CPU_IO_MAP( gmaster_io)
	MDRV_CPU_CONFIG( config )
	MDRV_CPU_VBLANK_INT("screen", gmaster_interrupt)

	MDRV_SCREEN_ADD("screen", LCD)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_SIZE(64, 64)
	MDRV_SCREEN_VISIBLE_AREA(0, 64-1-3, 0, 64-1)
	MDRV_PALETTE_LENGTH(sizeof(gmaster_palette)/sizeof(gmaster_palette[0]))
	MDRV_VIDEO_UPDATE(gmaster)
	MDRV_PALETTE_INIT(gmaster)
	MDRV_DEFAULT_LAYOUT(layout_lcd)

	MDRV_SPEAKER_STANDARD_MONO("speaker")
	MDRV_SOUND_ADD("custom", GMASTER, 0)
	MDRV_SOUND_ROUTE(0, "speaker", 0.50)

	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_MANDATORY
	MDRV_CARTSLOT_INTERFACE("gmaster_cart")
	MDRV_CARTSLOT_LOAD(gmaster_cart)
	MDRV_SOFTWARE_LIST_ADD("cart_list","gmaster")
MACHINE_CONFIG_END


ROM_START(gmaster)
	ROM_REGION(0x10000,"maincpu", 0)
	ROM_LOAD("gmaster.bin", 0x0000, 0x1000, CRC(05cc45e5) SHA1(05d73638dea9657ccc2791c0202d9074a4782c1e) )
//  ROM_CART_LOAD(0, "bin", 0x8000, 0x7f00, 0)
ROM_END

static DRIVER_INIT( gmaster )
{
	gmaster_state *state = machine->driver_data<gmaster_state>();
	memset(&state->video, 0, sizeof(state->video));
}

/*    YEAR      NAME            PARENT  MACHINE   INPUT     INIT  COMPANY                 FULLNAME */
CONS( 1990, gmaster,       0,          0, gmaster,  gmaster,    gmaster,    "Hartung", "Game Master", GAME_IMPERFECT_SOUND)
