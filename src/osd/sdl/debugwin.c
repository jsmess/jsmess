//============================================================
//
//  debugwin.c - SDL/GTK+ debug window handling
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "debug-intf.h"
#include "debug-sup.h"
#include "debug-cb.h"

#include "dview.h"

typedef struct hentry {
	struct hentry *h;
	char *e;
} hentry;

typedef struct {
	GtkEntry *edit_w;
	struct hentry *h, *ch;
	char *hold;
	int keep_last;
	void (*cb)(const char *, void *);
	void *cbp;
} edit;

typedef struct {
	GtkWidget *win;
	DView *console_w, *disasm_w, *registers_w;
	edit ed;
	debug_view *console;
	debug_view *disasm;
	debug_view *registers;
	int cpu;
} debugmain_i;

typedef struct memorywin_i {
	struct memorywin_i *next;
	GtkWidget *win;
	DView *memory_w;
	edit ed;
	GtkComboBox *zone_w;
	debug_view *memory;
} memorywin_i;

typedef struct disasmwin_i {
	struct disasmwin_i *next;
	GtkWidget *win;
	DView *disasm_w;
	edit ed;
	GtkComboBox *cpu_w;
	debug_view *disasm;
} disasmwin_i;

typedef struct logwin_i {
	struct logwin_i *next;
	GtkWidget *win;
	DView *log_w;
	debug_view *log;
} logwin_i;

typedef struct memorycombo_item
{
	struct memorycombo_item *next;
	char					name[256];
	UINT8					cpunum;
	UINT8					spacenum;
	void *					base;
	UINT32					length;
	UINT8					offset_xor;
	UINT8					little_endian;
	UINT8					prefsize;
} memorycombo_item;


static debugmain_i *dmain;
static memorywin_i *memorywin_list;
static disasmwin_i *disasmwin_list;
static logwin_i    *logwin_list;

static memorycombo_item *memorycombo;

static void debugmain_init(void);

static void debugwin_show(int show)
{
	memorywin_i *p1;
	disasmwin_i *p2;
	logwin_i *p3;
	void (*f)(GtkWidget *widget) = show ? gtk_widget_show : gtk_widget_hide;
	if(dmain) {
		f(dmain->win);
		//		dview_set_updatable(dmain->console_w, show);
	}
	for(p1 = memorywin_list; p1; p1 = p1->next)
		f(p1->win);
	for(p2 = disasmwin_list; p2; p2 = p2->next)
		f(p2->win);
	for(p3 = logwin_list; p3; p3 = p3->next)
		f(p3->win);
}

//============================================================
//  memory_determine_combo_items
//============================================================

