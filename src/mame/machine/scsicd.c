/***************************************************************************

 scsicd.c - Implementation of a SCSI CD-ROM device, using MAME's cdrom.c primitives

***************************************************************************/

#include "driver.h"
#include "scsidev.h"
#include "cdrom.h"
#include "sound/cdda.h"
#ifdef MESS
#include "devices/chd_cd.h"
#endif

typedef struct
{
	UINT32 lba, blocks, last_lba, bytes_per_sector, num_subblocks, cur_subblock, play_err_flag;
	int last_command, last_retval;
 	cdrom_file *cdrom;
	UINT8 last_packet[16];
} SCSICd;


static void phys_frame_to_msf(int phys_frame, int *m, int *s, int *f)
{
	*m = phys_frame / (60*75);
	phys_frame -= (*m * 60 * 75);
	*s = phys_frame / 75;
	*f = phys_frame % 75;
}


// scsicd_exec_command
//
// Execute a SCSI command passed in via pCmdBuf.

int scsicd_exec_command(SCSICd *our_this, UINT8 *pCmdBuf)
{
	cdrom_file *cdrom = our_this->cdrom;
	int retval = 12, trk, temp;
	int cddanum;

	// remember the last command for the data transfer phase
	our_this->last_command = pCmdBuf[0];

	// remember the actual command packet too
	memcpy(our_this->last_packet, pCmdBuf, 16);

	switch (our_this->last_command)
	{
		case 0:		// TEST UNIT READY
			retval = 12;
			break;
		case 3: 	// REQUEST SENSE
			logerror("SCSICD: REQUEST SENSE\n");

			temp = pCmdBuf[5]<<8 | pCmdBuf[4];
			if (temp < 18)
			{
				retval = temp;
			}
			else
			{
				retval = 18;
			}
			break;
		case 0x12:	// INQUIRY
			break;
		case 0x15:	// MODE SELECT (6)
			logerror("SCSICD: MODE SELECT (6) length %x control %x\n", pCmdBuf[4], pCmdBuf[5]);
			retval = 0x18;
			break;
		case 0x1a:	// MODE SENSE
			retval = 8;
			break;
		case 0x1b:	// START/STOP UNIT
			cddanum = cdda_num_from_cdrom(cdrom);
			if (cddanum != -1)
				cdda_stop_audio(cddanum);
			retval = 0;
			break;
		case 0x25:	// READ CD-ROM CAPACITY
			retval = 8;
			break;
		case 0x28: 	// READ (10 byte)
			cddanum = cdda_num_from_cdrom(cdrom);
			if (cddanum != -1)
				cdda_stop_audio(cddanum);
			our_this->lba = pCmdBuf[2]<<24 | pCmdBuf[3]<<16 | pCmdBuf[4]<<8 | pCmdBuf[5];
			our_this->blocks = pCmdBuf[7]<<8 | pCmdBuf[8];

			retval = our_this->blocks * our_this->bytes_per_sector;

			if (our_this->num_subblocks > 1)
			{
				our_this->cur_subblock = our_this->lba % our_this->num_subblocks;
				our_this->lba /= our_this->num_subblocks;
			}
			else
			{
				our_this->cur_subblock = 0;
			}

			/* convert physical frame to CHD */
			if (cdrom)
			{
				if (cddanum != -1)
					cdda_stop_audio(cddanum);
			}

			logerror("SCSICD: READ (10) at LBA %x for %d blocks (%d bytes)\n", our_this->lba, our_this->blocks, retval);
			break;
		case 0x42:	// READ SUB-CHANNEL
	//                      logerror("SCSICD: READ SUB-CHANNEL type %d\n", pCmdBuf[3]);
			retval = 16;
			break;
		case 0x43:	// READ TOC
		{
			int start_trk = pCmdBuf[6];
			int end_trk = cdrom_get_last_track(cdrom);
			int allocation_length = pCmdBuf[7]<<8 | pCmdBuf[8];

			if( start_trk == 0 )
			{
				start_trk = 1;
			}
			if( start_trk == 0xaa )
			{
				end_trk = start_trk;
			}

			retval = 4 + ( 8 * ( ( end_trk - start_trk ) + 1 ) );
			if( retval > allocation_length )
			{
				retval = allocation_length;
			}
			else if( retval < 4 )
			{
				retval = 4;
			}

			cddanum = cdda_num_from_cdrom(cdrom);
			if (cddanum != -1)
				cdda_stop_audio(cddanum);
			break;
		}
		case 0x45:	// PLAY AUDIO  (10 byte)
			our_this->lba = pCmdBuf[2]<<24 | pCmdBuf[3]<<16 | pCmdBuf[4]<<8 | pCmdBuf[5];
			our_this->blocks = pCmdBuf[7]<<8 | pCmdBuf[8];

			// special cases: lba of 0 means MSF of 00:02:00
			if (our_this->lba == 0)
			{
				our_this->lba = 150;
			}
			else if (our_this->lba == 0xffffffff)
			{
				logerror("SCSICD: play audio from current not implemented!\n");
			}

			logerror("SCSICD: PLAY AUDIO (10) at LBA %x for %x blocks\n", our_this->lba, our_this->blocks);

			trk = cdrom_get_track(cdrom, our_this->lba);

			if (cdrom_get_track_type(cdrom, trk) == CD_TRACK_AUDIO)
			{
				our_this->play_err_flag = 0;
				cddanum = cdda_num_from_cdrom(cdrom);
				if (cddanum != -1)
					cdda_start_audio(cddanum, our_this->lba, our_this->blocks);
			}
			else
			{
				logerror("SCSICD: track is NOT audio!\n");
				our_this->play_err_flag = 1;
			}
			retval = 0;
			break;
		case 0x48:	// PLAY AUDIO TRACK/INDEX
			// be careful: tracks here are zero-based, but the SCSI command
			// uses the real CD track number which is 1-based!
			our_this->lba = cdrom_get_track_start(cdrom, pCmdBuf[4]-1);
			our_this->blocks = cdrom_get_track_start(cdrom, pCmdBuf[7]-1) - our_this->lba;
			if (pCmdBuf[4] > pCmdBuf[7])
			{
				our_this->blocks = 0;
			}

			if (pCmdBuf[4] == pCmdBuf[7])
			{
				our_this->blocks = cdrom_get_track_start(cdrom, pCmdBuf[4]) - our_this->lba;
			}

			if (our_this->blocks && cdrom)
			{
				cddanum = cdda_num_from_cdrom(cdrom);
				if (cddanum != -1)
					cdda_start_audio(cddanum, our_this->lba, our_this->blocks);
			}

			logerror("SCSICD: PLAY AUDIO T/I: strk %d idx %d etrk %d idx %d frames %d\n", pCmdBuf[4], pCmdBuf[5], pCmdBuf[7], pCmdBuf[8], our_this->blocks);
			retval = 0;
			break;
		case 0x4b:	// PAUSE/RESUME
			if (cdrom)
			{
				cddanum = cdda_num_from_cdrom(cdrom);
				if (cddanum != -1)
					cdda_pause_audio(cddanum, (pCmdBuf[8] & 0x01) ^ 0x01);
			}

			logerror("SCSICD: PAUSE/RESUME: %s\n", pCmdBuf[8]&1 ? "RESUME" : "PAUSE");
			retval = 0;
			break;
		case 0x55:	// MODE SELECT
			logerror("SCSICD: MODE SELECT length %x control %x\n", pCmdBuf[7]<<8 | pCmdBuf[8], pCmdBuf[1]);
			retval = 0x18;
			break;
		case 0x5a:	// MODE SENSE
			retval = 0x18;
			break;
		case 0xa5:	// PLAY AUDIO  (12 byte)
			our_this->lba = pCmdBuf[2]<<24 | pCmdBuf[3]<<16 | pCmdBuf[4]<<8 | pCmdBuf[5];
			our_this->blocks = pCmdBuf[6]<<24 | pCmdBuf[7]<<16 | pCmdBuf[8]<<8 | pCmdBuf[9];

			// special cases: lba of 0 means MSF of 00:02:00
			if (our_this->lba == 0)
			{
				our_this->lba = 150;
			}
			else if (our_this->lba == 0xffffffff)
			{
				logerror("SCSICD: play audio from current not implemented!\n");
			}

			logerror("SCSICD: PLAY AUDIO (12) at LBA %x for %x blocks\n", our_this->lba, our_this->blocks);

			trk = cdrom_get_track(cdrom, our_this->lba);

			if (cdrom_get_track_type(cdrom, trk) == CD_TRACK_AUDIO)
			{
				our_this->play_err_flag = 0;
				cddanum = cdda_num_from_cdrom(cdrom);
				if (cddanum != -1)
					cdda_start_audio(cddanum, our_this->lba, our_this->blocks);
			}
			else
			{
				logerror("SCSICD: track is NOT audio!\n");
				our_this->play_err_flag = 1;
			}
			retval = 0;
			break;
		case 0xa8: 	// READ (12 byte)
			cddanum = cdda_num_from_cdrom(cdrom);
			if (cddanum != -1)
				cdda_stop_audio(cddanum);
			our_this->lba = pCmdBuf[2]<<24 | pCmdBuf[3]<<16 | pCmdBuf[4]<<8 | pCmdBuf[5];
			our_this->blocks = pCmdBuf[7]<<16 | pCmdBuf[8]<<8 | pCmdBuf[9];

			retval = our_this->blocks * our_this->bytes_per_sector;

			if (our_this->num_subblocks > 1)
			{
				our_this->cur_subblock = our_this->lba % our_this->num_subblocks;
				our_this->lba /= our_this->num_subblocks;
			}
			else
			{
				our_this->cur_subblock = 0;
			}

			/* convert physical frame to CHD */
			if (cdrom)
			{
				cdda_stop_audio(cddanum);
			}

			logerror("SCSICD: READ (12) at LBA %x for %x blocks (%x bytes)\n", our_this->lba, our_this->blocks, retval);
			break;
		case 0xbb:	// SET CD SPEED
			logerror("SCSICD: SET CD SPEED to %d kbytes/sec.\n", pCmdBuf[2]<<8 | pCmdBuf[3]);
			break;
		default:
			logerror("SCSICD: unknown SCSI command %x!\n", our_this->last_command);
			break;
	}

	our_this->last_retval = retval;
	return retval;
}

