#pragma once

#ifndef __COCO_FDC_H__
#define __COCO_FDC_H__

#include "emu.h"
#include "machine/cococart.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> coco_fdc_device

class coco_fdc_device :
		public device_t,
		public device_cococart_interface,
		public device_slot_card_interface
{
public:
		// construction/destruction
        coco_fdc_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
		coco_fdc_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock);
		
		// optional information overrides
		virtual machine_config_constructor device_mconfig_additions() const;
		virtual const rom_entry *device_rom_region() const;
		
		virtual UINT8* get_cart_base();
protected:
        // device-level overrides
        virtual void device_start();
		virtual void device_config_complete();

        // internal state
		cococart_slot_device *m_owner;
		astring m_region_name;
};


// device type definition
extern const device_type COCO_FDC;

// ======================> coco_fdc_v11_device

class coco_fdc_v11_device :
		public coco_fdc_device
{
public:
		// construction/destruction
        coco_fdc_v11_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

		// optional information overrides
		virtual const rom_entry *device_rom_region() const;
protected:
        // device-level overrides
		virtual void device_config_complete();
};


// device type definition
extern const device_type COCO_FDC_V11;

// ======================> cp400_fdc_device

class cp400_fdc_device :
		public coco_fdc_device
{
public:
		// construction/destruction
        cp400_fdc_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

		// optional information overrides
		virtual const rom_entry *device_rom_region() const;
protected:
        // device-level overrides
		virtual void device_config_complete();
};


// device type definition
extern const device_type CP400_FDC;

// ======================> dragon_fdc_device

class dragon_fdc_device :
		public coco_fdc_device
{
public:
		// construction/destruction
        dragon_fdc_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
		dragon_fdc_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock);

		// optional information overrides
		virtual machine_config_constructor device_mconfig_additions() const;
		virtual const rom_entry *device_rom_region() const;
protected:
        // device-level overrides
		virtual void device_config_complete();
private:		
};


// device type definition
extern const device_type DRAGON_FDC;

// ======================> sdtandy_fdc_device

class sdtandy_fdc_device :
		public dragon_fdc_device
{
public:
		// construction/destruction
        sdtandy_fdc_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

		// optional information overrides
		virtual const rom_entry *device_rom_region() const;
protected:
        // device-level overrides
		virtual void device_config_complete();
};


// device type definition
extern const device_type SDTANDY_FDC;

#endif  /* __COCO_FDC_H__ */
