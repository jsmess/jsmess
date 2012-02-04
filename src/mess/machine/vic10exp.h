/**********************************************************************

    Commodore VIC-10 Expansion Port emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************


**********************************************************************/

#pragma once

#ifndef __VIC10_EXPANSION_SLOT__
#define __VIC10_EXPANSION_SLOT__

#include "emu.h"



//**************************************************************************
//  CONSTANTS
//**************************************************************************

#define VIC10_EXPANSION_SLOT_TAG		"exp"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define VIC10_EXPANSION_INTERFACE(_name) \
	const vic10_expansion_slot_interface (_name) =


#define MCFG_VIC10_EXPANSION_SLOT_ADD(_tag, _config, _slot_intf, _def_slot, _def_inp) \
    MCFG_DEVICE_ADD(_tag, VIC10_EXPANSION_SLOT, 0) \
    MCFG_DEVICE_CONFIG(_config) \
	MCFG_DEVICE_SLOT_INTERFACE(_slot_intf, _def_slot, _def_inp)



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> vic10_expansion_slot_interface

struct vic10_expansion_slot_interface
{
    devcb_write_line	m_out_irq_cb;
    devcb_write_line	m_out_sp_cb;
    devcb_write_line	m_out_cnt_cb;
    devcb_write_line	m_out_res_cb;
};


// ======================> vic10_expansion_slot_device

class device_vic10_expansion_card_interface;

class vic10_expansion_slot_device : public device_t,
								    public vic10_expansion_slot_interface,
								    public device_slot_interface,
								    public device_image_interface
{
public:
	// construction/destruction
	vic10_expansion_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	virtual ~vic10_expansion_slot_device();

	DECLARE_READ8_MEMBER( lorom_r );
	DECLARE_WRITE8_MEMBER( lorom_w );
	DECLARE_READ8_MEMBER( uprom_r );
	DECLARE_WRITE8_MEMBER( uprom_w );
	DECLARE_READ8_MEMBER( exram_r );
	DECLARE_WRITE8_MEMBER( exram_w );

	DECLARE_READ_LINE_MEMBER( p0_r );
	DECLARE_WRITE_LINE_MEMBER( p0_w );

	DECLARE_WRITE_LINE_MEMBER( irq_w );
	DECLARE_WRITE_LINE_MEMBER( sp_w );
	DECLARE_WRITE_LINE_MEMBER( cnt_w );
	DECLARE_WRITE_LINE_MEMBER( res_w );

	UINT32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

protected:
	// device-level overrides
	virtual void device_config_complete();
	virtual void device_start();

	// image-level overrides
	virtual bool call_load();
	virtual bool call_softlist_load(char *swlist, char *swname, rom_entry *start_entry);

	virtual iodevice_t image_type() const { return IO_CARTSLOT; }

	virtual bool is_readable()  const { return 1; }
	virtual bool is_writeable() const { return 0; }
	virtual bool is_creatable() const { return 0; }
	virtual bool must_be_loaded() const { return 1; }
	virtual bool is_reset_on_load() const { return 1; }
	virtual const char *image_interface() const { return "vic10_cart"; }
	virtual const char *file_extensions() const { return "80,e0"; }
	virtual const option_guide *create_option_guide() const { return NULL; }

	// slot interface overrides
	virtual const char * get_default_card_software(const machine_config &config, emu_options &options) const;

	devcb_resolved_write_line	m_out_irq_func;
	devcb_resolved_write_line	m_out_sp_func;
	devcb_resolved_write_line	m_out_cnt_func;
	devcb_resolved_write_line	m_out_res_func;

	device_vic10_expansion_card_interface *m_cart;
};


// ======================> device_vic10_expansion_card_interface

// class representing interface-specific live vic10_expansion card
class device_vic10_expansion_card_interface : public device_slot_card_interface
{
public:
	// construction/destruction
	device_vic10_expansion_card_interface(const machine_config &mconfig, device_t &device);
	virtual ~device_vic10_expansion_card_interface();

	// RAM
	virtual UINT8* vic10_exram_pointer() { return NULL; }
	virtual UINT8 vic10_exram_r(address_space &space, offs_t offset) { return 0; };
	virtual void vic10_exram_w(address_space &space, offs_t offset, UINT8 data) { };
	
	// ROM
	virtual UINT8* vic10_lorom_pointer() { return NULL; }
	virtual UINT8 vic10_lorom_r(address_space &space, offs_t offset) { return 0; };
	virtual void vic10_lorom_w(address_space &space, offs_t offset, UINT8 data) { };

	virtual UINT8* vic10_uprom_pointer() { return NULL; }
	virtual UINT8 vic10_uprom_r(address_space &space, offs_t offset) { return 0; };
	virtual void vic10_uprom_w(address_space &space, offs_t offset, UINT8 data) { };

	// I/O
	virtual int vic10_p0_r() { return 0; };
	virtual void vic10_p0_w(int state) { };
	virtual void vic10_sp_w(int state) { };
	virtual void vic10_cnt_w(int state) { };

	// video
	virtual UINT32 vic10_screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect) { return false; }

protected:
	vic10_expansion_slot_device *m_slot;
};


// device type definition
extern const device_type VIC10_EXPANSION_SLOT;



#endif