static void memory_determine_combo_items(void)
{
	memorycombo_item **tail = &memorycombo;
	UINT32 cpunum, spacenum;
	int rgnnum, itemnum;

	// first add all the CPUs' address spaces
	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
	{
		const debug_cpu_info *cpuinfo = debug_get_cpu_info(cpunum);
		if (cpuinfo->valid)
			for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
				if (cpuinfo->space[spacenum].databytes)
				{
					memorycombo_item *ci = malloc_or_die(sizeof(*ci));
					memset(ci, 0, sizeof(*ci));
					ci->cpunum = cpunum;
					ci->spacenum = spacenum;
					ci->prefsize = MIN(cpuinfo->space[spacenum].databytes, 4);
					sprintf(ci->name, "CPU #%d (%s) %s memory", cpunum, cpunum_name(cpunum), address_space_names[spacenum]);
					*tail = ci;
					tail = &ci->next;
				}
	}

	// then add all the memory regions
	for (rgnnum = 0; rgnnum < MAX_MEMORY_REGIONS; rgnnum++)
	{
		UINT8 *base = memory_region(rgnnum);
		UINT32 type = memory_region_type(Machine, rgnnum);
		if (base != NULL && type > REGION_INVALID && (type - REGION_INVALID) < ARRAY_LENGTH(memory_region_names))
		{
			memorycombo_item *ci = malloc_or_die(sizeof(*ci));
			UINT32 flags = memory_region_flags(Machine, rgnnum);
			UINT8 width, little_endian;
			memset(ci, 0, sizeof(*ci));
			ci->base = base;
			ci->length = memory_region_length(rgnnum);
			width = 1 << (flags & ROMREGION_WIDTHMASK);
			little_endian = ((flags & ROMREGION_ENDIANMASK) == ROMREGION_LE);
			if (type >= REGION_CPU1 && type <= REGION_CPU8)
			{
				const debug_cpu_info *cpuinfo = debug_get_cpu_info(type - REGION_CPU1);
				if (cpuinfo)
				{
					width = cpuinfo->space[ADDRESS_SPACE_PROGRAM].databytes;
					little_endian = (cpuinfo->endianness == CPU_IS_LE);
				}
			}
			ci->prefsize = MIN(width, 4);
			ci->offset_xor = width - 1;
			ci->little_endian = little_endian;
			strcpy(ci->name, memory_region_names[type - REGION_INVALID]);
			*tail = ci;
			tail = &ci->next;
		}
	}

	// finally add all global array symbols
	for (itemnum = 0; itemnum < 10000; itemnum++)
	{
		UINT32 valsize, valcount;
		const char *name;
		void *base;

		/* stop when we run out of items */
		name = state_save_get_indexed_item(itemnum, &base, &valsize, &valcount);
		if (name == NULL)
			break;

		/* if this is a single-entry global, add it */
		if (valcount > 1 && strstr(name, "/globals/"))
		{
			memorycombo_item *ci = malloc_or_die(sizeof(*ci));
			memset(ci, 0, sizeof(*ci));
			ci->base = base;
			ci->length = valcount * valsize;
			ci->prefsize = MIN(valsize, 4);
			ci->little_endian = TRUE;
			strcpy(ci->name, strrchr(name, '/') + 1);
			*tail = ci;
			tail = &ci->next;
		}
	}
}


static void edit_add_hist(edit *e, const char *text)
{
	if(e->ch)
		e->ch = 0;

	if(e->hold) {
		free(e->hold);
		e->hold =0;
	}

	if(!e->h || (text[0] && strcmp(text, e->h->e))) {
		hentry *h = malloc(sizeof(hentry));
		h->h = e->h;
		h->e = mame_strdup(text);
		e->h = h;
	}
}

static void edit_set_field(edit *e)
{
	if(e->keep_last) {
		gtk_entry_set_text(e->edit_w, e->h->e);
		gtk_editable_select_region(GTK_EDITABLE(e->edit_w), 0, -1);
	} else
		gtk_entry_set_text(e->edit_w, "");
}

static void edit_activate(GtkEntry *item, gpointer user_data)
{
	edit *e = user_data;
	const char *text = gtk_entry_get_text(e->edit_w);
	edit_add_hist(e, text);
	e->cb(text, e->cbp);
	edit_set_field(e);
	
}

static void edit_hist_back(edit *e)
{
	const char *text = gtk_entry_get_text(e->edit_w);
	if(e->ch) {
		if(!e->ch->h)
			return;
		e->ch = e->ch->h;
	} else {
		if(!e->h)
			return;
		if(!strcmp(text, e->h->e)) {
			if(!e->h->h)
				return;
			e->ch = e->h->h;
		} else {
			if(text[0])
				e->hold = mame_strdup(text);
			e->ch = e->h;
		}
	}
	gtk_entry_set_text(e->edit_w, e->ch->e);
	gtk_editable_select_region(GTK_EDITABLE(e->edit_w), 0, -1);
}

static void edit_hist_forward(edit *e)
{
	hentry *h, *hp;
	if(!e->ch)
		return;
	h = e->h;
	hp = 0;
	while(h != e->ch) {
		hp = h;
		h = h->h;
	}

	if(hp) {
		e->ch = hp;
		gtk_entry_set_text(e->edit_w, hp->e);
	} else if(e->hold) {
		gtk_entry_set_text(e->edit_w, e->hold);
		free(e->hold);
		e->hold = 0;
		e->ch = 0;
	} else {
		gtk_entry_set_text(e->edit_w, "");
		e->ch = 0;
	}
	gtk_editable_select_region(GTK_EDITABLE(e->edit_w), 0, -1);
}

