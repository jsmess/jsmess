/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#define ptr_offset(ptr, offset)	((void *) (((const char *) ptr) + offset))

// config helpers types
typedef enum
{
	RO_BOOL = 0, // BOOL value
	RO_INT,      // int value
	RO_DOUBLE,   // double value
	RO_COLOR,    // COLORREF value
	RO_STRING,   // pointer to string    - m_vpData is an allocated buffer
	RO_ENCODE    // encode/decode string - calls decode/encode functions
} rotype_t;



// used to be "registry option", now is just for a game/global option
typedef struct
{
	const char *ini_name;                         // ini name
	rotype_t m_iType;                             // key type
	size_t m_iDataOffset;                         // offset to data within struct
	const char *m_pDefaultValue;                  // default value on startup
	BOOL (*m_pfnQualifier)(int driver_index);     // used to identify when this option is relevant
	void (*encode)(void *data, char *str);        // encode function
	void (*decode)(const char *str, void *data);  // decode function
} REG_OPTION;



const REG_OPTION *GetOption(const REG_OPTION *option_array, const char *key);
void LoadOption(void *option_struct, const REG_OPTION *option, const char *value_str);
void LoadDefaultOptions(void *option_struct, const REG_OPTION *option_array);
BOOL CopyOptionStruct(void *dest, const void *source, const REG_OPTION *option_array, size_t struct_size);
void FreeOptionStruct(void *option_struct, const REG_OPTION *option_array);

BOOL IsOptionEqual(const REG_OPTION *opt, const void *o1, const void *o2);
BOOL AreOptionsEqual(const REG_OPTION *option_array, const void *o1, const void *o2);

// --------------------------------------------------------------------------

typedef enum
{
	SH_END,
	SH_OPTIONSTRUCT,
	SH_MANUAL
} settings_handler_type_t;

struct SettingsHandler
{
	settings_handler_type_t type;
	const char *comment;

	union
	{
		struct
		{
			void *option_struct;
			const REG_OPTION *option_array;
			const void *comparison_struct;
			int qualifier_param;
		} option_struct;

		struct
		{
			BOOL (*parse)(DWORD nSettingsFile, char *key, const char *value_str);
			void (*emit)(DWORD nSettingsFile, void (*emit_callback)(void *param_, const char *key, const char *value_str, const char *comment), void *param);
		} manual;
	} u;
};

#define SETTINGS_FILE_GAME				0x10000000
#define SETTINGS_FILE_FOLDER			0x20000000
#define SETTINGS_FILE_EXFOLDER			0x30000000
#define SETTINGS_FILE_SOURCEFILE		0x40000000
#define SETTINGS_FILE_EXFOLDERPARENT	0x50000000
#define SETTINGS_FILE_UI				0xE0000000
#define SETTINGS_FILE_GLOBAL			0xF0000000
#define SETTINGS_FILE_TYPEMASK			0xF0000000

BOOL LoadSettingsFile(DWORD nSettingsFile, void *option_struct, const REG_OPTION *option_array);
BOOL LoadSettingsFileEx(DWORD nSettingsFile, const struct SettingsHandler *handlers);
BOOL SaveSettingsFileEx(DWORD nSettingsFile, const struct SettingsHandler *handlers);
BOOL SaveSettingsFile(DWORD nSettingsFile, const void *option_struct, const void *comparison_struct, const REG_OPTION *option_array);

