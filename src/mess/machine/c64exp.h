/**********************************************************************

    Commodore 64 Expansion Port emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************

                    GND      1      A       GND
                    +5V      2      B       _ROMH
                    +5V      3      C       _RESET
                    _IRQ     4      D       _NMI
                    _CR/W    5      E       Sphi2
                    DOTCLK   6      F       CA15
                    _I/O1    7      H       CA14
                    _GAME    8      J       CA13
                    _EXROM   9      K       CA12
                    _I/O2   10      L       CA11
                    _ROML   11      M       CA10
                    BA      12      N       CA9
                    _DMA    13      P       CA8
                    CD7     14      R       CA7
                    CD6     15      S       CA6
                    CD5     16      T       CA5
                    CD4     17      U       CA4
                    CD3     18      V       CA3
                    CD2     19      W       CA2
                    CD1     20      X       CA1
                    CD0     21      Y       CA0
                    GND     22      Z       GND

**********************************************************************/

#pragma once

#ifndef __C64_EXPANSION_SLOT__
#define __C64_EXPANSION_SLOT__

#include "emu.h"



//**************************************************************************
//  CONSTANTS
//**************************************************************************

#define C64_EXPANSION_SLOT_TAG		"exp"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define C64_EXPANSION_INTERFACE(_name) \
	const c64_expansion_slot_interface (_name) =


#define MCFG_C64_EXPANSION_SLOT_ADD(_tag, _config, _slot_intf, _def_slot, _def_inp) \
    MCFG_DEVICE_ADD(_tag, C64_EXPANSION_SLOT, 0) \
    MCFG_DEVICE_CONFIG(_config) \
	MCFG_DEVICE_SLOT_INTERFACE(_slot_intf, _def_slot, _def_inp)



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> c64_expansion_slot_interface

struct c64_expansion_slot_interface
{
    devcb_write_line	m_out_irq_cb;
    devcb_write_line	m_out_nmi_cb;
    devcb_write_line	m_out_dma_cb;
    devcb_write_line	m_out_reset_cb;
};


// ======================> device_c64_expansion_card_interface

// class representing interface-specific live c64_expansion card
class device_c64_expansion_card_interface : public device_slot_card_interface
{
public:
	// construction/destruction
	device_c64_expansion_card_interface(const machine_config &mconfig, device_t &device);
	virtual ~device_c64_expansion_card_interface();

	// memory access
	virtual UINT8 c64_cd_r(offs_t offset, int roml, int romh, int io1, int io2) { return 0; };
	virtual void c64_cd_w(offs_t offset, UINT8 data, int roml, int romh, int io1, int io2) { };

	// memory banking
	virtual int c64_game_r() { return 1; };
	virtual int c64_exrom_r() { return 1; };

	// reset
	virtual void c64_reset_w() { };

	// video
	virtual UINT32 c64_screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect) { return false; }

	virtual UINT8* get_cart_base() { return NULL; }
};


// ======================> c64_expansion_slot_device

class c64_expansion_slot_device : public device_t,
								  public c64_expansion_slot_interface,
								  public device_slot_interface,
								  public device_image_interface
{
public:
	// construction/destruction
	c64_expansion_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	virtual ~c64_expansion_slot_device();

	DECLARE_READ8_MEMBER( roml_r );
	DECLARE_READ8_MEMBER( romh_r );
	DECLARE_READ8_MEMBER( io1_r );
	DECLARE_WRITE8_MEMBER( io1_w );
	DECLARE_READ8_MEMBER( io2_r );
	DECLARE_WRITE8_MEMBER( io2_w );
	DECLARE_READ_LINE_MEMBER( game_r );
	DECLARE_READ_LINE_MEMBER( exrom_r );

	DECLARE_WRITE_LINE_MEMBER( irq_w );
	DECLARE_WRITE_LINE_MEMBER( nmi_w );
	DECLARE_WRITE_LINE_MEMBER( dma_w );
	DECLARE_WRITE_LINE_MEMBER( reset_w );

	UINT32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
	virtual void device_config_complete();

	// image-level overrides
	virtual bool call_load();
	virtual bool call_softlist_load(char *swlist, char *swname, rom_entry *start_entry);

	virtual iodevice_t image_type() const { return IO_CARTSLOT; }

	virtual bool is_readable()  const { return 1; }
	virtual bool is_writeable() const { return 0; }
	virtual bool is_creatable() const { return 0; }
	virtual bool must_be_loaded() const { return 0; }
	virtual bool is_reset_on_load() const { return 1; }
	virtual const char *image_interface() const { return "c64_cart"; }
	virtual const char *file_extensions() const { return "bin,rom,80"; }
	virtual const option_guide *create_option_guide() const { return NULL; }

	// slot interface overrides
	virtual const char * get_default_card_software(const machine_config &config, emu_options &options) const;

	virtual UINT8* get_cart_base();

	devcb_resolved_write_line	m_out_irq_func;
	devcb_resolved_write_line	m_out_nmi_func;
	devcb_resolved_write_line	m_out_dma_func;
	devcb_resolved_write_line	m_out_reset_func;

	device_c64_expansion_card_interface *m_cart;
};


// device type definition
extern const device_type C64_EXPANSION_SLOT;



#endif
