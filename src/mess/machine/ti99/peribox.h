#ifndef __PBOX__
#define __PBOX__
/*
    header file for ti99_peb
*/
#include "p_code.h"
#include "ti32kmem.h"
#include "samsmem.h"
#include "myarcmem.h"
#include "ti99defs.h"

/*
    inta: callback called when the state of INTA changes (may be NULL)
    intb: callback called when the state of INTB changes (may be NULL)
    ready: callback called when the state of INTB changes (may be NULL)
    amx: Determines whether the AMA, AMB, and AMC address bus extension lines
         must be preset to some value. The standard TI Flex Cable Interface
         locks these valuies to 1. The Geneve, however, can drive these lines.
*/

typedef struct _ti99_peb_config
{
	write_line_device_func	inta;
	write_line_device_func	intb;
	write_line_device_func	ready;
	int						amx;
} ti99_peb_config;

/*
    Callback interface. The callback functions are in the console and are
    accessed from the PEB.
*/

#define MAXSLOTS 16

/*
    This interface is used by the PEB to communicate with the expansion cards.
    It actually forwards the accesses from the main system.
*/
typedef struct _ti99_peb_card_interface
{
	void			(*card_read_data)(device_t *card, offs_t offset, UINT8 *value);
	void			(*card_write_data)(device_t *card, offs_t offset, UINT8 value);

	void			(*card_read_cru)(device_t *card, offs_t offset, UINT8 *value);
	void			(*card_write_cru)(device_t *card, offs_t offset, UINT8 value);

	void			(*senila)(device_t *card, int value);
	void			(*senilb)(device_t *card, int value);

	/* For SGCPU only ("expansion bus") */
	void			(*card_read_data16)(device_t *card, offs_t offset, UINT16 *value);
	void			(*card_write_data16)(device_t *card, offs_t offset, UINT16 value);

} ti99_peb_card;

/*
    Called from the expansion cards. These are the callbacks from the cards to
    the PEB (which forwards them to the main system).
*/
typedef struct _peb_callback_if
{
	devcb_write_line	inta;
	devcb_write_line	intb;
	devcb_write_line	ready;

} peb_callback_if;

/*
    Used to call back the main system. Also imported by the cards.
    This structure holds the resolved callbacks within the PEB and the cards.
*/
typedef struct _peb_connect
{
	devcb_resolved_write_line inta;
	devcb_resolved_write_line intb;
	devcb_resolved_write_line ready;

} ti99_peb_connect;

/*
    Accessor functions from the console. Note that we make use of the enhanced
    read handler.
*/
READ8Z_DEVICE_HANDLER( ti99_peb_data_rz );
WRITE8_DEVICE_HANDLER( ti99_peb_data_w );

READ8Z_DEVICE_HANDLER( ti99_peb_cru_rz );
WRITE8_DEVICE_HANDLER( ti99_peb_cru_w );

WRITE_LINE_DEVICE_HANDLER( ti99_peb_senila );
WRITE_LINE_DEVICE_HANDLER( ti99_peb_senilb );


/*
    For SGCPU only. Actually the SGCPU uses an external cable to add the
    8 data lines that are not included in the PEB
*/
READ16Z_DEVICE_HANDLER( ti99_peb_data16_rz );
WRITE16_DEVICE_HANDLER( ti99_peb_data16_w );

/*
    Value passed to the cards
*/
typedef struct _ti99_pebcard_config
{
	int slot;
} ti99_pebcard_config;

INLINE const ti99_pebcard_config *get_pebcard_config(device_t *device)
{
	assert(device != NULL);
	return (const ti99_pebcard_config *) downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
}

/*
    Management functions
*/
int mount_card(device_t *device, device_t *cardptr, const ti99_peb_card *card, int slotindex);
void unmount_card(device_t *device, int slotindex);


