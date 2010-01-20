/******************************************************************************

driver by ?

PeT mess@utanet.at around February 2008:
 added apfm1000 cartridge loading
 fixed apfm1000 pads
 added apf video mode

todo for apf m1000:
 add exact cpu/video timing. memory controller+6847 memory operations hold cpu
  (backgammon relies on exact video timing)
 support special cartridges (basic, space destroyer)

 ******************************************************************************/
#include "emu.h"
#include "cpu/m6800/m6800.h"
#include "video/m6847.h"

#include "machine/6821pia.h"
#include "machine/wd17xx.h"
#include "devices/flopdrv.h"
#include "formats/basicdsk.h"
#include "devices/cassette.h"
#include "devices/cartslot.h"
#include "formats/apf_apt.h"
#include "sound/speaker.h"

 /*
0000- 2000-2003 PIA of M1000. is itself repeated until 3fff.
She controls " keypads ", the way of video and the loudspeaker. Putting to 0 one of the 4 least
significant bits of 2002, the corresponding line of keys is ***reflxed mng in 2000 (32 keys altogether).
The other four bits of 2002 control the video way. Bit 0 of 2003 controls the interruptions. Bit 3 of 2003 controls
the loudspeaker. 4000-47ff ROM of M1000. is repeated until 5fff and in e000-ffff. 6000-6003 PIA of the APF.
is itself repeated until 63ff. She controls the keyboard of the APF and the cassette. The value of the
number that represents the three bits less significant of 6002 they determine the line of eight
keys that it is ***reflxed mng in 6000. Bit 4 of 6002 controls the motor of the cassette. Bit 5 of
6002 activates or deactivates the recording. Bit 5 of 6002 indicates the level of recording in
the cassette. 6400-64ff would be the interface Here series, optional. 6500-66ff would be the
disk controller Here, optional. 6800-77ff ROM (" cartridge ") 7800-7fff Probably ROM. The BASIC
leaves frees this zone. 8000-9fff ROM (" cartridge ") A000-BFFF ram (8 Kb) C000-DFFF ram
additional (8 Kb) E000-FFFF ROM of M1000 (to see 4000-47FF) The interruption activates with
the vertical synchronism of the video. routine that it executes is in the ROM of M1000
and puts the video in way text during a short interval, so that the first line is seen of
text screen in the superior part of the graphical screen.
*/

/* 6600, 6500-6503 wd179x disc controller? 6400, 6401 */
static unsigned char keyboard_data;
static unsigned char pad_data;
static UINT8 mc6847_css = 0;
static UINT8 *apf_video_ram;

static unsigned char apf_ints;
static void apf_update_ints(running_machine *machine);


static READ8_DEVICE_HANDLER( apf_mc6847_videoram_r )
{
	mc6847_css_w(device, mc6847_css && BIT(apf_video_ram[offset + 0x200], 6));
	mc6847_inv_w(device, BIT(apf_video_ram[offset + 0x200], 6));
	mc6847_as_w(device, BIT(apf_video_ram[offset + 0x200], 7));

	return apf_video_ram[offset + 0x200];
}

static WRITE_LINE_DEVICE_HANDLER( apf_mc6847_fs_w )
{
	if (state)
	  	apf_ints |= 0x10;
	else
		apf_ints &= ~0x10;

	apf_update_ints(device->machine);
}

static VIDEO_UPDATE( apf )
{
	const device_config *mc6847 = devtag_get_device(screen->machine, "mc6847");
	return mc6847_update(mc6847, bitmap, cliprect);
}

static  READ8_DEVICE_HANDLER(apf_m1000_pia_in_a_func)
{

  UINT8 data=~0;
  if (!(pad_data & 0x08))
    data &= input_port_read(device->machine, "joy3");
  if (!(pad_data & 0x04))
    data &= input_port_read(device->machine, "joy2");
  if (!(pad_data & 0x02))
    data &= input_port_read(device->machine, "joy1");
  if (!(pad_data & 0x01))
    data &= input_port_read(device->machine, "joy0");

	return data;
}

