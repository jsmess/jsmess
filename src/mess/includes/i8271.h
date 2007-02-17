#include "driver.h"

/* data request */
#define I8271_FLAGS_DATA_REQUEST 0x01
/* data direction. If 0x02, then it is from fdc to cpu, else
it is from cpu to fdc */
#define I8271_FLAGS_DATA_DIRECTION 0x02

typedef enum
{
	I8271_STATE_EXECUTION_READ = 0,
	I8271_STATE_EXECUTION_WRITE
} I8271_STATE_t;

extern I8271_STATE_t I8271_STATE;


typedef struct i8271_interface
{
	void (*interrupt)(int state);
	void (*dma_request)(int state, int read_);
} i8271_interface;

typedef struct I8271
{
	int flags;
	int state;
	unsigned char Command;
	unsigned char StatusRegister;
	unsigned char CommandRegister;
	unsigned char ResultRegister;
	unsigned char ParameterRegister;
	unsigned char ResetRegister;
	unsigned char data;

	/* number of parameters required after command is specified */
	unsigned long ParameterCount;
	/* number of parameters written so far */
	unsigned long ParameterCountWritten;

	unsigned char CommandParameters[8];

	/* current track for each drive */
	unsigned long	CurrentTrack[2];

	/* 2 bad tracks for drive 0, followed by 2 bad tracks for drive 1 */
	unsigned long	BadTracks[4];

	/* mode special register */
	unsigned long Mode;

	
	/* drive outputs */
	int drive;
	int side;

	/* drive control output special register */
	int drive_control_output;
	/* drive control input special register */
	int drive_control_input;

	unsigned long StepRate;
	unsigned long HeadSettlingTime;
	unsigned long IndexCountBeforeHeadUnload;
	unsigned long HeadLoadTime;

	/* id on disc to find */
	int	ID_C;
	int ID_H;
	int ID_R;
	int ID_N;

	/* id of data for read/write */
	int data_id;

	int ExecutionPhaseTransferCount;
	char *pExecutionPhaseData;
	int ExecutionPhaseCount;

	/* sector counter and id counter */
	int Counter;
	
	/* ==0, to cpu, !=0 =from cpu */
	int data_direction;
	i8271_interface	fdc_interface;

	void *data_timer;
	void *command_complete_timer;
} I8271;

/* commands accepted */
#define I8271_COMMAND_SPECIFY										0x035
#define I8271_COMMAND_SEEK											0x029
#define I8271_COMMAND_READ_DRIVE_STATUS								0x02c
#define I8271_COMMAND_READ_SPECIAL_REGISTER							0x03d
#define	I8271_COMMAND_WRITE_SPECIAL_REGISTER						0x03a
#define I8271_COMMAND_FORMAT										0x023
#define I8271_COMMAND_READ_ID										0x01b
#define I8271_COMMAND_READ_DATA_SINGLE_RECORD						0x012
#define I8271_COMMAND_READ_DATA_AND_DELETED_DATA_SINGLE_RECORD		0x016
#define I8271_COMMAND_WRITE_DATA_SINGLE_RECORD						0x00a
#define I8271_COMMAND_WRITE_DELETED_DATA_SINGLE_RECORD				0x00e
#define	I8271_COMMAND_VERIFY_DATA_AND_DELETED_DATA_SINGLE_RECORD	0x01e
#define I8271_COMMAND_READ_DATA_MULTI_RECORD						0x013
#define I8271_COMMAND_READ_DATA_AND_DELETED_DATA_MULTI_RECORD		0x017
#define I8271_COMMAND_WRITE_DATA_MULTI_RECORD						0x00b
#define I8271_COMMAND_WRITE_DELETED_DATA_MULTI_RECORD				0x00f
#define	I8271_COMMAND_VERIFY_DATA_AND_DELETED_DATA_MULTI_RECORD		0x01f
#define I8271_COMMAND_SCAN_DATA										0x000
#define I8271_COMMAND_SCAN_DATA_AND_DELETED_DATA					0x004

/*
#define I8271_COMMAND_READ_OPERATION							(1<<4)
#define I8271_COMMAND_DELETED_DATA								(1<<2)
#define I8271_COMMAND_MULTI_RECORD								(1<<0)
*/



/* first parameter for specify command */
#define I8271_SPECIFY_INITIALIZATION								0x0d
#define I8271_SPECIFY_LOAD_BAD_TRACKS_SURFACE_0						0x010
#define I8271_SPECIFY_LOAD_BAD_TRACKS_SURFACE_1						0x018

/* first parameter for read/write special register */
#define I8271_SPECIAL_REGISTER_SCAN_SECTOR_NUMBER					0x06
#define I8271_SPECIAL_REGISTER_SCAN_MSB_OF_COUNT					0x014
#define I8271_SPECIAL_REGISTER_SCAN_LSB_OF_COUNT					0x013
#define I8271_SPECIAL_REGISTER_SURFACE_0_CURRENT_TRACK				0x012
#define I8271_SPECIAL_REGISTER_SURFACE_1_CURRENT_TRACK				0x01a
#define I8271_SPECIAL_REGISTER_MODE_REGISTER						0x017
#define I8271_SPECIAL_REGISTER_DRIVE_CONTROL_OUTPUT_PORT			0x023
#define I8271_SPECIAL_REGISTER_DRIVE_CONTROL_INPUT_PORT				0x022
#define I8271_SPECIAL_REGISTER_SURFACE_0_BAD_TRACK_1				0x010
#define I8271_SPECIAL_REGISTER_SURFACE_0_BAD_TRACK_2				0x011
#define I8271_SPECIAL_REGISTER_SURFACE_1_BAD_TRACK_1				0x018
#define I8271_SPECIAL_REGISTER_SURFACE_1_BAD_TRACK_2				0x019


/* status register bits */
#define I8271_STATUS_COMMAND_BUSY	0x080
#define I8271_STATUS_COMMAND_FULL	0x040
#define I8271_STATUS_PARAMETER_FULL	0x020
#define I8271_STATUS_RESULT_FULL	0x010
#define I8271_STATUS_INT_REQUEST	0x008
#define I8271_STATUS_NON_DMA_REQUEST	0x004

/* initialise emulation */
void i8271_init(i8271_interface *);
/* reset */
void i8271_reset(void);

 READ8_HANDLER(i8271_r);
WRITE8_HANDLER(i8271_w);


 READ8_HANDLER(i8271_dack_r);
WRITE8_HANDLER(i8271_dack_w);

 READ8_HANDLER(i8271_data_r);
WRITE8_HANDLER(i8271_data_w);
