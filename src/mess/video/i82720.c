/******************************************************************************

	compis_gdc.c
	Video driver for the Intel 82720 and NEC uPD7220 GDC.

	Per Ola Ingvarsson
	Tomas Karlsson
			
 ******************************************************************************/

/*-------------------------------------------------------------------------*/
/* Include files                                                           */
/*-------------------------------------------------------------------------*/

#include "driver.h"
#include "state.h"
#include "video/generic.h"
#include "i82720.h"
#include "i82720cm.h"
#include <math.h>

/*-------------------------------------------------------------------------*/
/* Defines, constants, and global variables                                */
/*-------------------------------------------------------------------------*/
#define LOG_PORTS 0

#define GDC_FIFO_CMD_FLAG 1
#define GDC_FIFO_DIR_WRITE 0
#define GDC_FIFO_DIR_READ 1

#define GDC_LOG_CMDS
#ifdef GDC_LOG_CMDS
#define GDC_LOG_CMD_1(x) (logerror(x))
#define GDC_LOG_CMD_2(x,y) (logerror(x,y))
#define GDC_LOG_CMD_3(x,y,z) (logerror(x,y,z))
#define GDC_LOG_CMD_4(x,y,z,a) (logerror(x,y,z,a))
#else
#define GDC_LOG_CMD_1(x)     
#define GDC_LOG_CMD_2(x,y)   
#define GDC_LOG_CMD_3(x,y,z)
#define GDC_LOG_CMD_4(x,y,z,a) 
#endif

/* FIFO */
typedef struct
{
	UINT8	data[16];	      /* The data of the fifo               */
	UINT8	cmd_flags[16];		/* 0 for params 1 for commands        */
	UINT8	direction;	      /* Direction flag (R=1/W=0)           */
   UINT8 write_pos;        /* Position for next write operation  */                            
   UINT8 read_pos;         /* Position for next read operation   */
   UINT8 count;            /* The number of entries in the fifo  */
   UINT8 nbr_cmds;         /* The number of commands in the fifo */
} TYP_GDC_FIFO;


/* Raw command registers */
#define	GDC_CMD_REG_SYNC_MAX 8
#define	GDC_CMD_REG_CCHAR_MAX 3

typedef struct
{
	UINT8	sync[GDC_CMD_REG_SYNC_MAX];
} TYP_GDC_REGS_COMMAND;

enum gdc_display_mode {
   dm_mixed    = 0,
   dm_graphics = 1,
   dm_char     = 2,
   dm_invalid  = 3
};

/* Encoded registers */
typedef struct
{
   enum gdc_display_mode display_mode; /* Display mode    */
	UINT8	   words_per_line;	 /* Words per line  */
   UINT8    display_width;     /* Words per line visible */
   UINT8    horiz_sync_width;  /* Horizontal sync width */
   UINT8    vert_sync_width;   /* Vertical sync width */
   UINT8    horiz_front_porch; /* Horizontal front porch width */
   UINT8    vert_front_porch;  /* Vertical front porch width */
   UINT8    vert_back_porch;   /* Vertical back porch width */
	UINT16	lines_per_field;	 /* Number of lines */
   UINT8    disp_cursor;       /* True if cursor should be displayed */
   UINT8    lines_per_ch_row;  /* Number of lines per character row */
   UINT8    cursor_blinking;   /* True if the cursor should blink */
   UINT8    cursor_blink_rate; /* Cursor blink rate in 2xBR */
   UINT8    cursor_top_line;
   UINT8    cursor_bottom_line;
   UINT8    figs_operation;
   UINT8    figs_direction;
   UINT16   figs_dc_param;
   UINT16   figs_d_param;
   UINT16   figs_d2_param;
   UINT16   figs_d1_param;
   UINT16   figs_dm_param;
   UINT8    cur_mod;           /* Modification (set, reset, etc.) */
   UINT8    zoom_disp;
   UINT8    zoom_gchr;         /* Zoom for graphics chars */
   UINT16   mask;              /* Mask register */
   UINT32   ead;               /* Execute word address */
} TYP_GDC_REGS_DISPLAY;

typedef struct
{
	TYP_GDC_REGS_DISPLAY	display;
	TYP_GDC_REGS_COMMAND	command;
} TYP_GDC_REGS;


typedef struct
{
	TYP_GDC_REGS	registers;	/* Registers       */
	TYP_GDC_FIFO	fifo;		/* FIFO buffer     */
	UINT8		status;		/* Status register */
	UINT8		pram[16];	/* Parameter RAM   */
} TYP_GDC;



/* MESS stuff */
typedef struct
{
	UINT16	*vram;			/* Display memory  */
	UINT16	vramsize;		/* Size in words   */
	UINT8	mode;			/* Resolution mode */
	UINT8   dirty;
	mame_bitmap *tmpbmp;
} TYP_GDC_MESS;

static TYP_GDC_MESS gdc_mess;


/* Misc */
enum COMPIS_GDC_BITSS
{
	BIT_1	= 0x01,
	BIT_2	= 0x02,
	BIT_3	= 0x04,
	BIT_4	= 0x08,
	BIT_5	= 0x10,
	BIT_6	= 0x20,
	BIT_7	= 0x40,
	BIT_8	= 0x80
};

#define MOD_REPLACE    0
#define MOD_COMPLEMENT 1
#define MOD_RESET      2
#define MOD_SET        3

/*-------------------------------------------------------------------------*/
/* Name: gdc_fifo_clear                                                    */
/* Desc: FIFO - Clears the FIFO buffer                                     */
/*-------------------------------------------------------------------------*/
INLINE void gdc_fifo_clear(TYP_GDC* gdcp)
{
   gdcp->fifo.count     = 0;
   gdcp->fifo.direction = GDC_FIFO_DIR_WRITE;     /* Unknown really */
   gdcp->fifo.write_pos = 0;
   gdcp->fifo.read_pos  = 0;
   gdcp->fifo.nbr_cmds  = 0;
}

/*-------------------------------------------------------------------------*/
/* Name: gdc_fifo_reset                                                    */
/* Desc: FIFO - Resets the FIFO buffer                                     */
/*-------------------------------------------------------------------------*/
INLINE int gdc_fifo_reading(TYP_GDC* gdcp)
{
   return gdcp->fifo.direction != GDC_FIFO_DIR_WRITE;
}

/*-------------------------------------------------------------------------*/
/* Name: gdc_fifo_reset                                                    */
/* Desc: FIFO - Resets the FIFO buffer                                     */
/*-------------------------------------------------------------------------*/
INLINE void gdc_fifo_reset(TYP_GDC* gdcp)
{
   /* Maybe memset the contents */
	gdc_fifo_clear(gdcp);
}

/*-------------------------------------------------------------------------*/
/* Name: gdc_fifo_full                                                     */
/* Desc: FIFO - Returns 0 if the fifo is not full                          */
/*-------------------------------------------------------------------------*/
INLINE int gdc_fifo_full(TYP_GDC* gdcp)
{
   return gdcp->fifo.count == 16;
}

