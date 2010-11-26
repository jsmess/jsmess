
#include "emu.h"
#include "ti85_ser.h"

enum
{
	TI85_SEND_STOP,
	TI85_SEND_HEADER,
	TI85_RECEIVE_OK_1,
	TI85_RECEIVE_ANSWER_1,
	TI85_RECEIVE_ANSWER_2,
	TI85_RECEIVE_ANSWER_3,
	TI85_SEND_OK,
	TI85_SEND_DATA,
	TI85_RECEIVE_OK_2,
	TI85_SEND_END,
	TI85_RECEIVE_OK_3,
	TI85_RECEIVE_HEADER_1,
	TI85_PREPARE_VARIABLE_DATA,
	TI85_RECEIVE_HEADER_2,
	TI85_SEND_OK_1,
	TI85_SEND_CONTINUE,
	TI85_RECEIVE_OK,
	TI85_RECEIVE_DATA,
	TI85_SEND_OK_2,
	TI85_RECEIVE_END_OR_HEADER_1,
	TI85_SEND_OK_3,
	TI85_PREPARE_SCREEN_REQUEST,
	TI85_SEND_SCREEN_REQUEST
};

#define TI85_HEADER_SIZE		0x37

#define TI85_SEND_VARIABLES		1
#define TI85_SEND_BACKUP		2
#define TI85_RECEIVE_BACKUP		3
#define TI85_RECEIVE_VARIABLES	4
#define TI85_RECEIVE_SCREEN		5

#define TI85_PC_OK_PACKET_SIZE	4
#define TI85_PC_END_PACKET_SIZE	4

static const UINT8 ti85_pc_ok_packet[] = {0x05, 0x56, 0x00, 0x00};
static const UINT8 ti86_pc_ok_packet[] = {0x06, 0x56, 0x00, 0x00};
static const UINT8 ti85_pc_continue_packet[] = {0x05, 0x09, 0x00, 0x00};
static const UINT8 ti86_pc_continue_packet[] = {0x06, 0x09, 0x00, 0x00};
static const UINT8 ti85_pc_screen_request_packet[] = {0x05, 0x6d, 0x00, 0x00};
static const UINT8 ti85_pc_end_packet[] = {0x05, 0x92, 0x00, 0x00};
static const UINT8 ti86_pc_end_packet[] = {0x06, 0x92, 0x00, 0x00};

static const UINT8 ti85_file_signature[] = {0x2a, 0x2a, 0x54, 0x49, 0x38, 0x35, 0x2a, 0x2a, 0x1a, 0x0c, 0x00};
static const UINT8 ti86_file_signature[] = {0x2a, 0x2a, 0x54, 0x49, 0x38, 0x36, 0x2a, 0x2a, 0x1a, 0x0a, 0x00};

typedef struct {
	UINT16 head_size;
	UINT16 data_size;
	unsigned char type;
	unsigned char name_size;
	unsigned int offset;
} ti85_entry;


typedef struct {
	UINT8* header;
	UINT16 header_size;
	UINT8* ok;
	UINT16 ok_size;
	UINT8* data;
	UINT32 data_size;
} ti85_serial_variable;


typedef struct {
	ti85_serial_variable * variables;
	UINT8* end;
	UINT16 end_size;
	UINT16 number_of_variables;
} ti85_serial_data;


typedef struct
{
	UINT8	status;
	int		transfer_type;
	UINT8	red_out;			/* signal line from machine */
	UINT8	white_out;			/* signal line from machine */
	UINT8	red_in;				/* signal line to machine */
	UINT8	white_in;			/* signal line to machine */
	UINT8	*receive_buffer;
	UINT8	*receive_data;
	ti85_serial_data	stream;
	/* receive_data_counter and send_data_counter can be combined? */
	UINT32	receive_data_counter;			/* from ti85_receive_serial */
	UINT32	send_data_counter;				/* from ti85_send_serial */
	/* variables_variable_number, send_backup_variable_number, and backup_variable_number can be combined? */
	UINT16	variables_variable_number;		/* from ti85_send_variables */
	UINT16	send_backup_variable_number;	/* from ti85_send_backup */
	int		variable_number;				/* from ti85_receive_variables */
	UINT8	*var_data;						/* from ti85_receive_variables */
	UINT32	var_file_number;				/* from ti85_receive_variables */
	UINT8	*var_file_data;					/* from ti85_receive_variables */
	int		var_file_size;					/* from ti85_receive_variables */
	int		backup_variable_number;			/* from ti85_receive_backup */
	int		backup_data_size[3];			/* from ti85_receive_backup */
	UINT8	*backup_file_data;				/* from ti85_receive_backup */
	UINT32	backup_file_number;				/* from ti85_receive_backup */
	UINT32	image_file_number;				/* from ti85_receive_screen */
} ti85serial_state;


INLINE ti85serial_state *get_token(running_device *device)
{
	assert(device != NULL);
	assert((device->type() == TI85SERIAL) || (device->type() == TI86SERIAL));
	return (ti85serial_state *) downcast<legacy_device_base *>(device)->token();
}


static UINT16 ti85_calculate_checksum(const UINT8* data, unsigned int size)
{
	UINT16 checksum = 0;
	unsigned int i;

	for (i = 0; i<size; i++)
		checksum += data[i];
	return checksum;
}


static UINT16 ti85_variables_count (const UINT8 * ti85_data, unsigned int ti85_data_size)
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


static void ti85_free_serial_data_memory (running_device *device)
{
	ti85serial_state *ti85serial = get_token( device );

	if (ti85serial->receive_buffer)
	{
		free (ti85serial->receive_buffer);
		ti85serial->receive_buffer = NULL;
	}
	if (ti85serial->receive_data)
	{
		free (ti85serial->receive_data);
		ti85serial->receive_data = NULL;
	}
}


static int ti85_alloc_serial_data_memory (running_device *device, UINT32 size)
{
	ti85serial_state *ti85serial = get_token( device );

	if (!ti85serial->receive_buffer)
	{
		ti85serial->receive_buffer = (UINT8*) malloc (8*size*sizeof(UINT8));
		if (!ti85serial->receive_buffer)
			return 0;
	}

	if (!ti85serial->receive_data)
	{
		ti85serial->receive_data = (UINT8*) malloc (size * sizeof(UINT8));
		if (!ti85serial->receive_data)
		{
			free (ti85serial->receive_buffer);
			ti85serial->receive_buffer = NULL;
			return 0;
		}
	}
	return 1;
}