static  READ8_DEVICE_HANDLER(apf_m1000_pia_in_b_func)
{
	return 0x0ff;
}

static  READ8_DEVICE_HANDLER(apf_m1000_pia_in_ca1_func)
{
	return 0;
}

static  READ8_DEVICE_HANDLER(apf_m1000_pia_in_cb1_func)
{
	return 0x00;
}

static  READ8_DEVICE_HANDLER(apf_m1000_pia_in_ca2_func)
{
	return 0;
}

static  READ8_DEVICE_HANDLER(apf_m1000_pia_in_cb2_func)
{
	return 0x00;
}


static WRITE8_DEVICE_HANDLER(apf_m1000_pia_out_a_func)
{
}

//static unsigned char previous_mode;

static WRITE8_DEVICE_HANDLER( apf_m1000_pia_out_b_func )
{
	const device_config *mc6847 = devtag_get_device(device->machine, "mc6847");

	/* bit 7..4 video control -- TODO: bit 5 and 4? */
	mc6847_ag_w(mc6847, BIT(data, 7));
	mc6847_gm0_w(mc6847, BIT(data, 6));

	/* bit 3..0 keypad line select */
	pad_data = data;
}

static WRITE_LINE_DEVICE_HANDLER(apf_m1000_pia_out_ca2_func)
{
	mc6847_css = state;
}

static WRITE8_DEVICE_HANDLER(apf_m1000_pia_out_cb2_func)
{
	const device_config *speaker = devtag_get_device(device->machine, "speaker");
	speaker_level_w(speaker, data);
}

/* use bit 0 to identify state of irq from pia 0 */
/* use bit 1 to identify state of irq from pia 0 */
/* use bit 2 to identify state of irq from pia 1 */
/* use bit 3 to identify state of irq from pia 1 */
/* use bit 4 to identify state of irq from video */

static void apf_update_ints(running_machine *machine)
{
	cputag_set_input_line(machine, "maincpu", 0, apf_ints ? HOLD_LINE : CLEAR_LINE);
}

static WRITE_LINE_DEVICE_HANDLER( apf_m1000_irq_a_func )
{
	if (state)
	{
	  	apf_ints|=1;
	}
	else
	{
		apf_ints&=~1;
	}

	apf_update_ints(device->machine);
}


static WRITE_LINE_DEVICE_HANDLER( apf_m1000_irq_b_func )
{
	//logerror("pia 0 irq b %d\n",state);

	if (state)
	{
	  	apf_ints|=2;
	}
	else
	{
		apf_ints&=~2;
	}

	apf_update_ints(device->machine);

}

static const pia6821_interface apf_m1000_pia_interface=
{
	DEVCB_HANDLER(apf_m1000_pia_in_a_func),
	DEVCB_HANDLER(apf_m1000_pia_in_b_func),
	DEVCB_HANDLER(apf_m1000_pia_in_ca1_func),
	DEVCB_HANDLER(apf_m1000_pia_in_cb1_func),
	DEVCB_HANDLER(apf_m1000_pia_in_ca2_func),
	DEVCB_HANDLER(apf_m1000_pia_in_cb2_func),
	DEVCB_HANDLER(apf_m1000_pia_out_a_func),
	DEVCB_HANDLER(apf_m1000_pia_out_b_func),
	DEVCB_LINE(apf_m1000_pia_out_ca2_func),
	DEVCB_HANDLER(apf_m1000_pia_out_cb2_func),
	DEVCB_LINE(apf_m1000_irq_a_func),
	DEVCB_LINE(apf_m1000_irq_b_func)
};


static  READ8_DEVICE_HANDLER(apf_imagination_pia_in_a_func)
{
	return keyboard_data;
}

static READ8_DEVICE_HANDLER(apf_imagination_pia_in_b_func)
{
	unsigned char data;

	data = 0x000;

	if (cassette_input(devtag_get_device(device->machine, "cassette")) > 0.0038)
		data =(1<<7);

	return data;
}

