/******************************************************************************

  ssystem3.c

Driver file to handle emulation of the Chess Champion Super System III / Chess
Champion MK III
by PeT mess@utanet.at November 2000, march 2008

Hardware descriptions:
- A 6502 CPU
- A 6522 VIA ($6000?) (PB6 and PB7 are tied)
- 2 x 2114  1kx4 SRAM to provide 1KB of RAM ($0000)
- 2xXXKB ROM (both connected to the same pins!,
  look into mess source mess/messroms/rddil24.c for notes)
  - signetics 7947e c19081e ss-3-lrom
  - signetics 7945e c19082 ss-3-hrom ($d000??)

optional printer (special serial connection)
optional board display (special serial connection)
internal expansion/cartridge port
 (special power saving pack)


todo:
not sure about lcd signs and their assignments
not sure about all buttons and switches
playfield displayment currently based on simulation and on screen
not check for audio yet
convert to new artwork system

needed:
artwork for board display
backup of playfield rom and picture/description of its board
*/

#include "emu.h"

#include "includes/ssystem3.h"
#include "machine/6522via.h"
#include "cpu/m6502/m6502.h"
#include "sound/dac.h"

static struct {
  UINT8 porta;
} ssystem3;


// in my opinion own cpu to display lcd field and to handle own buttons
static struct {
  int signal;
  //  int on;

  int count, bit, started;
  UINT8 data;
  attotime time, high_time, low_time;
  union {
    struct {
      UINT8 header[7];
      UINT8 field[8][8/2];
      UINT8 unknown[5];
    } s;
    UINT8 data[7+8*8/2+5];
  } u;
} playfield;

void ssystem3_playfield_getfigure(int x, int y, int *figure, int *black)
{
  int d;
  if (x&1)
    d=playfield.u.s.field[y][x/2]&0xf;
  else
    d=playfield.u.s.field[y][x/2]>>4;

  *figure=d&7;
  *black=d&8;
}

static void ssystem3_playfield_reset(void)
{
  memset(&playfield, 0, sizeof(playfield));
  playfield.signal=FALSE;
  //  playfield.on=TRUE; //input_port_read(machine, "Configuration")&1;
}

static void ssystem3_playfield_write(running_machine *machine, int reset, int signal)
{
  int d=FALSE;

  if (!reset) {
    playfield.count=0;
    playfield.bit=0;
    playfield.started=FALSE;
    playfield.signal=signal;
    playfield.time=timer_get_time(machine);
  }
  if (!signal && playfield.signal) {
    attotime t=timer_get_time(machine);
    playfield.high_time=attotime_sub(t, playfield.time);
    playfield.time=t;

    //    logerror("%.4x playfield %d lowtime %s hightime %s\n",(int)activecpu_get_pc(), playfield.count,
    //       attotime_string(playfield.low_time, 7), attotime_string(playfield.high_time,7) );

    if (playfield.started) {
      // 0 twice as long low
      // 1 twice as long high
      if (attotime_compare(playfield.low_time,playfield.high_time)>0) d=TRUE;

      playfield.data&=~(1<<(playfield.bit^7));
      if (d) playfield.data|=1<<(playfield.bit^7);
      playfield.bit++;
      if (playfield.bit==8) {
	logerror("%.4x playfield wrote %d %02x\n", (int)cpu_get_pc(machine->device("maincpu")), playfield.count, playfield.data);
	playfield.u.data[playfield.count]=playfield.data;
	playfield.bit=0;
	playfield.count=(playfield.count+1)%ARRAY_LENGTH(playfield.u.data);
	if (playfield.count==0) playfield.started=FALSE;
      }
    }

  } else if (signal && !playfield.signal) {
    attotime t=timer_get_time(machine);
    playfield.low_time=attotime_sub(t, playfield.time);
    playfield.time=t;
    playfield.started=TRUE;
  }
  playfield.signal=signal;
}

static void ssystem3_playfield_read(running_machine *machine, int *on, int *ready)
{
	*on=!(input_port_read(machine, "Configuration")&1);
	//  *on=!playfield.on;
	*ready=FALSE;
}

static WRITE8_DEVICE_HANDLER(ssystem3_via_write_a)
{
  ssystem3.porta=data;
  //  logerror("%.4x via port a write %02x\n",(int)activecpu_get_pc(), data);
}