static void ti85_backup_read (const UINT8 * ti85_data, unsigned int ti85_data_size, ti85_entry * ti85_entries)
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


static void ti85_variables_read (const UINT8 * ti85_data, unsigned int ti85_data_size, ti85_entry * ti85_entries)
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


static int ti85_receive_serial (running_device *device, UINT8* received_data, UINT32 received_data_size)
{
	ti85serial_state *ti85serial = get_token( device );

	if (ti85serial->receive_data_counter >= received_data_size)
	{
		if (!ti85serial->red_out && !ti85serial->white_out)
		{
			ti85serial->red_in = ti85serial->white_in = 1;
			ti85serial->receive_data_counter = 0;
			return 0;
		}
		return 1;
	}

	if (ti85serial->red_in && ti85serial->white_in && (ti85serial->red_out!=ti85serial->white_out))
	{
		ti85serial->white_in = ti85serial->white_out;
		ti85serial->red_in = ti85serial->red_out;
		received_data[ti85serial->receive_data_counter] = ti85serial->white_out;
		return 1;
	}

	if (ti85serial->red_in!=ti85serial->white_in && !ti85serial->red_out && !ti85serial->white_out)
	{
		ti85serial->red_in = ti85serial->white_in = 1;
		ti85serial->receive_data_counter ++;
		return 1;
	}
	return 1;
}


static int ti85_send_serial(running_device *device, UINT8* serial_data, UINT32 serial_data_size)
{
	ti85serial_state *ti85serial = get_token( device );

	if (ti85serial->send_data_counter>=serial_data_size)
	{
		if (!ti85serial->red_out && !ti85serial->white_out)
		{
			ti85serial->red_in = ti85serial->white_in = 1;
			ti85serial->send_data_counter = 0;
			return 0;
		}
		ti85serial->red_in = ti85serial->white_in = 1;
		return 1;
	}

	if (ti85serial->red_in && ti85serial->white_in && !ti85serial->red_out && !ti85serial->white_out)
	{
		ti85serial->red_in =  serial_data[ti85serial->send_data_counter] ? 1 : 0;
		ti85serial->white_in = serial_data[ti85serial->send_data_counter] ? 0 : 1;
		return 1;
	}

	if ((ti85serial->red_in == ti85serial->red_out) && (ti85serial->white_in == ti85serial->white_out))
	{
		ti85serial->red_in = ti85serial->white_in = 1;
		ti85serial->send_data_counter++;
		return 1;
	}
	return 1;
}


static void ti85_convert_data_to_stream (const UINT8* file_data, unsigned int size, UINT8* serial_data)
{
	unsigned int i, bits;

	for (i=0; i<size; i++)
		for (bits = 0; bits < 8; bits++)
			serial_data[i*8+bits] = (file_data[i]>>bits) & 0x01;
}


static void ti85_convert_stream_to_data (const UINT8* serial_data, UINT32 size, UINT8* data)
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


static int ti85_convert_file_data_to_serial_stream (running_device *device, const UINT8* file_data, unsigned int file_size, ti85_serial_data*  serial_data)
{
	ti85serial_state *ti85serial = get_token( device );
	UINT16 i;
	UINT16 number_of_entries;

	ti85_entry* ti85_entries = NULL;

	UINT8* temp_data_to_convert = NULL;
	UINT16 checksum;

	if (device->type() == TI85SERIAL)
		if (strncmp((char *) file_data, (char *) ti85_file_signature, 11))
			return 0;
	if (device->type() == TI86SERIAL)
		if (strncmp((char *) file_data, (char *) ti86_file_signature, 11))
			return 0;

	/*Serial stream preparing*/
	serial_data->end = NULL;

	number_of_entries = (file_data[0x3b]==0x1d) ? 3 : ti85_variables_count(file_data, file_size);
	if (!number_of_entries) return 0;

	serial_data->variables = (ti85_serial_variable*)malloc(sizeof(ti85_serial_variable)*number_of_entries);
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
		ti85serial->transfer_type = TI85_SEND_BACKUP;
		ti85_backup_read (file_data, file_size, ti85_entries);
	}
	else
	{
		ti85serial->transfer_type = TI85_SEND_VARIABLES;
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

			if (device->type() == TI85SERIAL)
				temp_data_to_convert[0] = 0x05;	//PC to TI-85
			if (device->type() == TI86SERIAL)
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
		if (device->type() == TI85SERIAL)
			ti85_convert_data_to_stream(ti85_pc_ok_packet, TI85_PC_OK_PACKET_SIZE, serial_data->variables[i].ok);
		if (device->type() == TI86SERIAL)
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

		if (device->type() == TI85SERIAL)
			temp_data_to_convert[0] = 0x05;	//PC to TI-85
		if (device->type() == TI86SERIAL)
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
	if (device->type() == TI85SERIAL)
		ti85_convert_data_to_stream(ti85_pc_end_packet, TI85_PC_END_PACKET_SIZE, serial_data->end);
	if (device->type() == TI86SERIAL)
		ti85_convert_data_to_stream(ti86_pc_end_packet, TI85_PC_END_PACKET_SIZE, serial_data->end);
	serial_data->end_size = TI85_PC_END_PACKET_SIZE*8;

	free (ti85_entries);

	return 1;
}


static void ti85_free_serial_stream (ti85_serial_data*  serial_data)
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


