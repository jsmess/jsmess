/*********************************************************************

	coco_cas.h

	Format code for CoCo CAS (*.cas) files

*********************************************************************/

#ifndef COCO_CAS_H
#define COCO_CAS_H

#include "formats/cassimg.h"

extern struct CassetteFormat coco_cas_format;
extern const struct CassetteModulation coco_cas_modulation;

CASSETTE_FORMATLIST_EXTERN(coco_cassette_formats);

#endif /* COCO_CAS_H */
