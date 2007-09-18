/* Bitmask.c - Bitmask support routines - MSH 11/19/1998 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "bitmask.h"
#include <stdlib.h> /* For malloc and free */

/* Bit routines */
static UCHAR maskTable[8] = {
	0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
};

/* Create a new LPBITS struct and return the new structure
 * initialized to 'all bits cleared'
 *
 * nLength is the number of desired bits
 */
LPBITS NewBits(UINT nLength)
{
	LPBITS lpBits = 0;
	UINT   nSize = (nLength+7) / 8;

	lpBits = (LPBITS)malloc(sizeof(BITS));
	if (lpBits)
	{
		lpBits->m_lpBits = (UCHAR *)malloc(nSize);
		if (lpBits->m_lpBits)
		{
			memset(lpBits->m_lpBits, '\0', nSize);
			lpBits->m_nSize = nSize;
		}
		else
		{
			free(lpBits);
			lpBits = 0;
		}
	}

	return lpBits;
}

void DeleteBits(LPBITS lpBits)
{
	if (!lpBits)
		return;

	if (lpBits->m_nSize && lpBits->m_lpBits)
		free(lpBits->m_lpBits);

	free(lpBits);
}

/* Test the 'nBit'th bit */
BOOL TestBit(LPBITS lpBits, UINT nBit)
{
	UINT	offset;
	UCHAR	mask;

	if (nBit < 0 || !lpBits || !lpBits->m_lpBits)
		return FALSE;

	offset = nBit >> 3;

	if (offset >= lpBits->m_nSize)
	{
		return FALSE;
	}

	mask = maskTable[nBit & 7];
	return	(lpBits->m_lpBits[offset] & mask) ? TRUE : FALSE;
}

/* Set the 'nBit'th bit */
void SetBit(LPBITS lpBits, UINT nBit)
{
	UINT	offset;
	UCHAR	mask;

	if (nBit < 0 || !lpBits || !lpBits->m_lpBits)
		return;

	offset = nBit >> 3;

	if (offset >= lpBits->m_nSize)
	{
		return;
	}

	mask = maskTable[nBit & 7];
	lpBits->m_lpBits[offset] |= mask;
}

/* Clear the 'nBit'th bit */
void ClearBit(LPBITS lpBits, UINT nBit)
{
	UINT	offset;
	UCHAR	mask;

	if (nBit < 0 || !lpBits || !lpBits->m_lpBits)
		return;

	offset = nBit >> 3;

	if (offset >= lpBits->m_nSize)
	{
		return;
	}

	mask = maskTable[nBit & 7];
	lpBits->m_lpBits[offset] &= ~mask;
}

/* Set or Clear all bits as specified by 'bSet' */
void SetAllBits(LPBITS lpBits, BOOL bSet)
{
	if (lpBits && lpBits->m_nSize != 0 && lpBits->m_lpBits)
		memset(lpBits->m_lpBits, (!bSet) ? '\0' : '\xFF', lpBits->m_nSize);
}

/* Find next bit that matches 'bSet'
 *
 * 'nStartPos' specifies the bit to start search after
 * 'bSet' specifies to search for a set or unset bit
 *
 * Returns -1 if no bits are found
 */
int FindBit(LPBITS lpBits, int nStartPos, BOOL bSet)
{
	UINT	end;
	UINT	i;
	BOOL	res;

	if (!lpBits || !lpBits->m_nSize || !lpBits->m_lpBits)
		return -1;

	end = lpBits->m_nSize << 3;

	if (nStartPos < 0)
		nStartPos = 0;

	for (i = nStartPos; i < end; i++)
	{
		res = (TestBit(lpBits, i)) ? TRUE : FALSE;
		if ((res && bSet) || (!res && !bSet))
			return i;
	}

	return -1;
}

/* End of source file */