static void ti85_send_variables (running_device *device)
{
	ti85serial_state *ti85serial = get_token( device );

	if (!ti85_alloc_serial_data_memory(device, 7))
		ti85serial->status = TI85_SEND_STOP;

	switch (ti85serial->status)
	{
		case TI85_SEND_HEADER:
			if(!ti85_send_serial(device, ti85serial->stream.variables[ti85serial->variables_variable_number].header,ti85serial->stream.variables[ti85serial->variables_variable_number].header_size))
			{
				ti85serial->status = TI85_RECEIVE_OK_1;
				memset (ti85serial->receive_data, 0, 7);
				logerror ("Header sent\n");
			}
			break;
		case TI85_RECEIVE_OK_1:
			if(!ti85_receive_serial (device, ti85serial->receive_buffer, 4*8))
			{
				ti85_convert_stream_to_data (ti85serial->receive_buffer, 4*8, ti85serial->receive_data);
				ti85serial->status = TI85_RECEIVE_ANSWER_1;
				logerror ("OK received\n");
			}
			break;
		case TI85_RECEIVE_ANSWER_1:
			if(!ti85_receive_serial (device, ti85serial->receive_buffer, 4*8))
			{
				ti85_convert_stream_to_data (ti85serial->receive_buffer, 4*8, ti85serial->receive_data);
				switch (ti85serial->receive_data[1])
				{
					case 0x09:	//continue
						ti85serial->status = TI85_SEND_OK;
						logerror ("Continue received\n");
						break;
					case 0x36:      //out of memory, skip or exit
						ti85serial->status = TI85_RECEIVE_ANSWER_2;
						break;
				}
			}
			break;
		case TI85_RECEIVE_ANSWER_2:
			if(!ti85_receive_serial (device, ti85serial->receive_buffer+4*8, 1*8))
			{
				ti85_convert_stream_to_data (ti85serial->receive_buffer, 5*8, ti85serial->receive_data);
				switch (ti85serial->receive_data[4])
				{
					case 0x01:	//exit
					case 0x02:	//skip
						ti85serial->status = TI85_RECEIVE_ANSWER_3;
						break;
					case 0x03:	//out of memory
						ti85serial->variables_variable_number = 0;
						ti85_free_serial_data_memory(device);
						ti85serial->status = TI85_SEND_STOP;
						break;
				}
			}
			break;
		case TI85_RECEIVE_ANSWER_3:
			if(!ti85_receive_serial (device, ti85serial->receive_buffer+5*8, 2*8))
			{
				ti85_convert_stream_to_data (ti85serial->receive_buffer, 7*8, ti85serial->receive_data);
				switch (ti85serial->receive_data[4])
				{
					case 0x01:	//exit
						ti85serial->variables_variable_number = 0;
						ti85_free_serial_data_memory(device);
						ti85serial->status = TI85_SEND_STOP;
						break;
					case 0x02:	//skip
						ti85serial->variables_variable_number++;
						ti85serial->status = TI85_SEND_OK;
						break;
				}
			}
			break;
		case TI85_SEND_OK:
			if(!ti85_send_serial(device, ti85serial->stream.variables[ti85serial->variables_variable_number].ok,ti85serial->stream.variables[ti85serial->variables_variable_number].ok_size))
			{
				ti85serial->status = (ti85serial->receive_data[4]==0x02) ?  ((ti85serial->variables_variable_number < ti85serial->stream.number_of_variables) ? TI85_SEND_HEADER : TI85_SEND_END) : TI85_SEND_DATA;
				memset(ti85serial->receive_data, 0, 7);
			}
			break;
		case TI85_SEND_DATA:
			if(!ti85_send_serial(device, ti85serial->stream.variables[ti85serial->variables_variable_number].data,ti85serial->stream.variables[ti85serial->variables_variable_number].data_size))
				ti85serial->status = TI85_RECEIVE_OK_2;
			break;
		case TI85_RECEIVE_OK_2:
			if(!ti85_receive_serial (device, ti85serial->receive_buffer, 4*8))
			{
				ti85serial->variables_variable_number++;
				ti85_convert_stream_to_data (ti85serial->receive_buffer, 4*8, ti85serial->receive_data);
				ti85serial->status = (ti85serial->variables_variable_number < ti85serial->stream.number_of_variables) ? TI85_SEND_HEADER : TI85_SEND_END;
			}
			break;
		case TI85_SEND_END:
			if(!ti85_send_serial(device, ti85serial->stream.end,ti85serial->stream.end_size))
			{
				logerror ("End sent\n");
				ti85serial->variables_variable_number = 0;
				ti85serial->status = TI85_RECEIVE_OK_3;
			}
			break;
		case TI85_RECEIVE_OK_3:
			if(!ti85_receive_serial (device, ti85serial->receive_buffer, 4*8))
			{
				logerror ("OK received\n");
				ti85_convert_stream_to_data (ti85serial->receive_buffer, 4*8, ti85serial->receive_data);
				ti85_free_serial_data_memory(device);
				ti85serial->status = TI85_SEND_STOP;
			}
			break;
	}
}


static void ti85_send_backup (running_device *device)
{
	ti85serial_state *ti85serial = get_token( device );

	if (!ti85_alloc_serial_data_memory(device, 7))
		ti85serial->status = TI85_SEND_STOP;

	switch (ti85serial->status)
	{
		case TI85_SEND_HEADER:
			if(!ti85_send_serial(device, ti85serial->stream.variables[ti85serial->send_backup_variable_number].header,ti85serial->stream.variables[ti85serial->send_backup_variable_number].header_size))
				ti85serial->status = TI85_RECEIVE_OK_1;
			break;
		case TI85_RECEIVE_OK_1:
			if(!ti85_receive_serial (device, ti85serial->receive_buffer, 4*8))
			{
				ti85_convert_stream_to_data (ti85serial->receive_buffer, 4*8, ti85serial->receive_data);
				ti85serial->status = TI85_RECEIVE_ANSWER_1;
			}
			break;
		case TI85_RECEIVE_ANSWER_1:
			if(!ti85_receive_serial (device, ti85serial->receive_buffer, 4*8))
			{
				ti85_convert_stream_to_data (ti85serial->receive_buffer, 4*8, ti85serial->receive_data);
				switch (ti85serial->receive_data[1])
				{
					case 0x09:	//continue
						ti85serial->status = TI85_SEND_OK;
						break;
					case 0x36:      //out of memory, skip or exit
						ti85serial->status = TI85_RECEIVE_ANSWER_2;
						break;
				}
			}
			break;
		case TI85_RECEIVE_ANSWER_2:
			if(!ti85_receive_serial (device, ti85serial->receive_buffer+4*8, 3*8))
			{
				ti85_convert_stream_to_data (ti85serial->receive_buffer, 5*8, ti85serial->receive_data);
				ti85serial->send_backup_variable_number = 0;
				ti85_free_serial_data_memory(device);
				ti85serial->status = TI85_SEND_STOP;
			}
			break;
		case TI85_SEND_OK:
			if(!ti85_send_serial(device, ti85serial->stream.variables[ti85serial->send_backup_variable_number].ok,ti85serial->stream.variables[ti85serial->send_backup_variable_number].ok_size))
				ti85serial->status = TI85_SEND_DATA;
			break;
		case TI85_SEND_DATA:
			if(!ti85_send_serial(device, ti85serial->stream.variables[ti85serial->send_backup_variable_number].data,ti85serial->stream.variables[ti85serial->send_backup_variable_number].data_size))
				ti85serial->status = TI85_RECEIVE_OK_2;
			break;
		case TI85_RECEIVE_OK_2:
			if(!ti85_receive_serial (device, ti85serial->receive_buffer, 4*8))
			{
				ti85serial->send_backup_variable_number++;
				ti85_convert_stream_to_data (ti85serial->receive_buffer, 4*8, ti85serial->receive_data);
				ti85serial->status = (ti85serial->send_backup_variable_number < ti85serial->stream.number_of_variables) ? TI85_SEND_DATA : TI85_SEND_END;
			}
			break;
		case TI85_SEND_END:
			if(!ti85_send_serial(device, ti85serial->stream.end,ti85serial->stream.end_size))
			{
				ti85serial->send_backup_variable_number = 0;
				ti85_free_serial_data_memory(device);
				ti85serial->status = TI85_SEND_STOP;
			}
			break;
	}
}


