/***************************************************************************

  $Id: pc8801.c,v 1.21 2006/09/15 02:51:43 npwoods Exp $

***************************************************************************/

#include "driver.h"
#include "includes/pc8801.h"

/* NPW 23-Oct-2001 - Adding this so that it compiles */
#define palette_transparent_pen	0

unsigned char *pc88sr_textRAM=NULL;
int pc8801_is_24KHz;
static unsigned char *gVRAM=NULL;
static int selected_vram;
static int crtcON,textON,text_width=40,text_height=20,text_invert,text_color;
static int text_cursor,text_cursorX,text_cursorY;
static int blink_period=24;
static int analog_palette;
static int ALU1,ALU2,ALUON;
static unsigned char ALU_save0,ALU_save1,ALU_save2;

static int dmac_FL;
static UINT16 dmac_addr[3],dmac_size[3];
static UINT8 dmac_flag,dmac_status;

enum
{
	noblink_underline,
	blink_underline,
	noblink_block,
	blink_block
} cursor_mode;

enum
{
	parameter0,
	parameter1,
	parameter2,
	parameter3,
	parameter4,
	cursorx,
	cursory,
	lpenx,
	lpeny,
	other
} crtc_state;

typedef enum
{
	GRAPH_NO,
	GRAPH_200_COL,
	GRAPH_200_BW,
	GRAPH_400_BW
} GMODE;

static GMODE gmode=GRAPH_NO;
static int disp_plane[3];

#define TX_SEC		0x0001
#define TX_BL		0x0002
#define TX_REV		0x0004
#define TX_CUR		0x0008
#define TX_OL		0x0010
#define TX_UL		0x0020
#define TX_GL		0x1000
#define TX_COL_MASK	0xe000
#define TX_COL_SHIFT    13
#define TX_80		0x0000
#define TX_40L		0x0100
#define TX_40R		0x0200
#define TX_WID_MASK	0x0300
#define TX_WID_SHIFT	8

static char *graph_dirty=NULL;
static unsigned short *attr_tmp=NULL,*attr_old=NULL,*text_old=NULL;
static mame_bitmap *wbm1,*wbm2;

#define TRAM(x,y) (pc88sr_is_highspeed ? \
	pc88sr_textRAM[(dmac_addr[2]+(x)+(y)*120)&0xfff] : \
	pc8801_mainRAM[(dmac_addr[2]+(x)+(y)*120)&0xffff])
#define ATTR_TMP(x,y) (attr_tmp[(x)+(y)*80])
#define TEXT_OLD(x,y) (text_old[(x)+(y)*80])
#define ATTR_OLD(x,y) (attr_old[(x)+(y)*80])
#define GRP_DIRTY(x,y) (graph_dirty[(x)+(y)*80])

void pc8801_video_init (int hireso)
{
	wbm1 = auto_bitmap_alloc(Machine->screen[0].width, Machine->screen[0].height, BITMAP_FORMAT_INDEXED16);
	wbm2 = auto_bitmap_alloc(Machine->screen[0].width, Machine->screen[0].height, BITMAP_FORMAT_INDEXED16);
	pc8801_is_24KHz=hireso;
	crtcON=0;
	textON=1;

	dmac_FL=0;
	dmac_addr[0]=dmac_size[0]=0;
	dmac_addr[1]=dmac_size[1]=0;
	dmac_addr[2]=dmac_size[2]=0;
	dmac_flag=dmac_status=0;

	gVRAM = (UINT8*) auto_malloc(0xc000);
	pc88sr_textRAM = (UINT8*) auto_malloc(0x1000);
	attr_tmp = auto_malloc(sizeof(unsigned short)*80*25);
	attr_old = auto_malloc(sizeof(unsigned short)*80*100);
	text_old = auto_malloc(sizeof(unsigned short)*80*100);
	graph_dirty = auto_malloc(80*100);

	if (!wbm1 || !wbm2)
	{
		logerror ("pc8801: out of memory!\n");
		return;
	}

	memset(gVRAM,0,0xc000);
	memset(pc88sr_textRAM,0,0x1000);
	selected_vram=0;
	ALUON=0;
	analog_palette=0;
}

