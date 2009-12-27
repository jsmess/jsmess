/*
    video/rmnimbus.c
    
    Research machines Nimbus.
    
    2009-11-14, P.Harvey-Smith.
    
*/

#include "driver.h"
#include "video/border.h"
#include "memory.h"
#include "debug/debugcpu.h"
#include "debug/debugcon.h"
#include "includes/rmnimbus.h"
#include "mame.h"

#include "debug/debugcpu.h"
#include "debug/debugcon.h"

UINT8 *video_mem;

#define BYTES_PER_LINE  160
#define MAX_PIXELX_2BPP 639
#define MAX_PIXELX_4BPP 319

#define WIDTH_MASK      0x03
#define XOR_MASK        0x08

UINT16  reg002_x;
UINT16  reg004;
UINT16  reg010;
UINT16  reg00C_y;
UINT16  reg022; 
UINT16  reg024;
UINT16  reg026;
UINT16  reg028;

UINT8   bpp;            // Bits / pixel
UINT8   ppb;            // Pixels / byte
UINT8   border_colour;

int debug_on;

static void set_pixel(UINT16 x, UINT16 y, UINT8 on_off);
static void write_pixel_line(UINT16 x, UINT16 y, UINT16    data, UINT16    mem_mask, UINT8 width);
static void write_pixel_data(UINT16 x, UINT16 y, UINT16    data, UINT16    mem_mask);
static void write_reg_004(UINT16    data, UINT16    mem_mask);
static void write_reg_010(UINT16    data, UINT16    mem_mask);
static void write_reg_014(UINT16    data, UINT16    mem_mask);
static void write_reg_01A(UINT16    data, UINT16    mem_mask);
static void write_reg_026(UINT16    data, UINT16    mem_mask);

static void video_debug(running_machine *machine, int ref, int params, const char *param[]);

READ16_HANDLER (nimbus_video_io_r)
{
    int     pc=cpu_get_pc(space->cpu);
    UINT16  result;
        
    switch (offset*2)
    {
        case    0x002   : result=reg002_x; break;
        case    0x004   : result=reg004; break;
        case    0x00C   : result=reg00C_y; break;
        case    0x010   : result=reg010; break;
        case    0x022   : result=reg022; break;
        case    0x024   : result=reg024; break;
        case    0x026   : result=reg024; break;
        case    0x028   : result=reg028; break;
        default         : result=0; break;
    }

    if(debug_on & DEBUG_TEXT)
        logerror("Nimbus video IOR at pc=%08X from %04X mask=%04X, data=%04X\n",pc,(offset*2),mem_mask,result);
    
    return result;
}

WRITE16_HANDLER (nimbus_video_io_w)
{
    int pc=cpu_get_pc(space->cpu);

    if(debug_on & DEBUG_TEXT)
        logerror("Nimbus video IOW at %08X write of %04X to %04X mask=%04X\n",pc,data,(offset*2),mem_mask);

    if(debug_on & DEBUG_DB)
        logerror("dw %05X,%05X\n",(offset*2),data);
    
    
    switch (offset*2)
    {
        case    0x002   : COMBINE_DATA(&reg002_x); break;
        case    0x004   : write_reg_004(data,mem_mask); break;
        case    0x00C   : COMBINE_DATA(&reg00C_y); break;
        case    0x010   : write_reg_010(data,mem_mask); break;
        case    0x014   : write_reg_014(data,mem_mask); break;
        case    0x01A   : write_reg_01A(data,mem_mask); break;
        case    0x022   : COMBINE_DATA(&reg022); break;
        case    0x024   : COMBINE_DATA(&reg024); break;
        case    0x026   : write_reg_026(data,mem_mask); break;
        case    0x028   : COMBINE_DATA(&reg028); break;
        default         : break;
    }
}

static void set_pixel(UINT16 x, UINT16 y, UINT8 on_off )
{
    offs_t  vaddr;
    UINT8   rshifts;
    UINT8   lshifts;
    UINT8   last;
    UINT8   mask;
    UINT8   pixel_offset;
    UINT8   colour;
    
    vaddr=(BYTES_PER_LINE*y)+(x / ppb);
    pixel_offset=(x % ppb);
    last=video_mem[vaddr];
    colour=on_off ? (reg024&0x0F) : ((reg024&0xF0)>>4);
    
    if(bpp==2)
    {
        colour&=0x03;
        rshifts=(pixel_offset%4)*2;
        mask=~(0xC0>>rshifts);
        lshifts=6-rshifts;
    }
    else
    {
        colour&=0x0F;
        rshifts=(pixel_offset%2)*4;
        mask=~(0xF0>>rshifts);
        lshifts=4-rshifts;        
    }
    
    if(reg022 & XOR_MASK)
        video_mem[vaddr]=(video_mem[vaddr] & mask) ^ (colour<<(lshifts)); 
    else
        video_mem[vaddr]=(video_mem[vaddr] & mask) | (colour<<(lshifts)); 

    if(debug_on & DEBUG_TEXT)
        logerror("set_pixel(X=%04X, y=%04x, pixel_offset=%02X, on_off=%02X), vaddr=%04X, mask=%02X, last=%02X, current=%02X\n",x,y,pixel_offset,on_off,vaddr,mask,last,video_mem[vaddr]);
}


