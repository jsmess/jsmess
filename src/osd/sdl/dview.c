
#include "dview.h"
#include <gconf/gconf-client.h>

G_DEFINE_TYPE(DView, dview, GTK_TYPE_CONTAINER);

static gboolean dview_expose(GtkWidget *wdv, GdkEventExpose *event)
{
	debug_view_char *viewdata;
	DView *dv = DVIEW(wdv);
	DViewClass *dvc = DVIEW_GET_CLASS(dv);
	UINT32 trow = debug_view_get_property_UINT32(dv->dw, DVP_TOTAL_ROWS);
	UINT32 tcol = debug_view_get_property_UINT32(dv->dw, DVP_TOTAL_COLS);
	UINT32 prow = debug_view_get_property_UINT32(dv->dw, DVP_TOP_ROW);
	UINT32 pcol = debug_view_get_property_UINT32(dv->dw, DVP_LEFT_COL);
	UINT32 vrow = debug_view_get_property_UINT32(dv->dw, DVP_VISIBLE_ROWS);
	UINT32 vcol = debug_view_get_property_UINT32(dv->dw, DVP_VISIBLE_COLS);
	UINT32 i, j, xx, yy;
	GdkColor bg, fg;

	bg.red = bg.green = bg.blue = 0xffff;
	gdk_gc_set_rgb_fg_color(dv->gc, &bg);
	gdk_draw_rectangle(GDK_DRAWABLE(wdv->window), dv->gc, TRUE, 0, 0, wdv->allocation.width - (dv->vs ? dv->vsz : 0), wdv->allocation.height - (dv->hs ? dv->hsz : 0));

	if(dv->hs && dv->vs) {
		gdk_gc_set_foreground(dv->gc, &wdv->style->bg[GTK_STATE_NORMAL]);
		gdk_draw_rectangle(GDK_DRAWABLE(wdv->window), dv->gc, TRUE,
						   wdv->allocation.width - dv->vsz, wdv->allocation.height - dv->hsz,
						   dv->vsz, dv->hsz);
	}

	viewdata = debug_view_get_property_ptr(dv->dw, DVP_VIEW_DATA);

	yy = wdv->style->ythickness;
	for(j=0; j<vrow; j++) {
		xx = wdv->style->xthickness;
		for(i=0; i<vcol; i++) {
			unsigned char attr = viewdata->attrib;
			char s[3];
			unsigned char v = viewdata->byte;
			if(v < 128) {
				s[0] = v;
				s[1] = 0;
			} else {
				s[0] = 0xc0 | (v>>6);
				s[1] = 0x80 | (v & 0x3f);
				s[2] = 0;
			}

			bg.red = bg.green = bg.blue = 0xffff;
			fg.red = fg.green = fg.blue = 0;

			if(attr & DCA_ANCILLARY)
				bg.red = bg.green = bg.blue = 0xe0e0;
			if(attr & DCA_SELECTED) {
				bg.red = 0xffff;
				bg.green = bg.blue = 0x8080;
			}
			if(attr & DCA_CURRENT) {
				bg.red = bg.green = 0xffff;
				bg.blue = 0;
			}
			if(attr & DCA_CHANGED) {
				fg.red = 0xffff;
				fg.green = fg.blue = 0;
			}
			if(attr & DCA_INVALID) {
				fg.red = fg.green = 0;
				fg.blue = 0xffff;
			}
			if(attr & DCA_DISABLED) {
				fg.red   = (fg.red   + bg.red)   >> 1;
				fg.green = (fg.green + bg.green) >> 1;
				fg.blue  = (fg.blue  + bg.blue)   >> 1;
			}
			if(attr & DCA_COMMENT) {
				fg.red = fg.blue = 0;
				fg.green = 0x8080;
			}

			pango_layout_set_text(dv->playout, s, -1);
			gdk_gc_set_rgb_fg_color(dv->gc, &bg);
			gdk_draw_rectangle(GDK_DRAWABLE(wdv->window), dv->gc, TRUE, xx, yy, dvc->fixedfont_width, dvc->fixedfont_height);
			gdk_gc_set_rgb_fg_color(dv->gc, &fg);
			gdk_draw_layout(GDK_DRAWABLE(wdv->window), dv->gc, xx, yy, dv->playout);

			xx += dvc->fixedfont_width;
			viewdata++;
		}
		yy += dvc->fixedfont_height;
	}

	gtk_paint_shadow(wdv->style, wdv->window,
					 GTK_STATE_NORMAL, GTK_SHADOW_IN,
					 &event->area, wdv, "scrolled_window",
					 0, 0,
					 wdv->allocation.width - (dv->vs ? dv->vsz : 0),
					 wdv->allocation.height - (dv->hs ? dv->hsz : 0));

	GTK_WIDGET_CLASS(g_type_class_peek_parent(dvc))->expose_event(wdv, event);

	return FALSE;
}

