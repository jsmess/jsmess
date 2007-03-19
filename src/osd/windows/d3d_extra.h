
// versions of the standard vertex structures with an extra set of texture coordinates

typedef struct _D3DTLVERTEX2 {
	union { D3DVALUE sx; D3DVALUE dvSX; };
	union { D3DVALUE sy; D3DVALUE dvSY; };
	union { D3DVALUE sz; D3DVALUE dvSZ; };
	union { D3DVALUE rhw; D3DVALUE dvRHW; };
	union { D3DCOLOR color; D3DCOLOR dcColor; };
	union { D3DCOLOR specular; D3DCOLOR dcSpecular; };
	union { D3DVALUE tu; D3DVALUE dvTU; };
	union { D3DVALUE tv; D3DVALUE dvTV; };
	union { D3DVALUE tu1; D3DVALUE dvTU1; };
	union { D3DVALUE tv1; D3DVALUE dvTV1; };
#if(DIRECT3D_VERSION >= 0x0500)
 #if (defined __cplusplus) && (defined D3D_OVERLOADS)
	_D3DTLVERTEX2() { }
	_D3DTLVERTEX2(const D3DVECTOR& v, float _rhw, D3DCOLOR _color, D3DCOLOR _specular, float _tu, float _tv, float _tu1, float _tv1)
	{
		sx = v.x; sy = v.y; sz = v.z; rhw = _rhw;
		color = _color; specular = _specular;
		tu = _tu; tv = _tv;
		tu1 = _tu1; tv1 = _tv1;
	}
 #endif
#endif
} D3DTLVERTEX2, *LPD3DTLVERTEX2;

#define D3DFVF_TLVERTEX2 ( D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2 | D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEXCOORDSIZE2(1) )

typedef struct _D3DLVERTEX2 {
	union { D3DVALUE x; D3DVALUE dvX; };
	union { D3DVALUE y; D3DVALUE dvY; };
	union { D3DVALUE z; D3DVALUE dvZ; };
    DWORD dwReserved;
	union { D3DCOLOR color; D3DCOLOR dcColor; };
	union { D3DCOLOR specular; D3DCOLOR dcSpecular; };
	union { D3DVALUE tu; D3DVALUE dvTU; };
	union { D3DVALUE tv; D3DVALUE dvTV; };
	union { D3DVALUE tu1; D3DVALUE dvTU1; };
	union { D3DVALUE tv1; D3DVALUE dvTV1; };
#if(DIRECT3D_VERSION >= 0x0500)
 #if (defined __cplusplus) && (defined D3D_OVERLOADS)
	_D3DLVERTEX2() { }
	_D3DLVERTEX2(const D3DVECTOR& v, D3DCOLOR _color, D3DCOLOR _specular, float _tu, float _tv, float _tu1, float _tv1)
	{
		x = v.x; y = v.y; z = v.z; dwReserved = 0;
		color = _color; specular = _specular;
		tu = _tu; tv = _tv;
		tu1 = _tu1; tv1 = _tv1;
	}
 #endif
#endif
} D3DLVERTEX2, *LPD3DLVERTEX2;

#define D3DFVF_LVERTEX2 ( D3DFVF_XYZ | D3DFVF_RESERVED1 | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2 | D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEXCOORDSIZE2(1) )

typedef struct _D3DVERTEX2 {
	union { D3DVALUE x; D3DVALUE dvX; };
	union { D3DVALUE y; D3DVALUE dvY; };
	union { D3DVALUE z; D3DVALUE dvZ; };
    union { D3DVALUE nx; D3DVALUE dvNX; };
    union { D3DVALUE ny; D3DVALUE dvNY; };
    union { D3DVALUE nz; D3DVALUE dvNZ; };
	union { D3DVALUE tu; D3DVALUE dvTU; };
	union { D3DVALUE tv; D3DVALUE dvTV; };
	union { D3DVALUE tu1; D3DVALUE dvTU1; };
	union { D3DVALUE tv1; D3DVALUE dvTV1; };
#if(DIRECT3D_VERSION >= 0x0500)
 #if (defined __cplusplus) && (defined D3D_OVERLOADS)
    _D3DVERTEX2() { }
    _D3DVERTEX2(const D3DVECTOR& v, const D3DVECTOR& n, float _tu, float _tv, float _tu1, float _tv1)
        {
			x = v.x; y = v.y; z = v.z;
			nx = n.x; ny = n.y; nz = n.z;
			tu = _tu; tv = _tv;
			tu1 = _tu1; tv1 = _tv1;
        }
 #endif
#endif
} D3DVERTEX2, *LPD3DVERTEX2;

#define D3DFVF_VERTEX2 ( D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX2 | D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEXCOORDSIZE2(1) )

