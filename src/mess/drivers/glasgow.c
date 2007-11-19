/***************************************************************************
Mephisto Glasgow 3 S chess computer
Dirk V.
sp_rinter@gmx.de

68000 CPU
64 KB ROM
16 KB RAM
4 Digit LC Display

3* 74LS138  Decoder/Multiplexer
1*74LS74    Dual positive edge triggered D Flip Flop
1*74LS139 1of4 Demultiplexer
1*74LS05    HexInverter
1*NE555     R=100K C=10uF
2*74LS04  Hex Inverter
1*74LS164   8 Bit Shift register
1*74121 Monostable Multivibrator with Schmitt Trigger Inputs
1*74LS20 Dual 4 Input NAND GAte
1*74LS367 3 State Hex Buffers


***************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68k.h"
#include "cpu/m68000/m68000.h"
#include "glasgow.lh"
#include "sound/beep.h"

#include "render.h"
#include "rendlay.h"

static UINT8 lcd_shift_counter;
// static UINT8 led_status;
static UINT8 led7;
static UINT8 key_low,
       key_hi,
       key_select,
       irq_flag,
       lcd_invert,
       key_selector;
       UINT8 board_value;
static UINT16 beeper;


  #define MOUSE_X		56
  #define MOUSE_Y		56
  #define MAX_X			530
  #define MAX_Y			660
  #define MOUSE_SPEED	3

  #define MIN_BOARD_X	60.0
  #define MAX_BOARD_X	480.0

  #define MIN_BOARD_Y	60.0
  #define MAX_BOARD_Y	480.0

  #define MIN_CURSOR_X	-10
  #define MAX_CURSOR_X	496

  #define MIN_CURSOR_Y	0
  #define MAX_CURSOR_Y	622

  static INT16 m_x, m_y;
  static UINT8 m_button1 , m_button2;
  static UINT8 MOUSE_MOVE = 0;
  static UINT8 MOUSE_BUTTON1_WAIT = 0;
  static UINT8 MOUSE_BUTTON2_WAIT = 0;

  static UINT16 Line18_LED;
  static UINT16 Line18_REED;

  static UINT8 read_board_flag;

  static  char EMP[4] = "EMP";
  static  char NO_PIECE[4]  = "NOP";


  typedef struct {
       unsigned int field;
	   unsigned int x;
	   unsigned int y;
       unsigned char piece[4];
  } BOARD_FIELD;


	#define WR	12
	#define WB	13
	#define WKN	14
	#define WQ	15
	#define WK	16

	#define BP	21
	#define BR	22
	#define BB	23
	#define BKN	24
	#define BQ	25
	#define BK	26



    BOARD_FIELD m_board[8][8];

    BOARD_FIELD start_board[8][8] =
	{
		{ { 7,44,434,"WR1"}, { 6,100,434,"WN1"}, { 5,156,434,"WB1"}, { 4,212,434,"WQ1"}, { 3,268,434,"WK"}, { 2,324,434,"WB2"}, { 1,380,434,"WN2"}, { 0,436,434,"WR2"} },
		{ {15,44,378,"WP1"}, {14,100,378,"WP2"}, {13,156,378,"WP3"}, {12,212,378,"WP4"}, {11,268,378,"WP5"}, {10,324,378,"WP6"}, { 9,380,378,"WP7"}, { 8,436,378,"WP8"} },

		{ {23,44,322, "EMP"}, {22,100,322, "EMP"}, {21,156,322, "EMP"}, {20,212,322, "EMP"}, {19,268,322, "EMP"}, {18,324,322, "EMP"}, {17,380,322, "EMP"}, {16,436,322, "EMP"} },
		{ {31,44,266, "EMP"}, {30,100,266, "EMP"}, {29,156,266, "EMP"}, {28,212,266, "EMP"}, {27,268,266, "EMP"}, {26,324,266, "EMP"}, {25,380,266, "EMP"}, {24,436,266, "EMP"} },
		{ {39,44,210, "EMP"}, {38,100,210, "EMP"}, {37,156,210, "EMP"}, {36,212,210, "EMP"}, {35,268,210, "EMP"}, {34,324,210, "EMP"}, {33,380,210, "EMP"}, {32,436,210, "EMP"} },
		{ {47,44,154, "EMP"}, {46,100,154, "EMP"}, {45,156,154, "EMP"}, {44,212,154, "EMP"}, {43,268,154, "EMP"}, {42,324,154, "EMP"}, {41,380,154, "EMP"}, {40,436,154, "EMP"} },

		{ {55,44,100,"BP1"}, {54,100,100,"BP2"}, {53,156,100,"BP3"}, {52,212,100,"BP4"}, {51,268,100,"BP5"}, {50,324,100,"BP6"}, {49,380,100,"BP7"}, {48,436,100,"BP8"} },
		{ {63,44,44 ,"BR1"}, {62,100,44 ,"BN1"}, {61,156,44 ,"BB1"}, {60,212,44 ,"BQ1"}, {59,268,44 ,"BK"}, {58,324,44 ,"BB2"}, {57,380,44 ,"BN2"}, {56,436,44 ,"BR2"} }

	};


   static BOARD_FIELD cursor_field;


     typedef struct {
       unsigned char piece[4];
	   unsigned int selected;
	   unsigned int set;
  } P_STATUS;


	 static P_STATUS all_pieces[48] =
	 {
		 {"WR1",0,0}, {"WN1",0,0},{"WB1",0,0}, {"WQ1",0,0},  {"WK",0,0},  {"WB2",0,0}, {"WN2",0,0}, {"WR2",0,0},
		 {"WP1",0,0}, {"WP2",0,0},{"WP3",0,0}, {"WP4",0,0}, {"WP5",0,0},  {"WP6",0,0}, {"WP7",0,0}, {"WP8",0,0},


		{"BP1",0,0}, {"BP2",0,0},{"BP3",0,0}, {"BP4",0,0}, {"BP5",0,0},  {"BP6",0,0}, {"BP7",0,0}, {"BP8",0,0},
	    {"BR1",0,0}, {"BN1",0,0},{"BB1",0,0}, {"BQ1",0,0},  {"BK",0,0},  {"BB2",0,0}, {"BN2",0,0}, {"BR2",0,0},


        {"BQ2",0,0}, {"BQ3",0,0}, {"BQ4",0,0}, {"BQ5",0,0}, {"BQ6",0,0}, {"BQ7",0,0},


		{"WQ2",0,0}, {"WQ3",0,0}, {"WQ4",0,0}, {"WQ5",0,0}, {"WQ6",0,0}, {"WQ7",0,0}

	 };

     static UINT8 start_i;


  struct _render_target
{
	render_target *		next;				/* keep a linked list of targets */
	layout_view *		curview;			/* current view */
	layout_file *		filelist;			/* list of layout files */
	UINT32				flags;				/* creation flags */
	render_primitive_list primlist[2];/* list of primitives */
	int					listindex;			/* index of next primlist to use */
	INT32				width;				/* width in pixels */
	INT32				height;				/* height in pixels */
	render_bounds		bounds;				/* bounds of the target */
	float				pixel_aspect;		/* aspect ratio of individual pixels */
	float				max_refresh;		/* maximum refresh rate, 0 or if none */
	int					orientation;		/* orientation */
	int					layerconfig;		/* layer configuration */
	layout_view *		base_view;			/* the view at the time of first frame */
	int					base_orientation;	/* the orientation at the time of first frame */
	int					base_layerconfig;	/* the layer configuration at the time of first frame */
	int					maxtexwidth;		/* maximum width of a texture */
	int					maxtexheight;		/* maximum height of a texture */
};

  static render_target *my_target;

  static view_item *my_cursor;

  static void set_cursor (view_item *view_cursor);

  static view_item *get_view_item(render_target *target, const char *v_name);

  static BOARD_FIELD  get_field( float i_x, float i_y, UINT8 mouse_move);
  static void update_board(render_target *target, BOARD_FIELD board_field, unsigned int update_clear_flag);

  static void calculate_bounds(view_item *view_item, float new_x0, float new_y0, float new_del_x, float new_del_y );

  static void set_render_board(void);
  static void clear_layout(void);
  static void set_status_of_pieces(void);

  static char * get_non_set_pieces(const char *cur_piece);

  static unsigned int out_of_board( float x0, float y0);



