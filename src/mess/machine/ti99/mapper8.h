#ifndef __MAPPER8__
#define __MAPPER8__

typedef void (*mapper_read_function)(device_t *device, offs_t offset, UINT8 *value);
typedef void (*mapper_write_function)(device_t *device, offs_t offset, UINT8 value);

typedef struct _mapper8_dev_config
{
	const char				*name;
	int						mode;
	int						stop;
	UINT32					address_mask;
	UINT32					select_pattern;
	UINT32					write_select;
	mapper_read_function	read;
	mapper_write_function	write;
} mapper8_dev_config;

#define SRAMNAME "SRAM"
#define ROM0NAME "ROM0"
#define ROM1NAME "ROM1"
#define ROM1ANAME "ROM1A"
#define INTSNAME "INTS"
#define DRAMNAME "DRAM"

READ8_DEVICE_HANDLER( ti99_mapper8_r );
WRITE8_DEVICE_HANDLER( ti99_mapper8_w );
WRITE8_DEVICE_HANDLER( ti99_mapreg_w );
WRITE8_DEVICE_HANDLER( mapper8_CRUS_w );
WRITE8_DEVICE_HANDLER( mapper8_PTGEN_w );
READ8Z_DEVICE_HANDLER( ti998dsr_rz );
WRITE8_DEVICE_HANDLER( mapper8c_w );

/* device interface */
DECLARE_LEGACY_DEVICE( MAPPER8, mapper8 );

#define MCFG_MAPPER8_ADD(_tag, _devices)			\
	MCFG_DEVICE_ADD(_tag, MAPPER8, 0) \
	MCFG_DEVICE_CONFIG( _devices )

#endif
