#include "emu.h"
#include "machine/coleco.h"

void coleco_scan_paddles(running_machine &machine, int *joy_status0, int *joy_status1)
{
	UINT8 analog1 = 0x00;
	UINT8 analog2 = 0x00;
	UINT8 ctrl_sel = input_port_read_safe(machine, "CTRLSEL", 0);

	/* which controller shall we read? */
	if ((ctrl_sel & 0x07) == 0x03)	// Driving controller
		analog1 = input_port_read_safe(machine, "DRIV", 0);

	else
	{
		if ((ctrl_sel & 0x07) == 0x02)	// Super Action Controller P1
			analog1 = input_port_read_safe(machine, "SAC_SLIDE1", 0);

		if ((ctrl_sel & 0x70) == 0x20)	// Super Action Controller P2
			analog2 = input_port_read_safe(machine, "SAC_SLIDE2", 0);

		/* In principle, even if not supported by any game, I guess we could have two Super
        Action Controllers plugged into the Roller controller ports. Since I found no info
        about the behavior of sliders in such a configuration, we overwrite SAC sliders with
        the Roller trackball inputs and actually use the latter ones, when both are selected. */
		if (ctrl_sel & 0x80)				// Roller controller
		{
			analog1 = input_port_read_safe(machine, "ROLLER_X", 0);
			analog2 = input_port_read_safe(machine, "ROLLER_Y", 0);
		}
	}

    if (analog1 == 0)
		*joy_status0 = 0;
    else if (analog1 & 0x08)
		*joy_status0 = -1;
    else
		*joy_status0 = 1;

    if (analog2 == 0)
		*joy_status1 = 0;
    else if (analog2 & 0x08)
		*joy_status1 = -1;
    else
		*joy_status1 = 1;
}


UINT8 coleco_paddle1_read(running_machine &machine, int joy_mode, int joy_status)
{
	UINT8 ctrl_sel = input_port_read_safe(machine, "CTRLSEL", 0);

	/* is there a controller connected to port1? */
	if ((ctrl_sel & 0x07) != 0x01 )
	{
		/* Keypad and fire 1 (SAC Yellow Button) */
		if (joy_mode == 0)
		{
			UINT8 data = 0x0f;	/* No key pressed by default */
			UINT16 ipt1 = 0x00;

			if ((ctrl_sel & 0x07) == 0x00)			// colecovision controller
				ipt1 = input_port_read(machine, "KEYPAD1");
			else if ((ctrl_sel & 0x07) == 0x02)		// super action controller controller
				ipt1 = input_port_read(machine, "SAC_KPD1");

			/* Numeric pad buttons are not independent on a real ColecoVision, if you push more
            than one, a real ColecoVision think that it is a third button, so we are going to emulate
            the right behaviour */
			/* Super Action Controller additional buttons are read in the same way */
			if ((ctrl_sel & 0x07) != 0x03) /* If Driving Controller enabled -> no keypad 1*/
			{
				if (!(ipt1 & 0x0001)) data &= 0x0a; /* 0 */
				if (!(ipt1 & 0x0002)) data &= 0x0d; /* 1 */
				if (!(ipt1 & 0x0004)) data &= 0x07; /* 2 */
				if (!(ipt1 & 0x0008)) data &= 0x0c; /* 3 */
				if (!(ipt1 & 0x0010)) data &= 0x02; /* 4 */
				if (!(ipt1 & 0x0020)) data &= 0x03; /* 5 */
				if (!(ipt1 & 0x0040)) data &= 0x0e; /* 6 */
				if (!(ipt1 & 0x0080)) data &= 0x05; /* 7 */
				if (!(ipt1 & 0x0100)) data &= 0x01; /* 8 */
				if (!(ipt1 & 0x0200)) data &= 0x0b; /* 9 */
				if (!(ipt1 & 0x0400)) data &= 0x06; /* # */
				if (!(ipt1 & 0x0800)) data &= 0x09; /* . */
				if (!(ipt1 & 0x1000)) data &= 0x04; /* Blue Action Button */
				if (!(ipt1 & 0x2000)) data &= 0x08; /* Purple Action Button */
			}

			return ((ipt1 & 0x4000) >> 8) | 0x30 | (data);
		}
		/* Joystick and fire 2 (SAC Red Button) */
		else
		{
			UINT8 data = 0x0f;

			if ((ctrl_sel & 0x07) == 0x00)			// colecovision controller
				data = input_port_read(machine, "JOY1") & 0xcf;
			else if ((ctrl_sel & 0x07) == 0x02)		// super action controller controller
				data = input_port_read(machine, "SAC_JOY1") & 0xcf;

			 /* If any extra contoller enabled */
			if ((ctrl_sel & 0x80) || ((ctrl_sel & 0x07) == 0x02) || ((ctrl_sel & 0x07) == 0x03))
			{
				if (joy_status == 0) data |= 0x30; /* Spinner Move Left*/
				else if (joy_status == 1) data |= 0x20; /* Spinner Move Right */
			}

			return data | 0x80;
		}
	}

	return 0x0f;
}