static READ8_DEVICE_HANDLER(ssystem3_via_read_a)
{
  UINT8 data=0xff;
#if 1 // time switch
  if (!(ssystem3.porta&0x10)) data&=input_port_read(device->machine, "matrix1")|0xf1;
  if (!(ssystem3.porta&0x20)) data&=input_port_read(device->machine, "matrix2")|0xf1;
  if (!(ssystem3.porta&0x40)) data&=input_port_read(device->machine, "matrix3")|0xf1;
  if (!(ssystem3.porta&0x80)) data&=input_port_read(device->machine, "matrix4")|0xf1;
#else
  if (!(ssystem3.porta&0x10)) data&=input_port_read(device->machine, "matrix1")|0xf0;
  if (!(ssystem3.porta&0x20)) data&=input_port_read(device->machine, "matrix2")|0xf0;
  if (!(ssystem3.porta&0x40)) data&=input_port_read(device->machine, "matrix3")|0xf0;
  if (!(ssystem3.porta&0x80)) data&=input_port_read(device->machine, "matrix4")|0xf0;
#endif
  if (!(ssystem3.porta&1)) {
    if (!(input_port_read(device->machine, "matrix1")&1)) data&=~0x10;
    if (!(input_port_read(device->machine, "matrix2")&1)) data&=~0x20;
    if (!(input_port_read(device->machine, "matrix3")&1)) data&=~0x40;
    if (!(input_port_read(device->machine, "matrix4")&1)) data&=~0x80;
  }
  if (!(ssystem3.porta&2)) {
    if (!(input_port_read(device->machine, "matrix1")&2)) data&=~0x10;
    if (!(input_port_read(device->machine, "matrix2")&2)) data&=~0x20;
    if (!(input_port_read(device->machine, "matrix3")&2)) data&=~0x40;
    if (!(input_port_read(device->machine, "matrix4")&2)) data&=~0x80;
  }
  if (!(ssystem3.porta&4)) {
    if (!(input_port_read(device->machine, "matrix1")&4)) data&=~0x10;
    if (!(input_port_read(device->machine, "matrix2")&4)) data&=~0x20;
    if (!(input_port_read(device->machine, "matrix3")&4)) data&=~0x40;
    if (!(input_port_read(device->machine, "matrix4")&4)) data&=~0x80;
  }
  if (!(ssystem3.porta&8)) {
    if (!(input_port_read(device->machine, "matrix1")&8)) data&=~0x10;
    if (!(input_port_read(device->machine, "matrix2")&8)) data&=~0x20;
    if (!(input_port_read(device->machine, "matrix3")&8)) data&=~0x40;
    if (!(input_port_read(device->machine, "matrix4")&8)) data&=~0x80;
  }
  //  logerror("%.4x via port a read %02x\n",(int)activecpu_get_pc(), data);
  return data;
}


/*
  port b
   bit 0: output opt device reset?

    hi speed serial 1 (d7d7 transfers 40 bit $2e)
   bit 1: output data
   bit 2: output clock (hi data is taken)

   bit 3: output opt data read
   bit 4: input low opt data available
   bit 5: input low opt device available


    bit 6: input clocks!?

  port a:
   bit 7: input low x/$37 2
   bit 6: input low x/$37 3
   bit 5: input low x/$37 4 (else 1)

 */
static READ8_DEVICE_HANDLER(ssystem3_via_read_b)
{
	UINT8 data=0xff;
	int on, ready;
	ssystem3_playfield_read(device->machine, &on, &ready);
	if (!on) data&=~0x20;
	if (!ready) data&=~0x10;
	return data;
}

static WRITE8_DEVICE_HANDLER(ssystem3_via_write_b)
{
	running_device *via_0 = device->machine->device("via6522_0");
	UINT8 d;

	ssystem3_playfield_write(device->machine, data&1, data&8);
	ssystem3_lcd_write(device->machine, data&4, data&2);

	d=ssystem3_via_read_b(via_0, 0)&~0x40;
	if (data&0x80) d|=0x40;
	//  d&=~0x8f;
	via_portb_w( via_0, 0, d );
}

static const via6522_interface ssystem3_via_config=
{
	DEVCB_HANDLER(ssystem3_via_read_a),//read8_machine_func in_a_func;
	DEVCB_HANDLER(ssystem3_via_read_b),//read8_machine_func in_b_func;
	DEVCB_NULL,//read8_machine_func in_ca1_func;
	DEVCB_NULL,//read8_machine_func in_cb1_func;
	DEVCB_NULL,//read8_machine_func in_ca2_func;
	DEVCB_NULL,//read8_machine_func in_cb2_func;
	DEVCB_HANDLER(ssystem3_via_write_a),//write8_machine_func out_a_func;
	DEVCB_HANDLER(ssystem3_via_write_b),//write8_machine_func out_b_func;
	DEVCB_NULL,//write8_machine_func out_ca2_func;
	DEVCB_NULL,//write8_machine_func out_cb2_func;
	DEVCB_NULL,//void (*irq_func)(int state);
};

static DRIVER_INIT( ssystem3 )
{
	ssystem3_playfield_reset();
	ssystem3_lcd_reset();
}

static ADDRESS_MAP_START( ssystem3_map , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x03ff) AM_RAM
				  /*
67-de playfield ($40 means white, $80 black)
                   */
//  AM_RANGE( 0x4000, 0x40ff) AM_NOP
/*
  probably zusatzger??t memory (battery powered ram 256x4? at 0x4000)
  $40ff low nibble ram if playfield module (else init with normal playfield)
 */
	AM_RANGE( 0x6000, 0x600f) AM_DEVREADWRITE("via6522_0", via_r, via_w)
