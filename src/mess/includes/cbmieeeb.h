/*****************************************************************************
 *
 * includes/cbmieeeb.h
 *
 ****************************************************************************/

#ifndef CBMIEEEB_H_
#define CBMIEEEB_H_


/*----------- defined in machine/cbmieeeb.c -----------*/

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define CBM_IEEEBUS		DEVICE_GET_INFO_NAME(cbm_ieee_bus)

#define MDRV_CBM_IEEEBUS_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, CBM_IEEEBUS, 0)

/***************************************************************************
    PROTOTYPES
***************************************************************************/

DEVICE_GET_INFO( cbm_ieee_bus );

void cbm_ieee_dav_w(const device_config *ieeedev, int device, int data);
void cbm_ieee_nrfd_w(const device_config *ieeedev, int device, int data);
void cbm_ieee_ndac_w(const device_config *ieeedev, int device, int data);
void cbm_ieee_atn_w(const device_config *ieeedev, int device, int data);
void cbm_ieee_eoi_w(const device_config *ieeedev, int device, int data);
void cbm_ieee_data_w(const device_config *ieeedev, int device, int data);

WRITE8_DEVICE_HANDLER( cbm_ieee_dav_write );
WRITE8_DEVICE_HANDLER( cbm_ieee_nrfd_write );
WRITE8_DEVICE_HANDLER( cbm_ieee_ndac_write );
WRITE8_DEVICE_HANDLER( cbm_ieee_atn_write );
WRITE8_DEVICE_HANDLER( cbm_ieee_eoi_write );
WRITE8_DEVICE_HANDLER( cbm_ieee_data_write );

READ8_DEVICE_HANDLER( cbm_ieee_srq_r );
READ8_DEVICE_HANDLER( cbm_ieee_dav_r );
READ8_DEVICE_HANDLER( cbm_ieee_nrfd_r );
READ8_DEVICE_HANDLER( cbm_ieee_ndac_r );
READ8_DEVICE_HANDLER( cbm_ieee_atn_r );
READ8_DEVICE_HANDLER( cbm_ieee_eoi_r );
READ8_DEVICE_HANDLER( cbm_ieee_data_r );

/* for debugging  */
READ8_DEVICE_HANDLER( cbm_ieee_state );


#endif /* CBMIEEEB_H_ */
