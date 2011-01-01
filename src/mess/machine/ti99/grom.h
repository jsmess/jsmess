/***************************************************************************
    GROM emulation

    TI99/4x hardware supports GROMs.  GROMs are slow ROM devices, which are
    interfaced via a 8-bit data bus, and include an internal address pointer
    which is incremented after each read.  This implies that accesses are
    faster when reading consecutive bytes, although the address pointer can be
    read and written at any time.

    They are generally used to store programs in GPL (Graphic Programming
    Language: a proprietary, interpreted language - the interpreter takes most
    of a TI99/4(a) CPU ROMs).  They can used to store large pieces of data,
    too.

    Both TI-99/4 and TI-99/4a include three GROMs, with some start-up code,
    system routines and TI-Basic.  TI99/4 includes an additional Equation
    Editor.  According to the preliminary schematics found on ftp.whtech.com,
    TI-99/8 includes the three standard GROMs and 16 GROMs for the UCSD
    p-system.  TI99/2 does not include GROMs at all, and was not designed to
    support any, although it should be relatively easy to create an expansion
    card with the GPL interpreter and a /4a cartridge port.

The simple way:

    Each GROM is logically 8kb long.

    Communication with GROM is done with 4 memory-mapped locations.  You can read or write
    a 16-bit address pointer, and read data from GROMs.  One register allows to write data, too,
    which would support some GRAMs, but AFAIK TI never built such GRAMs (although docs from TI
    do refer to this possibility).

    Since address are 16-bit long, you can have up to 8 GROMs.  So, a cartridge may
    include up to 5 GROMs.  (Actually, there is a way more GROMs can be used - see below...)

    The address pointer is incremented after each GROM operation, but it will always remain
    within the bounds of the currently selected GROM (e.g. after 0x3fff comes 0x2000).

Some details:

    Original TI-built GROM are 6kb long, but take 8kb in address space.  The extra 2kb can be
    read, and follow the following formula:
        GROM[0x1800+offset] = GROM[0x0800+offset] | GROM[0x1000+offset];
    (sounds like address decoding is incomplete - we are lucky we don't burn any silicon when
    doing so...)

    Needless to say, some hackers simulated 8kb GRAMs and GROMs with normal RAM/PROM chips and
    glue logic.

GPL ports:

    When accessing the GROMs registers, 8 address bits (cpu_addr & 0x03FC) may
    be used as a port number, which permits the use of up to 256 independant
    GROM ports, with 64kb of address space in each.  TI99/4(a) ROMs can take
    advantage of the first 16 ports: it will look for GPL programs in every
    GROM of the 16 first ports.  Additionally, while the other 240 ports cannot
    contain standard GROMs with GPL code, they may still contain custom GROMs
    with data.

    Note, however, that the TI99/4(a) console does not decode the page number, so console GROMs
    occupy the first 24kb of EVERY port, and cartridge GROMs occupy the next 40kb of EVERY port
    (with the exception of one demonstration from TI that implements several distinct ports).
    (Note that the TI99/8 console does have the required decoder.)  Fortunately, GROM drivers
    have a relatively high impedance, and therefore extension cards can use TTL drivers to impose
    another value on the data bus with no risk of burning the console GROMs.  This hack permits
    the addition of additionnal GROMs in other ports, with the only side effect that whenever
    the address pointer in port N is altered, the address pointer of console GROMs in port 0
    is altered, too.  Overriding the system GROMs with custom code is possible, too (some hackers
    have done so), but I would not recommended it, as connecting such a device to a TI99/8 might
    burn some drivers.

    The p-code card (-> UCSD Pascal system) contains 8 GROMs, all in port 16.  This port is not
    in the range recognized by the TI ROMs (0-15), and therefore it is used by the p-code DSR ROMs
    as custom data.  Additionally, some hackers used the extra ports to implement "GRAM" devices.


***************************************************************************/

#define READ8Z_DEVICE_HANDLER(name)		void name(ATTR_UNUSED device_t *device, ATTR_UNUSED offs_t offset, UINT8 *value)

typedef struct _ti99grom_config
{
	int						writeable;
	int 					ident;
	const char				*region;
	UINT8					*(*get_memory)(device_t *device);
	offs_t					offset;
	int 					size;
	int 					rollover;	// some GRAM simulations do not implement rollover
	write_line_device_func	ready;
} ti99grom_config;

typedef struct _grom_callback
{
	devcb_write_line ready;
} grom_callback;

WRITE8_DEVICE_HANDLER( ti99grom_w );
READ8Z_DEVICE_HANDLER( ti99grom_rz );

DECLARE_LEGACY_DEVICE( GROM, ti99grom );

#define MCFG_GROM_ADD(_tag, _id, _region, _offset, _size, _ready)	\
	MCFG_DEVICE_ADD(_tag, GROM, 0)	\
	MCFG_DEVICE_CONFIG_DATA32(ti99grom_config, writeable, FALSE) \
	MCFG_DEVICE_CONFIG_DATA32(ti99grom_config, ident, _id) \
	MCFG_DEVICE_CONFIG_DATAPTR(ti99grom_config, region, _region) \
	MCFG_DEVICE_CONFIG_DATA32(ti99grom_config, offset, _offset) \
	MCFG_DEVICE_CONFIG_DATA32(ti99grom_config, size, _size) \
	MCFG_DEVICE_CONFIG_DATA32(ti99grom_config, rollover, FALSE) \
	MCFG_DEVICE_CONFIG_DATAPTR(ti99grom_config, ready, _ready)

#define MCFG_GROM_ADD_P(_tag, _id, _memptr, _offset, _size, _ready)	\
	MCFG_DEVICE_ADD(_tag, GROM, 0)	\
	MCFG_DEVICE_CONFIG_DATA32(ti99grom_config, writeable, FALSE) \
	MCFG_DEVICE_CONFIG_DATA32(ti99grom_config, ident, _id) \
	MCFG_DEVICE_CONFIG_DATAPTR(ti99grom_config, get_memory, _memptr) \
	MCFG_DEVICE_CONFIG_DATAPTR(ti99grom_config, region, NULL) \
	MCFG_DEVICE_CONFIG_DATA32(ti99grom_config, offset, _offset) \
	MCFG_DEVICE_CONFIG_DATA32(ti99grom_config, size, _size) \
	MCFG_DEVICE_CONFIG_DATA32(ti99grom_config, rollover, FALSE) \
	MCFG_DEVICE_CONFIG_DATAPTR(ti99grom_config, ready, _ready)

#define MCFG_GRAM_ADD(_tag, _id, _region, _offset, _size, _rollover, _ready)		\
	MCFG_DEVICE_ADD(_tag, GROM, 0)								\
	MCFG_DEVICE_CONFIG_DATA32(ti99grom_config, writeable, TRUE)	\
	MCFG_DEVICE_CONFIG_DATA32(ti99grom_config, ident, _id)		\
	MCFG_DEVICE_CONFIG_DATAPTR(ti99grom_config, region, _region) \
	MCFG_DEVICE_CONFIG_DATA32(ti99grom_config, offset, _offset) 	\
	MCFG_DEVICE_CONFIG_DATA32(ti99grom_config, size, _size)	\
	MCFG_DEVICE_CONFIG_DATA32(ti99grom_config, rollover, _rollover) \
	MCFG_DEVICE_CONFIG_DATAPTR(ti99grom_config, ready, _ready)


