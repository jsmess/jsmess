/*********************************************************************

    gromport.h
    See grompoprt.c for documentation

    Michael Zapf

    February 2012: Rewritten as class

*********************************************************************/

#ifndef __TI99CART_H__
#define __TI99CART_H__

#include "emu.h"
#include "ti99defs.h"
#include "grom.h"
#include "image.h"

extern const device_type GROMPORT;
extern const device_type TICARTRIDGE;
extern const device_type TICARTPCBSTD;
extern const device_type TICARTPCBPAG;
extern const device_type TICARTPCBMIN;
extern const device_type TICARTPCBSUP;
extern const device_type TICARTPCBMBX;
extern const device_type TICARTPCBPGI;
extern const device_type TICARTPCBPGC;
extern const device_type TICARTPCBGRK;

/* We set the number of slots to 4, although we may have up to 16. From a
   logical point of view we could have 256, but the operating system only checks
   the first 16 banks. */
#define NUMBER_OF_CARTRIDGE_SLOTS 4

#define GROMPORT_CONFIG(name) \
	const gromport_config(name) =

typedef struct _gromport_config
{
	devcb_write_line	ready;
} gromport_config;

class cartridge_pcb_device;
class rpk;

/*****************************************************************************
    The base class of a cartridge
    Cartridges are implemented as subdevices of the cartridge slot. They
    are always present as devices, irrespective of the fact that they are
    plugged in. Plugging in, however, means that there is a PCB and sizes
    for ROM or RAM.
*****************************************************************************/
class cartridge_device : public bus8z_device, public device_image_interface
{
	friend class gromport_device;
	friend class cartridge_pcb_device;

public:
	cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	DECLARE_READ8Z_MEMBER(readz);
	DECLARE_WRITE8_MEMBER(write);

	bool is_available() { return m_pcb != NULL; }
	bool has_grom();
	void set_switch(int swtch, int val);	// forwarding to GK

	virtual void crureadz(offs_t offset, UINT8 *value);
	virtual void cruwrite(offs_t offset, UINT8 data);

	// Image handling: implementation of methods which are abstract in the parent
	bool call_load();
	void call_unload();
	bool call_softlist_load(char *swlist, char *swname, rom_entry *start_entry);

	iodevice_t image_type() const { return IO_CARTSLOT; }
	bool is_readable()  const			{ return true; }
	bool is_writeable() const			{ return false; }
	bool is_creatable() const			{ return false; }
	bool must_be_loaded() const 		{ return false; }
	bool is_reset_on_load() const		{ return false; }
	const char *image_interface() const { return "ti99_cart"; }
	const char *file_extensions() const { return "rpk"; }
	const option_guide *create_option_guide() const { return NULL; }
	machine_config_constructor device_mconfig_additions() const;

protected:
	void device_start(void);
	void device_stop(void);
	void device_reset(void);
	void device_config_complete(void);
	const rom_entry *device_rom_region() const;

private:
	bool has_extension(const char* name);
	void prepare_cartridge();
	int get_index_from_tagname();
	UINT8* get_grom_ptr();
	void set_gk_guest(cartridge_device *guest);

	int m_type;

	// PCB device associated to this cartridge. If NULL, the slot is empty.
	cartridge_pcb_device *m_pcb;

	// RPK which is associated to this cartridge
	rpk	*m_rpk;

	// If we are using softlists
	bool m_softlist;
};

/*****************************************************************************
    The class of a cartridge PCB
*****************************************************************************/

class cartridge_pcb_device : public bus8z_device
{
	friend class cartridge_device;
	friend class gromport_device;

public:
	cartridge_pcb_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock);

	virtual DECLARE_READ8Z_MEMBER(readz) =0;
	virtual DECLARE_WRITE8_MEMBER(write) =0;

	/* Read function for this cartridge, CRU. */
	virtual void crureadz(offs_t offset, UINT8 *value);

	/* Write function for this cartridge, CRU. */
	virtual void cruwrite(offs_t offset, UINT8 data);

protected:
	void device_start(void);
	DECLARE_READ8Z_MEMBER( gromreadz );
	DECLARE_WRITE8_MEMBER( gromwrite );
	void set_gk_guest(int slot);

	/* Points to the (max 5) GROM devices. */
	ti99_grom_device *m_grom[5];

	// ROM buffer.
	UINT8 *m_rom_ptr;

	// ROM buffer for the second bank of paged cartridges.
	UINT8 *m_rom2_ptr;

	// RAM buffer. The persistence service is done by the cartridge system.
	// The RAM space is a consecutive space; all banks are in one buffer.
	UINT8 *m_ram_ptr;

	// ROM page.
	int m_rom_page;

	// RAM page.
	int m_ram_page;

	// GROM buffer size. Depends of the size of the resource loaded into "grom_socket".
	int m_grom_size;

	// ROM buffer size. All banks have equal sizes.
	int m_rom_size;

	// RAM buffer size. All banks have equal sizes.
	int m_ram_size;
};