static void write_pixel_line(UINT16 x, UINT16 y, UINT16    data, UINT16    mem_mask, UINT8 width)
{
    UINT8   bit;
    UINT16  pixel_x;
    UINT8   shifts;
    
    shifts=(width==4) ? 2 : 1;    
    
    for(bit=0x80, pixel_x=(x*width); bit>0; bit=(bit>>shifts), pixel_x++)
    {
        set_pixel(pixel_x,y,(data&bit));
        if(width==16)
            set_pixel(pixel_x+1,y,(data&bit));
    }
}

static void write_pixel_data(UINT16 x, UINT16 y, UINT16    data, UINT16    mem_mask)
{    
    switch (reg022 & WIDTH_MASK)
    {
        case 0x00   : write_pixel_line(x,y,data,mem_mask,16); break;
        case 0x01   : write_pixel_line(x,y,data,mem_mask,8); break;
        case 0x02   : write_pixel_line(x,y,data,mem_mask,8); break;
        case 0x03   : set_pixel(x,y,(reg024&0x0F)); break;      
    }
}

static void write_reg_004(UINT16    data, UINT16    mem_mask)
{
    COMBINE_DATA(&reg004);
    
    reg002_x=1;
    reg00C_y++;
}

static void write_reg_010(UINT16    data, UINT16    mem_mask)
{
    write_pixel_data(0,reg00C_y,data,mem_mask);    
}

static void write_reg_014(UINT16    data, UINT16    mem_mask)
{  
    write_pixel_data(reg002_x+1,reg00C_y,data,mem_mask);
    
    reg00C_y++;
}

static void write_reg_01A(UINT16    data, UINT16    mem_mask)
{
    write_pixel_data(reg002_x+1,reg00C_y,data,mem_mask);  
    
    reg002_x++;
}

static void write_reg_026(UINT16    data, UINT16    mem_mask)
{
    COMBINE_DATA(&reg026);

    switch (data & 0x30)
    {
        case    0x00    : bpp=4;
        case    0x10    : bpp=2;
        case    0x20    : bpp=2;
        case    0x30    : bpp=2;        
    }

    ppb=(8/bpp);

    border_colour=data & 0x0F;

    if(debug_on & DEBUG_TEXT)
        logerror("reg 026 write, bpp=%02X, border_colour=%02X\n",bpp,border_colour);
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


VIDEO_START( nimbus )
{
    debug_on=0;
    
    reg002_x=0x0000;
    reg00C_y=0x0000;
    
    reg022=0x0000;
    reg024=0x0000;
    reg028=0x0000;
    bpp=4;          // bits per pixel

    logerror("VIDEO_START\n");
    video_mem=(UINT8 *)memory_region_alloc(machine,VRAM_NAME,VRAM_SIZE,ROMREGION_8BIT);

    memset(video_mem,0,VRAM_SIZE); 
    
   	if (machine->debug_flags & DEBUG_FLAG_ENABLED)
	{
        debug_console_register_command(machine, "nimbus_vid_debug", CMDFLAG_NONE, 0, 0, 1, video_debug);
    }
}

VIDEO_EOF( nimbus )
{

//    logerror("VIDEO_EOF( nimbus )\n");
}

VIDEO_UPDATE( nimbus )
{
    int     XCoord;
    int     YCoord = video_screen_get_vpos(screen);
    offs_t  vaddr;
    UINT8   pixel_offset;
    UINT8   shifts;
    UINT8   pixel;
    
    UINT16  xmax;

    if (bpp==2) 
        xmax=MAX_PIXELX_2BPP;
    else
        xmax=MAX_PIXELX_4BPP;

    vaddr=(YCoord*BYTES_PER_LINE)-1;
    
    for(XCoord=0;XCoord<xmax;XCoord++)
    {
        if ((XCoord % (8/bpp))==0)
            vaddr++;
            
        pixel_offset=(XCoord % 8);
            
        if(bpp==2)
        {
            shifts=(pixel_offset%4)*2;
            pixel=(video_mem[vaddr] & (0xC0>>shifts)) >> (6-shifts);
        }
        else
        {
            shifts=(pixel_offset%2)*4;
            pixel=(video_mem[vaddr] & (0xF0>>shifts)) >> (4-shifts);
        }
            
        *BITMAP_ADDR16(bitmap, YCoord, XCoord)=pixel;
    }

    reg028++;
    if((reg028 & 0x000F)>0x0A)
        reg028&=0xFFF0;


    return 0;
}

