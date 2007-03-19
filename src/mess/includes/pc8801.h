/***************************************************************************

  $Id: pc8801.h,v 1.7 2006/02/24 13:07:04 npwoods Exp $

***************************************************************************/

WRITE8_HANDLER(pc8801_write_interrupt_level);
WRITE8_HANDLER(pc8801_write_interrupt_mask);
 READ8_HANDLER(pc88sr_inport_30);
 READ8_HANDLER(pc88sr_inport_31);
 READ8_HANDLER(pc88sr_inport_32);
WRITE8_HANDLER(pc88sr_outport_30);
WRITE8_HANDLER(pc88sr_outport_31);
WRITE8_HANDLER(pc88sr_outport_32);
WRITE8_HANDLER(pc88sr_outport_40);
 READ8_HANDLER(pc88sr_inport_40);
 READ8_HANDLER(pc8801_inport_70);
WRITE8_HANDLER(pc8801_outport_70);
WRITE8_HANDLER(pc8801_outport_78);
 READ8_HANDLER(pc88sr_inport_71);
WRITE8_HANDLER(pc88sr_outport_71);

extern INTERRUPT_GEN( pc8801_interrupt );
extern MACHINE_RESET( pc88srl );
extern MACHINE_RESET( pc88srh );

void pc8801_update_bank(void);
extern unsigned char *pc8801_mainRAM;
extern int pc88sr_is_highspeed;
int pc8801_floppy_id (int id);
int pc8801_floppy_init (int id);
 READ8_HANDLER(pc8801fd_nec765_tc);
void pc88sr_sound_interupt(int irq);
WRITE8_HANDLER(pc8801_write_kanji1);
 READ8_HANDLER(pc8801_read_kanji1);
WRITE8_HANDLER(pc8801_write_kanji2);
 READ8_HANDLER(pc8801_read_kanji2);
WRITE8_HANDLER(pc8801_calender);
 READ8_HANDLER(pc8801_read_extmem);
WRITE8_HANDLER(pc8801_write_extmem);

void pc8801_video_init (int hireso);
int is_pc8801_vram_select(void);
WRITE8_HANDLER(pc8801_vramsel);
 READ8_HANDLER(pc8801_vramtest);
extern unsigned char *pc88sr_textRAM;

extern VIDEO_START( pc8801 );
extern VIDEO_UPDATE( pc8801 );
extern PALETTE_INIT( pc8801 );

extern int pc8801_is_24KHz;
WRITE8_HANDLER(pc8801_crtc_write);
 READ8_HANDLER(pc8801_crtc_read);
WRITE8_HANDLER(pc8801_dmac_write);
 READ8_HANDLER(pc8801_dmac_read);
WRITE8_HANDLER(pc88sr_disp_30);
WRITE8_HANDLER(pc88sr_disp_31);
WRITE8_HANDLER(pc88sr_disp_32);
WRITE8_HANDLER(pc88sr_ALU);
WRITE8_HANDLER(pc8801_palette_out);
