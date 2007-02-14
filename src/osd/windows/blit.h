//============================================================
//
//  blit.h - Win32 blit handling
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

#ifndef __WIN_BLIT__
#define __WIN_BLIT__


//============================================================
//  PARAMETERS
//============================================================

// maximum X/Y scale values
#define MAX_X_MULTIPLY		3
#define MAX_Y_MULTIPLY		4

// standard effects
#define EFFECT_NONE			0
#define EFFECT_SCANLINE_25	1
#define EFFECT_SCANLINE_50	2
#define EFFECT_SCANLINE_75	3

// custom effects
#define EFFECT_CUSTOM		10
#define EFFECT_RGB16		10
#define EFFECT_RGB6			11
#define EFFECT_RGB4			12
#define EFFECT_RGB4V		13
#define EFFECT_RGB3			14
#define EFFECT_RGB_TINY		15
#define EFFECT_SCANLINE_75V	16
#define EFFECT_SHARP		17



//============================================================
//  TYPE DEFINITIONS
//============================================================

typedef struct _win_blit_params win_blit_params;

struct _win_blit_params
{
	void *		dstdata;
	int			dstpitch;
	int			dstdepth;
	int			dstxoffs;
	int			dstyoffs;
	int			dstxscale;
	int			dstyscale;
	int			dstyskip;
	int			dsteffect;

	void *		srcdata;
	int			srcpitch;
	int			srcdepth;
	UINT32 *	srclookup;
	int			srcxoffs;
	int			srcyoffs;
	int			srcwidth;
	int			srcheight;

	void *		vecdirty;

	int			flipx;
	int			flipy;
	int			swapxy;
};



//============================================================
//  PROTOTYPES
//============================================================

int win_perform_blit(const win_blit_params *blit, int update);

#endif