class cartridge_pcb_standard_device : public cartridge_pcb_device
{
public:
	cartridge_pcb_standard_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	DECLARE_READ8Z_MEMBER(readz);
	DECLARE_WRITE8_MEMBER(write);
};

class cartridge_pcb_paged_device : public cartridge_pcb_device
{
public:
	cartridge_pcb_paged_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	DECLARE_READ8Z_MEMBER(readz);
	DECLARE_WRITE8_MEMBER(write);
};

class cartridge_pcb_minimem_device : public cartridge_pcb_device
{
public:
	cartridge_pcb_minimem_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	DECLARE_READ8Z_MEMBER(readz);
	DECLARE_WRITE8_MEMBER(write);
};

class cartridge_pcb_superspace_device : public cartridge_pcb_device
{
public:
	cartridge_pcb_superspace_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	DECLARE_READ8Z_MEMBER(readz);
	DECLARE_WRITE8_MEMBER(write);
	void crureadz(offs_t offset, UINT8 *value);
	void cruwrite(offs_t offset, UINT8 data);
};

class cartridge_pcb_mbx_device : public cartridge_pcb_device
{
public:
	cartridge_pcb_mbx_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	DECLARE_READ8Z_MEMBER(readz);
	DECLARE_WRITE8_MEMBER(write);
};

class cartridge_pcb_paged379i_device : public cartridge_pcb_device
{
public:
	cartridge_pcb_paged379i_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	DECLARE_READ8Z_MEMBER(readz);
	DECLARE_WRITE8_MEMBER(write);
private:
	int get_paged379i_bank(int rompage);
};

class cartridge_pcb_pagedcru_device : public cartridge_pcb_device
{
public:
	cartridge_pcb_pagedcru_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	DECLARE_READ8Z_MEMBER(readz);
	DECLARE_WRITE8_MEMBER(write);
	void crureadz(offs_t offset, UINT8 *value);
	void cruwrite(offs_t offset, UINT8 data);
};

class cartridge_pcb_gramkracker_device : public cartridge_pcb_device
{
	friend class cartridge_device;

public:
	cartridge_pcb_gramkracker_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	DECLARE_READ8Z_MEMBER(readz);
	DECLARE_WRITE8_MEMBER(write);
	void crureadz(offs_t offset, UINT8 *value);
	void cruwrite(offs_t offset, UINT8 data);

	void set_gk_guest(cartridge_device *guest);
	void set_switch(int swtch, int val);

private:
	// GROM emulation in GK
	bool	m_waddr_LSB;
	UINT16	m_grom_address;
	UINT8*	m_grom_ptr;

	int 	m_gk_switch[6];			// Used to cache the switch settings.

	// GRAMKracker guest slot
	cartridge_device *m_gk_guest;
};

/*****************************************************************************
    The overall cartridge system
*****************************************************************************/

class gromport_device : public bus8z_device
{
	friend class cartridge_device;

public:
	gromport_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	DECLARE_READ8Z_MEMBER(readz);
	DECLARE_WRITE8_MEMBER(write);
	DECLARE_WRITE_LINE_MEMBER(ready_line);

	void crureadz(offs_t offset, UINT8 *value);
	void cruwrite(offs_t offset, UINT8 value);

	DECLARE_INPUT_CHANGED_MEMBER( gk_changed );

protected:
	void device_start(void);
	void device_stop(void);
	void device_reset(void);
	machine_config_constructor device_mconfig_additions() const;
	ioport_constructor device_input_ports() const;

private:
	void set_slot(int slot);
	int get_active_slot(bool changebase, offs_t offset);
	void change_slot(bool inserted, int index);
	void set_gk_slot(int slot);
	void find_gk_guest();
	void crureadz_gk(offs_t offset, UINT8 *value);
	void cruwrite_gk(offs_t offset, UINT8 value);

	// Points to all cartridges. This is faster than iterating the list
	// of subdevices on each memory access.
	cartridge_device*	m_cartridge[NUMBER_OF_CARTRIDGE_SLOTS];

	// Number of available cartridge positions
	int 	m_numcart;

	// Determines which slot is currently active. This value is changed when
	// there are accesses to other GROM base addresses.
	int 	m_active_slot;

