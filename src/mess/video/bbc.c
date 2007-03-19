/******************************************************************************
    BBC Model B

    MESS Driver By:

	Gordon Jefferyes
	mess_bbc@gjeffery.dircon.co.uk

******************************************************************************/

#include "driver.h"
#include "includes/bbc.h"
#include "video/m6845.h"
#include "saa505x.h"
//#include "video/bbctext.h"


void BBC_draw_hi_res(void);
void BBC_draw_teletext(void);

static void (*draw_function)(void);

void BBC_draw_RGB_in(int offset,int data);

/************************************************************************
 * video_refresh flag is used in optimising the screen redrawing
 * it is set whenever a 6845 or VideoULA registers are change.
 * This will then cause a full screen refresh.
 * The vidmem array is used to optimise the screen redrawing
 * whenever a memory location is written to the same location is set in the vidmem array
 * if none of the video registers have been changed and a full redraw is not needed
 * the video display emulation will only redraw the video memory locations that have been changed.
 ************************************************************************/

static int video_refresh;
unsigned char vidmem[0x10000];

// this is the real location of the start of the BBC's ram in the emulation
// it can be changed if shadow ram is being used to point at the upper 32K of RAM
static unsigned char *BBC_Video_RAM;

// this is the real location of the start of the memory changed look up optimisation array
// it can be changed if shadow ram is being used to point at the upper 32K of the lookup table
static unsigned char *vidmem_RAM;

// this is the screen memory location of the next pixels to be drawn
static UINT16 *BBC_display;

static UINT16 *BBC_display_left;
static UINT16 *BBC_display_right;

// this is a more global variable to store the bitmap variable passed in in the bbc_vh_screenrefresh function
static mame_bitmap *BBC_bitmap;

// this is the X and Y screen location in emulation pixels of the next pixels to be drawn
static int y_screen_pos;

/************************************************************************
 * video memory lookup arrays.
 * this is a set of quick lookup arrays that stores the logic for the following:
 * the array to be used is selected by the output from bits 4 and 5 (C0 and C1) on IC32 74LS259
 * which is controlled by the system VIA.
 * C0 and C1 along with MA12 output from the 6845 drive 4 NAND gates in ICs 27,36 and 40
 * the outputs from these NAND gates (B1 to B4) along with MA8 to MA11 from the 6845 (A1 to B4) are added together
 * in IC39 74LS283 4 bit adder to form (S1 to S4) the logic is used to loop the screen memory for hardware scrolling.
 * when MA13 from the 6845 is low the latches IC8 and IC9 are enabled
 * they control the memory addressing for the Hi-Res modes.
 * when MA13 from the 6845 is high the latches IC10 and IC11 are enabled
 * they control the memory addressing for the Teletext mode.
 * IC 8 or IC10 drives the row select in the memory (the lower 7 bits in the memory address) and
 * IC 9 or IC11 drives the column select in the memory (the next 7 bits in the memory address) this
 * gives control of the bottom 14 bits of the memory, in a 32K model B 15 bits are needed to access
 * all the RAM, so S4 for the adder drives the CAS0 and CAS1 to access the top bit, in a 16K model A
 * the output of S4 is linked out to a 0v supply by link S25 to just access the 16K memory area.
 ************************************************************************/

static unsigned int video_ram_lookup0[0x4000];
static unsigned int video_ram_lookup1[0x4000];
static unsigned int video_ram_lookup2[0x4000];
static unsigned int video_ram_lookup3[0x4000];

static unsigned int *video_ram_lookup;