static  READ8_DEVICE_HANDLER(apf_imagination_pia_in_ca1_func)
{
	return 0x00;
}

static  READ8_DEVICE_HANDLER(apf_imagination_pia_in_cb1_func)
{
	return 0x00;
}

static  READ8_DEVICE_HANDLER(apf_imagination_pia_in_ca2_func)
{
	return 0x00;
}

static  READ8_DEVICE_HANDLER(apf_imagination_pia_in_cb2_func)
{
	return 0x00;
}


static WRITE8_DEVICE_HANDLER(apf_imagination_pia_out_a_func)
{
}

static WRITE8_DEVICE_HANDLER(apf_imagination_pia_out_b_func)
{
	/* bits 2..0 = keyboard line */
	/* bit 3 = ??? */
	/* bit 4 = cassette motor */
	/* bit 5 = ?? */
	/* bit 6 = cassette write data */
	/* bit 7 = ??? */

	int keyboard_line;
	static const char *const keynames[] = { "key0", "key1", "key2", "key3", "key4", "key5", "key6", "key7" };

	keyboard_line = data & 0x07;
	keyboard_data = input_port_read(device->machine, keynames[keyboard_line]);

	/* bit 4: cassette motor control */
	cassette_change_state(devtag_get_device(device->machine, "cassette"),
		(data & 0x10) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED,
		CASSETTE_MASK_MOTOR);

	/* bit 6: cassette write */
	cassette_output(devtag_get_device(device->machine, "cassette"),
		(data & 0x40) ? -1.0 : 1.0);
}

static WRITE8_DEVICE_HANDLER(apf_imagination_pia_out_ca2_func)
{
}

static WRITE8_DEVICE_HANDLER(apf_imagination_pia_out_cb2_func)
{
}

static WRITE_LINE_DEVICE_HANDLER( apf_imagination_irq_a_func )
{
	if (state)
	{
		apf_ints|=4;
	}
	else
	{
		apf_ints&=~4;
	}

	apf_update_ints(device->machine);

}

static WRITE_LINE_DEVICE_HANDLER( apf_imagination_irq_b_func )
{
	if (state)
	{
		apf_ints|=8;
	}
	else
	{
		apf_ints&=~8;
	}

	apf_update_ints(device->machine);

}

static const pia6821_interface apf_imagination_pia_interface=
{
	DEVCB_HANDLER(apf_imagination_pia_in_a_func),
	DEVCB_HANDLER(apf_imagination_pia_in_b_func),
	DEVCB_HANDLER(apf_imagination_pia_in_ca1_func),
	DEVCB_HANDLER(apf_imagination_pia_in_cb1_func),
	DEVCB_HANDLER(apf_imagination_pia_in_ca2_func),
	DEVCB_HANDLER(apf_imagination_pia_in_cb2_func),
	DEVCB_HANDLER(apf_imagination_pia_out_a_func),
	DEVCB_HANDLER(apf_imagination_pia_out_b_func),
	DEVCB_HANDLER(apf_imagination_pia_out_ca2_func),
	DEVCB_HANDLER(apf_imagination_pia_out_cb2_func),
	DEVCB_LINE(apf_imagination_irq_a_func),
	DEVCB_LINE(apf_imagination_irq_b_func)
};


static MACHINE_START( apf_imagination )
{
	apf_ints = 0;
}

static WRITE8_HANDLER(apf_dischw_w)
{
	int drive;
	const device_config *fdc = devtag_get_device(space->machine, "wd179x");

	/* bit 3 is index of drive to select */
	drive = (data>>3) & 0x01;

	wd17xx_set_drive(fdc, drive);

	logerror("disc w %04x %04x\n",offset,data);
}

static  READ8_HANDLER(serial_r)
{
	logerror("serial r %04x\n",offset);
	return 0x00;
}

static WRITE8_HANDLER(serial_w)
{
	logerror("serial w %04x %04x\n",offset,data);
}

static WRITE8_HANDLER(apf_wd179x_command_w)
{
	wd17xx_command_w(devtag_get_device(space->machine, "wd179x"), offset,~data);
}

