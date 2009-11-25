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

typedef struct _pc88_state pc88_state;
struct _pc88_state
{
	/* floppy state */
	UINT8 i8255_0_pc;
	UINT8 i8255_1_pc;

	/* memory state */
	UINT16 kanji;
	UINT16 kanji2;

	const device_config *upd765;
	const device_config *upd1990a;
	const device_config *cassette;
	const device_config *centronics;
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

extern INTERRUPT_GEN( pc8801_interrupt );
extern MACHINE_START( pc88srl );
extern MACHINE_START( pc88srh );
extern MACHINE_RESET( pc88srl );
extern MACHINE_RESET( pc88srh );

void pc8801_update_bank(running_machine *machine);
extern unsigned char *pc8801_mainRAM;
extern int pc88sr_is_highspeed;
READ8_HANDLER(pc8801fd_upd765_tc);
void pc88sr_sound_interupt(const device_config *device, int irq);
WRITE8_HANDLER(pc88_kanji_w);
READ8_HANDLER(pc88_kanji_r);
WRITE8_HANDLER(pc88_kanji2_w);
READ8_HANDLER(pc88_kanji2_r);
WRITE8_HANDLER(pc88_rtc_w);
READ8_HANDLER(pc88_extmem_r);
WRITE8_HANDLER(pc88_extmem_w);

/*----------- defined in video/pc8801.c -----------*/

void pc8801_video_init (running_machine *machine, int hireso);
int is_pc8801_vram_select(running_machine *machine);
WRITE8_HANDLER(pc88_vramsel_w);
 READ8_HANDLER(pc88_vramtest_r);
extern unsigned char *pc88sr_textRAM;

extern VIDEO_START( pc8801 );
extern VIDEO_UPDATE( pc8801 );
extern PALETTE_INIT( pc8801 );

extern int pc8801_is_24KHz;
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
