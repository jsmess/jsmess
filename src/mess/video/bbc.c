/******************************************************************************
    BBC Model B

    MESS Driver By:

    Gordon Jefferyes
    mess_bbc@gjeffery.dircon.co.uk

******************************************************************************/

#include "emu.h"
#include "includes/bbc.h"
#include "video/m6845.h"
#include "saa505x.h"


static void BBC_draw_hi_res(running_machine &machine);
static void BBC_draw_teletext(running_machine &machine);

/************************************************************************
 * video_refresh flag is used in optimising the screen redrawing
 * it is set whenever a 6845 or VideoULA registers are change.
 * This will then cause a full screen refresh.
 * The vidmem array is used to optimise the screen redrawing
 * whenever a memory location is written to the same location is set in the vidmem array
 * if none of the video registers have been changed and a full redraw is not needed
 * the video display emulation will only redraw the video memory locations that have been changed.
 ************************************************************************/


// this is the real location of the start of the BBC's ram in the emulation
// it can be changed if shadow ram is being used to point at the upper 32K of RAM

// this is the real location of the start of the memory changed look up optimisation array
// it can be changed if shadow ram is being used to point at the upper 32K of the lookup table

// this is the screen memory location of the next pixels to be drawn


// this is a more global variable to store the bitmap variable passed in in the bbc_vh_screenrefresh function

// this is the X and Y screen location in emulation pixels of the next pixels to be drawn

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



void bbc_set_video_memory_lookups(running_machine &machine, int ramsize)
{
	bbc_state *state = machine.driver_data<bbc_state>();

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
			if ((c0==0) && (c1==0)) state->m_video_ram_lookup=state->m_video_ram_lookup0;
			if ((c0==1) && (c1==0)) state->m_video_ram_lookup=state->m_video_ram_lookup1;
			if ((c0==0) && (c1==1)) state->m_video_ram_lookup=state->m_video_ram_lookup2;
			if ((c0==1) && (c1==1)) state->m_video_ram_lookup=state->m_video_ram_lookup3;

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
					state->m_video_ram_lookup[ma]=m & 0x3fff;
				} else {
					state->m_video_ram_lookup[ma]=m;
				}

			}
		}
	}
}


/* called from 6522 system via */
void bbc_setscreenstart(running_machine &machine, int c0, int c1)
{
	bbc_state *state = machine.driver_data<bbc_state>();
	if ((c0==0) && (c1==0)) state->m_video_ram_lookup=state->m_video_ram_lookup0;
	if ((c0==1) && (c1==0)) state->m_video_ram_lookup=state->m_video_ram_lookup1;
	if ((c0==0) && (c1==1)) state->m_video_ram_lookup=state->m_video_ram_lookup2;
	if ((c0==1) && (c1==1)) state->m_video_ram_lookup=state->m_video_ram_lookup3;

	// emulation refresh optimisation
	state->m_video_refresh=1;
}



/* this is a quick lookup array that puts bits 0,2,4,6 into bits 0,1,2,3
   this is used by the pallette lookup in the video ULA */

static void set_pixel_lookup(bbc_state *state)
{
	int i;
	for (i=0; i<256; i++)
	{
		state->m_pixel_bits[i] = (((i>>7)&1)<<3) | (((i>>5)&1)<<2) | (((i>>3)&1)<<1) | (((i>>1)&1)<<0);
	}
}

/************************************************************************
 * Outputs from the 6845
 ************************************************************************/




/************************************************************************
 * Teletext Interface circuits
 ************************************************************************/



