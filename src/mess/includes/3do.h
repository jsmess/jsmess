#ifndef REAL_3DO_H
#define REAL_3DO_H

READ32_HANDLER( nvarea_r );
WRITE32_HANDLER( nvarea_w );

READ32_HANDLER( vram_sport_r );
WRITE32_HANDLER( vram_sport_w );

READ32_HANDLER( madam_r );
WRITE32_HANDLER( madam_w );

READ32_HANDLER( clio_r );
WRITE32_HANDLER( clio_w );

#endif /* REAL_3DO_H */
