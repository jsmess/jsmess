/*************************************************************************
 *
 *      pc_joy.h
 *
 *      joystick port
 *
 *************************************************************************/

#include "emu.h"
#include "pc_joy.h"
#include "memconv.h"


static attotime JOY_time;


READ8_HANDLER ( pc_JOY_r )
{
	UINT8 data = 0;
	int delta;
	attotime new_time = space->machine().time();
	const input_port_config *joystick_port = space->machine().m_portlist.find("pc_joy");

	if (joystick_port != NULL)
	{
		data = input_port_read_direct(joystick_port) ^ 0xf0;

		/* timer overflow? */
		if ((new_time - JOY_time) > attotime::from_msec(10))
		{
			/* do nothing */
		}
		else
		{
			delta = ((new_time - JOY_time) * 256 * 1000).seconds;
			if (input_port_read(space->machine(), "pc_joy_1") < delta) data &= ~0x01;
			if (input_port_read(space->machine(), "pc_joy_2") < delta) data &= ~0x02;
			if (input_port_read(space->machine(), "pc_joy_3") < delta) data &= ~0x04;
			if (input_port_read(space->machine(), "pc_joy_4") < delta) data &= ~0x08;
		}
	}
	return data;
}



WRITE8_HANDLER ( pc_JOY_w )
{
	JOY_time = space->machine().time();
}



READ16_HANDLER ( pc16le_JOY_r ) { return read16le_with_read8_handler(pc_JOY_r, space, offset, mem_mask); }
WRITE16_HANDLER ( pc16le_JOY_w ) { write16le_with_write8_handler(pc_JOY_w, space, offset, data, mem_mask); }



INPUT_PORTS_START( pc_joystick_none )
	PORT_START("pc_joy")
    PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )
	PORT_START("pc_joy_1")
    PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )
	PORT_START("pc_joy_2")
    PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )
	PORT_START("pc_joy_3")
    PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )
	PORT_START("pc_joy_4")
    PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )
INPUT_PORTS_END



INPUT_PORTS_START( pc_joystick )
	PORT_START("pc_joy")
	PORT_BIT ( 0xf, 0xf,	 IPT_UNUSED )
	PORT_BIT( 0x0010, 0x0000, IPT_BUTTON1) PORT_NAME("Joystick 1 Button 1")
	PORT_BIT( 0x0020, 0x0000, IPT_BUTTON2) PORT_NAME("Joystick 1 Button 2")
	PORT_BIT( 0x0040, 0x0000, IPT_BUTTON1) PORT_NAME("Joystick 2 Button 1") PORT_CODE(JOYCODE_BUTTON1) PORT_PLAYER(2)
	PORT_BIT( 0x0080, 0x0000, IPT_BUTTON2) PORT_NAME("Joystick 2 Button 2") PORT_CODE(JOYCODE_BUTTON2) PORT_PLAYER(2)

	PORT_START("pc_joy_1")
	PORT_BIT(0xff,0x80,IPT_AD_STICK_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(1) PORT_MINMAX(1,0xff) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH) PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH) PORT_REVERSE

	PORT_START("pc_joy_2")
	PORT_BIT(0xff,0x80,IPT_AD_STICK_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(1) PORT_MINMAX(1,0xff) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH) PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH) PORT_REVERSE

	PORT_START("pc_joy_3")
	PORT_BIT(0xff,0x80,IPT_AD_STICK_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(1) PORT_MINMAX(1,0xff) PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH) PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH) PORT_PLAYER(2) PORT_REVERSE

	PORT_START("pc_joy_4")
	PORT_BIT(0xff,0x80,IPT_AD_STICK_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(1) PORT_MINMAX(1,0xff) PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH) PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH) PORT_PLAYER(2) PORT_REVERSE
INPUT_PORTS_END

