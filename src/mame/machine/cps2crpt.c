/******************************************************************************

CPS-2 Encryption

All credit goes to Andreas Naive for breaking the encryption algorithm.
Code by Nicola Salmoria.
Thanks to Charles MacDonald and Razoola for extracting the data from the hardware.


The encryption only affects opcodes, not data.

It consists of two 4-round Feistel networks (FN) and involves both
the 16-bit opcode and the low 16 bits of the address.

Let be:

E = 16-bit ciphertext
A = 16-bit address
K = 64-bit key
D = 16-bit plaintext
y = FN1(x,k) = function describing the first Feistel network (x,y = 16 bit, k = 64 bit)
y = FN2(x,k) = function describing the second Feistel network (x,y = 16 bit, k = 64 bit)
y = EX(x) = fixed function that expands the 16-bit x to the 64-bit y

Then the cipher can be described as:

D = FN2( E, K XOR EX( FN1(A, K ) ) )


Each round of the Feistel networks consists of four substitution boxes. The boxes
have 6 inputs and 2 outputs. Usually the input is the XOR of a data bit and a key
bit, however in some cases only the key is used.

(TODO-notes about accuracy of s-boxes)

The s-boxes were chosen in order to use an empty key (all FF) for the dead board.


Also, the hardware has different watchdog opcodes and address range (see below)
which are stored in the battery backed RAM. There doesn't appear to be any relation
between those and the 64-bit encryption key, so they probably use an additional
64 bits of battery-backed RAM.



First FN:

 B(0 1 3 5 8 9 11 12)        A(10 4 6 7 2 13 15 14)
         L0                             R0
         |                              |
        XOR<-----------[F1]<------------|
         |                              |
         R1                             L1
         |                              |
         |------------>[F2]----------->XOR
         |                              |
         L2                             R2
         |                              |
        XOR<-----------[F3]<------------|
         |                              |
         R3                             L3
         |                              |
         |------------>[F4]----------->XOR
         |                              |
         L4                             R4
  (10 4 6 7 2 13 15 14)       (0 1 3 5 8 9 11 12)


Second FN:

 B(3 5 9 10 8 15 12 11)      A(6 0 2 13 1 4 14 7)
         L0                             R0
         |                              |
        XOR<-----------[F1]<------------|
         |                              |
         R1                             L1
         |                              |
         |------------>[F2]----------->XOR
         |                              |
         L2                             R2
         |                              |
        XOR<-----------[F3]<------------|
         |                              |
         R3                             L3
         |                              |
         |------------>[F4]----------->XOR
         |                              |
         L4                             R4
  (6 0 2 13 1 4 14 7)         (3 5 9 10 8 15 12 11)

******************************************************************************

Some Encryption notes.
----------------------

Address range.

The encryption does _not_ cover the entire address space. The range covered
differs per game.


Encryption Watchdog.

The CPS2 system has a watchdog system that will disable the decryption
of data if the watchdog isn't triggered at least once every few seconds.
The trigger varies from game to game (some games do use the same) and is
basically a 68000 opcode/s instruction. The instruction is the same for
all regions of the game. The watchdog instructions are listed alongside
the decryption keys.

*******************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68kmame.h"
#include "ui.h"
#include "includes/cps1.h"


/******************************************************************************/

static const int fn1_groupA[8] = { 10, 4, 6, 7, 2, 13, 15, 14 };
static const int fn1_groupB[8] = {  0, 1, 3, 5, 8,  9, 11, 12 };

static const int fn2_groupA[8] = { 6, 0, 2, 13, 1,  4, 14,  7 };
static const int fn2_groupB[8] = { 3, 5, 9, 10, 8, 15, 12, 11 };

/******************************************************************************/

// The order of the input and output bits in the s-boxes is arbitrary.
// Each s-box can be XORed with an arbitrary vale in range 0-3 (but the same value
// must be used for the corresponding output bits in f1 and f3 or in f2 and f4)

struct sbox
{
	const int table[64];
	const int inputs[6];		// positions of the inputs bits, -1 means no input except from key
	const int outputs[2];		// positions of the output bits
};


static const struct sbox fn1_r1_boxes[4] =
{
	{	// subkey bits  0- 5
		{
			0,2,2,0,1,0,1,1,3,2,0,3,0,3,1,2,1,1,1,2,1,3,2,2,2,3,3,2,1,1,1,2,
			2,2,0,0,3,1,3,1,1,1,3,0,0,1,0,0,1,2,2,1,2,3,2,2,2,3,1,3,2,0,1,3,
		},
		{ 3, 4, 5, 6, -1, -1 },
		{ 3, 6 }
	},
	{	// subkey bits  6-11
		{
			3,0,2,2,2,1,1,1,1,2,1,0,0,0,2,3,2,3,1,3,0,0,0,2,1,2,2,3,0,3,3,3,
			0,1,3,2,3,3,3,1,1,1,1,2,0,1,2,1,3,2,3,1,1,3,2,2,2,3,1,3,2,3,0,0,
		},
		{ 0, 1, 2, 4, 7, -1 },
		{ 2, 7 }
	},
	{	// subkey bits 12-17
		{
			3,0,3,1,1,0,2,2,3,1,2,0,3,3,2,3,0,1,0,1,2,3,0,2,0,2,0,1,0,0,1,0,
			2,3,1,2,1,0,2,0,2,1,0,1,0,2,1,0,3,1,2,3,1,3,1,1,1,2,0,2,2,0,0,0,
		},
		{ 0, 1, 2, 3, 6, 7 },
		{ 0, 1 }
	},
	{	// subkey bits 18-23
		{
			3,2,0,3,0,2,2,1,1,2,3,2,1,3,2,1,2,2,1,3,3,2,1,0,1,0,1,3,0,0,0,2,
			2,1,0,1,0,1,0,1,3,1,1,2,2,3,2,0,3,3,2,0,2,1,3,3,0,0,3,0,1,1,3,3,
		},
		{ 0, 1, 3, 5, 6, 7 },
		{ 4, 5 }
	},
};

static const struct sbox fn1_r2_boxes[4] =
{
	{	// subkey bits 24-29
		{
			3,3,2,0,3,0,3,1,0,3,0,1,0,2,1,3,1,3,0,3,3,1,3,3,3,2,3,2,2,3,1,2,
			0,2,2,1,0,1,2,0,3,3,0,1,3,2,1,2,3,0,1,3,0,1,2,2,1,2,1,2,0,1,3,0,
		},
		{ 0, 1, 2, 3, 6, -1 },
		{ 1, 6 }
	},
	{	// subkey bits 30-35
		{
			1,2,3,2,1,3,0,1,1,0,2,0,0,2,3,2,3,3,0,1,2,2,1,0,1,0,1,2,3,2,1,3,
			2,2,2,0,1,0,2,3,2,1,2,1,2,1,0,3,0,1,2,3,1,2,1,3,2,0,3,2,3,0,2,0,
		},
		{ 2, 4, 5, 6, 7, -1 },
		{ 5, 7 }
	},
	{	// subkey bits 36-41
		{
			0,1,0,2,1,1,0,1,0,2,2,2,1,3,0,0,1,1,3,1,2,2,2,3,1,0,3,3,3,2,2,2,
			1,1,3,0,3,1,3,0,1,3,3,2,1,1,0,0,1,2,2,2,1,1,1,2,2,0,0,3,2,3,1,3,
		},
		{ 1, 2, 3, 4, 5, 7 },
		{ 0, 3 }
	},
	{	// subkey bits 42-47
		{
			2,1,0,3,3,3,2,0,1,2,1,1,1,0,3,1,1,3,3,0,1,2,1,0,0,0,3,0,3,0,3,0,
			1,3,3,3,0,3,2,0,2,1,2,2,2,1,1,3,0,1,0,1,0,1,1,1,1,3,1,0,1,2,3,3,
		},
		{ 0, 1, 3, 4, 6, 7 },
		{ 2, 4 }
	},
};

