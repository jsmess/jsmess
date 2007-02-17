#define TI85_HEADER_SIZE	0x37

#define TI85_SEND_VARIABLES	1
#define TI85_SEND_BACKUP	2
#define TI85_RECEIVE_BACKUP	3
#define TI85_RECEIVE_VARIABLES	4
#define TI85_RECEIVE_SCREEN	5

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

extern UINT8 ti85_file_signature[];
extern UINT8 ti86_file_signature[];
extern UINT8 ti85_pc_ok_packet[];
extern UINT8 ti86_pc_ok_packet[];
extern UINT8 ti85_pc_continue_packet[];
extern UINT8 ti86_pc_continue_packet[];
extern UINT8 ti85_pc_screen_request_packet[];
extern int ti85_serial_transfer_type;

extern UINT16 ti85_calculate_checksum(UINT8*, unsigned int);
extern UINT16 ti85_variables_count (UINT8 *, unsigned int);
extern void ti85_variables_read (UINT8 *, unsigned int, ti85_entry *);
extern int ti85_convert_file_data_to_serial_stream (UINT8*, UINT32, ti85_serial_data*, char*);
extern void ti85_convert_data_to_stream (UINT8*, unsigned int, UINT8*);
extern void ti85_convert_stream_to_data (UINT8*, unsigned int, UINT8*);
extern void ti85_free_serial_stream (ti85_serial_data*);
