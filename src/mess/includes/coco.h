/*****************************************************************************

    includes/coco.h

    CoCo/Dragon

****************************************************************************/

#ifndef __COCO_H__
#define __COCO_H__

#include "devices/snapquik.h"
#include "machine/wd17xx.h"
#include "machine/6883sam.h"
#include "machine/6821pia.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define COCO_CPU_SPEED_HZ		894886	/* 0.894886 MHz */
#define COCO_FRAMES_PER_SECOND	(COCO_CPU_SPEED_HZ / 57.0 / 263)
#define COCO_CPU_SPEED			(ATTOTIME_IN_HZ(COCO_CPU_SPEED_HZ))
#define COCO_TIMER_CMPCARRIER	(COCO_CPU_SPEED * 0.25)


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _coco_state coco_state;
struct _coco_state
{
	const device_config *cococart_device;
	const device_config *cassette_device;
	const device_config *bitbanger_device;
	const device_config *printer_device;
	const device_config *dac;
	const device_config *sam;
	const device_config *pia_0;
	const device_config *pia_1;
	const device_config *pia_2;
};


/***************************************************************************
    PROTOTYPES
***************************************************************************/

/*----------- defined in video/coco.c -----------*/

ATTR_CONST UINT8 coco_get_attributes(running_machine *machine, UINT8 c,int scanline, int pos);

VIDEO_START( dragon );
VIDEO_START( coco );
VIDEO_START( coco2b );


/*----------- defined in video/coco3.c -----------*/

VIDEO_START( coco3 );
VIDEO_START( coco3p );
VIDEO_UPDATE( coco3 );
WRITE8_HANDLER ( coco3_palette_w );
UINT32 coco3_get_video_base(UINT8 ff9d_mask, UINT8 ff9e_mask);
void coco3_vh_blink(void);


/*----------- defined in machine/coco.c -----------*/
extern const wd17xx_interface dgnalpha_wd17xx_interface;
extern const pia6821_interface coco_pia_intf_0;
extern const pia6821_interface coco_pia_intf_1;
extern const pia6821_interface coco2_pia_intf_0;
extern const pia6821_interface coco2_pia_intf_1;
extern const pia6821_interface coco3_pia_intf_0;
extern const pia6821_interface coco3_pia_intf_1;
extern const pia6821_interface dragon32_pia_intf_0;
extern const pia6821_interface dragon32_pia_intf_1;
extern const pia6821_interface dragon64_pia_intf_0;
extern const pia6821_interface dragon64_pia_intf_1;
extern const pia6821_interface dgnalpha_pia_intf_0;
extern const pia6821_interface dgnalpha_pia_intf_1;
extern const pia6821_interface dgnalpha_pia_intf_2;
extern const sam6883_interface coco_sam_intf;
extern const sam6883_interface coco3_sam_intf;
extern UINT8 coco3_gimereg[16];

MACHINE_START( dragon32 );
MACHINE_START( dragon64 );
MACHINE_START( tanodr64 );
MACHINE_START( dgnalpha );
MACHINE_START( coco );
MACHINE_START( coco2 );
MACHINE_START( coco3 );
MACHINE_RESET( coco3 );

DEVICE_IMAGE_LOAD(coco_rom);
DEVICE_IMAGE_UNLOAD(coco_rom);

INPUT_CHANGED(coco_keyboard_changed);
INPUT_CHANGED(coco_joystick_mode_changed);

SNAPSHOT_LOAD ( coco_pak );
SNAPSHOT_LOAD ( coco3_pak );
QUICKLOAD_LOAD ( coco );
READ8_HANDLER ( coco3_mmu_r );
WRITE8_HANDLER ( coco3_mmu_w );
READ8_HANDLER ( coco3_gime_r );
WRITE8_HANDLER ( coco3_gime_w );
offs_t coco3_mmu_translate(running_machine *machine,int bank, int offset);
WRITE8_DEVICE_HANDLER( coco_pia_1_w );
void coco3_horizontal_sync_callback(running_machine *machine,int data);
void coco3_field_sync_callback(running_machine *machine,int data);
void coco3_gime_field_sync_callback(running_machine *machine);

void coco_cart_w(const device_config *device, int data);
void coco3_cart_w(const device_config *device, int data);
void coco_nmi_w(const device_config *device, int data);
void coco_halt_w(const device_config *device, int data);

/* Compusense Dragon Plus board */
READ8_HANDLER ( dgnplus_reg_r );
WRITE8_HANDLER ( dgnplus_reg_w );

READ8_HANDLER( dgnalpha_mapped_irq_r );

/* Dragon Alpha AY-8912 */
READ8_HANDLER ( dgnalpha_psg_porta_read );
WRITE8_HANDLER ( dgnalpha_psg_porta_write );

/* Dragon Alpha WD2797 FDC */
READ8_HANDLER(dgnalpha_wd2797_r);
WRITE8_HANDLER(dgnalpha_wd2797_w);

/* Dragon Alpha Modem, just dummy funcs at the mo */
READ8_HANDLER(dgnalpha_modem_r);
WRITE8_HANDLER(dgnalpha_modem_w);

#ifdef UNUSED_FUNCTION
void coco_set_halt_line(running_machine *machine, int halt_line);
#endif

/* CoCo 3 video vars; controlling key aspects of the emulation */
struct coco3_video_vars
{
	UINT8 bordertop_192;
	UINT8 bordertop_199;
	UINT8 bordertop_0;
	UINT8 bordertop_225;
	unsigned int hs_gime_flip : 1;
	unsigned int fs_gime_flip : 1;
	unsigned int hs_pia_flip : 1;
	unsigned int fs_pia_flip : 1;
	UINT16 rise_scanline;
	UINT16 fall_scanline;
};

extern const struct coco3_video_vars coco3_vidvars;

#endif /* __COCO_H__ */