static WRITE8_HANDLER(write_gvram)
{
  int x,y;

  if(gVRAM[offset]!=data) {
    gVRAM[offset]=data;
    switch(gmode) {
    case GRAPH_200_COL:
    case GRAPH_200_BW:
      x=(offset&0x3fff)%80;
      y=(offset&0x3fff)/80/2;
      break; 
    case GRAPH_400_BW:
      x=(offset&0x3fff)%80;
      y=(offset&0x3fff)/80/4+(offset/0x4000)*50;
      break;
    case GRAPH_NO:
    default:
      return;
    }
    if(y<100) GRP_DIRTY(x,y)=1;
  }
}

/* NPW 8-Feb-2004 - This pseudo template stuff is an abomination.  Whomever wrote this should be shot */

#define VVV \
	XXX(0) \
	XXX(1) \
	XXX(2)

#define XXX(n) \
static WRITE8_HANDLER(write_gvram##n##_bank5){write_gvram(offset+0x4000*n,data);} \
static WRITE8_HANDLER(write_gvram##n##_bank6){write_gvram(offset+0x4000*n+0x3000,data);}
	VVV
#undef XXX

#define YYY \
	XXX(0) \
	XXX(1) \
	XXX(2) \
	XXX(3) \
	XXX(4) \
	XXX(5) \
	XXX(6) \
	XXX(7)

#define ZZZ \
	XXX(0) \
	XXX(1) \
	XXX(2) \
	XXX(3)

#define XXX(x) \
static  READ8_HANDLER(read_gvram_alu##x) \
{ \
  ALU_save0 = gVRAM[offset+0x0000]; \
  ALU_save1 = gVRAM[offset+0x4000]; \
  ALU_save2 = gVRAM[offset+0x8000]; \
  return \
    (x & 1 ? gVRAM[offset+0x0000] : ~gVRAM[offset+0x0000]) & \
    (x & 2 ? gVRAM[offset+0x4000] : ~gVRAM[offset+0x4000]) & \
    (x & 4 ? gVRAM[offset+0x8000] : ~gVRAM[offset+0x8000]); \
} \
static  READ8_HANDLER(read_gvram_alu##x##_bank5){return read_gvram_alu##x(offset);} \
static  READ8_HANDLER(read_gvram_alu##x##_bank6){return read_gvram_alu##x(offset+0x3000);}

YYY

#undef XXX

static WRITE8_HANDLER(write_gvram_alu0)
{
#define WWW(x) \
  switch(ALU1&(0x11<<x)) { \
  case 0x00<<x: \
    write_gvram(offset+x*0x4000,gVRAM[offset+x*0x4000]&(~data)); \
    break; \
  case 0x01<<x: \
    write_gvram(offset+x*0x4000,gVRAM[offset+x*0x4000]|data); \
    break; \
  case 0x10<<x: \
    write_gvram(offset+x*0x4000,gVRAM[offset+x*0x4000]^data); \
    break; \
  case 0x11<<x: \
    break; \
  }

  WWW(0)
  WWW(1)
  WWW(2)

#undef WWW
}
static WRITE8_HANDLER(write_gvram_alu1)
{
  write_gvram(offset+0x0000 , ALU_save0);
  write_gvram(offset+0x4000 , ALU_save1);
  write_gvram(offset+0x8000 , ALU_save2);
}
static WRITE8_HANDLER(write_gvram_alu2){write_gvram(offset+0x0000,ALU_save1);}
static WRITE8_HANDLER(write_gvram_alu3){write_gvram(offset+0x4000,ALU_save0);}

#define XXX(x) \
static WRITE8_HANDLER(write_gvram_alu##x##_bank5){write_gvram_alu##x(offset,data);} \
static WRITE8_HANDLER(write_gvram_alu##x##_bank6){write_gvram_alu##x(offset+0x3000,data);}

ZZZ

#undef XXX

int is_pc8801_vram_select(void)
{
	read8_handler rh5 = NULL, rh6 = NULL;
	write8_handler wh5 = NULL, wh6 = NULL;

  if(ALUON) {
    /* ALU mode */
    if(ALU2&0x80) {
      /* ALU VRAM */
      switch(ALU2&0x07) {
#define XXX(x) \
      case x: \
	rh5 = read_gvram_alu##x##_bank5; \
	rh6 = read_gvram_alu##x##_bank6; \
	break;

	YYY

#undef XXX
#undef YYY
      }
      switch(ALU2&0x30) {
#define XXX(x) \
      case x<<4: \
	wh5 = write_gvram_alu##x##_bank5; \
	wh6 = write_gvram_alu##x##_bank6; \
	break;

	ZZZ

#undef XXX
#undef ZZZ
      }
      return 1;
    } else {
      return 0; /* MAIN RAM */
    }
  } else {
    /* old mode */
    switch(selected_vram) {
#define XXX(n) \
    case (n+1): \
      rh5 = MRA8_BANK5; \
      rh6 = MRA8_BANK6; \
      wh5 = write_gvram##n##_bank5; \
      wh6 = write_gvram##n##_bank6; \
      memory_set_bankptr(5, gVRAM + 0x4000*n ); \
      memory_set_bankptr(6, gVRAM + 0x4000*n + 0x3000 ); \
      return 1;

      VVV

#undef XXX
#undef VVV

    default:
      return 0; /* MAIN RAM */
    }
  }

  
	if (rh5) memory_install_read8_handler(0,  ADDRESS_SPACE_PROGRAM, 0xc000, 0xefff, 0, 0, rh5);
	if (wh5) memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xefff, 0, 0, wh5);
	if (rh6) memory_install_read8_handler(0,  ADDRESS_SPACE_PROGRAM, 0xf000, 0xffff, 0, 0, rh6);
	if (wh6) memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf000, 0xffff, 0, 0, wh6);
}

WRITE8_HANDLER(pc88sr_disp_32)
{
  analog_palette=((data & 0x20) != 0x00);
  ALUON=((data & 0x40) != 0x00);
  pc8801_update_bank();
}

WRITE8_HANDLER(pc88sr_ALU)
{
  switch(offset) {
  case 0:
    ALU1=data;
    break;
  case 1:
    ALU2=data;
    break;
  }
  pc8801_update_bank();
}

WRITE8_HANDLER(pc8801_vramsel)
{
  if(offset==3) {
    selected_vram=0;
  } else {
    selected_vram=offset+1;
  }
  pc8801_update_bank();
}

 READ8_HANDLER(pc8801_vramtest)
{
  switch(selected_vram) {
  case 1: return 0xf9;
  case 2: return 0xfa;
  case 3: return 0xfc;
  default: return 0xf8;
  }
}

VIDEO_START( pc8801 )
{
	return 0;
}

#define BLOCK_YSIZE (pc8801_is_24KHz ? 4 : 2)

VIDEO_UPDATE( pc8801 )
{
  int x,y,attr_new,text_new,i,a,tx,ty,oy,gx,gy,ct,cg;
  static int blink_count;
  int full_refresh = 1;

  blink_count=(blink_count+1)%(blink_period*4);
  /* attribute expand */
  for(y=0;y<25;y++) {
    x=0;
    attr_new=7 << TX_COL_SHIFT;
    for(i=0;i<=20;i++) {
      a=TRAM(80+i*2+1,y);
      if(text_color) {
	/* color mode */
	if(a&0x08) {
	  /* color */
	  attr_new=(attr_new&0x00ff)|((a&0xf0)<<8);
	} else {
	    /* attr */
	  attr_new=(attr_new&0xff00)|(a&37);
	}
      } else {
	/* B/W mode */
	attr_new=(a&0x37)|((a&0x80)<<5)|(7 << TX_COL_SHIFT);
      }
      if(TRAM(118,y)==0x80) {
	while(x<TRAM(80+i*2,y) && x<80) {
	  ATTR_TMP(x,y)=attr_new;
	  x++;
	};
      } else {
	while(x<TRAM(80+i*2+2,y) && x<80) {
	  ATTR_TMP(x,y)=attr_new;
	  x++;
	};
      }
    }
    while(x<80) {
      ATTR_TMP(x,y)=attr_new;
      x++;
    };
  }
  if(text_cursor &&
     (text_cursorX>=0 || text_cursorX<80) &&
     (text_cursorY>=0 || text_cursorY<25)) {
    ATTR_TMP(text_cursorX,text_cursorY)|=TX_CUR;
  }

  /* display draw */
  for(y=0;y<100;y++)
  {
    ty=y/(100/text_height);
    oy=y%(100/text_height);
    for(x=0;x<80;x++) {
      /* text */
      if(text_width==40) {
	tx=x&(~1);
      } else {
	tx=x;
      }
      attr_new=ATTR_TMP(tx,ty);
      if(oy<4) {
	text_new=TRAM(tx,ty)*4+oy;
      } else {
	text_new=0;
      }
      if(attr_new&TX_SEC) {
	text_new=0;
	attr_new&=~TX_SEC;
      }
      if(attr_new&TX_BL) {
	attr_new&=~TX_BL;
	if((blink_count/blink_period)&1) attr_new^=TX_REV;
      }
      if(attr_new&TX_CUR) {
	attr_new&=~TX_CUR;
	switch(cursor_mode) {
	case noblink_underline:
	  attr_new^=TX_UL;
	  break;
	case blink_underline:
	  if(blink_count/blink_period) attr_new^=TX_UL;
	  break;
	case noblink_block:
	  attr_new^=TX_REV;
	  break;
	case blink_block:
	  if(blink_count/blink_period) attr_new^=TX_REV;
	  break;
	}
      }
      if((attr_new&TX_UL) && oy!=3) {
	attr_new&=~TX_UL;
      }
      if((attr_new&TX_OL) && oy!=0) {
	attr_new&=~TX_OL;
      }
      if(text_invert) {
	attr_new^=TX_REV;
      }
      if(!crtcON || ((dmac_flag&0x04)==0x00) || !textON) {
	text_new=0;
	attr_new=7 << TX_COL_SHIFT;
      }
      if(text_width==40) {
	if(x&1) {
	  attr_new|=TX_40R;
	} else {
	  attr_new|=TX_40L;
	}
      } else {
	attr_new|=TX_80;
      }
      if(text_new!=TEXT_OLD(x,y) ||
	 attr_new!=ATTR_OLD(x,y) ||
	 GRP_DIRTY(x,y) ||
	 full_refresh) {
	plot_box(wbm2,x*8,y*BLOCK_YSIZE,8,BLOCK_YSIZE,palette_transparent_pen);
	ct=Machine->pens[((attr_new&TX_COL_MASK)>>TX_COL_SHIFT)+8];
	TEXT_OLD(x,y)=text_new;
	ATTR_OLD(x,y)=attr_new;
	if(attr_new&TX_GL) {
	  /* low resolution(N-BASIC mode) graphics */
	  text_new>>=(text_new&3);
	  if(attr_new&TX_REV) {
	    text_new=~text_new;
	  }
	  switch(attr_new&TX_WID_MASK) {
	  case TX_80:
	    if(text_new&0x04) plot_box(wbm2,x*8,y*BLOCK_YSIZE,4,BLOCK_YSIZE,ct);
	    if(text_new&0x40) plot_box(wbm2,x*8+4,y*BLOCK_YSIZE,4,BLOCK_YSIZE,ct);
	    break;
	  case TX_40L:
	    if(text_new&0x04) plot_box(wbm2,x*8,y*BLOCK_YSIZE,8,BLOCK_YSIZE,ct);
	    break;
	  case TX_40R:
	    if(text_new&0x40) plot_box(wbm2,x*8,y*BLOCK_YSIZE,8,BLOCK_YSIZE,ct);
	    break;
	  }
	} else {
	  /* normal text */
	  drawgfx(wbm2,Machine->gfx
		  [((attr_new&TX_WID_MASK)>>TX_WID_SHIFT)+
		  (pc8801_is_24KHz ? 3 : 0)],
		  text_new,
		  ((attr_new&TX_REV) ? 8 : 0)
		  + ((attr_new&TX_COL_MASK)>>TX_COL_SHIFT),
		  0,0,x*8,y*BLOCK_YSIZE,
		  &Machine->screen[0].visarea,TRANSPARENCY_PEN,
		  (attr_new&TX_REV) ? 1 : 0);
	}
	if(attr_new&TX_UL) {
	  plot_box(wbm2,x*8,y*BLOCK_YSIZE+(pc8801_is_24KHz ? 3 : 1),8,1,
		   (attr_new&TX_REV) ? palette_transparent_pen : ct);
	}
	if(attr_new&TX_OL) {
	  plot_box(wbm2,x*8,y*BLOCK_YSIZE,8,1,
		   (attr_new&TX_REV) ? palette_transparent_pen : ct);
	}

      /* graphics */
	GRP_DIRTY(x,y)=0;
	if(pc8801_is_24KHz) {
	  switch(gmode) {
	  case GRAPH_200_COL:
	    for(gy=0;gy<2;gy++) {
	      for(gx=0;gx<8;gx++) {
		cg=
		  (((gVRAM[0x0000+x+y*2*80+gy*80] << gx) & 0x80) >> 7) |
		  (((gVRAM[0x4000+x+y*2*80+gy*80] << gx) & 0x80) >> 6) |
		  (((gVRAM[0x8000+x+y*2*80+gy*80] << gx) & 0x80) >> 5);
		plot_pixel(wbm1,x*8+gx,y*4+gy*2,Machine->pens[cg]);
		plot_pixel(wbm1,x*8+gx,y*4+gy*2+1,Machine->pens[17]);
	      }
	    }
	    break;
	  case GRAPH_200_BW:
	    for(gy=0;gy<2;gy++) {
	      for(gx=0;gx<8;gx++) {
		cg=
		  (((gVRAM[0x0000+x+y*2*80+gy*80] << gx) & 0x80) &&
		   disp_plane[0]) ||
		  (((gVRAM[0x4000+x+y*2*80+gy*80] << gx) & 0x80) &&
		   disp_plane[1]) ||
		  (((gVRAM[0x8000+x+y*2*80+gy*80] << gx) & 0x80) &&
		   disp_plane[2]) ?
		  ((attr_new&TX_COL_MASK)>>TX_COL_SHIFT)+8 : 16;
		plot_pixel(wbm1,x*8+gx,y*4+gy*2,Machine->pens[cg]);
		plot_pixel(wbm1,x*8+gx,y*4+gy*2+1,Machine->pens[17]);
	      }
	    }
	    break;
	  case GRAPH_400_BW:
	    for(gy=0;gy<4;gy++) {
	      for(gx=0;gx<8;gx++) {
		cg=
		  ((gVRAM[(y<50 ? 0x0000 : 0x4000)+x+(y%50)*4*80+gy*80] << gx)
		   & 0x80) &&
		  disp_plane[y<200 ? 0 : 1] ?
		  ((attr_new&TX_COL_MASK)>>TX_COL_SHIFT)+8 : 16;
		plot_pixel(wbm1,x*8+gx,y*4+gy,Machine->pens[cg]);
	      }
	    }
	    break;
	  case GRAPH_NO:
	  default:
	    plot_box(wbm1,x*8,y*4,8,4,Machine->pens[16]);
	    break;
	  }
	} else {
	  switch(gmode) {
	  case GRAPH_200_COL:
	    for(gy=0;gy<2;gy++) {
	      for(gx=0;gx<8;gx++) {
		cg=
		  (((gVRAM[0x0000+x+y*2*80+gy*80] << gx) & 0x80) >> 7) |
		  (((gVRAM[0x4000+x+y*2*80+gy*80] << gx) & 0x80) >> 6) |
		  (((gVRAM[0x8000+x+y*2*80+gy*80] << gx) & 0x80) >> 5);
		plot_pixel(wbm1,x*8+gx,y*2+gy,Machine->pens[cg]);
	      }
	    }
	    break;
	  case GRAPH_200_BW:
	    for(gy=0;gy<2;gy++) {
	      for(gx=0;gx<8;gx++) {
		cg=
		  (((gVRAM[0x0000+x+y*2*80+gy*80] << gx) & 0x80) &&
		   disp_plane[0]) ||
		  (((gVRAM[0x4000+x+y*2*80+gy*80] << gx) & 0x80) &&
		   disp_plane[1]) ||
		  (((gVRAM[0x8000+x+y*2*80+gy*80] << gx) & 0x80) &&
		   disp_plane[2]) ?
		  ((attr_new&TX_COL_MASK)>>TX_COL_SHIFT)+8 : 16;
		plot_pixel(wbm1,x*8+gx,y*2+gy,Machine->pens[cg]);
	      }
	    }
	    break;
	  case GRAPH_NO:
	  default:
	    plot_box(wbm1,x*8,y*2,8,2,Machine->pens[16]);
	    break;
	  }
	}
      }
    }
  }

  copybitmap(wbm1,wbm2,0,0,0,0,
	     &Machine->screen[0].visarea,TRANSPARENCY_PEN,palette_transparent_pen);
  copybitmap(bitmap,wbm1,0,0,0,0,
	     &Machine->screen[0].visarea,TRANSPARENCY_NONE,0);
	return 0;
}

PALETTE_INIT( pc8801 )
{
	int i;

	for (i = 0; i < 8; i++)
		palette_set_color(machine, i, 0, 0, 0);	/* for graphics */

	/* for text */
	for (i = 0; i < 8; i++)
	{
		palette_set_color(machine, i+8,
			(i & 2) ? 0xff : 0x00,
			(i & 4) ? 0xff : 0x00,
			(i & 1) ? 0xff : 0x00);
		colortable[i*2+0] = 0;
		colortable[i*2+1] = i+8;
		colortable[(i+8)*2+0] = i+8;
		colortable[(i+8)*2+1] = 0;
	}

	/* for background and scanline */
	palette_set_color(machine, 16, 0, 0, 0);
	palette_set_color(machine, 17, 0, 0, 0);
}

WRITE8_HANDLER(pc8801_crtc_write)
{
  if(offset==1) {
    /* command */
    switch(data&0xe0) {
    case 0x00:
      /* disp stop / initilize */
      crtcON=0;
      crtc_state=parameter0;
      return;
    case 0x20:
      /* disp on */
      crtcON=1;
      text_invert=((data&1)!=0x00);
      return;
    case 0x40:
      /* case 0x43: */
      /* interupt mask */
      return;
    case 0x60:
      /* get light pen point */
      crtc_state=lpenx;
      return;
    case 0x80:
      /* set cursor */
      crtc_state=cursorx;
      text_cursor=((data&1)!=0x00);
      return;
    default:
      logerror("pc8801:illegal crtc command 0x%.2x.\n",data);
      return;
    }
  } else {
    /* parameter */
    switch(crtc_state) {
    case parameter0:
      crtc_state=parameter1;
      if(data==0xce) return;
      logerror("pc8801:illegal crtc parameter0 0x%.2x.\n",data);
      return;
    case parameter1:
      crtc_state=parameter2;
      switch(data&0xc0) {
      case 0x00:
	blink_period=8;
	break;
      case 0x40:
	blink_period=16;
	break;
      case 0x80:
	blink_period=24;
	break;
      case 0xc0:
	blink_period=32;
	break;
      }	
      if((data&0x3f)==0x13) {
	text_height=20;
	return;
      } else if((data&0x3f)==0x18) {
	text_height=25;
	return;
      }
      logerror("pc8801:illegal crtc parameter1 0x%.2x.\n",data);
      return;
    case parameter2:
      crtc_state=parameter3;
      switch(data&0x60) {
      case 0x00:
	cursor_mode=noblink_underline;
	break;
      case 0x20:
	cursor_mode=blink_underline;
	break;
      case 0x40:
	cursor_mode=noblink_block;
	break;
      case 0x60:
	cursor_mode=blink_block;
	break;
      }
      if(!pc8801_is_24KHz && (data&0x9f)==0x09 && text_height==20) {
	return;
      } else if(!pc8801_is_24KHz && (data&0x9f)==0x07 && text_height==25) {
	return;
      } else if(pc8801_is_24KHz && (data&0x9f)==0x13 && text_height==20) {
	return;
      } else if(pc8801_is_24KHz && (data&0x9f)==0x0f && text_height==25) {
	return;
      }
      logerror("pc8801:illegal crtc parameter2 0x%.2x.\n",data);
      return;
    case parameter3:
      crtc_state=parameter4;
      if(!pc8801_is_24KHz && data==0xbe && text_height==20) {
	return;
      } else if(!pc8801_is_24KHz && data==0xde && text_height==25) {
	return;
      } else if(pc8801_is_24KHz && data==0x38 && text_height==20) {
	return;
      } else if(pc8801_is_24KHz && data==0x58 && text_height==25) {
	return;
      }
      logerror("pc8801:illegal crtc parameter3 0x%.2x.\n",data);
      return;
    case parameter4:
      crtc_state=other;
      if(data==0x13) {
	text_color=0;
	return;
      } else if(data==0x53) {
	text_color=1;
	return;
      }
      logerror("pc8801:illegal crtc parameter4 0x%.2x.\n",data);
      return;
    case cursorx:
      crtc_state=cursory;
      text_cursorX=data;
      return;
    case cursory:
      crtc_state=other;
      text_cursorY=data;
      return;
    default:
      logerror("pc8801:illegal crtc parameter 0x%.2x.\n",data);
      return;
    }
  }
}

 READ8_HANDLER(pc8801_crtc_read)
{
  if(offset==0) {
    /* light pen point */
    switch(crtc_state) {
    case lpenx:
      crtc_state=lpeny;
      return 0xff;
    case lpeny:
      crtc_state=other;
      return 0xff;
    default:
      logerror("pc8801:illegal light pen read\n");
      return 0xff;
    }
  } else {
    /* status */
    return crtcON ? 0x10 : 0x00;
  }
}

WRITE8_HANDLER(pc8801_dmac_write)
{
  switch(offset) {
  case 8:
    dmac_FL=0;
    dmac_flag=data;
    return;
  case 1:
  case 3:
  case 5:
  case 7:
    if(dmac_FL==0) {
      dmac_FL=1;
      dmac_size[offset/2]=(dmac_size[offset/2]&0xff00)|(data&0x00ff);
      return;
    } else {
      dmac_FL=0;
      dmac_size[offset/2]=(dmac_size[offset/2]&0x00ff)|((data<<8)&0xff00);
      return;
    }
  case 0:
  case 2:
  case 4:
  case 6:
    if(dmac_FL==0) {
      dmac_FL=1;
      dmac_addr[offset/2]=(dmac_addr[offset/2]&0xff00)|(data&0x00ff);
      return;
    } else {
      dmac_FL=0;
      dmac_addr[offset/2]=(dmac_addr[offset/2]&0x00ff)|((data<<8)&0xff00);
      return;
    }
  }
}

 READ8_HANDLER(pc8801_dmac_read)
{
  switch(offset) {
  case 8:
    return dmac_status;
  case 1:
  case 3:
  case 5:
  case 7:
    if(dmac_FL==0) {
      dmac_FL=1;
      return(dmac_size[offset/2]&0x00ff);
    } else {
      dmac_FL=0;
      return((dmac_size[offset/2]>>8)&0x00ff);
    }
  case 0:
  case 2:
  case 4:
  case 6:
    if(dmac_FL==0) {
      dmac_FL=1;
      return(dmac_addr[offset/2]&0x00ff);
    } else {
      dmac_FL=0;
      return((dmac_addr[offset/2]>>8)&0x00ff);
    }
  }
  return 0;
}

WRITE8_HANDLER(pc88sr_disp_30)
{
  text_width=((data&0x01)==0x00) ? 40 : 80;
}

WRITE8_HANDLER(pc88sr_disp_31)
{
  GMODE gmode_new;

  gmode_new=GRAPH_NO;
  switch(data&0x19) {
  case 0x00:
  case 0x01:
  case 0x10:
  case 0x11:
    gmode_new=GRAPH_NO;
    break;
  case 0x08:
    gmode_new=pc8801_is_24KHz ? GRAPH_400_BW : GRAPH_200_BW;
    break;
  case 0x09:
    gmode_new=GRAPH_200_BW;
    break;
  case 0x18:
  case 0x19:
    gmode_new=GRAPH_200_COL;
    break;
  }
  if(gmode!=gmode_new) {
    gmode=gmode_new;
  }
}

WRITE8_HANDLER(pc8801_palette_out)
{
  int palno;
  int i;
  static int r[10],g[10],b[10];

  if(offset==0) {
    palno=16;
  } else if(offset==1) {
    textON = (data&1)==0x00;
    for(i=0;i<3;i++) {
      if(disp_plane[i] != ((data&(2<<i))==0x00)) {
	disp_plane[i] = (data&(2<<i))==0x00;
      }
    }
    return;
  } else {
    palno=offset-2;
  }
  if(analog_palette) {
    if((data&0x40)==0x00) {
      b[offset] =
	((data << 5) & 0xe0) |
	((data << 2) & 0x1c) |
	((data >> 1) & 0x03);
      r[offset] =
	((data << 2) & 0xe0) |
	((data >> 1) & 0x1c) |
	((data >> 4) & 0x03);
    } else {
      g[offset] =
	((data << 5) & 0xe0) |
	((data << 2) & 0x1c) |
	((data >> 1) & 0x03);
    }
  } else {
	b[offset] = (data & 1) ? 0xff : 0x00;
	r[offset] = (data & 2) ? 0xff : 0x00;
	g[offset] = (data & 4) ? 0xff : 0x00;
  }
  palette_set_color(Machine, palno,r[offset],g[offset],b[offset]);
}
