////////////////////////////////////////////////////////////////////////////
//                                                                        //
// vacfdisp.c: Vacuum Fluorescent Display and Controller emulation        //
//                                                                        //
// emulated displays: BFM BD1 vfd display, OKI MSC1937                    //
//                                                                        //
// 05-03-2004: Re-Animator                                                //
//                                                                        //
// Theory: VFD controllers have a built in character table, which converts//
// the displayable characters into on/off signals for the various segments//
// However, each controller has its own way of assigning the segments,    //
// making standardisation a problem.                                      //
// Currently, the model used by the OKIMSC1937 is considered as a standard//
// with the BD1 model converted to it at drawing time.                    //
//                                                                        //
// TODO: - BFM sys85 seems to use a slightly different vfd to OKI MSC1937 //
//       - Add 'skew' to display - characters usually slope 20 degrees    //
//       - Implement display flashing (need precise rate figures)         //
//       - Make display better (convert to segment shapes like LED)       //
//                                                                        //
// Any fixes for this driver should be forwarded to AGEMAME HQ            //
// (http://www.mameworld.net/agemame/)                                    //
//                                                                        //
////////////////////////////////////////////////////////////////////////////

#include "driver.h"
#include "vacfdisp.h"

static struct
{
	UINT8	type,				// type of alpha display
								// VFDTYPE_BFMBD1 or VFDTYPE_MSC1937
			reversed,			// Allows for the data being written from right to left, not left to right.

			changed,			// flag <>0, if vfd contents are changed
			window_start,		// display window start pos 0-15
			window_end,			// display window end   pos 0-15
			window_size,		// window  size
			pad;				// unused align byte

	INT8	pcursor_pos,		// previous cursor pos
			cursor_pos;			// current cursor pos

	UINT16  brightness;			// display brightness level 0-31 (31=MAX)

	UINT16  user_def,			// user defined character state
			user_data;			// user defined character data (16 bit)

	UINT8	scroll_active,		// flag <>0, scrolling active
			display_mode,		// display scroll   mode, 0/1/2/3
			display_blanking,	// display blanking mode, 0/1/2/3
			flash_rate,			// flash rate 0-F
			flash_control;		// flash control 0/1/2/3

	UINT8	string[18];			// text buffer
	UINT16  segments[16],		// segments
			outputs[16];		// standardised outputs

	UINT8	count,				// bit counter
			data;				// receive register

} vfds[MAX_VFDS];


// local prototypes ///////////////////////////////////////////////////////

static void ScrollLeft( int id);
static int  BD1_setdata(int id, int segdata, int data);

// local vars /////////////////////////////////////////////////////////////

//
// Bellfruit BD1 charset to ASCII conversion table
//
static const char BFM2ASCII[] =
//0123456789ABCDEF0123456789ABC DEF01 23456789ABCDEF0123456789ABCDEF
 "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ ?\"#$%%'()*+.-./0123456789&%<=>?"\
 "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ ?\"#$%%'()*+.-./0123456789&%<=>?"\
 "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ ?\"#$%%'()*+.-./0123456789&%<=>?";

//
// OKI MSC1937 charset to ASCII conversion table
//
static const char OKI1937ASCII[]=
//0123456789ABCDEF0123456789ABC DEF01 23456789ABCDEF0123456789ABCDEF
 "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ ?\"#$%%'()*+;-./0123456789&%<=>?"\
 "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ ?\"#$%%'()*+;-./0123456789&%<=>?"\
 "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ ?\"#$%%'()*+;-./0123456789&%<=>?";

/*
   BD1 14 segment charset lookup table
        2
    ---------
   |\   |3  /|
 0 | \6 |  /7| 1
   |  \ | /  |
    -F-- --E-
   |  / | \  |
 D | /4 |A \B| 5
   |/   |   \|
    ---------  C
        9

  */