/*-------------------------------------------------------------------------*/
/* Name: gdc_fifo_empty                                                    */
/* Desc: FIFO - Returns 1 if the fifo is empty                             */
/*-------------------------------------------------------------------------*/
INLINE int gdc_fifo_empty(TYP_GDC* gdcp)
{
   return gdcp->fifo.count == 0;
}

/*-------------------------------------------------------------------------*/
/* Name: gdc_fifo_enqueue                                                  */
/* Desc: FIFO - Adds a command or parameter to the FIFO                    */
/*-------------------------------------------------------------------------*/
INLINE
void gdc_fifo_enqueue(TYP_GDC* gdcp, UINT8 data, UINT8 cmdflag)
{
   if ( ! gdc_fifo_full(gdcp) ) {
      gdcp->fifo.data[gdcp->fifo.write_pos] = data;
      gdcp->fifo.cmd_flags[gdcp->fifo.write_pos] = cmdflag;
      if ( cmdflag ) {
         gdcp->fifo.nbr_cmds++;
      }
      gdcp->fifo.count++;
      /* We cannot go higher than 15 */
      gdcp->fifo.write_pos = (gdcp->fifo.write_pos + 1) & 0x0f;
   } else {
      GDC_LOG_CMD_2("gdc_fifo_enqueue: The fifo is full at PC = %04X\n",
                    activecpu_get_pc());
   }
}

/*-------------------------------------------------------------------------*/
/* Name: gdc_fifo_add_from_outside                                         */
/* Desc: FIFO - Adds a command or parameter from cpu to fifo.              */
/*-------------------------------------------------------------------------*/
INLINE
void gdc_fifo_add_from_outside(TYP_GDC* gdcp, UINT8 data, UINT8 cmdflag)
{
   if ( gdcp->fifo.direction != GDC_FIFO_DIR_WRITE ) {
      gdc_fifo_clear(gdcp);
      gdcp->fifo.direction = GDC_FIFO_DIR_WRITE;
   }
   gdc_fifo_enqueue(gdcp, data, cmdflag);
   
}

/*-------------------------------------------------------------------------*/
/* Name: gdc_fifo_add_from_inside                                          */
/* Desc: FIFO - Adds a command or parameter from gdc to fifo.              */
/*-------------------------------------------------------------------------*/
INLINE
void gdc_fifo_add_from_inside(TYP_GDC* gdcp, UINT8 data, UINT8 cmdflag)
{
   if ( gdcp->fifo.direction != GDC_FIFO_DIR_READ ) {
      gdc_fifo_clear(gdcp);
      gdcp->fifo.direction = GDC_FIFO_DIR_READ;
   }
   gdc_fifo_enqueue(gdcp, data, cmdflag);
}

/*-------------------------------------------------------------------------*/
/* Name: gdc_fifo_dequeue                                                  */
/* Desc: FIFO - Dequeues a command or parameter from the FIFO              */
/*-------------------------------------------------------------------------*/
INLINE UINT8* gdc_fifo_dequeue(TYP_GDC* gdcp)
{
   if ( gdc_fifo_empty(gdcp) ) {
      return NULL;
   } else {
      UINT8* retval = &(gdcp->fifo.data[gdcp->fifo.read_pos]);
      if ( gdcp->fifo.cmd_flags[gdcp->fifo.read_pos] ) {
         /* Decrease the number of commands */
         gdcp->fifo.nbr_cmds--;
      }
      gdcp->fifo.count--;
      gdcp->fifo.read_pos = (gdcp->fifo.read_pos + 1) & 0x0f;
      return retval;
   }
}

/*-------------------------------------------------------------------------*/
/* Name: gdc_fifo_dequeue_cmd                                              */
/* Desc: FIFO - Dequeues a command from the fifo or returns NULL.          */
/*-------------------------------------------------------------------------*/
INLINE UINT8* gdc_fifo_dequeue_cmd(TYP_GDC* gdcp)
{
   /* Peek into the fifo and check if the next thing is a cmd */
   if ( gdcp->fifo.cmd_flags[gdcp->fifo.read_pos] ) {
      return gdc_fifo_dequeue(gdcp);
   } else {
      return NULL;
   }
}

/*-------------------------------------------------------------------------*/
/* Name: gdc_fifo_dequeue_param                                            */
/* Desc: FIFO - Dequeues a command from the fifo or returns NULL.          */
/*-------------------------------------------------------------------------*/
INLINE UINT8* gdc_fifo_dequeue_param(TYP_GDC* gdcp)
{
   /* Peek into the fifo and see if the next position has a parameter */
   if ( !gdcp->fifo.cmd_flags[gdcp->fifo.read_pos] ) {
      return gdc_fifo_dequeue(gdcp);
   } else {
      return NULL;
   }
}

/*-------------------------------------------------------------------------*/
/* Name: gdc_fifo_contains_cmd                                             */
/* Desc: FIFO - Return 1 if there are commands in the fifo.                */
/*-------------------------------------------------------------------------*/
INLINE int
gdc_fifo_contains_cmd(TYP_GDC* gdcp)
{
   return gdcp->fifo.nbr_cmds != 0;
}

/*-------------------------------------------------------------------------*/
/* Name: gdc_fifo_contains_cmd                                             */
/* Desc: FIFO - Return 1 if there are commands in the fifo.                */
/*-------------------------------------------------------------------------*/
INLINE int
gdc_fifo_get_nbr_params(TYP_GDC* gdcp)
{
   return gdcp->fifo.count - gdcp->fifo.nbr_cmds;
}

/* A gdc */
static TYP_GDC gdc;

INLINE UINT16 rotleft16(UINT16 nbr)
{
   return (nbr << 1) | (nbr >> 15);
}

INLINE UINT16 rotright16(UINT16 nbr)
{
   return (nbr >> 1) | (nbr << 15);
}

UINT32 gdc_get_base_addr(TYP_GDC* gdcp)
{  
  int i;
  UINT32 ret_val = 0;
  int shift_amount = 0;
  for( i = 0; i < 3; ++i ) {
     /* Get a byte and put it into a temp */
     UINT32 cur_pram_byte = gdcp->pram[i];
     cur_pram_byte <<= shift_amount;
     shift_amount += 8;
     ret_val |= cur_pram_byte;
  }
  return ret_val & 0x03ffff;
}


UINT32 gdc_partition_length(TYP_GDC* gdcp)
{  
   /* Get a byte and put it into a temp */
   UINT32 pram_byte_low = gdcp->pram[2] >> 4;
   UINT32 pram_byte_high = gdcp->pram[3] & 0x3f;
   return pram_byte_high << 8 | pram_byte_low;
}

