/***************************************************************************
Mephisto Glasgow 3 S chess computer
Dirk V.
sp_rinter@gmx.de

68000 CPU
64 KB ROM
16 KB RAM
4 Digit LC Display

3* 74LS138	Decoder/Multiplexer
1*74LS74	Dual positive edge triggered D Flip Flop
1*74LS139 1of4 Demultiplexer
1*74LS05	HexInverter
1*NE555		R=100K C=10uF
2*74LS04  Hex Inverter
1*74LS164	8 Bit Shift register
1*74121 Monostable Multivibrator with Schmitt Trigger Inputs 
1*74LS20 Dual 4 Input NAND GAte
1*74LS367 3 State Hex Buffers


***************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68k.h"
// #include "cpu/m68000/m68000.h"
#include "glasgow.lh"
#include "sound/beep.h"

static UINT8 lcd_shift_counter;
// static UINT8 led_status;
static UINT8 led7;
static UINT8 key_low,
       key_hi,
       key_select;

static WRITE16_HANDLER ( write_lcd ) 
{ 
  UINT8 lcd_data;
  lcd_data = data>>8; 
  output_set_digit_value(lcd_shift_counter,lcd_data ^led7);  
  lcd_shift_counter--;
  lcd_shift_counter&=3; 
  //logerror("LCD Offset = %d Data low  = %d \n  ",offset,lcd_data); 
  
}

static WRITE16_HANDLER ( write_lcd_flag ) 
{ 
 UINT8 lcd_flag;
 lcd_flag=data>>8;
 beep_set_state(0,lcd_flag&1?1:0);
 if (lcd_flag!=0) led7=255;else led7=0;
 logerror("LCD Flag   = %d \n  ",lcd_flag); 
}

static WRITE16_HANDLER ( write_keys ) 
{ 
 key_select=data>>8;
 //logerror("Write Key   = %d \n  ",key_select); 
}


static WRITE16_HANDLER ( write_beeper ) 
{ 
 //beep_set_state(0,led_status&64?1:0);
 logerror("Write Beeper   = %d \n  ",data>>8); 
}

static READ16_HANDLER(read_keys)
{
  UINT16 data;
  data=0x0300;
  key_low=readinputport(0x00);
  key_hi=readinputport(0x01);
  // logerror("Keyboard Offset = %d Data = %d\n  ",offset,data);
  
  if (key_select==key_low){data=data&0x100;}
  if (key_select==key_hi){data=data&0x200;}
  return data;
}


static READ16_HANDLER(read_board)
{
  return 0xffff;	// Mephisto need it for working
}

static WRITE16_HANDLER(write_board)
{
  logerror("Write Board   = %d \n  ",data>>8); 
}

static TIMER_CALLBACK( update_nmi )
{
  
	//beep_set_state(0,led_status&64?1:0);
	cpunum_set_input_line_and_vector(0,  MC68000_IRQ_7, PULSE_LINE, MC68000_INT_ACK_AUTOVECTOR);
	

}

static MACHINE_START( glasgow )
{
  lcd_shift_counter=3;
	mame_timer_pulse(MAME_TIME_IN_HZ(60), 0, update_nmi);
  beep_set_frequency(0, 44);
}

static MACHINE_RESET( glasgow )
{
	lcd_shift_counter=3;
}



static VIDEO_START( glasgow )
{
}
 
static VIDEO_UPDATE( glasgow )
{
    return 0;
}
static ADDRESS_MAP_START(glasgow_mem, ADDRESS_SPACE_PROGRAM, 16)
    AM_RANGE( 0x0000, 0xffff ) AM_ROM 
    AM_RANGE( 0x00FF0000, 0x00FF0001) AM_WRITE( write_lcd)
    AM_RANGE( 0x00FF0002, 0x00FF0003) AM_READWRITE( read_keys,write_keys)
    AM_RANGE( 0x00FF0004, 0x00FF0005) AM_WRITE( write_lcd_flag)
    AM_RANGE( 0x00FF0006, 0x00FF0007) AM_READWRITE( read_board, write_board)
    AM_RANGE( 0x00FF0008, 0x00FF0009) AM_WRITE( write_beeper)
    //AM_RANGE( 0x00FF8000, 0x00FFFFFF ) AM_RAM
    AM_RANGE( 0x00FFC000, 0x00FFFFFF ) AM_RAM      // 16KB
ADDRESS_MAP_END

INPUT_PORTS_START( glasgow )
  PORT_START
    PORT_BIT(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
    PORT_BIT(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CL") PORT_CODE(KEYCODE_F5)
    PORT_BIT(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C3") PORT_CODE(KEYCODE_C)   
    PORT_BIT(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C3") PORT_CODE(KEYCODE_3)  
    PORT_BIT(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENT") PORT_CODE(KEYCODE_ENTER) 
    PORT_BIT(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D4") PORT_CODE(KEYCODE_D) //
    PORT_BIT(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D4") PORT_CODE(KEYCODE_4) //
    PORT_BIT(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A1") PORT_CODE(KEYCODE_A) //
    PORT_BIT(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A1") PORT_CODE(KEYCODE_1) //
    PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F6") PORT_CODE(KEYCODE_F) // 
    PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F6") PORT_CODE(KEYCODE_6) // 
    PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B2") PORT_CODE(KEYCODE_B)//
    PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B2") PORT_CODE(KEYCODE_2)//
  PORT_START
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E5") PORT_CODE(KEYCODE_E) //
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E5") PORT_CODE(KEYCODE_5) //
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("INFO") PORT_CODE(KEYCODE_F1) //
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)  
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("POS") PORT_CODE(KEYCODE_F2) //
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H8") PORT_CODE(KEYCODE_H) //
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H8") PORT_CODE(KEYCODE_8) //
    PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LEV") PORT_CODE(KEYCODE_F3) //
    PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G7") PORT_CODE(KEYCODE_G) // 
    PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G7") PORT_CODE(KEYCODE_7) // 
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MEM") PORT_CODE(KEYCODE_F4) //
INPUT_PORTS_END



static MACHINE_DRIVER_START(glasgow )
    /* basic machine hardware */
    MDRV_CPU_ADD_TAG("main", M68000, 20000000)
	  MDRV_CPU_PROGRAM_MAP(glasgow_mem, 0)
	  MDRV_MACHINE_START( glasgow )
	  MDRV_MACHINE_RESET( glasgow )
    /* video hardware */
    MDRV_SCREEN_REFRESH_RATE(60)
    MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)
    MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 639, 0, 479)
    MDRV_PALETTE_LENGTH(4)
    MDRV_DEFAULT_LAYOUT(layout_glasgow)
    MDRV_SPEAKER_STANDARD_MONO("mono")
	  MDRV_SOUND_ADD(BEEP, 0)
	  MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
    MDRV_VIDEO_START(glasgow)
    MDRV_VIDEO_UPDATE(glasgow)
