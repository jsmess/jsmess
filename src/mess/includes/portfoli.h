#pragma once

#ifndef __PORTFOLIO__
#define __PORTFOLIO__

#define SCREEN_TAG		"screen"
#define M80C88A_TAG		"u1"
#define M82C55A_TAG		"hpc101_u1"
#define M82C50A_TAG		"hpc102_u1"
#define HD61830_TAG		"hd61830"
#define CENTRONICS_TAG	"centronics"
#define SPEAKER_TAG		"speaker"
#define TIMER_TICK_TAG	"tick"

class portfolio_state : public driver_device
{
public:
	portfolio_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* interrupt state */
	UINT8 ip;						/* interrupt pending */
	UINT8 ie;						/* interrupt enable */
	UINT8 sivr;						/* serial interrupt vector register */

	/* counter state */
	UINT16 counter;

	/* keyboard state */
	UINT8 keylatch;

	/* video state */
	UINT8 contrast;

	/* peripheral state */
	UINT8 pid;						/* peripheral identification */

	/* devices */
	running_device *hd61830;
	running_device *centronics;
	running_device *speaker;
	timer_device *timer_tick;
};

#endif