/* Address here is from the top of the screen */
void gdc_plot_word(UINT32 address, UINT16 writeWord)
{
   TYP_GDC_REGS_DISPLAY* dispregs = &gdc.registers.display;
   UINT32 x;
   UINT32 y = address/dispregs->words_per_line;
   UINT32 xBase = (address) - (dispregs->words_per_line * y);
   xBase <<= 4; /* Multiply by the 16 bits per word */
   for ( x = xBase; x < xBase+16; ++x ) {
      if ( (x < 640) && (y < 400) ) { /* FIXME */
         if ( writeWord & 0x8000 ) {
            plot_pixel(gdc_mess.tmpbmp, x, y, 2);
         } else {
            plot_pixel(gdc_mess.tmpbmp, x, y, 0);
         }
      }
      writeWord = rotright16(writeWord); /* Shift would do */
   }
}

INLINE void gdc_write(UINT32 address,
               UINT16 data)
{   
   gdc_mess.vram[address % gdc_mess.vramsize] = data;
}

INLINE UINT16 gdc_read(UINT32 address)
{
   return gdc_mess.vram[address % gdc_mess.vramsize];
}


void gdc_write_data(UINT16 data,
                    UINT16 mask,
                    UINT8 operation)
{
   /* This could probably be optimized */
/*   UINT32 x,y; */
/*   UINT32 xBase; */
   UINT16 writeWord;
/*   TYP_GDC_REGS_DISPLAY* dispregs = &gdc.registers.display; */
   UINT32 address = gdc.registers.display.ead % gdc_mess.vramsize;
   switch ( operation ) {
      case MOD_REPLACE:
      default:
         writeWord = (mask & data) |
            ((~mask) & gdc_read(address) );
         gdc_write( address, writeWord );
         break;
      case MOD_COMPLEMENT:
         gdc_write(address, gdc_read(address) ^ (data & mask));
         break;
      case MOD_RESET:
         // Reset sets the ones to zero!! Thanks GSX Driver skeleton!
         gdc_write(address, gdc_read(address) & (~data & mask));
         break;
      case MOD_SET:
         gdc_write(address, gdc_read(address) | (data & mask));
         break;
   }
   /* GDC_LOG_CMD_3("gdc_write_data: address = %04x, base = %04x\n",
      address, base_addr); */
}

void gdc_update_ead_and_mask(void)
{
   UINT32 maxAddr = gdc.registers.display.words_per_line *
                    gdc.registers.display.lines_per_field;
   switch ( gdc.registers.display.figs_direction ) {
      case 0:
         gdc.registers.display.ead += gdc.registers.display.words_per_line;
         if ( gdc.registers.display.ead > maxAddr ) {
            gdc.registers.display.ead %=
               gdc.registers.display.words_per_line;
            gdc.registers.display.ead += 1;
         }
         break;
      case 2:
      default:
         /* Direction == 2 assumed */
         /* FIXME: Should the mask always be rotated ? */
         /* if ( gdc.registers.display.mask & 0x8000 ) { */
         gdc.registers.display.ead++;
         /* When I do this modding, the first screen is more beautiful */
         /* Why ? */
         //gdc.registers.display.ead %= gdc.registers.display.words_per_line;
            /*   } */
         /*gdc.registers.display.mask = rotleft16(gdc.registers.display.mask); */
         break;
   }
   gdc.registers.display.ead %= maxAddr;
}


void gdc_auto_write_data(UINT16 data,
                         UINT8  operation)
{
   gdc_write_data(data, gdc.registers.display.mask, operation);
   
   gdc_mess.dirty = 1;
   gdc_update_ead_and_mask();
}

UINT16 gdc_read_data(UINT16 mask)
{
   /* Save address */
   UINT32 address = gdc.registers.display.ead;
   
   return gdc_mess.vram[address & gdc_mess.vramsize] & mask;
}

UINT16 gdc_auto_read_data(void)
{
   UINT16 ret_val = gdc_read_data(gdc.registers.display.mask);   
   /* Update the ead and mask */
   gdc_update_ead_and_mask();
   return ret_val;
}

/*-------------------------------------------------------------------------*/
/* Name: gdc_dump_disp_regs                                                */
/* Desc: Dumps the contents of the display regs on logerror                */
/*-------------------------------------------------------------------------*/
INLINE
void gdc_dump_disp_regs(void)
{
	GDC_LOG_CMD_2("gdc: display mode   = %d\n",
					gdc.registers.display.display_mode);
	GDC_LOG_CMD_2("gdc: words_per_line = %d\n",
					gdc.registers.display.words_per_line);
	GDC_LOG_CMD_2("gdc: horiz_sync_width = %d\n",
					gdc.registers.display.horiz_sync_width);
	GDC_LOG_CMD_2("gdc: lines_per_field = %d\n",
					gdc.registers.display.lines_per_field);
}

INLINE const char*
gdc_disp_mode_to_str(int mode)
{
	switch ( mode ) {
		case 0:
			return "Mixed";
		case 1:
			return "Graphics";
		case 2:
			return "Character";
			break;
		case 3:
			return "Invalid";
			break;
	}
	return "";
}

/*-------------------------------------------------------------------------*/
/* Name: gdc_cmd_sync                                                      */
/* Desc: CMD - Sync                                                        */
/*-------------------------------------------------------------------------*/
void gdc_cmd_sync(void)
{
	UINT16 reg16;
	UINT8 reg;
	UINT8 i;
   UINT8* param_p;

   i = 0;
	/* Get the parameters */
	while ( (param_p = gdc_fifo_dequeue_param(&gdc)) ) {
		gdc.registers.command.sync[i] = *param_p;
      ++i;
	}


   if ( i == 0 )
      return;
   
	/* Update display mode */   
	reg = (gdc.registers.command.sync[0] & 0x20) ? 0x02 : 0;
	reg += (gdc.registers.command.sync[0] & 0x02) ? 0x01 : 0;
	gdc.registers.display.display_mode = reg;
   
   GDC_LOG_CMD_2("gdc_cmd_sync: mode byte = %02X\n",
                 gdc.registers.command.sync[0]);
   GDC_LOG_CMD_2("gdc_cmd_sync: mode is %s\n", gdc_disp_mode_to_str(reg));
   

   if ( i == 1 )
      return;
   
	/* Update words per line */
	reg = gdc.registers.command.sync[1] & 0xfe;
   /* The register contains the number of lines - 2 */
	gdc.registers.display.words_per_line = reg + 2;
   gdc.registers.display.display_width =  reg + 2;
   
   GDC_LOG_CMD_2("gdc_cmd_sync: words_per_line set to %d\n",
                 gdc.registers.display.words_per_line);
   GDC_LOG_CMD_2("gdc_cmd_sync: display_width set to %d\n",
                 gdc.registers.display.display_width);

   if ( i == 2 )
      return;

   /* Register should contain sync width - 1 */
   gdc.registers.display.horiz_sync_width =
      (gdc.registers.command.sync[2] & 0x1f) + 1;

   /* And vertical sync low bits */
   reg16 = gdc.registers.command.sync[2] >> 5;

   if ( i == 3 )
      return;

   /* Contains high bits of vertical sync */
   reg16 += (gdc.registers.command.sync[3] & 0x03 ) << 3;
   gdc.registers.display.vert_sync_width = reg16;

   /* And horizontal front porch width - 1 */
   gdc.registers.display.horiz_front_porch =
      (gdc.registers.command.sync[3] >> 2) + 1;

   if ( i == 4 )
      return;

   /* Horizontal back porch width - 1 */
   gdc.registers.display.horiz_front_porch =
      (gdc.registers.command.sync[4] & 0x3f) + 1;

   if ( i == 5 )
      return;

   /* Vertical back porch width - 1 */
   gdc.registers.display.vert_front_porch =
      (gdc.registers.command.sync[5] & 0x3f) + 1;

   if ( i == 6 )
      return;
   
   /* Low bits of active display lines per video field */
   reg16 = gdc.registers.command.sync[6];

   if ( i == 7 )
      return;

   /* High bits of active display lines per video field */
   reg16 += ( gdc.registers.command.sync[7] & 0x03 ) << 8;
   gdc.registers.display.lines_per_field = reg16;

   gdc.registers.display.vert_back_porch =
      gdc.registers.command.sync[7] >> 2;
   
   gdc_dump_disp_regs();
}