static const struct sbox fn1_r3_boxes[4] =
{
	{	// subkey bits 48-53
		{
			0,0,0,3,3,1,1,0,2,0,2,0,0,0,3,2,0,1,2,3,2,2,1,0,3,0,0,0,0,0,2,3,
			3,0,0,1,1,2,3,3,0,1,3,2,0,1,3,3,2,0,0,1,0,2,0,0,0,3,1,3,3,3,3,3,
		},
		{ 0, 1, 5, 6, 7, -1 },
		{ 0, 5 }
	},
	{	// subkey bits 54-59
		{
			2,3,2,3,0,2,3,0,2,2,3,0,3,2,0,2,1,0,2,3,1,1,1,0,0,1,0,2,1,2,2,1,
			3,0,2,1,2,3,3,0,3,2,3,1,0,2,1,0,1,2,2,3,0,2,1,3,1,3,0,2,1,1,1,3,
		},
		{ 2, 3, 4, 6, 7, -1 },
		{ 6, 7 }
	},
	{	// subkey bits 60-65
		{
			3,0,2,1,1,3,1,2,2,1,2,2,2,0,0,1,2,3,1,0,2,0,0,2,3,1,2,0,0,0,3,0,
			2,1,1,2,0,0,1,2,3,1,1,2,0,1,3,0,3,1,1,0,0,2,3,0,0,0,0,3,2,0,0,0,
		},
		{ 0, 2, 3, 4, 5, 6 },
		{ 1, 4 }
	},
	{	// subkey bits 66-71
		{
			0,1,0,0,2,1,3,2,3,3,2,1,0,1,1,1,1,1,0,3,3,1,1,0,0,2,2,1,0,3,3,2,
			1,3,3,0,3,0,2,1,1,2,3,2,2,2,1,0,0,3,3,3,2,2,3,1,0,2,3,0,3,1,1,0,
		},
		{ 0, 1, 2, 3, 5, 7 },
		{ 2, 3 }
	},
};

static const struct sbox fn1_r4_boxes[4] =
{
	{	// subkey bits 72-77
		{
			1,1,1,1,1,0,1,3,3,2,3,0,1,2,0,2,3,3,0,1,2,1,2,3,0,3,2,3,2,0,1,2,
			0,1,0,3,2,1,3,2,3,1,2,3,2,0,1,2,2,0,0,0,2,1,3,0,3,1,3,0,1,3,3,0,
		},
		{ 1, 2, 3, 4, 5, 7 },
		{ 0, 4 }
	},
	{	// subkey bits 78-83
		{
			3,0,0,0,0,1,0,2,3,3,1,3,0,3,1,2,2,2,3,1,0,0,2,0,1,0,2,2,3,3,0,0,
			1,1,3,0,2,3,0,3,0,3,0,2,0,2,0,1,0,3,0,1,3,1,1,0,0,1,3,3,2,2,1,0,
		},
		{ 0, 1, 2, 3, 5, 6 },
		{ 1, 3 }
	},
	{	// subkey bits 84-89
		{
			0,1,1,2,0,1,3,1,2,0,3,2,0,0,3,0,3,0,1,2,2,3,3,2,3,2,0,1,0,0,1,0,
			3,0,2,3,0,2,2,2,1,1,0,2,2,0,0,1,2,1,1,1,2,3,0,3,1,2,3,3,1,1,3,0,
		},
		{ 0, 2, 4, 5, 6, 7 },
		{ 2, 6 }
	},
	{	// subkey bits 90-95
		{
			0,1,2,2,0,1,0,3,2,2,1,1,3,2,0,2,0,1,3,3,0,2,2,3,3,2,0,0,2,1,3,3,
			1,1,1,3,1,2,1,1,0,3,3,2,3,2,3,0,3,1,0,0,3,0,0,0,2,2,2,1,2,3,0,0,
		},
		{ 0, 1, 3, 4, 6, 7 },
		{ 5, 7 }
	},
};

/******************************************************************************/

static const struct sbox fn2_r1_boxes[4] =
{
	{	// subkey bits  0- 5
		{
			2,0,2,0,3,0,0,3,1,1,0,1,3,2,0,1,2,0,1,2,0,2,0,2,2,2,3,0,2,1,3,0,
			0,1,0,1,2,2,3,3,0,3,0,2,3,0,1,2,1,1,0,2,0,3,1,1,2,2,1,3,1,1,3,1,
		},
		{ 0, 3, 4, 5, 7, -1 },
		{ 6, 7 }
	},
	{	// subkey bits  6-11
		{
			1,1,0,3,0,2,0,1,3,0,2,0,1,1,0,0,1,3,2,2,0,2,2,2,2,0,1,3,3,3,1,1,
			1,3,1,3,2,2,2,2,2,2,0,1,0,1,1,2,3,1,1,2,0,3,3,3,2,2,3,1,1,1,3,0,
		},
		{ 1, 2, 3, 4, 6, -1 },
		{ 3, 5 }
	},
	{	// subkey bits 12-17
		{
			1,0,2,2,3,3,3,3,1,2,2,1,0,1,2,1,1,2,3,1,2,0,0,1,2,3,1,2,0,0,0,2,
			2,0,1,1,0,0,2,0,0,0,2,3,2,3,0,1,3,0,0,0,2,3,2,0,1,3,2,1,3,1,1,3,
		},
		{ 1, 2, 4, 5, 6, 7 },
		{ 1, 4 }
	},
	{	// subkey bits 18-23
		{
			1,3,3,0,3,2,3,1,3,2,1,1,3,3,2,1,2,3,0,3,1,0,0,2,3,0,0,0,3,3,0,1,
			2,3,0,0,0,1,2,1,3,0,0,1,0,2,2,2,3,3,1,2,1,3,0,0,0,3,0,1,3,2,2,0,
		},
		{ 0, 2, 3, 5, 6, 7 },
		{ 0, 2 }
	},
};

static const struct sbox fn2_r2_boxes[4] =
{
	{	// subkey bits 24-29
		{
			3,1,3,0,3,0,3,1,3,0,0,1,1,3,0,3,1,1,0,1,2,3,2,3,3,1,2,2,2,0,2,3,
			2,2,2,1,1,3,3,0,3,1,2,1,1,1,0,2,0,3,3,0,0,2,0,0,1,1,2,1,2,1,1,0,
		},
		{ 0, 2, 4, 6, -1, -1 },
		{ 4, 6 }
	},
	{	// subkey bits 30-35
		{
			0,3,0,3,3,2,1,2,3,1,1,1,2,0,2,3,0,3,1,2,2,1,3,3,3,2,1,2,2,0,1,0,
			2,3,0,1,2,0,1,1,2,0,2,1,2,0,2,3,3,1,0,2,3,3,0,3,1,1,3,0,0,1,2,0,
		},
		{ 1, 3, 4, 5, 6, 7 },
		{ 0, 3 }
	},
	{	// subkey bits 36-41
		{
			0,0,2,1,3,2,1,0,1,2,2,2,1,1,0,3,1,2,2,3,2,1,1,0,3,0,0,1,1,2,3,1,
			3,3,2,2,1,0,1,1,1,2,0,1,2,3,0,3,3,0,3,2,2,0,2,2,1,2,3,2,1,0,2,1,
		},
		{ 0, 1, 3, 4, 5, 7 },
		{ 1, 7 }
	},
	{	// subkey bits 42-47
		{
			0,2,1,2,0,2,2,0,1,3,2,0,3,2,3,0,3,3,2,3,1,2,3,1,2,2,0,0,2,2,1,2,
			2,3,3,3,1,1,0,0,0,3,2,0,3,2,3,1,1,1,1,0,1,0,1,3,0,0,1,2,2,3,2,0,
		},
		{ 1, 2, 3, 5, 6, 7 },
		{ 2, 5 }
	},
};