void set_video_memory_lookups(int ramsize)
{

	int ma; // output from IC2 6845 MA address

	int c0,c1; // output from IC32 74LS259 bits 4 and 5
	int ma12; // bit 12 of 6845 MA address

	int b1,b2,b3,b4; // 4 bit input B on IC39 74LS283 (4 bit adder)
	int a,b,s;

	unsigned int m;

	for(c1=0;c1<2;c1++)
	{
		for(c0=0;c0<2;c0++)
		{
			if ((c0==0) && (c1==0)) video_ram_lookup=video_ram_lookup0;
			if ((c0==1) && (c1==0)) video_ram_lookup=video_ram_lookup1;
			if ((c0==0) && (c1==1)) video_ram_lookup=video_ram_lookup2;
			if ((c0==1) && (c1==1)) video_ram_lookup=video_ram_lookup3;

			for(ma=0;ma<0x4000;ma++)
			{


				/* the 4 bit input port b on IC39 are produced by 4 NAND gates.
				these NAND gates take their
				inputs from c0 and c1 (from IC32) and ma12 (from the 6845) */

				/* get bit m12 from the 6845 */
				ma12=(ma>>12)&1;

				/* 3 input NAND part of IC 36 */
				b1=(~(c1&c0&ma12))&1;
				/* 2 input NAND part of IC40 (b3 is calculated before b2 and b4 because b3 feed back into b2 and b4) */
				b3=(~(c0&ma12))&1;
				/* 3 input NAND part of IC 36 */
				b2=(~(c1&b3&ma12))&1;
				/* 2 input NAND part of IC 27 */
				b4=(~(b3&ma12))&1;

				/* inputs port a to IC39 are MA8 to MA11 from the 6845 */
				a=(ma>>8)&0xf;
				/* inputs port b to IC39 are taken from the NAND gates b1 to b4 */
				b=(b1<<0)|(b2<<1)|(b3<<2)|(b4<<3);

				/* IC39 performs the 4 bit add with the carry input set high */
				s=(a+b+1)&0xf;

				/* if MA13 (TTXVDU) is low then IC8 and IC9 are used to calculate
				   the memory location required for the hi res video.
				   if MA13 is hight then IC10 and IC11 are used to calculate the memory location for the teletext chip
				   Note: the RA0,RA1,RA2 inputs to IC8 in high res modes will need to be added else where */
				if ((ma>>13)&1)
				{
					m=((ma&0x3ff)|0x3c00)|((s&0x8)<<11);
				} else {
					m=((ma&0xff)<<3)|(s<<11);
				}
				if (ramsize==16)
				{
					video_ram_lookup[ma]=m & 0x3fff;
				} else {
					video_ram_lookup[ma]=m;
				}

			}
		}
	}
}


/* called from 6522 system via */
void setscreenstart(int c0,int c1)
{
	if ((c0==0) && (c1==0)) video_ram_lookup=video_ram_lookup0;
	if ((c0==1) && (c1==0)) video_ram_lookup=video_ram_lookup1;
	if ((c0==0) && (c1==1)) video_ram_lookup=video_ram_lookup2;
	if ((c0==1) && (c1==1)) video_ram_lookup=video_ram_lookup3;

	// emulation refresh optimisation
	video_refresh=1;
}



/* this is a quick lookup array that puts bits 0,2,4,6 into bits 0,1,2,3
   this is used by the pallette lookup in the video ULA */
static unsigned char pixel_bits[256];

static void set_pixel_lookup(void)
{
	int i;
	for (i=0; i<256; i++)
	{
		pixel_bits[i] = (((i>>7)&1)<<3) | (((i>>5)&1)<<2) | (((i>>3)&1)<<1) | (((i>>1)&1)<<0);
	}
}

/************************************************************************
 * Outputs from the 6845
 ************************************************************************/

static int BBC_HSync=0;
static int BBC_VSync=0;
static int BBC_Character_Row=0;
static int BBC_DE=0;



/************************************************************************
 * Teletext Interface circuits
 ************************************************************************/

static int Teletext_Latch_Input_D7=0;
static int Teletext_Latch=0;


static struct saa505x_interface
BBCsaa5050= {
	BBC_draw_RGB_in,
};

void BBC_draw_teletext(void)
{

	//Teletext Latch bits 0 to 5 go to bits 0 to 5 on the Teletext chip
	//Teletext Latch bit 6 is only passed onto bits 6 on the Teletext chip if DE is true
	//Teletext Latch bit 7 goes to LOSE on the Teletext chip

    int meml;

	teletext_LOSE_w(0,(Teletext_Latch>>7)&1);

	teletext_F1();

	teletext_data_w(0,(Teletext_Latch&0x3f)|((Teletext_Latch&0x40)|(BBC_DE?0:0x40)));

	meml=crtc6845_memory_address_r(0);

	if (((meml>>13)&1)==0)
	{
		Teletext_Latch=0;
	} else {
		Teletext_Latch=(BBC_Video_RAM[video_ram_lookup[meml]]&0x7f)|Teletext_Latch_Input_D7;
	}

}
/************************************************************************
 * VideoULA
 ************************************************************************/


static int VideoULA_DE=0;          // internal videoULA Display Enabled set by 6845 DE and the scanlines<8
static int VideoULA_CR=7;		   // internal videoULA Cursor Enabled set by 6845 CR and then cleared after a number clock cycles
static int VideoULA_CR_counter=0;  // number of clock cycles left before the CR is disabled