static void ti85_receive_variables (running_device *device)
{
	ti85serial_state *ti85serial = get_token( device );
	char var_file_name[16];
	mame_file * var_file;
	UINT8* temp;
	file_error filerr;

	switch (ti85serial->status)
	{
		case TI85_RECEIVE_HEADER_1:
			if(!ti85_receive_serial(device, ti85serial->receive_buffer+4*8, 3*8))
			{
				ti85_convert_stream_to_data (ti85serial->receive_buffer+4*8, 3*8, ti85serial->receive_data+4);
				ti85serial->status = TI85_PREPARE_VARIABLE_DATA;
			}
			break;
		case TI85_PREPARE_VARIABLE_DATA:
			ti85serial->var_data = (UINT8*) malloc (ti85serial->receive_data[2]+2+ti85serial->receive_data[4]+ti85serial->receive_data[5]*256+2);
			if(!ti85serial->var_data)
				ti85serial->status = TI85_SEND_STOP;
			memcpy (ti85serial->var_data, ti85serial->receive_data+2, 5);
			ti85_free_serial_data_memory(device);
			if (!ti85_alloc_serial_data_memory(device, ti85serial->var_data[0]-1))
			{
				free(ti85serial->var_data); ti85serial->var_data = NULL;
				free(ti85serial->var_file_data); ti85serial->var_file_data = NULL;
				ti85serial->status = TI85_SEND_STOP;
				return;
			}
			ti85serial->status = TI85_RECEIVE_HEADER_2;
		case TI85_RECEIVE_HEADER_2:
			if(!ti85_receive_serial(device, ti85serial->receive_buffer, (ti85serial->var_data[0]-1)*8))
			{
				ti85_convert_stream_to_data (ti85serial->receive_buffer, (ti85serial->var_data[0]-1)*8, ti85serial->receive_data);
				memcpy (ti85serial->var_data+5, ti85serial->receive_data, ti85serial->var_data[0]-3);
				ti85_free_serial_data_memory(device);
				if(!ti85_alloc_serial_data_memory (device, 8))
				{
					free(ti85serial->var_data); ti85serial->var_data = NULL;
					free(ti85serial->var_file_data); ti85serial->var_file_data = NULL;
					ti85serial->status = TI85_SEND_STOP;
					return;
				}
				ti85_convert_data_to_stream (ti85_pc_ok_packet, 4, ti85serial->receive_buffer);
				ti85_convert_data_to_stream (ti85_pc_continue_packet, 4, ti85serial->receive_buffer+4*8);
				ti85serial->status = TI85_SEND_OK_1;
			}
			break;
		case TI85_SEND_OK_1:
			if(!ti85_send_serial(device, ti85serial->receive_buffer, 4*8))
				ti85serial->status = TI85_SEND_CONTINUE;
			break;
		case TI85_SEND_CONTINUE:
			if(!ti85_send_serial(device, ti85serial->receive_buffer+4*8, 4*8))
			{
				ti85_free_serial_data_memory(device);
				if(!ti85_alloc_serial_data_memory(device, 4))
				{
					free(ti85serial->var_data); ti85serial->var_data = NULL;
					free(ti85serial->var_file_data); ti85serial->var_file_data = NULL;
					ti85serial->status = TI85_SEND_STOP;
					return;
				}
				ti85serial->status = TI85_RECEIVE_OK;
			}
			break;
		case TI85_RECEIVE_OK:
			if(!ti85_receive_serial (device, ti85serial->receive_buffer, 4*8))
			{
				ti85_convert_stream_to_data (ti85serial->receive_buffer, 4*8, ti85serial->receive_data);
				ti85_free_serial_data_memory(device);
				if(!ti85_alloc_serial_data_memory(device, ti85serial->var_data[2]+ti85serial->var_data[3]*256+6))
				{
					free(ti85serial->var_data); ti85serial->var_data = NULL;
					free(ti85serial->var_file_data); ti85serial->var_file_data = NULL;
					ti85serial->status = TI85_SEND_STOP;
					return;
				}
				ti85serial->status = TI85_RECEIVE_DATA;
			}
			break;
		case TI85_RECEIVE_DATA:
			if(!ti85_receive_serial (device, ti85serial->receive_buffer, (ti85serial->var_data[2]+ti85serial->var_data[3]*256+6)*8))
			{
				ti85_convert_stream_to_data (ti85serial->receive_buffer, (ti85serial->var_data[2]+ti85serial->var_data[3]*256+6)*8, ti85serial->receive_data);
				memcpy (ti85serial->var_data+ti85serial->var_data[0]+2, ti85serial->receive_data+2, ti85serial->var_data[2]+ti85serial->var_data[3]*256+2);
				ti85_free_serial_data_memory(device);
				if(!ti85_alloc_serial_data_memory (device, 4))
				{
					free(ti85serial->var_data); ti85serial->var_data = NULL;
					free(ti85serial->var_file_data); ti85serial->var_file_data = NULL;
					ti85serial->status = TI85_SEND_STOP;
					return;
				}
				ti85_convert_data_to_stream (ti85_pc_ok_packet, 4, ti85serial->receive_buffer);
				ti85serial->status = TI85_SEND_OK_2;
			}
			break;
		case TI85_SEND_OK_2:
			if(!ti85_send_serial(device, ti85serial->receive_buffer, 4*8))
			{
				ti85_free_serial_data_memory(device);
				if(!ti85_alloc_serial_data_memory(device, 7))
				{
					free(ti85serial->var_data); ti85serial->var_data = NULL;
					free(ti85serial->var_file_data); ti85serial->var_file_data = NULL;
					ti85serial->status = TI85_SEND_STOP;
					return;
				}
				ti85serial->status =  TI85_RECEIVE_END_OR_HEADER_1;
			}
			break;
		case TI85_RECEIVE_END_OR_HEADER_1:
			if(!ti85_receive_serial (device, ti85serial->receive_buffer, 4*8))
			{
				if (ti85serial->variable_number == 0)
				{
					ti85serial->var_file_data = (UINT8*) malloc (0x39);
					if (!ti85serial->var_file_data)
					{
						free (ti85serial->var_data); ti85serial->var_data = NULL;
						free (ti85serial->var_file_data); ti85serial->var_file_data = NULL;
						ti85serial->status = TI85_SEND_STOP;
					}
					else
					{
						memcpy(ti85serial->var_file_data, ti85_file_signature, 0x0b);
						memcpy(ti85serial->var_file_data+0x0b, "File created by MESS", 0x14);
						memset(ti85serial->var_file_data+0x1f, 0, 0x1a);
						ti85serial->var_file_size = 0x39;
					}
				}
				temp = (UINT8*)malloc (ti85serial->var_file_size+ti85serial->var_data[0]+2+ti85serial->var_data[2]+ti85serial->var_data[3]*256+2);
				if (temp)
				{
					memcpy (temp, ti85serial->var_file_data, ti85serial->var_file_size);
					free (ti85serial->var_file_data);
					ti85serial->var_file_data = temp;
					memcpy (ti85serial->var_file_data + ti85serial->var_file_size -2, ti85serial->var_data, ti85serial->var_data[0]+2+ti85serial->var_data[2]+ti85serial->var_data[3]*256+2);
					ti85serial->var_file_size += ti85serial->var_data[0]+2+ti85serial->var_data[2]+ti85serial->var_data[3]*256+2;
					ti85serial->var_file_data[0x35] = (ti85serial->var_file_size-0x39)&0x00ff;
					ti85serial->var_file_data[0x36] = ((ti85serial->var_file_size-0x39)&0xff00)>>8;
					ti85serial->var_file_data[ti85serial->var_file_size-2] = ti85_calculate_checksum(ti85serial->var_file_data+0x37, ti85serial->var_file_size-0x39)&0x00ff;
					ti85serial->var_file_data[ti85serial->var_file_size-1] = (ti85_calculate_checksum(ti85serial->var_file_data+0x37, ti85serial->var_file_size-0x39)&0xff00)>>8;
				}
				free (ti85serial->var_data);
				ti85serial->var_data = NULL;
				ti85serial->variable_number ++;

				ti85_convert_stream_to_data (ti85serial->receive_buffer, 4*8, ti85serial->receive_data);
				if (ti85serial->receive_data[1] == 0x06)
					ti85serial->status = TI85_RECEIVE_HEADER_1;
				else
				{
					ti85_free_serial_data_memory(device);
					if(!ti85_alloc_serial_data_memory (device, 4))
					{
						free(ti85serial->var_data); ti85serial->var_data = NULL;
						free(ti85serial->var_file_data); ti85serial->var_file_data = NULL;
						ti85serial->status = TI85_SEND_STOP;
						return;
					}
					ti85_convert_data_to_stream (ti85_pc_ok_packet, 4, ti85serial->receive_buffer);
					ti85serial->status = TI85_SEND_OK_3;
				}
			}
			break;
		case TI85_SEND_OK_3:
			if(!ti85_send_serial(device, ti85serial->receive_buffer, 4*8))
			{
				ti85_free_serial_data_memory(device);
				ti85serial->variable_number = 0;
				ti85serial->status =  TI85_SEND_STOP;
				sprintf (var_file_name, "%08d.85g", ti85serial->var_file_number);
				filerr = mame_fopen(NULL, var_file_name, OPEN_FLAG_READ | OPEN_FLAG_WRITE | OPEN_FLAG_CREATE, &var_file);
				if (filerr == FILERR_NONE)
				{
					mame_fwrite(var_file, ti85serial->var_file_data, ti85serial->var_file_size);
					mame_fclose(var_file);
					free (ti85serial->var_file_data);
					ti85serial->var_file_data = NULL;
					ti85serial->var_file_size = 0;
					ti85serial->var_file_number++;
				}
			}
			break;
	}
}


