#include "driver.h"
#include "abcbus.h"

#include "formats/basicdsk.h"

typedef struct _abcbus_daisy_state abcbus_daisy_state;
struct _abcbus_daisy_state
{
	abcbus_daisy_state *	next;			/* next device */
	const device_config *	device;			/* associated device */
	abcbus_card_select		card_select;	/* card select callback */
};

static abcbus_daisy_state *daisy_state;

void abcbus_init(running_machine *machine, const char *cputag, const abcbus_daisy_chain *daisy)
{
	astring *tempstring = astring_alloc();
	abcbus_daisy_state *head = NULL;
	abcbus_daisy_state **tailptr = &head;

	/* create a linked list of devices */
	for ( ; daisy->tag != NULL; daisy++)
	{
		*tailptr = auto_alloc(machine, abcbus_daisy_state);
		(*tailptr)->next = NULL;
		(*tailptr)->device = devtag_get_device(machine, device_inherit_tag(tempstring, cputag, daisy->tag));
		if ((*tailptr)->device == NULL)
		{
			astring_free(tempstring);
			fatalerror("Unable to locate device '%s'", daisy->tag);
		}
		(*tailptr)->card_select = (abcbus_card_select)device_get_info_fct((*tailptr)->device, DEVINFO_FCT_ABCBUS_CARD_SELECT);
		tailptr = &(*tailptr)->next;
	}

	astring_free(tempstring);

	daisy_state = head;
}

WRITE8_HANDLER( abcbus_channel_w )
{
	abcbus_daisy_state *daisy = daisy_state;

	/* loop over all devices and call their card select function */
	for ( ; daisy != NULL; daisy = daisy_state->next)
		(*daisy->card_select)(daisy->device, data);
}

READ8_HANDLER( abcbus_reset_r )
{
	abcbus_daisy_state *daisy = daisy_state;

	/* loop over all devices and call their reset function */
	for ( ; daisy != NULL; daisy = daisy->next)
		device_reset(daisy->device);

	/* uninstall I/O handlers */
	memory_install_readwrite8_handler(cpu_get_address_space(space->machine->firstcpu, ADDRESS_SPACE_IO), ABCBUS_INP, ABCBUS_OUT, 0x18, 0, SMH_NOP, SMH_NOP);
	memory_install_read8_handler(cpu_get_address_space(space->machine->firstcpu, ADDRESS_SPACE_IO), ABCBUS_STAT, ABCBUS_STAT, 0x18, 0, SMH_NOP);
	memory_install_write8_handler(cpu_get_address_space(space->machine->firstcpu, ADDRESS_SPACE_IO), ABCBUS_C1, ABCBUS_C1, 0x18, 0, SMH_NOP);
	memory_install_write8_handler(cpu_get_address_space(space->machine->firstcpu, ADDRESS_SPACE_IO), ABCBUS_C2, ABCBUS_C2, 0x18, 0, SMH_NOP);
	memory_install_write8_handler(cpu_get_address_space(space->machine->firstcpu, ADDRESS_SPACE_IO), ABCBUS_C3, ABCBUS_C3, 0x18, 0, SMH_NOP);
	memory_install_write8_handler(cpu_get_address_space(space->machine->firstcpu, ADDRESS_SPACE_IO), ABCBUS_C4, ABCBUS_C4, 0x18, 0, SMH_NOP);

	return 0xff;
}

FLOPPY_OPTIONS_START(abc80)
	FLOPPY_OPTION(abc80, "dsk", "Scandia Metric FD2", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([8])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([0]))
	FLOPPY_OPTION(abc80, "dsk", "ABC 830", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([0]))
	FLOPPY_OPTION(abc80, "dsk", "ABC 832/834", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([0]))
	FLOPPY_OPTION(abc80, "dsk", "ABC 838", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([77])
		SECTORS([26])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([0]))
FLOPPY_OPTIONS_END
