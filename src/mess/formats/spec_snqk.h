/*****************************************************************************
 *
 * formats/spec_snqk.h
 *
 ****************************************************************************/

#ifndef __SPEC_SNQK_H__
#define __SPEC_SNQK_H__

#include "devices/snapquik.h"
#include "devices/cartslot.h"

typedef enum
{
    SPECTRUM_SNAPSHOT_NONE,
    SPECTRUM_SNAPSHOT_SNA,
    SPECTRUM_SNAPSHOT_Z80,
    SPECTRUM_SNAPSHOT_SP,
    SPECTRUM_TAPEFILE_TAP
} SPECTRUM_SNAPSHOT_TYPE;

typedef enum {
    SPECTRUM_Z80_SNAPSHOT_INVALID,
    SPECTRUM_Z80_SNAPSHOT_48K_OLD,
    SPECTRUM_Z80_SNAPSHOT_48K,
    SPECTRUM_Z80_SNAPSHOT_SAMRAM,
    SPECTRUM_Z80_SNAPSHOT_128K,
    SPECTRUM_Z80_SNAPSHOT_TS2068
} SPECTRUM_Z80_SNAPSHOT_TYPE;

SNAPSHOT_LOAD( spectrum );
QUICKLOAD_LOAD( spectrum );

#endif  /* __SPEC_SNQK_H__ */