static unsigned short BFMcharset[]=
{           // FEDC BA98 7654 3210
	0xA626, // 1010 0110 0010 0110 @.
	0xE027, // 1110 0000 0010 0111 A.
	0x462E, // 0100 0110 0010 1110 B.
	0x2205, // 0010 0010 0000 0101 C.
	0x062E, // 0000 0110 0010 1110 D.
	0xA205, // 1010 0010 0000 0101 E.
	0xA005, // 1010 0000 0000 0101 F.
	0x6225, // 0110 0010 0010 0101 G.
	0xE023, // 1110 0000 0010 0011 H.
	0x060C, // 0000 0110 0000 1100 I.
	0x2222, // 0010 0010 0010 0010 J.
	0xA881, // 1010 1000 1000 0001 K.
	0x2201, // 0010 0010 0000 0001 L.
	0x20E3, // 0010 0000 1110 0011 M.
	0x2863, // 0010 1000 0110 0011 N.
	0x2227, // 0010 0010 0010 0111 O.
	0xE007, // 1110 0000 0000 0111 P.
	0x2A27, // 0010 1010 0010 0111 Q.
	0xE807, // 1110 1000 0000 0111 R.
	0xC225, // 1100 0010 0010 0101 S.
	0x040C, // 0000 0100 0000 1100 T.
	0x2223, // 0010 0010 0010 0011 U.
	0x2091, // 0010 0000 1001 0001 V.
	0x2833, // 0010 1000 0011 0011 W.
	0x08D0, // 0000 1000 1101 0000 X.
	0x04C0, // 0000 0100 1100 0000 Y.
	0x0294, // 0000 0010 1001 0100 Z.
	0x2205, // 0010 0010 0000 0101 [.
	0x0840, // 0000 1000 0100 0000 \.
	0x0226, // 0000 0010 0010 0110 ].
	0x0810, // 0000 1000 0001 0000 ^.
	0x0200, // 0000 0010 0000 0000 _
	0x0000, // 0000 0000 0000 0000
	0xC290, // 1100 0010 1001 0000 POUND.
	0x0009, // 0000 0000 0000 1001 ".
	0xC62A, // 1100 0110 0010 1010 #.
	0xC62D, // 1100 0110 0010 1101 $.
	0x0100, // 0000 0000 0000 0000 flash ?
	0x0000, // 0000 0000 0000 0000 not defined
	0x0040, // 0000 0000 1000 0000 '.
	0x0880, // 0000 1000 1000 0000 (.
	0x0050, // 0000 0000 0101 0000 ).
	0xCCD8, // 1100 1100 1101 1000 *.
	0xC408, // 1100 0100 0000 1000 +.
	0x1000, // 0001 0000 0000 0000 .
	0xC000, // 1100 0000 0000 0000 -.
	0x1000, // 0001 0000 0000 0000 ..
	0x0090, // 0000 0000 1001 0000 /.
	0x22B7, // 0010 0010 1011 0111 0.
	0x0408, // 0000 0100 0000 1000 1.
	0xE206, // 1110 0010 0000 0110 2.
	0x4226, // 0100 0010 0010 0110 3.
	0xC023, // 1100 0000 0010 0011 4.
	0xC225, // 1100 0010 0010 0101 5.
	0xE225, // 1110 0010 0010 0101 6.
	0x0026, // 0000 0000 0010 0110 7.
	0xE227, // 1110 0010 0010 0111 8.
	0xC227, // 1100 0010 0010 0111 9.
	0xFFFF, // 0000 0000 0000 0000 user defined, can be replaced by main program
	0x0000, // 0000 0000 0000 0000 dummy
	0x0290, // 0000 0010 1001 0000 <.
	0xC200, // 1100 0010 0000 0000 =.
	0x0A40, // 0000 1010 0100 0000 >.
	0x4406, // 0100 0100 0000 0110 ?
};

/*
   MSC1937 16 segment charset lookup table
     0     1
    ---- ----
   |\   |   /|
 7 | \F |8 /9| 2
   |  \ | /  |
    -E-- --A-
   |  / | \  |
 6 | /D |C \B| 3
   |/   |   \|
    ---- ----  .11
      5    4   ,10

In 14 segment mode, 0 represents the whole top line,
and 5 the bottom line, allowing both modes to share
a charset.
*/

