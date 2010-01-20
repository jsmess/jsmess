/*
    video/rmnimbus.c
    
    Research machines Nimbus.
    
    2009-11-14, P.Harvey-Smith.
    
    This is my best guess implementation of the operation of the Nimbus 
    video system.
    
    On the real machine, the Video chip has a block of 64K of memory which is
    completely seperate from the main 80186 memory. 
    
    The main CPU write to the video chip via a series of registers in the 
    0x0000 to 0x002F reigon, the video chip then manages all video memory 
    from there.
    
    As I cannot find a datasheet for the vide chip marked 	
    MB61H201 Fujitsu RML 12835 GCV, I have had to determine most of its
    operation by disassembling the Nimbus bios and by writing experemental
    code on the real machine.  
*/

#include "emu.h"
#include "video/border.h"
#include "memory.h"
#include "debug/debugcpu.h"
#include "debug/debugcon.h"
#include "includes/rmnimbus.h"
#include "mame.h"

#include "debug/debugcpu.h"
#include "debug/debugcon.h"

UINT8 video_mem[SCREEN_WIDTH_PIXELS][SCREEN_HEIGHT_LINES];

#define WIDTH_MASK      0x07
#define XOR_MASK        0x08
#define MASK_4080       0x10

// Offsets of nimbus video registers within register array

#define NO_VIDREGS      (0x30/2)
#define reg000          0x00
#define reg002          0x01
#define reg004          0x02
#define reg006          0x03
#define reg008          0x04
#define reg00A          0x05
#define reg00C          0x06
#define reg00E          0x07
#define reg010          0x08
#define reg012          0x09
#define reg014          0x0A
#define reg016          0x0B
#define reg018          0x0C
#define reg01A          0x0D
#define reg01C          0x0E
#define reg01E          0x0F
#define reg020          0x10
#define reg022          0x11 
#define reg024          0x12
#define reg026          0x13
#define reg028          0x14
#define reg02A          0x15
#define reg02C          0x16
#define reg02E          0x17

#define FG_COLOUR       (vidregs[reg024]&0x0F)
#define BG_COLOUR       ((vidregs[reg024]&0xF0)>>4)

#define IS_80COL        (vidregs[reg026]&MASK_4080)
#define IS_XOR          (vidregs[reg022]&XOR_MASK)


UINT16  vidregs[NO_VIDREGS];

UINT8   bpp;            // Bits / pixel
UINT8   ppb;            // Pixels / byte
UINT16  pixel_mask;
UINT8   border_colour;

int debug_on;

static void set_pixel(UINT16 x, UINT16 y, UINT8 colour);
static void write_pixel_line(UINT16 x, UINT16 y, UINT16    data, UINT8 width);
static void write_pixel_data(UINT16 x, UINT16 y, UINT16    data);
static void move_pixel_line(UINT16 x, UINT16 y, UINT16    data, UINT8 width);
static void write_reg_004(void);
static void write_reg_010(void);
static void write_reg_012(void);
static void write_reg_014(void);
static void write_reg_01A(void);
static void write_reg_01C(void);
static void write_reg_026(void);
static void change_palette(running_machine *machine, UINT8 first, UINT16 colours);

static void video_debug(running_machine *machine, int ref, int params, const char *param[]);
static void video_regdump(running_machine *machine, int ref, int params, const char *param[]);

/*
    I'm not sure which of thes return values on a real machine, so for the time being I'm going
    to return the values for all of them, it doesn't seem to hurt !
*/

READ16_HANDLER (nimbus_video_io_r)
{
    int     pc=cpu_get_pc(space->cpu);
    UINT16  result;
        
    switch (offset)
    {
        case    reg000  : result=vidregs[reg000]; break;
        case    reg002  : result=vidregs[reg002]; break;
        case    reg004  : result=vidregs[reg004]; break;
        case    reg006  : result=vidregs[reg006]; break;
        case    reg008  : result=vidregs[reg008]; break;
        case    reg00A  : result=vidregs[reg00A]; break;
        case    reg00C  : result=vidregs[reg00C]; break;
        case    reg00E  : result=vidregs[reg00E]; break;

        case    reg010  : result=vidregs[reg010]; break;
        case    reg012  : result=vidregs[reg012]; break;
        case    reg014  : result=vidregs[reg014]; break;
        case    reg016  : result=vidregs[reg016]; break;
        case    reg018  : result=vidregs[reg018]; break;
        case    reg01A  : result=vidregs[reg01A]; break;
        case    reg01C  : result=vidregs[reg01C]; break;
        case    reg01E  : result=vidregs[reg01E]; break;

        case    reg020  : result=vidregs[reg020]; break;
        case    reg022  : result=vidregs[reg022]; break;
        case    reg024  : result=vidregs[reg024]; break;
        case    reg026  : result=vidregs[reg026]; break;
        case    reg028  : result=vidregs[reg028]; break;
        case    reg02A  : result=vidregs[reg02A]; break;
        case    reg02C  : result=vidregs[reg02C]; break;
        case    reg02E  : result=vidregs[reg02E]; break;
        default         : result=0; break;
    }

    if(debug_on & DEBUG_TEXT)
        logerror("Nimbus video IOR at pc=%08X from %04X mask=%04X, data=%04X\n",pc,(offset*2),mem_mask,result);
    
    return result;
}