// Used by Glasgow and Dallas
static WRITE16_HANDLER ( write_lcd_gg )
{
  UINT8 lcd_data;
  lcd_data = data>>8;
  if (led7==0) output_set_digit_value(lcd_shift_counter,lcd_data);
  lcd_shift_counter--;
  lcd_shift_counter&=3;
//  logerror("LCD Offset = %d Data low  = %x \n  ",offset,lcd_data);
}

static WRITE16_HANDLER ( write_lcd )
{
  UINT8 lcd_data;
  lcd_data = data>>8;
  output_set_digit_value(lcd_shift_counter,lcd_invert&1?lcd_data^0xff:lcd_data);
  lcd_shift_counter--;
  lcd_shift_counter&=3;
//  logerror("LCD Offset = %d Data low  = %x \n  ",offset,lcd_data);
}
static WRITE16_HANDLER ( write_lcd_flag )
{
  UINT8 lcd_flag;
  lcd_invert=0;
  lcd_flag=data>>8;
  //beep_set_state(0,lcd_flag&1?1:0);
  if (lcd_flag == 0) key_selector=1;
  if (lcd_flag!=0) led7=255;else led7=0;
//  logerror("LCD Flag 16  = %x \n  ",data);
}


static WRITE16_HANDLER ( write_lcd_flag_gg )
{
  UINT8 lcd_flag;
  lcd_flag=data>>8;
  beep_set_state(0,lcd_flag&1?1:0);
  if (lcd_flag == 0) key_selector=1;
  if (lcd_flag!=0) led7=255;else led7=0;

//  logerror("LCD Flag gg  = %x \n  ",lcd_flag);
}

static WRITE16_HANDLER ( write_keys )
{
 key_select=data>>8;
// logerror("Write Key   = %x \n  ",data);
}

static WRITE16_HANDLER ( write_board )
{
UINT8 beep_flag;

Line18_REED=data>>8;

// LED's or REED's ?
//
if (read_board_flag)
	{
	Line18_LED=0;
	read_board_flag=0;
	}
else
	{
	Line18_LED=data>>8;
	}

//logerror("write_board Line18_LED   = %x \n  ", Line18_LED);
//logerror("write_board Line18_REED   = %x \n  ",Line18_REED);

lcd_invert=1;
beep_flag=data>>8;
//if ((beep_flag &02)== 0) key_selector=0;else key_selector=1;
//logerror("Write Beeper   = %x \n  ",data);
 beeper=data;
}

static WRITE16_HANDLER ( write_irq_flag )
{

 beep_set_state(0,data&0x100);
 logerror("Write 0x800004   = %x \n  ",data);
 irq_flag=1;
 beeper=data;
}