static void ti85_receive_backup (running_device *device)
{
	ti85serial_state *ti85serial = get_token( device );

	file_error filerr;
	char backup_file_name[] = "00000000.85b";
	mame_file * backup_file;

	switch (ti85serial->status)
	{
		case TI85_RECEIVE_HEADER_2:
			if(!ti85_receive_serial(device, ti85serial->receive_buffer+7*8, 8*8))
			{
				ti85_convert_stream_to_data (ti85serial->receive_buffer+7*8, 8*8, ti85serial->receive_data+7);
				ti85serial->backup_data_size[0] = ti85serial->receive_data[4] + ti85serial->receive_data[5]*256;
				ti85serial->backup_data_size[1] = ti85serial->receive_data[7] + ti85serial->receive_data[8]*256;
				ti85serial->backup_data_size[2] = ti85serial->receive_data[9] + ti85serial->receive_data[10]*256;
				ti85serial->backup_file_data = (UINT8*)malloc (0x42+0x06+ti85serial->backup_data_size[0]+ti85serial->backup_data_size[1]+ti85serial->backup_data_size[2]+0x02);
				if(!ti85serial->backup_file_data)
				{
					ti85serial->backup_variable_number = 0;
					ti85serial->status = TI85_SEND_STOP;
					return;
				}
				memcpy(ti85serial->backup_file_data, ti85_file_signature, 0x0b);
				memcpy(ti85serial->backup_file_data+0x0b, "File created by MESS", 0x14);
				memset(ti85serial->backup_file_data+0x1f, 0, 0x16);
				ti85serial->backup_file_data[0x35] = (ti85serial->backup_data_size[0]+ti85serial->backup_data_size[1]+ti85serial->backup_data_size[2]+0x11)&0x00ff;
				ti85serial->backup_file_data[0x36] = ((ti85serial->backup_data_size[0]+ti85serial->backup_data_size[1]+ti85serial->backup_data_size[2]+0x11)&0xff00)>>8;
				memcpy(ti85serial->backup_file_data+0x37, ti85serial->receive_data+0x02, 0x0b);
				ti85_free_serial_data_memory(device);
				if(!ti85_alloc_serial_data_memory (device, 8))
				{
					free(ti85serial->backup_file_data); ti85serial->backup_file_data = NULL;
					ti85serial->backup_variable_number = 0;
					ti85serial->status = TI85_SEND_STOP;
					return;
				}
				ti85_convert_data_to_stream (ti85_pc_ok_packet, 4, ti85serial->receive_buffer);
				ti85_convert_data_to_stream (ti85_pc_continue_packet, 4, ti85serial->receive_buffer+4*8);
				ti85serial->status = TI85_SEND_OK_1;
			}
			break;
		case TI85_SEND_OK_1:
			if(!ti85_send_serial(device, ti85serial->receive_buffer, 4*8))
				ti85serial->status = TI85_SEND_CONTINUE;
			break;
		case TI85_SEND_CONTINUE:
			if(!ti85_send_serial(device, ti85serial->receive_buffer+4*8, 4*8))
			{
				ti85_free_serial_data_memory(device);
				if(!ti85_alloc_serial_data_memory(device, 4))
				{
					free(ti85serial->backup_file_data); ti85serial->backup_file_data = NULL;
					ti85serial->backup_variable_number = 0;
					ti85serial->status = TI85_SEND_STOP;
					return;
				}
				ti85serial->status = TI85_RECEIVE_OK;
			}
			break;
		case TI85_RECEIVE_OK:
			if(!ti85_receive_serial (device, ti85serial->receive_buffer, 4*8))
			{
				ti85_convert_stream_to_data (ti85serial->receive_buffer, 4*8, ti85serial->receive_data);
				ti85_free_serial_data_memory(device);
				if(!ti85_alloc_serial_data_memory(device, ti85serial->backup_data_size[ti85serial->backup_variable_number]+6))
				{
					free(ti85serial->backup_file_data); ti85serial->backup_file_data = NULL;
					ti85serial->backup_variable_number = 0;
					ti85serial->status = TI85_SEND_STOP;
					return;
				}
				ti85serial->status = TI85_RECEIVE_DATA;
			}
			break;
		case TI85_RECEIVE_DATA:
			if(!ti85_receive_serial (device, ti85serial->receive_buffer, (ti85serial->backup_data_size[ti85serial->backup_variable_number]+6)*8))
			{
				ti85_convert_stream_to_data (ti85serial->receive_buffer, (ti85serial->backup_data_size[ti85serial->backup_variable_number]+6)*8, ti85serial->receive_data);
				switch (ti85serial->backup_variable_number)
				{
					case 0:
						memcpy(ti85serial->backup_file_data+0x42, ti85serial->receive_data+0x02, ti85serial->backup_data_size[0]+0x02);
						break;
					case 1:
						memcpy(ti85serial->backup_file_data+0x42+ti85serial->backup_data_size[0]+0x02, ti85serial->receive_data+0x02, ti85serial->backup_data_size[1]+0x02);
						break;
					case 2:
						memcpy(ti85serial->backup_file_data+0x42+ti85serial->backup_data_size[0]+ti85serial->backup_data_size[1]+0x04, ti85serial->receive_data+0x02, ti85serial->backup_data_size[2]+0x02);
						break;
				}
				ti85_free_serial_data_memory(device);
				if(!ti85_alloc_serial_data_memory (device, 4))
				{
					free(ti85serial->backup_file_data); ti85serial->backup_file_data = NULL;
					ti85serial->backup_variable_number = 0;
					ti85serial->status = TI85_SEND_STOP;
					return;
				}
				ti85_convert_data_to_stream (ti85_pc_ok_packet, 4, ti85serial->receive_buffer);
				ti85serial->status = TI85_SEND_OK_2;
			}
			break;
		case TI85_SEND_OK_2:
			if(!ti85_send_serial(device, ti85serial->receive_buffer, 4*8))
			{
				ti85_free_serial_data_memory(device);
				ti85serial->backup_variable_number++;
				if (ti85serial->backup_variable_number<3)
				{
					if(!ti85_alloc_serial_data_memory(device, ti85serial->backup_data_size[ti85serial->backup_variable_number]+6))
					{
						free(ti85serial->backup_file_data); ti85serial->backup_file_data = NULL;
						ti85serial->backup_variable_number = 0;
						ti85serial->status = TI85_SEND_STOP;
						return;
					}
					ti85serial->status =  TI85_RECEIVE_DATA;
				}
				else
				{
					ti85serial->backup_variable_number = 0;
					ti85serial->backup_file_data[0x42+0x06+ti85serial->backup_data_size[0]+ti85serial->backup_data_size[1]+ti85serial->backup_data_size[2]] = ti85_calculate_checksum(ti85serial->backup_file_data+0x37, 0x42+ti85serial->backup_data_size[0]+ti85serial->backup_data_size[1]+ti85serial->backup_data_size[2]+0x06-0x37)&0x00ff;
					ti85serial->backup_file_data[0x42+0x06+ti85serial->backup_data_size[0]+ti85serial->backup_data_size[1]+ti85serial->backup_data_size[2]+0x01] = (ti85_calculate_checksum(ti85serial->backup_file_data+0x37, 0x42+ti85serial->backup_data_size[0]+ti85serial->backup_data_size[1]+ti85serial->backup_data_size[2]+0x06-0x37)&0xff00)>>8;
					sprintf (backup_file_name, "%08d.85b", ti85serial->backup_file_number);
					filerr = mame_fopen(NULL, backup_file_name, OPEN_FLAG_READ | OPEN_FLAG_WRITE | OPEN_FLAG_CREATE, &backup_file);
					if (filerr == FILERR_NONE)
					{
						mame_fwrite(backup_file, ti85serial->backup_file_data, 0x42+0x06+ti85serial->backup_data_size[0]+ti85serial->backup_data_size[1]+ti85serial->backup_data_size[2]+0x02);
						mame_fclose(backup_file);
						ti85serial->backup_file_number++;
					}
					free(ti85serial->backup_file_data); ti85serial->backup_file_data = NULL;
					ti85serial->status =  TI85_SEND_STOP;
				}
			}
			break;
	}
}

