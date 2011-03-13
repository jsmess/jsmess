/*
    Common definitions for TI family
*/

#ifndef __TI99DEFS__
#define __TI99DEFS__

#define region_grom "cons_grom"
#define READ8Z_DEVICE_HANDLER(name)		void name(ATTR_UNUSED device_t *device, ATTR_UNUSED offs_t offset, UINT8 *value)
#define READ16Z_DEVICE_HANDLER(name)		void name(ATTR_UNUSED device_t *device, ATTR_UNUSED offs_t offset, UINT16 *value)

#define GENMOD 0x01

enum
{
	GM_TURBO = 1,
	GM_TIM = 2,
	GM_NEVER = 42
};

enum
{
	RAM_NONE = 0,
	RAM_TI32_INT,
	RAM_TI32_EXT,
	RAM_SUPERAMS1024,
	RAM_FOUNDATION128,
	RAM_FOUNDATION512,
	RAM_MYARC128,
	RAM_MYARC512,
	RAM_99_4P,
	RAM_99_8
};

enum
{
	DISK_NONE = 0,
	DISK_TIFDC,
	DISK_BWG,
	DISK_HFDC
};

enum
{
	HD_NONE = 0,
	HD_IDE,
	HD_WHTECH,
	HD_USB = 4
};

enum
{
	SERIAL_NONE = 0,
	SERIAL_TI
};

enum
{
	EXT_NONE = 0,
	EXT_HSGPL_FLASH = 1,
	EXT_HSGPL_ON = 2,
	EXT_PCODE = 4
};

enum
{
	HCI_NONE = 0,
	HCI_MECMOUSE = 1,
	HCI_IR = 4
};

enum
{
	CART_AUTO = 0,
	CART_1,
	CART_2,
	CART_3,
	CART_4,
	CART_GK = 15
};

enum
{
	GK_OFF = 0,
	GK_NORMAL = 1,
	GK_GRAM0 = 0,
	GK_OPSYS = 1,
	GK_GRAM12 = 0,
	GK_TIBASIC = 1,
	GK_BANK1 = 0,
	GK_WP = 1,
	GK_BANK2 = 2,
	GK_LDON = 0,
	GK_LDOFF = 1
};

#endif