	// Used in order to enforce a special slot. This value is retrieved
	// from the dipswitch setting. A value of -1 means automatic, that is,
	// the grom base switch is used. Values 0 .. max refer to the
	// respective slot.
	int 	m_fixed_slot;

	// Holds the highest index of a cartridge being plugged in plus one.
	// If we only have one cartridge inserted, we don't want to get a
	// selection option, so we just mirror the memory contents.
	int 	m_next_free_slot;

	/* GK support. */
	int		m_gk_slot;

	/* Ready line */
	devcb_resolved_write_line m_ready;
};

typedef struct _pcb_type
{
	int id;
	const char* name;
} pcb_type;

/*************************************************************************
    RPK support
*************************************************************************/

class rpk_socket
{
	friend class simple_list<rpk_socket>;
	friend class rpk;

public:
	rpk_socket(const char *id, int length, void *contents);
	rpk_socket(const char *id, int length, void *contents, const char *pathname);

	const char*		id() { return m_id; }
	int 			get_content_length() { return m_length; }
	void*			get_contents() { return m_contents; }
	bool			persistent_ram() { return m_pathname != NULL; }
	const char*		get_pathname() { return m_pathname; }
	void			cleanup() { if (m_contents != NULL) free(m_contents); }

private:
	const char		*m_id;
	UINT32			m_length;
	rpk_socket		*m_next;
	void			*m_contents;
	const char		*m_pathname;
};

class rpk_reader
{
public:
	rpk_reader(const pcb_type *types)
	: m_types(types) { };

	rpk 				*open(emu_options &options, const char *filename, const char *system_name);

private:
	const zip_file_header*	find_file(zip_file *zip, const char *filename, UINT32 crc);
	rpk_socket*				load_rom_resource(zip_file* zip, xml_data_node* rom_resource_node, const char* socketname);
	rpk_socket* 			load_ram_resource(emu_options &options, xml_data_node* ram_resource_node, const char* socketname, const char* system_name);
	const pcb_type*			m_types;
};

class rpk
{
	friend class rpk_reader;
public:
	rpk(emu_options& options, const char* sysname);
	~rpk();

	int			get_type(void) { return m_type; }
	void*		get_contents_of_socket(const char *socket_name);
	int			get_resource_length(const char *socket_name);
	void		close();

private:
	emu_options&			m_options;		// need this to find the path to the nvram files
	int						m_type;
	const char*				m_system_name;	// need this to find the path to the nvram files
	tagged_list<rpk_socket>	m_sockets;

	void add_socket(const char* id, rpk_socket *newsock);
};

enum _rpk_open_error
{
	RPK_OK,
	RPK_NOT_ZIP_FORMAT,
	RPK_CORRUPT,
	RPK_OUT_OF_MEMORY,
	RPK_XML_ERROR,
	RPK_INVALID_FILE_REF,
	RPK_ZIP_ERROR,
	RPK_MISSING_RAM_LENGTH,
	RPK_INVALID_RAM_SPEC,
	RPK_UNKNOWN_RESOURCE_TYPE,
	RPK_INVALID_RESOURCE_REF,
	RPK_INVALID_LAYOUT,
	RPK_MISSING_LAYOUT,
	RPK_NO_PCB_OR_RESOURCES,
	RPK_UNKNOWN_PCB_TYPE
};
typedef enum _rpk_open_error rpk_open_error;

static const char error_text[15][30] =
{
	"No error",
	"Not a RPK (zip) file",
	"Module definition corrupt",
	"Out of memory",
	"XML format error",
	"Invalid file reference",
	"Zip file error",
	"Missing RAM length",
	"Invalid RAM specification",
	"Unknown resource type",
	"Invalid resource reference",
	"layout.xml not valid",
	"Missing layout",
	"No pcb or resource found",
	"Unknown pcb type"
};

class rpk_exception
{
public:
	rpk_exception(rpk_open_error value): m_err(value), m_detail(NULL) { };
	rpk_exception(rpk_open_error value, const char* detail): m_err(value), m_detail(detail) { };

	const char* to_string()
	{
		if (m_detail==NULL) return error_text[(int)m_err];
		astring errormsg(error_text[(int)m_err], ": ", m_detail);
		return core_strdup(errormsg.cstr());
	}

private:
	rpk_open_error m_err;
	const char* m_detail;
};

/*************************************************************************
    Macros
*************************************************************************/

#define MCFG_TI99_GROMPORT_ADD(_tag, _intf) \
	MCFG_DEVICE_ADD(_tag, GROMPORT, 0)	\
	MCFG_DEVICE_CONFIG(_intf)

#endif /* __TI99CART_H__ */
