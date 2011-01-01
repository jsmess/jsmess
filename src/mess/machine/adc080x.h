/**********************************************************************

    ADC0808/ADC0809 8-Bit A/D Converter emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
                   IN3   1 |*    \_/     | 28  IN2
                   IN4   2 |             | 27  IN1
                   IN5   3 |             | 26  IN0
                   IN6   4 |             | 25  ADD A
                   IN7   5 |             | 24  ADD B
                 START   6 |             | 23  ADD C
                   EOC   7 |   ADC0808   | 22  ALE
                   2-5   8 |   ADC0809   | 21  2-1 MSB
         OUTPUT ENABLE   9 |             | 20  2-2
                 CLOCK  10 |             | 19  2-3
                   Vcc  11 |             | 18  2-4
                 Vref+  12 |             | 17  2-8 LSB
                   GND  13 |             | 16  Vref-
                   2-7  14 |_____________| 15  2-6

**********************************************************************/

#ifndef __ADC080X__
#define __ADC080X__

typedef void (*adc080x_on_eoc_changed_func) (device_t *device, int level);
#define ADC080X_ON_EOC_CHANGED(name) void name(device_t *device, int level)

typedef double (*adc080x_vref_pos_read) (device_t *device);
#define ADC080X_VREF_POSITIVE_READ(name) double name(device_t *device)

typedef double (*adc080x_vref_neg_read) (device_t *device);
#define ADC080X_VREF_NEGATIVE_READ(name) double name(device_t *device)

typedef double (*adc080x_input_read) (device_t *device, int channel);
#define ADC080X_INPUT_READ(name) double name(device_t *device, int channel)

DECLARE_LEGACY_DEVICE(ADC0808, adc0808);
DECLARE_LEGACY_DEVICE(ADC0809, adc0809);

#define MCFG_ADC0808_ADD(_tag, _clock, _intrf) \
	MCFG_DEVICE_ADD(_tag, ADC0808, _clock) \
	MCFG_DEVICE_CONFIG(_intrf)

#define MCFG_ADC0809_ADD(_tag, _clock, _intrf) \
	MCFG_DEVICE_ADD(_tag, ADC0809, _clock) \
	MCFG_DEVICE_CONFIG(_intrf)


/* interface */
typedef struct _adc080x_interface adc080x_interface;
struct _adc080x_interface
{
	/* this gets called for every change of the EOC pin (pin 7) */
	adc080x_on_eoc_changed_func		on_eoc_changed;

	/* this gets called for every sampling of the positive reference voltage (pin 12) */
	adc080x_vref_pos_read			vref_pos_r;

	/* this gets called for every sampling of the negative reference voltage (pin 16) */
	adc080x_vref_neg_read			vref_neg_r;

	/* this gets called for every sampling of the input pins IN0-IN7 */
	adc080x_input_read				input_r;
};
#define ADC080X_INTERFACE(name) const adc080x_interface (name)=

/* address latching */
void adc080x_ale_w(device_t *device, int level, int address);

/* start conversion */
void adc080x_start_w(device_t *device, int level);

/* conversion data */
READ8_DEVICE_HANDLER( adc080x_data_r );

#endif
