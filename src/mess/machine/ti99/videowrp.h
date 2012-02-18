/****************************************************************************

    TI-99/4(A) and /8 Video subsystem
    See videowrp.c for documentation

    Michael Zapf
    October 2010
    January 2012: Rewritten as class

*****************************************************************************/

#ifndef __TIVIDEO__
#define __TIVIDEO__

#include "video/tms9928a.h"
#include "video/v9938.h"
#include "ti99defs.h"

class ti_video_device : public bus8z_device
{
public:
	DECLARE_READ16_MEMBER(noread);
	DECLARE_WRITE16_MEMBER(nowrite);

protected:
	address_space	*m_space;
	device_t		*m_cpu;
	tms9928a_device *m_tms9928a;

	/* Constructor */
	ti_video_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock);
	virtual void device_start(void);
	virtual void device_reset(void);
	virtual DECLARE_READ8Z_MEMBER(readz) { };
	virtual DECLARE_WRITE8_MEMBER(write) { };
};

/*
    Connected to the high 8 bits of the 16 bit data bus of the TI-99/4A, D0-D7
*/
class ti_std_video_device : public ti_video_device
{
public:
	ti_std_video_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	DECLARE_READ16_MEMBER(read16);
	DECLARE_WRITE16_MEMBER(write16);
};

/*
    Connected to the multiplexed 8 bit data bus of the TI-99/8
*/
class ti_std8_video_device : public ti_video_device
{
public:
	ti_std8_video_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	DECLARE_READ8Z_MEMBER(readz);
	DECLARE_WRITE8_MEMBER(write);
};

/*
    Used in the EVPC and Geneve
*/
class ti_exp_video_device : public ti_video_device
{
public:
	ti_exp_video_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	void video_update_mouse(int delta_x, int delta_y, int buttons);
	DECLARE_READ8Z_MEMBER(readz);
	DECLARE_WRITE8_MEMBER(write);
	DECLARE_READ16_MEMBER(read16);
	DECLARE_WRITE16_MEMBER(write16);

protected:
	void			device_start(void);
	v9938_device	*m_v9938;
};

/*
    Sound device wrapper (required until sn74296 is converted to a modern device);
    let's hitchhike here - it is not required to create yet another file
    that will be dropped later.
*/
class ti_sound_system_device : public bus8z_device
{
public:
	ti_sound_system_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	// Cannot read from sound; just ignore silently
	DECLARE_READ8Z_MEMBER(readz) { };
	DECLARE_WRITE8_MEMBER(write);

protected:
	void device_start(void);

private:
	device_t *m_sound_chip;
};

/****************************************************************************/

#define MCFG_TI_TMS991x_ADD_NTSC(_tag, _chip, _tmsparam)	\
	MCFG_DEVICE_ADD(_tag, TI994AVIDEO, 0)										\
	MCFG_TMS9928A_ADD( TMS9928A_TAG, _chip, _tmsparam ) 					\
	MCFG_TMS9928A_SCREEN_ADD_NTSC( SCREEN_TAG ) 							\
	MCFG_SCREEN_UPDATE_DEVICE( TMS9928A_TAG, tms9928a_device, screen_update )

#define MCFG_TI_TMS991x_ADD_PAL(_tag, _chip, _tmsparam)		\
	MCFG_DEVICE_ADD(_tag, TI994AVIDEO, 0)										\
	MCFG_TMS9928A_ADD( TMS9928A_TAG, _chip, _tmsparam )						\
	MCFG_TMS9928A_SCREEN_ADD_PAL( SCREEN_TAG )								\
	MCFG_SCREEN_UPDATE_DEVICE( TMS9928A_TAG, tms9928a_device, screen_update )

#define MCFG_TI998_ADD_NTSC(_tag, _chip, _tmsparam)	\
	MCFG_DEVICE_ADD(_tag, TI998VIDEO, 0)										\
	MCFG_TMS9928A_ADD( TMS9928A_TAG, _chip, _tmsparam ) 					\
	MCFG_TMS9928A_SCREEN_ADD_NTSC( SCREEN_TAG ) 							\
	MCFG_SCREEN_UPDATE_DEVICE( TMS9928A_TAG, tms9928a_device, screen_update )

#define MCFG_TI998_ADD_PAL(_tag, _chip, _tmsparam)		\
	MCFG_DEVICE_ADD(_tag, TI998VIDEO, 0)										\
	MCFG_TMS9928A_ADD( TMS9928A_TAG, _chip, _tmsparam )						\
	MCFG_TMS9928A_SCREEN_ADD_PAL( SCREEN_TAG )								\
	MCFG_SCREEN_UPDATE_DEVICE( TMS9928A_TAG, tms9928a_device, screen_update )

#define MCFG_TI_V9938_ADD(_tag, _rate, _screen, _blank, _x, _y, _devtag, _class, _int)		\
	MCFG_DEVICE_ADD(_tag, V9938VIDEO, 0)										\
	MCFG_V9938_ADD(V9938_TAG, _screen, 0x20000)							\
	MCFG_V99X8_INTERRUPT_CALLBACK_DEVICE(_devtag, _class, _int)			\
	MCFG_SCREEN_ADD(_screen, RASTER)										\
	MCFG_SCREEN_REFRESH_RATE(_rate)											\
	MCFG_SCREEN_UPDATE_DEVICE(V9938_TAG, v9938_device, screen_update)	\
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(_blank))					\
	MCFG_SCREEN_SIZE(_x, _y)												\
	MCFG_SCREEN_VISIBLE_AREA(0, _x - 1, 0, _y - 1)							\
	MCFG_PALETTE_LENGTH(512)												\
	MCFG_PALETTE_INIT(v9938)

#define MCFG_TI_SOUND_ADD(_tag)			\
	MCFG_DEVICE_ADD(_tag, TISOUND, 0)

extern const device_type TI994AVIDEO;
extern const device_type TI998VIDEO;
extern const device_type V9938VIDEO;
extern const device_type TISOUND;

#endif /* __TIVIDEO__ */