// scsicd_read_data
//
// Read data from the device resulting from the execution of a command

void scsicd_read_data(SCSICd *our_this, int bytes, UINT8 *pData)
{
	int i;
	UINT32 last_phys_frame;
	UINT8 *fifo = our_this->last_packet;
	cdrom_file *cdrom = our_this->cdrom;
	UINT32 temp;
	UINT8 tmp_buffer[2048];
	int cddanum;

	switch (our_this->last_command)
	{
		case 0x03:	// REQUEST SENSE
			logerror("SCSICD: Reading REQUEST SENSE data\n");

			pData[0] = 0x71;	// deferred error

			if (our_this->last_retval == -1)
			{
				our_this->last_retval = 16;
			}

			for (i = 1; i < our_this->last_retval; i++)
			{
				pData[i] = 0;
			}

			cddanum = cdda_num_from_cdrom(cdrom);
			if (cddanum != -1 && cdda_audio_active(cddanum))
			{
				pData[12] = 0x00;
				pData[13] = 0x11;	// AUDIO PLAY OPERATION IN PROGRESS
			}
			else if (our_this->play_err_flag)
			{
				our_this->play_err_flag = 0;
				pData[12] = 0x64;	// ILLEGAL MODE FOR THIS TRACK
				pData[13] = 0x00;
			}
			// (else 00/00 means no error to report)
			break;

		case 0x12:	// INQUIRY
			pData[0] = 0x05;	// device is present, device is CD/DVD (MMC-3)
			pData[1] = 0x80;	// media is removable
			pData[2] = 0x05;	// device complies with SPC-3 standard
			pData[3] = 0x02;	// response data format = SPC-3 standard
			memset(&pData[8], 0, 8*3);
			strcpy((char *)&pData[8], "Sony");	// some Konami games freak out if this isn't "Sony", so we'll lie
			strcpy((char *)&pData[16], "CDU-76S");	// this is the actual drive on my Nagano '98 board
			strcpy((char *)&pData[32], "1.0");
			break;

		case 0x25:	// READ CAPACITY
			logerror("SCSICD: READ CAPACITY\n");

			temp = cdrom_get_track_start(cdrom, 0xaa);
			temp--;	// return the last used block on the disc

			pData[0] = (temp>>24) & 0xff;
			pData[1] = (temp>>16) & 0xff;
			pData[2] = (temp>>8) & 0xff;
			pData[3] = (temp & 0xff);
			pData[4] = 0;
			pData[5] = 0;
			pData[6] = (our_this->bytes_per_sector>>8)&0xff;
			pData[7] = (our_this->bytes_per_sector & 0xff);
			break;

		case 0x28:	// READ (10 byte)
		case 0xa8:	// READ (12 byte)
			logerror("SCSICD: read %x bytes, \n", bytes);
			if ((our_this->cdrom) && (our_this->blocks))
			{
				while (bytes > 0)
				{
					if (!cdrom_read_data(our_this->cdrom, our_this->lba, tmp_buffer, CD_TRACK_MODE1))
					{
						logerror("SCSICD: CD read error!\n");
					}

					logerror("True LBA: %d, buffer half: %d\n", our_this->lba, our_this->cur_subblock * our_this->bytes_per_sector);

					memcpy(pData, &tmp_buffer[our_this->cur_subblock * our_this->bytes_per_sector], our_this->bytes_per_sector);

					our_this->cur_subblock++;
					if (our_this->cur_subblock >= our_this->num_subblocks)
					{
						our_this->cur_subblock = 0;

						our_this->lba++;
						our_this->blocks--;
					}

					our_this->last_lba = our_this->lba;
					bytes -= our_this->bytes_per_sector;
					pData += our_this->bytes_per_sector;
				}
			}
			break;

		case 0x42:	// READ SUB-CHANNEL
			switch (fifo[3])
			{
				case 1:	// return current position
				{
					int audio_active;
					int msf;

					if (!cdrom)
					{
						return;
					}

					logerror("SCSICD: READ SUB-CHANNEL Time = %x, SUBQ = %x\n", fifo[1], fifo[2]);

					msf = fifo[1] & 0x2;

					cddanum = cdda_num_from_cdrom(cdrom);
					audio_active = cddanum != -1 && cdda_audio_active(cddanum);
					if (audio_active)
					{
						if (cdda_audio_paused(cddanum))
						{
							pData[1] = 0x12;		// audio is paused
						}
						else
						{
							pData[1] = 0x11;		// audio in progress
						}
					}
					else
					{
						if (cddanum != -1 && cdda_audio_ended(cddanum))
						{
							pData[1] = 0x13;	// ended successfully
						}
						else
						{
//                          pData[1] = 0x14;    // stopped due to error
							pData[1] = 0x15;	// No current audio status to return
						}
					}

					// if audio is playing, get the latest LBA from the CDROM layer
					if (audio_active)
					{
						our_this->last_lba = cdda_get_audio_lba(cddanum);
					}
					else
					{
						our_this->last_lba = 0;
					}

					pData[2] = 0;
					pData[3] = 12;		// data length
					pData[4] = 0x01;	// sub-channel format code
					pData[5] = 0x10 | (audio_active ? 0 : 4);
					pData[6] = cdrom_get_track(cdrom, our_this->last_lba) + 1;	// track
					pData[7] = 0;	// index

					last_phys_frame = our_this->last_lba;

					if (msf)
					{
						int m,s,f;
						phys_frame_to_msf(last_phys_frame, &m, &s, &f);
						pData[8] = 0;
						pData[9] = m;
						pData[10] = s;
						pData[11] = f;
					}
					else
					{
						pData[8] = last_phys_frame>>24;
						pData[9] = (last_phys_frame>>16)&0xff;
						pData[10] = (last_phys_frame>>8)&0xff;
						pData[11] = last_phys_frame&0xff;
					}

					last_phys_frame -= cdrom_get_track_start(cdrom, pData[6] - 1);

					if (msf)
					{
						int m,s,f;
						phys_frame_to_msf(last_phys_frame, &m, &s, &f);
						pData[12] = 0;
						pData[13] = m;
						pData[14] = s;
						pData[15] = f;
					}
					else
					{
						pData[12] = last_phys_frame>>24;
						pData[13] = (last_phys_frame>>16)&0xff;
						pData[14] = (last_phys_frame>>8)&0xff;
						pData[15] = last_phys_frame&0xff;
					}
					break;
				}
				default:
					logerror("SCSICD: Unknown subchannel type %d requested\n", fifo[3]);
			}
			break;

		case 0x43:	// READ TOC
			/*
                Track numbers are problematic here: 0 = lead-in, 0xaa = lead-out.
                That makes sense in terms of how real-world CDs are referred to, but
                our internal routines for tracks use "0" as track 1.  That probably
                should be fixed...
            */
			logerror("SCSICD: READ TOC, format = %d time=%d\n", fifo[2]&0xf,(fifo[1]>>1)&1);
			switch (fifo[2] & 0x0f)
			{
				case 0:		// normal
					{
						int start_trk;
						int end_trk;
						int len;
						int in_len;
						int dptr;
						UINT32 tstart;

						start_trk = fifo[6];
						if( start_trk == 0 )
						{
							start_trk = 1;
						}

						end_trk = cdrom_get_last_track(cdrom);
						len = (end_trk * 8) + 2;

						// the returned TOC DATA LENGTH must be the full amount,
						// regardless of how much we're able to pass back due to in_len
						dptr = 0;
						pData[dptr++] = (len>>8) & 0xff;
						pData[dptr++] = (len & 0xff);
						pData[dptr++] = 1;
						pData[dptr++] = end_trk;

						if( start_trk == 0xaa )
						{
							end_trk = 0xaa;
						}

						in_len = fifo[7]<<8 | fifo[8];

						for (i = start_trk; i <= end_trk; i++)
						{
							int cdrom_track = i;
							if( cdrom_track != 0xaa )
							{
								cdrom_track--;
							}

							if( dptr >= in_len )
							{
								break;
							}

							pData[dptr++] = 0;
							pData[dptr++] = cdrom_get_adr_control(cdrom, cdrom_track);
							pData[dptr++] = i;
							pData[dptr++] = 0;

							tstart = cdrom_get_track_start(cdrom, cdrom_track);
							if ((fifo[1]&2)>>1)
								tstart = lba_to_msf(tstart);
							pData[dptr++] = (tstart>>24) & 0xff;
							pData[dptr++] = (tstart>>16) & 0xff;
							pData[dptr++] = (tstart>>8) & 0xff;
							pData[dptr++] = (tstart & 0xff);
						}
					}
					break;
				default:
					logerror("SCSICD: Unhandled READ TOC format %d\n", fifo[2]&0xf);
					break;
			}
			break;

		case 0x48:	// PLAY AUDIO TRACK/INDEX
			break;

		case 0x5a:	// MODE SENSE
			logerror("SCSICD: MODE SENSE page code = %x, PC = %x\n", fifo[2] & 0x3f, (fifo[2]&0xc0)>>6);

			switch (fifo[2] & 0x3f)
			{
				case 0xe:	// CD Audio control page
					pData[0] = 0x8e;	// page E, parameter is savable
					pData[1] = 0x0e;	// page length
					pData[2] = 0x04;	// IMMED = 1, SOTC = 0
					pData[3] = pData[4] = pData[5] = pData[6] = pData[7] = 0; // reserved

					// connect each audio channel to 1 output port
					pData[8] = 1;
					pData[10] = 2;
					pData[12] = 4;
					pData[14] = 8;

					// indicate max volume
					pData[9] = pData[11] = pData[13] = pData[15] = 0xff;
					break;

				default:
					logerror("SCSICD: MODE SENSE unknown page %x\n", fifo[2] & 0x3f);
					break;
			}
			break;
	}
}

