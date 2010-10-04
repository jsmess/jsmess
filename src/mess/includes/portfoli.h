#pragma once

#ifndef __CGC7900__
#define __CGC7900__

#define SCREEN_TAG		"screen"
#define M80C88A_TAG		"u1"
#define M82C55A_TAG		"hpc101_u1"
#define M82C50A_TAG		"hpc102_u1"
#define HD61830_TAG		"hd61830"
#define CENTRONICS_TAG	"centronics"
#define SPEAKER_TAG		"speaker"

class portfolio_state : public driver_device
{
public:
	portfolio_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* video state */
	UINT8 contrast;

	/* serial state */
	UINT8 sivr;						/* serial interrupt vector register */

	/* devices */
	running_device *hd61830;
	running_device *centronics;
	running_device *speaker;
};

#endif
