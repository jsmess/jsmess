/*********************************************************************

    cococart.h

    CoCo/Dragon cartridge management

*********************************************************************/

#ifndef __COCOCART_H__
#define __COCOCART_H__

#include "device.h"
#include "image.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define COCO_CARTRIDGE								DEVICE_GET_INFO_NAME(coco_cartridge)
#define DRAGON_CARTRIDGE							DEVICE_GET_INFO_NAME(dragon_cartridge)

#define COCO_CARTRIDGE_PCB_FDC_COCO					DEVICE_GET_INFO_NAME(coco_cartridge_pcb_fdc_coco)
#define COCO_CARTRIDGE_PCB_FDC_DRAGON				DEVICE_GET_INFO_NAME(coco_cartridge_pcb_fdc_dragon)
#define COCO_CARTRIDGE_PCB_PAK						DEVICE_GET_INFO_NAME(coco_cartridge_pcb_pak)
#define COCO_CARTRIDGE_PCB_PAK_BANKED16K			DEVICE_GET_INFO_NAME(coco_cartridge_pcb_pak_banked16k)
#define COCO_CARTRIDGE_PCB_ORCH90					DEVICE_GET_INFO_NAME(coco_cartridge_pcb_orch90)
#define COCO_CARTRIDGE_PCB_RS232					DEVICE_GET_INFO_NAME(coco_cartridge_pcb_rs232)
#define COCO_CARTRIDGE_PCB_SSC						DEVICE_GET_INFO_NAME(coco_cartridge_pcb_speechsound)

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
	void (*cart_callback)(const device_config *device, int line);
	void (*nmi_callback)(const device_config *device, int line);
	void (*halt_callback)(const device_config *device, int line);
};


/***************************************************************************
    CALLS
***************************************************************************/

/* device get info function */
DEVICE_GET_INFO(coco_cartridge);
DEVICE_GET_INFO(dragon_cartridge);

/* reading and writing to $FF40-$FF7F */
READ8_DEVICE_HANDLER(coco_cartridge_r);
WRITE8_DEVICE_HANDLER(coco_cartridge_w);

/* cartridge PCB types */
DEVICE_GET_INFO(coco_cartridge_pcb_fdc_coco);
DEVICE_GET_INFO(coco_cartridge_pcb_fdc_dragon);
DEVICE_GET_INFO(coco_cartridge_pcb_pak);
DEVICE_GET_INFO(coco_cartridge_pcb_pak_banked16k);
DEVICE_GET_INFO(coco_cartridge_pcb_orch90);
DEVICE_GET_INFO(coco_cartridge_pcb_rs232);
DEVICE_GET_INFO(coco_cartridge_pcb_speechsound);

/* sets a cartridge line */
void coco_cartridge_set_line(const device_config *device, cococart_line line, cococart_line_value value);

/* hack to support twiddling the Q line */
void coco_cartridge_twiddle_q_lines(const device_config *device);


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_COCO_CARTRIDGE_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, COCO_CARTRIDGE, 0)

#define MDRV_COCO_CARTRIDGE_CART_CALLBACK(_callback) \
	MDRV_DEVICE_CONFIG_DATAPTR(cococart_config, cart_callback, _callback)

#define MDRV_COCO_CARTRIDGE_NMI_CALLBACK(_callback) \
	MDRV_DEVICE_CONFIG_DATAPTR(cococart_config, nmi_callback, _callback)

#define MDRV_COCO_CARTRIDGE_HALT_CALLBACK(_callback) \
	MDRV_DEVICE_CONFIG_DATAPTR(cococart_config, halt_callback, _callback)

#define MDRV_DRAGON_CARTRIDGE_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, DRAGON_CARTRIDGE, 0)

#define MDRV_DRAGON_CARTRIDGE_CART_CALLBACK(_callback) \
	MDRV_COCO_CARTRIDGE_CART_CALLBACK(_callback)

#define MDRV_DRAGON_CARTRIDGE_NMI_CALLBACK(_callback) \
	MDRV_COCO_CARTRIDGE_NMI_CALLBACK(_callback)

#define MDRV_DRAGON_CARTRIDGE_HALT_CALLBACK(_callback) \
	MDRV_COCO_CARTRIDGE_HALT_CALLBACK(_callback)

#endif /* __COCOCART_H__ */
