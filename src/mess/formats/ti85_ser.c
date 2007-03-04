#include "driver.h"
#include "ti85_ser.h"

#define TI85_PC_OK_PACKET_SIZE	4
#define TI85_PC_END_PACKET_SIZE	4

UINT8 ti85_pc_ok_packet[] = {0x05, 0x56, 0x00, 0x00};
UINT8 ti86_pc_ok_packet[] = {0x06, 0x56, 0x00, 0x00};
UINT8 ti85_pc_continue_packet[] = {0x05, 0x09, 0x00, 0x00};
UINT8 ti86_pc_continue_packet[] = {0x06, 0x09, 0x00, 0x00};
UINT8 ti85_pc_screen_request_packet[] = {0x05, 0x6d, 0x00, 0x00};
static UINT8 ti85_pc_end_packet[] = {0x05, 0x92, 0x00, 0x00};
static UINT8 ti86_pc_end_packet[] = {0x06, 0x92, 0x00, 0x00};

UINT8 ti85_file_signature[] = {0x2a, 0x2a, 0x54, 0x49, 0x38, 0x35, 0x2a, 0x2a, 0x1a, 0x0c, 0x00};
UINT8 ti86_file_signature[] = {0x2a, 0x2a, 0x54, 0x49, 0x38, 0x36, 0x2a, 0x2a, 0x1a, 0x0a, 0x00};

int ti85_serial_transfer_type = TI85_SEND_VARIABLES;

UINT16 ti85_calculate_checksum(UINT8* data, unsigned int size)
{
	UINT16 checksum = 0;
	unsigned int i;
	
	for (i = 0; i<size; i++)
		checksum += data[i];
	return checksum;
}

UINT16 ti85_variables_count (UINT8 * ti85_data, unsigned int ti85_data_size)
{
	unsigned int pos, head_size, var_size;
	UINT16 number_of_entries = 0;
	pos = TI85_HEADER_SIZE;
	while (pos < ti85_data_size-2)
	{
		head_size = ti85_data[pos+0x00] + ti85_data[pos+0x01]*256;
		var_size = ti85_data[pos+0x02] + ti85_data[pos+0x03]*256;
		pos += head_size+var_size+4;
		number_of_entries++;
	}
	return number_of_entries;
}

static void ti85_backup_read (UINT8 * ti85_data, unsigned int ti85_data_size, ti85_entry * ti85_entries)
{
	unsigned int pos = 0x42+2;
        
	ti85_entries[0].head_size = 0x09;
	ti85_entries[0].data_size = ti85_data[0x39] + ti85_data[0x3a]*256;
	ti85_entries[0].offset = pos;
	pos += ti85_entries[0].data_size + 2;
	ti85_entries[1].head_size = 0;
	ti85_entries[1].data_size = ti85_data[0x3c] + ti85_data[0x3d]*256;
	ti85_entries[1].offset = pos;
	pos += ti85_entries[1].data_size + 2;
	ti85_entries[2].head_size = 0;
	ti85_entries[2].data_size = ti85_data[0x3e] + ti85_data[0x3f]*256;
	ti85_entries[2].offset = pos;
}

void ti85_variables_read (UINT8 * ti85_data, unsigned int ti85_data_size, ti85_entry * ti85_entries)
{
	unsigned int pos, i=0;

	pos = TI85_HEADER_SIZE;
	while (pos < ti85_data_size-2)
	{
		ti85_entries[i].head_size = ti85_data[pos+0x00] + ti85_data[pos+0x01]*256;
		ti85_entries[i].data_size = ti85_data[pos+0x02] + ti85_data[pos+0x03]*256;
		ti85_entries[i].type = ti85_data[pos+0x04];
		ti85_entries[i].name_size = ti85_data[pos+0x05];
		ti85_entries[i].offset = pos;
		pos += ti85_entries[i].head_size+ti85_entries[i].data_size+4;
		i++;
	}
}


void ti85_convert_data_to_stream (UINT8* file_data, unsigned int size, UINT8* serial_data)
{
	unsigned int i, bits;

	for (i=0; i<size; i++)
 		for (bits = 0; bits < 8; bits++)
			serial_data[i*8+bits] = (file_data[i]>>bits) & 0x01;
}

void ti85_convert_stream_to_data (UINT8* serial_data, UINT32 size, UINT8* data)
{
	UINT32 i;
	UINT8 bits;

	size = size/8;

	for (i=0; i<size; i++)
	{
		data[i] = 0;
 		for (bits = 0; bits < 8; bits++)
			data[i] |= serial_data[i*8+bits]<<bits;
	}
}

