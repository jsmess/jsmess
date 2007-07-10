/* kan_pand.h */

#include "driver.h"

void pandora_start(UINT8 region);
void pandora_update(running_machine *machine, mame_bitmap *bitmap, const rectangle *cliprect);

WRITE8_HANDLER ( pandora_spriteram_w );
READ8_HANDLER( pandora_spriteram_r );
WRITE16_HANDLER( pandora_spriteram_LSB_w );
READ16_HANDLER( pandora_spriteram_LSB_r );
WRITE16_HANDLER( pandora_spriteram_MSB_w );
READ16_HANDLER( pandora_spriteram_MSB_r );