static gboolean edit_key(GtkWidget *item, GdkEventKey *k, gpointer user_data)
{
	switch(k->keyval) {
	case GDK_Up:
		edit_hist_back((edit *)user_data);
		return TRUE;
	case GDK_Down:
		edit_hist_forward((edit *)user_data);
		return TRUE;
	}
	return FALSE;
}

static void edit_init(edit *e, GtkWidget *w, const char *ft, int kl, void (*cb)(const char *, void *), void *cbp)
{
	e->edit_w = GTK_ENTRY(w);
	e->h = 0;
	e->ch = 0;
	e->hold = 0;
	e->keep_last = kl;
	e->cb = cb;
	e->cbp = cbp;
	if(ft)
		edit_add_hist(e, ft);
	edit_set_field(e);
	g_signal_connect(e->edit_w, "activate", G_CALLBACK(edit_activate), e);
	g_signal_connect(e->edit_w, "key-press-event", G_CALLBACK(edit_key), e);
}

static void edit_del(edit *e)
{
	hentry *h = e->h;
	while(h) {
		hentry *he = h->h;
		free(h->e);
		free(h);
		h = he;
	}
}

static void debugmain_update_checks(debugmain_i *info)
{
	int rc = debug_view_get_property_UINT32(info->disasm, DVP_DASM_RIGHT_COLUMN);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(lookup_widget(info->win, "raw_opcodes")), rc == DVP_DASM_RIGHTCOL_RAW);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(lookup_widget(info->win, "enc_opcodes")), rc == DVP_DASM_RIGHTCOL_ENCRYPTED);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(lookup_widget(info->win, "comments")), rc == DVP_DASM_RIGHTCOL_COMMENTS);
}

void debugmain_raw_opcodes_activate(GtkMenuItem *item, gpointer user_data)
{
	debugmain_i *info = user_data;
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(lookup_widget(info->win, "raw_opcodes")))) {
		debug_view_begin_update(info->disasm);
		debug_view_set_property_UINT32(info->disasm, DVP_DASM_RIGHT_COLUMN, DVP_DASM_RIGHTCOL_RAW);
		debug_view_end_update(info->disasm);
	}
}

void debugmain_enc_opcodes_activate(GtkMenuItem *item, gpointer user_data)
{
	debugmain_i *info = user_data;
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(lookup_widget(info->win, "enc_opcodes")))) {
		debug_view_begin_update(info->disasm);
		debug_view_set_property_UINT32(info->disasm, DVP_DASM_RIGHT_COLUMN, DVP_DASM_RIGHTCOL_ENCRYPTED);
		debug_view_end_update(info->disasm);
	}
}

void debugmain_comments_activate(GtkMenuItem *item, gpointer user_data)
{
	debugmain_i *info = user_data;
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(lookup_widget(info->win, "comments")))) {
		debug_view_begin_update(info->disasm);
		debug_view_set_property_UINT32(info->disasm, DVP_DASM_RIGHT_COLUMN, DVP_DASM_RIGHTCOL_COMMENTS);
		debug_view_end_update(info->disasm);
	}
}

static void debugmain_set_cpunum(int cpunum)
{
	if(cpunum != dmain->cpu) {
		char title[256];
		dmain->cpu = cpunum;
		
		// first set all the views to the new cpu number
		debug_view_set_property_UINT32(dmain->disasm, DVP_DASM_CPUNUM, cpunum);
		debug_view_set_property_UINT32(dmain->registers, DVP_REGS_CPUNUM, cpunum);

		// then update the caption
		sprintf(title, "Debug: %s - CPU %d (%s)", Machine->gamedrv->name, cpunum, cpunum_name(cpunum));
		gtk_window_set_title(GTK_WINDOW(dmain->win), title);
		debugmain_update_checks(dmain);
	}
}


