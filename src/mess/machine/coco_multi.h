#pragma once

#ifndef __COCO_MULTI_H__
#define __COCO_MULTI_H__

#include "emu.h"
#include "machine/cococart.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> coco_multipak_device

class coco_multipak_device :
		public device_t,
		public device_cococart_interface
{
public:
		// construction/destruction
        coco_multipak_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

		// optional information overrides
		virtual machine_config_constructor device_mconfig_additions() const;

		virtual UINT8* get_cart_base();
protected:
        // device-level overrides
        virtual void device_start();
		virtual DECLARE_READ8_MEMBER(read);
		virtual DECLARE_WRITE8_MEMBER(write);

        // internal state
		cococart_slot_device *m_owner;

		cococart_slot_device *m_slot1;
		cococart_slot_device *m_slot2;
		cococart_slot_device *m_slot3;
		cococart_slot_device *m_slot4;

		cococart_slot_device *m_active;
};


// device type definition
extern const device_type COCO_MULTIPAK;

#endif  /* __COCO_MULTI_H__ */
