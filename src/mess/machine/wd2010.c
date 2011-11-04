/**********************************************************************

    Western Digital WD2010 Winchester Disk Controller

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "machine/wd2010.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define LOG 1


// task file
enum
{
	TASK_FILE_ERROR = 1,
	TASK_FILE_WRITE_PRECOMP_CYLINDER = TASK_FILE_ERROR,
	TASK_FILE_SECTOR_COUNT,
	TASK_FILE_SECTOR_NUMBER,
	TASK_FILE_CYLINDER_LOW,
	TASK_FILE_CYLINDER_HIGH,
	TASK_FILE_SDH_REGISTER,
	TASK_FILE_STATUS,
	TASK_FILE_COMMAND = TASK_FILE_STATUS
};

#define WRITE_PRECOMP_CYLINDER \
	(m_task_file[TASK_FILE_WRITE_PRECOMP_CYLINDER] * 4)

#define SECTOR_COUNT \
	((m_task_file[TASK_FILE_SECTOR_COUNT] + 1) * 256)

#define SECTOR_NUMBER \
	(m_task_file[TASK_FILE_SECTOR_NUMBER])

#define CYLINDER \
	(((m_task_file[TASK_FILE_CYLINDER_HIGH] & 0x07) << 8) | m_task_file[TASK_FILE_CYLINDER_LOW])

#define HEAD \
	(m_task_file[TASK_FILE_SDH_REGISTER] & 0x07)

#define DRIVE \
	((m_task_file[TASK_FILE_SDH_REGISTER] >> 3) & 0x03)

static const int SECTOR_SIZES[4] = { 256, 512, 1024, 128 };
		
#define SECTOR_SIZE \
	SECTOR_SIZES[(m_task_file[TASK_FILE_SDH_REGISTER] >> 5) & 0x03]


// status register
#define STATUS_BSY		0x80
#define STATUS_RDY		0x40
#define STATUS_WF		0x20
#define STATUS_SC		0x10
#define STATUS_DRQ		0x08
#define STATUS_DWC		0x04
#define STATUS_CIP		0x02
#define STATUS_ERR		0x01


// error register
#define ERROR_BB		0x80
#define ERROR_CRC_ECC	0x40
#define ERROR_ID		0x10
#define ERROR_AC		0x04
#define ERROR_TK		0x02
#define ERROR_DM		0x01


// command register
#define COMMAND_MASK				0xf0
#define COMMAND_RESTORE				0x10
#define COMMAND_SEEK				0x70
#define COMMAND_READ_SECTOR			0x20
#define COMMAND_WRITE_SECTOR		0x30
#define COMMAND_SCAN_ID				0x40
#define COMMAND_WRITE_FORMAT		0x50
#define COMMAND_COMPUTE_CORRECTION	0x08
#define COMMAND_SET_PARAMETER_MASK	0xfe
#define COMMAND_SET_PARAMETER		0x00



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type WD2010 = &device_creator<wd2010_device>;


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void wd2010_device::device_config_complete()
{
	// inherit a copy of the static data
	const wd2010_interface *intf = reinterpret_cast<const wd2010_interface *>(static_config());
	if (intf != NULL)
		*static_cast<wd2010_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		memset(&m_out_intrq_cb, 0, sizeof(m_out_intrq_cb));
		memset(&m_out_bdrq_cb, 0, sizeof(m_out_bdrq_cb));
		memset(&m_out_bcr_cb, 0, sizeof(m_out_bcr_cb));
	}
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  wd2010_device - constructor
//-------------------------------------------------

wd2010_device::wd2010_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, WD2010, "Western Digital WD2010", tag, owner, clock)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void wd2010_device::device_start()
{
	// resolve callbacks
	m_out_intrq_func.resolve(m_out_intrq_cb, *this);
	m_out_bdrq_func.resolve(m_out_bdrq_cb, *this);
	m_out_bcr_func.resolve(m_out_bcr_cb, *this);
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void wd2010_device::device_reset()
{
}


//-------------------------------------------------
//  read -
//-------------------------------------------------

READ8_MEMBER( wd2010_device::read )
{
	UINT8 data = 0;
	
	switch (offset)
	{
	case TASK_FILE_ERROR:
		data = m_error;
		break;
			
	case TASK_FILE_STATUS:
		data = m_status;
		break;
		
	default:
		data = m_task_file[offset];
		break;
	}
	
	return data;
}


//-------------------------------------------------
//  write -
//-------------------------------------------------

WRITE8_MEMBER( wd2010_device::write )
{
	m_task_file[offset] = data;
	
	switch (offset)
	{
	case TASK_FILE_WRITE_PRECOMP_CYLINDER:
		if (LOG) logerror("%s WD2010 '%s' Write Precomp Cylinder: %u\n", machine().describe_context(), tag(), WRITE_PRECOMP_CYLINDER);
		break;
		
	case TASK_FILE_SECTOR_COUNT:
		if (LOG) logerror("%s WD2010 '%s' Sector Count: %u\n", machine().describe_context(), tag(), SECTOR_COUNT);
		break;
		
	case TASK_FILE_SECTOR_NUMBER:
		if (LOG) logerror("%s WD2010 '%s' Sector Number: %u\n", machine().describe_context(), tag(), SECTOR_NUMBER);
		break;
		
	case TASK_FILE_CYLINDER_LOW:
		if (LOG) logerror("%s WD2010 '%s' Cylinder Low: %u\n", machine().describe_context(), tag(), CYLINDER);
		break;
		
	case TASK_FILE_CYLINDER_HIGH:
		if (LOG) logerror("%s WD2010 '%s' Cylinder Low: %u\n", machine().describe_context(), tag(), CYLINDER);
		break;
		
	case TASK_FILE_SDH_REGISTER:
		if (LOG)
		{
			logerror("%s WD2010 '%s' Head: %u\n", machine().describe_context(), tag(), HEAD);
			logerror("%s WD2010 '%s' Drive: %u\n", machine().describe_context(), tag(), DRIVE);
			logerror("%s WD2010 '%s' Sector Size: %u\n", machine().describe_context(), tag(), SECTOR_SIZE);
		}
		break;
		
	case TASK_FILE_COMMAND:
		if (data == COMMAND_COMPUTE_CORRECTION)
		{
			if (LOG) logerror("%s WD2010 '%s' COMPUTE CORRECTION\n", machine().describe_context(), tag());
		}
		else if ((data & COMMAND_SET_PARAMETER_MASK) == COMMAND_SET_PARAMETER)
		{
			if (LOG) logerror("%s WD2010 '%s' SET PARAMETER\n", machine().describe_context(), tag());
		}
		else
		{
			switch (data & COMMAND_MASK)
			{
			case COMMAND_RESTORE:
				if (LOG) logerror("%s WD2010 '%s' RESTORE\n", machine().describe_context(), tag());
				break;
				
			case COMMAND_SEEK:
				if (LOG) logerror("%s WD2010 '%s' SEEK\n", machine().describe_context(), tag());
				break;
				
			case COMMAND_READ_SECTOR:
				if (LOG) logerror("%s WD2010 '%s' READ SECTOR\n", machine().describe_context(), tag());
				break;
				
			case COMMAND_WRITE_SECTOR:
				if (LOG) logerror("%s WD2010 '%s' WRITE SECTOR\n", machine().describe_context(), tag());
				break;
				
			case COMMAND_SCAN_ID:
				if (LOG) logerror("%s WD2010 '%s' SCAN ID\n", machine().describe_context(), tag());
				break;
				
			case COMMAND_WRITE_FORMAT:
				if (LOG) logerror("%s WD2010 '%s' WRITE FORMAT\n", machine().describe_context(), tag());
				break;
			}
		}
		break;
	}
}