static const struct sbox fn2_r3_boxes[4] =
{
	{	// subkey bits 48-53
		{
			2,1,2,1,2,3,1,3,2,2,1,3,3,0,0,1,0,2,0,3,3,1,0,0,1,1,0,2,3,2,1,2,
			1,1,2,1,1,3,2,2,0,2,2,3,3,3,2,0,0,0,0,0,3,3,3,0,1,2,1,0,2,3,3,1,
		},
		{ 2, 3, 4, 6, -1, -1 },
		{ 3, 5 }
	},
	{	// subkey bits 54-59
		{
			3,2,3,3,1,0,3,0,2,0,1,1,1,0,3,0,3,1,3,1,0,1,2,3,2,2,3,2,0,1,1,2,
			3,0,0,2,1,0,0,2,2,0,1,0,0,2,0,0,1,3,1,3,2,0,3,3,1,0,2,2,2,3,0,0,
		},
		{ 0, 1, 3, 5, 7, -1 },
		{ 0, 2 }
	},
	{	// subkey bits 60-65
		{
			2,2,1,0,2,3,3,0,0,0,1,3,1,2,3,2,2,3,1,3,0,3,0,3,3,2,2,1,0,0,0,2,
			1,2,2,2,0,0,1,2,0,1,3,0,2,3,2,1,3,2,2,2,3,1,3,0,2,0,2,1,0,3,3,1,
		},
		{ 0, 1, 2, 3, 5, 7 },
		{ 1, 6 }
	},
	{	// subkey bits 66-71
		{
			1,2,3,2,0,2,1,3,3,1,0,1,1,2,2,0,0,1,1,1,2,1,1,2,0,1,3,3,1,1,1,2,
			3,3,1,0,2,1,1,1,2,1,0,0,2,2,3,2,3,2,2,0,2,2,3,3,0,2,3,0,2,2,1,1,
		},
		{ 0, 2, 4, 5, 6, 7 },
		{ 4, 7 }
	},
};

static const struct sbox fn2_r4_boxes[4] =
{
	{	// subkey bits 72-77
		{
			2,0,1,1,2,1,3,3,1,1,1,2,0,1,0,2,0,1,2,0,2,3,0,2,3,3,2,2,3,2,0,1,
			3,0,2,0,2,3,1,3,2,0,0,1,1,2,3,1,1,1,0,1,2,0,3,3,1,1,1,3,3,1,1,0,
		},
		{ 0, 1, 3, 6, 7, -1 },
		{ 0, 3 }
	},
	{	// subkey bits 78-83
		{
			1,2,2,1,0,3,3,1,0,2,2,2,1,0,1,0,1,1,0,1,0,2,1,0,2,1,0,2,3,2,3,3,
			2,2,1,2,2,3,1,3,3,3,0,1,0,1,3,0,0,0,1,2,0,3,3,2,3,2,1,3,2,1,0,2,
		},
		{ 0, 1, 2, 4, 5, 6 },
		{ 4, 7 }
	},
	{	// subkey bits 84-89
		{
			2,3,2,1,3,2,3,0,0,2,1,1,0,0,3,2,3,1,0,1,2,2,2,1,3,2,2,1,0,2,1,2,
			0,3,1,0,0,3,1,1,3,3,2,0,1,0,1,3,0,0,1,2,1,2,3,2,1,0,0,3,2,1,1,3,
		},
		{ 0, 2, 3, 4, 5, 7 },
		{ 1, 2 }
	},
	{	// subkey bits 90-95
		{
			2,0,0,3,2,2,2,1,3,3,1,1,2,0,0,3,1,0,3,2,1,0,2,0,3,2,2,3,2,0,3,0,
			1,3,0,2,2,1,3,3,0,1,0,3,1,1,3,2,0,3,0,2,3,2,1,3,2,3,0,0,1,3,2,1,
		},
		{ 2, 3, 4, 5, 6, 7 },
		{ 5, 6 }
	},
};

/******************************************************************************/


static int extract_inputs(unsigned int val, const int *inputs)
{
	int i;
	int res = 0;

	for (i = 0; i < 6; ++i)
	{
		if (inputs[i] != -1)
			res |= BIT(val, inputs[i]) << i;
	}

	return res;
}



static UINT8 fn(UINT8 in, const struct sbox *sboxes, UINT32 key)
{
	int box;
	UINT8 res = 0;

	for (box = 0; box < 4; ++box)
	{
		const struct sbox *sbox = &sboxes[box];
		const unsigned int subkey = (key >> (6*box)) & 0x3f;

		int out = sbox->table[extract_inputs(in, sbox->inputs) ^ subkey];

		if (out & 1)
			res |= 1 << sbox->outputs[0];
		if (out & 2)
			res |= 1 << sbox->outputs[1];
	}

	return res;
}



// srckey is the 64-bit master key (2x32 bits)
// dstkey will contain the 96-bit key for the 1st FN (4x24 bits)
void expand_1st_key(unsigned int *dstkey, const unsigned int *srckey)
{
	static const int bits[96] =
	{
		 0,  1,  2,  3,  4,  5,
		 6,  7,  8,  9, 10, 11,
		12, 13, 14, 15, 16, 17,
		18, 19, 20, 21, 22, 23,
		24, 25, 26, 27, 19, 28,
		18, 29, 23, 13, 30, 31,
		32, 33, 34, 35, 36, 11,
		37, 38, 39, 40, 41, 42,
		16,  0, 27, 32, 38, 43,
		44, 45, 46, 47,  9, 34,
		10, 48, 49, 50, 51, 31,
		25, 52, 53, 54, 24, 55,
		41, 22, 53, 56, 50, 36,
		40, 57, 58, 44,  6, 59,
		60, 28, 49, 61, 14, 29,
		55,  5, 30, 62, 47, 63,
	};
	int i;

	dstkey[0] = 0;
	dstkey[1] = 0;
	dstkey[2] = 0;
	dstkey[3] = 0;

	for (i = 0; i < 96; ++i)
		dstkey[i / 24] |= BIT(srckey[bits[i] / 32], bits[i] % 32) << (i % 24);
}


// srckey is the 64-bit master key (2x32 bits) XORed with the subkey
// dstkey will contain the 96-bit key for the 2nd FN (4x24 bits)
void expand_2nd_key(unsigned int *dstkey, const unsigned int *srckey)
{
	static const int bits[96] =
	{
		20, 57, 26, 58, 47, 40,
		51, 22, 33, 25, 62, 59,
		54,  1, 32, 45, 37, 42,
		43, 19, 52, 18, 17, 15,
		44, 14,  9, 53, 40, 61,
		 5,  8, 56, 22, 63, 31,
		33, 51, 39, 36, 10, 48,
		43, 28,  6, 59, 55, 15,
		34, 35, 54, 17, 21,  7,
		 2, 36,  5, 13, 60, 16,
		58,  3, 52, 44,  0,  9,
		56, 53, 24, 42, 41, 26,
		21,  7, 11, 24, 12,  4,
		46, 27,  8, 41, 29,  2,
		50, 49, 47, 55, 23,  0,
		39, 30, 14, 62, 38, 60,
	};
	int i;

	dstkey[0] = 0;
	dstkey[1] = 0;
	dstkey[2] = 0;
	dstkey[3] = 0;

	for (i = 0; i < 96; ++i)
		dstkey[i / 24] |= BIT(srckey[bits[i] / 32], bits[i] % 32) << (i % 24);
}


