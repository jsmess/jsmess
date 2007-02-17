void cbm_ieee_open(void);

void cbm_ieee_dav_w(int device, int data);
void cbm_ieee_nrfd_w(int device, int data);
void cbm_ieee_ndac_w(int device, int data);
void cbm_ieee_atn_w(int device, int data);
void cbm_ieee_eoi_w(int device, int data);
void cbm_ieee_data_w(int device, int data);

int cbm_ieee_srq_r(void);
int cbm_ieee_dav_r(void);
int cbm_ieee_nrfd_r(void);
int cbm_ieee_ndac_r(void);
int cbm_ieee_atn_r(void);
int cbm_ieee_eoi_r(void);
int cbm_ieee_data_r(void);

/* for debugging  */
 READ8_HANDLER(cbm_ieee_state);