int ti85_convert_file_data_to_serial_stream (UINT8* file_data, unsigned int file_size, ti85_serial_data*  serial_data, char* calc_type)
{
	UINT16 i;
	UINT16 number_of_entries;

	ti85_entry* ti85_entries = NULL;

	UINT8* temp_data_to_convert = NULL;
	UINT16 checksum;

	if (!strncmp(calc_type, "ti85", 4))
		if (strncmp((char *) file_data, (char *) ti85_file_signature, 11))
			return 0;
	if (!strncmp(calc_type, "ti86", 4))
		if (strncmp((char *) file_data, (char *) ti86_file_signature, 11))
			return 0;

	/*Serial stream preparing*/
	serial_data->end = NULL;

	number_of_entries = (file_data[0x3b]==0x1d) ? 3 : ti85_variables_count(file_data, file_size);
	if (!number_of_entries) return 0;

	serial_data->variables = malloc(sizeof(ti85_serial_variable)*number_of_entries);
	if (!serial_data->variables) return 0;

	for (i=0; i<number_of_entries; i++)
	{
		serial_data->variables[i].header = NULL;
		serial_data->variables[i].ok = NULL;
		serial_data->variables[i].data = NULL;
	}

	serial_data->number_of_variables = number_of_entries;

	ti85_entries = (ti85_entry*) malloc (sizeof(ti85_entry)*number_of_entries);
	if (!ti85_entries) return 0;

	if (file_data[0x3b]==0x1d)
	{
		ti85_serial_transfer_type = TI85_SEND_BACKUP;
		ti85_backup_read (file_data, file_size, ti85_entries);
	}
	else
	{
		ti85_serial_transfer_type = TI85_SEND_VARIABLES;
		ti85_variables_read (file_data, file_size, ti85_entries);
	}
	for (i=0; i<number_of_entries; i++)
	{
		/*Header packet*/
		if (file_data[0x3b]==0x1d)
		{
			if (!i)
			{
				temp_data_to_convert = (UINT8*) malloc (0x0f);
				if (!temp_data_to_convert)
				{
					free (ti85_entries);
					return 0;
				}
				serial_data->variables[i].header = (UINT8*) malloc (0x0f*8);

				if (!serial_data->variables[i].header)
				{
					free (ti85_entries);
					free (temp_data_to_convert);
					return 0;
				}
				serial_data->variables[i].header_size = 0x0f*8;
				temp_data_to_convert[0] = 0x05;	//PC sends
				temp_data_to_convert[1] = 0x06;	//header
				memcpy(	temp_data_to_convert+0x02, file_data+0x37, 0x0b);
				checksum = ti85_calculate_checksum(temp_data_to_convert+4, 0x09);
				temp_data_to_convert[13] = checksum&0x00ff;
				temp_data_to_convert[14] = (checksum&0xff00)>>8;
				ti85_convert_data_to_stream(temp_data_to_convert, 0x0f, serial_data->variables[i].header);
				free(temp_data_to_convert);
			}
			else
			{
				serial_data->variables[i].header = NULL;
				serial_data->variables[i].header_size = 0;
			}
		}
		else
		{
			temp_data_to_convert = (UINT8*) malloc (10+ti85_entries[i].name_size);
			if (!temp_data_to_convert)
			{
				free (ti85_entries);
				return 0;
			}
			serial_data->variables[i].header = (UINT8*) malloc ((10+ti85_entries[i].name_size)*8);
			if (!serial_data->variables[i].header)
			{
				free (temp_data_to_convert);
				free (ti85_entries);
				return 0;
			}
			serial_data->variables[i].header_size = (10+ti85_entries[i].name_size)*8;

			if (!strncmp(calc_type, "ti85", 4))
				temp_data_to_convert[0] = 0x05;	//PC to TI-85
			if (!strncmp(calc_type, "ti86", 4))
				temp_data_to_convert[0] = 0x06;	//PC to TI-86
			temp_data_to_convert[1] = 0x06;	//header
			temp_data_to_convert[2] = (4+ti85_entries[i].name_size)&0x00ff;
			temp_data_to_convert[3] = ((4+ti85_entries[i].name_size)&0xff00)>>8;
			temp_data_to_convert[4] = (ti85_entries[i].data_size)&0x00ff;
			temp_data_to_convert[5] = ((ti85_entries[i].data_size)&0xff00)>>8;
			temp_data_to_convert[6] = ti85_entries[i].type;
			temp_data_to_convert[7] = ti85_entries[i].name_size;
			memcpy(temp_data_to_convert+8, file_data+ti85_entries[i].offset+0x06, ti85_entries[i].name_size);
			checksum = ti85_calculate_checksum(temp_data_to_convert+4,ti85_entries[i].name_size+4);
			temp_data_to_convert[10+ti85_entries[i].name_size-2] = checksum&0x00ff;
			temp_data_to_convert[10+ti85_entries[i].name_size-1] = (checksum&0xff00)>>8;
			ti85_convert_data_to_stream(temp_data_to_convert, 10+ti85_entries[i].name_size, serial_data->variables[i].header);
			free(temp_data_to_convert);
		}

		/*OK packet*/
		serial_data->variables[i].ok = (UINT8*) malloc (TI85_PC_OK_PACKET_SIZE*8);
		if (!serial_data->variables[i].ok)
		{
			free (ti85_entries);
			return 0;
		}
		if (!strncmp(calc_type, "ti85", 4))
			ti85_convert_data_to_stream(ti85_pc_ok_packet, TI85_PC_OK_PACKET_SIZE, serial_data->variables[i].ok);
		if (!strncmp(calc_type, "ti86", 4))
			ti85_convert_data_to_stream(ti86_pc_ok_packet, TI85_PC_OK_PACKET_SIZE, serial_data->variables[i].ok);
		serial_data->variables[i].ok_size = TI85_PC_OK_PACKET_SIZE*8;

		/*Data packet*/
		temp_data_to_convert = (UINT8*) malloc (6+ti85_entries[i].data_size);
		if (!temp_data_to_convert)
		{
			free (ti85_entries);
			return 0;
		}
		serial_data->variables[i].data = (UINT8*) malloc ((6+ti85_entries[i].data_size)*8);
		if (!serial_data->variables[i].data)
		{
			free (temp_data_to_convert);
			free (ti85_entries);
			return 0;
		}
		serial_data->variables[i].data_size = (6+ti85_entries[i].data_size)*8;

		if (!strncmp(calc_type, "ti85", 4))
			temp_data_to_convert[0] = 0x05;	//PC to TI-85
		if (!strncmp(calc_type, "ti86", 4))
			temp_data_to_convert[0] = 0x06;	//PC to TI-86
		temp_data_to_convert[1] = 0x15;	//data
		temp_data_to_convert[2] = (ti85_entries[i].data_size)&0x00ff;
		temp_data_to_convert[3] = ((ti85_entries[i].data_size)&0xff00)>>8;
		if (file_data[0x3b]==0x1d)
			memcpy(temp_data_to_convert+4, file_data+ti85_entries[i].offset, ti85_entries[i].data_size);
		else
			memcpy(temp_data_to_convert+4, file_data+ti85_entries[i].offset+0x06+ti85_entries[i].name_size+0x02, ti85_entries[i].data_size);
		checksum = ti85_calculate_checksum(temp_data_to_convert+4,ti85_entries[i].data_size);
		temp_data_to_convert[6+ti85_entries[i].data_size-2] = checksum&0x00ff;
		temp_data_to_convert[6+ti85_entries[i].data_size-1] = (checksum&0xff00)>>8;
		ti85_convert_data_to_stream(temp_data_to_convert, 6+ti85_entries[i].data_size, serial_data->variables[i].data);
		free(temp_data_to_convert);
	}


	/*END packet*/
	serial_data->end = (UINT8*) malloc (TI85_PC_END_PACKET_SIZE*8);
	if (!serial_data->end)
	{
		free (ti85_entries);
		return 0;
	}
	if (!strncmp(calc_type, "ti85", 4))
		ti85_convert_data_to_stream(ti85_pc_end_packet, TI85_PC_END_PACKET_SIZE, serial_data->end);
	if (!strncmp(calc_type, "ti86", 4))
		ti85_convert_data_to_stream(ti86_pc_end_packet, TI85_PC_END_PACKET_SIZE, serial_data->end);
	serial_data->end_size = TI85_PC_END_PACKET_SIZE*8;

	free (ti85_entries);
	
	return 1;
}

void ti85_free_serial_stream (ti85_serial_data*  serial_data)
{
	UINT16 i;
	if (serial_data->variables)
	{
		for (i=0; i<serial_data->number_of_variables; i++)
		{
			if (serial_data->variables[i].header) free (serial_data->variables[i].header);
			if (serial_data->variables[i].ok) free (serial_data->variables[i].ok);
			if (serial_data->variables[i].data) free (serial_data->variables[i].data);
		}
		free (serial_data->variables);
		serial_data->variables = NULL;
	}
	serial_data->number_of_variables = 0;
	if (serial_data->end)
	{
		free (serial_data->end);
		serial_data->end = NULL;
	}
}