static void BBC_draw_teletext(running_machine &machine)
{
	bbc_state *state = machine.driver_data<bbc_state>();

	//Teletext Latch bits 0 to 5 go to bits 0 to 5 on the Teletext chip
	//Teletext Latch bit 6 is only passed onto bits 6 on the Teletext chip if DE is true
	//Teletext Latch bit 7 goes to LOSE on the Teletext chip

	int meml;

	teletext_LOSE_w(state->m_saa505x, 0, (state->m_Teletext_Latch>>7)&1);

	teletext_F1(state->m_saa505x);

	teletext_data_w(state->m_saa505x, 0, (state->m_Teletext_Latch&0x3f)|((state->m_Teletext_Latch&0x40)|(state->m_BBC_DE?0:0x40)));

	meml=m6845_memory_address_r(0);

	if (((meml>>13)&1)==0)
	{
		state->m_Teletext_Latch=0;
	} else {
		state->m_Teletext_Latch=(state->m_BBC_Video_RAM[state->m_video_ram_lookup[meml]]&0x7f)|state->m_Teletext_Latch_Input_D7;
	}

}
/************************************************************************
 * VideoULA
 ************************************************************************/




static const int pixels_per_byte_set[8]={ 2,4,8,16,1,2,4,8 };

static const int emulation_pixels_per_real_pixel_set[4]={ 8,4,2,1 };


static const int width_of_cursor_set[8]={ 0,0,1,2,1,0,2,4 };


// this is the pixel position of the start of a scanline
// -96 sets the screen display to the middle of emulated screen.




WRITE8_HANDLER ( bbc_videoULA_w )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();

	int tpal,tcol,vpos;


	// emulation refresh optimisation
	state->m_video_refresh=1;

	// Make sure vpos is never <0 2008-10-11 PHS.
	vpos=space->machine().primary_screen->vpos();
	if(vpos==0)
	  space->machine().primary_screen->update_partial(vpos);
	else
	  space->machine().primary_screen->update_partial(vpos -1 );

//  logerror("setting videoULA %s at %.4x size:%.4x\n",image_filename(image), addr, size);
	logerror("setting videoULA %.4x to:%.4x   at :%d \n",data,offset,space->machine().primary_screen->vpos() );


	switch (offset&0x01)
	{
	// Set the control register in the Video ULA
	case 0:
		state->m_videoULA_Reg=data;
		state->m_videoULA_master_cursor_size=    (state->m_videoULA_Reg>>7)&0x01;
		state->m_videoULA_width_of_cursor=       (state->m_videoULA_Reg>>5)&0x03;
		state->m_videoULA_6845_clock_rate=       (state->m_videoULA_Reg>>4)&0x01;
		state->m_videoULA_characters_per_line=   (state->m_videoULA_Reg>>2)&0x03;
		state->m_videoULA_teletext_normal_select=(state->m_videoULA_Reg>>1)&0x01;
		state->m_videoULA_flash_colour_select=    state->m_videoULA_Reg    &0x01;

		state->m_videoULA_pallet_lookup=state->m_videoULA_flash_colour_select?state->m_videoULA_pallet0:state->m_videoULA_pallet1;

		state->m_emulation_cursor_size=width_of_cursor_set[state->m_videoULA_width_of_cursor|(state->m_videoULA_master_cursor_size<<2)];

		if (state->m_videoULA_teletext_normal_select)
		{
			state->m_pixels_per_byte=6;
			state->m_emulation_pixels_per_byte=18;
			state->m_emulation_pixels_per_real_pixel=3;
			state->m_x_screen_offset=-154;
			state->m_y_screen_offset=0;
			state->m_draw_function=*BBC_draw_teletext;
		} else {
			// this is the number of BBC pixels held in each byte
			state->m_pixels_per_byte=pixels_per_byte_set[state->m_videoULA_characters_per_line|(state->m_videoULA_6845_clock_rate<<2)];

			// this is the number of emulation display pixels
			state->m_emulation_pixels_per_byte=state->m_videoULA_6845_clock_rate?8:16;
			state->m_emulation_pixels_per_real_pixel=emulation_pixels_per_real_pixel_set[state->m_videoULA_characters_per_line];
			state->m_x_screen_offset=-96;
			state->m_y_screen_offset=-11;
			state->m_draw_function=*BBC_draw_hi_res;
		}

		break;
	// Set a pallet register in the Video ULA
	case 1:
		tpal=(data>>4)&0x0f;
		tcol=data&0x0f;
		state->m_videoULA_pallet0[tpal]=tcol^7;
		state->m_videoULA_pallet1[tpal]=tcol<8?tcol^7:tcol;
		break;
	}

	// emulation refresh optimisation
	state->m_video_refresh=1;
}