static WRITE8_HANDLER(apf_wd179x_track_w)
{
	wd17xx_track_w(devtag_get_device(space->machine, "wd179x"), offset,~data);
}

static WRITE8_HANDLER(apf_wd179x_sector_w)
{
	wd17xx_sector_w(devtag_get_device(space->machine, "wd179x"), offset,~data);
}

static WRITE8_HANDLER(apf_wd179x_data_w)
{
	wd17xx_data_w(devtag_get_device(space->machine, "wd179x"), offset,~data);
}

static READ8_HANDLER(apf_wd179x_status_r)
{
	return ~wd17xx_status_r(devtag_get_device(space->machine, "wd179x"), offset);
}

static READ8_HANDLER(apf_wd179x_track_r)
{
	return ~wd17xx_track_r(devtag_get_device(space->machine, "wd179x"), offset);
}

static READ8_HANDLER(apf_wd179x_sector_r)
{
	return ~wd17xx_sector_r(devtag_get_device(space->machine, "wd179x"), offset);
}

static READ8_HANDLER(apf_wd179x_data_r)
{
	return wd17xx_data_r(devtag_get_device(space->machine, "wd179x"), offset);
}

static ADDRESS_MAP_START(apf_imagination_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x00000, 0x003ff) AM_RAM AM_BASE(&apf_video_ram) AM_MIRROR(0x1c00)
	AM_RANGE( 0x02000, 0x03fff) AM_DEVREADWRITE("pia_0", pia6821_r, pia6821_w)
	AM_RANGE( 0x04000, 0x047ff) AM_ROM AM_REGION("maincpu", 0x10000) AM_MIRROR(0x1800)
	AM_RANGE( 0x06000, 0x063ff) AM_DEVREADWRITE("pia_1", pia6821_r, pia6821_w)
	AM_RANGE( 0x06400, 0x064ff) AM_READWRITE(serial_r, serial_w)
	AM_RANGE( 0x06500, 0x06500) AM_READWRITE(apf_wd179x_status_r, apf_wd179x_command_w)
	AM_RANGE( 0x06501, 0x06501) AM_READWRITE(apf_wd179x_track_r, apf_wd179x_track_w)
	AM_RANGE( 0x06502, 0x06502) AM_READWRITE(apf_wd179x_sector_r, apf_wd179x_sector_w)
	AM_RANGE( 0x06503, 0x06503) AM_READWRITE(apf_wd179x_data_r, apf_wd179x_data_w)
	AM_RANGE( 0x06600, 0x06600) AM_WRITE( apf_dischw_w)
	AM_RANGE( 0x06800, 0x077ff) AM_ROM
	AM_RANGE( 0x07800, 0x07fff) AM_NOP
	AM_RANGE( 0x08000, 0x09fff) AM_ROM
	AM_RANGE( 0x0a000, 0x0dfff) AM_RAM
	AM_RANGE( 0x0e000, 0x0e7ff) AM_ROM AM_REGION("maincpu", 0x10000) AM_MIRROR(0x1800)
ADDRESS_MAP_END

static ADDRESS_MAP_START(apf_m1000_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x00000, 0x003ff) AM_RAM AM_BASE(&apf_video_ram)  AM_MIRROR(0x1c00)
	AM_RANGE( 0x02000, 0x03fff) AM_DEVREADWRITE("pia_0", pia6821_r, pia6821_w)
	AM_RANGE( 0x04000, 0x047ff) AM_ROM AM_REGION("maincpu", 0x10000) AM_MIRROR(0x1800)
	AM_RANGE( 0x06800, 0x077ff) AM_ROM
	AM_RANGE( 0x08000, 0x09fff) AM_ROM AM_REGION("maincpu", 0x8000)
	AM_RANGE( 0x0a000, 0x0dfff) AM_RAM
	AM_RANGE( 0x0e000, 0x0e7ff) AM_ROM AM_REGION("maincpu", 0x10000) AM_MIRROR(0x1800)