// scsicd_write_data
//
// Write data to the CD-ROM device as part of the execution of a command

void scsicd_write_data(SCSICd *our_this, int bytes, UINT8 *pData)
{
	switch (our_this->last_command)
	{
		case 0x15:	// MODE SELECT (6)
		case 0x55:	// MODE SELECT
			logerror("SCSICD: MODE SELECT page %x\n", pData[0] & 0x3f);

			switch (pData[0] & 0x3f)
			{
				case 0x0:	// vendor-specific
					// check for SGI extension to force 512-byte blocks
					if ((pData[3] == 8) && (pData[10] == 2))
					{
						logerror("SCSICD: Experimental SGI 512-byte block extension enabled\n");

						our_this->bytes_per_sector = 512;
						our_this->num_subblocks = 4;
					}
					else
					{
						logerror("SCSICD: Unknown vendor-specific page!\n");
					}
					break;

				case 0xe:	// audio page
					logerror("Ch 0 route: %x vol: %x\n", pData[8], pData[9]);
					logerror("Ch 1 route: %x vol: %x\n", pData[10], pData[11]);
					logerror("Ch 2 route: %x vol: %x\n", pData[12], pData[13]);
					logerror("Ch 3 route: %x vol: %x\n", pData[14], pData[15]);
					break;
			}
			break;
	}
}

