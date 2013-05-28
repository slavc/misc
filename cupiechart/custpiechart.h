/*
 * Copyright (c) 2011 Sviatoslav Chagaev <sviatoslav.chagaev@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __CUST_PIE_CHART_H__
#define __CUST_PIE_CHART_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CUST_TYPE_PIE_CHART           (cust_pie_chart_get_type())
#define CUST_PIE_CHART(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), CUST_TYPE_PIE_CHART, CustPieChart))
#define CUST_PIE_CHART_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST    ((obj), CUST_TYPE_PIE_CHART, CustPieChartClass))
#define CUST_IS_PIE_CHART(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CUST_TYPE_PIE_CHART))
#define CUST_IS_PIE_CHART_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE    ((obj), CUST_TYPE_PIE_CHART_CLASS))
#define CUST_PIE_CHART_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS  ((obj), CUST_TYPE_PIE_CHART, CustPieChartClass))

typedef struct _CustPieChart CustPieChart;
typedef struct _CustPieChartClass CustPieChartClass;

struct _CustPieChart {
    GtkDrawingArea parent;
    /* private */
};

struct _CustPieChartClass {
    GtkDrawingAreaClass parent_class;
    void (*sector_clicked)(GtkWidget *widget, gint sector_index, gpointer data);

    /* Padding for future expansion */
    void (*_gtk_reserved1) (void);
    void (*_gtk_reserved2) (void);
    void (*_gtk_reserved3) (void);
    void (*_gtk_reserved4) (void);
};

GType        cust_pie_chart_get_type(void) G_GNUC_CONST;
GtkWidget   *cust_pie_chart_new();

void         cust_pie_chart_append_sector(CustPieChart *pie_chart, const gchar *name, double value, double red, double green, double blue);
void         cust_pie_chart_remove_sector(CustPieChart *pie_chart, gint sector_index);
gint         cust_pie_chart_get_n_sectors(CustPieChart *pie_chart);

const gchar *cust_pie_chart_get_sector_name(CustPieChart *pie_chart, gint sector_index);
double       cust_pie_chart_get_sector_value(CustPieChart *pie_chart, gint sector_index);
void         cust_pie_chart_get_sector_color(CustPieChart *pie_chart, gint sector_index, double *red, double *green, double *blue);

void         cust_pie_chart_set_sector_name(CustPieChart *pie_chart, gint sector_index, const gchar *name);
void         cust_pie_chart_set_sector_value(CustPieChart *pie_chart, gint sector_index, double value);
void         cust_pie_chart_set_sector_color(CustPieChart *pie_chart, gint sector_index, double red, double green, double blue);

#endif
