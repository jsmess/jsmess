#ifndef CBMIPT_H_
#define CBMIPT_H_


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


#endif /* CBMIPT_H_ */