UINT8 coleco_paddle2_read(running_machine &machine, int joy_mode, int joy_status)
{
	UINT8 ctrl_sel = input_port_read_safe(machine, "CTRLSEL", 0);

	/* is there a controller connected to port2? */
	if ((ctrl_sel & 0x70) != 0x10 )
	{
		/* Keypad and fire 1 */
		if (joy_mode == 0)
		{
			UINT8 data = 0x0f;	/* No key pressed by default */
			UINT16 ipt2 = 0x00;

			if ((ctrl_sel & 0x70) == 0x00)			// colecovision controller
				ipt2 = input_port_read(machine, "KEYPAD2");
			else if ((ctrl_sel & 0x70) == 0x20)		// super action controller controller
				ipt2 = input_port_read(machine, "SAC_KPD2");

			/* Numeric pad buttons are not independent on a real ColecoVision, if you push more
            than one, a real ColecoVision think that it is a third button, so we are going to emulate
            the right behaviour */
			/* Super Action Controller additional buttons are read in the same way */
			if (!(ipt2 & 0x0001)) data &= 0x0a; /* 0 */
			if (!(ipt2 & 0x0002)) data &= 0x0d; /* 1 */
			if (!(ipt2 & 0x0004)) data &= 0x07; /* 2 */
			if (!(ipt2 & 0x0008)) data &= 0x0c; /* 3 */
			if (!(ipt2 & 0x0010)) data &= 0x02; /* 4 */
			if (!(ipt2 & 0x0020)) data &= 0x03; /* 5 */
			if (!(ipt2 & 0x0040)) data &= 0x0e; /* 6 */
			if (!(ipt2 & 0x0080)) data &= 0x05; /* 7 */
			if (!(ipt2 & 0x0100)) data &= 0x01; /* 8 */
			if (!(ipt2 & 0x0200)) data &= 0x0b; /* 9 */
			if (!(ipt2 & 0x0400)) data &= 0x06; /* # */
			if (!(ipt2 & 0x0800)) data &= 0x09; /* . */
			if (!(ipt2 & 0x1000)) data &= 0x04; /* Blue Action Button */
			if (!(ipt2 & 0x2000)) data &= 0x08; /* Purple Action Button */

			return ((ipt2 & 0x4000) >> 8) | 0x30 | (data);
		}
		/* Joystick and fire 2*/
		else
		{
			UINT8 data = 0x0f;

			if ((ctrl_sel & 0x70) == 0x00)			// colecovision controller
				data = input_port_read(machine, "JOY2") & 0xcf;
			else if ((ctrl_sel & 0x70) == 0x20)		// super action controller controller
				data = input_port_read(machine, "SAC_JOY2") & 0xcf;

			/* If Roller Controller or P2 Super Action Controller enabled */
			if ((ctrl_sel & 0x80) || ((ctrl_sel & 0x70) == 0x20))
			{
				if (joy_status == 0) data |= 0x30;
				else if (joy_status == 1) data |= 0x20;
			}

			return data | 0x80;
		}
	}

	return 0x0f;
}



