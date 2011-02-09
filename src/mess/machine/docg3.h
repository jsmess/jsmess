/*

    M-Systems DiskOnChip G3 - Flash Disk with MLC NAND and M-Systems? x2 Technology

    (c) 2009 Tim Schuerewegen

*/

#ifndef __DOCG3_H__
#define __DOCG3_H__

#include "devlegcy.h"

typedef struct _diskonchip_g3_config diskonchip_g3_config;
struct _diskonchip_g3_config
{
	const int size;
};

DEVICE_GET_INFO( diskonchip_g3 );

#define MCFG_DISKONCHIP_G3_ADD(_tag, _size) \
	MCFG_DEVICE_ADD(_tag, DISKONCHIP_G3, 0) \
	MCFG_DEVICE_CONFIG_DATA32(diskonchip_g3_config, size, _size)

READ16_DEVICE_HANDLER( diskonchip_g3_sec_1_r );
WRITE16_DEVICE_HANDLER( diskonchip_g3_sec_1_w );
READ16_DEVICE_HANDLER( diskonchip_g3_sec_2_r );
WRITE16_DEVICE_HANDLER( diskonchip_g3_sec_2_w );
READ16_DEVICE_HANDLER( diskonchip_g3_sec_3_r );
WRITE16_DEVICE_HANDLER( diskonchip_g3_sec_3_w );

DECLARE_LEGACY_NVRAM_DEVICE(DISKONCHIP_G3, diskonchip_g3);

#endif /* __DOCG3_H__ */
