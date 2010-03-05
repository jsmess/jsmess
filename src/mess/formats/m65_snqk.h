/******************************************************************************
 *  Microtan 65
 *
 *  Snapshot and quickload formats
 *
 *  Juergen Buchmueller <pullmoll@t-online.de>, Jul 2000
 *
 *  Thanks go to Geoff Macdonald <mail@geoff.org.uk>
 *  for his site http:://www.geo255.redhotant.com
 *  and to Fabrice Frances <frances@ensica.fr>
 *  for his site http://www.ifrance.com/oric/microtan.html
 *
 *****************************************************************************/

#ifndef __M65_SNQK_H__
#define __M65_SNQK_H__

#include "devices/snapquik.h"

int microtan_verify_snapshot(UINT8 *data, int size);

SNAPSHOT_LOAD( microtan );
QUICKLOAD_LOAD( microtan );

#endif /* __M65_SNQK_H__ */