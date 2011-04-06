/*****************************************************************************
 *
 * includes/pc_ide.h
 *
 * ibm at mfm hard disk
 * interface still used in ide devices!
 *
 ****************************************************************************/

#ifndef PC_IDE_H_
#define PC_IDE_H_


/*----------- defined in machine/pc_ide.c -----------*/

READ8_HANDLER(at_mfm_0_r);
WRITE8_HANDLER(at_mfm_0_w);

MACHINE_CONFIG_EXTERN( pc_ide );

#endif /* PC_IDE_H_ */
