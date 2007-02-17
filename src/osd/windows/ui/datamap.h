
#ifndef DATAMAP_H
#define DATAMAP_H

enum 
{
	// these allowed for dmType and encoded_type
	DM_NONE = 0,
	DM_BOOL,
	DM_INT,
	// these are only allowed for encoded_type
	DM_DOUBLE,
	DM_STRING
};

enum
{
	CT_NONE = 0,
	CT_BUTTON,
	CT_COMBOBOX,
	CT_SLIDER
};

typedef struct
{
	DWORD       dwCtrlId;
	int dmType;
	int cType;
	BOOL        bValue;
	BOOL*       bVar;
	int         nValue;
	int         nMin;
	int         nMax;
	int*        nVar;
	int encoded_type;
	void *encoded_var;
	void        (*func)(HWND hWnd);
} DATA_MAP;

void InitDataMap(void);
void DataMapAdd(DWORD dwCtrlId, int dmType, int cType,
				void *var, int encoded_type,void *encoded_var,
				int min, int max, void (*func)(HWND hWnd));
void PopulateControls(HWND hWnd);
void ReadControls(HWND hWnd);
BOOL ReadControl(HWND hWnd, DWORD dwCtrlId);

int GetControlID(HWND hDlg,HWND hwnd);
int GetControlType(HWND hDlg,HWND hwnd);
BOOL IsControlDifferent(HWND hDlg,HWND hwnd_ctrl,options_type *o,options_type *base);

#endif /* DATAMAP_H */
