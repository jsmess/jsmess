#pragma once

#ifndef __ISA_SOUND_BLASTER_H__
#define __ISA_SOUND_BLASTER_H__

#include "emu.h"
#include "machine/isa.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

struct sb8_dsp_state
{
    UINT8 reset_latch;
    UINT8 rbuf_status;
    UINT8 wbuf_status;
    UINT8 fifo[16],fifo_ptr;
    UINT8 fifo_r[16],fifo_r_ptr;
    UINT16 version;
    UINT8 test_reg;
    UINT8 speaker_on;
    UINT32 prot_count;
    INT32 prot_value;
    UINT32 frequency;
    UINT32 dma_length, dma_transferred;
    UINT8 dma_autoinit;
};

// ======================> sb8_device (parent)

class sb8_device :
		public device_t,
		public device_isa8_card_interface
{
public:
        // construction/destruction
        sb8_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, UINT32 clock, const char *name);

        void process_fifo(UINT8 cmd);
        void queue(UINT8 data);
        void queue_r(UINT8 data);
        UINT8 dequeue_r();

        DECLARE_READ8_MEMBER(dsp_reset_r);
        DECLARE_WRITE8_MEMBER(dsp_reset_w);
        DECLARE_READ8_MEMBER(dsp_data_r);
        DECLARE_WRITE8_MEMBER(dsp_data_w);
        DECLARE_READ8_MEMBER(dsp_rbuf_status_r);
        DECLARE_READ8_MEMBER(dsp_wbuf_status_r);
        DECLARE_WRITE8_MEMBER(dsp_rbuf_status_w);
        DECLARE_WRITE8_MEMBER(dsp_cmd_w);

protected:
        // device-level overrides
        virtual void device_start();
        virtual void device_reset();
		virtual bool have_dack(int line);
        virtual UINT8 dack_r(int line);
        virtual void dack_w(int line, UINT8 data);

        struct sb8_dsp_state m_dsp;
        UINT8 m_dack_out;
private:
};

class isa8_sblaster1_0_device : public sb8_device
{
public:
        // construction/destruction
        isa8_sblaster1_0_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

		// optional information overrides
		virtual machine_config_constructor device_mconfig_additions() const;
protected:
        // device-level overrides
        virtual void device_start();

private:
        // internal state
};

class isa8_sblaster1_5_device : public sb8_device
{
public:
        // construction/destruction
        isa8_sblaster1_5_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

		// optional information overrides
		virtual machine_config_constructor device_mconfig_additions() const;
protected:
        // device-level overrides
        virtual void device_start();

private:
        // internal state
};

// device type definition
extern const device_type ISA8_SOUND_BLASTER_1_0;
extern const device_type ISA8_SOUND_BLASTER_1_5;

#endif  /* __ISA_SOUND_BLASTER_H__ */
