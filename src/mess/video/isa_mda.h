#pragma once

#ifndef __ISA_MDA_H__
#define __ISA_MDA_H__

#include "emu.h"
#include "machine/isa.h"
#include "video/mc6845.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> isa8_mda_device

class isa8_mda_device :
		public device_t,
		public device_isa8_card_interface
{
public:
		friend class isa8_hercules_device;

        // construction/destruction
        isa8_mda_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
		isa8_mda_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock);

		// optional information overrides
		virtual machine_config_constructor device_mconfig_additions() const;
		virtual const rom_entry *device_rom_region() const;

		DECLARE_WRITE_LINE_MEMBER(hsync_changed);
		DECLARE_WRITE_LINE_MEMBER(vsync_changed);
		DECLARE_READ8_MEMBER(io_read);
		DECLARE_WRITE8_MEMBER(io_write);
		DECLARE_READ8_MEMBER(status_r);
		DECLARE_WRITE8_MEMBER(mode_control_w);

protected:
        // device-level overrides
        virtual void device_start();
        virtual void device_reset();
public:
		int  framecnt;

		UINT8 mode_control;

		mc6845_update_row_func  update_row;
		UINT8   *chr_gen;
		UINT8   vsync;
		UINT8   hsync;
		UINT8  *videoram;
};


// device type definition
extern const device_type ISA8_MDA;

// ======================> isa8_hercules_device

class isa8_hercules_device :
		public isa8_mda_device
{
public:
        // construction/destruction
        isa8_hercules_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
		// optional information overrides
		virtual machine_config_constructor device_mconfig_additions() const;
		virtual const rom_entry *device_rom_region() const;

protected:
        // device-level overrides
        virtual void device_start();
        virtual void device_reset();

private:
        // internal state
public:
		UINT8 configuration_switch; //hercules
};


// device type definition
extern const device_type ISA8_HERCULES;

#endif  /* __ISA_MDA_H__ */