static READ16_HANDLER(read_keys) // Glasgow, Dallas
{
  UINT16 data;
  data=0x0300;
  key_low=readinputport(0x00);
  key_hi=readinputport(0x01);
//logerror("Keyboard Offset = %x Data = %x\n  ",offset,data);
  if (key_select==key_low){data=data&0x100;}
  if (key_select==key_hi){data=data&0x200;}
  return data;
}

static READ16_HANDLER(read_newkeys16)  //Amsterdam, Roma
{
 UINT16 data;

 if (key_selector==0) data=readinputport(0x00);else data=readinputport(0x01);
 logerror("read Keyboard Offset = %x Data = %x   Select = %x \n  ",offset,data,key_selector);
 data=data<<8;
 return data ;
}

static READ16_HANDLER(read_board)
{
	  UINT8 i_18, i_AH;
	  UINT16 data;

	  data = 0xff;

      for ( i_18 = 0; i_18 < 8; i_18 = i_18 + 1)
	   	{

// Looking for cleared bit in Line18 -> current line
//
		if ( !(Line18_REED & (1<<i_18)) )
           {

// if there is a piece on the field -> set bit in data
//
           for ( i_AH = 0; i_AH < 8; i_AH = i_AH + 1)
				{
					if (strcmp ((char *)m_board[i_18][i_AH].piece, EMP))
					{
						data &= ~(1 << i_AH);  // clear bit
					}
				}
		   }
	    }

   	   data = data<<8;

		read_board_flag = TRUE;

		logerror("read_board_data = %x \n  ", data);


//	  return 0xff00;   // Mephisto need it for working


      return data;

}

static WRITE16_HANDLER(write_beeper)
{
//  logerror("Write Board   = %x \n  ",data>>8);

    UINT8 i_AH, i_18;
    UINT16 LineAH = 0;

	UINT8 LED;


//    logerror("Write Board   = %x \n  ",data);

    LineAH = data>>8;

	if (LineAH && Line18_LED)
	{

//  logerror("Line18_LED   = %x \n  ",Line18_LED);
//  logerror("LineAH   = %x \n  ",LineAH);

		for ( i_AH = 0; i_AH < 8; i_AH = i_AH + 1)
		{

         if ( LineAH & (1<<i_AH) )
            {
            for ( i_18 = 0; i_18 < 8; i_18 = i_18 + 1)
				{
					if ( !(Line18_LED & (1<<i_18)) )
						{

//						logerror("i_18   = %d \n  ",i_18);
//						logerror("i_AH   = %d \n  ",i_AH);
//						logerror("LED an:   = %d \n  ",m_board[i_18][i_AH]);

                        LED = m_board[i_18][i_AH].field;

                        output_set_led_value( LED, 1 );

//  LED on
						}
					else
						{
//  LED off

      					} // End IF 18
				} // End For 18
           } // End IF AH

		} // End For AH

	} // End IF LineAH

	else
	{
//  No LED  -> all LED's off
		for ( i_AH = 0; i_AH < 8; i_AH = i_AH + 1)
			{
			for ( i_18 = 0; i_18 < 8; i_18 = i_18 + 1)
				{
// LED off
                 LED = m_board[i_18][i_AH].field;
                 output_set_led_value( LED, 0 );

			    } //End For 18
			} // End For AH

	} //End ELSE LineAH

}

/*
static READ16_HANDLER(read_test)
{
  logerror("read test Offset = %x Data = %x\n  ",offset,data);
  return 0xffff;    // Mephisto need it for working
}

*/

/*

    *****           32 Bit Read and write Handler   ***********

*/

static WRITE32_HANDLER ( write_lcd32 )
{
  UINT8 lcd_data;
  lcd_data = data>>8;
  output_set_digit_value(lcd_shift_counter,lcd_invert&1?lcd_data^0xff:lcd_data);
  lcd_shift_counter--;
  lcd_shift_counter&=3;
  //logerror("LCD Offset = %d Data   = %x \n  ",offset,lcd_data);
}
static WRITE32_HANDLER ( write_lcd_flag32 )
{
  UINT8 lcd_flag;
  lcd_invert=0;
  lcd_flag=data>>24;
  if (lcd_flag == 0) key_selector=1;
  logerror("LCD Flag 32  = %x \n  ",lcd_flag);
  //beep_set_state(0,lcd_flag&1?1:0);
  if (lcd_flag!=0) led7=255;else led7=0;
}


static WRITE32_HANDLER ( write_keys32 )
{
 lcd_invert=1;
 key_select=data;
 logerror("Write Key = %x \n  ",key_select);
}

static READ32_HANDLER(read_newkeys32) // Dallas 32, Roma 32
{
 UINT32 data;
 if (key_selector==0) data=readinputport(0x00);else data=readinputport(0x01);
 //if (key_selector==1) data=readinputport(0x00);else data=0;
 logerror("read Keyboard Offset = %x Data = %x\n  ",offset,data);
 data=data<<24;
 return data ;
}



static READ32_HANDLER(read_board32)
{
  logerror("read board 32 Offset = %x \n  ",offset);
  return 0x00000000;
}



static WRITE32_HANDLER(write_board32)
{
 UINT8 board;
 board=data>>24;
 if (board==0xff) key_selector=0;
 logerror("Write Board   = %x \n  ",data);
}


static WRITE32_HANDLER ( write_beeper32 )
{
  beep_set_state(0,data&0x01000000);
 logerror("Write 0x8000004   = %x \n  ",data);
 irq_flag=1;
 beeper=data;
}

