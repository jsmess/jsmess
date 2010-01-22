/***************************************************************************

    Image device

***************************************************************************/

#ifndef __IMAGE_DEV_H__
#define __IMAGE_DEV_H__

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef enum
{
    IMAGE_CARTSLOT,    /* Cartridge Port, as found on most console and on some computers */
    IMAGE_FLOPPY,      /* Floppy Disk unit */
    IMAGE_HARDDISK,    /* Hard Disk unit */
    IMAGE_CYLINDER,    /* Magnetically-Coated Cylinder */
    IMAGE_CASSETTE,    /* Cassette Recorder (common on early home computers) */
    IMAGE_PUNCHCARD,   /* Card Puncher/Reader */
    IMAGE_PUNCHTAPE,   /* Tape Puncher/Reader (reels instead of punchcards) */
    IMAGE_PRINTER,     /* Printer device */
    IMAGE_SERIAL,      /* Generic Serial Port */
    IMAGE_PARALLEL,    /* Generic Parallel Port */
    IMAGE_SNAPSHOT,    /* Complete 'snapshot' of the state of the computer */
    IMAGE_QUICKLOAD,   /* Allow to load program/data into memory, without matching any actual device */
    IMAGE_MEMCARD,     /* Memory card */
    IMAGE_CDROM,       /* Optical CD-ROM disc */
	IMAGE_MAGTAPE      /* Magentic tape */
} image_type_t;

#define IMAGE_READABLE         0x00000001
#define IMAGE_WRITEABLE		   0x00000002
#define IMAGE_CREATABLE        0x00000004
#define IMAGE_MUST_BE_LOADED   0x00000008
#define IMAGE_RESET_ON_LOAD    0x00000010
	
typedef struct _image_interface image_interface;
struct _image_interface
{
	image_type_t image_type;
	UINT32 image_attributes;
};


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( image );

/* standard handlers */


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define IMAGE	DEVICE_GET_INFO_NAME(image)

#define MDRV_IMAGE_ADD(_tag,_intf) \
	MDRV_DEVICE_ADD(_tag, IMAGE, 0) \
	MDRV_DEVICE_CONFIG(_intf) 


/***************************************************************************
    DEVICE CALLBACKS
***************************************************************************/

#endif /* __IMAGE_DEV_H__ */