/*
    Write to the video registers, the default action is to write to the array of registers.
    If a register also needs some special action call the action function for that register.
    
    Incase anyone wonders about the DEBUG_DB statement, this allows me to log which registers 
    are being written to and then play them back at the real machine, this has helped greatly
    in figuring out what the video registers do.
    
*/

WRITE16_HANDLER (nimbus_video_io_w)
{
    int pc=cpu_get_pc(space->cpu);

    if(debug_on & DEBUG_TEXT)
        logerror("Nimbus video IOW at %08X write of %04X to %04X mask=%04X\n",pc,data,(offset*2),mem_mask);

    if(debug_on & DEBUG_DB)
        logerror("dw %05X,%05X\n",(offset*2),data);
    
    
    switch (offset)
    {
        case    reg000  : vidregs[reg000]=data; break;
        case    reg002  : vidregs[reg002]=data; break;
        case    reg004  : vidregs[reg004]=data; write_reg_004(); break;
        case    reg006  : vidregs[reg006]=data; break;
        case    reg008  : vidregs[reg008]=data; break;
        case    reg00A  : vidregs[reg00A]=data; break;
        case    reg00C  : vidregs[reg00C]=data; break;
        case    reg00E  : vidregs[reg00E]=data; break;

        case    reg010  : vidregs[reg010]=data; write_reg_010(); break;
        case    reg012  : vidregs[reg012]=data; write_reg_012(); break;
        case    reg014  : vidregs[reg014]=data; write_reg_014(); break;
        case    reg016  : vidregs[reg016]=data; break;
        case    reg018  : vidregs[reg018]=data; break;
        case    reg01A  : vidregs[reg01A]=data; write_reg_01A(); break;
        case    reg01C  : vidregs[reg01C]=data; write_reg_01C();break;
        case    reg01E  : vidregs[reg01E]=data; break;

        case    reg020  : vidregs[reg020]=data; break;
        case    reg022  : vidregs[reg022]=data; break;
        case    reg024  : vidregs[reg024]=data; break;
        case    reg026  : vidregs[reg026]=data; write_reg_026(); break;
        case    reg028  : vidregs[reg028]=data; change_palette(space->machine,0,data); break;
        case    reg02A  : vidregs[reg02A]=data; change_palette(space->machine,1,data); break;
        case    reg02C  : vidregs[reg02C]=data; change_palette(space->machine,2,data); break;
        case    reg02E  : vidregs[reg02E]=data; change_palette(space->machine,3,data); break;
        
        default         : break;
    }
}

static void set_pixel(UINT16 x, UINT16 y, UINT8 colour)
{
    if(debug_on & (DEBUG_TEXT | DEBUG_PIXEL))
        logerror("set_pixel(x=%04X, y=%04X, colour=%04X), IS_XOR=%02X\n",x,y,colour,IS_XOR);
    

    if((x<SCREEN_WIDTH_PIXELS) && (y<SCREEN_HEIGHT_LINES))
    {
        if(IS_XOR)
            video_mem[x][y]^=colour;
        else
            video_mem[x][y]=colour;
    }
}

static void write_pixel_line(UINT16 x, UINT16 y, UINT16    data, UINT8 width)
{
    UINT16  mask;
    UINT16  pixel_x;
    UINT16  colour;
    UINT8   shifts;

    if(debug_on & (DEBUG_TEXT | DEBUG_PIXEL))
        logerror("write_pixel_line(x=%04X, y=%04X, data=%04X, width=%02X, bpp=%02X, pixel_mask=%02X)\n",x,y,data,width,bpp,pixel_mask);
    
    shifts=width-bpp;    
    
    for(mask=pixel_mask, pixel_x=(x*(width/bpp)); mask>0; mask=(mask>>bpp), pixel_x++)
    {
        if(bpp==1)
            colour=(data & mask) ? FG_COLOUR : BG_COLOUR;
        else
            colour=(data & mask) >> shifts;
        
        //logerror("write_pixel_line: data=%04X, mask=%04X, shifts=%02X, bpp=%02X colour=%02X\n",data,mask,shifts,bpp,colour); 
        
        if(IS_80COL)
        {
            set_pixel(pixel_x,y,colour);
        }
        else
        {
            set_pixel((pixel_x*2),y,colour);
            set_pixel((pixel_x*2)+1,y,colour);
        }
        shifts-=bpp;
    }
}

