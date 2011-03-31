#pragma once

#ifndef __CGC7900__
#define __CGC7900__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "cpu/mcs48/mcs48.h"
#include "formats/basicdsk.h"
#include "machine/ram.h"
#include "machine/msm8251.h"
#include "sound/ay8910.h"

#define M68000_TAG		"uh8"
#define INS8251_0_TAG	"uc10"
#define INS8251_1_TAG	"uc8"
#define MM58167_TAG		"uc6"
#define AY8910_TAG		"uc4"
#define K1135A_TAG		"uc11"
#define I8035_TAG		"i8035"
#define AM2910_TAG		"11d"
#define SCREEN_TAG		"screen"

class cgc7900_state : public driver_device
{
public:
	cgc7900_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, M68000_TAG)
	{ }

	required_device<cpu_device> m_maincpu;

	virtual void machine_start();
	virtual void machine_reset();

	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	DECLARE_READ16_MEMBER( keyboard_r );
	DECLARE_WRITE16_MEMBER( keyboard_w );
	DECLARE_WRITE16_MEMBER( interrupt_mask_w );
	DECLARE_READ16_MEMBER( disk_data_r );
	DECLARE_WRITE16_MEMBER( disk_data_w );
	DECLARE_READ16_MEMBER( disk_status_r );
	DECLARE_WRITE16_MEMBER( disk_command_w );
	DECLARE_READ16_MEMBER( z_mode_r );
	DECLARE_WRITE16_MEMBER( z_mode_w );
	DECLARE_WRITE16_MEMBER( color_status_w );
	DECLARE_READ16_MEMBER( sync_r );
	
	void update_clut();
	void draw_bitmap(screen_device *screen, bitmap_t *bitmap);
	void draw_overlay(screen_device *screen, bitmap_t *bitmap);
		
	/* interrupt state */
	UINT16 m_int_mask;

	/* video state */
	UINT16 *m_plane_ram;
	UINT16 *m_clut_ram;
	UINT16 *m_overlay_ram;
	UINT8 *m_char_rom;
	UINT16 m_roll_bitmap;
	UINT16 m_pan_x;
	UINT16 m_pan_y;
	UINT16 m_zoom;
	UINT16 m_blink_select;
	UINT16 m_plane_select;
	UINT16 m_plane_switch;
	UINT16 m_color_status_fg;
	UINT16 m_color_status_bg;
	UINT16 m_roll_overlay;
	int m_blink;
	UINT16* m_chrom_ram;
};

/*----------- defined in video/cgc7900.c -----------*/

MACHINE_CONFIG_EXTERN( cgc7900_video );

#endif
