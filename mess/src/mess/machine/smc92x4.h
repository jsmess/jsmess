/* Interface */

#ifndef __SMC92X4_H__
#define __SMC92X4_H__

DECLARE_LEGACY_DEVICE(SMC92X4, smc92x4);

#define INPUT_STATUS	0x00
#define OUTPUT_DMA_ADDR 0x01
#define OUTPUT_OUTPUT1	0x02
#define OUTPUT_OUTPUT2	0x03

#define MFMHD_TRACK00	0x80
#define MFMHD_SEEKCOMP	0x40
#define MFMHD_WRFAULT	0x20
#define MFMHD_READY	0x10
#define MFMHD_INDEX	0x08

#define BAD_SECTOR	0x1000
#define BAD_CRC		0x2000

/*
    Definition of bits in the Disk-Status register
*/
#define DS_ECCERR	0x80		/* ECC error */
#define DS_INDEX	0x40		/* index point */
#define DS_SKCOM	0x20		/* seek complete */
#define DS_TRK00	0x10		/* track 0 */
#define DS_UDEF		0x08		/* user-defined */
#define DS_WRPROT	0x04		/* write protect */
#define DS_READY	0x02		/* drive ready bit */
#define DS_WRFAULT	0x01		/* write fault */

/*
    Needed to adapt to higher cylinder numbers. Floppies do not have such
    high numbers.
*/
typedef struct chrn_id_hd
{
	UINT16 C;
	UINT8 H;
	UINT8 R;
	UINT8 N;
	int data_id;			// id for read/write data command
	unsigned long flags;
} chrn_id_hd;

typedef struct _smc92x4_interface
{
	/* Disk format support. This flag allows to choose between the full
       FM/MFM format and an abbreviated track layout. The difference results
       from legal variations of the layout. This is not part of
       the smc92x4 specification, but it allows to keep the image format
       simple without too much case checking. Should be removed as soon as
       the respective disk formats support the full format. */
	int full_track_layout;

	/* Interrupt line. To be connected with the controller PCB. */
	devcb_write_line out_intrq_func;

	/* DMA in progress line. To be connected with the controller PCB. */
	devcb_write_line out_dip_func;

	/* Auxiliary Bus. These 8 lines need to be connected to external latches
    and to a counter circuitry which works together with the external RAM.
    We use the S0/S1 lines as address lines.
    */
	devcb_write8 out_auxbus_func;

	/* Auxiliary Bus. This is only used for S0=S1=0, so we need no address
    lines and consider this line just as a line with 256 states. */
	devcb_read_line in_auxbus_func;

	/* Get the currently selected floppy disk. This is determined by the
       circuitry on the controller board, not within the controller itself.
       The device is this controller.
    */
	device_t *(*current_floppy)(device_t *device);

	/* Callback to read the contents of the external RAM via the data bus.
       Note that the address must be set and automatically increased
       by external circuitry. */
	UINT8 (*dma_read_callback)(device_t *device);

	/* Callback to write the contents of the external RAM via the data bus.
       Note that the address must be set and automatically increased
       by external circuitry. */
	void (*dma_write_callback)(device_t *device, UINT8 data);

	/* Preliminary MFM hard disk interface. Gets next id. */
	void (*mfmhd_get_next_id)(device_t *device, int head, chrn_id_hd *id);

	/* Preliminary MFM hard disk interface. Performs a seek. */
	void (*mfmhd_seek)(device_t *device, int direction);

	/* Preliminary MFM hard disk interface. Reads a sector. */
	void (*mfmhd_read_sector)(device_t *device, int cylinder, int head, int sector, UINT8 **buf, int *sector_length);

	/* Preliminary MFM hard disk interface. Writes a sector. */
	void (*mfmhd_write_sector)(device_t *device, int cylinder, int head, int sector, UINT8 *buf, int sector_length);

	/* Preliminary MFM hard disk interface. Reads a track. */
	void (*mfmhd_read_track)(device_t *device, int head, UINT8 **buffer, int *data_count);

	/* Preliminary MFM hard disk interface. Writes a track. */
	void (*mfmhd_write_track)(device_t *device, int head, UINT8 *buffer, int data_count);
} smc92x4_interface;

void smc92x4_reset(device_t *device);

/* Used to turn off the delays. */
void smc92x4_set_timing(device_t *device, int realistic);

/* Generic function to translate between cylinders and idents */
UINT8 cylinder_to_ident(int cylinder);

READ8_DEVICE_HANDLER( smc92x4_r );
WRITE8_DEVICE_HANDLER( smc92x4_w );

#define MCFG_SMC92X4_ADD(_tag, _intrf) \
	MCFG_DEVICE_ADD(_tag, SMC92X4, 0) \
	MCFG_DEVICE_CONFIG(_intrf)

#endif
