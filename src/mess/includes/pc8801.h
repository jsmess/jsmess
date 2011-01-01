/*****************************************************************************
 *
 * includes/pc8801.h
 *
 ****************************************************************************/

#ifndef __PC8801__
#define __PC8801__

#define CPU_I8255A_TAG	"ppi8255_0"
#define FDC_I8255A_TAG	"ppi8255_1"
#define UPD765_TAG		"upd765"
#define UPD1990A_TAG	"upd1990a"
#define YM2203_TAG		"ym2203"
#define CASSETTE_TAG	"cassette"
#define CENTRONICS_TAG	"centronics"
#define SCREEN_TAG		"screen"

#include "machine/upd765.h"
#include "machine/i8255a.h"

class pc88_state : public driver_device
{
public:
	pc88_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* floppy state */
	UINT8 i8255_0_pc;
	UINT8 i8255_1_pc;

	/* memory state */
	UINT16 kanji;
	UINT16 kanji2;

	device_t *upd765;
	device_t *upd1990a;
	device_t *cassette;
	device_t *centronics;
	unsigned char *mainRAM;
	int pc88sr_is_highspeed;
	unsigned char *pc88sr_textRAM;
	int is_24KHz;
	int ROMmode;
	int RAMmode;
	int maptvram;
	int no4throm;
	int no4throm2;
	int port71_save;
	unsigned char *mainROM;
	int is_V2mode;
	int is_Nbasic;
	int is_8MHz;
	int port32_save;
	int text_window;
	int extmem_mode;
	UINT8 *extRAM;
	int extRAM_size;
	void *ext_bank_80[4];
	void *ext_bank_88[256];
	UINT8 extmem_ctrl[2];
	int use_5FD;
	int enable_FM_IRQ;
	int FM_IRQ_save;
	int interrupt_level_reg;
	int interrupt_mask_reg;
	int interrupt_trig_reg;
	unsigned char *gVRAM;
	int selected_vram;
	int crtc_on;
	int text_on;
	int text_width;
	int text_height;
	int text_invert;
	int text_color;
	int text_cursor;
	int text_cursorX;
	int text_cursorY;
	int blink_period;
	int analog_palette;
	int alu_1;
	int alu_2;
	int alu_on;
	unsigned char alu_save0;
	unsigned char alu_save1;
	unsigned char alu_save2;
	int dmac_fl;
	UINT16 dmac_addr[3];
	UINT16 dmac_size[3];
	UINT8 dmac_flag;
	UINT8 dmac_status;
	int cursor_mode;
	int crtc_state;
	int gmode;
	int disp_plane[3];
	char *graph_dirty;
	unsigned short *attr_tmp;
	unsigned short *attr_old;
	unsigned short *text_old;
	bitmap_t *wbm1;
	bitmap_t *wbm2;
	int blink_count;
	struct { UINT8 r,g,b; } pal[10];
};

/*----------- defined in machine/pc8801.c -----------*/

extern const i8255a_interface pc8801_8255_config_0;
extern const i8255a_interface pc8801_8255_config_1;
extern const upd765_interface pc8801_fdc_interface;

WRITE8_HANDLER(pc8801_write_interrupt_level);
WRITE8_HANDLER(pc8801_write_interrupt_mask);
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

INTERRUPT_GEN( pc8801_interrupt );
MACHINE_START( pc88srl );
MACHINE_START( pc88srh );
MACHINE_RESET( pc88srl );
MACHINE_RESET( pc88srh );

void pc8801_update_bank(running_machine *machine);
READ8_HANDLER(pc8801fd_upd765_tc);
void pc88sr_sound_interupt(device_t *device, int irq);
WRITE8_HANDLER(pc88_kanji_w);
READ8_HANDLER(pc88_kanji_r);
WRITE8_HANDLER(pc88_kanji2_w);
READ8_HANDLER(pc88_kanji2_r);
WRITE8_HANDLER(pc88_rtc_w);
READ8_HANDLER(pc88_extmem_r);
WRITE8_HANDLER(pc88_extmem_w);

/*----------- defined in video/pc8801.c -----------*/

void pc8801_video_init (running_machine *machine, int hireso);
int pc8801_is_vram_select(running_machine *machine);
WRITE8_HANDLER(pc88_vramsel_w);
 READ8_HANDLER(pc88_vramtest_r);
VIDEO_START( pc8801 );
VIDEO_UPDATE( pc8801 );
PALETTE_INIT( pc8801 );

WRITE8_HANDLER(pc88_crtc_w);
 READ8_HANDLER(pc88_crtc_r);
WRITE8_HANDLER(pc88_dmac_w);
 READ8_HANDLER(pc88_dmac_r);
WRITE8_HANDLER(pc88sr_disp_30);
WRITE8_HANDLER(pc88sr_disp_31);
WRITE8_HANDLER(pc88sr_disp_32);
WRITE8_HANDLER(pc88sr_alu);
WRITE8_HANDLER(pc88_palette_w);

#endif /* PC8801_H_ */
