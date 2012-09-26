/*********************************************************************

    cococart.h

    CoCo/Dragon cartridge management

*********************************************************************/

#ifndef __COCOCART_H__
#define __COCOCART_H__

#include "image.h"
#include "imagedev/cartslot.h"

/***************************************************************************
    CONSTANTS
***************************************************************************/

DECLARE_LEGACY_CART_SLOT_DEVICE(COCO_CARTRIDGE, coco_cartridge);
DECLARE_LEGACY_CART_SLOT_DEVICE(DRAGON_CARTRIDGE, dragon_cartridge);

DECLARE_LEGACY_CART_SLOT_DEVICE(COCO_CARTRIDGE_PCB_FDC_COCO, coco_cartridge_pcb_fdc_coco);
DECLARE_LEGACY_CART_SLOT_DEVICE(COCO_CARTRIDGE_PCB_FDC_DRAGON, coco_cartridge_pcb_fdc_dragon);
DECLARE_LEGACY_CART_SLOT_DEVICE(COCO_CARTRIDGE_PCB_PAK, coco_cartridge_pcb_pak);
DECLARE_LEGACY_CART_SLOT_DEVICE(COCO_CARTRIDGE_PCB_PAK_BANKED16K, coco_cartridge_pcb_pak_banked16k);
DECLARE_LEGACY_CART_SLOT_DEVICE(COCO_CARTRIDGE_PCB_ORCH90, coco_cartridge_pcb_orch90);
DECLARE_LEGACY_CART_SLOT_DEVICE(COCO_CARTRIDGE_PCB_RS232, coco_cartridge_pcb_rs232);

enum
{
	COCOCARTINFO_FCT_FF40_R = DEVINFO_FCT_DEVICE_SPECIFIC,		/* read8_device_handler */
	COCOCARTINFO_FCT_FF40_W,									/* write8_device_handler */
};

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* output lines on the CoCo cartridge slot */
enum _cococart_line
{
	COCOCART_LINE_CART,				/* connects to PIA1 CB1 */
	COCOCART_LINE_NMI,				/* connects to NMI line on CPU */
	COCOCART_LINE_HALT,				/* connects to HALT line on CPU */
	COCOCART_LINE_SOUND_ENABLE		/* sound enable */
};
typedef enum _cococart_line cococart_line;

/* since we have a special value "Q" - we have to use a special enum here */
enum _cococart_line_value
{
	COCOCART_LINE_VALUE_CLEAR,
	COCOCART_LINE_VALUE_ASSERT,
	COCOCART_LINE_VALUE_Q
};
typedef enum _cococart_line_value cococart_line_value;

typedef struct _cococart_config cococart_config;
struct _cococart_config
{
	void (*cart_callback)(device_t *device, int line);
	void (*nmi_callback)(device_t *device, int line);
	void (*halt_callback)(device_t *device, int line);
};


/***************************************************************************
    CALLS
***************************************************************************/

/* device get info function */

/* reading and writing to $FF40-$FF7F */
READ8_DEVICE_HANDLER(coco_cartridge_r);
WRITE8_DEVICE_HANDLER(coco_cartridge_w);

/* sets a cartridge line */
void coco_cartridge_set_line(device_t *device, cococart_line line, cococart_line_value value);

/* hack to support twiddling the Q line */
void coco_cartridge_twiddle_q_lines(device_t *device);


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MCFG_COCO_CARTRIDGE_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, COCO_CARTRIDGE, 0)

#define MCFG_COCO_CARTRIDGE_CART_CALLBACK(_callback) \
	MCFG_DEVICE_CONFIG_DATAPTR(cococart_config, cart_callback, _callback)

#define MCFG_COCO_CARTRIDGE_NMI_CALLBACK(_callback) \
	MCFG_DEVICE_CONFIG_DATAPTR(cococart_config, nmi_callback, _callback)

#define MCFG_COCO_CARTRIDGE_HALT_CALLBACK(_callback) \
	MCFG_DEVICE_CONFIG_DATAPTR(cococart_config, halt_callback, _callback)

#define MCFG_DRAGON_CARTRIDGE_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, DRAGON_CARTRIDGE, 0)

#define MCFG_DRAGON_CARTRIDGE_CART_CALLBACK(_callback) \
	MCFG_COCO_CARTRIDGE_CART_CALLBACK(_callback)

#define MCFG_DRAGON_CARTRIDGE_NMI_CALLBACK(_callback) \
	MCFG_COCO_CARTRIDGE_NMI_CALLBACK(_callback)

#define MCFG_DRAGON_CARTRIDGE_HALT_CALLBACK(_callback) \
	MCFG_COCO_CARTRIDGE_HALT_CALLBACK(_callback)

#endif /* __COCOCART_H__ */