static TIMER_CALLBACK( update_nmi )
{
	// if (irq_flag==0)cpunum_set_input_line_and_vector(0,  MC68000_IRQ_7, PULSE_LINE, MC68000_INT_ACK_AUTOVECTOR);
	cpunum_set_input_line_and_vector(0,  MC68000_IRQ_7, PULSE_LINE, MC68000_INT_ACK_AUTOVECTOR);

}

static TIMER_CALLBACK( update_nmi32 )
{
	cpunum_set_input_line_and_vector(0,  MC68020_IRQ_7, PULSE_LINE, MC68020_INT_ACK_AUTOVECTOR);
}

static MACHINE_START( glasgow )
{
  key_selector=0;
  irq_flag=0;
  lcd_shift_counter=3;
	timer_pulse(ATTOTIME_IN_HZ(50), 0, update_nmi);
  beep_set_frequency(0, 44);
}


static MACHINE_START( dallas32 )
{
  lcd_shift_counter=3;

	timer_pulse(ATTOTIME_IN_HZ(50), 0, update_nmi32);
  beep_set_frequency(0, 44);
}


static MACHINE_RESET( glasgow )
{
	lcd_shift_counter=3;

	 clear_layout();
	 set_render_board();
	 set_status_of_pieces();
}



static VIDEO_START( glasgow )
{
}

static VIDEO_UPDATE( glasgow )
{
//  INT32 lg_x, lg_y;
// 	UINT8 p_key;

    char *piece;

	float save_x, save_y;

	my_target = render_get_ui_target();

// Get view item of cursor
//
//	my_cursor = my_target->curview->itemlist[3]->next;
	if (my_cursor == NULL)
		my_cursor = get_view_item(my_target,"CURSOR_1");

//  my_cursor = my_target->curview->itemlist[2];

// Mouse movement
//
	if (my_cursor != NULL)
		set_cursor (my_cursor);

	m_button1=readinputport(0x04);
    m_button2=readinputport(0x05);

    if ( m_button1) MOUSE_BUTTON1_WAIT = 0;

    if ( m_button1 != 1 && !MOUSE_BUTTON1_WAIT )
		{

			MOUSE_BUTTON1_WAIT = 1;

			if (MOUSE_MOVE == 0)
				{

				cursor_field = get_field(my_cursor->rawbounds.x0,my_cursor->rawbounds.y0, MOUSE_MOVE);
				if ( strcmp((char *)cursor_field.piece, "0") && (strcmp((char *)cursor_field.piece, EMP))  && strcmp(my_cursor->name, (char *)cursor_field.piece ) )
					{
					update_board(my_target,cursor_field, 0);

					my_cursor->color.a = 0;

					my_cursor = get_view_item(my_target, (char *)cursor_field.piece);
					my_cursor->color.a = 0.5;

					MOUSE_MOVE = 1;

					}
				} // ENDIF MOUSE_MOVE
			else
				{

				save_x = my_cursor->rawbounds.x0;
				save_y = my_cursor->rawbounds.y0;

				cursor_field = get_field(my_cursor->rawbounds.x0,my_cursor->rawbounds.y0, MOUSE_MOVE);
				if ( strcmp((char *)cursor_field.piece, "0") && strcmp(my_cursor->name, "CURSOR_1") )
					{

// Put piece back to board
//
		           strcpy((char *)cursor_field.piece, (char *)my_cursor->name);
				   update_board(my_target,cursor_field, 1);
				   my_cursor->color.a = 1;

// Set piece to field
//
					calculate_bounds(my_cursor, (float) cursor_field.x, (float) cursor_field.y, 0 ,0  );


// Get old cursor and set new x,y
//
					my_cursor = get_view_item(my_target,"CURSOR_1");
					my_cursor->color.a = 1;

					calculate_bounds(my_cursor, save_x, save_y,MOUSE_X ,MOUSE_Y  );

					MOUSE_MOVE = 0;

					}//ENDIF curosr_field

// piece is set out of board -> remove it
//
        if  ( out_of_board(my_cursor->rawbounds.x0,my_cursor->rawbounds.y0) )
			{

			save_x = my_cursor->rawbounds.x0;
			save_y = my_cursor->rawbounds.y0;

            my_cursor->color.a = 0;
			my_cursor = get_view_item(my_target,"CURSOR_1");
			my_cursor->color.a = 1;
			calculate_bounds(my_cursor, save_x, save_y,MOUSE_X ,MOUSE_X  );

			MOUSE_MOVE = 0;
			}

			}//ELSE MOUSE_MOVE

	    } // ENDIF m_button1


// right mouse button -> set pieces
//
	if ( m_button2) MOUSE_BUTTON2_WAIT = 0;
    if ( m_button2 != 1 && !MOUSE_BUTTON2_WAIT )
		{

		MOUSE_BUTTON2_WAIT = 1;

// save cursor position
//
		save_x = my_cursor->rawbounds.x0;
		save_y = my_cursor->rawbounds.y0;

// cursor off
//
		my_cursor->color.a = 0;

// change cursor to piece
//
        piece = get_non_set_pieces(my_cursor->name);

		if (strcmp(piece,NO_PIECE))
			{
			my_cursor = get_view_item(my_target,piece);
			my_cursor->color.a = 0.5;

			calculate_bounds(my_cursor, save_x, save_y, 0 ,0  );

			MOUSE_MOVE = 1;
			}
		else
			{
     		my_cursor = get_view_item(my_target,"CURSOR_1");
			my_cursor->color.a = 1;
            calculate_bounds(my_cursor, save_x, save_y, MOUSE_X ,MOUSE_X  );
			MOUSE_MOVE = 0;
			}

		}

// Put Item back to render
//
    render_set_ui_target (my_target);

      return 0;
}


