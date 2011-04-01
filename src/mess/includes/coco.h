/*****************************************************************************

    includes/coco.h

    CoCo/Dragon

****************************************************************************/

#ifndef __COCO_H__
#define __COCO_H__

#include "imagedev/snapquik.h"
#include "machine/wd17xx.h"
#include "machine/6883sam.h"
#include "machine/6821pia.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define COCO_CPU_SPEED_HZ		894886	/* 0.894886 MHz */
#define COCO_FRAMES_PER_SECOND	(COCO_CPU_SPEED_HZ / 57.0 / 263)
#define COCO_CPU_SPEED			(attotime::from_hz(COCO_CPU_SPEED_HZ))
#define COCO_TIMER_CMPCARRIER	(COCO_CPU_SPEED * 0.25)


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _coco3_scanline_record coco3_scanline_record;
struct _coco3_scanline_record
{
	UINT8 ff98;
	UINT8 ff99;
	UINT8 ff9a;
	UINT8 index;

	UINT8 palette[16];

	UINT8 data[160];
};

typedef struct _coco3_video coco3_video;
struct _coco3_video
{
	/* Info set up on initialization */
	UINT32 composite_palette[64];
	UINT32 rgb_palette[64];
	UINT8 fontdata[128][8];
	emu_timer *gime_fs_timer;

	/* CoCo 3 palette status */
	UINT8 palette_ram[16];
	UINT32 palette_colors[16];

	/* Incidentals */
	UINT32 legacy_video;
	UINT32 top_border_scanlines;
	UINT32 display_scanlines;
	UINT32 video_position;
	UINT8 line_in_row;
	UINT8 blink;
	UINT8 dirty[2];
	UINT8 video_type;

	/* video state; every scanline the video state for the scanline is copied
     * here and only rendered in SCREEN_UPDATE */
	coco3_scanline_record scanlines[384];
};


class coco_state : public driver_device
{
public:
	coco_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	device_t *m_cococart_device;
	device_t *m_cassette_device;
	device_t *m_bitbanger_device;
	device_t *m_printer_device;
	device_t *m_dac;
	device_t *m_sam;
	device_t *m_pia_0;
	device_t *m_pia_1;
	device_t *m_pia_2;
	UINT8 *m_rom;
	int m_dclg_state;
	int m_dclg_output_h;
	int m_dclg_output_v;
	int m_dclg_timer;
	UINT8 (*update_keyboard)(running_machine &machine);
	emu_timer *m_update_keyboard_timer;
	emu_timer *m_mux_sel1_timer;
	emu_timer *m_mux_sel2_timer;
	UINT8 m_mux_sel1;
	UINT8 m_mux_sel2;
	UINT8 m_bitbanger_output_value;
	UINT8 m_bitbanger_input_value;
	UINT8 m_dac_value;
	int m_dgnalpha_just_reset;
	UINT8 *m_bas_rom_bank;
	UINT8 *m_bottom_32k;
	timer_expired_func m_recalc_interrupts;
	int m_hiresjoy_ca;
	attotime m_hiresjoy_xtransitiontime;
	attotime m_hiresjoy_ytransitiontime;
	int m_dclg_previous_bit;
	void (*printer_out)(running_machine &machine, int data);
};

class coco3_state : public coco_state
{
public:
	coco3_state(running_machine &machine, const driver_device_config_base &config)
		: coco_state(machine, config) { }

	int m_enable_64k;
	UINT8 m_mmu[16];
	UINT8 m_interupt_line;
	emu_timer *m_gime_timer;
	UINT8 m_gimereg[16];
	UINT8 m_gime_firq;
	UINT8 m_gime_irq;
	coco3_video *m_video;
};


/***************************************************************************
    PROTOTYPES
***************************************************************************/

/*----------- defined in video/coco.c -----------*/

ATTR_CONST UINT8 coco_get_attributes(running_machine &machine, UINT8 c,int scanline, int pos);

VIDEO_START( dragon );
VIDEO_START( coco );
VIDEO_START( coco2b );


/*----------- defined in video/coco3.c -----------*/

VIDEO_START( coco3 );
VIDEO_START( coco3p );
SCREEN_UPDATE( coco3 );
WRITE8_HANDLER ( coco3_palette_w );
UINT32 coco3_get_video_base(running_machine &machine, UINT8 ff9d_mask, UINT8 ff9e_mask);
void coco3_vh_blink(running_machine &machine);


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

MACHINE_START( dragon32 );
MACHINE_START( dragon64 );
MACHINE_START( tanodr64 );
MACHINE_START( dgnalpha );
MACHINE_RESET( dgnalpha );
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
offs_t coco3_mmu_translate(running_machine &machine,int bank, int offset);
WRITE8_DEVICE_HANDLER( coco_pia_1_w );
void coco3_horizontal_sync_callback(running_machine &machine,int data);
void coco3_field_sync_callback(running_machine &machine,int data);
void coco3_gime_field_sync_callback(running_machine &machine);

void coco_cart_w(device_t *device, int data);
void coco3_cart_w(device_t *device, int data);
void coco_nmi_w(device_t *device, int data);
void coco_halt_w(device_t *device, int data);

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
void coco_set_halt_line(running_machine &machine, int halt_line);
#endif

/* CoCo 3 video vars; controlling key aspects of the emulation */
struct coco3_video_vars
{
	UINT8 m_bordertop_192;
	UINT8 m_bordertop_199;
	UINT8 m_bordertop_0;
	UINT8 m_bordertop_225;
	unsigned int hs_gime_flip : 1;
	unsigned int fs_gime_flip : 1;
	unsigned int hs_pia_flip : 1;
	unsigned int fs_pia_flip : 1;
	UINT16 m_rise_scanline;
	UINT16 m_fall_scanline;
};

extern const struct coco3_video_vars coco3_vidvars;

/* Setting it bitbanger bit */
void coco_bitbanger_callback(running_machine &machine, UINT8 bit);
void coco3_bitbanger_callback(running_machine &machine, UINT8 bit);

#endif /* __COCO_H__ */
