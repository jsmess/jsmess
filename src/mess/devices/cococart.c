/*********************************************************************

    cococart.c

    CoCo/Dragon cartridge management

*********************************************************************/

#include "emu.h"
#include "cococart.h"
#include "imagedev/cartslot.h"


/***************************************************************************
    PARAMETERS
***************************************************************************/

#define CARTSLOT_TAG			"cart"
#define LOG_LINE				0

/* TIMER_POOL: Must be power of two */
#define TIMER_POOL         2

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _coco_cartridge_line coco_cartridge_line;
struct _coco_cartridge_line
{
	emu_timer					*timer[TIMER_POOL];
	int                  timer_index;
	int						delay;
	cococart_line_value			value;
	int							line;
	int							q_count;
	void						(*callback)(device_t *, int line);
};


typedef struct _coco_cartridge_t coco_cartridge_t;
struct _coco_cartridge_t
{
	device_t *pcb;
	read8_device_func			pcb_r;
	write8_device_func			pcb_w;

	coco_cartridge_line			cart_line;
	coco_cartridge_line			nmi_line;
	coco_cartridge_line			halt_line;
};


/***************************************************************************
    PROTOTYPES
***************************************************************************/

static void set_line_timer(device_t *device, coco_cartridge_line *line, cococart_line_value value);

static TIMER_CALLBACK( cart_timer_callback );
static TIMER_CALLBACK( nmi_timer_callback );
static TIMER_CALLBACK( halt_timer_callback );


/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE coco_cartridge_t *get_token(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == COCO_CARTRIDGE) || (device->type() == DRAGON_CARTRIDGE));
	return (coco_cartridge_t *) downcast<legacy_device_base *>(device)->token();
}


/***************************************************************************
    GENERAL IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    DEVICE_START(coco_cartridge)
-------------------------------------------------*/

static DEVICE_START(coco_cartridge)
{
	device_t *cartslot;
	coco_cartridge_t *cococart = get_token(device);
	const cococart_config *config = (const cococart_config *) downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
   int i;

	/* initialize */
	memset(cococart, 0, sizeof(*cococart));

	/* access the PCB, and get the read/write handlers */
	cartslot = device->subdevice(CARTSLOT_TAG);
	if (cartslot != NULL)
	{
		cococart->pcb = cartslot_get_pcb(cartslot);
		if (cococart->pcb == NULL)
		{
			throw device_missing_dependencies();
		}

      cococart->pcb_r = (read8_device_func)downcast<const legacy_cart_slot_device_config_base *>(&cococart->pcb->baseconfig())->get_config_fct(COCOCARTINFO_FCT_FF40_R);
      cococart->pcb_w = (write8_device_func)downcast<const legacy_cart_slot_device_config_base *>(&cococart->pcb->baseconfig())->get_config_fct(COCOCARTINFO_FCT_FF40_W);
	}

	/* finish setup */

	for( i=0; i<TIMER_POOL; i++ )
	{
	   cococart->cart_line.timer[i]		= device->machine().scheduler().timer_alloc(FUNC(cart_timer_callback), (void *) device);
	   cococart->nmi_line.timer[i]		= device->machine().scheduler().timer_alloc(FUNC(nmi_timer_callback), (void *) device);
	   cococart->halt_line.timer[i]		= device->machine().scheduler().timer_alloc(FUNC(halt_timer_callback), (void *) device);
	}

	cococart->cart_line.timer_index     = 0;
	cococart->cart_line.delay		      = 0;
	cococart->cart_line.callback        = config->cart_callback;

	cococart->nmi_line.timer_index      = 0;
	/* 12 allowed one more instruction to finished after the line is pulled */
	cococart->nmi_line.delay		      = 12;
	cococart->nmi_line.callback         = config->nmi_callback;

	cococart->halt_line.timer_index     = 0;
	/* 6 allowed one more instruction to finished after the line is pulled */
	cococart->halt_line.delay		      = 6;
	cococart->halt_line.callback        = config->halt_callback;
}


/*-------------------------------------------------
    coco_cartridge_r
-------------------------------------------------*/