static ADDRESS_MAP_START(glasgow_mem, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE( 0x0000, 0xffff ) AM_ROM
	AM_RANGE( 0x00ff0000, 0x00ff0001)  AM_MIRROR ( 0xfe0000 ) AM_WRITE     ( write_lcd_gg )
  AM_RANGE( 0x00ff0002, 0x00ff0003)  AM_MIRROR ( 0xfe0002 ) AM_READWRITE ( read_keys,write_keys )
  AM_RANGE( 0x00ff0004, 0x00ff0005)  AM_MIRROR ( 0xfe0004 ) AM_WRITE     ( write_lcd_flag_gg )
  AM_RANGE( 0x00ff0006, 0x00ff0007)  AM_MIRROR ( 0xfe0006 ) AM_READWRITE ( read_board, write_beeper )
  AM_RANGE( 0x00ff0008, 0x00ff0009)  AM_MIRROR ( 0xfe0008 ) AM_WRITE     ( write_board )
  AM_RANGE( 0x00ffC000, 0x00ffFFFF)  AM_MIRROR ( 0xfeC000 ) AM_RAM      // 16KB
ADDRESS_MAP_END


static ADDRESS_MAP_START(amsterd_mem, ADDRESS_SPACE_PROGRAM, 16)
   // ADDRESS_MAP_FLAGS( AMEF_ABITS(19) )
    AM_RANGE( 0x0000, 0xffff )         AM_ROM
    AM_RANGE( 0x00800002, 0x00800003)  AM_WRITE( write_lcd )
    AM_RANGE( 0x00800008, 0x00800009)  AM_WRITE( write_lcd_flag )
    AM_RANGE( 0x00800004, 0x00800005)  AM_WRITE( write_irq_flag )
    AM_RANGE( 0x00800010, 0x00800011)  AM_WRITE( write_board )
    AM_RANGE( 0x00800020, 0x00800021)  AM_READ ( read_board )
    AM_RANGE( 0x00800040, 0x00800041)  AM_READ ( read_newkeys16 )
    AM_RANGE( 0x00800088, 0x00800089)  AM_WRITE( write_beeper )
    AM_RANGE( 0x00ffC000, 0x00ffFFFF)  AM_RAM      // 16KB
ADDRESS_MAP_END


static ADDRESS_MAP_START(dallas32_mem, ADDRESS_SPACE_PROGRAM, 32)
     //ADDRESS_MAP_FLAGS( AMEF_ABITS(17) )
     AM_RANGE( 0x0000, 0xffff )        AM_ROM
     AM_RANGE( 0x00800000, 0x00800003) AM_WRITE ( write_lcd32 )
     AM_RANGE( 0x00800004, 0x00800007) AM_WRITE ( write_beeper32 )
     AM_RANGE( 0x00800008, 0x0080000B) AM_WRITE ( write_lcd_flag32 )
     AM_RANGE( 0x00800010, 0x00800013) AM_WRITE ( write_board32 )
     AM_RANGE( 0x00800020, 0x00800023) AM_READ  ( read_board32 )
     AM_RANGE( 0x00800040, 0x00800043) AM_READ  ( read_newkeys32 )
     AM_RANGE( 0x00800088, 0x0080008b) AM_WRITE ( write_keys32 )
     AM_RANGE( 0x0010000, 0x001FFFF)   AM_RAM      // 64KB
ADDRESS_MAP_END

static INPUT_PORTS_START( new_keyboard ) //Amsterdam, Dallas 32, Roma, Roma 32
  PORT_START
  PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A1")  PORT_CODE(KEYCODE_A )
  PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B2")  PORT_CODE(KEYCODE_B )
  PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C3")  PORT_CODE(KEYCODE_C )
  PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D4")  PORT_CODE(KEYCODE_D )
  PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E5")  PORT_CODE(KEYCODE_E )
  PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F6")  PORT_CODE(KEYCODE_F )
  PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A1")  PORT_CODE(KEYCODE_1 )
  PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B2")  PORT_CODE(KEYCODE_2 )
  PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C3")  PORT_CODE(KEYCODE_3 )
  PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D4")  PORT_CODE(KEYCODE_4 )
  PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E5")  PORT_CODE(KEYCODE_5 )
  PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F6")  PORT_CODE(KEYCODE_6 )
  PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0")   PORT_CODE(KEYCODE_9 )
  PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9")   PORT_CODE(KEYCODE_0 )
  PORT_START
  PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("INF") PORT_CODE(KEYCODE_F1 )
  PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("POS") PORT_CODE(KEYCODE_F2 )
  PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("LEV") PORT_CODE(KEYCODE_F3 )
  PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("MEM") PORT_CODE(KEYCODE_F4 )
  PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CLR") PORT_CODE(KEYCODE_F5 )
  PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ENT") PORT_CODE(KEYCODE_ENTER )
  PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G7")  PORT_CODE(KEYCODE_G )
  PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H8")  PORT_CODE(KEYCODE_H )
  PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G7")  PORT_CODE(KEYCODE_7 )
  PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H8")  PORT_CODE(KEYCODE_8 )
INPUT_PORTS_END



static INPUT_PORTS_START( old_keyboard )   //Glasgow,Dallas
  PORT_START
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9")   PORT_CODE(KEYCODE_9 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CL")  PORT_CODE(KEYCODE_F5 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C3")  PORT_CODE(KEYCODE_C )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C3")  PORT_CODE(KEYCODE_3 )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENT") PORT_CODE(KEYCODE_ENTER )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D4")  PORT_CODE(KEYCODE_D )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D4")  PORT_CODE(KEYCODE_4 )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A1")  PORT_CODE(KEYCODE_A )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A1")  PORT_CODE(KEYCODE_1 )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F6")  PORT_CODE(KEYCODE_F )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F6")  PORT_CODE(KEYCODE_6 )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B2")  PORT_CODE(KEYCODE_B )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B2")  PORT_CODE(KEYCODE_2 )
  PORT_START
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E5")  PORT_CODE(KEYCODE_E )
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E5")  PORT_CODE(KEYCODE_5 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("INF") PORT_CODE(KEYCODE_F1 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0")   PORT_CODE(KEYCODE_0 )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("POS") PORT_CODE(KEYCODE_F2 )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H8")  PORT_CODE(KEYCODE_H )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H8")  PORT_CODE(KEYCODE_8 )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LEV") PORT_CODE(KEYCODE_F3)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G7")  PORT_CODE(KEYCODE_G )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G7")  PORT_CODE(KEYCODE_7 )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MEM") PORT_CODE(KEYCODE_F4)

   	PORT_START
	PORT_BIT( 0xffff, 0x00, IPT_MOUSE_X)  PORT_SENSITIVITY(100) PORT_KEYDELTA(1) PORT_MINMAX(0, 65535) 	PORT_PLAYER(1)

    PORT_START
	PORT_BIT( 0xffff, 0x00, IPT_MOUSE_Y ) PORT_SENSITIVITY(100) PORT_KEYDELTA(1) PORT_MINMAX(0, 65535) 	PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_CODE(MOUSECODE_BUTTON1) PORT_NAME("left button")

   	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CODE(MOUSECODE_BUTTON2) PORT_NAME("right button")


