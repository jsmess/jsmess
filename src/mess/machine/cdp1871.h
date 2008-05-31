/**********************************************************************

    RCA CDP1871 Keyboard Encoder emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
							_____   _____
				    D1	 1 |*    \_/     | 40  Vdd
					D2	 2 |			 | 39  SHIFT
					D3	 3 |			 | 38  CONTROL
					D4	 4 |			 | 37  ALPHA
					D5	 5 |			 | 36  DEBOUNCE
					D6	 6 |			 | 35  _RTP
					D7	 7 |			 | 34  TPB
					D8	 8 |			 | 33  _DA
					D9	 9 |			 | 32  BUS 7
				   D10	10 |   CDP1871	 | 31  BUS 6
				   D11	11 |			 | 30  BUS 5
					S1	12 |			 | 29  BUS 4
					S2	13 |			 | 28  BUS 3
					S3	14 |			 | 27  BUS 2
					S4	15 |			 | 26  BUS 1
					S5	16 |			 | 25  BUS 0
					S6	17 |			 | 24  CS4
					S7	18 |			 | 23  CS3
					S8	19 |			 | 22  CS2
				   Vss  20 |_____________| 21  _CS1

**********************************************************************/

#ifndef __CDP1871__
#define __CDP1871__

typedef void (*cdp1871_on_da_changed_func) (const device_config *device, int level);
#define CDP1871_ON_DA_CHANGED(name) void name(const device_config *device, int level)

typedef void (*cdp1871_on_rpt_changed_func) (const device_config *device, int level);
#define CDP1871_ON_RPT_CHANGED(name) void name(const device_config *device, int level)

#define CDP1871		DEVICE_GET_INFO_NAME(cdp1871)

/* interface */
typedef struct _cdp1871_interface cdp1871_interface;
struct _cdp1871_interface
{
	int clock;					/* the clock (pin 34) of the chip */

	/* this gets called for every change of the DA pin (pin 33) */
	cdp1871_on_da_changed_func		on_da_changed;

	/* this gets called for every change of the RPT pin (pin 35) */
	cdp1871_on_rpt_changed_func		on_rpt_changed;
};
#define CDP1871_INTERFACE(name) const cdp1871_interface (name)=

/* device interface */
DEVICE_GET_INFO( cdp1871 );

/* keyboard matrix */
INPUT_PORTS_EXTERN( cdp1871 );

/* keyboard data */
READ8_DEVICE_HANDLER( cdp1871_data_r );

#endif