static int videoULA_Reg;
static int videoULA_master_cursor_size;
static int videoULA_width_of_cursor;
static int videoULA_6845_clock_rate;
static int videoULA_characters_per_line;
static int videoULA_teletext_normal_select;
static int videoULA_flash_colour_select;

static int pixels_per_byte_set[8]={ 2,4,8,16,1,2,4,8 };
static int pixels_per_byte;

static int emulation_pixels_per_real_pixel_set[4]={ 8,4,2,1 };
static int emulation_pixels_per_real_pixel;

static int emulation_pixels_per_byte;

static int width_of_cursor_set[8]={ 0,0,1,2,1,0,2,4 };
static int emulation_cursor_size=1;
static int cursor_state=0;

static int videoULA_pallet0[16];// flashing colours A
static int videoULA_pallet1[16];// flashing colours B
static int *videoULA_pallet_lookup;// holds the pallet now being used.

// this is the pixel position of the start of a scanline
// -96 sets the screen display to the middle of emulated screen.
static int x_screen_offset=-96;

static int y_screen_offset=-8;



WRITE8_HANDLER ( videoULA_w )
{

	int tpal,tcol;


	// emulation refresh optimisation
	video_refresh=1;
	video_screen_update_partial(0, cpu_getscanline() - 1);

//	logerror("setting videoULA %s at %.4x size:%.4x\n",image_filename(image), addr, size);
	logerror("setting videoULA %.4x to:%.4x   at :%d \n",data,offset,cpu_getscanline() );


	switch (offset&0x01)
	{
	// Set the control register in the Video ULA
	case 0:
		videoULA_Reg=data;
		videoULA_master_cursor_size=    (videoULA_Reg>>7)&0x01;
		videoULA_width_of_cursor=       (videoULA_Reg>>5)&0x03;
		videoULA_6845_clock_rate=       (videoULA_Reg>>4)&0x01;
		videoULA_characters_per_line=   (videoULA_Reg>>2)&0x03;
		videoULA_teletext_normal_select=(videoULA_Reg>>1)&0x01;
		videoULA_flash_colour_select=    videoULA_Reg    &0x01;

		videoULA_pallet_lookup=videoULA_flash_colour_select?videoULA_pallet0:videoULA_pallet1;

		emulation_cursor_size=width_of_cursor_set[videoULA_width_of_cursor|(videoULA_master_cursor_size<<2)];

		if (videoULA_teletext_normal_select)
		{
			pixels_per_byte=6;
			emulation_pixels_per_byte=18;
			emulation_pixels_per_real_pixel=3;
			x_screen_offset=-154;
			y_screen_offset=0;
			draw_function=*BBC_draw_teletext;
		} else {
			// this is the number of BBC pixels held in each byte
			pixels_per_byte=pixels_per_byte_set[videoULA_characters_per_line|(videoULA_6845_clock_rate<<2)];

			// this is the number of emulation display pixels
			emulation_pixels_per_byte=videoULA_6845_clock_rate?8:16;
			emulation_pixels_per_real_pixel=emulation_pixels_per_real_pixel_set[videoULA_characters_per_line];
			x_screen_offset=-96;
			y_screen_offset=-11;
			draw_function=*BBC_draw_hi_res;
		}

		break;
	// Set a pallet register in the Video ULA
	case 1:
		tpal=(data>>4)&0x0f;
		tcol=data&0x0f;
		videoULA_pallet0[tpal]=tcol^7;
		videoULA_pallet1[tpal]=tcol<8?tcol^7:tcol;
		break;
	}

	// emulation refresh optimisation
	video_refresh=1;
}


// VideoULA Internal Cursor controls

static void set_cursor(void)
{
	cursor_state=VideoULA_CR?0:7;
}

static void BBC_Clock_CR(void)
{
	if (VideoULA_CR)
	{
		VideoULA_CR_counter-=1;
		if (VideoULA_CR_counter<=0) {
			VideoULA_CR=0;
			video_refresh=video_refresh&0xfb;
			set_cursor();
		}
	}
}



// This is the actual output of the Video ULA this fuction does all the output to the screen in the BBC emulator

static void BBC_ula_drawpixel(int col,int number_of_pixels)
{
	int pixel_count;
	int pixel_temp;
	if ((BBC_display>=BBC_display_left) && ((BBC_display+number_of_pixels)<BBC_display_right))
	{

		pixel_temp=Machine->pens[col^cursor_state];
		for(pixel_count=0;pixel_count<number_of_pixels;pixel_count++)
		{
			*(BBC_display++) = pixel_temp;
		}
	} else {
		BBC_display += number_of_pixels;
	}
}