static void ti85_receive_screen (running_device *device)
{
	ti85serial_state *ti85serial = get_token( device );
	char image_file_name[] = "00000000.85i";
	file_error filerr;
	mame_file * image_file;
	UINT8 * image_file_data;

	switch (ti85serial->status)
	{
		case TI85_PREPARE_SCREEN_REQUEST:
			if(!ti85_alloc_serial_data_memory (device, 4))
			{
				ti85serial->status = TI85_SEND_STOP;
				return;
			}
			ti85_convert_data_to_stream (ti85_pc_screen_request_packet, 4, ti85serial->receive_buffer);
			ti85serial->status = TI85_SEND_SCREEN_REQUEST;
			break;
		case TI85_SEND_SCREEN_REQUEST:
			if(!ti85_send_serial(device, ti85serial->receive_buffer, 4*8))
			{
				ti85_free_serial_data_memory(device);
				if(!ti85_alloc_serial_data_memory (device, 4))
				{
					ti85serial->status = TI85_SEND_STOP;
					return;
				}
				ti85serial->status = TI85_RECEIVE_OK;
			}
			break;
		case TI85_RECEIVE_OK:
			if(!ti85_receive_serial (device, ti85serial->receive_buffer, 4*8))
			{
				ti85_convert_stream_to_data (ti85serial->receive_buffer, 4*8, ti85serial->receive_data);
				ti85_free_serial_data_memory(device);
				if(!ti85_alloc_serial_data_memory (device, 1030))
				{
					ti85serial->status = TI85_SEND_STOP;
					return;
				}
				ti85serial->status = TI85_RECEIVE_DATA;
			}
			break;
		case TI85_RECEIVE_DATA:
			if(!ti85_receive_serial (device, ti85serial->receive_buffer, 1030*8))
			{
				ti85_convert_stream_to_data (ti85serial->receive_buffer, 1030*8, ti85serial->receive_data);
				sprintf (image_file_name, "%08d.85i", ti85serial->image_file_number);
				filerr = mame_fopen(NULL, image_file_name, OPEN_FLAG_READ | OPEN_FLAG_WRITE | OPEN_FLAG_CREATE, &image_file);
				if (filerr == FILERR_NONE)
				{
					image_file_data = (UINT8*)malloc (0x49+1008);
					if(!image_file_data)
					{
						ti85_free_serial_data_memory(device);
						ti85serial->status = TI85_SEND_OK;
						return;
					}
					memcpy(image_file_data, ti85_file_signature, 0x0b);
					memcpy(image_file_data+0x0b, "File created by MESS", 0x14);
					memset(image_file_data+0x1f, 0, 0x16);
					image_file_data[0x35] = 0x00;
					image_file_data[0x36] = 0x04;
					image_file_data[0x37] = 0x0a; //header size
					image_file_data[0x38] = 0x00;
					image_file_data[0x39] = 0xf2; //data size
					image_file_data[0x3a] = 0x03;
					image_file_data[0x3b] = 0x11; //data type
					image_file_data[0x3c] = 0x06; //name size
					memcpy(image_file_data+0x3d, "Screen", 0x06);
					image_file_data[0x43] = 0xf2;
					image_file_data[0x44] = 0x03;
					image_file_data[0x45] = 0xf0;
					image_file_data[0x46] = 0x03;
					memcpy(image_file_data+0x47, ti85serial->receive_data+4, 1008);
					image_file_data[1008+0x49-2] = ti85_calculate_checksum(image_file_data+0x37, 1008+0x10)&0x00ff;
					image_file_data[1008+0x49-1] = (ti85_calculate_checksum(image_file_data+0x37, 1008+0x10)&0xff00)>>8;
					mame_fwrite(image_file, image_file_data, 1008+0x49);
					mame_fclose(image_file);
					free(image_file_data);
					ti85serial->image_file_number++;
				}
				ti85_free_serial_data_memory(device);
				if(!ti85_alloc_serial_data_memory (device, 4))
				{
					ti85serial->status = TI85_SEND_STOP;
					return;
				}
				ti85_convert_data_to_stream (ti85_pc_ok_packet, 4, ti85serial->receive_buffer);
				ti85serial->status = TI85_SEND_OK;
			}
			break;
		case TI85_SEND_OK:
			if(!ti85_send_serial(device, ti85serial->receive_buffer, 4*8))
			{
				ti85_free_serial_data_memory(device);
				ti85serial->status = TI85_SEND_STOP;
			}
			break;
	}
}