static void dview_hadj_changed(GtkAdjustment *adj, DView *dv)
{
	UINT32 pcol = debug_view_get_property_UINT32(dv->dw, DVP_LEFT_COL);
	UINT32 v = (UINT32)(adj->value);
	if(v != pcol) {
		debug_view_set_property_UINT32(dv->dw, DVP_LEFT_COL, v);
		gtk_widget_queue_draw(GTK_WIDGET(dv));
	}
}

static void dview_vadj_changed(GtkAdjustment *adj, DView *dv)
{
	UINT32 pcol = debug_view_get_property_UINT32(dv->dw, DVP_TOP_ROW);
	UINT32 v = (UINT32)(adj->value);
	if(v != pcol) {
		if(dv->this_one_is_stupidly_autoscrolling)
			debug_view_set_property_UINT32(dv->dw, DVP_TEXTBUF_LINE_LOCK, v);
		else
			debug_view_set_property_UINT32(dv->dw, DVP_TOP_ROW, v);
		gtk_widget_queue_draw(GTK_WIDGET(dv));
	}
}

static void dview_realize(GtkWidget *wdv)
{
	GdkWindowAttr attributes;
	gint attributes_mask;
	DView *dv;

	GTK_WIDGET_SET_FLAGS(wdv, GTK_REALIZED);
	dv = DVIEW(wdv);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = wdv->allocation.x;
	attributes.y = wdv->allocation.y;
	attributes.width = wdv->allocation.width;
	attributes.height = wdv->allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gtk_widget_get_visual(wdv);
	attributes.colormap = gtk_widget_get_colormap(wdv);
	attributes.event_mask = gtk_widget_get_events(wdv) | GDK_EXPOSURE_MASK;
	
	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

	wdv->window = gdk_window_new(gtk_widget_get_parent_window(wdv), &attributes, attributes_mask);
	gdk_window_set_user_data(wdv->window, dv);
	dv->gc = gdk_gc_new(GDK_DRAWABLE(wdv->window));
	wdv->style = gtk_style_attach(wdv->style, wdv->window);
}

