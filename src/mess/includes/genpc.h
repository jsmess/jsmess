/*****************************************************************************
 *
 * includes/pc.h
 *
 ****************************************************************************/

#ifndef GENPC_H_
#define GENPC_H_

#include "machine/ins8250.h"
#include "machine/i8255a.h"
#include "machine/8237dma.h"
#include "machine/isa.h"

#define MCFG_PC_MOTHERBOARD_ADD(_tag, _cputag) \
    MCFG_DEVICE_ADD(_tag, PC_MOTHERBOARD, 0) \
    pc_motherboard_device_config::static_set_cputag(device, _cputag); \
 	
// ======================> pc_motherboard_device_config
 
class pc_motherboard_device_config : public device_config
{
	friend class pc_motherboard_device;
 
	// construction/destruction
	pc_motherboard_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
 
public:
	// allocators
	static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
	virtual device_t *alloc_device(running_machine &machine) const;
 
	// inline configuration
	static void static_set_cputag(device_config *device, const char *tag);
 
	// optional information overrides
	virtual machine_config_constructor machine_config_additions() const;
	virtual const input_port_token *input_ports() const;
	
	const char *m_cputag;
};
 
 
// ======================> pc_motherboard_device
class pc_motherboard_device : public device_t
{
        friend class pc_motherboard_device_config;
 
        // construction/destruction
        pc_motherboard_device(running_machine &_machine, const pc_motherboard_device_config &config);

protected:
        // device-level overrides
        virtual void device_start();
        virtual void device_reset();
		void install_device(device_t *dev, offs_t start, offs_t end, offs_t mask, offs_t mirror, read8_device_func rhandler, write8_device_func whandler);
private:
        // internal state
        const pc_motherboard_device_config &m_config;
public:
        required_device<cpu_device>  maincpu;
		required_device<device_t>  pic8259;
		required_device<device_t>  dma8237;
		required_device<device_t>  pit8253;
		required_device<device_t>  ppi8255;
		required_device<device_t>  speaker;
		required_device<isa8_device>  isabus;
		/* U73 is an LS74 - dual flip flop */
		/* Q2 is set by OUT1 from the 8253 and goes to DRQ1 on the 8237 */
		UINT8	u73_q2;
		UINT8	out1;
		int dma_channel;
		UINT8 dma_offset[4];
		UINT8 pc_spkrdata;
		UINT8 pc_input;

		int						ppi_portc_switch_high;
		int						ppi_speaker;
		int						ppi_keyboard_clear;
		UINT8					ppi_keyb_clock;
		UINT8					ppi_portb;
		UINT8					ppi_clock_signal;
		UINT8					ppi_data_signal;
		UINT8					ppi_shift_register;
		UINT8					ppi_shift_enable;
		
		static IRQ_CALLBACK(pc_irq_callback);
};
 
 
// device type definition
extern const device_type PC_MOTHERBOARD;



class genpc_state : public driver_device
{
public:
	genpc_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};

/*----------- defined in machine/genpc.c -----------*/

WRITE8_DEVICE_HANDLER( genpc_kb_set_clock_signal );
WRITE8_DEVICE_HANDLER( genpc_kb_set_data_signal );

DRIVER_INIT( genpc );
DRIVER_INIT( genpcvga );

#endif /* GENPC_H_ */
