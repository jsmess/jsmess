/***************************************************************************

 scsihd.c - Implementation of a SCSI hard disk drive

***************************************************************************/

#include "driver.h"
#include "scsidev.h"
#include "harddisk.h"

#ifdef MESS
#include "devices/harddriv.h"
#endif

typedef struct
{
	UINT32 lba, blocks, last_lba;
	int last_command;
 	hard_disk_file *disk;
	UINT8 last_packet[16];
	UINT8 inquiry_buffer[96];
} SCSIHd;


// scsihd_exec_command

int scsihd_exec_command(SCSIHd *our_this, UINT8 *pCmdBuf)
{
	int retdata = 0;

	// remember the last command for the data transfer phase
	our_this->last_command = pCmdBuf[0] & 0x7f;

	// remember the actual command packet too
	memcpy(our_this->last_packet, pCmdBuf, 16);

	switch (our_this->last_command)
	{
		case 0:		// TEST UNIT READY
			retdata = 12;
			break;
		case 3: 	// REQUEST SENSE
			retdata = 16;
			break;
		case 4:		// FORMAT UNIT
			// (do nothing)
			break;

		case 0x08:    	// READ (6 byte)
			our_this->lba = (pCmdBuf[1]&0x1f)<<16 | pCmdBuf[2]<<8 | pCmdBuf[3];
			our_this->blocks = pCmdBuf[4];
			if (our_this->blocks == 0)
			{
				our_this->blocks = 256;
			}

			logerror("SCSIHD: READ at LBA %x for %x blocks\n", our_this->lba, our_this->blocks);

			retdata = our_this->blocks * 512;
			break;

		case 0x0a:	// WRITE (6 byte)
			our_this->lba = (pCmdBuf[1]&0x1f)<<16 | pCmdBuf[2]<<8 | pCmdBuf[3];
			our_this->blocks = pCmdBuf[4];
			if (our_this->blocks == 0)
			{
				our_this->blocks = 256;
			}

			logerror("SCSIHD: WRITE to LBA %x for %x blocks\n", our_this->lba, our_this->blocks);

			retdata = our_this->blocks * 512;
			break;

		case 0x12:	// INQUIRY
			retdata = 40;	// (12)
			break;
		case 0x15:	// MODE SELECT (used to set CDDA volume)
			logerror("SCSIHD: MODE SELECT length %x control %x\n", pCmdBuf[4], pCmdBuf[5]);
			retdata = 0x18;
			break;
		case 0x1a:	// MODE SENSE (6 byte)
			retdata = 8;

			// check for special Apple ID code
			if ((pCmdBuf[2]&0x3f) == 0x30)
			{
				retdata = 40;
			}
			break;
		case 0x25:	// READ CAPACITY
			retdata = 8;
			break;
		case 0x28: 	// READ (10 byte)
			our_this->lba = pCmdBuf[2]<<24 | pCmdBuf[3]<<16 | pCmdBuf[4]<<8 | pCmdBuf[5];
			our_this->blocks = pCmdBuf[6]<<24 | pCmdBuf[7]<<16 | pCmdBuf[8]<<8 | pCmdBuf[9];

			logerror("SCSIHD: READ at LBA %x for %x blocks\n", our_this->lba, our_this->blocks);

			retdata = our_this->blocks * 512;
			break;
		case 0xa8: 	// READ (12 byte)
			our_this->lba = pCmdBuf[2]<<24 | pCmdBuf[3]<<16 | pCmdBuf[4]<<8 | pCmdBuf[5];
			our_this->blocks = pCmdBuf[7]<<8 | pCmdBuf[8];

			logerror("SCSIHD: READ at LBA %x for %x blocks\n", our_this->lba, our_this->blocks);

			retdata = our_this->blocks * 512;
			break;
		default:
			logerror("SCSIHD: unknown SCSI command %x!\n", our_this->last_command);
			break;
	}

	return retdata;
}