static void dview_size_allocate(GtkWidget *wdv, GtkAllocation *allocation)
{
	DView *dv = DVIEW(wdv);
	DViewClass *dvc = DVIEW_GET_CLASS(dv);
	UINT32 trow = debug_view_get_property_UINT32(dv->dw, DVP_TOTAL_ROWS);
	UINT32 tcol = debug_view_get_property_UINT32(dv->dw, DVP_TOTAL_COLS);
	int    prow = debug_view_get_property_UINT32(dv->dw, DVP_TOP_ROW);
	int    pcol = debug_view_get_property_UINT32(dv->dw, DVP_LEFT_COL);
	UINT32 ah   = allocation->height-2*wdv->style->ythickness;
	UINT32 aw   = allocation->width-2*wdv->style->xthickness;
	UINT32 crow, ccol, vrow, vcol;
	int ohs = dv->hs;
	int ovs = dv->vs;

	dv->tr = trow;
	dv->tc = tcol;

	dv->hs = tcol*dvc->fixedfont_width > aw;
	dv->vs = trow*dvc->fixedfont_height > ah;

	if(dv->hs)
		ah -= dv->hsz;
	if(dv->vs)
		aw -= dv->vsz;

	dv->hs = tcol*dvc->fixedfont_width > aw;
	dv->vs = trow*dvc->fixedfont_height > ah;

	ah = allocation->height - (dv->hs ? dv->hsz : 0);
	aw = allocation->width  - (dv->vs ? dv->vsz : 0);

	crow = (ah-2*wdv->style->ythickness+dvc->fixedfont_height-1) / dvc->fixedfont_height;
	ccol = (aw-2*wdv->style->xthickness+dvc->fixedfont_width-1) / dvc->fixedfont_width;

	wdv->allocation = *allocation;

	vrow = trow-prow;
	vcol = tcol-pcol;
	if(vrow > crow)
		vrow = crow;
	else if(vrow < crow) {
		prow = trow-crow;
		if(prow < 0)
			prow = 0;
		vrow = trow-prow;
	}
	if(vcol > ccol)
		vcol = ccol;
	else if(vcol < ccol) {
		pcol = tcol-ccol;
		if(pcol < 0)
			pcol = 0;
		vcol = tcol-pcol;
	}

	debug_view_set_property_UINT32(dv->dw, DVP_VISIBLE_ROWS, vrow);
	debug_view_set_property_UINT32(dv->dw, DVP_VISIBLE_COLS, vcol);
	debug_view_set_property_UINT32(dv->dw, DVP_TOP_ROW,  prow);
	debug_view_set_property_UINT32(dv->dw, DVP_LEFT_COL, pcol);

	if(GTK_WIDGET_REALIZED(wdv))
		gdk_window_move_resize(wdv->window,
							   allocation->x, allocation->y,
							   allocation->width, allocation->height);

	if(dv->hs) {
		GtkAllocation al;
		int span = (aw-2*wdv->style->xthickness) / dvc->fixedfont_width;

		if(!ohs)
			gtk_widget_show(dv->hscrollbar);
		al.x = 0;
		al.y = ah;
		al.width = aw;
		al.height = dv->hsz;
		gtk_widget_size_allocate(dv->hscrollbar, &al);
		if(pcol+span > tcol)
			pcol = tcol-span;
		if(pcol < 0)
			pcol = 0;
		dv->hadj->lower = 0;
		dv->hadj->upper = tcol;
		dv->hadj->value = pcol;
		dv->hadj->step_increment = 1;
		dv->hadj->page_increment = span;
		dv->hadj->page_size = span;
		gtk_adjustment_changed(dv->hadj);
		debug_view_set_property_UINT32(dv->dw, DVP_LEFT_COL, pcol);
	} else {
		if(ohs)
			gtk_widget_hide(dv->hscrollbar);
	}

	if(dv->vs) {
		GtkAllocation al;
		int span = (ah-2*wdv->style->ythickness) / dvc->fixedfont_height;

		if(!ovs)
			gtk_widget_show(dv->vscrollbar);
		al.x = aw;
		al.y = 0;
		al.width = dv->vsz;
		al.height = ah;
		gtk_widget_size_allocate(dv->vscrollbar, &al);
		if(prow+span > trow)
			prow = trow-span;
		if(prow < 0)
			prow = 0;
		dv->vadj->lower = 0;
		dv->vadj->upper = trow;
		dv->vadj->value = prow;
		dv->vadj->step_increment = 1;
		dv->vadj->page_increment = span;
		dv->vadj->page_size = span;
		gtk_adjustment_changed(dv->vadj);
		debug_view_set_property_UINT32(dv->dw, DVP_TOP_ROW, prow);
	} else {
		if(ovs)
			gtk_widget_hide(dv->vscrollbar);
	}
}

static void dview_size_request(GtkWidget *wdv, GtkRequisition *req)
{
	GtkRequisition req2;
	DView *dv = DVIEW(wdv);
	DViewClass *dvc = DVIEW_GET_CLASS(dv);
	UINT32 trow = debug_view_get_property_UINT32(dv->dw, DVP_TOTAL_ROWS);
	UINT32 tcol = debug_view_get_property_UINT32(dv->dw, DVP_TOTAL_COLS);
	int vs = 0, hs = 0;

	if(tcol > 50) {
		tcol = 50;
		hs = 1;
	}
	if(trow > 20) {
		trow = 20;
		vs = 1;
	}
	req->width = tcol*dvc->fixedfont_width+2*wdv->style->xthickness;
	req->height = trow*dvc->fixedfont_height+2*wdv->style->ythickness;

	gtk_widget_size_request(dv->hscrollbar, &req2);
	dv->hsz = req2.height;

	gtk_widget_size_request(dv->vscrollbar, &req2);
	dv->vsz = req2.width;

	if(hs)
		req->height += dv->hsz;
	if(vs)
		req->width += dv->vsz;
}

static void dview_forall(GtkContainer *dvc, gboolean include_internals, GtkCallback callback, gpointer callback_data)
{
	if(include_internals) {
		DView *dv = DVIEW(dvc);
		callback(dv->hscrollbar, callback_data);
		callback(dv->vscrollbar, callback_data);
	}
}


