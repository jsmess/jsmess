/*********************************************************************

	cococart.c

	CoCo/Dragon cartridge management

*********************************************************************/

#include "cococart.h"
#include "cpuintrf.h"
#include "sndintrf.h"


struct _coco_cartridge
{
	UINT8				(*rh)(coco_cartridge *cartridge, UINT16 addr);
	void				(*wh)(coco_cartridge *cartridge, UINT16 addr, UINT8 data);
	void				(*set_line)(coco_cartridge *cartridge, cococart_line line, cococart_line_value value);
	void				(*map_memory)(coco_cartridge *cartridge, UINT32 offset, UINT32 mask);
	void				*extra_data;
	cococart_line_value	line[3];
	char				dummy[1];
};



static const cococart_getinfo cart_procs[] =
{
	cococart_fdc_coco,
	cococart_fdc_coco3_plus,
	cococart_fdc_dragon,
	cococart_pak,
	cococart_pak_banked16k,
	cococart_orch90
};



/*-------------------------------------------------
    find_cart
-------------------------------------------------*/

static cococart_getinfo find_cart(const char *carttype)
{
	int i;
	for (i = 0; i < ARRAY_LENGTH(cart_procs); i++)
	{
		const char *that_name = cococart_get_info_string(cart_procs[i], COCOCARTINFO_STR_NAME);
		if (!mame_stricmp(carttype, that_name))
			return cart_procs[i];
	}
	return NULL;
}



/*-------------------------------------------------
    cococart_init
-------------------------------------------------*/

coco_cartridge *cococart_init(const char *carttype, const coco_cartridge_config *callbacks)
{
	coco_cartridge *cartridge;
	cococart_getinfo cart_proc;
	size_t extra_data_size;
	void (*init)(coco_cartridge *cartridge);

	/* sanity check */
	assert_always(mame_get_phase(Machine) == MAME_PHASE_INIT, "Can only call cococart_init at init time!");

	/* identify the cartridge type */
	cart_proc = find_cart(carttype);
	assert_always(cart_proc != NULL, "Invalid cartridge hardware type");	

	/* allocate the cartridge structure */
	extra_data_size = (size_t) cococart_get_info_int(cart_proc, COCOCARTINFO_INT_DATASIZE);
	cartridge = auto_malloc(sizeof(*cartridge) + extra_data_size - 1);
	memset(cartridge, '\0', sizeof(*cartridge) + extra_data_size - 1);

	/* populate the cartridge structure */
	cartridge->rh			= (UINT8 (*)(coco_cartridge *, UINT16)) cococart_get_info_fct(cart_proc, COCOCARTINFO_PTR_FF40_R);
	cartridge->wh			= (void (*)(coco_cartridge *, UINT16, UINT8)) cococart_get_info_fct(cart_proc, COCOCARTINFO_PTR_FF40_W);
	cartridge->extra_data	= (extra_data_size > 0) ? cartridge->dummy : NULL;

	/* copy callbacks */
	if (callbacks != NULL)
	{
		cartridge->set_line = callbacks->set_line;
		cartridge->map_memory = callbacks->map_memory;
	}

	/* invoke init function */
	init = (void (*)(coco_cartridge *cartridge)) cococart_get_info_fct(cart_proc, COCOCARTINFO_PTR_INIT);
	if (init != NULL)
		init(cartridge);

	/* finish up */
	return cartridge;
}



/*-------------------------------------------------
    cococart_read
-------------------------------------------------*/

UINT8 cococart_read(coco_cartridge *cartridge, UINT16 address)
{
	return cartridge->rh ? cartridge->rh(cartridge, address) : 0xFF;
}



/*-------------------------------------------------
    cococart_write
-------------------------------------------------*/

void cococart_write(coco_cartridge *cartridge, UINT16 address, UINT8 data)
{
	if (cartridge->wh)
		cartridge->wh(cartridge, address, data);
}



/*-------------------------------------------------
    cococart_set_line
-------------------------------------------------*/

void cococart_set_line(coco_cartridge *cartridge, cococart_line line, cococart_line_value value)
{
	assert_always((0 <= line) && (line < ARRAY_LENGTH(cartridge->line)), "Invalid line value");

	if (cartridge->line[line] != value)
	{
		cartridge->line[line] = value;
		if (cartridge->set_line != NULL)
			(*cartridge->set_line)(cartridge, line, value);
	}
}



/*-------------------------------------------------
    cococart_get_line
-------------------------------------------------*/

cococart_line_value cococart_get_line(coco_cartridge *cartridge, cococart_line line)
{
	assert_always((0 <= line) && (line < ARRAY_LENGTH(cartridge->line)), "Invalid line value");
	return cartridge->line[line];
}



/*-------------------------------------------------
    cococart_map_memory
-------------------------------------------------*/

void cococart_map_memory(coco_cartridge *cartridge, UINT32 offset, UINT32 mask)
{
	if (cartridge->map_memory != NULL)
		(*cartridge->map_memory)(cartridge, offset, mask);
}



/*-------------------------------------------------
    cococart_enable_sound
-------------------------------------------------*/

void cococart_enable_sound(coco_cartridge *cartridge, int enable)
{
	/* NYI */
}



/*-------------------------------------------------
    cococart_temp_str
-------------------------------------------------*/

char *cococart_temp_str(void)
{
	static char temp_string_pool[16][256];
	static int temp_string_pool_index;
	char *string = &temp_string_pool[temp_string_pool_index++ % ARRAY_LENGTH(temp_string_pool)][0];
	string[0] = 0;
	return string;
}



/*-------------------------------------------------
    cococart_get_extra_data
-------------------------------------------------*/

void *cococart_get_extra_data(coco_cartridge *cartridge)
{
	return cartridge->extra_data;
}
