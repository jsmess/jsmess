/**********************************************************************

    Sinclair ZX8302 emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
							_____   _____
				 ERASE   1 |*    \_/     | 40  
			   EXTINTL   2 |			 | 39  DB4
				MDRDWL   3 |			 | 38  DB5
				         4 |			 | 37  DB0
				BAUDX4   5 |			 | 36  DB1
				NETOUT   6 |			 | 35  COMDATA
				         7 |			 | 34  MDSELDH
				 DCSML   8 |			 | 33  MDSELCKH
					     9 |			 | 32  VSYNCH
				 PCENL  10 |	ZX8302	 | 31  XTAL2
				        11 |	 ULA	 | 30  XTAL1
				   DB2  12 |			 | 29  
				        13 |			 | 28  RESETOUTL
				        14 |			 | 27  DB7
				    A5  15 |			 | 26  IPLIL
				 NETIN  16 |			 | 25  CLKCPU
				    A1  17 |			 | 24  DB3
				    A0  18 |			 | 23  DB6
				  RAW2  19 |			 | 22  COMCTL
			 RESETOUTL  20 |_____________| 21  RAW1

**********************************************************************/

#ifndef __ZX8302__
#define __ZX8302__

typedef void (*zx8302_irq_callback_func) (const device_config *device, int state);
#define ZX8302_IRQ_CALLBACK(name) void name(const device_config *device, int state)

typedef void (*zx8302_on_baudx4_changed_func) (const device_config *device, int level);
#define ZX8302_ON_BAUDX4_CHANGED(name) void name(const device_config *device, int level)

typedef void (*zx8302_comdata_write_func) (const device_config *device, int level);
#define ZX8302_COMDATA_WRITE(name) void name(const device_config *device, int level)

#define ZX8302 DEVICE_GET_INFO_NAME( zx8302 )

/* interface */
typedef struct _zx8302_interface zx8302_interface;
struct _zx8302_interface
{
	int clock;					/* the CPU clock (pin 25) of the chip */
	int rtc_clock;				/* the RTC clock (pin 30) of the chip */

	const char *mdv1_tag;		/* microdrive 1 */
	const char *mdv2_tag;		/* microdrive 2 */

	/* this gets called for every change of the IPLIL pin (pin 26) */
	zx8302_irq_callback_func		irq_callback;

	/* this gets called for every change of the BAUDX4 pin (pin 5) */
	zx8302_on_baudx4_changed_func	on_baudx4_changed;

	/* this gets called for every write of the COMDATA pin (pin 35) */
	zx8302_comdata_write_func		comdata_w;
};
#define ZX8302_INTERFACE(name) const zx8302_interface(name) =

/* device interface */
DEVICE_GET_INFO( zx8302 );

/* real time clock */
READ8_DEVICE_HANDLER( zx8302_rtc_r );
WRITE8_DEVICE_HANDLER( zx8302_rtc_w );

/* control */
WRITE8_DEVICE_HANDLER( zx8302_control_w );

/* microdrive track data */
READ8_DEVICE_HANDLER( zx8302_mdv_track_r );

/* status */
READ8_DEVICE_HANDLER( zx8302_status_r );

/* IPC command */
WRITE8_DEVICE_HANDLER( zx8302_ipc_command_w );

/* microdrive control */
WRITE8_DEVICE_HANDLER( zx8302_mdv_control_w );

/* interrupt status */
READ8_DEVICE_HANDLER( zx8302_irq_status_r );

/* interrupt acknowledge */
WRITE8_DEVICE_HANDLER( zx8302_irq_acknowledge_w );

/* serial data */
WRITE8_DEVICE_HANDLER( zx8302_data_w );

/* vertical sync */
void zx8302_vsync_w(const device_config *device, int level);

/* communication control */
void zx8302_comctl_w(const device_config *device, int level);

/* communication data */
void zx8302_comdata_w(const device_config *device, int level);

#endif