/*-------------------------------------------------------------------------*/
/* Name: gdc_cmd_pitch                                                     */
/* Desc: CMD - Pitch Specification Command                                 */
/*             Sets the witdth of the display memory.                      */
/*-------------------------------------------------------------------------*/
INLINE void gdc_cmd_pitch(void)
{
   UINT8* param_in_fifo = gdc_fifo_dequeue_param(&gdc);
   if ( param_in_fifo ) {
      gdc.registers.display.words_per_line = *param_in_fifo;
      GDC_LOG_CMD_2("gdc_cmd_pitch: words_per_line set to %d\n",
                    gdc.registers.display.words_per_line);
   }
}

/*-------------------------------------------------------------------------*/
/* Name: gdc_cmd_start                                                     */
/* Desc: CMD - Start Display And End Idle Mode                             */
/*                                                                         */
/*-------------------------------------------------------------------------*/
INLINE void gdc_cmd_start(void)
{
  /*   MDRV_SCREEN_SIZE(gdc.registers.display.words_per_line*16, */
/*                      gdc.registers.display.lines_per_field); */
/*     MDRV_SCREEN_VISIBLE_AREA(0, */
/*                       gdc.registers.display.words_per_line*16-1, */
/*                       0, */
/*                       gdc.registers.display.lines_per_field-1); */
}

/*-------------------------------------------------------------------------*/
/* Name: gdc_cmd_lprd                                                      */
/* Desc: CMD - Light Pen Address Read                                      */
/*                                                                         */
/*-------------------------------------------------------------------------*/
INLINE void gdc_cmd_lprd(void)
{
   int i;
   for ( i = 0; i < 3; ++i ) {
      /* Don't know what to put here */
      gdc_fifo_add_from_inside(&gdc, 0, 0);
   }
}

/*-------------------------------------------------------------------------*/
/* Name: gdc_cmd_pram                                                      */
/* Desc: CMD - Parameter Ram Write                                         */
/*-------------------------------------------------------------------------*/
INLINE void gdc_cmd_pram(UINT8 offset)
{
   UINT8* param_p;
   int i,k,x;
   char tmp_str[9];
   UINT8 tmpParam[16];
   UINT32 base_addr_before = gdc_get_base_addr(&gdc);  
   /* Upside down */   
   k = 0;
   while ( (param_p = gdc_fifo_dequeue_param(&gdc)) != NULL ) {
      /* This is for printing */
      tmpParam[k] = *param_p;
      /* This is what should be done. (What if offset is too large?) */
      if ( offset + k < 16 ) {
         gdc.pram[k+offset] = *param_p;
      }
      ++k;
   }
   for ( x = k-1; x >= 0; --x ) {
      UINT8 param = tmpParam[x];
      /* Print backwards */
      for ( i = 0; i < 8; ++i ) {
         if ( param & 0x80 ) {
            tmp_str[7-i] = 'X';
         } else {
            tmp_str[7-i] = ' ';
         }
         param <<= 1;
      }
      tmp_str[8] = 0;
      GDC_LOG_CMD_2("gdc_cmd_pram: %s\n", tmp_str);
   }
   if ( gdc_get_base_addr(&gdc) != base_addr_before ) {
      GDC_LOG_CMD_3("gdc_cmd_pram: base_addr changed from %08X to %08X\n",
                    base_addr_before, gdc_get_base_addr(&gdc));
   }
}

/*-------------------------------------------------------------------------*/
/* Name: gdc_cmd_cchar                                                     */
/* Desc: CMD - Cursor and Character Characteristics                        */
/*-------------------------------------------------------------------------*/
INLINE void gdc_cmd_cchar(void)
{
   UINT8 temp_reg;
   UINT8* param_p;
   TYP_GDC_REGS_DISPLAY* dispregs = &gdc.registers.display;
   
   if ( ( param_p = gdc_fifo_dequeue_param(&gdc)) == NULL ) return;
   /* Cursor on or off */
   dispregs->disp_cursor = (*param_p & 0x80) != 0;
   /* Number of lines per character row - 1 */
   dispregs->lines_per_ch_row = (*param_p & 0x1f) + 1;
   GDC_LOG_CMD_2("gdc_cmd_cchar: Display cursor = %d\n",
                 dispregs->disp_cursor);
   GDC_LOG_CMD_2("gdc_cmd_cchar: Lines per character row = %d\n",
                 dispregs->lines_per_ch_row);
   /* Next parameter */
   if ( ( param_p = gdc_fifo_dequeue_param(&gdc)) == NULL  ) return;
   /* Lower bits of blink rate (there are 5) */
   temp_reg = (*param_p >> 3) & 0x18;
   /* Steady or blinking cursor - 0 is blinking */
   dispregs->cursor_blinking = (*param_p & 0x20) == 0;
   /* Cursor top lite number in the row */
   dispregs->cursor_top_line = *param_p & 0x1f;
   GDC_LOG_CMD_2("gdc_cmd_cchar: Cursor blinking = %d\n",
                 dispregs->cursor_blinking);
   GDC_LOG_CMD_2("gdc_cmd_cchar: Cursor top line = %d\n",
                 dispregs->cursor_top_line);
   /* Next parameter */
   if ( ( param_p = gdc_fifo_dequeue_param(&gdc)) == NULL ) return;
   dispregs->cursor_blink_rate = (*param_p & 0x07) | temp_reg;
   dispregs->cursor_bottom_line = (*param_p & 0xf8) >> 3;
   GDC_LOG_CMD_2("gdc_cmd_cchar: Cursor blink rate = %d\n",
                 dispregs->cursor_blink_rate);
   GDC_LOG_CMD_2("gdc_cmd_cchar: Cursor bottom line = %d\n",
                 dispregs->cursor_bottom_line);
   
   
}