READ8_DEVICE_HANDLER(coco_cartridge_r)
{
	UINT8 result = 0x00;
	coco_cartridge_t *cococart = get_token(device);

	if (cococart->pcb_r != NULL)
		result = (*cococart->pcb_r)(cococart->pcb, offset);

	return result;
}


/*-------------------------------------------------
    coco_cartridge_w
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER(coco_cartridge_w)
{
	coco_cartridge_t *cococart = get_token(device);

	if (cococart->pcb_w != NULL)
		(*cococart->pcb_w)(cococart->pcb, offset, data);
}


/*-------------------------------------------------
    line_value_string
-------------------------------------------------*/

static const char *line_value_string(cococart_line_value value)
{
	const char *s = NULL;
	switch(value)
	{
		case COCOCART_LINE_VALUE_CLEAR:
			s = "CLEAR";
			break;
		case COCOCART_LINE_VALUE_ASSERT:
			s = "ASSERT";
			break;
		case COCOCART_LINE_VALUE_Q:
			s = "Q";
			break;
		default:
			fatalerror("Invalid value");
			break;
	}
	return s;
}


/*-------------------------------------------------
    set_line
-------------------------------------------------*/

static void set_line(device_t *device, const char *line_name, coco_cartridge_line *line, cococart_line_value value)
{
	if ((line->value != value) || (value == COCOCART_LINE_VALUE_Q))
	{
		line->value = value;

		if (LOG_LINE)
			logerror("[%s]: set_line(): %s <= %s\n", device->machine().describe_context(), line_name, line_value_string(value));

		/* engage in a bit of gymnastics for this odious 'Q' value */
		switch(line->value)
		{
			case COCOCART_LINE_VALUE_CLEAR:
				line->line = 0x00;
				line->q_count = 0;
				break;

			case COCOCART_LINE_VALUE_ASSERT:
				line->line = 0x01;
				line->q_count = 0;
				break;

			case COCOCART_LINE_VALUE_Q:
				line->line = line->line ? 0x00 : 0x01;
				if (line->q_count++ < 4)
					set_line_timer(device, line, value);
				break;
		}

		/* invoke the callback, if present */
		if (line->callback)
			(*line->callback)(device, line->line);
	}
}


/*-------------------------------------------------
    TIMER_CALLBACK( cart_timer_callback )
-------------------------------------------------*/

static TIMER_CALLBACK( cart_timer_callback )
{
	device_t *device = (device_t *) ptr;
	set_line(device, "CART", &get_token(device)->cart_line, (cococart_line_value) param);
}


/*-------------------------------------------------
    TIMER_CALLBACK( nmi_timer_callback )
-------------------------------------------------*/

static TIMER_CALLBACK( nmi_timer_callback )
{
	device_t *device = (device_t *) ptr;
	set_line(device, "NMI", &get_token(device)->nmi_line, (cococart_line_value) param);
}


/*-------------------------------------------------
    TIMER_CALLBACK( halt_timer_callback )
-------------------------------------------------*/

static TIMER_CALLBACK( halt_timer_callback )
{
	device_t *device = (device_t *) ptr;
	set_line(device, "HALT", &get_token(device)->halt_line, (cococart_line_value) param);
}


/*-------------------------------------------------
    set_line_timer()
-------------------------------------------------*/

static void set_line_timer(device_t *device, coco_cartridge_line *line, cococart_line_value value)
{
	/* calculate delay; delay dependant on cycles per second */
	attotime delay = (line->delay != 0)
		? device->machine().firstcpu->cycles_to_attotime(line->delay)
		: attotime::zero;

   line->timer[line->timer_index]->adjust(delay, (int) value);
   line->timer_index = (line->timer_index + 1) % TIMER_POOL;
}


/*-------------------------------------------------
    twiddle_line_if_q
-------------------------------------------------*/

static void twiddle_line_if_q(device_t *device, coco_cartridge_line *line)
{
	if (line->value == COCOCART_LINE_VALUE_Q)
	{
		line->q_count = 0;
		set_line_timer(device, line, COCOCART_LINE_VALUE_Q);
	}
}


/*-------------------------------------------------
    coco_cartridge_twiddle_q_lines - hack to
    support twiddling the Q line
-------------------------------------------------*/