// The entry point

void osd_wait_for_debugger(void)
{
	// create a console window
	if(!dmain) {
		// GTK init should probably be done earlier
		gtk_init(0, 0);
		debugmain_init();
	}

	// update the views in the console to reflect the current CPU
	debugmain_set_cpunum(cpu_getactivecpu());

	debugwin_show(1);
	gtk_main_iteration();
}

void debugwin_update_during_game(void)
{
	if(dmain)
		gtk_main_iteration_do(FALSE);
}

static void debugmain_process_string(const char *str, void *dmp)
{
	if(!str[0])
		debug_cpu_single_step(1);
	else
		debug_console_execute_command(str, 1);
}

static void debugmain_destroy(GtkObject *obj, gpointer user_data)
{
	mame_schedule_exit(Machine);
}

static void debugmain_init(void)
{
	dmain = malloc(sizeof(*dmain));
	memset(dmain, 0, sizeof(*dmain));
	dmain->win = create_debugmain();
	dmain->cpu = -1;

	dmain->console_w   = DVIEW(lookup_widget(dmain->win, "console"));
	dmain->disasm_w    = DVIEW(lookup_widget(dmain->win, "disasm"));
	dmain->registers_w = DVIEW(lookup_widget(dmain->win, "registers"));

	dmain->console   = debug_view_alloc(DVT_CONSOLE);
	dmain->disasm    = debug_view_alloc(DVT_DISASSEMBLY);
	dmain->registers = debug_view_alloc(DVT_REGISTERS);

	dview_set_debug_view(dmain->console_w,   dmain->console);
	dview_set_debug_view(dmain->disasm_w,    dmain->disasm);
	dview_set_debug_view(dmain->registers_w, dmain->registers);

	dview_this_one_is_stupidly_autoscrolling(dmain->console_w);

	edit_init(&dmain->ed, lookup_widget(dmain->win, "edit"), 0, 0, debugmain_process_string, &dmain);

	debug_view_begin_update(dmain->disasm);
	debug_view_set_property_string(dmain->disasm, DVP_DASM_EXPRESSION, "pc");
	debug_view_set_property_UINT32(dmain->disasm, DVP_DASM_TRACK_LIVE, 1);
	debug_view_end_update(dmain->disasm);

	g_signal_connect(dmain->win, "destroy", G_CALLBACK(debugmain_destroy), 0);
	g_signal_connect(lookup_widget(dmain->win, "raw_opcodes"), "activate", G_CALLBACK(debugmain_raw_opcodes_activate), dmain);
	g_signal_connect(lookup_widget(dmain->win, "enc_opcodes"), "activate", G_CALLBACK(debugmain_enc_opcodes_activate), dmain);
	g_signal_connect(lookup_widget(dmain->win, "comments"),    "activate", G_CALLBACK(debugmain_comments_activate), dmain);

	gtk_widget_show_all(dmain->win);
}

static void memorywin_update_checks(memorywin_i *info)
{
	int bpc = debug_view_get_property_UINT32(info->memory, DVP_MEM_BYTES_PER_CHUNK);
	int rev = debug_view_get_property_UINT32(info->memory, DVP_MEM_REVERSE_VIEW);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(lookup_widget(info->win, "chunks_1")), bpc == 1);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(lookup_widget(info->win, "chunks_2")), bpc == 2);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(lookup_widget(info->win, "chunks_4")), bpc == 4);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(lookup_widget(info->win, "reverse")), rev);
}

