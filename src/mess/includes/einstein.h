/*****************************************************************************
 *
 * includes/einstein.h
 *
 ****************************************************************************/

#ifndef EINSTEIN_H_
#define EINSTEIN_H_

#include "cpu/z80/z80daisy.h"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

class einstein_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, einstein_state(machine)); }

	einstein_state(running_machine &machine) { }

	running_device *color_screen;
	running_device *ctc;

	int rom_enabled;
	int interrupt;
	int interrupt_mask;
	int ctc_trigger;

	/* keyboard */
	UINT8 keyboard_line;
	UINT8 keyboard_data;

	/* 80 column device */
	running_device *mc6845;
	screen_device *crtc_screen;
	UINT8 *crtc_ram;
};

/***************************************************************************
    CONSTANTS
***************************************************************************/

/* xtals */
#define XTAL_X001  XTAL_10_738635MHz
#define XTAL_X002  XTAL_8MHz

/* integrated circuits */
#define IC_I001  "i001"  /* Z8400A */
#define IC_I030  "i030"  /* AY-3-8910 */
#define IC_I038  "i038"  /* TMM9129 */
#define IC_I042  "i042"  /* WD1770-PH */
#define IC_I050  "i050"  /* ADC0844CCN */
#define IC_I058  "i058"  /* Z8430A */
#define IC_I060  "i060"  /* uPD8251A */
#define IC_I063  "i063"  /* Z8420A */

/* interrupt sources */
#define EINSTEIN_KEY_INT   (1<<0)
#define EINSTEIN_ADC_INT   (1<<1)
#define EINSTEIN_FIRE_INT  (1<<2)

// ======================>  einstein_keyboard_daisy_device_config

class einstein_keyboard_daisy_device_config :	public device_config,
								public device_config_z80daisy_interface
{
	friend class einstein_keyboard_daisy_device;

	// construction/destruction
	einstein_keyboard_daisy_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
	// allocators
	static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
	virtual device_t *alloc_device(running_machine &machine) const;
};



// ======================> einstein_keyboard_daisy_device

class einstein_keyboard_daisy_device :	public device_t,
						public device_z80daisy_interface
{
	friend class einstein_keyboard_daisy_device_config;

	// construction/destruction
	einstein_keyboard_daisy_device(running_machine &_machine, const einstein_keyboard_daisy_device_config &_config);

private:
	virtual void device_start();
	// z80daisy_interface overrides
	virtual int z80daisy_irq_state();
	virtual int z80daisy_irq_ack();
	virtual void z80daisy_irq_reti();

	// internal state
	const einstein_keyboard_daisy_device_config &m_config;
};

extern const device_type EINSTEIN_KEYBOARD_DAISY;

// ======================>  einstein_adc_daisy_device_config

class einstein_adc_daisy_device_config :	public device_config,
								public device_config_z80daisy_interface
{
	friend class einstein_adc_daisy_device;

	// construction/destruction
	einstein_adc_daisy_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
	// allocators
	static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
	virtual device_t *alloc_device(running_machine &machine) const;
};



// ======================> einstein_adc_daisy_device

class einstein_adc_daisy_device :	public device_t,
						public device_z80daisy_interface
{
	friend class einstein_adc_daisy_device_config;

	// construction/destruction
	einstein_adc_daisy_device(running_machine &_machine, const einstein_adc_daisy_device_config &_config);

private:
	virtual void device_start();
	// z80daisy_interface overrides
	virtual int z80daisy_irq_state();
	virtual int z80daisy_irq_ack();
	virtual void z80daisy_irq_reti();

	// internal state
	const einstein_adc_daisy_device_config &m_config;
};

extern const device_type EINSTEIN_ADC_DAISY;

// ======================>  einstein_fire_daisy_device_config

class einstein_fire_daisy_device_config :	public device_config,
								public device_config_z80daisy_interface
{
	friend class einstein_fire_daisy_device;

	// construction/destruction
	einstein_fire_daisy_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
	// allocators
	static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
	virtual device_t *alloc_device(running_machine &machine) const;
};



// ======================> einstein_fire_daisy_device

class einstein_fire_daisy_device :	public device_t,
						public device_z80daisy_interface
{
	friend class einstein_fire_daisy_device_config;

	// construction/destruction
	einstein_fire_daisy_device(running_machine &_machine, const einstein_fire_daisy_device_config &_config);

private:
	virtual void device_start();
	// z80daisy_interface overrides
	virtual int z80daisy_irq_state();
	virtual int z80daisy_irq_ack();
	virtual void z80daisy_irq_reti();

	// internal state
	const einstein_fire_daisy_device_config &m_config;
};

extern const device_type EINSTEIN_FIRE_DAISY;

#endif /* EINSTEIN_H_ */