ADDRESS_MAP_END

/* The following input ports definitions are wrong and can't be debugged unless the driver
   is capable of running more cartridges. However each of the controllers supported by the M-1000
   have these features:

   1 8-way joystick
   1 big red fire button on the upper side
   12-keys keypad with the following layout

   7 8 9 0
   4 5 6 Cl
   1 2 3 En

   On the control panel of the M-1000 there are two big buttons: a Reset key and the Power switch

   Reference: http://www.nausicaa.net/~lgreenf/apfpage2.htm
*/

static INPUT_PORTS_START( apf_m1000 )

	/*
       There must be a bug lurking somewhere, because the lines 0-3 are not detected correctly:
       Using another known APF emulator, this simple Basic program can be used to read
       the joysticks and the keyboard:

       10 PRINT KEY$(n);
       20 GOTO 10

       where n = 0, 1 or 2 - 0 = keyboard, 1,2 = joysticks #1 and #2

       When reading the keyboard KEY$(0) returns the character associated to the key, with the
       following exceptions:

       Ctrl =    CHR$(1)
       Rept =    CHR$(2)
       Here Is = CHR$(4)
       Rubout =  CHR$(8)

       When reading the joysticks, KEY$() = "N", "S", "E", "W" for the directions
                                            "0" - "9" for the keypad digits
                                            "?" for "Cl"
                                            "!" for "En"

       Current code doesn't behaves this way...

    */