MACHINE_DRIVER_END


/***************************************************************************
  ROM definitions
***************************************************************************/

ROM_START( glasgow )
    ROM_REGION16_BE( 0x1000000, REGION_CPU1, 0 )
    //ROM_LOAD("glasgow.rom", 0x000000, 0x10000, CRC(3e73eff3) ) 
    ROM_LOAD16_BYTE("me3_3_1u.410",0x00000, 0x04000,CRC(bc8053ba) SHA1(57ea2d5652bfdd77b17d52ab1914de974bd6be12))
    ROM_LOAD16_BYTE("me3_1_1l.410",0x00001, 0x04000,CRC(d5263c39) SHA1(1bef1cf3fd96221eb19faecb6ec921e26ac10ac4))
    ROM_LOAD16_BYTE("me3_4_2u.410",0x08000, 0x04000,CRC(8dba504a) SHA1(6bfab03af835cdb6c98773164d32c76520937efe))
    ROM_LOAD16_BYTE("me3_2_2l.410",0x08001, 0x04000,CRC(b3f27827) SHA1(864ba897d24024592d08c4ae090aa70a2cc5f213))
    
ROM_END

ROM_START( amsterd )
    ROM_REGION16_BE( 0x1000000, REGION_CPU1, 0 )
    //ROM_LOAD("glasgow.rom", 0x000000, 0x10000, CRC(3e73eff3) ) 
    ROM_LOAD16_BYTE("me3_3_1u.410",0x00000, 0x04000,CRC(bc8053ba) SHA1(57ea2d5652bfdd77b17d52ab1914de974bd6be12))
    ROM_LOAD16_BYTE("me3_1_1l.410",0x00001, 0x04000,CRC(d5263c39) SHA1(1bef1cf3fd96221eb19faecb6ec921e26ac10ac4))
    ROM_LOAD16_BYTE("me3_4_2u.410",0x08000, 0x04000,CRC(8dba504a) SHA1(6bfab03af835cdb6c98773164d32c76520937efe))
    ROM_LOAD16_BYTE("me3_2_2l.410",0x08001, 0x04000,CRC(b3f27827) SHA1(864ba897d24024592d08c4ae090aa70a2cc5f213))    