static void memorywin_update_selection(memorywin_i *info, memorycombo_item *ci)
{
	debug_view_set_property_UINT32(info->memory, DVP_MEM_CPUNUM, ci->cpunum);
	debug_view_set_property_UINT32(info->memory, DVP_MEM_SPACENUM, ci->spacenum);
	debug_view_set_property_ptr(info->memory, DVP_MEM_RAW_BASE, ci->base);
	debug_view_set_property_UINT32(info->memory, DVP_MEM_RAW_LENGTH, ci->length);
	debug_view_set_property_UINT32(info->memory, DVP_MEM_RAW_OFFSET_XOR, ci->offset_xor);
	debug_view_set_property_UINT32(info->memory, DVP_MEM_RAW_LITTLE_ENDIAN, ci->little_endian);
	debug_view_set_property_UINT32(info->memory, DVP_MEM_BYTES_PER_CHUNK, ci->prefsize);
	memorywin_update_checks(info);
	gtk_window_set_title(GTK_WINDOW(info->win), ci->name);
}

static void memorywin_zone_changed(GtkComboBox *zone_w, memorywin_i *mem)
{
	int sel = gtk_combo_box_get_active(mem->zone_w);
	int i;
	memorycombo_item *ci;
	for(i=0, ci=memorycombo; i != sel; i++)
		ci = ci->next;
	memorywin_update_selection(mem, ci);
}

static void memorywin_process_string(const char *str, void *memp)
{
	memorywin_i *mem = memp;
	debug_view_set_property_string(mem->memory, DVP_MEM_EXPRESSION, str);
}

static void memorywin_destroy(GtkObject *obj, gpointer user_data)
{
	memorywin_i *mem = user_data;
	memorywin_i **p = &memorywin_list;
	while(*p != mem)
		p = &(*p)->next;
	if(mem->next)
		*p = mem->next;
	else
		*p = 0;
	edit_del(&mem->ed);
	free(mem);
}

static void memorywin_new(void)
{
	memorywin_i *mem;
	int item, cursel, curcpu = cpu_getactivecpu();
	memorycombo_item *ci, *selci = NULL;

	mem = malloc(sizeof(*mem));
	memset(mem, 0, sizeof(*mem));
	mem->next = memorywin_list;
	memorywin_list = mem;
	mem->win = create_memorywin();

	mem->memory_w = DVIEW(lookup_widget(mem->win, "memoryview"));
	mem->zone_w   = GTK_COMBO_BOX(lookup_widget(mem->win, "zone"));

	mem->memory = debug_view_alloc(DVT_MEMORY);

	dview_set_debug_view(mem->memory_w, mem->memory);

	edit_init(&mem->ed, lookup_widget(mem->win, "edit"), "0", 1, memorywin_process_string, mem);

	debug_view_begin_update(mem->memory);
	debug_view_set_property_string(mem->memory, DVP_MEM_EXPRESSION, "0");
	debug_view_set_property_UINT32(mem->memory, DVP_MEM_TRACK_LIVE, 1);
	debug_view_end_update(mem->memory);


	// populate the combobox
	if (!memorycombo)
		memory_determine_combo_items();
	item = 0;
	cursel = 0;
	for (ci = memorycombo; ci; ci = ci->next)
	{
		gtk_combo_box_append_text(mem->zone_w, ci->name);
		if (ci->base == NULL && ci->cpunum == curcpu && ci->spacenum == ADDRESS_SPACE_PROGRAM)
		{
			cursel = item;
			selci = ci;
		}
		item++;

	}
	gtk_combo_box_set_active(mem->zone_w, cursel);
	memorywin_update_selection(mem, selci);

	g_signal_connect(mem->zone_w, "changed", G_CALLBACK(memorywin_zone_changed), mem);
	g_signal_connect(lookup_widget(mem->win, "chunks_1"), "activate", G_CALLBACK(on_chunks_1_activate), mem);
	g_signal_connect(lookup_widget(mem->win, "chunks_2"), "activate", G_CALLBACK(on_chunks_2_activate), mem);
	g_signal_connect(lookup_widget(mem->win, "chunks_4"), "activate", G_CALLBACK(on_chunks_4_activate), mem);
	g_signal_connect(lookup_widget(mem->win, "reverse"),  "activate", G_CALLBACK(on_reverse_activate),  mem);
	g_signal_connect(lookup_widget(mem->win, "ibpl"),     "activate", G_CALLBACK(on_ibpl_activate),     mem);
	g_signal_connect(lookup_widget(mem->win, "dbpl"),     "activate", G_CALLBACK(on_dbpl_activate),     mem);

	g_signal_connect(mem->win, "destroy", G_CALLBACK(memorywin_destroy), mem);
	gtk_widget_show_all(mem->win);
}

