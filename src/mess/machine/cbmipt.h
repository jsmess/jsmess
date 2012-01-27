#ifndef CBMIPT_H_
#define CBMIPT_H_

#include "machine/c64_std.h"
#include "machine/c64_xl80.h"
#include "machine/c1541.h"
#include "machine/c1571.h"
#include "machine/c1581.h"
#include "machine/c2031.h"
#include "machine/c2040.h"
#include "machine/c8280.h"
#include "machine/d9060.h"
#include "machine/interpod.h"
#include "machine/serialbox.h"
#include "machine/softbox.h"
#include "machine/vic1110.h"
#include "machine/vic1112.h"
#include "machine/vic1210.h"


#define MCFG_CBM_IEC_ADD(_intf, _default_drive) \
	MCFG_CBM_IEC_BUS_ADD(_intf) \
	MCFG_CBM_IEC_SLOT_ADD("iec4", 4, cbm_iec_devices, NULL, NULL) \
	MCFG_CBM_IEC_SLOT_ADD("iec8", 8, cbm_iec_devices, _default_drive, NULL) \
	MCFG_CBM_IEC_SLOT_ADD("iec9", 9, cbm_iec_devices, NULL, NULL) \
	MCFG_CBM_IEC_SLOT_ADD("iec10", 10, cbm_iec_devices, NULL, NULL) \
	MCFG_CBM_IEC_SLOT_ADD("iec11", 11, cbm_iec_devices, NULL, NULL)


#define MCFG_CBM_IEEE488_ADD(_intf, _default_drive) \
	MCFG_IEEE488_BUS_ADD(_intf) \
	MCFG_IEEE488_SLOT_ADD("ieee8", 8, cbm_ieee488_devices, _default_drive, NULL) \
	MCFG_IEEE488_SLOT_ADD("ieee9", 9, cbm_ieee488_devices, NULL, NULL) \
	MCFG_IEEE488_SLOT_ADD("ieee10", 10, cbm_ieee488_devices, NULL, NULL) \
	MCFG_IEEE488_SLOT_ADD("ieee11", 11, cbm_ieee488_devices, NULL, NULL)


/* Commodore 64 */

INPUT_PORTS_EXTERN( common_cbm_keyboard );	/* shared with c16, c65, c128 */
INPUT_PORTS_EXTERN( c64_special );
INPUT_PORTS_EXTERN( c64_controls );			/* shared with c65, c128, cbmb */


/* Commodore 16 */

INPUT_PORTS_EXTERN( c16_special );
INPUT_PORTS_EXTERN( c16_controls );


/* Commodore 65 */

INPUT_PORTS_EXTERN( c65_special );


/* Commodore 128 */

INPUT_PORTS_EXTERN( c128_special );


/* PET2001 */

INPUT_PORTS_EXTERN( pet_keyboard );
INPUT_PORTS_EXTERN( pet_business_keyboard );
INPUT_PORTS_EXTERN( pet_special );
INPUT_PORTS_EXTERN( pet_config );


/* CBMB 500 / 600/ 700 */

INPUT_PORTS_EXTERN( cbmb_keyboard );
INPUT_PORTS_EXTERN( cbmb_special );


/* Vic 20 */

INPUT_PORTS_EXTERN( vic_keyboard );
INPUT_PORTS_EXTERN( vic_special );
INPUT_PORTS_EXTERN( vic_controls );



extern const slot_interface slot_interface_cbm_iec_devices[];
extern const slot_interface slot_interface_cbm_ieee488_devices[];
extern const slot_interface slot_interface_vic20_expansion_cards[];
extern const slot_interface slot_interface_c64_expansion_cards[];



#endif /* CBMIPT_H_ */
