/*********************************************************************

    inputx.h

    Secondary input related functions for MESS specific functionality

*********************************************************************/

#ifndef __INPUTX_H__
#define __INPUTX_H__

#include "mame.h"
#include "driver.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

/* input classes */
enum
{
	INPUT_CLASS_INTERNAL,
	INPUT_CLASS_KEYBOARD,
	INPUT_CLASS_CONTROLLER,
	INPUT_CLASS_CONFIG,
	INPUT_CLASS_DIPSWITCH,
	INPUT_CLASS_CATEGORIZED,
	INPUT_CLASS_MISC
};

/* MESS uses Supplementary private use B to represent code points
 * corresponding to MAME keycodes and shift keys.  The nice thing about
 * supplemental private use B is that it is higher than any possible code
 * points */
#define UCHAR_PRIVATE		(0x100000)
#define UCHAR_SHIFT_1		(UCHAR_PRIVATE + 0)
#define UCHAR_SHIFT_2		(UCHAR_PRIVATE + 1)
#define UCHAR_MAMEKEY_BEGIN	(UCHAR_PRIVATE + 2)
#define UCHAR_MAMEKEY_END	(UCHAR_MAMEKEY_BEGIN + __code_key_last)
#define UCHAR_MAMEKEY(code)	(UCHAR_MAMEKEY_BEGIN + KEYCODE_##code)

#define UCHAR_SHIFT_BEGIN	(UCHAR_SHIFT_1)
#define UCHAR_SHIFT_END		(UCHAR_SHIFT_2)



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* these are called by the core; they should not be called from FEs */
void inputx_init(running_machine *machine);
void mess_input_port_update_hook(running_machine *machine, const input_port_config *port, input_port_value *digital);
astring *mess_get_keyboard_key_name(const input_field_config *field);

/* called by drivers to setup natural keyboard support */
void inputx_setup_natural_keyboard(
	int (*queue_chars)(const unicode_char *text, size_t text_len),
	int (*accept_char)(unicode_char ch),
	int (*charqueue_empty)(void));

/* validity checks */
int mess_validate_input_ports(int drivnum, const machine_config *config, const input_port_list *portlist);
int mess_validate_natural_keyboard_statics(void);

/* these can be called from FEs */
int inputx_can_post(running_machine *machine);
int inputx_can_post_key(running_machine *machine, unicode_char ch);
int inputx_is_posting(running_machine *machine);
const char *inputx_key_name(unicode_char ch);

/* various posting functions; can be called from FEs */
void inputx_post(running_machine *machine, const unicode_char *text);
void inputx_post_rate(running_machine *machine, const unicode_char *text, attotime rate);
void inputx_postc(running_machine *machine, unicode_char ch);
void inputx_postc_rate(running_machine *machine, unicode_char ch, attotime rate);
void inputx_postn(running_machine *machine, const unicode_char *text, size_t text_len);
void inputx_postn_rate(running_machine *machine, const unicode_char *text, size_t text_len, attotime rate);
void inputx_post_utf16(running_machine *machine, const utf16_char *text);
void inputx_post_utf16_rate(running_machine *machine, const utf16_char *text, attotime rate);
void inputx_postn_utf16(running_machine *machine, const utf16_char *text, size_t text_len);
void inputx_postn_utf16_rate(running_machine *machine, const utf16_char *text, size_t text_len, attotime rate);
void inputx_post_utf8(running_machine *machine, const char *text);
void inputx_post_utf8_rate(running_machine *machine, const char *text, attotime rate);
void inputx_postn_utf8(running_machine *machine, const char *text, size_t text_len);
void inputx_postn_utf8_rate(running_machine *machine, const char *text, size_t text_len, attotime rate);
void inputx_post_coded(running_machine *machine, const char *text);
void inputx_post_coded_rate(running_machine *machine, const char *text, attotime rate);
void inputx_postn_coded(running_machine *machine, const char *text, size_t text_len);
void inputx_postn_coded_rate(running_machine *machine, const char *text, size_t text_len, attotime rate);

/* miscellaneous functions */
int input_classify_port(const input_field_config *field);
int input_has_input_class(running_machine *machine, int inputclass);
int input_player_number(const input_field_config *field);
int input_count_players(running_machine *machine);
int input_category_active(running_machine *machine, int category);

#endif /* INPUTX_H */