// seed is the 16-bit seed generated by the first FN
// subkey will contain the 64-bit key to be XORed with the master key
// for the 2nd FN (2x32 bits)
static void expand_subkey(UINT32* subkey, UINT16 seed)
{
	// Note that every bit of the seed is used exactly 4 times.
	// This might be a hint to the proper order of the 64 bits of the master key.
	static const int bits[64] =
	{
		10,  0, 11,  6,  5,  3,  9, 15,
		 9,  5,  0,  5,  3,  7, 14,  2,
		10,  1,  8, 15,  2, 12,  6, 11,
		14,  7,  4,  0, 15, 13,  4,  1,
		 7,  3, 14,  9, 15, 13,  1, 10,
		 2,  9,  8, 11,  3, 10, 12, 13,
		 4,  5,  7, 12,  2, 13,  0, 14,
		 1,  8,  4,  6,  6, 11,  8, 12,
	};
	int i;

	subkey[0] = 0;
	subkey[1] = 0;

	for (i = 0; i < 64; ++i)
		subkey[i / 32] |= BIT(seed, bits[i]) << (i % 32);
}



static UINT16 feistel(UINT16 val, const int *bitsA, const int *bitsB,
		const struct sbox* boxes1, const struct sbox* boxes2, const struct sbox* boxes3, const struct sbox* boxes4,
		UINT32 key1, UINT32 key2, UINT32 key3, UINT32 key4)
{
	const UINT8 l0 = BITSWAP8(val, bitsB[7],bitsB[6],bitsB[5],bitsB[4],bitsB[3],bitsB[2],bitsB[1],bitsB[0]);
	const UINT8 r0 = BITSWAP8(val, bitsA[7],bitsA[6],bitsA[5],bitsA[4],bitsA[3],bitsA[2],bitsA[1],bitsA[0]);

	const UINT8 l1 = r0;
	const UINT8 r1 = l0 ^ fn(r0, boxes1, key1);

	const UINT8 l2 = r1;
	const UINT8 r2 = l1 ^ fn(r1, boxes2, key2);

	const UINT8 l3 = r2;
	const UINT8 r3 = l2 ^ fn(r2, boxes3, key3);

	const UINT8 l4 = r3;
	const UINT8 r4 = l3 ^ fn(r3, boxes4, key4);

	return
		(BIT(l4, 0) << bitsA[0]) |
		(BIT(l4, 1) << bitsA[1]) |
		(BIT(l4, 2) << bitsA[2]) |
		(BIT(l4, 3) << bitsA[3]) |
		(BIT(l4, 4) << bitsA[4]) |
		(BIT(l4, 5) << bitsA[5]) |
		(BIT(l4, 6) << bitsA[6]) |
		(BIT(l4, 7) << bitsA[7]) |
		(BIT(r4, 0) << bitsB[0]) |
		(BIT(r4, 1) << bitsB[1]) |
		(BIT(r4, 2) << bitsB[2]) |
		(BIT(r4, 3) << bitsB[3]) |
		(BIT(r4, 4) << bitsB[4]) |
		(BIT(r4, 5) << bitsB[5]) |
		(BIT(r4, 6) << bitsB[6]) |
		(BIT(r4, 7) << bitsB[7]);
}



static void cps2_decrypt(const UINT32 *master_key, unsigned int upper_limit)
{
	UINT16 *rom = (UINT16 *)memory_region(REGION_CPU1);
	int length = memory_region_length(REGION_CPU1);
	UINT16 *dec = auto_malloc(length);
	int i;
	UINT32 key1[4];


	// expand master key to 1st FN 96-bit key
	expand_1st_key(key1, master_key);

	// add extra bits for s-boxes with less than 6 inputs
	key1[0] ^= BIT(key1[0], 1) <<  4;
	key1[0] ^= BIT(key1[0], 2) <<  5;
	key1[0] ^= BIT(key1[0], 8) << 11;
	key1[1] ^= BIT(key1[1], 0) <<  5;
	key1[1] ^= BIT(key1[1], 8) << 11;
	key1[2] ^= BIT(key1[2], 1) <<  5;
	key1[2] ^= BIT(key1[2], 8) << 11;

	for (i = 0; i < 0x10000; ++i)
	{
		int a;
		UINT16 seed;
		UINT32 subkey[2];
		UINT32 key2[4];

		if ((i & 0xff) == 0)
		{
			char loadingMessage[256]; // for displaying with UI
			sprintf(loadingMessage, "Decrypting %d%%", i*100/0x10000);
			ui_set_startup_text(loadingMessage,FALSE);
		}


		// pass the address through FN1
		seed = feistel(i, fn1_groupA, fn1_groupB,
				fn1_r1_boxes, fn1_r2_boxes, fn1_r3_boxes, fn1_r4_boxes,
				key1[0], key1[1], key1[2], key1[3]);


		// expand the result to 64-bit
		expand_subkey(subkey, seed);

		// XOR with the master key
		subkey[0] ^= master_key[0];
		subkey[1] ^= master_key[1];

		// expand key to 2nd FN 96-bit key
		expand_2nd_key(key2, subkey);

		// add extra bits for s-boxes with less than 6 inputs
		key2[0] ^= BIT(key2[0], 0) <<  5;
		key2[0] ^= BIT(key2[0], 6) << 11;
		key2[1] ^= BIT(key2[1], 0) <<  5;
		key2[1] ^= BIT(key2[1], 1) <<  4;
		key2[2] ^= BIT(key2[2], 2) <<  5;
		key2[2] ^= BIT(key2[2], 3) <<  4;
		key2[2] ^= BIT(key2[2], 7) << 11;
		key2[3] ^= BIT(key2[3], 1) <<  5;


		// decrypt the opcodes
		for (a = i; a < length/2 && a < upper_limit/2; a += 0x10000)
		{
			dec[a] = feistel(rom[a], fn2_groupA, fn2_groupB,
				fn2_r1_boxes, fn2_r2_boxes, fn2_r3_boxes, fn2_r4_boxes,
				key2[0], key2[1], key2[2], key2[3]);
		}
		// copy the unencrypted part (not really needed)
		while (a < length/2)
		{
			dec[a] = rom[a];
			a += 0x10000;
		}
	}

	memory_set_decrypted_region(0, 0x000000, length - 1, dec);
	m68k_set_encrypted_opcode_range(0,0,length);

#if 0
{
FILE *f;

f = fopen("d:/s.rom","wb");
fwrite(rom,1,0x100000,f);
fclose(f);
f = fopen("d:/s.dec","wb");
fwrite(dec,1,0x100000,f);
fclose(f);
}
#endif
}










struct game_keys
{
	const char *name;             /* game driver name */
	const UINT32 keys[2];
	UINT32 upper_limit;
};


/*
(1) On a dead board, the only encrypted range is actually FF0000-FFFFFF.
It doesn't start from 0, and it's the upper half of a 128kB bank.
*/

