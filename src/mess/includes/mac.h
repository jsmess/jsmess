/*****************************************************************************
 *
 * includes/mac.h
 *
 * Macintosh driver declarations
 *
 ****************************************************************************/

#ifndef MAC_H_
#define MAC_H_

#include "machine/8530scc.h"
#include "machine/6522via.h"

/* tells which model is being emulated (set by macxxx_init) */
typedef enum
{
	MODEL_MAC_128K512K,	// 68000 machines
	MODEL_MAC_512KE,
	MODEL_MAC_PLUS,
	MODEL_MAC_SE,
	MODEL_MAC_CLASSIC,
	MODEL_MAC_PORTABLE,
	MODEL_MAC_PB100,

	MODEL_MAC_II,		// Mac II class 68020/030 machines
	MODEL_MAC_II_FDHD,
	MODEL_MAC_IIX,
	MODEL_MAC_IICX,
	MODEL_MAC_IICI,
	MODEL_MAC_IISI,
	MODEL_MAC_IIFX,
	MODEL_MAC_SE30,

	MODEL_MAC_LC,		// LC class 68030 machines, generally using a V8 or compatible gate array
	MODEL_MAC_LC_II,
	MODEL_MAC_LC_III,
	MODEL_MAC_CLASSIC_II,
	MODEL_MAC_TV,
	MODEL_MAC_COLOR_CLASSIC,

	MODEL_MAC_PB140,	// 68030 PowerBooks
	MODEL_MAC_PB170,
	MODEL_MAC_PB145,
	MODEL_MAC_PB160,
	MODEL_MAC_PB180,
	MODEL_MAC_PB165c,
	MODEL_MAC_PB180c,
	MODEL_MAC_PB145b,
	MODEL_MAC_PB165,
	MODEL_MAC_PB150,

	MODEL_MAC_PBDUO_210,	// 68030 PowerBook Duos
	MODEL_MAC_PBDUO_230,
	MODEL_MAC_PBDUO_250,
	MODEL_MAC_PBDUO_270c,

	MODEL_MAC_QUADRA_700,	// 68(LC)040 desktops
	MODEL_MAC_QUADRA_900,
	MODEL_MAC_QUADRA_950,
	MODEL_MAC_QUADRA_660AV,
	MODEL_MAC_QUADRA_840AV,
	MODEL_MAC_QUADRA_605,
	MODEL_MAC_QUADRA_610,
	MODEL_MAC_QUADRA_630,
	MODEL_MAC_QUADRA_650,

	MODEL_MAC_PB550c,	// 68(LC)040 PowerBooks
	MODEL_MAC_PB520,
	MODEL_MAC_PB520c,
	MODEL_MAC_PB540,
	MODEL_MAC_PB540c,
	MODEL_MAC_PB190,
	MODEL_MAC_PB190cs,

	MODEL_MAC_POWERMAC_6100	// original PowerMac
} mac_model_t;

// video parameters for classic Macs
#define MAC_H_VIS	(512)
#define MAC_V_VIS	(342)
#define MAC_H_TOTAL	(704)		// (512+192)
#define MAC_V_TOTAL	(370)		// (342+28)

/*----------- defined in machine/mac.c -----------*/

extern mac_model_t mac_model;
extern const via6522_interface mac_via6522_intf;
extern const via6522_interface mac_via6522_2_intf;
extern const via6522_interface mac_via6522_adb_intf;

MACHINE_START( macscsi );
MACHINE_START( mac );
MACHINE_RESET( mac );

DRIVER_INIT(mac128k512k);
DRIVER_INIT(mac512ke);
DRIVER_INIT(macplus);
DRIVER_INIT(macse);
DRIVER_INIT(macclassic);
DRIVER_INIT(maclc);
DRIVER_INIT(macii);
DRIVER_INIT(maciifdhd);
DRIVER_INIT(maciix);
DRIVER_INIT(maciicx);
DRIVER_INIT(maciici);
DRIVER_INIT(maciisi);
DRIVER_INIT(macse30);
DRIVER_INIT(macclassic2);
DRIVER_INIT(maclc2);
DRIVER_INIT(maclc3);
DRIVER_INIT(macpm6100);
DRIVER_INIT(macprtb);
DRIVER_INIT(macpb100);

