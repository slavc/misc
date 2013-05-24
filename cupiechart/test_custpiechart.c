#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <gtk/gtk.h>

#include "custpiechart.h"

GtkWidget *pie_chart;

void
on_add_sector_but_clicked(GtkWidget *widget, gpointer data)
{
    GtkEntry *entry = data;
    const gchar *param_text = gtk_entry_get_text(entry);

    int n_scanned;
    char name[64];
    double value;
    double red, green, blue;

    n_scanned = sscanf(param_text, "%63s %lf %lf %lf %lf", name, &value, &red, &green, &blue);
    g_print("adding sector: \"%s\" value(%f) color(%f, %f, %f)\n", name, value, red, green, blue);

    if (n_scanned != 5)
        return;
    cust_pie_chart_append_sector(CUST_PIE_CHART(pie_chart), name, value, red, green, blue);
}

void
on_del_sector_but_clicked(GtkWidget *widget, gpointer data)
{
    GtkEntry *entry = data;
    const gchar *sector_index_text = gtk_entry_get_text(entry);
    int sector_index;

    if (sscanf(sector_index_text, "%i", &sector_index) != 1)
        return;
    cust_pie_chart_remove_sector(CUST_PIE_CHART(pie_chart), sector_index);
}

void
test_getters(CustPieChart *pie_chart, int n_sectors, gboolean do_try_to_crash)
{
    int i;
    int n;
    double r, g, b;

    if (do_try_to_crash)
        n = n_sectors + 10;
    else
        n = n_sectors;

    g_print("n_sectors = %i\n", cust_pie_chart_get_n_sectors(CUST_PIE_CHART(pie_chart)));
    for (i = 0; i < n; ++i)
        g_print("%i sector name = %s\n", i, cust_pie_chart_get_sector_name(CUST_PIE_CHART(pie_chart), i));
    for (i = 0; i < n; ++i)
        g_print("%i sector value = %f\n", i, cust_pie_chart_get_sector_value(CUST_PIE_CHART(pie_chart), i));
    for (i = 0; i < n; ++i) {
        cust_pie_chart_get_sector_color(CUST_PIE_CHART(pie_chart), i, &r, &g, &b);
        g_print("%i sector color = %f %f %f\n", i, r, g, b);
    }
}

void
test_setters(CustPieChart *pie_chart, int n_sectors, gboolean do_try_to_crash)
{
    int i;
    int n;
    char buf[64];
    const size_t bufsize = sizeof buf;

    if (do_try_to_crash)
        n = n_sectors + 10;
    else
        n = n_sectors;

    srand(time(NULL));

#define RAND_DOUBLE ((double) rand() / (double) RAND_MAX)

    for (i = 0; i < n; ++i) {
        snprintf(buf, bufsize, "Sector %i", i);
        cust_pie_chart_set_sector_name(pie_chart, i, buf);
        cust_pie_chart_set_sector_value(pie_chart, i, RAND_DOUBLE);
        cust_pie_chart_set_sector_color(pie_chart, i, RAND_DOUBLE, RAND_DOUBLE, RAND_DOUBLE);
    }

#undef RAND_DOUBLE
}

void
on_pie_chart_sector_clicked(GtkWidget *widget, gint sector_index, gpointer data)
{
    g_debug("sector_index = %i", sector_index);
    g_debug("sector \"%s\" clicked", cust_pie_chart_get_sector_name(CUST_PIE_CHART(pie_chart), sector_index));
}

int
main(int argc, char **argv)
{
    int i;
    GtkWidget *window;
    GtkWidget *paned;
    GtkWidget *table;
    GtkWidget *sector_param_entry;
    GtkWidget *sector_index_entry;
    GtkWidget *add_sector_but;
    GtkWidget *del_sector_but;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    paned = gtk_vpaned_new();
    pie_chart = cust_pie_chart_new();
    gtk_widget_destroy(pie_chart);
    pie_chart = cust_pie_chart_new();
    g_signal_connect(G_OBJECT(pie_chart), "sector-clicked", G_CALLBACK(on_pie_chart_sector_clicked), NULL);

    gtk_paned_pack1(GTK_PANED(paned), pie_chart, TRUE, TRUE);

    table = gtk_table_new(2, 2, TRUE);
    sector_param_entry = gtk_entry_new();
    sector_index_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(sector_param_entry), "Apples 0.8 1.0 0.0 0.0");
    add_sector_but = gtk_button_new_with_label("Add sector");
    del_sector_but = gtk_button_new_with_label("Del sector");

    gtk_table_attach(GTK_TABLE(table), sector_param_entry,
            0, 1,
            0, 1,
            GTK_SHRINK | GTK_EXPAND | GTK_FILL,
            GTK_SHRINK | GTK_EXPAND | GTK_FILL,
            0, 0);
    gtk_table_attach(GTK_TABLE(table), sector_index_entry,
            0, 1,
            1, 2,
            GTK_SHRINK | GTK_EXPAND | GTK_FILL,
            GTK_SHRINK | GTK_EXPAND | GTK_FILL,
            0, 0);
    gtk_table_attach(GTK_TABLE(table), add_sector_but,
            1, 2,
            0, 1,
            GTK_SHRINK | GTK_EXPAND | GTK_FILL,
            GTK_SHRINK | GTK_EXPAND | GTK_FILL,
            0, 0);
    gtk_table_attach(GTK_TABLE(table), del_sector_but,
            1, 2,
            1, 2,
            GTK_SHRINK | GTK_EXPAND | GTK_FILL,
            GTK_SHRINK | GTK_EXPAND | GTK_FILL,
            0, 0);
    gtk_paned_pack2(GTK_PANED(paned), table, FALSE, FALSE);


    gtk_container_add(GTK_CONTAINER(window), paned);


    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(G_OBJECT(add_sector_but), "clicked", G_CALLBACK(on_add_sector_but_clicked), sector_param_entry);
    g_signal_connect(G_OBJECT(del_sector_but), "clicked", G_CALLBACK(on_del_sector_but_clicked), sector_index_entry);

    gtk_widget_set_size_request(window, 500, 400);
    gtk_widget_show_all(window);

    cust_pie_chart_append_sector(CUST_PIE_CHART(pie_chart), "Pears", 0.3, 0.0, 1.0, 0.1);
    cust_pie_chart_append_sector(CUST_PIE_CHART(pie_chart), "Wines", 0.2, 1.0, 0.1, 1.0);
    cust_pie_chart_append_sector(CUST_PIE_CHART(pie_chart), "Oranges", 0.1, 1.0, 0.7, 0.0);

    for (i = 0; i < 0; ++i) {
        test_getters(CUST_PIE_CHART(pie_chart), 3, FALSE);
        test_setters(CUST_PIE_CHART(pie_chart), 3, FALSE);
    }

    //cust_pie_chart_remove_sector(CUST_PIE_CHART(pie_chart), 2);

    gtk_main();

    return 0;
}