INPUT_PORTS_END



static MACHINE_DRIVER_START(glasgow )
    /* basic machine hardware */
    MDRV_CPU_ADD_TAG("main", M68000, 12000000)
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


static MACHINE_DRIVER_START(amsterd )
	MDRV_IMPORT_FROM( glasgow )
	/* basic machine hardware */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(amsterd_mem, 0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START(dallas32 )
	MDRV_IMPORT_FROM( glasgow )
	/* basic machine hardware */
	MDRV_CPU_REPLACE("main", M68020, 14000000)
	MDRV_CPU_PROGRAM_MAP(dallas32_mem, 0)
	MDRV_MACHINE_START( dallas32 )
MACHINE_DRIVER_END


/***************************************************************************
  ROM definitions
***************************************************************************/

ROM_START( glasgow )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )
    //ROM_LOAD("glasgow.rom", 0x000000, 0x10000, CRC(3e73eff3) )
    ROM_LOAD16_BYTE("me3_3_1u.410",0x00000, 0x04000,CRC(bc8053ba) SHA1(57ea2d5652bfdd77b17d52ab1914de974bd6be12))
    ROM_LOAD16_BYTE("me3_1_1l.410",0x00001, 0x04000,CRC(d5263c39) SHA1(1bef1cf3fd96221eb19faecb6ec921e26ac10ac4))
    ROM_LOAD16_BYTE("me3_4_2u.410",0x08000, 0x04000,CRC(8dba504a) SHA1(6bfab03af835cdb6c98773164d32c76520937efe))
    ROM_LOAD16_BYTE("me3_2_2l.410",0x08001, 0x04000,CRC(b3f27827) SHA1(864ba897d24024592d08c4ae090aa70a2cc5f213))
ROM_END

ROM_START( amsterd )
    ROM_REGION16_BE( 0x1000000, REGION_CPU1, 0 )
    //ROM_LOAD16_BYTE("output.bin", 0x000000, 0x10000, CRC(3e73eff3) )
    ROM_LOAD16_BYTE("amsterda-u.bin",0x00000, 0x05a00,CRC(16cefe29) SHA1(9f8c2896e92fbfd47159a59cb5e87706092c86f4))
    ROM_LOAD16_BYTE("amsterda-l.bin",0x00001, 0x05a00,CRC(c859dfde) SHA1(b0bca6a8e698c322a8c597608db6735129d6cdf0))
ROM_END


ROM_START( dallas )
    ROM_REGION16_BE( 0x1000000, REGION_CPU1, 0 )
    ROM_LOAD16_BYTE("dal_g_pr.dat",0x00000, 0x04000,CRC(66deade9) SHA1(07ec6b923f2f053172737f1fc94aec84f3ea8da1))
    ROM_LOAD16_BYTE("dal_g_pl.dat",0x00001, 0x04000,CRC(c5b6171c) SHA1(663167a3839ed7508ecb44fd5a1b2d3d8e466763))
    ROM_LOAD16_BYTE("dal_g_br.dat",0x08000, 0x04000,CRC(e24d7ec7) SHA1(a936f6fcbe9bfa49bf455f2d8a8243d1395768c1))
    ROM_LOAD16_BYTE("dal_g_bl.dat",0x08001, 0x04000,CRC(144a15e2) SHA1(c4fcc23d55fa5262f5e01dbd000644a7feb78f32))
ROM_END

ROM_START( dallas16 )
    ROM_REGION16_BE( 0x1000000, REGION_CPU1, 0 )
    ROM_LOAD16_BYTE("dallas-u.bin",0x00000, 0x06f00,CRC(8c1462b4) SHA1(8b5f5a774a835446d08dceacac42357b9e74cfe8))
    ROM_LOAD16_BYTE("dallas-l.bin",0x00001, 0x06f00,CRC(f0d5bc03) SHA1(4b1b9a71663d5321820b4cf7da205e5fe5d3d001))
ROM_END