static void dview_class_init(DViewClass *dvc)
{
    GConfClient *conf = gconf_client_get_default();
    char *name = 0;
    dvc->fixedfont = 0;

	if(conf)
		name = gconf_client_get_string(conf, "/desktop/gnome/interface/monospace_font_name", 0);

    if(name) {
        dvc->fixedfont = pango_font_description_from_string(name);
        g_free(name);
    }

    if(!dvc->fixedfont)
		dvc->fixedfont = pango_font_description_from_string("Monospace 10");
    
    if(!dvc->fixedfont) {
		mame_printf_error("Couldn't find a monospace font, aborting\n");
		abort();
    }

	GTK_CONTAINER_CLASS(dvc)->forall = dview_forall;
	GTK_WIDGET_CLASS(dvc)->expose_event = dview_expose;
	GTK_WIDGET_CLASS(dvc)->realize = dview_realize;
	GTK_WIDGET_CLASS(dvc)->size_request = dview_size_request;
	GTK_WIDGET_CLASS(dvc)->size_allocate = dview_size_allocate;
}


static void dview_init(DView *dv)
{
	DViewClass *dvc;

	dvc = DVIEW_GET_CLASS(dv);

	if(!dvc->fixedfont_width) {
		PangoFontMetrics *metrics;
		metrics = pango_context_get_metrics(gtk_widget_get_pango_context(GTK_WIDGET(dv)), dvc->fixedfont, 0);
  
		dvc->fixedfont_width = PANGO_PIXELS(pango_font_metrics_get_approximate_char_width(metrics));
		dvc->fixedfont_height = PANGO_PIXELS(pango_font_metrics_get_ascent(metrics) +
											 pango_font_metrics_get_descent(metrics));
	}

	dv->dw = 0;
	gtk_widget_modify_font(GTK_WIDGET(dv), dvc->fixedfont);
	dv->playout = gtk_widget_create_pango_layout(GTK_WIDGET(dv), 0);
	pango_layout_set_font_description(dv->playout, dvc->fixedfont);
	dv->gc = 0;

	dv->hadj = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 1, 1, 1, 1));
	dv->hscrollbar = gtk_hscrollbar_new(dv->hadj);
	gtk_widget_set_parent(dv->hscrollbar, GTK_WIDGET(dv));
	g_object_ref(dv->hscrollbar);
	g_signal_connect(dv->hadj, "value-changed", G_CALLBACK(dview_hadj_changed), dv);

	dv->vadj = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 1, 1, 1, 1));
	dv->vscrollbar = gtk_vscrollbar_new(dv->vadj);
	gtk_widget_set_parent(dv->vscrollbar, GTK_WIDGET(dv));
	g_object_ref(dv->vscrollbar);
	g_signal_connect(dv->vadj, "value-changed", G_CALLBACK(dview_vadj_changed), dv);
}

static void dview_view_update(debug_view *dw)
{
	DView *dv = debug_view_get_property_ptr(dw, DVP_OSD_PRIVATE);
	UINT32 trow = debug_view_get_property_UINT32(dv->dw, DVP_TOTAL_ROWS);
	UINT32 tcol = debug_view_get_property_UINT32(dv->dw, DVP_TOTAL_COLS);

	if(dv->tr != trow || dv->tc != tcol)
		gtk_widget_queue_resize(GTK_WIDGET(dv));
	else
		gtk_widget_queue_draw(GTK_WIDGET(dv));
}

GtkWidget *dview_new(gchar *widget_name, gchar *string1, gchar *string2, gint int1, gint int2)
{
	GtkWidget *wdv = g_object_new(DVIEW_TYPE, NULL);
	DView *dv = DVIEW(wdv);
	dv->name = widget_name;
	dv->this_one_is_stupidly_autoscrolling = 0;
	return wdv;
}

void dview_set_debug_view(DView *dv, debug_view *dw)
{
	dv->dw = dw;
	debug_view_set_property_ptr(dw, DVP_OSD_PRIVATE, dv);
	debug_view_set_property_fct(dw, DVP_UPDATE_CALLBACK, (void *)dview_view_update);
}

void dview_this_one_is_stupidly_autoscrolling(DView *dv)
{
	dv->this_one_is_stupidly_autoscrolling = 1;
}