// the Video ULA hi-res shift registers, pallette lookup and display enabled circuits

void BBC_draw_hi_res(void)
{
	int meml;
	unsigned char i=0;
	int sc1;

	if (VideoULA_DE)
	{

		// read the memory location for the next screen location.
		// the logic for the memory location address is very complicated so it
		// is stored in a number of look up arrays (and is calculated once at the start of the emulator).
		// this is actually does by the latch IC's not the Video ULA
		meml=video_ram_lookup[crtc6845_memory_address_r(0)]|(BBC_Character_Row&0x7);

		if (vidmem_RAM[meml] || video_refresh )
		{
			vidmem_RAM[meml]=0;
			i=BBC_Video_RAM[meml];

			for(sc1=0;sc1<pixels_per_byte;sc1++)
			{
				BBC_ula_drawpixel(videoULA_pallet_lookup[pixel_bits[i]],emulation_pixels_per_real_pixel);
				i=(i<<1)|1;
			}
		} else {
			BBC_display += emulation_pixels_per_byte;
		}
	} else {
		if (video_refresh)
		{
			// if the display is not enable, just draw a blank area.
			BBC_ula_drawpixel(0, emulation_pixels_per_byte);
		} else {
			BBC_display += emulation_pixels_per_byte;
		}
	}
}


// RGB input to the Video ULA from the Teletext IC
// Just pass on the output at the correct pixel size.
void BBC_draw_RGB_in(int offset,int data)
{
	BBC_ula_drawpixel(data,emulation_pixels_per_real_pixel);
}







/************************************************************************
 * BBC circuits controlled by 6845 Outputs
 ************************************************************************/

static void BBC_Set_VideoULA_DE(void)
{
	VideoULA_DE=(BBC_DE) && (!(BBC_Character_Row&8));
}

static void BBC_Set_Teletext_DE(void)
{
	Teletext_Latch_Input_D7=BBC_DE?0x80:0;
}

// called when the 6845 changes the character row
static void BBC_Set_Character_Row(int offset, int data)
{
	BBC_Character_Row=data;
	BBC_Set_VideoULA_DE();
}

// called when the 6845 changes the HSync
static void BBC_Set_HSync(int offset, int data)
{
	// catch the falling edge
	if((!data)&&(BBC_HSync))
	{
		y_screen_pos+=1;

		if ((y_screen_pos>=0) && (y_screen_pos<300))
		{
			BBC_display_left = BITMAP_ADDR16(BBC_bitmap, y_screen_pos, 0);
			BBC_display_right = BBC_display_left + 800;

		} else {
			BBC_display_left = BITMAP_ADDR16(BBC_bitmap, 0, 0);
			BBC_display_right = BBC_display_left;
		}

		BBC_display = BBC_display_left + x_screen_offset;

	}
	BBC_HSync=data;
}

// called when the 6845 changes the VSync
static void BBC_Set_VSync(int offset, int data)
{
	// catch the falling edge
	if ((!data)&&(BBC_VSync))
	{
		y_screen_pos=y_screen_offset;

		if ((y_screen_pos>=0) && (y_screen_pos<300))
		{
			BBC_display_left = BITMAP_ADDR16(BBC_bitmap, y_screen_pos, 0);
			BBC_display_right = BBC_display_left + 800;

		} else {
			BBC_display_left = BITMAP_ADDR16(BBC_bitmap, 0, 0);
			BBC_display_right = BBC_display_left;
		}

		BBC_display = BBC_display_left + x_screen_offset;

		teletext_DEW();
	}
	BBC_VSync=data;

}

// called when the 6845 changes the Display Enabled
static void BBC_Set_DE(int offset, int data)
{
	BBC_DE=data;
	BBC_Set_VideoULA_DE();
	BBC_Set_Teletext_DE();

}


// called when the 6845 changes the Cursor Enabled
static void BBC_Set_CRE(int offset, int data)
{
	if (data&2) {
		VideoULA_CR_counter=emulation_cursor_size;
		VideoULA_CR=1;
		// turn on the video refresh for the cursor area
		video_refresh=video_refresh|4;
		// set the pallet on
		if (data&1) set_cursor();
	}
}


static struct crtc6845_interface
BBC6845= {
	0,// Memory Address register
	BBC_Set_Character_Row,// Row Address register
	BBC_Set_HSync,// Horizontal status
	BBC_Set_VSync,// Vertical status
	BBC_Set_DE,// Display Enabled status
	0,// Cursor status
	BBC_Set_CRE, // Cursor status Emulation
};





