/***************************************************************************

    memconv.h

    Functions which help convert between different handlers.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/


/*************************************
 *
 *  16-bit LE using 8-bit handlers
 *
 *************************************/

INLINE UINT16 read16le_with_read8_handler(read8_handler handler, offs_t offset, UINT16 mem_mask)
{
	UINT16 result = 0;
	if ((mem_mask & 0x00ff) != 0x00ff)
		result |= ((UINT16) handler(offset * 2 + 0)) << 0;
	if ((mem_mask & 0xff00) != 0xff00)
		result |= ((UINT16) handler(offset * 2 + 1)) << 8;
	return result;
}


INLINE void write16le_with_write8_handler(write8_handler handler, offs_t offset, UINT16 data, UINT16 mem_mask)
{
	if ((mem_mask & 0x00ff) != 0x00ff)
		handler(offset * 2 + 0, data >> 0);
	if ((mem_mask & 0xff00) != 0xff00)
		handler(offset * 2 + 1, data >> 8);
}



/*************************************
 *
 *  32-bit LE using 8-bit handlers
 *
 *************************************/

INLINE UINT32 read32le_with_read8_handler(read8_handler handler, offs_t offset, UINT32 mem_mask)
{
	UINT32 result = 0;
	if ((mem_mask & 0x0000ffff) != 0x0000ffff)
		result |= read16le_with_read8_handler(handler, offset * 2 + 0, mem_mask) << 0;
	if ((mem_mask & 0xffff0000) != 0xffff0000)
		result |= read16le_with_read8_handler(handler, offset * 2 + 1, mem_mask >> 16) << 16;
	return result;
}


INLINE void write32le_with_write8_handler(write8_handler handler, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	if ((mem_mask & 0x0000ffff) != 0x0000ffff)
		write16le_with_write8_handler(handler, offset * 2 + 0, data, mem_mask);
	if ((mem_mask & 0xffff0000) != 0xffff0000)
		write16le_with_write8_handler(handler, offset * 2 + 1, data >> 16, mem_mask >> 16);
}



/*************************************
 *
 *  64-bit BE using 8-bit handlers
 *
 *************************************/

INLINE UINT64 read64be_with_read8_handler(read8_handler handler, offs_t offset, UINT64 mem_mask)
{
	UINT64 result = 0;
	int i;

	for (i = 0; i < 8; i++)
	{
		if (((mem_mask >> (56 - i * 8)) & 0xff) == 0)
			result |= ((UINT64) handler(offset * 8 + i)) << (56 - i * 8);
	}
	return result;
}

INLINE void write64be_with_write8_handler(write8_handler handler, offs_t offset, UINT64 data, UINT64 mem_mask)
{
	int i;

	for (i = 0; i < 8; i++)
	{
		if (((mem_mask >> (56 - i * 8)) & 0xff) == 0)
			handler(offset * 8 + i, data >> (56 - i * 8));
	}
}



/*************************************
 *
 *  64-bit LE using 32-bit LE handlers
 *
 *************************************/

INLINE UINT64 read64le_with_32le_handler(read32_handler handler, offs_t offset, UINT64 mem_mask)
{
	UINT64 result = 0;
	if ((mem_mask & U64(0x00000000ffffffff)) != U64(0x00000000ffffffff))
		result |= ((UINT64) handler(offset * 2 + 0, (UINT32) (mem_mask >> 0))) << 0;
	if ((mem_mask & U64(0xffffffff00000000)) != U64(0xffffffff00000000))
		result |= ((UINT64) handler(offset * 2 + 1, (UINT32) (mem_mask >> 32))) << 32;
	return result;
}

INLINE void write64le_with_32le_handler(write32_handler handler, offs_t offset, UINT64 data, UINT64 mem_mask)
{
	if ((mem_mask & U64(0x00000000ffffffff)) != U64(0x00000000ffffffff))
		handler(offset * 2 + 0, data >>  0, mem_mask >>  0);
	if ((mem_mask & U64(0xffffffff00000000)) != U64(0xffffffff00000000))
		handler(offset * 2 + 1, data >> 32, mem_mask >> 32);
}



/*************************************
 *
 *  64-bit BE using 32-bit LE handlers
 *
 *************************************/

INLINE UINT64 read64be_with_32le_handler(read32_handler handler, offs_t offset, UINT64 mem_mask)
{
	UINT64 result;
	mem_mask = FLIPENDIAN_INT64(mem_mask);
	result = read64le_with_32le_handler(handler, offset, mem_mask);
	return FLIPENDIAN_INT64(result);
}

INLINE void write64be_with_32le_handler(write32_handler handler, offs_t offset, UINT64 data, UINT64 mem_mask)
{
	data = FLIPENDIAN_INT64(data);
	mem_mask = FLIPENDIAN_INT64(mem_mask);
	write64le_with_32le_handler(handler, offset, data, mem_mask);
}
