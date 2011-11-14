#pragma once

#ifndef __ISA_EGA_H__
#define __ISA_EGA_H__

#include "emu.h"
#include "machine/isa.h"
#include "video/crtc_ega.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> isa8_ega_device

class isa8_ega_device :
		public device_t,
		public device_isa8_card_interface
{
public:
        // construction/destruction
        isa8_ega_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
		isa8_ega_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock);

		// optional information overrides
		virtual machine_config_constructor device_mconfig_additions() const;
		virtual const rom_entry *device_rom_region() const;

		DECLARE_READ8_MEMBER(pc_ega8_3b0_r);
		DECLARE_WRITE8_MEMBER(pc_ega8_3b0_w);
		DECLARE_READ8_MEMBER(pc_ega8_3c0_r);
		DECLARE_WRITE8_MEMBER(pc_ega8_3c0_w);
		DECLARE_READ8_MEMBER(pc_ega8_3d0_r);
		DECLARE_WRITE8_MEMBER(pc_ega8_3d0_w);
protected:
        // device-level overrides
        virtual void device_start();
        virtual void device_reset();

public:
		device_t *m_crtc_ega;

		void install_banks();
		void change_mode();
		void pc_ega8_3X0_w(UINT8 offset, UINT8 data);
		UINT8 pc_ega8_3X0_r(UINT8 offset);

		crtc_ega_update_row_func	m_update_row;

		/* Video memory and related variables */
		UINT8	*m_videoram;
		UINT8	*m_videoram_a0000;
		UINT8	*m_videoram_b0000;
		UINT8	*m_videoram_b8000;
		UINT8	*m_charA;
		UINT8	*m_charB;

		/* Registers */
		UINT8	m_misc_output;
		UINT8	m_feature_control;

		/* Attribute registers AR00 - AR14
        */
		struct {
			UINT8	index;
			UINT8	data[32];
			UINT8	index_write;
		} m_attribute;

		/* Sequencer registers SR00 - SR04
        */
		struct {
			UINT8	index;
			UINT8	data[8];
		} m_sequencer;

		/* Graphics controller registers GR00 - GR08
        */
		struct {
			UINT8	index;
			UINT8	data[16];
		} m_graphics_controller;

		UINT8	m_frame_cnt;
		UINT8	m_vsync;
		UINT8	m_vblank;
		UINT8	m_display_enable;
};


// device type definition
extern const device_type ISA8_EGA;

#endif  /* __ISA_EGA_H__ */

