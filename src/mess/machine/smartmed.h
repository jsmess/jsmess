/*
    smartmed.h: header file for smartmed.c
*/

#ifndef __SMARTMEDIA_H__
#define __SMARTMEDIA_H__


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

#define nand_present smartmedia_present
#define nand_protected smartmedia_protected
#define nand_busy smartmedia_busy
#define nand_data_r smartmedia_data_r
#define nand_command_w smartmedia_command_w
#define nand_address_w smartmedia_address_w
#define nand_data_w smartmedia_data_w
#define nand_read smartmedia_read
#define nand_set_data_ptr smartmedia_set_data_ptr

int smartmedia_present(device_t *device);
int smartmedia_protected(device_t *device);
int smartmedia_busy(device_t *device);

UINT8 smartmedia_data_r(device_t *device);
void smartmedia_command_w(device_t *device, UINT8 data);
void smartmedia_address_w(device_t *device, UINT8 data);
void smartmedia_data_w(device_t *device, UINT8 data);

void smartmedia_read(device_t *device, int offset, void *data, int size);

void smartmedia_set_data_ptr(device_t *device, void *ptr);

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _smartmedia_cartslot_config smartmedia_cartslot_config;
struct _smartmedia_cartslot_config
{
	const char *					interface;
};

// "Sequential Row Read is available only on K9F5608U0D_Y,P,V,F or K9F5608D0D_Y,P"

#define NAND_CHIP_K9F5608U0D   { 2, { 0xEC, 0x75                   },  512, 16,  32, 2048, 1, 2, 1 } /* K9F5608U0D */
#define NAND_CHIP_K9F5608U0D_J { 2, { 0xEC, 0x75                   },  512, 16,  32, 2048, 1, 2, 0 } /* K9F5608U0D-Jxxx */
#define NAND_CHIP_K9F5608U0B   { 2, { 0xEC, 0x75                   },  512, 16,  32, 2048, 1, 2, 0 } /* K9F5608U0B */
#define NAND_CHIP_K9F1G08U0B   { 5, { 0xEC, 0xF1, 0x00, 0x95, 0x40 }, 2048, 64,  64, 1024, 2, 2, 0 } /* K9F1G08U0B */
#define NAND_CHIP_K9LAG08U0M   { 5, { 0xEC, 0xD5, 0x55, 0x25, 0x68 }, 2048, 64, 128, 8192, 2, 3, 0 } /* K9LAG08U0M */

typedef struct _nand_chip nand_chip;
struct _nand_chip
{
	int id_len;
	UINT8 id[5];
	int page_size;
	int oob_size;
	int pages_per_block;
	int blocks_per_device;
	int col_address_cycles;
	int row_address_cycles;
	int sequential_row_read;
};

typedef struct _nand_interface nand_interface;
struct _nand_interface
{
	nand_chip chip;
	devcb_write_line devcb_write_line_rnb;
};

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

DECLARE_LEGACY_DEVICE(NAND, nand);

#define MCFG_NAND_ADD(_tag, _config) \
	MCFG_DEVICE_ADD(_tag, NAND, 0) \
    MCFG_DEVICE_CONFIG(_config)

#define NAND_INTERFACE(name) \
	const nand_interface(name) =

DECLARE_LEGACY_IMAGE_DEVICE(SMARTMEDIA, smartmedia);

#define MCFG_SMARTMEDIA_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, SMARTMEDIA, 0)

#define MCFG_SMARTMEDIA_INTERFACE(_interface)							\
	MCFG_DEVICE_CONFIG_DATAPTR(smartmedia_cartslot_config, interface, _interface )

#endif /* __SMARTMEDIA_H__ */
