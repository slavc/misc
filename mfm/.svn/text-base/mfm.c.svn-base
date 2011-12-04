#include <gtk/gtk.h>

#include "conf.h"

static struct conf conf;

static GtkWidget *window;

static GtkWidget *create_icon_view(void);

void
mfm(void)
{
	int error;

	conf_defaults(&conf);
	load_conf(&conf);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_move(GTK_WINDOW(window), conf.window_x, conf.window_y);
	gtk_window_set_default_size(GTK_WINDOW(window), conf.window_w, conf.window_h);

	gtk_container_add(GTK_CONTAINER(window), create_icon_view());

	gtk_widget_show_all(window);
}

static GtkWidget *
create_icon_view(void)
{
	GtkWidget *swin, *icon_view;

	icon_view = gtk_icon_view_new();

	swin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(swin, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swin), icon_view);

	return swin;
}
