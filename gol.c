/* Conway's Game of Life implementation */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#define CELL_SIZE 20

int dflag;

int grid_w, grid_h;
gboolean *grid;
gboolean *shadow_grid;
gint timer_id = -1;

GtkWidget *draw_area;

int
dbgprintf(const char *fmt, ...) {
	int n;
	va_list ap;
	
	if (!dflag)
		return 0;
	
	va_start(ap, fmt);
	n = vfprintf(stderr, fmt, ap);
	va_end(ap);

	return n;
}

int
count_live_neighbours(int row, int col) {
	int i, j;
	int istart, iend;
	int jstart, jend;
	int n = 0;
	
	if (row == 0)
		istart = 0;
	else
		istart = row - 1;
	
	if (col == 0)
		jstart = 0;
	else
		jstart = col - 1;
	
	if (row == (grid_h - 1))
		iend = grid_h - 1;
	else
		iend = row + 1;
	
	if (col == (grid_w - 1))
		jend = grid_w - 1;
	else
		jend = col + 1;
	
	
	for (i = istart; i <= iend; ++i) {
		for (j = jstart; j <= jend; ++j) {
			if (i == row && j == col)
				continue;
			if (grid[i*grid_w+j])
				++n;
		}
	}
	
	return n;
}	

gboolean
update_grid(gpointer data) {
	int i, j, k, n;
	gboolean alive;
	
	dbgprintf("update_grid()\n");
	
	for (i = 0; i < grid_h; ++i) {
		k = grid_w * i;
		for (j = 0; j < grid_w; ++j) {
			alive = grid[k+j];
			n = count_live_neighbours(i, j);
			
			if (alive && n < 2)
				shadow_grid[k+j] = FALSE;
			else if (alive && n > 3)
				shadow_grid[k+j] = FALSE;
			else if (alive && (n == 2 || n == 3))
				shadow_grid[k+j] = TRUE;
			else if (!alive && n == 3)
				shadow_grid[k+j] = TRUE;
		}
	}
	
	memcpy(grid, shadow_grid, grid_w * grid_h * sizeof(gboolean));
	gdk_window_invalidate_rect(draw_area->window, NULL, TRUE);
			
	return TRUE;
}

void
resize_grid(int w, int h) {
	grid = realloc(grid, w * h * sizeof(gboolean));
	shadow_grid = realloc(shadow_grid, w * h * sizeof(gboolean));
	grid_w = w;
	grid_h = h;
	memset(grid, 0, grid_w * grid_h * sizeof(gboolean));
	memset(shadow_grid, 0, grid_w * grid_h * sizeof(gboolean));
}

gboolean
expose_event_cb(GtkWidget *widget, GdkEventExpose *event, gpointer data) {
	int i, j, k;
	GtkDrawingArea *da = GTK_DRAWING_AREA(widget);
	
	dbgprintf("expose_event_cb()\n");
	
	for (i = 0; i < grid_h; ++i) {
		k = i*grid_w;
		for (j = 0; j < grid_w; ++j) {
			gdk_draw_rectangle(widget->window,
				widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
				grid[k+j],
				j*CELL_SIZE,
				i*CELL_SIZE,
				CELL_SIZE,
				CELL_SIZE);
		}
	}
	
	return TRUE;
}

gboolean
click_event_cb(GtkWidget *w, GdkEventButton *e, gpointer data) {
	int i, j, k, n; 
	
	dbgprintf("click_event_cb()\n");
	
	if (e->type != GDK_BUTTON_RELEASE)
		return FALSE;
	
	i = e->y / CELL_SIZE;
	j = e->x / CELL_SIZE;
	
	dbgprintf("i = %i, j = %i\n", i, j);
	
	if (i < 0 || j < 0 || i >= grid_h || j >= grid_w) {
		dbgprintf("discarding\n");
		return TRUE;
	}
	
	k = i*grid_w+j;
	if (grid[k])
		grid[k] = FALSE;
	else
		grid[k] = TRUE;
	
	n = count_live_neighbours(i, j);
	dbgprintf("live neighbours: %i\n", n);
	
	gdk_window_invalidate_rect(w->window, NULL, TRUE);
	
	return FALSE;
}

void
run_cb(GtkWidget *w, gpointer data) {
	timer_id = g_timeout_add(500, update_grid, NULL);
}

void
pause_cb(GtkWidget *w, gpointer data) {
	if (timer_id == -1)
		return;
	g_source_remove(timer_id);
	timer_id = -1;
}