static void move_pixel_line(UINT16 x, UINT16 y, UINT16    data, UINT8 width)
{
    UINT16  pixelno;
    UINT16  pixelx;
    
    if(debug_on & (DEBUG_TEXT | DEBUG_PIXEL))
       logerror("move_pixel_line(x=%04X, y=%04X, data=%04X, width=%02X)\n",x,y,data,width);
       
    for(pixelno=0;pixelno<width;pixelno++)
    {
        pixelx=(x*width)+pixelno;
        if(debug_on & (DEBUG_TEXT | DEBUG_PIXEL))
            logerror("pixelx=%04X\n",pixelx);
        video_mem[pixelx][vidregs[reg020]]=video_mem[pixelx][y];
    }
}


/* 
    The values in the bottom 3 bits of reg022 seem to determine the number of bits per pixel
    for following operations.
    
    The values that I have decoded so far are :
    
    000 1bpp, foreground and background colours taken from reg024
    001 2bpp, using the first 4 colours of the pallette
    010 
    011 
    100 4bpp, must be a 16 bit word, of which the upper byte is a mask anded with the lower byte
              containing the pixel data for two pixels.
    101
    110 4bpp, 16 bit word containing the pixel data for 4 pixels.
    111
    
    Bit 3 of reg022 is as follows :
    
    0   pixels are written from supplied colour data
    1   pixels are xor'ed onto the screen
*/

static void write_pixel_data(UINT16 x, UINT16 y, UINT16    data)
{    
    if(debug_on & (DEBUG_TEXT | DEBUG_PIXEL))
        logerror("write_pixel_data(x=%04X, y=%04X, data=%04X), reg022=%04X\n",x,y,data,vidregs[reg022]);
    
    if(IS_80COL)
    {
        switch (vidregs[reg022] & WIDTH_MASK)
        {
            case 0x00   : bpp=1; pixel_mask=0x8000;
                          write_pixel_line(x,y,data,16); 
                          break;
                      
            case 0x01   : bpp=1; pixel_mask=0x80;
                          write_pixel_line(x,y,data,8); 
                          break;
                      
            case 0x02   : bpp=1; pixel_mask=0x0080;
                          write_pixel_line(x,y,data,8); break;
        
            case 0x03   : bpp=1; 
                          set_pixel(x,y,FG_COLOUR); 
                          break; 
        
            case 0x04   : bpp=2; pixel_mask=0xC0;
                          write_pixel_line(x,y,((data & 0xFF) & ((data & 0xFF00)>>8)),8); 
                          break;
        
            case 0x05   : move_pixel_line(x,y,data,8);
                          break;
        
            case 0x06   : bpp=2; pixel_mask=0xC000;
                          write_pixel_line(x,y,data,16); 
                          break;
        
            case 0x07   : bpp=1; 
                          set_pixel(x,y,FG_COLOUR);
                          break;   
        }
    }
    else
    {
        switch (vidregs[reg022] & WIDTH_MASK)
        {
            case 0x00   : bpp=1; pixel_mask=0x0080;
                          write_pixel_line(x,y,data,8); 
                          break;
                      
            case 0x01   : bpp=2; pixel_mask=0xC0;
                          write_pixel_line(x,y,data,8); 
                          break;
                      
            case 0x02   : bpp=1; pixel_mask=0x0080;
                          write_pixel_line(x,y,data,8); break;
        
            case 0x03   : bpp=1; 
                          set_pixel(x,y,FG_COLOUR); 
                          break; 
        
            case 0x04   : bpp=4; pixel_mask=0xF0;
                          write_pixel_line(x,y,((data & 0xFF) & ((data & 0xFF00)>>8)),8); 
                          break;
        
            case 0x05   : move_pixel_line(x,y,data,16);
                          break;
        
            case 0x06   : bpp=4; pixel_mask=0xF000;
                          write_pixel_line(x,y,data,16); 
                          break;
        
            case 0x07   : bpp=1; 
                          set_pixel(x,y,FG_COLOUR);
                          break;   
        }
    }
}

static void write_reg_004(void)
{
    vidregs[reg002]=0;
    vidregs[reg00C]++;
}