// VideoULA Internal Cursor controls

static void set_cursor(bbc_state *state)
{
	state->m_cursor_state=state->m_VideoULA_CR?0:7;
}

static void BBC_Clock_CR(bbc_state *state)
{
	if (state->m_VideoULA_CR)
	{
		state->m_VideoULA_CR_counter-=1;
		if (state->m_VideoULA_CR_counter<=0) {
			state->m_VideoULA_CR=0;
			state->m_video_refresh=state->m_video_refresh&0xfb;
			set_cursor(state);
		}
	}
}



// This is the actual output of the Video ULA this fuction does all the output to the screen in the BBC emulator

static void BBC_ula_drawpixel(bbc_state *state, int col, int number_of_pixels)
{
	int pixel_count;
	int pixel_temp;
	if ((state->m_BBC_display>=state->m_BBC_display_left) && ((state->m_BBC_display+number_of_pixels)<state->m_BBC_display_right))
	{

		pixel_temp=col^state->m_cursor_state;
		for(pixel_count=0;pixel_count<number_of_pixels;pixel_count++)
		{
			*(state->m_BBC_display++) = pixel_temp;
		}
	} else {
		state->m_BBC_display += number_of_pixels;
	}
}


// the Video ULA hi-res shift registers, pallette lookup and display enabled circuits

static void BBC_draw_hi_res(running_machine &machine)
{
	bbc_state *state = machine.driver_data<bbc_state>();
	int meml;
	unsigned char i=0;
	int sc1;

	if (state->m_VideoULA_DE)
	{

		// read the memory location for the next screen location.
		// the logic for the memory location address is very complicated so it
		// is stored in a number of look up arrays (and is calculated once at the start of the emulator).
		// this is actually does by the latch IC's not the Video ULA
		meml=state->m_video_ram_lookup[m6845_memory_address_r(0)]|(state->m_BBC_Character_Row&0x7);

		if (state->m_vidmem_RAM[meml] || state->m_video_refresh )
		{
			state->m_vidmem_RAM[meml]=0;
			i=state->m_BBC_Video_RAM[meml];

			for(sc1=0;sc1<state->m_pixels_per_byte;sc1++)
			{
				BBC_ula_drawpixel(state, state->m_videoULA_pallet_lookup[state->m_pixel_bits[i]], state->m_emulation_pixels_per_real_pixel);
				i=(i<<1)|1;
			}
		} else {
			state->m_BBC_display += state->m_emulation_pixels_per_byte;
		}
	} else {
		if (state->m_video_refresh)
		{
			// if the display is not enable, just draw a blank area.
			BBC_ula_drawpixel(state, 0, state->m_emulation_pixels_per_byte);
		} else {
			state->m_BBC_display += state->m_emulation_pixels_per_byte;
		}
	}
}


// RGB input to the Video ULA from the Teletext IC
// Just pass on the output at the correct pixel size.
void bbc_draw_RGB_in(device_t *device, int offset,int data)
{
	bbc_state *state = device->machine().driver_data<bbc_state>();
	BBC_ula_drawpixel(state, data, state->m_emulation_pixels_per_real_pixel);
}







/************************************************************************
 * BBC circuits controlled by 6845 Outputs
 ************************************************************************/

static void BBC_Set_VideoULA_DE(bbc_state *state)
{
	state->m_VideoULA_DE=(state->m_BBC_DE) && (!(state->m_BBC_Character_Row&8));
}