static void disasmwin_update_checks(disasmwin_i *info)
{
	int rc = debug_view_get_property_UINT32(info->disasm, DVP_DASM_RIGHT_COLUMN);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(lookup_widget(info->win, "raw_opcodes")), rc == DVP_DASM_RIGHTCOL_RAW);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(lookup_widget(info->win, "enc_opcodes")), rc == DVP_DASM_RIGHTCOL_ENCRYPTED);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(lookup_widget(info->win, "comments")), rc == DVP_DASM_RIGHTCOL_COMMENTS);
}

static void disasmwin_update_selection(disasmwin_i *info, int cpunum)
{
	char title[4096];
	debug_view_begin_update(info->disasm);
	debug_view_set_property_UINT32(info->disasm, DVP_DASM_CPUNUM, (cpunum == -1) ? 0 : cpunum);
	debug_view_end_update(info->disasm);

	disasmwin_update_checks(info);

	sprintf(title, "Disassembly: %s (%d)", cpunum_name(cpunum), cpunum);
	gtk_window_set_title(GTK_WINDOW(info->win), title);
}

static void disasmwin_cpu_changed(GtkComboBox *cpu_w, disasmwin_i *dis)
{
	int sel = gtk_combo_box_get_active(dis->cpu_w);
	int cpunum, item;
	item = 0;
	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
	{
		const debug_cpu_info *cpuinfo = debug_get_cpu_info(cpunum);
		if (cpuinfo->valid)
			if (cpuinfo->space[ADDRESS_SPACE_PROGRAM].databytes)
			{
				if(item == sel) {
					disasmwin_update_selection(dis, cpunum);
					return;
				}
				item++;
			}
	}
}

void disasmwin_raw_opcodes_activate(GtkMenuItem *item, gpointer user_data)
{
	disasmwin_i *info = user_data;
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(lookup_widget(info->win, "raw_opcodes")))) {
		debug_view_begin_update(info->disasm);
		debug_view_set_property_UINT32(info->disasm, DVP_DASM_RIGHT_COLUMN, DVP_DASM_RIGHTCOL_RAW);
		debug_view_end_update(info->disasm);
	}
}

void disasmwin_enc_opcodes_activate(GtkMenuItem *item, gpointer user_data)
{
	disasmwin_i *info = user_data;
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(lookup_widget(info->win, "enc_opcodes")))) {
		debug_view_begin_update(info->disasm);
		debug_view_set_property_UINT32(info->disasm, DVP_DASM_RIGHT_COLUMN, DVP_DASM_RIGHTCOL_ENCRYPTED);
		debug_view_end_update(info->disasm);
	}
}

void disasmwin_comments_activate(GtkMenuItem *item, gpointer user_data)
{
	disasmwin_i *info = user_data;
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(lookup_widget(info->win, "comments")))) {
		debug_view_begin_update(info->disasm);
		debug_view_set_property_UINT32(info->disasm, DVP_DASM_RIGHT_COLUMN, DVP_DASM_RIGHTCOL_COMMENTS);
		debug_view_end_update(info->disasm);
	}
}

static void disasmwin_process_string(const char *str, void *disp)
{
	disasmwin_i *dis = disp;
	debug_view_set_property_string(dis->disasm, DVP_DASM_EXPRESSION, str);
}

static void disasmwin_destroy(GtkObject *obj, gpointer user_data)
{
	disasmwin_i *dis = user_data;
	disasmwin_i **p = &disasmwin_list;
	while(*p != dis)
		p = &(*p)->next;
	if(dis->next)
		*p = dis->next;
	else
		*p = 0;
	edit_del(&dis->ed);
	free(dis);
}