void ti85_update_serial (running_device *device)
{
	ti85serial_state *ti85serial = get_token( device );

	if (ti85serial->status == TI85_SEND_STOP)
	{
		if (input_port_read(device->machine, "SERIAL") & 0x01)
		{
			if(!ti85_alloc_serial_data_memory(device, 15)) return;
			if(!ti85_receive_serial (device, ti85serial->receive_buffer, 7*8))
			{
				ti85_convert_stream_to_data (ti85serial->receive_buffer, 7*8, ti85serial->receive_data);
				if (ti85serial->receive_data[0]!=0x85 || ti85serial->receive_data[1]!=0x06)
				{
					ti85_receive_serial (device, NULL, 0);
					return;
				}
				ti85serial->transfer_type = (ti85serial->receive_data[6]==0x1d) ? TI85_RECEIVE_BACKUP : TI85_RECEIVE_VARIABLES;
				ti85serial->status = (ti85serial->receive_data[6]==0x1d) ? TI85_RECEIVE_HEADER_2 : TI85_PREPARE_VARIABLE_DATA;
			}
		}
		else
		{
			ti85_receive_serial(device, NULL, 0);
			ti85_free_serial_data_memory(device);
			if (input_port_read(device->machine, "DUMP") & 0x01)
			{
				ti85serial->status = TI85_PREPARE_SCREEN_REQUEST;
				ti85serial->transfer_type = TI85_RECEIVE_SCREEN;
			}
			return;
		}
	}

	switch (ti85serial->transfer_type)
	{
		case TI85_SEND_VARIABLES:
			ti85_send_variables(device);
			break;
		case TI85_SEND_BACKUP:
			ti85_send_backup(device);
			break;
		case TI85_RECEIVE_BACKUP:
			ti85_receive_backup(device);
			break;
		case TI85_RECEIVE_VARIABLES:
			ti85_receive_variables(device);
			break;
		case TI85_RECEIVE_SCREEN:
			ti85_receive_screen(device);
			break;
	}
}