static void BBC_Set_Teletext_DE(bbc_state *state)
{
	state->m_Teletext_Latch_Input_D7=state->m_BBC_DE?0x80:0;
}

// called when the 6845 changes the character row
static void BBC_Set_Character_Row(running_machine &machine, int offset, int data)
{
	bbc_state *state = machine.driver_data<bbc_state>();
	state->m_BBC_Character_Row=data;
	BBC_Set_VideoULA_DE(state);
}

// called when the 6845 changes the HSync
static void BBC_Set_HSync(running_machine &machine, int offset, int data)
{
	bbc_state *state = machine.driver_data<bbc_state>();
	// catch the falling edge
	if((!data)&&(state->m_BBC_HSync))
	{
		state->m_y_screen_pos+=1;

		if ((state->m_y_screen_pos>=0) && (state->m_y_screen_pos<300))
		{
			state->m_BBC_display_left = BITMAP_ADDR16(state->m_BBC_bitmap, state->m_y_screen_pos, 0);
			state->m_BBC_display_right = state->m_BBC_display_left + 800;

		} else {
			state->m_BBC_display_left = BITMAP_ADDR16(state->m_BBC_bitmap, 0, 0);
			state->m_BBC_display_right = state->m_BBC_display_left;
		}

		state->m_BBC_display = state->m_BBC_display_left + state->m_x_screen_offset;

	}
	state->m_BBC_HSync=data;
}

// called when the 6845 changes the VSync
static void BBC_Set_VSync(running_machine &machine, int offset, int data)
{
	bbc_state *state = machine.driver_data<bbc_state>();
	// catch the falling edge
	if ((!data)&&(state->m_BBC_VSync))
	{
		state->m_y_screen_pos=state->m_y_screen_offset;

		if ((state->m_y_screen_pos>=0) && (state->m_y_screen_pos<300))
		{
			state->m_BBC_display_left = BITMAP_ADDR16(state->m_BBC_bitmap, state->m_y_screen_pos, 0);
			state->m_BBC_display_right = state->m_BBC_display_left + 800;

		} else {
			state->m_BBC_display_left = BITMAP_ADDR16(state->m_BBC_bitmap, 0, 0);
			state->m_BBC_display_right = state->m_BBC_display_left;
		}

		state->m_BBC_display = state->m_BBC_display_left + state->m_x_screen_offset;

		teletext_DEW(state->m_saa505x);
	}
	state->m_BBC_VSync=data;

}

// called when the 6845 changes the Display Enabled
static void BBC_Set_DE(running_machine &machine, int offset, int data)
{
	bbc_state *state = machine.driver_data<bbc_state>();
	state->m_BBC_DE=data;
	BBC_Set_VideoULA_DE(state);
	BBC_Set_Teletext_DE(state);

}


// called when the 6845 changes the Cursor Enabled
static void BBC_Set_CRE(running_machine &machine, int offset, int data)
{
	bbc_state *state = machine.driver_data<bbc_state>();
	if (data&2) {
		state->m_VideoULA_CR_counter=state->m_emulation_cursor_size;
		state->m_VideoULA_CR=1;
		// turn on the video refresh for the cursor area
		state->m_video_refresh=state->m_video_refresh|4;
		// set the pallet on
		if (data&1) set_cursor(state);
	}
}


static const struct m6845_interface BBC6845 =
{
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

WRITE8_HANDLER ( bbc_6845_w )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	switch (offset&1)
	{
		case 0:
			m6845_address_w(0,data);
			break;
		case 1:
			m6845_register_w(0,data);
			break;
	}

	// emulation refresh optimisation
	state->m_video_refresh=1;
}

 READ8_HANDLER (bbc_6845_r)
{
	int retval=0;

	switch (offset&1)
	{
		case 0:
			break;
		case 1:
			retval=m6845_register_r(0);
			break;
	}
	return retval;
}