/*-------------------------------------------------------------------------*/
/* Name: gdc_cmd_figs                                                      */
/* Desc: CMD - Figure Drawing Parameters Specify Command                   */
/*-------------------------------------------------------------------------*/
INLINE void gdc_cmd_figs(void)
{
   int i;
   UINT8* param_p;
   UINT16 temp_reg;
   TYP_GDC_REGS_DISPLAY* dispregs = &gdc.registers.display;

   /* Set default parameters according to docu */
   dispregs->figs_dc_param = 0;
   dispregs->figs_d_param  = 8;
   dispregs->figs_d2_param = 8;
   dispregs->figs_d1_param = 0xff;
   dispregs->figs_dm_param = 0xff;

   
   if ( ( param_p = gdc_fifo_dequeue_param(&gdc)) == NULL ) return;
   /* What will be done next time? */
   dispregs->figs_operation = (*param_p & 0xf1) >> 3;
   dispregs->figs_direction = *param_p & 0x03;
   GDC_LOG_CMD_2("gdc_cmd_figs: Operation = %x\n", dispregs->figs_operation);
   GDC_LOG_CMD_2("gdc_cmd_figs: Direction = %x\n", dispregs->figs_direction);
   for ( i = 0; i < 5; ++i ) {
      UINT8 quit = 0;
      if ( ( param_p = gdc_fifo_dequeue_param(&gdc)) == NULL ) return;
      temp_reg = *param_p;
      if ( ( param_p = gdc_fifo_dequeue_param(&gdc)) != NULL ) {
         temp_reg |= (*param_p & 0x3f) << 8;         
      } else {
         quit = 1;
      }
      switch ( i ) {
         case 0:
            dispregs->figs_dc_param = temp_reg;
            break;
         case 1:
            dispregs->figs_d_param = temp_reg;
            break;
         case 2:
            dispregs->figs_d2_param = temp_reg;
            break;
         case 3:
            dispregs->figs_d1_param = temp_reg;
            break;
         case 4:            
            dispregs->figs_dm_param = temp_reg;
            break;
      }
      GDC_LOG_CMD_3("gdc_cmd_figs: dc[%d] = %x\n", i, temp_reg);
      if ( quit ) {
         return;
      }
   }
}

/*-------------------------------------------------------------------------*/
/* Name: gdc_cmd_mask                                                      */
/* Desc: CMD - Mask Register Load Command                                  */
/*-------------------------------------------------------------------------*/
INLINE void gdc_cmd_mask(void)
{
   UINT8* param_p;
   UINT16 temp_reg;
   TYP_GDC_REGS_DISPLAY* dispregs = &gdc.registers.display;
   
   if ( ( param_p = gdc_fifo_dequeue_param(&gdc)) == NULL ) return;
   temp_reg = *param_p;
   if ( ( param_p = gdc_fifo_dequeue_param(&gdc)) == NULL ) return;
   temp_reg |= (*param_p) << 8;
   dispregs->mask = temp_reg;
   GDC_LOG_CMD_2("gdc_cmd_mask: mask = %04X\n", dispregs->mask);
}

/*-------------------------------------------------------------------------*/
/* Name: gdc_cmd_curs                                                      */
/* Desc: CMD - Cursor Position Specify                                     */
/*-------------------------------------------------------------------------*/
INLINE void gdc_cmd_inner_curs(void)
{
   UINT8* param_p;
   UINT32 temp_reg;
   TYP_GDC_REGS_DISPLAY* dispregs = &gdc.registers.display;
   
   if ( ( param_p = gdc_fifo_dequeue_param(&gdc)) == NULL ) return;
   temp_reg = *param_p;
   dispregs->ead = temp_reg;
   if ( ( param_p = gdc_fifo_dequeue_param(&gdc)) == NULL ) return;
   temp_reg |= (*param_p) << 8;
   dispregs->ead = temp_reg;
   if ( ( param_p = gdc_fifo_dequeue_param(&gdc)) == NULL ) return;
   temp_reg |= (*param_p & 0x03) << 16;
   dispregs->ead = temp_reg;
   /* Get the dot-address */
   dispregs->mask = 1 << ((*param_p >> 4) & 0x0f);

}

INLINE void gdc_cmd_curs( void )
{
   TYP_GDC_REGS_DISPLAY* dispregs = &gdc.registers.display;
   gdc_cmd_inner_curs();
   GDC_LOG_CMD_2("gdc_cmd_curs: ead = %08X, ", dispregs->ead);
   GDC_LOG_CMD_2("mask = %04X\n", dispregs->mask);
}

/*-------------------------------------------------------------------------*/
/* Name: gdc_cmd_wdat                                                      */
/* Desc: CMD - Write Data Command                                          */
/*-------------------------------------------------------------------------*/
INLINE void gdc_cmd_wdat(UINT8 command)
{
   UINT8* param_p;
   UINT16 temp_reg;
   UINT16 i;
   TYP_GDC_REGS_DISPLAY* dispregs = &gdc.registers.display;

   dispregs->cur_mod = command & 0x03;
   
   /* Seems like there can be many parameters to the WDAT and that
      dc is only used for the first set */
   for ( ;; ) { /* Use up all parameters */
      UINT16 orig_mask = gdc.registers.display.mask;
      /* Low byte */
      if ( ( param_p = gdc_fifo_dequeue_param(&gdc)) == NULL ) return;
      
      temp_reg = *param_p;
      
      /* FIXME: Check type of transfer. There can be bytes or words */
      /* One optional high byte */
      switch(command & 0x18) {
         case 0x10:
         case 0x18:
            break;
         default:
            /* Word transfer - first low then high byte */
            if ( ( param_p = gdc_fifo_dequeue_param(&gdc)) != NULL ) {
               temp_reg |= (*param_p) << 8;
            }
            break;
      }
      switch(command & 0x18) {
         case 0x00:
            /* Word transfer */
            break;
         case 0x10:
            /* Low byte only */
            gdc.registers.display.mask &= 0x00ff;
            temp_reg &= 0xff;
            break;
         case 0x18:
            /* FIXME: Bör bara använda en byte i detta fallet */
            /* FIXME: Save the mask */
            gdc.registers.display.mask &= 0xff00; 
            temp_reg = ( (temp_reg  & 0xff ) << 8);
            break;
         default:
            return;
      }
      
      GDC_LOG_CMD_4("gdc_cmd_wdat: data = %04X, dc = %02X, mod = %x",
                    temp_reg, dispregs->figs_dc_param, dispregs->cur_mod);
      GDC_LOG_CMD_3(" mask = %04X, transfer = %02X\n", dispregs->mask,
                    command & 0x18 );

/*        if ( gdc.registers.display.display_mode == 1 ) { */
/*           if ( temp_reg & 0x01 || temp_reg & 0x0100 ) { */
/*              temp_reg = 0xffff; */
/*           } else { */
/*              temp_reg = 0x0000; */
/*           } */
/*        } */
      
      for ( i=0; i < dispregs->figs_dc_param + 1; ++i ) {
         gdc_auto_write_data(temp_reg, 
                             dispregs->cur_mod);
      }
      gdc.registers.display.mask = orig_mask;
      /* Now dc should be zero for the other parameters */
      dispregs->figs_dc_param = 0;
   }
   
}

