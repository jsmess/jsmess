/*************************************************************************
 *
 *		pc_joy.h
 *
 *		joystick port
 *
 *************************************************************************/

#include "pc_joy.h"

static double JOY_time = 0.0;


READ8_HANDLER ( pc_JOY_r )
{
	UINT8 data = 0;
	int delta;
	double new_time = timer_get_time();
	int joystick_port = port_tag_to_index("pc_joy");

	if (joystick_port >= 0)
	{
		data = readinputport(joystick_port + 0) ^ 0xf0;

		/* timer overflow? */
		if (new_time - JOY_time > 0.01)
		{
			/* do nothing */
		}
		else
		{
			delta = (int)( 256 * 1000 * (new_time - JOY_time) );
			if (readinputport(joystick_port + 1) < delta) data &= ~0x01;
			if (readinputport(joystick_port + 2) < delta) data &= ~0x02;
			if (readinputport(joystick_port + 3) < delta) data &= ~0x04;
			if (readinputport(joystick_port + 4) < delta) data &= ~0x08;
		}
	}
	return data;
}



WRITE8_HANDLER ( pc_JOY_w )
{
	JOY_time = timer_get_time();
}



INPUT_PORTS_START( pc_joystick_none )
	PORT_START_TAG("pc_joy")
    PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )
	PORT_START
    PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )
	PORT_START
    PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )
	PORT_START
    PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )
	PORT_START
    PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )
INPUT_PORTS_END



INPUT_PORTS_START( pc_joystick )
	PORT_START_TAG("pc_joy")
	PORT_BIT ( 0xf, 0xf,	 IPT_UNUSED ) 
	PORT_BIT( 0x0010, 0x0000, IPT_BUTTON1) PORT_NAME("Joystick 1 Button 1")
	PORT_BIT( 0x0020, 0x0000, IPT_BUTTON2) PORT_NAME("Joystick 1 Button 2")
	PORT_BIT( 0x0040, 0x0000, IPT_BUTTON1) PORT_NAME("Joystick 2 Button 1") PORT_CODE(JOYCODE_2_BUTTON1) PORT_PLAYER(2)
	PORT_BIT( 0x0080, 0x0000, IPT_BUTTON2) PORT_NAME("Joystick 2 Button 2") PORT_CODE(JOYCODE_2_BUTTON2) PORT_PLAYER(2)
		
	PORT_START
	PORT_BIT(0xff,0x80,IPT_AD_STICK_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(1) PORT_MINMAX(1,0xff) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_1_LEFT) PORT_CODE_INC(JOYCODE_1_RIGHT) PORT_REVERSE
		
	PORT_START
	PORT_BIT(0xff,0x80,IPT_AD_STICK_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(1) PORT_MINMAX(1,0xff) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_DEC(JOYCODE_1_UP) PORT_CODE_INC(JOYCODE_1_DOWN) PORT_REVERSE
		
	PORT_START
	PORT_BIT(0xff,0x80,IPT_AD_STICK_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(1) PORT_MINMAX(1,0xff) PORT_CODE_DEC(JOYCODE_2_LEFT) PORT_CODE_INC(JOYCODE_2_RIGHT) PORT_PLAYER(2) PORT_REVERSE
		
	PORT_START
	PORT_BIT(0xff,0x80,IPT_AD_STICK_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(1) PORT_MINMAX(1,0xff) PORT_CODE_DEC(JOYCODE_2_UP) PORT_CODE_INC(JOYCODE_2_DOWN) PORT_PLAYER(2) PORT_REVERSE
INPUT_PORTS_END