static void write_reg_010(void)
{
    write_pixel_data(vidregs[reg002],vidregs[reg00C],vidregs[reg010]);    
}

static void write_reg_012(void)
{
    // I dunno if this is actually what is happening as the regs seem to be write only....
    // doing this however does seem to make some programs (worms from the welcom disk)
    // work correctly.
    if(IS_80COL)
        vidregs[reg002]=vidregs[reg012];
    else
        vidregs[reg002]=vidregs[reg012]/2; 

    write_pixel_data(vidregs[reg012],vidregs[reg00C],FG_COLOUR);
}

static void write_reg_014(void)
{  
    write_pixel_data(vidregs[reg002],vidregs[reg00C]++,vidregs[reg014]);
}

static void write_reg_01A(void)
{
    write_pixel_data(++vidregs[reg002],vidregs[reg00C],vidregs[reg01A]);  
}

static void write_reg_01C()
{
    // I dunno if this is actually what is happening as the regs seem to be write only....
    // doing this however does seem to make some programs (welcome from the welcom disk, 
    // and others using the standard RM box menus) work correctly.
    
    if(IS_80COL)
        vidregs[reg00C]=vidregs[reg01C];
    else
        vidregs[reg00C]=vidregs[reg01C]/2; 

    write_pixel_data(vidregs[reg002],vidregs[reg01C],FG_COLOUR);
}

/*
    bits 0..3 of reg026 contain the border colour.
    bit 5 contains the 40/80 column (320/640 pixel) flag.
*/

static void write_reg_026(void)
{
    border_colour=vidregs[reg026] & 0x0F;

    if(debug_on & DEBUG_TEXT)
        logerror("reg 026 write, border_colour=%02X\n",border_colour);
}

static void change_palette(running_machine *machine, UINT8 first, UINT16 colours)
{
    UINT8   colourno;
    UINT16  mask;
    UINT8   shifts;
    UINT8   paletteidx;
    
    shifts=12;
    mask=0xF000;
    for(colourno=first; colourno<SCREEN_NO_COLOURS; colourno+=4)
    {
        paletteidx=(colours & mask) >> shifts;
        palette_set_color_rgb(machine, colourno, nimbus_palette[paletteidx][RED], nimbus_palette[paletteidx][GREEN], nimbus_palette[paletteidx][BLUE]);
        mask=mask>>4;
        shifts-=4;
    }
}

static void video_debug(running_machine *machine, int ref, int params, const char *param[])
{
    if(params>0)
    {   
        sscanf(param[0],"%d",&debug_on);        
    }
    else
    {
        debug_console_printf(machine,"Error usage : nimbus_vid_debug <debuglevel>\n");
        debug_console_printf(machine,"Current debuglevel=%02X\n",debug_on);
    }
}

static void video_regdump(running_machine *machine, int ref, int params, const char *param[])
{
    int regno;
    
    for(regno=0;regno<0x08;regno++)
        debug_console_printf(machine,"reg%03X=%04X reg%03X=%04X reg%03X=%04X\n",
                regno*2,vidregs[regno],
                (regno+0x08)*2,vidregs[regno+0x08],
                (regno+0x10)*2,vidregs[regno+0x10]);
}

VIDEO_START( nimbus )
{
    debug_on=0;
    
    logerror("VIDEO_START\n");
  
   	if (machine->debug_flags & DEBUG_FLAG_ENABLED)
	{
        debug_console_register_command(machine, "nimbus_vid_debug", CMDFLAG_NONE, 0, 0, 1, video_debug);
        debug_console_register_command(machine, "nimbus_vid_regdump", CMDFLAG_NONE, 0, 0, 1, video_regdump);
    }
}

VIDEO_RESET( nimbus )
{
    // When we reset clear the video registers and video memory.
    memset(&vidregs,0x00,sizeof(vidregs));
    memset(&video_mem,0,sizeof(video_mem)); 

    bpp=4;          // bits per pixel
    logerror("Video reset\n");
}

VIDEO_EOF( nimbus )
{

//    logerror("VIDEO_EOF( nimbus )\n");
}

VIDEO_UPDATE( nimbus )
{
    int     XCoord;
    int     YCoord = video_screen_get_vpos(screen);
     
    for(XCoord=0;XCoord<SCREEN_WIDTH_PIXELS;XCoord++)
    {       
        *BITMAP_ADDR16(bitmap, YCoord, XCoord)=video_mem[XCoord][YCoord];
    }

    vidregs[reg028]++;
    if((vidregs[reg028] & 0x000F)>0x0A)
        vidregs[reg028]&=0xFFF0;

    return 0;
}