/*-------------------------------------------------------------------------*/
/* Name: gdc_cmd_rdat                                                      */
/* Desc: CMD - Read Data Command                                           */
/*-------------------------------------------------------------------------*/
INLINE void gdc_cmd_rdat(UINT8 command)
{
   TYP_GDC_REGS_DISPLAY* dispregs = &gdc.registers.display;
   /* FIXME: Check in-parameter too */
   int i;
   for ( i=0; i < (dispregs->figs_dc_param); ++i ) {
      UINT16 data = gdc_auto_read_data();      
      gdc_fifo_add_from_inside(&gdc, data & 0xff, 0);
      gdc_fifo_add_from_inside(&gdc, (data >> 8) & 0xff, 0);
   }
}


/*-------------------------------------------------------------------------*/
/* Name: gdc_cmd_zoom                                                      */
/* Desc: CMD - Zoom Factors Specify Command                                */
/*-------------------------------------------------------------------------*/
INLINE void gdc_cmd_zoom(void)
{
   UINT8* param_p;
   TYP_GDC_REGS_DISPLAY* dispregs = &gdc.registers.display;
   if ( ( param_p = gdc_fifo_dequeue_param(&gdc)) == NULL ) return;
   dispregs->zoom_gchr = (*param_p & 0x0f) + 1;
   dispregs->zoom_disp = ((*param_p & 0xf0) >> 4) + 1;
}


/*-------------------------------------------------------------------------*/
/* Name: gdc_cmd_gchrd                                                     */
/* Desc: CMD - Graphics Char. Draw and Area Fill Start Command             */
/*-------------------------------------------------------------------------*/
INLINE void gdc_cmd_gchrd(void)
{
   int row;
   int bitnbr;
   UINT16 writeWord;
   
   TYP_GDC_REGS_DISPLAY* dispregs = &gdc.registers.display;
   /* This will only work if the character is 8 by 8 and direction is
      2 and it may not even work then */
   UINT16 saved_mask = dispregs->mask;
   UINT32 saved_ead = dispregs->ead;   
   for ( row = 0; row < 8;  ++row ) {
      int zy;
      for ( zy = 0; zy < dispregs->zoom_gchr; ++zy ) {
         /* Calculate word address */
         dispregs->ead = saved_ead -
             (dispregs->zoom_gchr*row+zy) * dispregs->words_per_line;
         GDC_LOG_CMD_2("gdc_cmd_gchrd: pram : %1x\n", gdc.pram[15-row]);
         /* Reset pixel mask (x) */         
         dispregs->mask = saved_mask;
         for ( bitnbr = 0; bitnbr < 8; ++bitnbr ) {
            int zx;            
            for ( zx = 0; zx < dispregs->zoom_gchr; ++zx ) {
               UINT16 bit = (gdc.pram[15-row] >> bitnbr) & 0x01;
               writeWord = bit ? 0xffff : 0x0000;
               writeWord = (dispregs->mask & writeWord) |
                  ((~dispregs->mask) & gdc_mess.vram[dispregs->ead]);
               /* The mode is set by wdat */
               gdc_write_data(writeWord, 0xffff, dispregs->cur_mod);
                              
               if ( dispregs->mask & 0x8000 ) {
                  dispregs->ead++;
               }
               /* GDC_LOG_CMD_2("gdc_cmd_gchrd: mask before = %02X\n", dispregs->mask); */
               dispregs->mask = rotleft16(dispregs->mask);
               /* GDC_LOG_CMD_2("gdc_cmd_gchrd: mask now = %02X\n", dispregs->mask); */
               /* GDC_LOG_CMD_2("gdc_cmd_gchrd: ead now = %04X\n", dispregs->ead); */
            }
         }
      }
   }
   // Back up 8 lines
   dispregs->ead = saved_ead -
                   dispregs->words_per_line * 8 * dispregs->zoom_gchr; 
   dispregs->mask = saved_mask;  
   gdc_mess.dirty = 1;
}