void
new_cb(GtkWidget *w, gpointer data) {
	dbgprintf("new_cb()\n");
	
	pause_cb(NULL, NULL);
	
	memset(grid, 0, grid_w * grid_h * sizeof(gboolean));
	memset(shadow_grid, 0, grid_w * grid_h * sizeof(gboolean));
	gdk_window_invalidate_rect(draw_area->window, NULL, TRUE);
	// expose_event_cb(draw_area, NULL, NULL);
}

GtkWidget *
create_main_window(void) {
	GtkWidget *w;
	
	w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(w), 400, 300);
	gtk_window_set_position(GTK_WINDOW(w), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW(w), "Conway's Game of Life");
	g_signal_connect_swapped(G_OBJECT(w), "destroy", G_CALLBACK(gtk_main_quit), NULL);
	
	return w;
}

GtkWidget *
create_scrolled_window(void) {
	GtkWidget *w;
	
	w = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW(w), GTK_CORNER_TOP_LEFT);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(w), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	
	return w;
}

GtkWidget *
create_drawing_area(void) {
	GtkWidget *w;
	
	w = gtk_drawing_area_new();
	/* gtk_widget_set_size_request(w, 300, 300); */
	g_signal_connect(G_OBJECT(w), "expose_event", G_CALLBACK(expose_event_cb), NULL);
	gtk_widget_add_events(w, GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK);
	g_signal_connect(G_OBJECT(w), "button-release-event", G_CALLBACK(click_event_cb), NULL);
	g_signal_connect_swapped(G_OBJECT(w), "button-release-event", G_CALLBACK(expose_event_cb), w);
	
	return w;
}

GtkWidget *
create_menu_bar(GtkAccelGroup *accel_group) {
	GtkWidget *menu_bar;
	GtkWidget *file_menu;
	GtkWidget *file;
	GtkWidget *new;
	GtkWidget *pause;
	GtkWidget *run;
	GtkWidget *sep, *sep2;
	GtkWidget *quit;
	
	menu_bar = gtk_menu_bar_new();
	file_menu = gtk_menu_new();
	file = gtk_menu_item_new_with_label("File");
	new = gtk_image_menu_item_new_from_stock(GTK_STOCK_NEW, NULL);
	run = gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PLAY, NULL);
	pause = gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PAUSE, NULL);
	sep = gtk_separator_menu_item_new();
	sep2 = gtk_separator_menu_item_new();
	quit = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
	
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(file), file_menu);
	
	gtk_menu_append(file_menu, new);
	gtk_menu_append(file_menu, sep);
	gtk_menu_append(file_menu, run);
	gtk_menu_append(file_menu, pause);
	gtk_menu_append(file_menu, sep2);
	gtk_menu_append(file_menu, quit);
	gtk_menu_append(menu_bar, file);
	
	g_signal_connect(G_OBJECT(new), "activate", G_CALLBACK(new_cb), NULL);
	g_signal_connect(G_OBJECT(run), "activate", G_CALLBACK(run_cb), NULL);
	g_signal_connect(G_OBJECT(pause), "activate", G_CALLBACK(pause_cb), NULL);
	g_signal_connect(G_OBJECT(quit), "activate", G_CALLBACK(gtk_main_quit), NULL);
	
	if (accel_group) {
		gtk_widget_add_accelerator(new, "activate", accel_group, GDK_n, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
		gtk_widget_add_accelerator(run, "activate", accel_group, GDK_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
		gtk_widget_add_accelerator(pause, "activate", accel_group, GDK_d, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
		gtk_widget_add_accelerator(quit, "activate", accel_group, GDK_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	}
	
	return menu_bar;
}

void
usage(const char *progname) {
	printf("usage: %s [-d]\n", progname);
	
	exit(0);
}

void
proc_args(int argc, char **argv) {
	int i;
	
	for (i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-d"))
			dflag = 1;
		else
			usage(argv[0]);
	}
}

int
main(int argc, char **argv) {
	int i;
	GtkWidget
		*window,
		*scrolled_window,
		*vbox,
		*menu_bar;
	GtkAccelGroup *accel_group;
	
	resize_grid(30, 30);
	memset(grid, FALSE, grid_w * grid_h * sizeof(gboolean));
	
	gtk_init(&argc, &argv);
	proc_args(argc, argv);
	
	accel_group = gtk_accel_group_new();
	window = create_main_window();
	vbox = gtk_vbox_new(FALSE, 0);
	scrolled_window = create_scrolled_window();
	draw_area = create_drawing_area();
	menu_bar = create_menu_bar(accel_group);
	
	gtk_widget_set_size_request(draw_area, grid_w*CELL_SIZE+1, grid_h*CELL_SIZE+1);
	
	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), draw_area);
	
	gtk_widget_show_all(window);
	
	gtk_main();
	
	exit(0);
}
