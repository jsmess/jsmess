/*********************************************************************

	cococart.h

	CoCo/Dragon cartridge management

*********************************************************************/

#ifndef __COCOCART_H__
#define __COCOCART_H__

#include "mamecore.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

typedef enum _cococart_line
{
	COCOCART_LINE_CART,		/* connects to PIA1 CB1 */
	COCOCART_LINE_NMI,		/* connects to NMI line on CPU */
	COCOCART_LINE_HALT		/* connects to HALT line on CPU */
} cococart_line;

typedef enum _cococart_line_value
{
	COCOCART_LINE_VALUE_CLEAR,
	COCOCART_LINE_VALUE_ASSERT,
	COCOCART_LINE_VALUE_Q
} cococart_line_value;

enum
{
	/* --- the following bits of info are returned as 64-bit signed integers --- */
	COCOCARTINFO_INT_FIRST = 0x00000,
	COCOCARTINFO_INT_DATASIZE = COCOCARTINFO_INT_FIRST,

	/* --- the following bits of info are returned as pointers to data or functions --- */
	COCOCARTINFO_PTR_FIRST = 0x10000,

	COCOCARTINFO_PTR_SET_INFO = COCOCARTINFO_PTR_FIRST,	/* R/O: void (*set_info)(UINT32 state, cococartinfo *info) */
	COCOCARTINFO_PTR_INIT,								/* R/O: void (*init)(coco_cartridge *cartridge) */
	COCOCARTINFO_PTR_FF40_R,							/* R/O: UINT8 (*rh)(coco_cartridge *cartridge, UINT16) */
	COCOCARTINFO_PTR_FF40_W,							/* R/O: void (*wh)(coco_cartridge *cartridge, UINT16, UINT8) */

	/* --- the following bits of info are returned as NULL-terminated strings --- */
	COCOCARTINFO_STR_FIRST = 0x20000,

	COCOCARTINFO_STR_NAME = COCOCARTINFO_PTR_FIRST,		/* R/O: name of hardware type */
};



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _coco_cartridge coco_cartridge;

typedef union _cococartinfo cococartinfo;
union _cococartinfo
{
	INT64	i;															/* generic integers */
	void *	p;															/* generic pointers */
	genf *  f;															/* generic function pointers */
	char *	s;															/* generic strings */

	void	(*init)(coco_cartridge *cartridge);							/* COCOCARTINFO_PTR_INIT */
	UINT8	(*rh)(coco_cartridge *cartridge, UINT16 addr);				/* COCOCARTINFO_PTR_FF40_R */
	void	(*wh)(coco_cartridge *cartridge, UINT16 addr, UINT8 data);	/* COCOCARTINFO_PTR_FF40_W */
};

typedef void (*cococart_getinfo)(UINT32 state, cococartinfo *info);


/* CoCo cartridge configuration */
typedef struct _coco_cartridge_config coco_cartridge_config;
struct _coco_cartridge_config
{
	/* callback to set a line */
	void (*set_line)(coco_cartridge *cartridge, cococart_line line, cococart_line_value value);

	/* callback to map memory */
	void (*map_memory)(coco_cartridge *cartridge, UINT32 offset, UINT32 mask);
};



/***************************************************************************
    CALLS
***************************************************************************/

char *cococart_temp_str(void);
void *cococart_get_extra_data(coco_cartridge *cartridge);
void cococart_set_line(coco_cartridge *cartridge, cococart_line line, cococart_line_value value);
cococart_line_value cococart_get_line(coco_cartridge *cartridge, cococart_line line);
void cococart_map_memory(coco_cartridge *cartridge, UINT32 offset, UINT32 mask);

coco_cartridge *cococart_init(const char *carttype, const coco_cartridge_config *callbacks);
UINT8 cococart_read(coco_cartridge *cartridge, UINT16 address);
void cococart_write(coco_cartridge *cartridge, UINT16 address, UINT8 data);
void cococart_enable_sound(coco_cartridge *cartridge, int enable);



/***************************************************************************
    CARTRIDGE ACCCESSORS
***************************************************************************/

INLINE INT64 cococart_get_info_int(cococart_getinfo getinfo, UINT32 state)
{
	cococartinfo info;
	info.i = 0;
	getinfo(state, &info);
	return info.i;
}

INLINE void *cococart_get_info_ptr(cococart_getinfo getinfo, UINT32 state)
{
	cococartinfo info;
	info.p = NULL;
	getinfo(state, &info);
	return info.p;
}

INLINE genf *cococart_get_info_fct(cococart_getinfo getinfo, UINT32 state)
{
	cococartinfo info;
	info.f = NULL;
	getinfo(state, &info);
	return info.f;
}

INLINE const char *cococart_get_info_string(cococart_getinfo getinfo, UINT32 state)
{
	cococartinfo info;
	info.s = cococart_temp_str();
	getinfo(state, &info);
	return info.s;
}



/***************************************************************************
    CARTRIDGE GET INFO FUNCS
***************************************************************************/

void cococart_fdc_coco(UINT32 state, cococartinfo *info);
void cococart_fdc_coco3_plus(UINT32 state, cococartinfo *info);
void cococart_fdc_dragon(UINT32 state, cococartinfo *info);
void cococart_pak(UINT32 state, cococartinfo *info);
void cococart_pak_banked16k(UINT32 state, cococartinfo *info);
void cococart_orch90(UINT32 state, cococartinfo *info);

#endif // __COCOCART_H__