/*-------------------------------------------------------------------------*/
/* Name: gdc_command_processor                                             */
/* Desc: The contents of the FIFO are interpreted by the command processor */
/*       The command bytes are decoded, and the succeeding parameters are  */
/*       distributed to their proper destinations within the GDC.          */
/*-------------------------------------------------------------------------*/
void gdc_command_processor(UINT8 command)
{
	switch(command)
	{
		/* Video control commands */

		case CMD_RESET:
         /* Resets the GDC to its idle mode */
         GDC_LOG_CMD_1("gdc_command: RESET\n");
         gdc_cmd_sync();
         gdc_fifo_reset(&gdc);
         break;
		case CMD_SYNC_OFF:
         /* Specifies the video format and disables the display */
         GDC_LOG_CMD_1("gdc_command: SYNC_OFF\n");
         gdc_cmd_sync();
         break;
		case CMD_SYNC_ON:
         /* Specifies the video format and enables the display */
         GDC_LOG_CMD_1("gdc_command: SYNC_ON\n");
			gdc_cmd_sync();
			break;

		case CMD_VSYNC_SLAVE:	/* Selects slave video synchronization mode */
         GDC_LOG_CMD_1("gdc_command: VSYNC_SLAVE\n");
			break;

		case CMD_VSYNC_MASTER:	/* Selects master video synchronization mode */
         GDC_LOG_CMD_1("gdc_command: VSYNC_MASTER\n");
			break;

		case CMD_CCHAR:		/* Specifies the cursor and character row heights */
         GDC_LOG_CMD_1("gdc_command: CCHAR\n");
         gdc_cmd_cchar();
			break;
         
		/* Display control commands */
		case CMD_START:		/* Ends idle mode and unblanks the display */
         GDC_LOG_CMD_1("gdc_command: START\n");
         gdc_cmd_start();
			break;

		case CMD_BCTRL_OFF:	/* Controls the blanking and unblanking of the display */
         GDC_LOG_CMD_1("gdc_command: BCTRL_OFF\n");
			break;
			
		case CMD_BCTRL_ON:	/* Controls the blanking and unblanking of the display */
         GDC_LOG_CMD_1("gdc_command: BCTRL_ON\n");
			break;
         
			/* Specifies zoom factors for the display and graphics characters writing */
		case CMD_ZOOM:
         GDC_LOG_CMD_1("gdc_command: ZOOM\n");
         gdc_cmd_zoom();
			break;
         
         
		case CMD_CURS:
          /* Sets the position of the cursor in display memory */
         GDC_LOG_CMD_1("gdc_command: CURS\n");
         gdc_cmd_curs();
			break;

		case CMD_PITCH:		
          /* Specifies the width of the X dimension of the display memory */
          GDC_LOG_CMD_1("gdc_command: PITCH\n");
          gdc_cmd_pitch();
          break;
          
		/* Drawing control commands */

		case CMD_MASK:		/* Sets the MASK register contents */
         GDC_LOG_CMD_1("gdc_command: MASK\n");
         gdc_cmd_mask();
			break;
			
		case CMD_FIGS:		/* Specifies the parameters for the drawing processor */
         GDC_LOG_CMD_1("gdc_command: FIGS\n");
         gdc_cmd_figs();
			break;

		case CMD_FIGD:		/* Draws the figure as specified above */
         GDC_LOG_CMD_1("gdc_command: FIGD\n");
			break;

		case CMD_GCHRD:		/* Draws the graphics character into display */
         GDC_LOG_CMD_1("gdc_command: GCHRD\n");
         gdc_cmd_gchrd();
			break;

		/* Memory data read commands */

		case CMD_CURD:		/* Reads the cursor position */
         GDC_LOG_CMD_1("gdc_command: CURD\n");
			break;

		case CMD_LPRD:		/* Reads the light pen address */
         GDC_LOG_CMD_1("gdc_command: LPRD\n");
         gdc_cmd_lprd();
			break;
	
		default:
			
			/* Display control commands */
			if ((command & CMD_PRAM_BITMASK) == CMD_PRAM_BITMASK)
			{
            GDC_LOG_CMD_2("gdc_command: PRAM write at offset %01X\n",
                          command & ~((UINT8)CMD_PRAM_BITMASK));
            gdc_cmd_pram(command & ~((UINT8)CMD_PRAM_BITMASK));
				break;
			}
			else
			{
				switch(command & 0xe4)
				{
					/* Drawing control commands */
	
					case CMD_WDAT_BITMASK:
                  /* Writes data words or bytes into display memory */
                  GDC_LOG_CMD_1("gdc_command: WDAT\n");
                  gdc_cmd_wdat(command);
						break;
					
					/* Memory data read commands */
					
					case CMD_RDAT_BITMASK:	/* Reads data words or bytes from display */
                  GDC_LOG_CMD_1("gdc_command: RDAT\n");
                  gdc_cmd_rdat(command);
						break;
	
					/* Data control commands */
			
					case CMD_DMAR_BITMASK:	/* Requests a DMA read transfer */
                  GDC_LOG_CMD_1("gdc_command: DMAR\n");
						break;
	
					case CMD_DMAW_BITMASK:	/* Requests a DMA write transfer */
                  GDC_LOG_CMD_1("gdc_command: DMAW\n");
						break;
	
					default:
						logerror("GDC Unknown Command %04X\n", command);
						
						break;
				}
			}
	}

   /* Empty the fifo. It could be that we haven't read
      everything correctly */
   if ( (!gdc_fifo_reading(&gdc)) && (! gdc_fifo_empty(&gdc)) ) {
      GDC_LOG_CMD_1("GDC Fifo not empty after processing command!\n");
      while ( gdc_fifo_dequeue_param(&gdc) ) {
         /**/
      }
   }
}


/*-------------------------------------------------------------------------*/
/* Name: gdc_nbr_expected_params                                           */
/* Desc: Returns the number of expected (max) parameters for a comand      */
/* Param: command - the command                                            */
/* Return: Number of expected parameter bytes for the command.             */
/*-------------------------------------------------------------------------*/
int gdc_nbr_expected_params(UINT8 command)
{
	switch(command)
	{
		/* Video control commands */

		case CMD_START:
		case CMD_VSYNC_SLAVE:
		case CMD_VSYNC_MASTER:
      case CMD_BCTRL_OFF:
      case CMD_BCTRL_ON:
      case CMD_FIGD:
      case CMD_GCHRD:
      case CMD_CURD:
      case CMD_LPRD:
         return 0;
      case CMD_PITCH:
      case CMD_ZOOM:
         return 1;
      case CMD_MASK:
         return 2;
		case CMD_CCHAR:
      case CMD_CURS:
         return 3;
		case CMD_RESET:		
		case CMD_SYNC_OFF:	
		case CMD_SYNC_ON:	   
         return 8;
      case CMD_FIGS:
         return 11;
      default:			
         if ((command & CMD_PRAM_BITMASK) == CMD_PRAM_BITMASK) {
            return 16; /* XXX */
         } else {
            switch(command & 0xe4) {
               case CMD_WDAT_BITMASK:
                  return 16;
               case CMD_DMAR_BITMASK:
               case CMD_RDAT_BITMASK:
               case CMD_DMAW_BITMASK:
                  return 0;
					default:
                  GDC_LOG_CMD_2("gdc_nbr_expected_params: cannot find command %X\n",
                                command);
                  return 16;
            }
         }
   }

}


/*-------------------------------------------------------------------------*/
/* Name: compis_gdc_w                                                      */
/* Desc: GDC - 82720 Write handler   GDC   82720                           */
/*-------------------------------------------------------------------------*/
WRITE8_HANDLER ( compis_gdc_w )
{
   /* FIXME: Move to better place. Must be signed and larger than 8 bits */  
   static INT16 last_command = -1;
   /* FIXME: Move to better place */
   static int nbr_expected_params = 17;
   
	switch (offset)
	{
		case 0x00:	/* Parameter into FIFO */
			if (LOG_PORTS)
            logerror("GDC Port %04X (Param) Write %02X\n", offset, data);

         if ( last_command >= 0 ) {
            gdc_fifo_add_from_outside(&gdc, data, 0);
            if ( gdc_fifo_get_nbr_params(&gdc) == nbr_expected_params ) {
               GDC_LOG_CMD_1("GDC: Number of parameters reached\n");
               gdc_command_processor(last_command);
               last_command = -1;
            }
         } else {
            GDC_LOG_CMD_1("GDC Got parameter without command\n");
         }

			break;

		case 0x02:	/* Command into FIFO */
			if (LOG_PORTS)
            logerror("GDC Port %04X (CMD) Write %02X\n", offset, data);
         
         if ( last_command >= 0 ) {
            /* If there is a command in the fifo we can process that one now */
            /* Later we should add a counter for the params too */               
            gdc_command_processor(last_command & 0xff);
         }
         
         /* Check if it is a command without parameters, if so process it now */
         if ( gdc_nbr_expected_params(data) == 0 ) {
            GDC_LOG_CMD_1("GDC: Command takes zero parameters\n");
            gdc_command_processor(data);
            last_command = -1;
         } else {
            /* Save the command */
            last_command = data;
            nbr_expected_params = gdc_nbr_expected_params(data);
         }         
			break;

		default:
			logerror("%04X: GDC UNKNOWN Port Write %04X = %04X\n",
                  activecpu_get_pc(), offset, data);
			break;
	}
}


