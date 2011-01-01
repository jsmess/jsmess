/*

                LMC1992 Digitally-Controlled Stereo Tone and Volume
                    Circuit with Four-Channel Input-Selector

                                ________________
                  Data   1  ---|       \/       |---  28  V+
                 Clock   2  ---|                |---  27  Bypass
                Enable   3  ---|                |---  26  Right Input 1
          Left Input 1   4  ---|                |---  25  Right Input 2
          Left Input 2   5  ---|                |---  24  Right Input 3
          Left Input 3   6  ---|                |---  23  Right Input 4
          Left Input 4   7  ---|    LMC1992     |---  22  Right Select Out
       Left Select Out   8  ---|    top view    |---  21  Right Select In
        Left Select In   9  ---|                |---  20  Right Tone In
          Left Tone In  10  ---|                |---  19  Right Tone Out
         Left Tone Out  11  ---|                |---  18  Right Op Amp Out
       Left Op Amp Out  12  ---|                |---  17  Right Rear Out
         Left Rear Out  13  ---|                |---  16  Right Front Out
        Left Front Out  14  ---|________________|---  15  Ground

*/

#ifndef __LMC1992_AUDIO__
#define __LMC1992_AUDIO__

#include "emu.h"

DECLARE_LEGACY_DEVICE(LMC1992, lmc1992);

#define MCFG_LMC1992_ADD(_tag)		\
	MCFG_DEVICE_ADD(_tag, LMC1992, 0)

/* interface */
typedef struct _lmc1992_interface lmc1992_interface;
struct _lmc1992_interface
{
	int dummy;
	// left input 1
	// left input 2
	// left input 3
	// left input 4
	// right input 1
	// right input 2
	// right input 3
	// right input 4
};
#define LMC1992_INTERFACE(name) const lmc1992_interface(name) =

enum
{
	LMC1992_FUNCTION_INPUT_SELECT = 0,
	LMC1992_FUNCTION_BASS,
	LMC1992_FUNCTION_TREBLE,
	LMC1992_FUNCTION_VOLUME,
	LMC1992_FUNCTION_RIGHT_FRONT_FADER,
	LMC1992_FUNCTION_LEFT_FRONT_FADER,
	LMC1992_FUNCTION_RIGHT_REAR_FADER,
	LMC1992_FUNCTION_LEFT_REAR_FADER
};

enum
{
	LCM1992_INPUT_SELECT_OPEN = 0,
	LCM1992_INPUT_SELECT_INPUT1,
	LCM1992_INPUT_SELECT_INPUT2,
	LCM1992_INPUT_SELECT_INPUT3,
	LCM1992_INPUT_SELECT_INPUT4
};

#define LMC1992_MICROWIRE_DEVICE_ADDRESS	2

void lmc1992_clock_w(device_t *device, int level);
void lmc1992_data_w(device_t *device, int level);
void lmc1992_enable_w(device_t *device, int level);

#endif