static unsigned int OKIcharset[]=
{           // 11 10 FEDC BA98 7654 3210
	0x0507F, //  0  0 0101 0000 0111 1111 @.
	0x044CF, //  0  0 0100 0100 1100 1111 A.
	0x0153F, //  0  0 0001 0101 0011 1111 B.
	0x000F3, //  0  0 0000 0000 1111 0011 C.
	0x0113F, //  0  0 0001 0001 0011 1111 D.
	0x040F3, //  0  0 0100 0000 1111 0011 E.
	0x040C3, //  0  0 0100 0000 1100 0011 F.
	0x004FB, //  0  0 0000 0100 1111 1011 G.
	0x044CC, //  0  0 0100 0100 1100 1100 H.
	0x01133, //  0  0 0001 0001 0011 0011 I.
	0x0007C, //  0  0 0000 0000 0111 1100 J.
	0x04AC0, //  0  0 0100 1010 1100 0000 K.
	0x000F0, //  0  0 0000 0000 1111 0000 L.
	0x082CC, //  0  0 1000 0010 1100 1100 M.
	0x088CC, //  0  0 1000 1000 1100 1100 N.
	0x000FF, //  0  0 0000 0000 1111 1111 O.
	0x044C7, //  0  0 0100 0100 1100 0111 P.
	0x008FF, //  0  0 0000 1000 1111 1111 Q.
	0x04CC7, //  0  0 0100 1100 1100 0111 R.
	0x044BB, //  0  0 0100 0100 1011 1011 S.
	0x01103, //  0  0 0001 0001 0000 0011 T.
	0x000FC, //  0  0 0000 0000 1111 1100 U.
	0x022C0, //  0  0 0010 0010 1100 0000 V.
	0x028CC, //  0  0 0010 1000 1100 1100 W.
	0x0AA00, //  0  0 1010 1010 0000 0000 X.
	0x09200, //  0  0 1001 0010 0000 0000 Y.
	0x02233, //  0  0 0010 0010 0011 0011 Z.
	0x000E1, //  0  0 0000 0000 1110 0001 [.
	0x08800, //  0  0 1000 1000 0000 0000 \.
	0x0001E, //  0  0 0000 0000 0001 1110 ].
	0x02800, //  0  0 0010 1000 0000 0000 ^.
	0x00030, //  0  0 0000 0000 0011 0000 _.
	0x00000, //  0  0 0000 0000 0000 0000 dummy.
	0x08121, //  0  0 1000 0001 0010 0001 Unknown symbol.
	0x00180, //  0  0 0000 0001 1000 0000 ".
	0x0553C, //  0  0 0101 0101 0011 1100 #.
	0x055BB, //  0  0 0101 0101 1011 1011 $.
	0x07799, //  0  0 0111 0111 1001 1001 %.
	0x0C979, //  0  0 1100 1001 0111 1001 &.
	0x00200, //  0  0 0000 0010 0000 0000 '.
	0x00A00, //  0  0 0000 1010 0000 0000 (.
	0x0A050, //  0  0 1010 0000 0000 0000 ).
	0x0FF00, //  0  0 1111 1111 0000 0000 *.
	0x05500, //  0  0 0101 0101 0000 0000 +.
	0x30000, //  1  1 0000 0000 0000 0000 ;.
	0x04400, //  0  0 0100 0100 0000 0000 --.
	0x20000, //  1  0 0000 0000 0000 0000 . .
	0x02200, //  0  0 0010 0010 0000 0000 /.
	0x022FF, //  0  0 0010 0010 1111 1111 0.
	0x01100, //  0  0 0001 0001 0000 0000 1.
	0x04477, //  0  0 0100 0100 0111 0111 2.
	0x0443F, //  0  0 0100 0100 0011 1111 3.
	0x0448C, //  0  0 0100 0100 1000 1100 4.
	0x044BB, //  0  0 0100 0100 1011 1011 5.
	0x044FB, //  0  0 0100 0100 1111 1011 6.
	0x0000E, //  0  0 0000 0000 0000 1110 7.
	0x044FF, //  0  0 0100 0100 1111 1111 8.
	0x044BF, //  0  0 0100 0100 1011 1111 9.
	0x00021, //  0  0 0000 0000 0010 0001 -
	         //                           -.
	0x02001, //  0  0 0010 0000 0000 0001 -
			 //                           /.
	0x02430, //  0  0 0010 0100 0011 0000 <.
	0x04430, //  0  0 0100 0100 0011 0000 =.
	0x08830, //  0  0 1000 1000 0011 0000 >.
	0x01407, //  0  0 0001 0100 0000 0111 ?.
};

static const int poslut1937[]=
{
	1,//0
	2,
	3,
	4,
	5,
	6,
	7,
	8,
	9,
	10,
	11,
	12,
	13,
	14,
	15,
	0//15
};

///////////////////////////////////////////////////////////////////////////

void vfd_init(int id, int type ,int reversed)
{
	memset( &vfds[id], 0, sizeof(vfds[0]));

	vfds[id].type = type;
	vfds[id].reversed = reversed;

	vfd_reset(id);
}

///////////////////////////////////////////////////////////////////////////

void vfd_reset(int id)
{
	if ( id > 2 || id < 0 ) return;

	vfds[id].window_end  = 15;
	vfds[id].window_size = (vfds[id].window_end - vfds[id].window_start)+1;
	memset(vfds[id].string, ' ', 16);

	vfds[id].brightness = 31;
	vfds[id].count      = 0;

	vfds[id].changed |= 1;
}

///////////////////////////////////////////////////////////////////////////

