/*****************************************************************************
 *
 * video/dl1416.h
 *
 * DL1416
 *
 * 4-Digit 16-Segment Alphanumeric Intelligent Display
 * with Memory/Decoder/Driver
 *
 * See video/dl1416.c for more info
 *
 ****************************************************************************/

#ifndef DL1416_H_
#define DL1416_H_


#define DL1416 dl1416_get_info


typedef enum
{
	DL1416B,
	DL1416T
} dl1416_type_t;

typedef struct _dl1416_t dl1416_t;
typedef struct _dl1416_interface dl1416_interface;

struct _dl1416_interface
{
	dl1416_type_t type;
	void (*digit_changed)(int digit, int data);
};

/* Device interface */
void dl1416_get_info(running_machine *machine, void *token, UINT32 state, deviceinfo *info);

/* Inputs */
void dl1416_set_input_w(dl1416_t *chip, int data); /* Write enable */
void dl1416_set_input_ce(dl1416_t *chip, int data); /* Chip enable */
void dl1416_set_input_cu(dl1416_t *chip, int data); /* Cursor enable */
void dl1416_write(dl1416_t *chip, offs_t offset, UINT8 data); /* Data */


#endif /* DL1416_H_ */