/* device interface */
DECLARE_LEGACY_DEVICE( PBOX4, ti99_peb );
DECLARE_LEGACY_DEVICE( PBOX4A, ti994a_peb );
DECLARE_LEGACY_DEVICE( PBOXEV, ti99ev_peb );
DECLARE_LEGACY_DEVICE( PBOX8, ti998_peb );
DECLARE_LEGACY_DEVICE( PBOXSG, ti99sg_peb );
DECLARE_LEGACY_DEVICE( PBOXGEN, geneve_peb );

#define MCFG_PBOX4_ADD(_tag, _inta, _intb, _ready)			\
	MCFG_DEVICE_ADD(_tag, PBOX4, 0)							\
	MCFG_DEVICE_CONFIG_DATAPTR(ti99_peb_config, inta, _inta)		\
	MCFG_DEVICE_CONFIG_DATAPTR(ti99_peb_config, intb, _intb)		\
	MCFG_DEVICE_CONFIG_DATAPTR(ti99_peb_config, ready, _ready)		\
	MCFG_DEVICE_CONFIG_DATA32(ti99_peb_config, amx, 0x07)

#define MCFG_PBOX4A_ADD(_tag, _inta, _intb, _ready)			\
	MCFG_DEVICE_ADD(_tag, PBOX4A, 0)							\
	MCFG_DEVICE_CONFIG_DATAPTR(ti99_peb_config, inta, _inta)		\
	MCFG_DEVICE_CONFIG_DATAPTR(ti99_peb_config, intb, _intb)		\
	MCFG_DEVICE_CONFIG_DATAPTR(ti99_peb_config, ready, _ready)		\
	MCFG_DEVICE_CONFIG_DATA32(ti99_peb_config, amx, 0x07)

#define MCFG_PBOXEV_ADD(_tag, _inta, _intb, _ready)			\
	MCFG_DEVICE_ADD(_tag, PBOXEV, 0)							\
	MCFG_DEVICE_CONFIG_DATAPTR(ti99_peb_config, inta, _inta)		\
	MCFG_DEVICE_CONFIG_DATAPTR(ti99_peb_config, intb, _intb)		\
	MCFG_DEVICE_CONFIG_DATAPTR(ti99_peb_config, ready, _ready)		\
	MCFG_DEVICE_CONFIG_DATA32(ti99_peb_config, amx, 0x07)

#define MCFG_PBOX8_ADD(_tag, _inta, _intb, _ready)			\
	MCFG_DEVICE_ADD(_tag, PBOX8, 0)							\
	MCFG_DEVICE_CONFIG_DATAPTR(ti99_peb_config, inta, _inta)		\
	MCFG_DEVICE_CONFIG_DATAPTR(ti99_peb_config, intb, _intb)		\
	MCFG_DEVICE_CONFIG_DATAPTR(ti99_peb_config, ready, _ready)		\
	MCFG_DEVICE_CONFIG_DATA32(ti99_peb_config, amx, 0x07)

#define MCFG_PBOXSG_ADD(_tag, _inta, _intb, _ready)			\
	MCFG_DEVICE_ADD(_tag, PBOXSG, 0)							\
	MCFG_DEVICE_CONFIG_DATAPTR(ti99_peb_config, inta, _inta)		\
	MCFG_DEVICE_CONFIG_DATAPTR(ti99_peb_config, intb, _intb)		\
	MCFG_DEVICE_CONFIG_DATAPTR(ti99_peb_config, ready, _ready)	\
	MCFG_DEVICE_CONFIG_DATA32(ti99_peb_config, amx, 0x07)

#define MCFG_PBOXGEN_ADD(_tag, _inta, _intb, _ready)			\
	MCFG_DEVICE_ADD(_tag, PBOXGEN, 0)							\
	MCFG_DEVICE_CONFIG_DATAPTR(ti99_peb_config, inta, _inta)		\
	MCFG_DEVICE_CONFIG_DATAPTR(ti99_peb_config, intb, _intb)		\
	MCFG_DEVICE_CONFIG_DATAPTR(ti99_peb_config, ready, _ready)	\
	MCFG_DEVICE_CONFIG_DATA32(ti99_peb_config, amx, 0x00)

#endif /* __PBOX__ */