/*
  ? player right is player 1
 */

	/* line 0 */
	PORT_START("joy0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("PAD 1/RIGHT 1") PORT_CODE(KEYCODE_1) PORT_PLAYER(1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("PAD 1/RIGHT 0") PORT_CODE(KEYCODE_0) PORT_PLAYER(1)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("PAD 1/RIGHT 4") PORT_CODE(KEYCODE_4) PORT_PLAYER(1)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("PAD 1/RIGHT 7") PORT_CODE(KEYCODE_7) PORT_PLAYER(1)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("PAD 2/LEFT 1") PORT_CODE(KEYCODE_1_PAD) PORT_PLAYER(2)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("PAD 2/LEFT 0") PORT_CODE(KEYCODE_0_PAD) PORT_PLAYER(2)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("PAD 2/LEFT 4") PORT_CODE(KEYCODE_4_PAD) PORT_PLAYER(2)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("PAD 2/LEFT 7") PORT_CODE(KEYCODE_7_PAD) PORT_PLAYER(2)

	/* line 1 */
	PORT_START("joy1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_NAME("PAD 1/RIGHT down") PORT_PLAYER(1) PORT_8WAY
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_NAME("PAD 1/RIGHT right") PORT_PLAYER(1) PORT_8WAY
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_NAME("PAD 1/RIGHT up") PORT_PLAYER(1) PORT_8WAY
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_NAME("PAD 1/RIGHT left") PORT_PLAYER(1) PORT_8WAY
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_NAME("PAD 2/LEFT down") PORT_PLAYER(2) PORT_8WAY
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_NAME("PAD 2/LEFT right") PORT_PLAYER(2) PORT_8WAY
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_NAME("PAD 2/LEFT up") PORT_PLAYER(2) PORT_8WAY
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_NAME("PAD 2/LEFT left") PORT_PLAYER(2) PORT_8WAY

	/* line 2 */
	PORT_START("joy2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("PAD 1/RIGHT 3") PORT_CODE(KEYCODE_3) PORT_PLAYER(1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("PAD 1/RIGHT clear") PORT_CODE(KEYCODE_DEL) PORT_PLAYER(1)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("PAD 1/RIGHT 6") PORT_CODE(KEYCODE_6) PORT_PLAYER(1)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("PAD 1/RIGHT 9") PORT_CODE(KEYCODE_9) PORT_PLAYER(1)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("PAD 2/LEFT 3") PORT_CODE(KEYCODE_3_PAD) PORT_PLAYER(2)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("PAD 2/LEFT clear") PORT_CODE(KEYCODE_DEL_PAD) PORT_PLAYER(2)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("PAD 2/LEFT 6") PORT_CODE(KEYCODE_6_PAD) PORT_PLAYER(2)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("PAD 2/LEFT 9") PORT_CODE(KEYCODE_9_PAD) PORT_PLAYER(2)

	/* line 3 */
	PORT_START("joy3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("PAD 1/RIGHT 2") PORT_CODE(KEYCODE_2) PORT_PLAYER(1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("PAD 1/RIGHT enter/fire") PORT_CODE(KEYCODE_ENTER) PORT_PLAYER(1)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("PAD 1/RIGHT 5") PORT_CODE(KEYCODE_5) PORT_PLAYER(1)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("PAD 1/RIGHT 8") PORT_CODE(KEYCODE_8) PORT_PLAYER(1)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("PAD 2/LEFT 2") PORT_CODE(KEYCODE_2_PAD) PORT_PLAYER(2)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("PAD 2/LEFT enter/fire") PORT_CODE(KEYCODE_ENTER_PAD) PORT_PLAYER(2)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("PAD 2/LEFT 5") PORT_CODE(KEYCODE_5_PAD) PORT_PLAYER(2)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("PAD 2/LEFT 8") PORT_CODE(KEYCODE_8_PAD) PORT_PLAYER(2)

//  PORT_INCLUDE( m6847_artifacting ) // breaks apfimag keyboard
INPUT_PORTS_END


static INPUT_PORTS_START( apf_imagination )

	PORT_INCLUDE( apf_m1000 )

	/* Reference: http://www.nausicaa.net/~lgreenf/apfpage2.htm */

	/* keyboard line 0 */
	PORT_START("key0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X")               PORT_CODE(KEYCODE_X)          PORT_CHAR('x')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z")               PORT_CODE(KEYCODE_Z)          PORT_CHAR('z')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q       IF")      PORT_CODE(KEYCODE_Q)          PORT_CHAR('q')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2   \"    LET")   PORT_CODE(KEYCODE_2)          PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A")               PORT_CODE(KEYCODE_A)          PORT_CHAR('a')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1   !   GOSUB")   PORT_CODE(KEYCODE_1)          PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W       STEP")    PORT_CODE(KEYCODE_W)          PORT_CHAR('w')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S")               PORT_CODE(KEYCODE_S)          PORT_CHAR('s')

	/* keyboard line 1 */
	PORT_START("key1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C")               PORT_CODE(KEYCODE_C)          PORT_CHAR('c')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V")               PORT_CODE(KEYCODE_V)          PORT_CHAR('v')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R       READ")    PORT_CODE(KEYCODE_R)          PORT_CHAR('r')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3   #   DATA")    PORT_CODE(KEYCODE_3)          PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F")               PORT_CODE(KEYCODE_F)          PORT_CHAR('f')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4   $   INPUT")   PORT_CODE(KEYCODE_4)          PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E       STOP")    PORT_CODE(KEYCODE_E)          PORT_CHAR('e')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D")               PORT_CODE(KEYCODE_D)          PORT_CHAR('d')

	/* keyboard line 2 */
	PORT_START("key2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N   ^")           PORT_CODE(KEYCODE_N)          PORT_CHAR('n') PORT_CHAR('^')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B")               PORT_CODE(KEYCODE_B)          PORT_CHAR('b')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T       NEXT")    PORT_CODE(KEYCODE_T)          PORT_CHAR('t')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6   &   FOR")     PORT_CODE(KEYCODE_6)          PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G")               PORT_CODE(KEYCODE_G)          PORT_CHAR('g')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5   %   DIM")     PORT_CODE(KEYCODE_5)          PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y       PRINT")   PORT_CODE(KEYCODE_Y)          PORT_CHAR('y')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H")               PORT_CODE(KEYCODE_H)          PORT_CHAR('h')

	/* keyboard line 3 */
	PORT_START("key3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M   ]")           PORT_CODE(KEYCODE_M)          PORT_CHAR('m') PORT_CHAR(']')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",   <")           PORT_CODE(KEYCODE_COMMA)      PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I       LIST")    PORT_CODE(KEYCODE_I)          PORT_CHAR('i')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7   '   RETURN")  PORT_CODE(KEYCODE_7)          PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K   [")           PORT_CODE(KEYCODE_K)          PORT_CHAR('k') PORT_CHAR('[')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8   (   THEN")    PORT_CODE(KEYCODE_8)          PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U       END")     PORT_CODE(KEYCODE_U)          PORT_CHAR('u')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J")               PORT_CODE(KEYCODE_J)          PORT_CHAR('j')

	/* keyboard line 4 */
	PORT_START("key4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/   ?")           PORT_CODE(KEYCODE_SLASH)      PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".   >")           PORT_CODE(KEYCODE_STOP)       PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O   _   REM")     PORT_CODE(KEYCODE_O)          PORT_CHAR('o') PORT_CHAR('_')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0       GOTO")    PORT_CODE(KEYCODE_0)          PORT_CHAR('0')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L   \\")          PORT_CODE(KEYCODE_L)          PORT_CHAR('l') PORT_CHAR('\\')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9   )   ON")      PORT_CODE(KEYCODE_9)          PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P   @   USING")   PORT_CODE(KEYCODE_P)          PORT_CHAR('p') PORT_CHAR('@')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";   +")           PORT_CODE(KEYCODE_COLON)      PORT_CHAR(';') PORT_CHAR('+')

	/* keyboard line 5 */
	PORT_START("key5")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space")           PORT_CODE(KEYCODE_SPACE)      PORT_CHAR(32)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(":   *")           PORT_CODE(KEYCODE_MINUS)      PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Return")          PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(13)