UINT16 *vfd_get_segments(int id)
{
	return vfds[id].segments;
}

///////////////////////////////////////////////////////////////////////////

UINT16 *vfd_get_outputs(int id)
{
	return vfds[id].outputs;
}

///////////////////////////////////////////////////////////////////////////

UINT16 *vfd_set_outputs(int id)
{
	int cursor;
	switch ( vfds[id].type )
	{
		case VFDTYPE_MSC1937:
		{
			for (cursor = 0; cursor < 16; cursor++)
			{
				if (!vfds[id].reversed)//Output to the screen is naturally backwards, so we need to invert it
				{
					vfds[id].outputs[cursor] = vfd_get_segments(id)[15-cursor];
				}
				else
				{
					//If the controller is reversed, things look normal.
					vfds[id].outputs[cursor] = vfd_get_segments(id)[cursor];
				}
			}
		}
		break;
		case VFDTYPE_BFMBD1:
		{
			for (cursor = 0; cursor < 16; cursor++)
			{
				if ( vfd_get_segments(id)[cursor] & 0x01 )	vfds[id].outputs[cursor] |=  0x0080;
				else                        				vfds[id].outputs[cursor] &= ~0x0080;
				if ( vfd_get_segments(id)[cursor] & 0x02 )	vfds[id].outputs[cursor] |=  0x0004;
				else                        				vfds[id].outputs[cursor] &= ~0x0004;
				if ( vfd_get_segments(id)[cursor] & 0x04 )	vfds[id].outputs[cursor] |=  0x0001;
				else                        				vfds[id].outputs[cursor] &= ~0x0001;
				if ( vfd_get_segments(id)[cursor] & 0x08 )	vfds[id].outputs[cursor] |=  0x0100;
				else                        				vfds[id].outputs[cursor] &= ~0x0100;
				if ( vfd_get_segments(id)[cursor] & 0x10 )	vfds[id].outputs[cursor] |=  0x2000;
				else                        				vfds[id].outputs[cursor] &= ~0x2000;
				if ( vfd_get_segments(id)[cursor] & 0x20 )	vfds[id].outputs[cursor] |=  0x0008;
				else                        				vfds[id].outputs[cursor] &= ~0x0008;
				if ( vfd_get_segments(id)[cursor] & 0x40 )	vfds[id].outputs[cursor] |=  0x8000;
				else                        				vfds[id].outputs[cursor] &= ~0x8000;
				if ( vfd_get_segments(id)[cursor] & 0x80 )	vfds[id].outputs[cursor] |=  0x0200;
				else                        				vfds[id].outputs[cursor] &= ~0x0200;

				//Flashing ? Set an unused pin as a control
				if ( vfd_get_segments(id)[cursor] & 0x100 )	vfds[id].outputs[cursor] |=  0x40000;
				else                        				vfds[id].outputs[cursor] &= ~0x40000;

				if ( vfd_get_segments(id)[cursor] & 0x200 )	vfds[id].outputs[cursor] |=  0x0020;
				else                        				vfds[id].outputs[cursor] &= ~0x0020;
				if ( vfd_get_segments(id)[cursor] & 0x400 )	vfds[id].outputs[cursor] |=  0x1000;
				else                        				vfds[id].outputs[cursor] &= ~0x1000;
				if ( vfd_get_segments(id)[cursor] & 0x800 )	vfds[id].outputs[cursor] |=  0x0800;
				else                        				vfds[id].outputs[cursor] &= ~0x0800;
				if ( vfd_get_segments(id)[cursor] & 0x1000 )vfds[id].outputs[cursor] |=  0x20000;
				else                        				vfds[id].outputs[cursor] &= ~0x20000;
				if ( vfd_get_segments(id)[cursor] & 0x2000 )vfds[id].outputs[cursor] |=  0x0040;
				else                        				vfds[id].outputs[cursor] &= ~0x0040;
				if ( vfd_get_segments(id)[cursor] & 0x4000 )vfds[id].outputs[cursor] |=  0x0400;
				else                        				vfds[id].outputs[cursor] &= ~0x0400;
				if ( vfd_get_segments(id)[cursor] & 0x8000 )vfds[id].outputs[cursor] |=  0x4000;
				else                        				vfds[id].outputs[cursor] &= ~0x4000;
			}
		}
	}
	return 0;
}
///////////////////////////////////////////////////////////////////////////

char  *vfd_get_string( int id)
{
	return (char *)vfds[id].string;
}

///////////////////////////////////////////////////////////////////////////