/************************************************************************
 * bbc_vh_screenrefresh
 * resfresh the BBC video screen
 ************************************************************************/

SCREEN_UPDATE( bbc )
{
	bbc_state *state = screen->machine().driver_data<bbc_state>();
	long c;
	int full_refresh;

	//logerror ("Box %d by %d \n",cliprect->min_y,cliprect->max_y);

	c = 0; // this is used to time out the screen redraw, in the case that the 6845 is in some way out state.
	full_refresh = 1;

	state->m_BBC_bitmap=bitmap;

	state->m_BBC_display_left=BITMAP_ADDR16(state->m_BBC_bitmap, 0, 0);
	state->m_BBC_display_right=state->m_BBC_display_left;
	state->m_BBC_display=state->m_BBC_display_left;

	// state->m_video_refresh is set if any of the 6845 or Video ULA registers are changed
	// this then forces a full screen redraw

	if (full_refresh)
	{
		state->m_video_refresh=state->m_video_refresh|2;
	}

	// loop until the end of the Vertical Sync pulse
	// or until a timeout (this catches the 6845 with silly register values that would not give a VSYNC signal)
	while((state->m_BBC_VSync)&&(c<60000))
	{
		// Clock the 6845
		m6845_clock(screen->machine());
		c++;
	}


	// loop until the Vertical Sync pulse goes high
	// or until a timeout (this catches the 6845 with silly register values that would not give a VSYNC signal)
	while((!state->m_BBC_VSync)&&(c<60000))
	{
		if ((state->m_y_screen_pos>=cliprect->min_y) && (state->m_y_screen_pos<=cliprect->max_y)) (state->m_draw_function)(screen->machine());

		// and check the cursor
		if (state->m_VideoULA_CR) BBC_Clock_CR(state);

		// Clock the 6845
		m6845_clock(screen->machine());
		c++;
	}

	// redrawn the screen so reset state->m_video_refresh
	state->m_video_refresh=0;

	return 0;
}

void bbc_frameclock(running_machine &machine)
{
	m6845_frameclock();
}

/**** BBC B+ Shadow Ram change ****/

void bbcbp_setvideoshadow(running_machine &machine, int vdusel)
{
	bbc_state *state = machine.driver_data<bbc_state>();
	if (vdusel)
	{
		state->m_BBC_Video_RAM= machine.region("maincpu")->base()+0x8000;
		state->m_vidmem_RAM=(state->m_vidmem)+0x8000;
	} else {
		state->m_BBC_Video_RAM= machine.region("maincpu")->base();
		state->m_vidmem_RAM=state->m_vidmem;
	}
}

/************************************************************************
 * bbc_vh_start
 * Initialize the BBC video emulation
 ************************************************************************/

static void common_init(running_machine &machine)
{
	bbc_state *state = machine.driver_data<bbc_state>();
	state->m_emulation_cursor_size = 1;
	state->m_x_screen_offset = -96;
	state->m_y_screen_offset = -8;

	state->m_VideoULA_DE = 0;
	state->m_VideoULA_CR = 7;
	state->m_VideoULA_CR_counter = 0;

	set_pixel_lookup(state);
	m6845_config(&BBC6845);
	state->m_saa505x = machine.device("saa505x");

	state->m_BBC_Video_RAM = machine.region("maincpu")->base();
	state->m_vidmem_RAM = state->m_vidmem;
	state->m_draw_function = *BBC_draw_hi_res;
}

VIDEO_START( bbca )
{
	common_init(machine);
	bbc_set_video_memory_lookups(machine, 16);
}

VIDEO_START( bbcb )
{
	common_init(machine);
}

VIDEO_START( bbcbp )
{
	common_init(machine);
	bbc_set_video_memory_lookups(machine, 32);
}

VIDEO_START( bbcm )
{
	common_init(machine);
	bbc_set_video_memory_lookups(machine, 32);
}