/* Not sure about the following, could be also bit 0x10 or 0x40: using other known APF emulators as benchmark,
   it looks like that Line Feed is not intercepted by KEY$(). Unless a better method is devised (APF assembly?)
   I'll stick with this assignment.
*/
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Line Feed")       PORT_CODE(KEYCODE_OPENBRACE)  PORT_CHAR(10)
	PORT_BIT(0x10, 0x10, IPT_UNUSED)                                                                       /* ??? */
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-   =   RESTORE") PORT_CODE(KEYCODE_EQUALS)     PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT(0x40, 0x40, IPT_UNUSED)                                                                       /* ??? */
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Rubout")          PORT_CODE(KEYCODE_QUOTE)      PORT_CHAR(8)

	/* line 6 */
	PORT_START("key6")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift")     PORT_CODE(KEYCODE_RSHIFT) PORT_CODE(KEYCODE_LSHIFT)     PORT_CHAR(UCHAR_SHIFT_1)
/* This key displays the glyph "[", but a quick test reveals that its ASCII code is 27. */
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Esc")             PORT_CODE(KEYCODE_TAB)        PORT_CHAR(27)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ctrl")            PORT_CODE(KEYCODE_LCONTROL)   PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Rept")            PORT_CODE(KEYCODE_ENTER)      PORT_CHAR(UCHAR_MAMEKEY(TAB))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Break")           PORT_CODE(KEYCODE_BACKSLASH)  PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Here Is")         PORT_CODE(KEYCODE_BACKSPACE)  PORT_CHAR(UCHAR_MAMEKEY(HOME))
/* It's very likely these inputs are actually disconnected: if connected they act as a duplicate of key "X" and "Z" */
//  PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Another X")       PORT_CODE(KEYCODE_8_PAD)
//  PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Another Z")       PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT(0x40, 0x40, IPT_UNUSED)
	PORT_BIT(0x80, 0x80, IPT_UNUSED)

	/* line 7 */
	PORT_START("key7")
	PORT_BIT(0xff, 0xff, IPT_UNUSED)                                                                       /* ??? */

INPUT_PORTS_END



static const cassette_config apf_cassette_config =
{
	apf_cassette_formats,
	NULL,
	(cassette_state)(CASSETTE_PLAY)
};