/************************************************************************
 * memory interface to BBC's 6845
 ************************************************************************/

WRITE8_HANDLER ( BBC_6845_w )
{
	switch (offset&1)
	{
		case 0:
			crtc6845_address_w(0,data);
			break;
		case 1:
			crtc6845_register_w(0,data);
			break;
	}

	// emulation refresh optimisation
	video_refresh=1;
}

 READ8_HANDLER (BBC_6845_r)
{
	int retval=0;

	switch (offset&1)
	{
		case 0:
			break;
		case 1:
			retval=crtc6845_register_r(0);
			break;
	}
	return retval;
}





/************************************************************************
 * bbc_vh_screenrefresh
 * resfresh the BBC video screen
 ************************************************************************/

VIDEO_UPDATE( bbc )
{
	long c;
	int full_refresh;

	//logerror ("Box %d by %d \n",cliprect->min_y,cliprect->max_y);

	c = 0; // this is used to time out the screen redraw, in the case that the 6845 is in some way out state.
	full_refresh = 1;

	BBC_bitmap=bitmap;

	BBC_display_left=BITMAP_ADDR16(BBC_bitmap, 0, 0);
	BBC_display_right=BBC_display_left;
	BBC_display=BBC_display_left;

	// video_refresh is set if any of the 6845 or Video ULA registers are changed
	// this then forces a full screen redraw

	if (full_refresh)
	{
		video_refresh=video_refresh|2;
	}

	// loop until the end of the Vertical Sync pulse
	// or until a timeout (this catches the 6845 with silly register values that would not give a VSYNC signal)
	while((BBC_VSync)&&(c<60000))
	{
		// Clock the 6845
		crtc6845_clock();
		c++;
	}


	// loop until the Vertical Sync pulse goes high
	// or until a timeout (this catches the 6845 with silly register values that would not give a VSYNC signal)
	while((!BBC_VSync)&&(c<60000))
	{
		if ((y_screen_pos>=cliprect->min_y) && (y_screen_pos<=cliprect->max_y)) (draw_function)();

		// and check the cursor
		if (VideoULA_CR) BBC_Clock_CR();

		// Clock the 6845
		crtc6845_clock();
		c++;
	}

	// redrawn the screen so reset video_refresh
	video_refresh=0;

	return 0;
}

void bbc_frameclock(void)
{
	crtc6845_frameclock();
}

/**** BBC B+ Shadow Ram change ****/

void bbcbp_setvideoshadow(int vdusel)
{
	if (vdusel)
	{
		BBC_Video_RAM= memory_region(REGION_CPU1)+0x8000;
		vidmem_RAM=(vidmem)+0x8000;
	} else {
		BBC_Video_RAM= memory_region(REGION_CPU1);
		vidmem_RAM=vidmem;
	}
}

/************************************************************************
 * bbc_vh_start
 * Initialize the BBC video emulation
 ************************************************************************/

VIDEO_START( bbca )
{
	set_pixel_lookup();
	set_video_memory_lookups(16);
	crtc6845_config(&BBC6845);
	saa505x_config(&BBCsaa5050);

	BBC_Video_RAM= memory_region(REGION_CPU1);
	vidmem_RAM=vidmem;
	draw_function=*BBC_draw_hi_res;
	return 0;
}

VIDEO_START( bbcb )
{
	set_pixel_lookup();

	crtc6845_config(&BBC6845);
	saa505x_config(&BBCsaa5050);

	BBC_Video_RAM= memory_region(REGION_CPU1);
	vidmem_RAM=vidmem;
	draw_function=*BBC_draw_hi_res;
	return 0;
}

VIDEO_START( bbcbp )
{
	/* need to set up the lookups to work with the BBC B plus memory */
	set_pixel_lookup();

	set_video_memory_lookups(32);
	crtc6845_config(&BBC6845);
	saa505x_config(&BBCsaa5050);

	BBC_Video_RAM= memory_region(REGION_CPU1);
	vidmem_RAM=vidmem;
	draw_function=*BBC_draw_hi_res;
	return 0;
}

VIDEO_START( bbcm )
{
	/* need to set up the lookups to work with the BBC B plus memory */
	set_pixel_lookup();
	set_video_memory_lookups(32);
	crtc6845_config(&BBC6845);
	saa505x_config(&BBCsaa5050);

	BBC_Video_RAM= memory_region(REGION_CPU1);
	vidmem_RAM=vidmem;
	draw_function=*BBC_draw_hi_res;
	return 0;
}
