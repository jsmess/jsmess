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
extern const slot_interface slot_interface_c64_expansion_cards[];


	
#endif /* CBMIPT_H_ */