static const struct game_keys keys_table[] =
{
	// name                 key               upper                  watchdog
	{ "dead",     { 0xffffffff,0xffffffff },  /*(1)*/ },	// ffff ffff ffff
	{ "ssf2",     { 0xb7443350,0x2f4653d7 }, 0x400000 },	// 0838 0007 2000  btst    #7,$2000
	{ "ssf2u",    { 0xf0ae3d08,0x420dd6bf }, 0x400000 },	// 0838 0007 2000  btst    #7,$2000
	{ "ssf2a",    { 0x6260014f,0xd857f7a7 }, 0x400000 },	// 0838 0007 2000  btst    #7,$2000
	{ "ssf2ar1",  { 0x6260014f,0xd857f7a7 }, 0x400000 },	// 0838 0007 2000  btst    #7,$2000
	{ "ssf2j",    { 0x3d9e1e15,0xa58c32ce }, 0x400000 },	// 0838 0007 2000  btst    #7,$2000
	{ "ssf2jr1",  { 0x3d9e1e15,0xa58c32ce }, 0x400000 },	// 0838 0007 2000  btst    #7,$2000
	{ "ssf2jr2",  { 0x3d9e1e15,0xa58c32ce }, 0x400000 },	// 0838 0007 2000  btst    #7,$2000
	{ "ssf2tb",   { 0x3599df35,0xad98284c }, 0x400000 },	// 0838 0007 2000  btst    #7,$2000
	{ "ssf2tbr1", { 0x3599df35,0xad98284c }, 0x400000 },	// 0838 0007 2000  btst    #7,$2000
	{ "ssf2tbj",  { 0x8758e392,0x3ffa1a50 }, 0x400000 },	// 0838 0007 2000  btst    #7,$2000
	{ "ddtod",    { 0xbea213ce,0xb61e8d81 }, 0x180000 },	// 0C78 1019 4000  cmpi.w  #$1019,$4000
	{ "ddtodr1",  { 0xbea213ce,0xb61e8d81 }, 0x180000 },	// 0C78 1019 4000  cmpi.w  #$1019,$4000
	{ "ddtodu",   { 0x41e77fbe,0x30db090c }, 0x180000 },	// 0C78 1019 4000  cmpi.w  #$1019,$4000
	{ "ddtodur1", { 0x41e77fbe,0x30db090c }, 0x180000 },	// 0C78 1019 4000  cmpi.w  #$1019,$4000
	{ "ddtodj",   { 0x2f543984,0x1fa51d7a }, 0x180000 },	// 0C78 1019 4000  cmpi.w  #$1019,$4000
	{ "ddtodjr1", { 0x2f543984,0x1fa51d7a }, 0x180000 },	// 0C78 1019 4000  cmpi.w  #$1019,$4000
//  { "ddtoda",   {  /*not enough data*/  }, 0x180000 },    // 0C78 1019 4000  cmpi.w  #$1019,$4000
	{ "ddtodh",   { 0x11e78b50,0x6429d20e }, 0x180000 },	// 0C78 1019 4000  cmpi.w  #$1019,$4000
	{ "ecofghtr", { 0xd2782128,0x4c8f96f4 }, 0x200000 },	// 0838 0003 7345  btst    #3,$7345
	{ "ecofghtu", { 0x97382d30,0x6dac12fa }, 0x200000 },	// 0838 0003 7345  btst    #3,$7345
	{ "uecology", { 0xc23c0d2f,0x4cadb6ea }, 0x200000 },	// 0838 0003 7345  btst    #3,$7345
	{ "ecofghta", { 0xd2200d2d,0x4c01f76c }, 0x200000 },	// 0838 0003 7345  btst    #3,$7345
	{ "ssf2t",    { 0x5910806e,0xd220add9 }, 0x400000 },	// 0838 0007 2000  btst    #7,$2000
	{ "ssf2ta",   { 0x3460406e,0x502ca919 }, 0x400000 },	// 0838 0007 2000  btst    #7,$2000
	{ "ssf2tu",   { 0x5d90c86f,0xd030eb68 }, 0x400000 },	// 0838 0007 2000  btst    #7,$2000
	{ "ssf2tur1", { 0x5d90c86f,0xd030eb68 }, 0x400000 },	// 0838 0007 2000  btst    #7,$2000
	{ "ssf2xj",   { 0x38b09823,0xd304e0c0 }, 0x400000 },	// 0838 0007 2000  btst    #7,$2000
	{ "xmcota",   { 0xe6017c4a,0xcddc1dd1 }, 0x100000 },	// 0C80 1972 0301  cmpi.l  #$19720301,D0
	{ "xmcotau",  { 0xbfac7311,0x6ada1605 }, 0x100000 },	// 0C80 1972 0301  cmpi.l  #$19720301,D0
//  { "xmcotah",  {     /* MISSING */     }, 0x100000 },    // 0C80 1972 0301  cmpi.l  #$19720301,D0
	{ "xmcotaj",  { 0xbb76009f,0xb38512d6 }, 0x100000 },	// 0C80 1972 0301  cmpi.l  #$19720301,D0
	{ "xmcotaj1", { 0xbb76009f,0xb38512d6 }, 0x100000 },	// 0C80 1972 0301  cmpi.l  #$19720301,D0
	{ "xmcotajr", { 0xbb76009f,0xb38512d6 }, 0x100000 },	// 0C80 1972 0301  cmpi.l  #$19720301,D0
	{ "xmcotaa",  { 0xdb187e0c,0x1d247e33 }, 0x100000 },	// 0C80 1972 0301  cmpi.l  #$19720301,D0
	{ "armwar",   { 0xed15db39,0x50143231 }, 0x100000 },	// 3039 0080 4020  move.w  $00804020,D0
	{ "armwarr1", { 0xed15db39,0x50143231 }, 0x100000 },	// 3039 0080 4020  move.w  $00804020,D0
	{ "armwaru",  { 0x0f2644f0,0x781abc8e }, 0x100000 },	// 3039 0080 4020  move.w  $00804020,D0
	{ "pgear",    { 0x1857a169,0x7f14a641 }, 0x100000 },	// 3039 0080 4020  move.w  $00804020,D0
	{ "pgearr1",  { 0x1857a169,0x7f14a641 }, 0x100000 },	// 3039 0080 4020  move.w  $00804020,D0
//  { "armwara",  {     /* MISSING */     }, 0x100000 },    // 3039 0080 4020  move.w  $00804020,D0
	{ "avsp",     { 0x548e151d,0x56374bdc }, 0x100000 },	// 0C80 1234 5678  cmpi.l  #$12345678,D0
	{ "avspu",    { 0x45a46977,0xd860d3ad }, 0x100000 },	// 0C80 1234 5678  cmpi.l  #$12345678,D0
	{ "avspj",    { 0x4339cde4,0x0d7b6c29 }, 0x100000 },	// 0C80 1234 5678  cmpi.l  #$12345678,D0
	{ "avspa",    { 0x22e485fe,0x2e8d8b5c }, 0x100000 },	// 0C80 1234 5678  cmpi.l  #$12345678,D0
	{ "avsph",    { 0x3f94a389,0xc57c61d2 }, 0x100000 },	// 0C80 1234 5678  cmpi.l  #$12345678,D0
	{ "dstlk",    { 0x8202d548,0x4e068862 }, 0x100000 },	// 0838 0000 6160  btst    #0,$6160
	{ "dstlku",   { 0xae094c0c,0x7a138c44 }, 0x100000 },	// 0838 0000 6160  btst    #0,$6160
	{ "dstlkur1", { 0xae094c0c,0x7a138c44 }, 0x100000 },	// 0838 0000 6160  btst    #0,$6160
	{ "dstlka",   { 0x04048b4e,0x2a498879 }, 0x100000 },	// 0838 0000 6160  btst    #0,$6160
	{ "vampj",    { 0x8817cae1,0xb4d48118 }, 0x100000 },	// 0838 0000 6160  btst    #0,$6160
	{ "vampja",   { 0x8817cae1,0xb4d48118 }, 0x100000 },	// 0838 0000 6160  btst    #0,$6160
	{ "vampjr1",  { 0x8817cae1,0xb4d48118 }, 0x100000 },	// 0838 0000 6160  btst    #0,$6160
	{ "ringdest", { 0x04055413,0x67806575 }, 0x180000 },	// 3039 0080 4020  move.w  $00804020,D0
	{ "smbomb",   { 0x15014117,0x47008431 }, 0x180000 },	// 3039 0080 4020  move.w  $00804020,D0
	{ "smbombr1", { 0x15014117,0x47008431 }, 0x180000 },	// 3039 0080 4020  move.w  $00804020,D0
	{ "cybots",   { 0x252c00d2,0xb5142040 }, 0x100000 },	// 0C38 00FF 0C38  cmpi.b  #$FF,$0C38
	{ "cybotsu",  { 0xcd002398,0x05184145 }, 0x100000 },	// 0C38 00FF 0C38  cmpi.b  #$FF,$0C38
	{ "cybotsj",  { 0x04048215,0x34388354 }, 0x100000 },	// 0C38 00FF 0C38  cmpi.b  #$FF,$0C38
	{ "msh",      { 0xf755160b,0x63b0f8b4 }, 0x100000 },	// 0C81 1966 0419  cmpi.l  #$19660419,D1
	{ "mshu",     { 0xcf0a032d,0x36a2ea11 }, 0x100000 },	// 0C81 1966 0419  cmpi.l  #$19660419,D1
	{ "mshj",     { 0x01c0c951,0x370f4c80 }, 0x100000 },	// 0C81 1966 0419  cmpi.l  #$19660419,D1
	{ "mshjr1",   { 0x01c0c951,0x370f4c80 }, 0x100000 },	// 0C81 1966 0419  cmpi.l  #$19660419,D1
	{ "msha",     { 0x7fd080dc,0xb6104eee }, 0x100000 },	// 0C81 1966 0419  cmpi.l  #$19660419,D1
	{ "mshh",     { 0x606b80ea,0x73555487 }, 0x100000 },	// 0C81 1966 0419  cmpi.l  #$19660419,D1
	{ "mshb",     { 0x83571a87,0x41cbf22f }, 0x100000 },	// 0C81 1966 0419  cmpi.l  #$19660419,D1
	{ "nwarr",    { 0x6b74b14f,0xc1968654 }, 0x180000 },	// 0838 0000 6160  btst    #0,$6160
	{ "nwarrh",   { 0x406040b0,0x921242fd }, 0x180000 },    // 0838 0000 6160  btst    #0,$6160
	{ "nwarrb",   { 0xe2646956,0xd5025459 }, 0x180000 },    // 0838 0000 6160  btst    #0,$6160
	{ "vhuntj",   { 0x13ea2a1b,0x6e0cb9af }, 0x180000 },	// 0838 0000 6160  btst    #0,$6160
	{ "vhuntjr1", { 0x13ea2a1b,0x6e0cb9af }, 0x180000 },	// 0838 0000 6160  btst    #0,$6160
	{ "vhuntjr2", { 0x13ea2a1b,0x6e0cb9af }, 0x180000 },	// 0838 0000 6160  btst    #0,$6160
	{ "sfa",      { 0xad2dff0f,0x14b2f040 }, 0x080000 },	// 0C80 0564 2194  cmpi.l  #$05642194,D0
	{ "sfar1",    { 0xad2dff0f,0x14b2f040 }, 0x080000 },	// 0C80 0564 2194  cmpi.l  #$05642194,D0
	{ "sfar2",    { 0xad2dff0f,0x14b2f040 }, 0x080000 },	// 0C80 0564 2194  cmpi.l  #$05642194,D0
	{ "sfar3",    { 0xad2dff0f,0x14b2f040 }, 0x080000 },	// 0C80 0564 2194  cmpi.l  #$05642194,D0
	{ "sfau",     { 0x0b80d40e,0x95d57df5 }, 0x080000 },	// 0C80 0564 2194  cmpi.l  #$05642194,D0
//  { "sfza",     {  /*not enough data*/  }, 0x080000 },    // 0C80 0564 2194  cmpi.l  #$05642194,D0
	{ "sfzj",     { 0x54bd7724,0xb62d70b0 }, 0x080000 },	// 0C80 0564 2194  cmpi.l  #$05642194,D0
	{ "sfzjr1",   { 0x54bd7724,0xb62d70b0 }, 0x080000 },	// 0C80 0564 2194  cmpi.l  #$05642194,D0
	{ "sfzjr2",   { 0x54bd7724,0xb62d70b0 }, 0x080000 },	// 0C80 0564 2194  cmpi.l  #$05642194,D0
//  { "sfzh",     {  /*not enough data*/  }, 0x080000 },    // 0C80 0564 2194  cmpi.l  #$05642194,D0
//  { "sfzb",     {     /* MISSING */     }, 0x080000 },    // 0C80 0564 2194  cmpi.l  #$05642194,D0
//  { "sfzbr1",   {  /*not enough data*/  }, 0x080000 },    // 0C80 0564 2194  cmpi.l  #$05642194,D0
//  { "mmancp2u", {     /* MISSING */     },          },    // 0C80 0564 2194  cmpi.l  #$05642194,D0 (found in CPS1 version)
//  { "rmancp2j", {     /* MISSING */     },          },    // 0C80 0564 2194  cmpi.l  #$05642194,D0 (found in CPS1 version)
	{ "19xx",     { 0xc0230312,0x91b3a797 }, 0x200000 },	// 0C81 0095 1101  cmpi.l  #$00951101,D1
	{ "19xxa",    { 0x74c976fe,0xb9979591 }, 0x200000 },	// 0C81 0095 1101  cmpi.l  #$00951101,D1
	{ "19xxj",    { 0x293c170d,0x081fc06e }, 0x200000 },	// 0C81 0095 1101  cmpi.l  #$00951101,D1
	{ "19xxjr1",  { 0x293c170d,0x081fc06e }, 0x200000 },	// 0C81 0095 1101  cmpi.l  #$00951101,D1
	{ "19xxh",    { 0x1f7f8fcb,0x5f333c14 }, 0x200000 },	// 0C81 0095 1101  cmpi.l  #$00951101,D1
	{ "ddsom",    { 0xd524e525,0x3ebdde0c }, 0x100000 },	// 0C81 1966 0419  cmpi.l  #$19660419,D1
	{ "ddsomr1",  { 0xd524e525,0x3ebdde0c }, 0x100000 },	// 0C81 1966 0419  cmpi.l  #$19660419,D1
	{ "ddsomr2",  { 0xd524e525,0x3ebdde0c }, 0x100000 },	// 0C81 1966 0419  cmpi.l  #$19660419,D1
	{ "ddsomr3",  { 0xd524e525,0x3ebdde0c }, 0x100000 },	// 0C81 1966 0419  cmpi.l  #$19660419,D1
	{ "ddsomu",   { 0x1905f689,0x818ee7f9 }, 0x100000 },	// 0C81 1966 0419  cmpi.l  #$19660419,D1
	{ "ddsomur1", { 0x1905f689,0x818ee7f9 }, 0x100000 },	// 0C81 1966 0419  cmpi.l  #$19660419,D1
	{ "ddsomj",   { 0xe6714024,0x9bdd8d22 }, 0x100000 },	// 0C81 1966 0419  cmpi.l  #$19660419,D1
	{ "ddsomjr1", { 0xe6714024,0x9bdd8d22 }, 0x100000 },	// 0C81 1966 0419  cmpi.l  #$19660419,D1
	{ "ddsoma",   { 0x8658a336,0x3fb81a72 }, 0x100000 },	// 0C81 1966 0419  cmpi.l  #$19660419,D1
//  { "ddsomb",   {  /*not enough data*/  }, 0x100000 },    // 0C81 1966 0419  cmpi.l  #$19660419,D1
	{ "megaman2", { 0x50601dca,0x69ba5224 }, 0x100000 },	// 0C80 0164 7101  cmpi.l  #$01647101,D0
//  { "megamn2a", {  /*not enough data*/  }, 0x100000 },    // 0C80 0164 7101  cmpi.l  #$01647101,D0
	{ "rckman2j", { 0x6108e418,0xc659ed29 }, 0x100000 },	// 0C80 0164 7101  cmpi.l  #$01647101,D0
	{ "qndream",  { 0x3b4b2c9e,0x62353d11 }, 0x080000 },	// 0C81 1973 0827  cmpi.l  #$19730827,D1
	{ "sfa2",     { 0xcaa1da01,0xec9b73f5 }, 0x100000 },	// 0C80 3039 9783  cmpi.l  #$30399783,D0
	{ "sfz2j",    { 0xe7a0597c,0x2e335731 }, 0x100000 },	// 0C80 3039 9783  cmpi.l  #$30399783,D0
	{ "sfz2a",    { 0xf61b4c61,0x9df58912 }, 0x100000 },	// 0C80 3039 9783  cmpi.l  #$30399783,D0
//  { "sfz2b",    {     /* MISSING */     }, 0x100000 },    // 0C80 3039 9783  cmpi.l  #$30399783,D0
//  { "sfz2br1",  {     /* MISSING */     }, 0x100000 },    // 0C80 3039 9783  cmpi.l  #$30399783,D0
//  { "sfz2h",    {     /* MISSING */     }, 0x100000 },    // 0C80 3039 9783  cmpi.l  #$30399783,D0
//  { "sfz2n",    {     /* MISSING */     }, 0x100000 },    // 0C80 3039 9783  cmpi.l  #$30399783,D0
	{ "sfz2aj",   { 0x1c551364,0x4c1e1001 }, 0x100000 },	// 0C80 8E73 9110  cmpi.l  #$8E739110,D0
//  { "sfz2ah",   {     /* MISSING */     }, 0x100000 },    // 0C80 8E73 9110  cmpi.l  #$8E739110,D0
//  { "sfz2ab",   {     /* MISSING */     }, 0x100000 },    // 0C80 8E73 9110  cmpi.l  #$8E739110,D0
	{ "sfz2aa",   { 0x209a00e7,0xec434828 }, 0x100000 },	// 0C80 8E73 9110  cmpi.l  #$8E739110,D0
//  { "spf2t",    {  /*not enough data*/  }, 0x040000 },    // 0C80 3039 9819  cmpi.l  #$30399819,D0
//  { "spf2xj",   {  /*not enough data*/  }, 0x040000 },    // 0C80 3039 9819  cmpi.l  #$30399819,D0
//  { "spf2ta",   {  /*not enough data*/  }, 0x040000 },    // 0C80 3039 9819  cmpi.l  #$30399819,D0
	{ "xmvsf",    { 0x5847db77,0xd54b0edb }, 0x100000 },	// 0C81 1972 0327  cmpi.l  #$19720327,D1
	{ "xmvsfr1",  { 0x5847db77,0xd54b0edb }, 0x100000 },	// 0C81 1972 0327  cmpi.l  #$19720327,D1
	{ "xmvsfu",   { 0x854feac0,0xbe2bf740 }, 0x100000 },	// 0C81 1972 0327  cmpi.l  #$19720327,D1
	{ "xmvsfur1", { 0x854feac0,0xbe2bf740 }, 0x100000 },	// 0C81 1972 0327  cmpi.l  #$19720327,D1
//  { "xmvsfj",   {  /* search failed */  }, 0x100000 },    // 0C81 1972 0327  cmpi.l  #$19720327,D1
//  { "xmvsfjr1", {      /* TODO */       }, 0x100000 },    // 0C81 1972 0327  cmpi.l  #$19720327,D1
//  { "xmvsfjr2", {     /* MISSING */     }, 0x100000 },    // 0C81 1972 0327  cmpi.l  #$19720327,D1
//  { "xmvsfa",   {      /* TODO */       }, 0x100000 },    // 0C81 1972 0327  cmpi.l  #$19720327,D1
//  { "xmvsfar1", {     /* MISSING */     }, 0x100000 },    // 0C81 1972 0327  cmpi.l  #$19720327,D1
//  { "xmvsfh",   {      /* TODO */       }, 0x100000 },    // 0C81 1972 0327  cmpi.l  #$19720327,D1
//  { "xmvsfb",   {      /* TODO */       }, 0x100000 },    // 0C81 1972 0327  cmpi.l  #$19720327,D1
	{ "batcir",   { 0x66527aba,0x4c876eef }, 0x200000 },	// 0C81 0097 0131  cmpi.l  #$00970131,D1
//  { "batcira",  {  /* search failed */  }, 0x200000 },    // 0C81 0097 0131  cmpi.l  #$00970131,D1
	{ "batcirj",  { 0x288ed340,0x88190069 }, 0x200000 },	// 0C81 0097 0131  cmpi.l  #$00970131,D1
	{ "csclub",   { 0xd4a2b480,0xba7a0857 }, 0x200000 },	// 0C81 0097 0310  cmpi.l  #$00970310,D1
	{ "cscluba",  { 0xe4b41d49,0xe6127e99 }, 0x200000 },	// 0C81 0097 0310  cmpi.l  #$00970310,D1
	{ "csclubj",  { 0x808ba692,0x2ab885d1 }, 0x200000 },	// 0C81 0097 0310  cmpi.l  #$00970310,D1
	{ "csclubh",  { 0x53420cbe,0x48d8eb33 }, 0x200000 },	// 0C81 0097 0310  cmpi.l  #$00970310,D1
	{ "mshvsf",   { 0xdf1a540f,0x66125f89 }, 0x100000 },	// 0C81 1972 1027  cmpi.l  #$19721027,D1
	{ "mshvsfu",  { 0xe98cae72,0x2471d3d9 }, 0x100000 },	// 0C81 1972 1027  cmpi.l  #$19721027,D1
	{ "mshvsfu1", { 0xe98cae72,0x2471d3d9 }, 0x100000 },	// 0C81 1972 1027  cmpi.l  #$19721027,D1
	{ "mshvsfj",  { 0x017b4fce,0xdc03585e }, 0x100000 },	// 0C81 1972 1027  cmpi.l  #$19721027,D1
	{ "mshvsfj1", { 0x017b4fce,0xdc03585e }, 0x100000 },	// 0C81 1972 1027  cmpi.l  #$19721027,D1
	{ "mshvsfj2", { 0x017b4fce,0xdc03585e }, 0x100000 },	// 0C81 1972 1027  cmpi.l  #$19721027,D1
	{ "mshvsfh",  { 0xbf195a84,0x7bd400fa }, 0x100000 },	// 0C81 1972 1027  cmpi.l  #$19721027,D1
//  { "mshvsfa",  {  /* search failed */  }, 0x100000 },    // 0C81 1972 1027  cmpi.l  #$19721027,D1
//  { "mshvsfa1", {     /* MISSING */     }, 0x100000 },    // 0C81 1972 1027  cmpi.l  #$19721027,D1
	{ "mshvsfb",  { 0xe589b9f4,0x43220fd6 }, 0x100000 },	// 0C81 1972 1027  cmpi.l  #$19721027,D1
	{ "mshvsfb1", { 0xe589b9f4,0x43220fd6 }, 0x100000 },	// 0C81 1972 1027  cmpi.l  #$19721027,D1
	{ "sgemf",    { 0x299c262f,0xb0bfe65a }, 0x080000 },	// 0C80 1F74 0D12  cmpi.l  #$1F740D12,D0
	{ "pfghtj",   { 0xb75a406d,0xde1ccce2 }, 0x080000 },	// 0C80 1F74 0D12  cmpi.l  #$1F740D12,D0
	{ "sgemfa",   { 0x64078f0f,0xa4267146 }, 0x080000 },	// 0C80 1F74 0D12  cmpi.l  #$1F740D12,D0
//  { "sgemfh",   {  /* search failed */  }, 0x080000 },    // 0C80 1F74 0D12  cmpi.l  #$1F740D12,D0
	{ "vhunt2",   { 0xfb446e5a,0x7a5028d0 }, 0x100000 },	// 0C80 0692 0760  cmpi.l  #$06920760,D0
	{ "vhunt2r1", { 0xfb446e5a,0x7a5028d0 }, 0x100000 },	// 0C80 0692 0760  cmpi.l  #$06920760,D0
	{ "vsav",     { 0x6064eafc,0x297e848b }, 0x100000 },	// 0C80 726A 4BAF  cmpi.l  #$726A4BAF,D0
	{ "vsavu",    { 0xc29aad27,0xb9ec6c19 }, 0x100000 },	// 0C80 726A 4BAF  cmpi.l  #$726A4BAF,D0
	{ "vsavj",    { 0xec43deba,0xe255308b }, 0x100000 },	// 0C80 726A 4BAF  cmpi.l  #$726A4BAF,D0
	{ "vsava",    { 0xc8b2ecc0,0x9411dcc9 }, 0x100000 },	// 0C80 726A 4BAF  cmpi.l  #$726A4BAF,D0
	{ "vsavh",    { 0xef367b7c,0xc9fa54b2 }, 0x100000 },	// 0C80 726A 4BAF  cmpi.l  #$726A4BAF,D0
	{ "vsav2",    { 0xff5e7ead,0x58a9c800 }, 0x100000 },	// 0C80 0692 0760  cmpi.l  #$06920760,D0
	{ "mvsc",     { 0x253d2983,0xaa97a70c }, 0x100000 },	// 0C81 1972 0121  cmpi.l  #$19720121,D1
	{ "mvscu",    { 0x65d59b92,0x04612f97 }, 0x100000 },	// 0C81 1972 0121  cmpi.l  #$19720121,D1
	{ "mvscj",    { 0xf207676e,0x15439458 }, 0x100000 },	// 0C81 1972 0121  cmpi.l  #$19720121,D1
	{ "mvscjr1",  { 0xf207676e,0x15439458 }, 0x100000 },	// 0C81 1972 0121  cmpi.l  #$19720121,D1
	{ "mvsca",    { 0x975c90e9,0x6bf06e06 }, 0x100000 },	// 0C81 1972 0121  cmpi.l  #$19720121,D1
	{ "mvscar1",  { 0x975c90e9,0x6bf06e06 }, 0x100000 },	// 0C81 1972 0121  cmpi.l  #$19720121,D1
	{ "mvsch",    { 0x2f778c77,0x573cb129 }, 0x100000 },	// 0C81 1972 0121  cmpi.l  #$19720121,D1
	{ "mvscb",    { 0x38ab355c,0x2b22ada3 }, 0x100000 },	// 0C81 1972 0121  cmpi.l  #$19720121,D1
	{ "sfa3",     { 0xeae8e6b2,0x9deac8b0 }, 0x100000 },	// 0C80 1C62 F5A8  cmpi.l  #$1C62F5A8,D0
	{ "sfa3r1",   { 0xeae8e6b2,0x9deac8b0 }, 0x100000 },	// 0C80 1C62 F5A8  cmpi.l  #$1C62F5A8,D0
	{ "sfa3b",    { 0x609226ad,0x5901f806 }, 0x100000 },	// 0C80 1C62 F5A8  cmpi.l  #$1C62F5A8,D0
	{ "sfz3j",    { 0x7e258ade,0x745e6a88 }, 0x100000 },	// 0C80 1C62 F5A8  cmpi.l  #$1C62F5A8,D0
	{ "sfz3jr1",  { 0x7e258ade,0x745e6a88 }, 0x100000 },	// 0C80 1C62 F5A8  cmpi.l  #$1C62F5A8,D0
	{ "sfz3jr2",  { 0x7e258ade,0x745e6a88 }, 0x100000 },	// 0C80 1C62 F5A8  cmpi.l  #$1C62F5A8,D0
	{ "sfz3a",    { 0x18758a3b,0xc60a59c8 }, 0x100000 },	// 0C80 1C62 F5A8  cmpi.l  #$1C62F5A8,D0
	{ "sfz3ar1",  { 0x18758a3b,0xc60a59c8 }, 0x100000 },	// 0C80 1C62 F5A8  cmpi.l  #$1C62F5A8,D0
	{ "jyangoku", { 0x5e8d6c8c,0x3adaf591 },  /*?*/   },	// 0C80 3652 1573  cmpi.l  #$36521573,D0
	{ "hsf2",     { 0x0ff5fe6c,0x42411399 }, 0x100000 },	// 0838 0007 2000  btst    #7,$2000
//  { "hsf2j",    {     /* MISSING */     }, 0x100000 },    // 0838 0007 2000  btst    #7,$2000
	{ "gigawing", { 0x3282aaa0,0x3f418f17 }, 0x100000 },	// 0C81 1972 1027  cmpi.l  #$19721027,D1
	{ "gwingj",   { 0x45fe0d0a,0xc41077d1 }, 0x100000 },	// 0C81 1972 1027  cmpi.l  #$19721027,D1
//  { "gwinga",   {  /* search failed */  }, 0x100000 },    // 0C81 1972 1027  cmpi.l  #$19721027,D1
	{ "mmatrix",  { 0x1e3bf539,0xb17b4865 }, 0x180000 },	// B6C0 B447 BACF  cmpa.w  D0,A3   cmp.w   D7,D2   cmpa.w  A7,A5
	{ "mmatrixj", { 0x05e5d8d1,0x1eb5c5ba }, 0x180000 },	// B6C0 B447 BACF  cmpa.w  D0,A3   cmp.w   D7,D2   cmpa.w  A7,A5
	{ "mpang",    { 0x2dc8626f,0xdda27163 }, 0x100000 },	// 0C84 347D 89A3  cmpi.l  #$347D89A3,D4
	{ "mpangj",   { 0x2dc8626f,0xdda27163 }, 0x100000 },	// 0C84 347D 89A3  cmpi.l  #$347D89A3,D4
	{ "pzloop2",  { 0x3332206a,0x0077f829 }, 0x400000 },	// 0C82 9A73 15F1  cmpi.l  #$9A7315F1,D2
	{ "pzloop2j", { 0x3332206a,0x0077f829 }, 0x400000 },	// 0C82 9A73 15F1  cmpi.l  #$9A7315F1,D2
	{ "choko",    { 0x94fae2ee,0xefa4e6b8 }, 0x400000 },	// 0C86 4D17 5B3C  cmpi.l  #$4D175B3C,D6
	{ "dimahoo",  { 0x56ca3bdc,0x16575de3 }, 0x080000 },	// BE4C B244 B6C5  cmp.w   A4,D7   cmp.w   D4,D1   cmpa.w  D5,A3
	{ "gmahou",   { 0x83ec7b61,0xf73b082f }, 0x080000 },	// BE4C B244 B6C5  cmp.w   A4,D7   cmp.w   D4,D1   cmpa.w  D5,A3
	{ "1944",     { 0x7bad8109,0xd78e933f }, 0x080000 },	// 0C86 7B5D 94F1  cmpi.l  #$7B5D94F1,D6
//  { "1944j",    {  /*not enough data*/  }, 0x080000 },    // 0C86 7B5D 94F1  cmpi.l  #$7B5D94F1,D6
	{ "progear",  { 0xf56ec08f,0xadf83a38 }, 0x400000 },	// 0C81 63A1 B8D3  cmpi.l  #$63A1B8D3,D1
	{ "progearj", { 0xf9fbb86d,0xf5952fad }, 0x400000 },	// 0C81 63A1 B8D3  cmpi.l  #$63A1B8D3,D1
//  { "progeara", {  /* search failed */  }, 0x400000 },    // 0C81 63A1 B8D3  cmpi.l  #$63A1B8D3,D1

	{ 0 }	// end of table
};




DRIVER_INIT( cps2 )
{
	const char *gamename = machine->gamedrv->name;
	const struct game_keys *k = &keys_table[0];

	while (k->name)
	{
		if (strcmp(k->name, gamename) == 0)
		{
			break;
		}
		++k;
	}

	if (k->name)
	{
		// we have a proper key so use it to decrypt
		cps2_decrypt(k->keys, k->upper_limit ? k->upper_limit : 0x400000);
	}
	else
	{
		// we don't have a proper key so use the XOR tables if available

		UINT16 *rom = (UINT16 *)memory_region(REGION_CPU1);
		UINT16 *xor = (UINT16 *)memory_region(REGION_USER1);
		int length = memory_region_length(REGION_CPU1);
		int i;

#if 0
{
FILE *f;

f = fopen("d:/s.rom","wb");
fwrite(rom,1,0x100000,f);
fclose(f);
}
#endif
		if (xor)
		{
			for (i = 0; i < length/2; i++)
				xor[i] ^= rom[i];

			memory_set_decrypted_region(0, 0x000000, length - 1, xor);
			m68k_set_encrypted_opcode_range(0,0,length);
		}
	}

	init_cps2_video(machine);
}