/*-------------------------------------------------------------------------*/
/* Name: compis_gdc_r                                                      */
/* Desc: GDC - 82720 Read handler                                          */
/*-------------------------------------------------------------------------*/
 READ8_HANDLER (compis_gdc_r)
{
	UINT16 data;
   
	data = 0xff;
	
	switch(offset & 0x0f) {
		case 0x00:	/* Status register */
         
			GDC_LOG_CMD_3("%04X: GDC Port %04X (Status) Read",
                       activecpu_get_pc(), offset);
         /* Optimize this later, i.e. when we know what to return */
			data = ( gdc_fifo_reading(&gdc) && !gdc_fifo_empty(&gdc)) |
            (gdc_fifo_full(&gdc) << 1) |
            (gdc_fifo_empty(&gdc) << 2);
         if ( ! gdc_fifo_reading(&gdc) ) {
            /* Simulate fifo empty */
            data |= 0x04;
         }
			break;

		case 0x02:	/* FIFO Read */
         
			GDC_LOG_CMD_3("%04X: GDC Port %04X (Fifo) Read",
                       activecpu_get_pc(),
                       offset);
         if ( gdc_fifo_reading(&gdc)) {
            UINT8* data_p = gdc_fifo_dequeue(&gdc);
            if ( data_p ) {
               data = *data_p;
            } else {
               /* What to do when the fifo is empty or not in read-mode?*/
               data = 0;
            }
         } else {
            data = 0;
         }
         break;

      default:
         GDC_LOG_CMD_3("%04X: GDC UNKNOWN Port Read %04X",
                          activecpu_get_pc(), offset);
         data = 0x44;
         if ( activecpu_get_pc() == 0xF952D ) {
            data = 0x04;
         } else if ( activecpu_get_pc() == 0xF953B ) {
            data = 0x64;
         }
         /* If we return 0xff zoom will be 2 */
         data = 0xff;
         data = 0x00;
         break;
   }
   GDC_LOG_CMD_2(" returns %1X\n", data);
	return(data);
}

/*-------------------------------------------------------------------------*/
/* Name: compis_gdc_vblank_int                                             */
/* Desc: GDC - Vertical Blanking Interrupt                                 */
/*-------------------------------------------------------------------------*/
void compis_gdc_vblank_int(void)
{
	//gdc.vblank = 1;
}

/*-------------------------------------------------------------------------*/
/* Name: compis_gdc_reset                                                  */
/* Desc: GDC - Reset                                                       */
/*-------------------------------------------------------------------------*/
void compis_gdc_reset(void)
{
	gdc_fifo_reset(&gdc);

}

/*
*  MESS Stuff
*/


static unsigned char COMPIS_palette[16*3] =
{
	0, 0, 0,
	0, 0, 0,
	33, 200, 66,
	94, 220, 120,
	84, 85, 237,
	125, 118, 252,
	212, 82, 77,
	66, 235, 245,
	252, 85, 84,
	255, 121, 120,
	212, 193, 84,
	230, 206, 128,
	33, 176, 59,
	201, 91, 186,
	204, 204, 204,
	255, 255, 255
};

static PALETTE_INIT( compis_gdc )
{
	palette_set_colors(machine, 0, COMPIS_palette, COMPIS_PALETTE_SIZE);
}

static compis_gdc_interface sIntf;

static int compis_gdc_start (const compis_gdc_interface *intf)
{
	/* Only 32KB or 128KB of VRAM */
	switch(intf->vramsize)
	{
		case 0x8000:
		case 0x20000:
			gdc_mess.vramsize = (intf->vramsize)/2;
			break;
		default:
			return 1;
	}

	/* Video mode, HRG or UHRG */
	gdc_mess.mode = intf->mode;

	/* Video RAM */
	gdc_mess.vram = (UINT16*)auto_malloc (gdc_mess.vramsize * sizeof(UINT16) );
	memset (gdc_mess.vram, 0, gdc_mess.vramsize*sizeof(UINT16) );

	/* back bitmap */

	gdc_mess.tmpbmp = auto_bitmap_alloc (640, 400, BITMAP_FORMAT_INDEXED16);
	if (!gdc_mess.tmpbmp ) {
		return 1;
	}
	gdc_fifo_reset(&gdc);
	videoram_size = gdc_mess.vramsize;
	return video_start_generic_bitmapped(Machine);
}

VIDEO_UPDATE(compis_gdc)
{
	int dirty;

	/* TODO - copy only the dirty parts of the memory to the bitmap */
	/* Or plot stuff in write-functions until scrolling occurs */
	dirty = gdc_mess.dirty;
	if (dirty) 
	{
/*      TYP_GDC_REGS_DISPLAY* dispregs = &gdc.registers.display; */
		UINT32 base_addr = gdc_get_base_addr(&gdc);
		/* FIXME: Should be found in the parameters instead */
		UINT32 end_addr = base_addr + gdc_partition_length(&gdc);
			/* dispregs->words_per_line * dispregs->lines_per_field; */
		UINT32 address;
		for( address = base_addr; address < end_addr; ++address )
		{
			gdc_plot_word( (address - gdc_get_base_addr(&gdc)) %
						gdc_mess.vramsize,
						gdc_mess.vram[address % gdc_mess.vramsize]);
		}
		copybitmap(bitmap,
					gdc_mess.tmpbmp, 0, 0, 0, 0,
					NULL, TRANSPARENCY_NONE, 0);
		gdc_mess.dirty = 0;
	}
	return dirty ? 0 : UPDATE_HAS_NOT_CHANGED;
}

VIDEO_START ( compis_gdc )
{
	return compis_gdc_start(&sIntf);
}

void mdrv_compisgdc(machine_config *machine,
                    const compis_gdc_interface *intf)

{
	int scr_width = (intf->mode == GDC_MODE_UHRG) ? 1280 : 640;
	int scr_height = (intf->mode == GDC_MODE_UHRG) ? 800 : 400;
	screen_config *screen = &machine->screen[0];

	sIntf = *intf;

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_UPDATE_BEFORE_VBLANK | VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(scr_width, scr_height)
	MDRV_SCREEN_VISIBLE_AREA(0, scr_width-1, 0, scr_height-1)
	MDRV_PALETTE_LENGTH(COMPIS_PALETTE_SIZE)
	MDRV_COLORTABLE_LENGTH(0)
	MDRV_PALETTE_INIT(compis_gdc)

	MDRV_VIDEO_START(compis_gdc)
	MDRV_VIDEO_UPDATE(compis_gdc)
}