void coco_cartridge_twiddle_q_lines(device_t *device)
{
	coco_cartridge_t *cococart = get_token(device);
	twiddle_line_if_q(device, &cococart->cart_line);
	twiddle_line_if_q(device, &cococart->nmi_line);
	twiddle_line_if_q(device, &cococart->halt_line);
}


/*-------------------------------------------------
    coco_cartridge_set_line
-------------------------------------------------*/

void coco_cartridge_set_line(device_t *device, cococart_line line, cococart_line_value value)
{
	switch (line)
	{
		case COCOCART_LINE_CART:
			set_line_timer(device, &get_token(device)->cart_line, value);
			break;

		case COCOCART_LINE_NMI:
			set_line_timer(device, &get_token(device)->nmi_line, value);
			break;

		case COCOCART_LINE_HALT:
			set_line_timer(device, &get_token(device)->halt_line, value);
			break;

		case COCOCART_LINE_SOUND_ENABLE:
			/* do nothing for now */
			break;
	}
}


/*-------------------------------------------------
    DEVICE_GET_INFO(general_cartridge)
-------------------------------------------------*/

static DEVICE_GET_INFO(general_cartridge)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(coco_cartridge_t);			break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(cococart_config);			break;

		/* --- the following bits of info are returned as pointers to functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(coco_cartridge);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "CoCo Cartridge Slot");		break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "CoCo Cartridge Slot");		break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
	}
}


/***************************************************************************
    COCO-SPECIFIC IMPLEMENTATION
***************************************************************************/

static MACHINE_CONFIG_FRAGMENT( coco_cartridge )
	MCFG_CARTSLOT_ADD(CARTSLOT_TAG)
	MCFG_CARTSLOT_EXTENSION_LIST("ccc,rom")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_PCBTYPE(0, "coco_fdc",	COCO_CARTRIDGE_PCB_FDC_COCO)
	MCFG_CARTSLOT_PCBTYPE(1, "banked_16k",	COCO_CARTRIDGE_PCB_PAK_BANKED16K)
	MCFG_CARTSLOT_PCBTYPE(2, "orch90",		COCO_CARTRIDGE_PCB_ORCH90)
	MCFG_CARTSLOT_PCBTYPE(3, "rs232",		COCO_CARTRIDGE_PCB_RS232)
	//MCFG_CARTSLOT_PCBTYPE(4, "coco_ssc",  COCO_CARTRIDGE_PCB_SSC)
	MCFG_CARTSLOT_PCBTYPE(4, "",			COCO_CARTRIDGE_PCB_PAK)
MACHINE_CONFIG_END

/*-------------------------------------------------
    DEVICE_GET_INFO(coco_cartridge)
-------------------------------------------------*/

DEVICE_GET_INFO(coco_cartridge)
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers to data --- */
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(coco_cartridge); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "CoCo Cartridge Slot");		break;

		default:										DEVICE_GET_INFO_CALL(general_cartridge);	break;
	}
}


/***************************************************************************
    DRAGON-SPECIFIC IMPLEMENTATION
***************************************************************************/

static MACHINE_CONFIG_FRAGMENT( dragon_cartridge )
	MCFG_CARTSLOT_ADD(CARTSLOT_TAG)
	MCFG_CARTSLOT_EXTENSION_LIST("ccc,rom")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_PCBTYPE(0, "dragon_fdc", COCO_CARTRIDGE_PCB_FDC_DRAGON)
	MCFG_CARTSLOT_PCBTYPE(1, "",           COCO_CARTRIDGE_PCB_PAK)
MACHINE_CONFIG_END

/*-------------------------------------------------
    DEVICE_GET_INFO(dragon_cartridge)
-------------------------------------------------*/

DEVICE_GET_INFO(dragon_cartridge)
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers to data --- */
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(dragon_cartridge); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Dragon Cartridge Slot");		break;

		default:										DEVICE_GET_INFO_CALL(general_cartridge);	break;
	}
}

DEFINE_LEGACY_CART_SLOT_DEVICE(COCO_CARTRIDGE, coco_cartridge);
DEFINE_LEGACY_CART_SLOT_DEVICE(DRAGON_CARTRIDGE, dragon_cartridge);