ROM_START( roma )
    ROM_REGION16_BE( 0x1000000, REGION_CPU1, 0 )
    ROM_LOAD("roma32.bin", 0x000000, 0x10000, CRC(587d03bf) SHA1(504e9ff958084700076d633f9c306fc7baf64ffd))
ROM_END

ROM_START( dallas32 )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD("dallas32.epr", 0x000000, 0x10000, CRC(83b9ff3f) SHA1(97bf4cb3c61f8ec328735b3c98281bba44b30a28) )
ROM_END

ROM_START( roma32 )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD("roma32.bin", 0x000000, 0x10000, CRC(587d03bf) SHA1(504e9ff958084700076d633f9c306fc7baf64ffd) )
ROM_END

/***************************************************************************
  System config
***************************************************************************/



/***************************************************************************
  Game drivers
***************************************************************************/

/*     YEAR, NAME,     PARENT,   BIOS, COMPAT MACHINE,INPUT,          INIT,     CONFIG, COMPANY,                      FULLNAME,                 FLAGS */
CONS(  1984, glasgow,  0,        0,    glasgow,       old_keyboard,   NULL,     NULL,   "Hegener & Glaser Muenchen",  "Mephisto III S Glasgow", 0)
CONS(  1984, amsterd,  0,        0,    amsterd,       new_keyboard,   NULL,     NULL,   "Hegener & Glaser Muenchen",  "Mephisto Amsterdam",     0)
CONS(  1984, dallas,   0,        0,    glasgow,       old_keyboard,   NULL,     NULL,   "Hegener & Glaser Muenchen",  "Mephisto Dallas",        0)
CONS(  1984, roma,     0,        0,    glasgow,       new_keyboard,   NULL,     NULL,   "Hegener & Glaser Muenchen",  "Mephisto Roma",          0)
CONS(  1984, dallas32, 0,        0,    dallas32,      new_keyboard,   NULL,     NULL,   "Hegener & Glaser Muenchen",  "Mephisto Dallas 32 Bit", 0)
CONS(  1984, roma32,   0,        0,    dallas32,      new_keyboard,   NULL,     NULL,   "Hegener & Glaser Muenchen",  "Mephisto Roma 32 Bit",   0)
CONS(  1984, dallas16, 0,        0,    amsterd,       new_keyboard,   NULL,     NULL,   "Hegener & Glaser Muenchen",  "Mephisto Dallas 16 Bit", 0)



static void set_cursor (view_item *view_cursor)
  {

   static INT16 save_x, save_y;
   static float diff_x, diff_y, del_x, del_y;

    save_x = m_x;
	save_y = m_y;

    m_x=readinputport(0x02);
    m_y=readinputport(0x03);

//    logerror("m_x   = %d \n  ",m_x);
//    logerror("m_y   = %d \n  ",m_y);

    diff_x = (float) (save_x - m_x) / MOUSE_SPEED;
    diff_y = (float) (save_y - m_y) / MOUSE_SPEED;

    del_x = view_cursor->rawbounds.x1 - view_cursor->rawbounds.x0;
    del_y = view_cursor->rawbounds.y1 - view_cursor->rawbounds.y0;

// Set Cursor, depending on limits
//
      view_cursor->rawbounds.x0 =   view_cursor->rawbounds.x0 - diff_x;

      if ( view_cursor->rawbounds.x0 > MAX_CURSOR_X )
           view_cursor->rawbounds.x0 = MAX_CURSOR_X;

	  if ( view_cursor->rawbounds.x0 < MIN_CURSOR_X )
           view_cursor->rawbounds.x0 = MIN_CURSOR_X;

      view_cursor->rawbounds.y0 =   view_cursor->rawbounds.y0 - diff_y;

      if ( view_cursor->rawbounds.y0 > MAX_CURSOR_Y )
           view_cursor->rawbounds.y0 = MAX_CURSOR_Y;

	  if ( view_cursor->rawbounds.y0 < MIN_CURSOR_Y )
         view_cursor->rawbounds.y0 = MIN_CURSOR_Y;

      view_cursor->rawbounds.x1 = view_cursor->rawbounds.x0 + del_x;
      view_cursor->rawbounds.y1 = view_cursor->rawbounds.y0 + del_y;

// Calculate bounds
//
	  view_cursor->bounds.x0 = view_cursor->rawbounds.x0 / MAX_X;
	  view_cursor->bounds.y0 = view_cursor->rawbounds.y0 / MAX_Y;
	  view_cursor->bounds.x1 = view_cursor->rawbounds.x1 / MAX_X;
	  view_cursor->bounds.y1 = view_cursor->rawbounds.y1 / MAX_Y;

  }

//render_target *render_target_get_indexed


view_item *get_view_item(render_target *target, const char *v_name)
{

 view_item *cur_view;
 cur_view = target->curview->itemlist[3]->next;

 for (; cur_view != NULL; cur_view = cur_view->next)
 {
	if (!strcmp(cur_view->name, v_name))
		{
		return (cur_view);
		}
 }

return (NULL);
}


BOARD_FIELD get_field( float i_x, float i_y, UINT8 mouse_move)
{
UINT8 i_AH, i_18;
float corr_x, corr_y;


float f_x, f_y;

BOARD_FIELD null_board  = { 0,0,0,"0"};


// Correction of mouse pointer
//
     if (!mouse_move)
		{
		corr_x = i_x+29.0;
		corr_y = i_y+22.0;
		}else
	    {
		corr_x = i_x+28;
		corr_y = i_y+28;
		}

// Loop über alle Spalten
		for ( i_AH = 0; i_AH < 8; i_AH = i_AH + 1)
		{

            for ( i_18 = 0; i_18 < 8; i_18 = i_18 + 1)
				{
				 f_x =	(float) m_board [i_18] [i_AH].x;
				 f_y =	(float) m_board [i_18] [i_AH].y;

                if (   ( corr_x >= f_x  && corr_x <= f_x+56 ) && ( corr_y >= f_y  && corr_y <= f_y+56 ) )
				 {
				 return (m_board [i_18] [i_AH]);
				 }

				}
		}

return (null_board);
}