READ16_HANDLER ( mac_via_r );
WRITE16_HANDLER ( mac_via_w );
READ16_HANDLER ( mac_via2_r );
WRITE16_HANDLER ( mac_via2_w );
READ16_HANDLER ( mac_autovector_r );
WRITE16_HANDLER ( mac_autovector_w );
READ16_HANDLER ( mac_iwm_r );
WRITE16_HANDLER ( mac_iwm_w );
READ16_HANDLER ( mac_scc_r );
WRITE16_HANDLER ( mac_scc_w );
WRITE16_HANDLER ( mac_scc_2_w );
READ16_HANDLER ( macplus_scsi_r );
WRITE16_HANDLER ( macplus_scsi_w );
WRITE16_HANDLER ( macii_scsi_w );
READ32_HANDLER (macii_scsi_drq_r);
WRITE32_HANDLER (macii_scsi_drq_w);
NVRAM_HANDLER( mac );
void mac_scc_irq(running_device *device, int status);
void mac_scc_mouse_irq( running_machine *machine, int x, int y );
void mac_fdc_set_enable_lines(running_device *device, int enable_mask);

void mac_nubus_slot_interrupt(running_machine *machine, UINT8 slot, UINT32 state);

/*----------- defined in video/mac.c -----------*/

extern UINT32 *mac_se30_vram;

VIDEO_START( mac );
VIDEO_UPDATE( mac );
VIDEO_UPDATE( macse30 );
PALETTE_INIT( mac );

void mac_set_screen_buffer( int buffer );

extern UINT32 *mac_cb264_vram;
VIDEO_START( mac_cb264 );
VIDEO_UPDATE( mac_cb264 );
READ32_HANDLER( mac_cb264_r );
WRITE32_HANDLER( mac_cb264_w );
WRITE32_HANDLER( mac_cb264_ramdac_w );
INTERRUPT_GEN( mac_cb264_vbl );

/*----------- defined in audio/mac.c -----------*/

#define SOUND_MAC_SOUND DEVICE_GET_INFO_NAME(mac_sound)

DEVICE_GET_INFO( mac_sound );

void mac_enable_sound( running_device *device, int on );
void mac_set_sound_buffer( running_device *device, int buffer );
void mac_set_volume( running_device *device, int volume );

void mac_sh_updatebuffer(running_device *device);

/* Mac driver data */

typedef struct
{
	mac_model_t mac_model;

	UINT32 mac_overlay;
	int mac_drive_select;
	int mac_scsiirq_enable;

	UINT32 mac_via2_vbl;
	UINT32 mac_se30_vbl_enable;
	UINT8 mac_nubus_irq_state;
	UINT8 via2_ca1;

	/* used to store the reply to most keyboard commands */
	int keyboard_reply;

	/* Keyboard communication in progress? */
	int kbd_comm;
	int kbd_receive;
	/* timer which is used to time out inquiry */
	emu_timer *inquiry_timeout;

	int kbd_shift_reg;
	int kbd_shift_count;

	/* keyboard matrix to detect transition */
	int key_matrix[7];

	/* keycode buffer (used for keypad/arrow key transition) */
	int keycode_buf[2];
	int keycode_buf_index;

	int mouse_bit_x;
	int mouse_bit_y;


	/* state of rTCEnb and rTCClk lines */
	UINT8 rtc_rTCEnb;
	UINT8 rtc_rTCClk;

	/* serial transmit/receive register : bits are shifted in/out of this byte */
	UINT8 rtc_data_byte;
	/* serial transmitted/received bit count */
	UINT8 rtc_bit_count;
	/* direction of the current transfer (0 : VIA->RTC, 1 : RTC->VIA) */
	UINT8 rtc_data_dir;
	/* when rtc_data_dir == 1, state of rTCData as set by RTC (-> data bit seen by VIA) */
	UINT8 rtc_data_out;

	/* set to 1 when command in progress */
	UINT8 rtc_cmd;

	/* write protect flag */
	UINT8 rtc_write_protect;

	/* internal seconds register */
	UINT8 rtc_seconds[/*8*/4];
	/* 20-byte long PRAM, or 256-byte long XPRAM */
	UINT8 rtc_ram[256];
	/* current extended address and RTC state */
	UINT8 rtc_xpaddr;
	UINT8 rtc_state;

	// Mac ADB state
	INT32 adb_irq_pending, adb_waiting_cmd, adb_datasize, adb_buffer[257];
	INT32 adb_state, adb_command, adb_send, adb_timer_ticks, adb_extclock, adb_direction;
	INT32 adb_listenreg, adb_listenaddr, adb_last_talk, adb_srq_switch;

	// Portable/PB100 Power Manager IC comms (chapter 4, "Guide to the Macintosh Family Hardware", second edition)
	UINT8 pm_data_send, pm_data_recv, pm_ack, pm_req;

	// Apple Sound Chip
	UINT8 mac_asc_regs[0x2000];
	INT16 xfersamples[0x800];
} mac_state;

#endif /* MAC_H_ */