static FLOPPY_OPTIONS_START(apfimag)
	FLOPPY_OPTION(apfimag, "apd", "APF disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([8])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static const floppy_config apfimag_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_SS_40,
	FLOPPY_OPTIONS_NAME(apfimag),
	DO_NOT_KEEP_GEOMETRY
};

static const mc6847_interface apf_mc6847_intf =
{
	DEVCB_HANDLER(apf_mc6847_videoram_r),
	DEVCB_LINE_VCC,
	DEVCB_LINE_VCC,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(apf_mc6847_fs_w),
	DEVCB_NULL,
	DEVCB_NULL
};

static MACHINE_DRIVER_START( apf_imagination )
	/* basic machine hardware */
	//  MDRV_CPU_ADD("maincpu", M6800, 3750000)        /* 7.8336 MHz, only 6800p type used 1 MHz max*/
	MDRV_CPU_ADD("maincpu", M6800, 1000000 )        /* backgammon uses timing from vertical interrupt to switch between video modes during frame */
	MDRV_CPU_PROGRAM_MAP(apf_imagination_map)
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(M6847_NTSC_FRAMES_PER_SECOND)

	MDRV_MACHINE_START( apf_imagination )

	/* video hardware */
	MDRV_VIDEO_UPDATE(apf)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(320, 25+192+26)
	MDRV_SCREEN_VISIBLE_AREA(0, 319, 1, 239)

	MDRV_MC6847_ADD("mc6847", apf_mc6847_intf)

	MDRV_PIA6821_ADD( "pia_0", apf_m1000_pia_interface )
	MDRV_PIA6821_ADD( "pia_1", apf_imagination_pia_interface )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_CASSETTE_ADD( "cassette", apf_cassette_config )

	MDRV_WD179X_ADD("wd179x", default_wd17xx_interface )

	MDRV_FLOPPY_2_DRIVES_ADD(apfimag_floppy_config)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( apf_m1000 )
	MDRV_IMPORT_FROM( apf_imagination )
	MDRV_CPU_MODIFY( "maincpu" )
	MDRV_CPU_PROGRAM_MAP( apf_m1000_map)

	MDRV_DEVICE_REMOVE( "pia_1" )

	MDRV_DEVICE_REMOVE( "cassette" )
	MDRV_CARTSLOT_ADD("cart")
	MDRV_FLOPPY_2_DRIVES_REMOVE()
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(apfimag)
	ROM_REGION(0x10000+0x0800,"maincpu",0)
	ROM_LOAD("apf_4000.rom",0x010000, 0x00800, CRC(2a331a33) SHA1(387b90882cd0b66c192d9cbaa3bec250f897e4f1))
	ROM_LOAD("basic_68.rom",0x06800, 0x01000, CRC(ef049ab8) SHA1(c4c12aade95dd89a4750fe7f89d57256c93da068))
	ROM_LOAD("basic_80.rom",0x08000, 0x02000, CRC(a4c69fae) SHA1(7f98aa482589bf7c5a26d338fec105e797ba43f6))
ROM_END

ROM_START(apfm1000)
	ROM_REGION(0x10000+0x0800,"maincpu",0)
	ROM_LOAD("apf_4000.rom",0x010000, 0x0800, CRC(2a331a33) SHA1(387b90882cd0b66c192d9cbaa3bec250f897e4f1))
//  ROM_LOAD("apf-m1000rom.bin",0x010000, 0x0800, CRC(cc6ac840) SHA1(1110a234bcad99bd0894ad44c591389d16376ca4))
	ROM_CART_LOAD("cart", 0x8000, 0x2000, ROM_OPTIONAL)
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME        PARENT  COMPAT  MACHINE             INPUT               INIT         COMPANY               FULLNAME */
COMP(1977, apfimag,	0,	0,	apf_imagination,	apf_imagination,	0,		"APF Electronics Inc",  "APF Imagination Machine" , 0 )
CONS(1978, apfm1000,	0,	0,	apf_m1000,		apf_m1000,		0,			"APF Electronics Inc",  "APF M-1000" ,GAME_NOT_WORKING)