static unsigned int out_of_board( float x0, float y0)
{

	if ( ( x0 >= MIN_BOARD_X  && x0 <= MAX_BOARD_X ) && ( y0 >= MIN_BOARD_Y  && y0 <= MAX_BOARD_Y ) )
		{
		return (0);
		}
	else
		{
		return (1);
		}
}



static void update_board(render_target *target, BOARD_FIELD board_field, unsigned int update_clear_flag)
{

   UINT8 i_18, i_AH;

   view_item *view_item;

   div_t div_result;

   div_result = div(board_field.field, 8);

   i_18 = div_result.quot;
   i_AH = 7 - div_result.rem;

   if (update_clear_flag)
	{
// there is already a piece on target field - > set it invisible
//
		view_item = get_view_item(target, (char *)m_board[i_18][i_AH].piece);
		if (view_item != NULL)
			{
			view_item->color.a = 0;
			}
		strcpy ((char *)&m_board[i_18][i_AH].piece[0], (char *)&board_field.piece[0]);
	}else
	{
		strcpy ((char *)m_board[i_18][i_AH].piece, EMP);
	}

	set_status_of_pieces();

}

 static void calculate_bounds(view_item *view_item, float new_x0, float new_y0, float new_del_x, float new_del_y )
  {

   static float del_x, del_y;

      if (new_del_x)
		{
		del_x = new_del_x;
		}
	  else
		{
		del_x = view_item->rawbounds.x1 - view_item->rawbounds.x0;
		}

      if (new_del_y)
		{
		del_y = new_del_y;
		}
	  else
		{
		del_y = view_item->rawbounds.y1 - view_item->rawbounds.y0;
		}

      view_item->rawbounds.x0 = new_x0;
      view_item->rawbounds.y0 = new_y0;
      view_item->rawbounds.x1 = view_item->rawbounds.x0 + del_x;
      view_item->rawbounds.y1 = view_item->rawbounds.y0 + del_y;

// Calculate bounds
//
	  view_item->bounds.x0 = view_item->rawbounds.x0 / MAX_X;
	  view_item->bounds.y0 = view_item->rawbounds.y0 / MAX_Y;
	  view_item->bounds.x1 = view_item->rawbounds.x1 / MAX_X;
	  view_item->bounds.y1 = view_item->rawbounds.y1 / MAX_Y;

  }


static void set_render_board()
	{

    view_item *view_item;
    UINT8 i_AH, i_18;

	my_target = render_get_ui_target();


    for ( i_AH = 0; i_AH < 8; i_AH = i_AH + 1)
		{
         for ( i_18 = 0; i_18 < 8; i_18 = i_18 + 1)
			{

// copy start postition to m_board
//
            m_board[i_18][i_AH] = start_board [i_18] [i_AH];

// Get view item of this piece
//
			if (strcmp((char *)m_board[i_18][i_AH].piece, EMP))
				{
				view_item = get_view_item(my_target, (char *)m_board[i_18][i_AH].piece);

// change all pieces
//
				if (view_item != NULL)
					{
					calculate_bounds(view_item, m_board[i_18][i_AH].x, m_board[i_18][i_AH].y, 0, 0 );
					view_item->color.a = 1.0;
					}//ENDIF view_item

				} // ENDIF strcmp
			}// FOR i_18
		}// FOR i_AH

    render_set_ui_target (my_target);

	}

static void clear_layout()
	{

    view_item *view_item;
    UINT8 i;

	my_target = render_get_ui_target();

	for ( i = 0; i < 44; i = i + 1)
		{
         view_item = get_view_item(my_target,(char *)all_pieces[i].piece);
		 if (view_item != NULL)
			{
			view_item->color.a = 0.0;
			}
		}

    render_set_ui_target (my_target);

	}



static void set_status_of_pieces()
	{

    UINT8 i_AH, i_18 , i;
    BOARD_FIELD field;


	for ( i = 0; i < 47; i = i + 1)
		{

        all_pieces[i].set = 0;

	    for ( i_AH = 0; i_AH < 8; i_AH = i_AH + 1)
			{
			for ( i_18 = 0; i_18 < 8; i_18 = i_18 + 1)
				{

// set status depending piece is in game or not
//
            field = m_board[i_18][i_AH];

				if (!strcmp((char *)m_board[i_18][i_AH].piece, (char *) all_pieces[i].piece))
					{
					all_pieces[i].set = 1;
					break;
					}

				} // END for i_18=0

			if (all_pieces[i].set) break;

			}// END for i_AH=0
		} // END for i=0

	}


static char *get_non_set_pieces(const char *cur_piece)
{
	UINT8 i=0;

	do
		{

		start_i = start_i+1;
		if ( start_i ==  44) start_i = 0;


	    if (!all_pieces[start_i].set && ( all_pieces[start_i].piece[0] != cur_piece[0] ||
		                                  all_pieces[start_i].piece[1] != cur_piece[1]) )
  			{
			return ((char *)&all_pieces[start_i].piece[0]);
			}

		i=i+1;
}
	while (i != 44);

	return (NO_PIECE);
}