#if 1
	AM_RANGE( 0xc000, 0xdfff) AM_ROM
	AM_RANGE( 0xf000, 0xffff) AM_ROM
#else
	AM_RANGE( 0xc000, 0xffff) AM_ROM
#endif
ADDRESS_MAP_END

static INPUT_PORTS_START( ssystem3 )
/*
  switches: light(hardware?) sound time power(hardware!)

  new game (hardware?)
*/


  PORT_START( "Switches" )
//PORT_BIT(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NEW GAME") PORT_CODE(KEYCODE_F3) // seems to be direct wired to reset
//  PORT_BIT(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("?CLEAR") PORT_CODE(KEYCODE_F1)
//  PORT_BIT(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("?ENTER") PORT_CODE(KEYCODE_ENTER)
  PORT_START( "matrix1" )
     PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("?1") PORT_CODE(KEYCODE_1_PAD)
     PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9   C SQ     EP") PORT_CODE(KEYCODE_9)
     PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER?") PORT_CODE(KEYCODE_ENTER)
     PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0   C BOARD  MD") PORT_CODE(KEYCODE_0)
  PORT_START( "matrix2" )
     PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("?2") PORT_CODE(KEYCODE_2_PAD)
     PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 F springer zeitvorgabe") PORT_CODE(KEYCODE_6)  PORT_CODE(KEYCODE_F)
     PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 E laeufer ruecknahme") PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_E)
     PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CE  interrupt") PORT_CODE(KEYCODE_BACKSPACE)
  PORT_START( "matrix3" )
     PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("?3") PORT_CODE(KEYCODE_3_PAD)
     PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 G bauer zugvorschlaege") PORT_CODE(KEYCODE_7) PORT_CODE(KEYCODE_G)
     PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 D turm #") PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_D)
     PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 A white") PORT_CODE(KEYCODE_1) PORT_CODE(KEYCODE_A)
  PORT_START( "matrix4" )
     PORT_DIPNAME( 0x01, 0, "Time") PORT_CODE(KEYCODE_T) PORT_TOGGLE PORT_DIPSETTING( 0, DEF_STR(Off) ) PORT_DIPSETTING( 0x01, DEF_STR( On ) )
     PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 H black") PORT_CODE(KEYCODE_8) PORT_CODE(KEYCODE_H)
     PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 C dame #50") PORT_CODE(KEYCODE_3) PORT_CODE(KEYCODE_C)
     PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 B koenig FP") PORT_CODE(KEYCODE_2) PORT_CODE(KEYCODE_B)
  PORT_START( "Configuration" )
     PORT_DIPNAME( 0x0001, 0, "Schachbrett") PORT_TOGGLE
     PORT_DIPSETTING( 0, DEF_STR( Off ) )
     PORT_DIPSETTING( 1, "angeschlossen" )
#if 0
	PORT_DIPNAME( 0x0002, 0, "Memory") PORT_TOGGLE
	PORT_DIPSETTING( 0, DEF_STR( Off ) )
	PORT_DIPSETTING( 2, "angeschlossen" )
	PORT_DIPNAME( 0x0004, 0, "Drucker") PORT_TOGGLE
	PORT_DIPSETTING( 0, DEF_STR( Off ) )
	PORT_DIPSETTING( 4, "angeschlossen" )
#endif
INPUT_PORTS_END



static MACHINE_DRIVER_START( ssystem3 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M6502, 1000000)
	MDRV_CPU_PROGRAM_MAP(ssystem3_map)
	MDRV_QUANTUM_TIME(HZ(60))

    /* video hardware */
	MDRV_SCREEN_ADD("screen", LCD)
	MDRV_SCREEN_REFRESH_RATE(LCD_FRAMES_PER_SECOND)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(728, 437)
	MDRV_SCREEN_VISIBLE_AREA(0, 728-1, 0, 437-1)
	MDRV_PALETTE_LENGTH(242 + 32768)
	MDRV_PALETTE_INIT( ssystem3 )

	MDRV_VIDEO_START( ssystem3 )
	MDRV_VIDEO_UPDATE( ssystem3 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("dac", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)

	/* via */
	MDRV_VIA6522_ADD("via6522_0", 0, ssystem3_via_config)
MACHINE_DRIVER_END


ROM_START(ssystem3)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("ss3lrom", 0xc000, 0x1000, CRC(9ea46ed3) SHA1(34eef85b356efbea6ddac1d1705b104fc8e2731a) )
//  ROM_RELOAD(0xe000, 0x1000)
	ROM_LOAD("ss3hrom", 0xf000, 0x1000, CRC(52741e0b) SHA1(2a7b950f9810c5a14a1b9d5e6b2bd93da621662e) )
	ROM_RELOAD(0xd000, 0x1000)
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT     INIT        COMPANY     FULLNAME */
CONS( 1979,	ssystem3, 0,		0,		ssystem3, ssystem3,	ssystem3,	"NOVAG Industries Ltd",  "Chess Champion Super System III", GAME_NOT_WORKING | GAME_NO_SOUND)
//chess champion MK III in germany