void vfd_shift_data(int id, int data)
{
	if ( id > 2 || id < 0 ) return;

	vfds[id].data <<= 1;

	if ( !data ) vfds[id].data |= 1;

	if ( ++vfds[id].count >= 8 )
	{
		if ( vfd_newdata(id, vfds[id].data) )
		{
			vfds[id].changed |= 1;
		}
	//logerror("vfd %3d -> %02X \"%s\"\n", id, vfds[id].data, vfds[id].string);

	vfds[id].count = 0;
	vfds[id].data  = 0;
  }
}

///////////////////////////////////////////////////////////////////////////

int vfd_newdata(int id, int data)
{
	int change = 0;
	int cursor;

	switch ( vfds[id].type )
	{
		case VFDTYPE_BFMBD1:

		if ( vfds[id].user_def )
		{
			vfds[id].user_def--;

			vfds[id].user_data <<= 8;
			vfds[id].user_data |= data;

			if ( vfds[id].user_def )
			{
				return 0;
			}

		data = '@';
		change = BD1_setdata(id, vfds[id].user_def, data);
		}
		else
		{
		}

		switch ( data & 0xF0 )
		{
			case 0x80:	// 0x80 - 0x8F Set display blanking

			vfds[id].display_blanking = data & 0x0F;
			change = 1;
			break;

			case 0x90:	// 0x90 - 0x9F Set cursor pos

			vfds[id].cursor_pos = data & 0x0F;

			vfds[id].scroll_active = 0;
			if ( vfds[id].display_mode == 2 )
			{
				if ( vfds[id].cursor_pos >= vfds[id].window_end) vfds[id].scroll_active = 1;
			}
			break;

			case 0xA0:	// 0xA0 - 0xAF Set display mode

			vfds[id].display_mode = data &0x03;
			break;

			case 0xB0:	// 0xB0 - 0xBF Clear display area

			switch ( data & 0x03 )
			{
				case 0x00:	// clr nothing
				break;

				case 0x01:	// clr inside window

				if ( vfds[id].window_size > 0 )
					{
						memset( vfds[id].string+vfds[id].window_start, ' ',vfds[id].window_size );
					}
					break;

				case 0x02:	// clr outside window

				if ( vfds[id].window_size > 0 )
					{
						if ( vfds[id].window_start > 0 )
						{
							memset( vfds[id].string, ' ', vfds[id].window_start);
							for (cursor = 0; cursor < vfds[id].window_start; cursor++)
							{
								vfds[id].segments[cursor] = 0x0000;
							}
						}

						if (vfds[id].window_end < 15 )
						{
							memset( vfds[id].string+vfds[id].window_end, ' ', 15-vfds[id].window_end);
							for (cursor = vfds[id].window_end; cursor < 15-vfds[id].window_end; cursor++)
							{
								vfds[id].segments[cursor] = 0x0000;
							}

						}
					}
				case 0x03:	// clr entire display

				memset(vfds[id].string, ' ' , 16);
				for (cursor = 0; cursor < 16; cursor++)
				{
				vfds[id].segments[cursor] = 0x0000;
				}
				break;
			}
			change = 1;
			break;

			case 0xC0:	// 0xC0 - 0xCF Set flash rate

			vfds[id].flash_rate = data & 0x0F;
			break;

			case 0xD0:	// 0xD0 - 0xDF Set Flash control

			vfds[id].flash_control = data & 0x03;
			break;

			case 0xE0:	// 0xE0 - 0xEF Set window start pos

			vfds[id].window_start = data &0x0F;
			vfds[id].window_size  = (vfds[id].window_end - vfds[id].window_start)+1;
			break;

			case 0xF0:	// 0xF0 - 0xFF Set window end pos

			vfds[id].window_end   = data &0x0F;
			vfds[id].window_size  = (vfds[id].window_end - vfds[id].window_start)+1;

			vfds[id].scroll_active = 0;
			if ( vfds[id].display_mode == 2 )
			{
				if ( vfds[id].cursor_pos >= vfds[id].window_end)
				{
					vfds[id].scroll_active = 1;
					vfds[id].cursor_pos    = vfds[id].window_end;
				}
			}
			break;

			default:	// normal character

			change = BD1_setdata(id, BFMcharset[data & 0x3F], data);
			break;
		}
		break;

		case VFDTYPE_MSC1937:

		if ( data & 0x80 )
		{ // Control data received
			if ( (data & 0xF0) == 0xA0 ) // 1010 xxxx
			{ // 1010 xxxx Buffer Pointer control
				vfds[id].cursor_pos = poslut1937[data & 0x0F];
			}
			else if ( (data & 0xF0) == 0xC0 ) // 1100 xxxx
				{ // 1100 xxxx Set number of digits
					data &= 0x07;

					if ( data == 0 ) vfds[id].window_size = 16;
					else             vfds[id].window_size = data+8;
					vfds[id].window_start = 0;
					vfds[id].window_end   = vfds[id].window_size-1;
				}
			else if ( (data & 0xE0) == 0xE0 ) // 111x xxxx
				{ // 111x xxxx Set duty cycle ( brightness )
					vfds[id].brightness = (data & 0xF)*2;
					change = 1;
				}
			else if ( (data & 0xE0) == 0x80 ) // 100x ---
				{ // 100x xxxx Test mode
				}
		}
		else
		{ // Display data
			data &= 0x3F;
			change = 1;

			switch ( data )
			{
				case 0x2C: // ;
				vfds[id].segments[vfds[id].pcursor_pos] |= (1<<17);//.
				vfds[id].segments[vfds[id].pcursor_pos] |= (1<<18);//,
				break;
				case 0x2E: //
				vfds[id].segments[vfds[id].pcursor_pos] |= (1<<17);//.
				break;
				default :
				vfds[id].pcursor_pos = vfds[id].cursor_pos;
				vfds[id].string[ vfds[id].cursor_pos ] = OKI1937ASCII[data];
				vfds[id].segments[vfds[id].cursor_pos] = OKIcharset[data & 0x3F];

				vfds[id].cursor_pos++;
				if (  vfds[id].cursor_pos > vfds[id].window_end )
					{
						vfds[id].cursor_pos = 0;
					}
			}
		}
		break;
	}

	return change;
}

