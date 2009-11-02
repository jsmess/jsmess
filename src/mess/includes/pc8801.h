/*****************************************************************************
 *
 * includes/pc8801.h
 *
 ****************************************************************************/

#ifndef __PC8801__
#define __PC8801__

#define UPD1990A_TAG	"upd1990a"

#include "machine/upd765.h"
#include "machine/i8255a.h"

typedef struct _pc88_state pc88_state;
struct _pc88_state
{
	/* RTC state */
	int rtc_data;

	const device_config *upd1990a;
};

/*----------- defined in machine/pc8801.c -----------*/

extern const i8255a_interface pc8801_8255_config_0;
extern const i8255a_interface pc8801_8255_config_1;
extern const upd765_interface pc8801_fdc_interface;

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
extern MACHINE_START( pc88srl );
extern MACHINE_START( pc88srh );
extern MACHINE_RESET( pc88srl );
extern MACHINE_RESET( pc88srh );

void pc8801_update_bank(running_machine *machine);
extern unsigned char *pc8801_mainRAM;
extern int pc88sr_is_highspeed;
READ8_HANDLER(pc8801fd_upd765_tc);
void pc88sr_sound_interupt(const device_config *device, int irq);
WRITE8_HANDLER(pc8801_write_kanji1);
READ8_HANDLER(pc8801_read_kanji1);
WRITE8_HANDLER(pc8801_write_kanji2);
READ8_HANDLER(pc8801_read_kanji2);
WRITE8_HANDLER(pc8801_calender);
READ8_HANDLER(pc8801_read_extmem);
WRITE8_HANDLER(pc8801_write_extmem);

/*----------- defined in video/pc8801.c -----------*/

void pc8801_video_init (running_machine *machine, int hireso);
int is_pc8801_vram_select(running_machine *machine);
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
WRITE8_HANDLER(pc88sr_alu);
WRITE8_HANDLER(pc8801_palette_out);

#endif /* PC8801_H_ */