WRITE8_DEVICE_HANDLER( ti85serial_red_out )
{
	ti85serial_state *ti85serial = get_token( device );

	ti85serial->red_out = data;
}


WRITE8_DEVICE_HANDLER( ti85serial_white_out )
{
	ti85serial_state *ti85serial = get_token( device );

	ti85serial->white_out = data;
}


READ8_DEVICE_HANDLER( ti85serial_red_in )
{
	ti85serial_state *ti85serial = get_token( device );

	return ti85serial->red_in;
}


READ8_DEVICE_HANDLER( ti85serial_white_in )
{
	ti85serial_state *ti85serial = get_token( device );

	return ti85serial->white_in;
}


static DEVICE_START( ti85serial )
{
	ti85serial_state *ti85serial = get_token( device );

	ti85serial->transfer_type = TI85_SEND_VARIABLES;
	ti85serial->receive_buffer = NULL;
	ti85serial->receive_data = NULL;
	ti85serial->var_file_number = 0;
	ti85serial->backup_file_number = 0;
	ti85serial->image_file_number = 0;
}


static DEVICE_STOP( ti85serial )
{
	ti85serial_state *ti85serial = get_token( device );

	ti85_free_serial_data_memory(device);
	if (ti85serial->var_data)
	{
		free(ti85serial->var_data); ti85serial->var_data = NULL;
	}
	if (ti85serial->var_file_data)
	{
		free(ti85serial->var_file_data); ti85serial->var_file_data = NULL;
	}
	if (ti85serial->backup_file_data)
	{
		free(ti85serial->backup_file_data); ti85serial->backup_file_data = NULL;
	}
}


static DEVICE_RESET( ti85serial )
{
	ti85serial_state *ti85serial = get_token( device );

	ti85serial->receive_data_counter = 0;
	ti85serial->send_data_counter = 0;
	ti85_free_serial_data_memory(device);
	ti85serial->red_in = 0;
	ti85serial->white_in = 0;
	ti85serial->red_out = 1;
	ti85serial->white_out = 1;
	ti85serial->variables_variable_number = 0;
	ti85serial->send_backup_variable_number = 0;
	ti85serial->var_data = NULL;
	ti85serial->var_file_data = NULL;
	ti85serial->var_file_size = 0;
	ti85serial->variable_number = 0;
	ti85serial->backup_file_data = NULL;
	ti85serial->backup_variable_number = 0;
	ti85serial->status = TI85_SEND_STOP;
}


static DEVICE_IMAGE_LOAD( ti85serial )
{
	ti85serial_state *ti85serial = get_token( image );
	UINT8* file_data;
	UINT16 file_size;

	if (ti85serial->status != TI85_SEND_STOP) return IMAGE_INIT_FAIL;

	file_size = image.length();

	if (file_size != 0)
	{
		file_data = auto_alloc_array(image.device().machine, UINT8, file_size);
		image.fread( file_data, file_size);

		if(!ti85_convert_file_data_to_serial_stream(image, file_data, file_size, &ti85serial->stream))
		{
			ti85_free_serial_stream (&ti85serial->stream);
			return IMAGE_INIT_FAIL;
		}

		ti85serial->status = TI85_SEND_HEADER;
	}
	else
	{
		return IMAGE_INIT_FAIL;
	}
	return IMAGE_INIT_PASS;
}


static DEVICE_IMAGE_UNLOAD( ti85serial )
{
	ti85serial_state *ti85serial = get_token( image );

	ti85_free_serial_data_memory(image);
	ti85serial->status = TI85_SEND_STOP;
	ti85_free_serial_stream (&ti85serial->stream);
}


DEVICE_GET_INFO( ti85serial )
{
	switch ( state )
	{
	case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof( ti85serial_state );											break;
	case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;																	break;
	case DEVINFO_INT_IMAGE_TYPE:					info->i = IO_SERIAL;															break;
	case DEVINFO_INT_IMAGE_READABLE:				info->i = 1;																	break;
	case DEVINFO_INT_IMAGE_WRITEABLE:				info->i = 0;																	break;
	case DEVINFO_INT_IMAGE_CREATABLE:				info->i = 0;																	break;
	case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( ti85serial );									break;
	case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME( ti85serial );									break;
	case DEVINFO_FCT_STOP:							info->stop = DEVICE_STOP_NAME( ti85serial );									break;
	case DEVINFO_FCT_IMAGE_LOAD:					info->f = (genf *) DEVICE_IMAGE_LOAD_NAME( ti85serial );						break;
	case DEVINFO_FCT_IMAGE_UNLOAD:					info->f = (genf *) DEVICE_IMAGE_UNLOAD_NAME( ti85serial );						break;
	case DEVINFO_STR_IMAGE_BRIEF_INSTANCE_NAME:		strcpy(info->s, "ser");															break;
	case DEVINFO_STR_IMAGE_INSTANCE_NAME:
	case DEVINFO_STR_NAME:							strcpy(info->s, "TI85 Serial");													break;
	case DEVINFO_STR_FAMILY:						strcpy(info->s, "serial protocol");												break;
	case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);														break;
	case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:			strcpy(info->s, "85p,85s,85i,85n,85c,85l,85k,85m,85v,85d,85e,85r,85g,85b");		break;
	}
}


DEVICE_GET_INFO( ti86serial )
{
	switch ( state )
	{
	case DEVINFO_STR_NAME:							strcpy(info->s, "TI86 Serial");													break;
	case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:			strcpy(info->s, "86p,86s,86i,86n,86c,86l,86k,86m,86v,86d,86e,86r,86g");			break;
	default:										DEVICE_GET_INFO_CALL( ti85serial );
	}
}

DEFINE_LEGACY_IMAGE_DEVICE(TI85SERIAL, ti85serial);
DEFINE_LEGACY_IMAGE_DEVICE(TI86SERIAL, ti86serial);