ROM_END

ROM_START( dallas )
    ROM_REGION16_BE( 0x1000000, REGION_CPU1, 0 )
    //ROM_LOAD("glasgow.rom", 0x000000, 0x10000, CRC(3e73eff3) ) 
    ROM_LOAD16_BYTE("me3_3_1u.410",0x00000, 0x04000,CRC(bc8053ba) SHA1(57ea2d5652bfdd77b17d52ab1914de974bd6be12))
    ROM_LOAD16_BYTE("me3_1_1l.410",0x00001, 0x04000,CRC(d5263c39) SHA1(1bef1cf3fd96221eb19faecb6ec921e26ac10ac4))
    ROM_LOAD16_BYTE("me3_4_2u.410",0x08000, 0x04000,CRC(8dba504a) SHA1(6bfab03af835cdb6c98773164d32c76520937efe))
    ROM_LOAD16_BYTE("me3_2_2l.410",0x08001, 0x04000,CRC(b3f27827) SHA1(864ba897d24024592d08c4ae090aa70a2cc5f213))    
ROM_END

ROM_START( roma )
    ROM_REGION16_BE( 0x1000000, REGION_CPU1, 0 )
    //ROM_LOAD("glasgow.rom", 0x000000, 0x10000, CRC(3e73eff3) ) 
    ROM_LOAD16_BYTE("me3_3_1u.410",0x00000, 0x04000,CRC(bc8053ba) SHA1(57ea2d5652bfdd77b17d52ab1914de974bd6be12))
    ROM_LOAD16_BYTE("me3_1_1l.410",0x00001, 0x04000,CRC(d5263c39) SHA1(1bef1cf3fd96221eb19faecb6ec921e26ac10ac4))
    ROM_LOAD16_BYTE("me3_4_2u.410",0x08000, 0x04000,CRC(8dba504a) SHA1(6bfab03af835cdb6c98773164d32c76520937efe))
    ROM_LOAD16_BYTE("me3_2_2l.410",0x08001, 0x04000,CRC(b3f27827) SHA1(864ba897d24024592d08c4ae090aa70a2cc5f213))    
ROM_END

/***************************************************************************
  System config
***************************************************************************/



/***************************************************************************
  Game drivers
***************************************************************************/

/*     YEAR  NAME      PARENT   BIOS     COMPAT   MACHINE  INPUT    INIT       CONFIG   COMPANY                             FULLNAME             FLAGS */
CONS(  1984, glasgow,   0,                0,       glasgow,  glasgow,   NULL,     NULL,   "Hegener & Glaser Muenchen",  "Mephisto III S Glasgow", 0)
CONS(  1984, amsterd,  0,               0,       glasgow,  glasgow,   NULL,     NULL,   "Hegener & Glaser Muenchen",  "Mephisto Amsterdam", 0)
CONS(  1984, dallas,   0,                0,       glasgow,  glasgow,   NULL,     NULL,   "Hegener & Glaser Muenchen",  "Mephisto Dallas 68000", 0)
CONS(  1984, roma,  0,               0,       glasgow,  glasgow,   NULL,     NULL,   "Hegener & Glaser Muenchen",  "Mephisto Roma 68000", 0)

