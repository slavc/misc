#include <stdio.h>
#include <stdlib.h>

#include <err.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

static void	 usage(void);
static void	 gtk_ssh_askpass(const char *);
static gboolean	 on_escape_key(GtkAccelGroup *, GObject *, guint, GdkModifierType);
static gboolean	 on_window_delete(GtkWidget *);
static void	 on_ok_clicked(GtkWidget *, gpointer);
static void	 on_cancel_clicked(GtkWidget *, gpointer);

static int		 exit_code = 1;
static GtkWidget	*entry;

int
main(int argc, char **argv)
{
	if (argc != 2)
		usage();

	gtk_ssh_askpass(argv[1]);

	exit(exit_code);
}

static void
usage()
{
	fprintf(stderr, "usage: gtk_ssh_askpass <prompt>\n");
	exit(1);
}

static void
gtk_ssh_askpass(const char *prompt)
{
	GtkWidget *window, *vbox, *label, *hbox, *ok, *cancel;
	GtkAccelGroup *accel;
	GClosure *closure;

	gtk_init(NULL, NULL);

	accel = gtk_accel_group_new();
	closure = g_cclosure_new(G_CALLBACK(on_escape_key), NULL, NULL);
	gtk_accel_group_connect(accel, GDK_KEY_Escape, 0, 0, closure);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), 10, 10);
	gtk_container_set_border_width(GTK_CONTAINER(window), 5);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
	gtk_window_add_accel_group(GTK_WINDOW(window), accel);

	vbox = gtk_vbox_new(FALSE, 4);
	label = gtk_label_new(prompt);
	entry = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
	hbox = gtk_hbox_new(FALSE, 5);
	ok = gtk_button_new_from_stock(GTK_STOCK_OK);
	cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);

	gtk_box_pack_end(GTK_BOX(hbox), cancel, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), ok, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(window), vbox);

	gtk_widget_show_all(window);

	gtk_widget_grab_focus(window);
	gtk_widget_grab_focus(entry);
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	gtk_widget_set_can_default(ok, TRUE);
	gtk_widget_grab_default(ok);

	g_signal_connect(G_OBJECT(window), "delete-event",
	    G_CALLBACK(on_window_delete), NULL);
	g_signal_connect(G_OBJECT(ok), "clicked",
	    G_CALLBACK(on_ok_clicked), NULL);
	g_signal_connect(G_OBJECT(cancel), "clicked",
	    G_CALLBACK(on_cancel_clicked), NULL);

	gtk_main();
}

static gboolean
on_escape_key(GtkAccelGroup *accel, GObject *acceleratable, guint keyval,
    GdkModifierType modifier)
{
	exit_code = 1;

	gtk_main_quit();

	return FALSE;
}

static gboolean
on_window_delete(GtkWidget *widget)
{
	exit_code = 1;

	gtk_main_quit();

	return FALSE;
}

static void
on_ok_clicked(GtkWidget *button, gpointer data)
{
	const char *passwd;

	passwd = gtk_entry_get_text(GTK_ENTRY(entry));
	printf("%s\n", passwd);

	exit_code = 0;

	gtk_main_quit();
}

static void
on_cancel_clicked(GtkWidget *button, gpointer data)
{
	exit_code = 1;

	gtk_main_quit();
}