static void disasmwin_new(void)
{
	disasmwin_i *dis;
	int cpunum, curcpu = cpu_getactivecpu();
	int item, citem;

	dis = malloc(sizeof(*dis));
	memset(dis, 0, sizeof(*dis));
	dis->next = disasmwin_list;
	disasmwin_list = dis;
	dis->win = create_disasmwin();

	dis->disasm_w = DVIEW(lookup_widget(dis->win, "disasmview"));
	dis->cpu_w    = GTK_COMBO_BOX(lookup_widget(dis->win, "cpu"));

	dis->disasm = debug_view_alloc(DVT_DISASSEMBLY);

	dview_set_debug_view(dis->disasm_w, dis->disasm);

	edit_init(&dis->ed, lookup_widget(dis->win, "edit"), "pc", 1, disasmwin_process_string, dis);

	debug_view_begin_update(dis->disasm);
	debug_view_set_property_string(dis->disasm, DVP_DASM_EXPRESSION, "pc");
	debug_view_set_property_UINT32(dis->disasm, DVP_DASM_TRACK_LIVE, 1);
	debug_view_end_update(dis->disasm);

	// populate the combobox
	item = 0;
	citem = -1;
	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
	{
		const debug_cpu_info *cpuinfo = debug_get_cpu_info(cpunum);
		if (cpuinfo->valid)
			if (cpuinfo->space[ADDRESS_SPACE_PROGRAM].databytes)
			{
				char name[100];
				sprintf(name, "CPU #%d (%s)", cpunum, cpunum_name(cpunum));
				gtk_combo_box_append_text(dis->cpu_w, name);
				if(cpunum == curcpu)
					citem = item;
				item++;
			}
	}

	if(citem == -1) {
		citem = 0;
		curcpu = 0;
	}

	gtk_combo_box_set_active(dis->cpu_w, citem);
	disasmwin_update_selection(dis, curcpu);

	g_signal_connect(dis->cpu_w, "changed", G_CALLBACK(disasmwin_cpu_changed), dis);
	g_signal_connect(lookup_widget(dis->win, "raw_opcodes"), "activate", G_CALLBACK(disasmwin_raw_opcodes_activate), dis);
	g_signal_connect(lookup_widget(dis->win, "enc_opcodes"), "activate", G_CALLBACK(disasmwin_enc_opcodes_activate), dis);
	g_signal_connect(lookup_widget(dis->win, "comments"),    "activate", G_CALLBACK(disasmwin_comments_activate), dis);

	//	g_signal_connect(dis->edit_w, "activate", G_CALLBACK(disasmwin_process_string), dis);
	g_signal_connect(dis->win, "destroy", G_CALLBACK(disasmwin_destroy), dis);
	gtk_widget_show_all(dis->win);
}

static void logwin_destroy(GtkObject *obj, gpointer user_data)
{
	logwin_i *log = user_data;
	logwin_i **p = &logwin_list;
	while(*p != log)
		p = &(*p)->next;
	if(log->next)
		*p = log->next;
	else
		*p = 0;
	free(log);
}

static void logwin_new(void)
{
	logwin_i *log;

	log = malloc(sizeof(*log));
	memset(log, 0, sizeof(*log));
	log->next = logwin_list;
	logwin_list = log;
	log->win = create_logwin();

	log->log_w = DVIEW(lookup_widget(log->win, "logview"));

	log->log = debug_view_alloc(DVT_LOG);

	dview_this_one_is_stupidly_autoscrolling(log->log_w);
	dview_set_debug_view(log->log_w, log->log);
	g_signal_connect(log->win, "destroy", G_CALLBACK(logwin_destroy), log);

	gtk_widget_show_all(log->win);
}

void on_new_mem_activate(GtkMenuItem *item, gpointer user_data)
{
	memorywin_new();
}

void on_new_disasm_activate(GtkMenuItem *item, gpointer user_data)
{
	disasmwin_new();
}

