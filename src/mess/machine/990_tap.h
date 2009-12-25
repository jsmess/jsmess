/*
    990_tap.h: include file for 990_tap.c
*/
extern READ16_DEVICE_HANDLER(ti990_tpc_r);
extern WRITE16_DEVICE_HANDLER(ti990_tpc_w);

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _ti990_tpc_interface ti990_tpc_interface;
struct _ti990_tpc_interface
{
	void (*interrupt_callback)(running_machine *machine, int state);
};
/***************************************************************************
    MACROS
***************************************************************************/

#define TI990_TAPE_CTRL		DEVICE_GET_INFO_NAME(tap_990)

#define MDRV_TI990_TAPE_CTRL_ADD(_tag, _intrf)	\
	MDRV_DEVICE_ADD((_tag),  TI990_TAPE_CTRL, 0)\
	MDRV_DEVICE_CONFIG(_intrf)

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

DEVICE_GET_INFO(tap_990);