///////////////////////////////////////////////////////////////////////////

static void ScrollLeft(int id)
{
	int i = vfds[id].window_start;

	while ( i < vfds[id].window_end )
	{
		vfds[id].string[ i ] = vfds[id].string[ i+1 ];
		vfds[id].segments[i] = vfds[id].segments[i+1];
		i++;
	}
}

///////////////////////////////////////////////////////////////////////////

static int BD1_setdata(int id, int segdata, int data)
{
	int change = 0, move = 0;

	switch ( data )
	{
		case 0x25:	// flash
		move++;
		break;

		case 0x26:  // undefined
		break;

		case 0x2C:  // semicolon
		case 0x2E:  // decimal point

		vfds[id].segments[vfds[id].pcursor_pos] |= (1<<12);
		change++;
		break;

		case 0x3B:	// dummy char
		move++;
		break;

		case 0x3A:
		vfds[id].user_def = 2;
		break;

		default:
		move   = 1;
		change = 1;
	}

	if ( move )
	{
		int mode = vfds[id].display_mode;

		vfds[id].pcursor_pos = vfds[id].cursor_pos;

		if ( vfds[id].window_size <= 0 || (vfds[id].window_size > 16))
			{ // no window selected default to rotate mode
	  			if ( mode == 2 )      mode = 0;
				else if ( mode == 3 ) mode = 1;
				//mode &= -2;
	}

	switch ( mode )
	{
		case 0: // rotate left

		vfds[id].cursor_pos &= 0x0F;

		if ( change )
		{
			vfds[id].string[vfds[id].cursor_pos]   = BFM2ASCII[data];
			vfds[id].segments[vfds[id].cursor_pos] = segdata;
		}
		vfds[id].cursor_pos++;
		if ( vfds[id].cursor_pos >= 16 ) vfds[id].cursor_pos = 0;
		break;


		case 1: // Rotate right

		vfds[id].cursor_pos &= 0x0F;

		if ( change )
		{
			vfds[id].string[vfds[id].cursor_pos]   = BFM2ASCII[data];
			vfds[id].segments[vfds[id].cursor_pos] = segdata;
		}
		vfds[id].cursor_pos--;
		if ( vfds[id].cursor_pos < 0  ) vfds[id].cursor_pos = 15;
		break;

		case 2: // Scroll left

		if ( vfds[id].cursor_pos < vfds[id].window_end )
		{
			vfds[id].scroll_active = 0;
			if ( change )
			{
				vfds[id].string[vfds[id].cursor_pos]   = BFM2ASCII[data];
				vfds[id].segments[vfds[id].cursor_pos] = segdata;
			}
			if ( move ) vfds[id].cursor_pos++;
		}
		else
		{
			if ( move )
			{
				if   ( vfds[id].scroll_active ) ScrollLeft(id);
				else                            vfds[id].scroll_active = 1;
			}

			if ( change )
			{
				vfds[id].string[vfds[id].window_end]   = BFM2ASCII[data];
				vfds[id].segments[vfds[id].cursor_pos] = segdata;
		  	}
			else
			{
				vfds[id].string[vfds[id].window_end]   = ' ';
				vfds[id].segments[vfds[id].cursor_pos] = 0;
			}
		}
		break;

		case 3: // Scroll right

		if ( vfds[id].cursor_pos > vfds[id].window_start )
			{
				if ( change )
				{
					vfds[id].string[vfds[id].cursor_pos]   = BFM2ASCII[data];
					vfds[id].segments[vfds[id].cursor_pos] = segdata;
				}
				vfds[id].cursor_pos--;
			}
			else
			{
				int i = vfds[id].window_end;

				while ( i > vfds[id].window_start )
				{
					vfds[id].string[ i ] = vfds[id].string[ i-1 ];
					vfds[id].segments[i] = vfds[id].segments[i-1];
					i--;
				}

				if ( change )
				{
					vfds[id].string[vfds[id].window_start]   = BFM2ASCII[data];
					vfds[id].segments[vfds[id].window_start] = segdata;
				}
			}
			break;
		}
	}
	return change;
}