void on_new_errorlog_activate(GtkMenuItem *item, gpointer user_data)
{
	logwin_new();
}

void on_run_activate(GtkMenuItem *item, gpointer user_data)
{
	debug_cpu_go(~0);
}

void on_run_h_activate(GtkMenuItem *item, gpointer user_data)
{
	debugwin_show(0);
	debug_cpu_go(~0);
}

void on_run_cpu_activate(GtkMenuItem *item, gpointer user_data)
{
	debug_cpu_next_cpu();
}

void on_run_irq_activate(GtkMenuItem *item, gpointer user_data)
{
	debug_cpu_go_interrupt(-1);
}

void on_run_vbl_activate(GtkMenuItem *item, gpointer user_data)
{
	debug_cpu_go_vblank();
}

void on_step_into_activate(GtkMenuItem *item, gpointer user_data)
{
	debug_cpu_single_step(1);
}

void on_step_over_activate(GtkMenuItem *item, gpointer user_data)
{
	debug_cpu_single_step_over(1);
}

void on_step_out_activate(GtkMenuItem *item, gpointer user_data)
{
	debug_cpu_single_step_out();
}

void on_hard_reset_activate(GtkMenuItem *item, gpointer user_data)
{
	mame_schedule_hard_reset(Machine);
}

void on_soft_reset_activate(GtkMenuItem *item, gpointer user_data)
{
	mame_schedule_soft_reset(Machine);
	debug_cpu_go(~0);
}

void on_exit_activate(GtkMenuItem *item, gpointer user_data)
{
	mame_schedule_exit(Machine);
}

void on_chunks_1_activate(GtkMenuItem *item, gpointer user_data)
{
	memorywin_i *info = user_data;
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(lookup_widget(info->win, "chunks_1")))) {
		debug_view_begin_update(info->memory);
		debug_view_set_property_UINT32(info->memory, DVP_MEM_BYTES_PER_CHUNK, 1);
		debug_view_end_update(info->memory);
	}
}

void on_chunks_2_activate(GtkMenuItem *item, gpointer user_data)
{
	memorywin_i *info = user_data;
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(lookup_widget(info->win, "chunks_2")))) {
		debug_view_begin_update(info->memory);
		debug_view_set_property_UINT32(info->memory, DVP_MEM_BYTES_PER_CHUNK, 2);
		debug_view_end_update(info->memory);
	}
}

void on_chunks_4_activate(GtkMenuItem *item, gpointer user_data)
{
	memorywin_i *info = user_data;
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(lookup_widget(info->win, "chunks_4")))) {
		debug_view_begin_update(info->memory);
		debug_view_set_property_UINT32(info->memory, DVP_MEM_BYTES_PER_CHUNK, 4);
		debug_view_end_update(info->memory);
	}
}

void on_reverse_activate(GtkMenuItem *item, gpointer user_data)
{
	memorywin_i *info = user_data;
	debug_view_begin_update(info->memory);
	debug_view_set_property_UINT32(info->memory, DVP_MEM_REVERSE_VIEW,
								   gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(lookup_widget(info->win, "reverse"))));
	debug_view_end_update(info->memory);
}

void on_ibpl_activate(GtkMenuItem *item, gpointer user_data)
{
	memorywin_i *info = user_data;
	debug_view_begin_update(info->memory);
	debug_view_set_property_UINT32(info->memory, DVP_MEM_WIDTH,
								   debug_view_get_property_UINT32(info->memory, DVP_MEM_WIDTH) + 1);
	debug_view_end_update(info->memory);
}

void on_dbpl_activate(GtkMenuItem *item, gpointer user_data)
{
	memorywin_i *info = user_data;
	debug_view_begin_update(info->memory);
	debug_view_set_property_UINT32(info->memory, DVP_MEM_WIDTH,
								   debug_view_get_property_UINT32(info->memory, DVP_MEM_WIDTH) - 1);
	debug_view_end_update(info->memory);
}
