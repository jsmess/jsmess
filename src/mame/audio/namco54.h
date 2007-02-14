#ifndef NAMCO54_H
#define NAMCO54_H

#define CPUTAG_54XX "54XX"

ADDRESS_MAP_EXTERN( namco_54xx_map_program );
ADDRESS_MAP_EXTERN( namco_54xx_map_data );
ADDRESS_MAP_EXTERN( namco_54xx_map_io );

void namco_54xx_write(UINT8 data);

#endif	/* NAMCO54_H */
