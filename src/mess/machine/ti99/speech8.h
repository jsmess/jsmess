/*
    TI-99/8 Speech Synthesizer
    Michael Zapf, October 2010
*/

#ifndef __TISPEECH8__
#define __TISPEECH8__

#include "ti99defs.h"

READ8Z_DEVICE_HANDLER( ti998spch_rz );
WRITE8_DEVICE_HANDLER( ti998spch_w );

DECLARE_LEGACY_DEVICE( TISPEECH8, ti99_speech8 );

#define MCFG_TISPEECH_ADD(_tag)		\
	MCFG_DEVICE_ADD(_tag, TISPEECH8, 0)
#endif