static INPUT_PORTS_START( ctrl1 )
	PORT_START("KEYPAD1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("0 (pad 1)") PORT_CODE(KEYCODE_0_PAD) PORT_CATEGORY(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("1 (pad 1)") PORT_CODE(KEYCODE_1_PAD) PORT_CATEGORY(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("2 (pad 1)") PORT_CODE(KEYCODE_2_PAD) PORT_CATEGORY(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("3 (pad 1)") PORT_CODE(KEYCODE_3_PAD) PORT_CATEGORY(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("4 (pad 1)") PORT_CODE(KEYCODE_4_PAD) PORT_CATEGORY(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("5 (pad 1)") PORT_CODE(KEYCODE_5_PAD) PORT_CATEGORY(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("6 (pad 1)") PORT_CODE(KEYCODE_6_PAD) PORT_CATEGORY(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("7 (pad 1)") PORT_CODE(KEYCODE_7_PAD) PORT_CATEGORY(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("8 (pad 1)") PORT_CODE(KEYCODE_8_PAD) PORT_CATEGORY(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("9 (pad 1)") PORT_CODE(KEYCODE_9_PAD) PORT_CATEGORY(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("# (pad 1)") PORT_CODE(KEYCODE_MINUS_PAD) PORT_CATEGORY(1)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME(". (pad 1)") PORT_CODE(KEYCODE_PLUS_PAD) PORT_CATEGORY(1)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) PORT_CATEGORY(1)
	PORT_BIT( 0xb000, IP_ACTIVE_LOW, IPT_UNKNOWN ) PORT_CATEGORY(1)

	PORT_START("JOY1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1) PORT_CATEGORY(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_CATEGORY(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1) PORT_CATEGORY(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1) PORT_CATEGORY(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) PORT_CATEGORY(1)
	PORT_BIT( 0xb0, IP_ACTIVE_LOW, IPT_UNKNOWN ) PORT_CATEGORY(1)
INPUT_PORTS_END


static INPUT_PORTS_START( ctrl2 )
	PORT_START("KEYPAD2")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("0 (pad 2)") PORT_CATEGORY(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("1 (pad 2)") PORT_CATEGORY(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("2 (pad 2)") PORT_CATEGORY(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("3 (pad 2)") PORT_CATEGORY(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("4 (pad 2)") PORT_CATEGORY(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("5 (pad 2)") PORT_CATEGORY(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("6 (pad 2)") PORT_CATEGORY(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("7 (pad 2)") PORT_CATEGORY(2)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("8 (pad 2)") PORT_CATEGORY(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("9 (pad 2)") PORT_CATEGORY(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("# (pad 2)") PORT_CATEGORY(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME(". (pad 2)") PORT_CATEGORY(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) PORT_CATEGORY(2)
	PORT_BIT( 0xb000, IP_ACTIVE_LOW, IPT_UNKNOWN ) PORT_CATEGORY(2)

	PORT_START("JOY2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2) PORT_CATEGORY(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_CATEGORY(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2) PORT_CATEGORY(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2) PORT_CATEGORY(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_CATEGORY(2)
	PORT_BIT( 0xb0, IP_ACTIVE_LOW, IPT_UNKNOWN ) PORT_CATEGORY(2)
INPUT_PORTS_END


static INPUT_PORTS_START( sac1 )
	PORT_START("SAC_KPD1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("0 (SAC pad 1)") PORT_CODE(KEYCODE_0_PAD) PORT_CATEGORY(3)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("1 (SAC pad 1)") PORT_CODE(KEYCODE_1_PAD) PORT_CATEGORY(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("2 (SAC pad 1)") PORT_CODE(KEYCODE_2_PAD) PORT_CATEGORY(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("3 (SAC pad 1)") PORT_CODE(KEYCODE_3_PAD) PORT_CATEGORY(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("4 (SAC pad 1)") PORT_CODE(KEYCODE_4_PAD) PORT_CATEGORY(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("5 (SAC pad 1)") PORT_CODE(KEYCODE_5_PAD) PORT_CATEGORY(3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("6 (SAC pad 1)") PORT_CODE(KEYCODE_6_PAD) PORT_CATEGORY(3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("7 (SAC pad 1)") PORT_CODE(KEYCODE_7_PAD) PORT_CATEGORY(3)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("8 (SAC pad 1)") PORT_CODE(KEYCODE_8_PAD) PORT_CATEGORY(3)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("9 (SAC pad 1)") PORT_CODE(KEYCODE_9_PAD) PORT_CATEGORY(3)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("# (SAC pad 1)") PORT_CODE(KEYCODE_MINUS_PAD) PORT_CATEGORY(3)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME(". (SAC pad 1)") PORT_CODE(KEYCODE_PLUS_PAD) PORT_CATEGORY(3)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Blue Action Button P1") PORT_PLAYER(1) PORT_CATEGORY(3)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Purple Action Button P1") PORT_PLAYER(1) PORT_CATEGORY(3)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Orange Action Button P1") PORT_PLAYER(1) PORT_CATEGORY(3)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN ) PORT_CATEGORY(3)

	PORT_START("SAC_JOY1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1) PORT_CATEGORY(3)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_CATEGORY(3)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1) PORT_CATEGORY(3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1) PORT_CATEGORY(3)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Yellow Action Button P1") PORT_PLAYER(1) PORT_CATEGORY(3)
	PORT_BIT( 0xb0, IP_ACTIVE_LOW, IPT_UNKNOWN ) PORT_CATEGORY(3)

	PORT_START("SAC_SLIDE1")	// SAC P1 slider
	PORT_BIT( 0x0f, 0x00, IPT_DIAL ) PORT_SENSITIVITY(20) PORT_KEYDELTA(10) PORT_CODE_DEC(KEYCODE_L) PORT_CODE_INC(KEYCODE_J) PORT_RESET PORT_PLAYER(1) PORT_CATEGORY(3)
INPUT_PORTS_END


static INPUT_PORTS_START( sac2 )
	PORT_START("SAC_KPD2")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("0 (SAC pad 2)") PORT_CATEGORY(4)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("1 (SAC pad 2)") PORT_CATEGORY(4)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("2 (SAC pad 2)") PORT_CATEGORY(4)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("3 (SAC pad 2)") PORT_CATEGORY(4)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("4 (SAC pad 2)") PORT_CATEGORY(4)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("5 (SAC pad 2)") PORT_CATEGORY(4)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("6 (SAC pad 2)") PORT_CATEGORY(4)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("7 (SAC pad 2)") PORT_CATEGORY(4)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("8 (SAC pad 2)") PORT_CATEGORY(4)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("9 (SAC pad 2)") PORT_CATEGORY(4)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("# (SAC pad 2)") PORT_CATEGORY(4)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME(". (SAC pad 2)") PORT_CATEGORY(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Blue Action Button P2") PORT_PLAYER(2) PORT_CATEGORY(4)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Purple Action Button P2") PORT_PLAYER(2) PORT_CATEGORY(4)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Orange Action Button P2") PORT_PLAYER(2) PORT_CATEGORY(4)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN ) PORT_CATEGORY(4)

	PORT_START("SAC_JOY2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2) PORT_CATEGORY(4)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_CATEGORY(4)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2) PORT_CATEGORY(4)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2) PORT_CATEGORY(4)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Yellow Action Button P2") PORT_PLAYER(2) PORT_CATEGORY(4)
	PORT_BIT( 0xb0, IP_ACTIVE_LOW, IPT_UNKNOWN ) PORT_CATEGORY(4)

	PORT_START("SAC_SLIDE2")	// SAC P2 slider
	PORT_BIT( 0x0f, 0x00, IPT_DIAL ) PORT_SENSITIVITY(20) PORT_KEYDELTA(10) PORT_CODE_DEC(KEYCODE_I) PORT_CODE_INC(KEYCODE_K) PORT_RESET PORT_PLAYER(2) PORT_CATEGORY(4)
INPUT_PORTS_END


static INPUT_PORTS_START( driving )
	PORT_START("DRIV")	// Driving Controller
	PORT_BIT( 0x0f, 0x00, IPT_DIAL ) PORT_SENSITIVITY(20) PORT_KEYDELTA(10) PORT_CODE_DEC(KEYCODE_L) PORT_CODE_INC(KEYCODE_J) PORT_RESET PORT_CATEGORY(5)

//  PORT_START("IN8")   //
//  PORT_BIT( 0x0f, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(20) PORT_KEYDELTA(10) PORT_CODE_DEC(KEYCODE_I) PORT_CODE_INC(KEYCODE_K) PORT_PLAYER(2) PORT_CATEGORY(5)
INPUT_PORTS_END


static INPUT_PORTS_START( roller )
	PORT_START("ROLLER_X")	// Roller Controller X Axis
	PORT_BIT( 0x0f, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(20) PORT_KEYDELTA(10) PORT_CODE_DEC(KEYCODE_L) PORT_CODE_INC(KEYCODE_J) PORT_RESET PORT_CATEGORY(6)

	PORT_START("ROLLER_Y")	// Roller Controller Y Axis
	PORT_BIT( 0x0f, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(20) PORT_KEYDELTA(10) PORT_CODE_DEC(KEYCODE_I) PORT_CODE_INC(KEYCODE_K) PORT_RESET PORT_CATEGORY(6)
INPUT_PORTS_END


INPUT_PORTS_START( coleco )
	PORT_START("CTRLSEL")  /* Select Controller Type */
	PORT_CATEGORY_CLASS( 0x07, 0x00, "Port 1 Controller" )
	PORT_CATEGORY_ITEM(  0x01, DEF_STR( None ),				0 )
	PORT_CATEGORY_ITEM(  0x00, "ColecoVision Controller",	1 )
	PORT_CATEGORY_ITEM(  0x02, "Super Action Controller",	3 )
	PORT_CATEGORY_ITEM(  0x03, "Driving Controller",		5 )
	PORT_CATEGORY_CLASS( 0x70, 0x00, "Port 2 Controller" )
	PORT_CATEGORY_ITEM(  0x10, DEF_STR( None ),				0 )
	PORT_CATEGORY_ITEM(  0x00, "ColecoVision Controller",	2 )
	PORT_CATEGORY_ITEM(  0x20, "Super Action Controller",	4 )
	PORT_CATEGORY_CLASS( 0x80, 0x00, "Extra Controller" )
	PORT_CATEGORY_ITEM(  0x00, DEF_STR( None ),				0 )
	PORT_CATEGORY_ITEM(  0x80, "Roller Controller",			6 )

	PORT_INCLUDE( ctrl1 )
	PORT_INCLUDE( ctrl2 )
	PORT_INCLUDE( sac1 )
	PORT_INCLUDE( sac2 )
	PORT_INCLUDE( driving )
	PORT_INCLUDE( roller )
INPUT_PORTS_END