int scsicd_dispatch(int operation, void *file, INT64 intparm, UINT8 *ptrparm)
{
	SCSICd *instance, **result;
	cdrom_file **devptr;

	switch (operation)
	{
		case SCSIOP_EXEC_COMMAND:
			return scsicd_exec_command((SCSICd *)file, ptrparm);
			break;

		case SCSIOP_READ_DATA:
			scsicd_read_data((SCSICd *)file, intparm, ptrparm);
			break;

		case SCSIOP_WRITE_DATA:
			scsicd_write_data((SCSICd *)file, intparm, ptrparm);
			break;

		case SCSIOP_ALLOC_INSTANCE:
			instance = (SCSICd *)malloc(sizeof(SCSICd));

			instance->lba = 0;
			instance->blocks = 0;
			instance->bytes_per_sector = 2048;
			instance->num_subblocks = 1;
 			instance->cur_subblock = 0;
			instance->play_err_flag = 0;

			#ifdef MESS
			instance->cdrom = mess_cd_get_cdrom_file_by_number(intparm);
			#else
			instance->cdrom = cdrom_open(get_disk_handle(intparm));

			if (!instance->cdrom)
			{
				logerror("SCSICD: no CD found!\n");
			}
			#endif

			result = (SCSICd **) file;
			*result = instance;
			break;

		case SCSIOP_DELETE_INSTANCE:
			free(file);
			break;

		case SCSIOP_GET_DEVICE:
			devptr = (cdrom_file **)ptrparm;
			instance = (SCSICd *)file;
			*devptr = instance->cdrom;
			break;

		case SCSIOP_SET_DEVICE:
			instance = (SCSICd *)file;
			instance->cdrom = (cdrom_file *)ptrparm;
			break;
	}

	return 0;
}