void scsihd_read_data(SCSIHd *our_this, int bytes, UINT8 *pData)
{
	int i;

	switch (our_this->last_command)
	{
		case 0x03:	// REQUEST SENSE
			pData[0] = 0x80;	// valid sense
			for (i = 1; i < 12; i++)
			{
				pData[i] = 0;
			}
			break;

		case 0x12:	// INQUIRY
			i = sizeof( our_this->inquiry_buffer );
			if( i > bytes )
			{
				i = bytes;
			}

			memcpy( pData, our_this->inquiry_buffer, i );

			if( i < bytes )
			{
				memset( pData + i, 0, bytes - i );
			}

			break;

		case 0x1a:	// MODE SENSE (6 byte)
			// special Apple ID page.  this is a vendor-specific page,
			// so unless collisions occur there should be no need
			// to change it.
			if ((our_this->last_packet[2] & 0x3f) == 0x30)
			{
				memset(pData, 0, 40);
				pData[0] = 0x14;
				strcpy((char *)&pData[14], "APPLE COMPUTER, INC.");
			}
			break;

		case 0x08:	// READ (6 byte)
		case 0x28:	// READ (10 byte)
		case 0xa8:	// READ (12 byte)
			if ((our_this->disk) && (our_this->blocks))
			{
				while (bytes > 0)
				{
					if (!hard_disk_read(our_this->disk, our_this->lba,  pData))
					{
						logerror("SCSIHD: HD read error!\n");
					}
					our_this->lba++;
					our_this->last_lba = our_this->lba;
					our_this->blocks--;
					bytes -= 512;
					pData += 512;
				}
			}
			break;


		case 0x25:	// READ CAPACITY
			{
				hard_disk_info *info;
				UINT32 temp;

				info = hard_disk_get_info(our_this->disk);

				logerror("SCSIHD: READ CAPACITY\n");

				// get # of sectors
				temp = info->cylinders * info->heads * info->sectors;
				temp--;

				pData[0] = (temp>>24) & 0xff;
				pData[1] = (temp>>16) & 0xff;
				pData[2] = (temp>>8) & 0xff;
				pData[3] = (temp & 0xff);
				pData[4] = (info->sectorbytes>>24)&0xff;
				pData[5] = (info->sectorbytes>>16)&0xff;
				pData[6] = (info->sectorbytes>>8)&0xff;
				pData[7] = (info->sectorbytes & 0xff);
			}
			break;

		default:
			logerror("SCSIHD: readback of data from unknown command %d\n", our_this->last_command);
			break;
	}
}

void scsihd_write_data(SCSIHd *our_this, int bytes, UINT8 *pData)
{
	switch (our_this->last_command)
	{
		case 0x0a:	// WRITE (6 byte)
			if ((our_this->disk) && (our_this->blocks))
			{
				while (bytes > 0)
				{
					if (!hard_disk_write(our_this->disk, our_this->lba, pData))
					{
						logerror("SCSIHD: HD write error!\n");
					}
					our_this->lba++;
					our_this->last_lba = our_this->lba;
					our_this->blocks--;
					bytes -= 512;
					pData += 512;
				}
			}
			break;
	}
}

int scsihd_dispatch(int operation, void *file, INT64 intparm, UINT8 *ptrparm)
{
	SCSIHd *instance;
	void **ptrresult;

	switch (operation)
	{
		case SCSIOP_EXEC_COMMAND:
			return scsihd_exec_command((SCSIHd *)file, ptrparm);
			break;

		case SCSIOP_READ_DATA:
			scsihd_read_data((SCSIHd *)file, intparm, ptrparm);
			break;

		case SCSIOP_WRITE_DATA:
			scsihd_write_data((SCSIHd *)file, intparm, ptrparm);
			break;

		case SCSIOP_ALLOC_INSTANCE:
			instance = (SCSIHd *)malloc_or_die(sizeof(SCSIHd));

			instance->lba = 0;
			instance->blocks = 0;

			memset(instance->inquiry_buffer, 0, sizeof(instance->inquiry_buffer));
			instance->inquiry_buffer[0] = 0x00; // device is direct-access (e.g. hard disk)
			instance->inquiry_buffer[1] = 0x00; // media is not removable
			instance->inquiry_buffer[2] = 0x05; // device complies with SPC-3 standard
			instance->inquiry_buffer[3] = 0x02; // response data format = SPC-3 standard
			// Apple HD SC setup utility needs to see this
			strcpy((char *)&instance->inquiry_buffer[8], " SEAGATE");
			strcpy((char *)&instance->inquiry_buffer[16], "          ST225N");
			strcpy((char *)&instance->inquiry_buffer[32], "1.0");

			#ifdef MESS
			instance->disk = mess_hd_get_hard_disk_file_by_number(intparm);
			#else
			instance->disk = hard_disk_open(get_disk_handle(intparm));

			if (!instance->disk)
			{
				logerror("SCSIHD: no HD found!\n");
			}
			#endif

			ptrresult = file;
			*ptrresult = instance;
			break;

		case SCSIOP_DELETE_INSTANCE:
			free(file);
			break;

		case SCSIOP_GET_DEVICE:
			ptrresult = (void **)ptrparm;
			instance = (SCSIHd *)file;
			*ptrresult = instance->disk;
			break;

		case SCSIOP_SET_DEVICE:
			instance = (SCSIHd *)file;
			instance->disk = (hard_disk_file *)ptrparm;
			break;


		case SCSIOP_GET_INQUIRY_BUFFER:
			ptrresult = (void **)ptrparm;
			instance = (SCSIHd *)file;

			if( intparm > sizeof( instance->inquiry_buffer ) )
			{
				*ptrresult = NULL;
			}
			else
			{
				*ptrresult = instance->inquiry_buffer;
			}
			break;
	}

	return 0;
}