void plot_vfd(mame_bitmap *bitmap,int vfd,int segs,int color, int col_off )
{
	int cursor;
	int	curwidth = 288/16;
	vfd_set_outputs(vfd);

	for (cursor = 0; cursor < 16; cursor++)
	{
		switch (segs)
		{
			case 14:
			{
				plot_box(bitmap, 1+(curwidth*cursor), 0, 11, 2, (vfd_get_outputs(vfd)[cursor] & 0x0001) ? color : col_off);//0
				plot_box(bitmap, 1+(curwidth*cursor), 27, 11, 2, (vfd_get_outputs(vfd)[cursor] & 0x0020) ? color : col_off);//5
				break;
			}
			case 16:
			{
				plot_box(bitmap, 1+(curwidth*cursor), 0, 5, 2, (vfd_get_outputs(vfd)[cursor] & 0x0001) ? color : col_off);//0
				plot_box(bitmap, 7+(curwidth*cursor), 0, 5, 2, (vfd_get_outputs(vfd)[cursor] & 0x0002) ? color : col_off);//1
				plot_box(bitmap, 7+(curwidth*cursor), 27, 5, 2, (vfd_get_outputs(vfd)[cursor] & 0x0010) ? color : col_off);//4
				plot_box(bitmap, 1+(curwidth*cursor), 27, 5, 2, (vfd_get_outputs(vfd)[cursor] & 0x0020) ? color : col_off);//5
				break;
			}
		}

		plot_box(bitmap, 12+(curwidth*cursor), 2, 2, 12, (vfd_get_outputs(vfd)[cursor] & 0x0004) ? color : col_off);//2
		plot_box(bitmap, 12+(curwidth*cursor), 16, 2, 12, (vfd_get_outputs(vfd)[cursor] & 0x0008) ? color : col_off);//3
		plot_box(bitmap, (curwidth*cursor), 16, 2, 12, (vfd_get_outputs(vfd)[cursor] & 0x0040) ? color : col_off);//6
		plot_box(bitmap, (curwidth*cursor), 2, 2, 12, (vfd_get_outputs(vfd)[cursor] & 0x0080) ? color : col_off);//7
		plot_box(bitmap, 7+(curwidth*cursor), 2, 2, 12, (vfd_get_outputs(vfd)[cursor] & 0x0100) ? color : col_off);//8

		plot_box(bitmap, 10+(curwidth*cursor), 9, 2, 2, (vfd_get_outputs(vfd)[cursor] & 0x0200) ? color : col_off);//9?
		plot_box(bitmap, 9+(curwidth*cursor), 10, 2, 4, (vfd_get_outputs(vfd)[cursor] & 0x0200) ? color : col_off);//9?
		plot_box(bitmap, 10+(curwidth*cursor), 5, 2, 4, (vfd_get_outputs(vfd)[cursor] & 0x0200) ? color : col_off);//9?

		plot_box(bitmap, 7+(curwidth*cursor), 14, 7, 2, (vfd_get_outputs(vfd)[cursor] & 0x0400) ? color : col_off);//10

		plot_box(bitmap, 10+(curwidth*cursor),21, 2, 2, (vfd_get_outputs(vfd)[cursor] & 0x0800) ? color : col_off);//11?
		plot_box(bitmap, 9+(curwidth*cursor), 17, 2, 4, (vfd_get_outputs(vfd)[cursor] & 0x0800) ? color : col_off);//11?
		plot_box(bitmap, 10+(curwidth*cursor), 22, 2, 4, (vfd_get_outputs(vfd)[cursor] & 0x0800) ? color : col_off);//11?

		plot_box(bitmap, 7+(curwidth*cursor), 16, 2, 11, (vfd_get_outputs(vfd)[cursor] & 0x1000) ? color : col_off);//12

		plot_box(bitmap, 3+(curwidth*cursor), 21, 2, 2, (vfd_get_outputs(vfd)[cursor] & 0x2000) ? color : col_off);//13?
		plot_box(bitmap, 4+(curwidth*cursor), 17, 2, 4, (vfd_get_outputs(vfd)[cursor] & 0x2000) ? color : col_off);//13? first step
		plot_box(bitmap, 3+(curwidth*cursor), 22, 2, 4, (vfd_get_outputs(vfd)[cursor] & 0x2000) ? color : col_off);//13? second step

		plot_box(bitmap, (curwidth*cursor), 14, 7, 2, (vfd_get_outputs(vfd)[cursor] & 0x4000) ? color : col_off);//14

		plot_box(bitmap, 3+(curwidth*cursor), 9, 2, 2, (vfd_get_outputs(vfd)[cursor] & 0x8000) ? color : col_off);//15?
		plot_box(bitmap, 4+(curwidth*cursor), 10, 2, 4, (vfd_get_outputs(vfd)[cursor] & 0x8000) ? color : col_off);//15? first step
		plot_box(bitmap, 3+(curwidth*cursor), 5, 2, 4, (vfd_get_outputs(vfd)[cursor] & 0x8000) ? color : col_off);//15? second step

		switch (vfds[vfd].type)
		{
			case VFDTYPE_BFMBD1:
			{
				plot_box(bitmap, 15+(curwidth*cursor), 22, 1, 3, (vfd_get_outputs(vfd)[cursor] & 0x20000) ? color : col_off);//18
				plot_box(bitmap, 16+(curwidth*cursor), 23, 1, 7, (vfd_get_outputs(vfd)[cursor] & 0x20000) ? color : col_off);//18
				plot_box(bitmap, 17+(curwidth*cursor), 27, 1, 6, (vfd_get_outputs(vfd)[cursor] & 0x20000) ? color : col_off);//18
				plot_box(bitmap, 18+(curwidth*cursor), 31, 1, 1, (vfd_get_outputs(vfd)[cursor] & 0x20000) ? color : col_off);//18
				break;
			}

			case VFDTYPE_MSC1937:
			{

				plot_box(bitmap, 15+(curwidth*cursor), 27, 1, 6, (vfd_get_outputs(vfd)[cursor] & 0x10000) ? color : col_off);//17
				plot_box(bitmap, 16+(curwidth*cursor), 27, 1, 3, (vfd_get_outputs(vfd)[cursor] & 0x10000) ? color : col_off);//17
				plot_box(bitmap, 15+(curwidth*cursor), 23, 2, 2, (vfd_get_outputs(vfd)[cursor] & 0x20000) ? color : col_off);//18

				break;
			}

			if (vfd_get_outputs(vfd)[cursor] & 0x40000)
				{
					//activate flashing (unimplemented, just toggle on and off)
				}
			else
				{
					//deactivate flashing (unimplemented)
				}
		}
	}
}
void draw_vfd(mame_bitmap *bitmap,int vfd,int segs,int col_on, int col_off )
{
	int cycle,color;
	if (vfds[vfd].type == VFDTYPE_BFMBD1)
	{
		if (segs != 14)
		{
			logerror("BD1 controller only supports 14 segments - defaulting to 14 seg mode \n");
			segs = 14;
		}

		color = col_on;
		plot_vfd(bitmap,vfd,segs,color,col_off );
	}
	if (vfds[vfd].type == VFDTYPE_MSC1937)
	{
		for (cycle = 0; cycle < 32; cycle++)
		{
			if ((vfds[vfd].brightness < cycle)||(vfds[vfd].brightness == cycle))
			{
				color = col_on;
				plot_vfd(bitmap,vfd,segs,color,col_off );
			}
			else
			{
				color = col_off;
				plot_vfd(bitmap,vfd,segs,color,col_off );
			}
		}
	}
}
//Helper functions
void draw_16seg(mame_bitmap *bitmap,int vfd, int col_on, int col_off )
{
	draw_vfd(bitmap,vfd,16,col_on,col_off );
}
void draw_14seg(mame_bitmap *bitmap,int vfd, int col_on, int col_off )
{
	draw_vfd(bitmap,vfd,14,col_on,col_off );
}
