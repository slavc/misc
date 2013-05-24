#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>

#include "custpiechart.h"

#define PADDING 5
#define LEGEND_PADDING 5
#define LEGEND_COLOR_BOX_PADDING 3
#define COMPUTE_RADIUS(widget) MIN(widget->allocation.width / 2, widget->allocation.height / 2) - PADDING

G_DEFINE_TYPE(CustPieChart, cust_pie_chart, GTK_TYPE_DRAWING_AREA);

enum {
    SECTOR_CLICKED,
    LAST_SIGNAL
};
static guint cust_pie_chart_signals[LAST_SIGNAL];

typedef struct _sector sector_t;
struct _sector {
    gchar *name;
    double value;
    double red, green, blue;
};

typedef struct _CustPieChartPrivate CustPieChartPrivate;
struct _CustPieChartPrivate {
    gint n_sectors;
    sector_t *sectors;
    gint hovered_sector_index;
    gboolean is_left_button_pressed;
};
#define CUST_PIE_CHART_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), CUST_TYPE_PIE_CHART, CustPieChartPrivate))

static void     cust_pie_chart_finalize(GObject *obj);
static gboolean cust_pie_chart_expose(GtkWidget *pie_chart, GdkEventExpose *event);
static void     cust_pie_chart_update(CustPieChart *pie_chart);
static gboolean cust_pie_chart_motion_notify(GtkWidget *pie_chart, GdkEventMotion *event);
static gboolean cust_pie_chart_button_press(GtkWidget *pie_chart, GdkEventButton *event);
static gboolean cust_pie_chart_button_release(GtkWidget *pie_chart, GdkEventButton *event);

static void
cust_pie_chart_class_init(CustPieChartClass *class)
{
    GObjectClass *obj_class;
    GtkWidgetClass *widget_class;
    g_debug("%s()", __func__);

    obj_class = G_OBJECT_CLASS(class);
    obj_class->finalize = cust_pie_chart_finalize;
    widget_class = GTK_WIDGET_CLASS(class);
    widget_class->expose_event = cust_pie_chart_expose;
    widget_class->button_press_event = cust_pie_chart_button_press;
    widget_class->button_release_event = cust_pie_chart_button_release;
    widget_class->motion_notify_event = cust_pie_chart_motion_notify;
    g_type_class_add_private(class, sizeof(CustPieChartPrivate));

    cust_pie_chart_signals[SECTOR_CLICKED] = g_signal_new("sector-clicked",
                                                           G_OBJECT_CLASS_TYPE(class),
                                                           G_SIGNAL_RUN_FIRST,
                                                           G_STRUCT_OFFSET(CustPieChartClass, sector_clicked),
                                                           NULL,
                                                           NULL,
                                                           g_cclosure_marshal_VOID__INT,
                                                           G_TYPE_NONE,
                                                           1,
                                                           G_TYPE_INT);
}

static void
cust_pie_chart_init(CustPieChart *pie_chart)
{
    CustPieChartPrivate *priv;
    g_debug("%s()", __func__);

    priv = CUST_PIE_CHART_GET_PRIVATE(pie_chart);
    memset(priv, 0, sizeof(*priv));
    priv->hovered_sector_index = -1;
    priv->is_left_button_pressed = FALSE;
    gtk_widget_add_events(GTK_WIDGET(pie_chart), GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
    cust_pie_chart_update(pie_chart);
}

static void
cust_pie_chart_finalize(GObject *obj)
{
    int i;
    CustPieChart *pie_chart;
    CustPieChartPrivate *priv;
    g_debug("%s()", __func__);

    pie_chart = CUST_PIE_CHART(obj);
    priv = CUST_PIE_CHART_GET_PRIVATE(pie_chart);
    for (i = 0; i < priv->n_sectors; ++i)
        g_free(priv->sectors[i].name);
    g_free(priv->sectors);
}

struct point {
    double x;
    double y;
};

/* returns the sign of half-plane, or 0 if c belongs to a,b */
static double
orient(struct point *a, struct point *b, struct point *c)
{
    return (a->x - c->x)*(b->y - c->y) - (a->y - c->y) * (b->x - c->x);
}

static gboolean
cust_pie_chart_motion_notify(GtkWidget *pie_chart, GdkEventMotion *event)
{
    CustPieChartPrivate *priv;
    int i;
    double radius, value_sum;
    double hyp;
    int sector_index = -1;
    double angle, start_angle;
    struct point mouse, center, secant;
    double o1, o2;
    int old_hovered_sector_index;

    priv = CUST_PIE_CHART_GET_PRIVATE(pie_chart);
    if (priv->n_sectors <= 0)
        return FALSE;

    /* Determine if mouse is in the circle */

    radius = COMPUTE_RADIUS(pie_chart);
    center.x = radius + PADDING;
    center.y = pie_chart->allocation.height / 2;
    mouse.x = event->x;
    mouse.y = event->y;
    hyp = sqrt((mouse.x - center.x) * (mouse.x - center.x) + (mouse.y - center.y) * (mouse.y - center.y));
    if (hyp > radius) {
        //g_debug("mouse NOT in pie chart");
        old_hovered_sector_index = priv->hovered_sector_index;
        priv->hovered_sector_index = -1;
        priv->is_left_button_pressed = FALSE;
        if (priv->hovered_sector_index != old_hovered_sector_index)
            cust_pie_chart_update(CUST_PIE_CHART(pie_chart));
        return FALSE;
    }
    //g_debug("mouse IN pie chart");
    
    /* Determine over which sector the mouse is hovering */

    for (i = 0, value_sum = 0.0; i < priv->n_sectors; ++i)
        value_sum += priv->sectors[i].value;
    start_angle = -M_PI/2.0; /* we rotated by -90deg when drawing, so we must do the same here */
    for (i = 0; i < priv->n_sectors; ++i) {
        //g_debug("sector[%i]", i);
        angle = start_angle + (M_PI * 2) * (priv->sectors[i].value / value_sum);

        secant.x = center.x + cos(start_angle);
        secant.y = center.y + sin(start_angle);
        //g_debug("center = (%4.2f, %4.2f); start_secant = (%4.2f, %4.2f); mouse = (%4.2f, %4.2f);", center.x, center.y, secant.x, secant.y, mouse.x, mouse.y);
        o1 = orient(&center, &secant, &mouse);

        secant.x = center.x + cos(angle);
        secant.y = center.y + sin(angle);
        //g_debug("center = (%4.2f, %4.2f);       secant = (%4.2f, %4.2f); mouse = (%4.2f, %4.2f);", center.x, center.y, secant.x, secant.y, mouse.x, mouse.y);
        o2 = orient(&center, &secant, &mouse);

        //g_debug("o1 = %4.2f; o2 = %4.2f;", o1, o2);


        if ( (o1 >= 0.0 && o2 <= 0.0) || ((angle - start_angle) >= M_PI && (o1 >= 0.0 || o2 <= 0.0)) ) {
            sector_index = i;
            break;
        }

        start_angle = angle;
    }

    //g_debug("sector_index = %i;", sector_index);
    //g_debug("");

    old_hovered_sector_index = priv->hovered_sector_index;
    priv->hovered_sector_index = sector_index;
    if (sector_index != old_hovered_sector_index)
        cust_pie_chart_update(CUST_PIE_CHART(pie_chart));

    return FALSE;
}

static gboolean
cust_pie_chart_button_press(GtkWidget *pie_chart, GdkEventButton *event)
{
    CustPieChartPrivate *priv;

    if (event->button != 1)
        return FALSE;

    priv = CUST_PIE_CHART_GET_PRIVATE(pie_chart);
    priv->is_left_button_pressed = TRUE;

    if (priv->hovered_sector_index != -1)
        cust_pie_chart_update(CUST_PIE_CHART(pie_chart));

    return FALSE;
}

static gboolean
cust_pie_chart_button_release(GtkWidget *pie_chart, GdkEventButton *event)
{
    CustPieChartPrivate *priv;

    if (event->button != 1)
        return FALSE;

    priv = CUST_PIE_CHART_GET_PRIVATE(pie_chart);
    priv->is_left_button_pressed = FALSE;


    if (priv->hovered_sector_index != -1) {
        cust_pie_chart_update(CUST_PIE_CHART(pie_chart));
        g_signal_emit(pie_chart, cust_pie_chart_signals[SECTOR_CLICKED], 0, priv->hovered_sector_index);
    }

    return FALSE;
}

static int
compute_legend_color_box_size(cairo_t *cr)
{
    cairo_text_extents_t ex;

    cairo_text_extents(cr, "100%", &ex);
    return LEGEND_COLOR_BOX_PADDING * 2 + ex.width;
}

static void
find_largest_sector_name(cairo_t *cr, CustPieChartPrivate *priv, int *pwidth, int *pheight)
{
    int i;
    cairo_text_extents_t ex;

    *pwidth = *pheight = 0;

    for (i = 0; i < priv->n_sectors; ++i) {
        cairo_text_extents(cr, priv->sectors[i].name, &ex);
        if (ex.height > *pheight)
            *pheight = ex.height;
        if (ex.width > *pwidth)
            *pwidth = ex.width;
    }
}

static void
draw_legend(cairo_t *cr, int x_shift, GtkWidget *pie_chart) 
{
    int i;
    int n_lines_per_col;
    int n_cols;
    int x, y;
    CustPieChartPrivate *priv;
    int legend_color_box_size;
    int legend_line_width, legend_line_height;
    cairo_text_extents_t ex;
    double value_sum;
    double r, g, b;
    double color_box_text_color;
    char buf[128];
    const size_t bufsize = sizeof buf;

    priv = CUST_PIE_CHART_GET_PRIVATE(pie_chart);

    for (i = 0, value_sum = 0.0; i < priv->n_sectors; ++i)
        value_sum += priv->sectors[i].value;

    legend_color_box_size = compute_legend_color_box_size(cr);
    find_largest_sector_name(cr, priv, &legend_line_width, &legend_line_height);
    legend_line_height = MAX(legend_line_height, legend_color_box_size) + LEGEND_PADDING * 2;
    legend_line_width += legend_color_box_size + LEGEND_PADDING * 3;

    n_lines_per_col = pie_chart->allocation.height / legend_line_height;
    x = x_shift;
    y = 0;
    for (i = 0; i < priv->n_sectors; ++i) {
        if (i != 0 && i % n_lines_per_col == 0) {
            x += legend_line_width;
            y = 0;
        }
        
        r = priv->sectors[i].red;
        g = priv->sectors[i].green;
        b = priv->sectors[i].blue;
        cairo_rectangle(cr, x + LEGEND_PADDING, y + LEGEND_PADDING, legend_color_box_size, legend_color_box_size);
        cairo_set_source_rgb(cr, r, g, b);
        cairo_fill(cr);

        if ((r + g + b) / 3.0 < 0.3)
            color_box_text_color = 1.0;
        else
            color_box_text_color = 0.0;
        cairo_set_source_rgb(cr, color_box_text_color, color_box_text_color, color_box_text_color);
        cairo_move_to(cr, x + LEGEND_PADDING + LEGEND_COLOR_BOX_PADDING, y + LEGEND_PADDING + LEGEND_COLOR_BOX_PADDING + legend_color_box_size / 2);
        snprintf(buf, bufsize, "%i%%", (int) (100.0*priv->sectors[i].value/value_sum));
        cairo_show_text(cr, buf);

        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_move_to(cr, x + 2*LEGEND_PADDING + legend_color_box_size, y + LEGEND_PADDING + LEGEND_COLOR_BOX_PADDING + legend_color_box_size / 2);
        cairo_show_text(cr, priv->sectors[i].name);

        y += legend_line_height;
    }
}

static void
draw(cairo_t *cr, GtkWidget *pie_chart)
{
    CustPieChartPrivate *priv;
    gint i;
    gint n_sectors;
    sector_t *sectors;
    double x, y, radius;
    double value_sum, fraction;
    double start_angle;
    double angle;
    double line_width;
    cairo_path_t *hovered_sector_path = NULL;
    cairo_text_extents_t txt_exts;
    g_debug("%s()", __func__);

#define STROKE_COLOR         0.0, 0.0, 0.0
#define HOVERED_LINE_WIDTH   2.0
#define PRESSED_STROKE_COLOR 0.0, 0.2, 1.0

    priv = CUST_PIE_CHART_GET_PRIVATE(pie_chart);
    n_sectors = priv->n_sectors;
    sectors = priv->sectors;

    /* Draw the pie chart itself */

    radius = COMPUTE_RADIUS(pie_chart);
    x = radius + PADDING; //pie_chart->allocation.x + pie_chart->allocation.width / 2;
    y = pie_chart->allocation.y + pie_chart->allocation.height / 2;

    for (i = 0, value_sum = 0.0; i < n_sectors; ++i)
        value_sum += sectors[i].value;
    start_angle = 0.0;
    cairo_save(cr);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_rotate(cr, -M_PI/2.0);
    cairo_device_to_user(cr, &x, &y);
    for (i = 0; i < n_sectors; ++i) {
        fraction = sectors[i].value / value_sum;
        angle = start_angle + (2 * M_PI) * fraction;

        cairo_move_to(cr, x, y);
        cairo_arc(cr, x, y, radius, start_angle, angle);
        if (n_sectors > 1)
            cairo_close_path(cr);
        cairo_set_source_rgb(cr, sectors[i].red, sectors[i].green, sectors[i].blue);
        cairo_fill_preserve(cr);
        cairo_set_source_rgb(cr, STROKE_COLOR);
        if (i == priv->hovered_sector_index)
            hovered_sector_path = cairo_copy_path(cr);
        cairo_stroke(cr);

        start_angle = angle;
    }

    if (hovered_sector_path) {
        cairo_append_path(cr, hovered_sector_path);
        line_width = cairo_get_line_width(cr);
        cairo_set_line_width(cr, line_width * HOVERED_LINE_WIDTH);
        if (priv->is_left_button_pressed)
            cairo_set_source_rgb(cr, PRESSED_STROKE_COLOR);
        else
            cairo_set_source_rgb(cr, STROKE_COLOR);
        cairo_stroke(cr);
        cairo_set_line_width(cr, line_width);
        cairo_path_destroy(hovered_sector_path);
        hovered_sector_path = NULL;
    }
    cairo_restore(cr);

    cairo_save(cr);
    cairo_select_font_face(cr, "sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    draw_legend(cr, 2*radius + 3*PADDING, pie_chart);
    cairo_restore(cr);
}

static gboolean
cust_pie_chart_expose(GtkWidget *pie_chart, GdkEventExpose *event)
{
    cairo_t *cr;
    g_debug("%s()", __func__);

    cr = gdk_cairo_create(pie_chart->window);

    cairo_rectangle(cr, event->area.x, event->area.y, event->area.width, event->area.height);
    cairo_clip(cr);

    draw(cr, pie_chart);

    cairo_destroy(cr);

    return FALSE;
}

static void
cust_pie_chart_update(CustPieChart *pie_chart)
{
    GtkWidget *widget;
    GdkRegion *region;
    g_debug("%s()", __func__);

    widget = GTK_WIDGET(pie_chart);

    if (!widget->window)
        return;

    region = gdk_drawable_get_clip_region(widget->window);
    /* redraw the cairo canvas completely by exposing it */
    gdk_window_invalidate_region(widget->window, region, TRUE);
    gdk_window_process_updates(widget->window, TRUE);

    gdk_region_destroy(region);
}

GtkWidget *
cust_pie_chart_new(void)
{
    g_debug("%s()", __func__);
    return g_object_new(CUST_TYPE_PIE_CHART, NULL);
}

void
cust_pie_chart_append_sector(CustPieChart *pie_chart, const gchar *name, double value, double red, double green, double blue)
{
    CustPieChartPrivate *priv;
    sector_t *sec;
    void *p;
    g_debug("%s()", __func__);

    g_assert(pie_chart != NULL && name != NULL && value >= 0.0 && red >= 0.0 && green >= 0.0 && blue >= 0.0 && red <= 1.0 && green <= 1.0 && blue <= 1.0);

    priv = CUST_PIE_CHART_GET_PRIVATE(pie_chart);

    priv->sectors = g_realloc(priv->sectors, sizeof(*priv->sectors) * (priv->n_sectors + 1));

    sec = priv->sectors + priv->n_sectors;
    memset(sec, 0, sizeof(*sec));
    sec->name = g_strdup(name);
    sec->value = value;
    sec->red = red;
    sec->green = green;
    sec->blue = blue;

    ++priv->n_sectors;

    cust_pie_chart_update(pie_chart);
}

void
cust_pie_chart_remove_sector(CustPieChart *pie_chart, gint sector_index)
{
    gchar *name;
    CustPieChartPrivate *priv;
    g_debug("%s()", __func__);

    g_assert(pie_chart != NULL);

    priv = CUST_PIE_CHART_GET_PRIVATE(pie_chart);

    g_assert(sector_index >= 0 && sector_index < priv->n_sectors);

    name = priv->sectors[sector_index].name;
    --(priv->n_sectors);
    memmove(priv->sectors + sector_index, priv->sectors + sector_index + 1, sizeof(*priv->sectors) * (priv->n_sectors - sector_index));
    priv->sectors = realloc(priv->sectors, sizeof(*priv->sectors) * priv->n_sectors);
    g_free(name);

    cust_pie_chart_update(pie_chart);
}

gint
cust_pie_chart_get_n_sectors(CustPieChart *pie_chart)
{
    CustPieChartPrivate *priv;
    g_debug("%s()", __func__);

    g_assert(pie_chart != NULL);

    priv = CUST_PIE_CHART_GET_PRIVATE(pie_chart);

    return priv->n_sectors;
}

const gchar *
cust_pie_chart_get_sector_name(CustPieChart *pie_chart, gint sector_index)
{
    CustPieChartPrivate *priv;

    g_assert(pie_chart != NULL);

    priv = CUST_PIE_CHART_GET_PRIVATE(pie_chart);
    
    g_assert(sector_index >= 0 && sector_index < priv->n_sectors);

    return priv->sectors[sector_index].name;
}

double
cust_pie_chart_get_sector_value(CustPieChart *pie_chart, gint sector_index)
{
    CustPieChartPrivate *priv;

    g_assert(pie_chart != NULL);

    priv = CUST_PIE_CHART_GET_PRIVATE(pie_chart);
    
    g_assert(sector_index >= 0 && sector_index < priv->n_sectors);

    return priv->sectors[sector_index].value;
}

void
cust_pie_chart_get_sector_color(CustPieChart *pie_chart, gint sector_index, double *red, double *green, double *blue)
{
    CustPieChartPrivate *priv;
    sector_t *sec;

    g_assert(pie_chart != NULL && red != NULL && green != NULL && blue != NULL);

    priv = CUST_PIE_CHART_GET_PRIVATE(pie_chart);
    
    g_assert(sector_index >= 0 && sector_index < priv->n_sectors);

    sec = priv->sectors + sector_index;
    *red = sec->red;
    *green = sec->green;
    *blue = sec->blue;
}

void
cust_pie_chart_set_sector_name(CustPieChart *pie_chart, gint sector_index, const gchar *name)
{
    gchar *old_name;
    CustPieChartPrivate *priv;

    g_assert(pie_chart != NULL && name != NULL);

    priv = CUST_PIE_CHART_GET_PRIVATE(pie_chart);
    
    g_assert(sector_index >= 0 && sector_index < priv->n_sectors);

    old_name = priv->sectors[sector_index].name;
    priv->sectors[sector_index].name = g_strdup(name);
    g_free(old_name);

    cust_pie_chart_update(pie_chart);
}

void
cust_pie_chart_set_sector_value(CustPieChart *pie_chart, gint sector_index, double value)
{
    CustPieChartPrivate *priv;

    g_assert(pie_chart != NULL && value >= 0.0);

    priv = CUST_PIE_CHART_GET_PRIVATE(pie_chart);
    
    g_assert(sector_index >= 0 && sector_index < priv->n_sectors);

    priv->sectors[sector_index].value = value;
    cust_pie_chart_update(pie_chart);
}

void
cust_pie_chart_set_sector_color(CustPieChart *pie_chart, gint sector_index, double red, double green, double blue)
{
    CustPieChartPrivate *priv;
    sector_t *sec;

    g_assert(pie_chart != NULL && red >= 0.0 && green >= 0.0 && blue >= 0.0 && red <= 1.0 && green <= 1.0 && blue <= 1.0);

    priv = CUST_PIE_CHART_GET_PRIVATE(pie_chart);
    
    g_assert(sector_index >= 0 && sector_index < priv->n_sectors);

    sec = priv->sectors + sector_index;
    sec->red = red;
    sec->green = green;
    sec->blue = blue;
    cust_pie_chart_update(pie_chart);
}

/* vim: sts=4:shiftwidth=4:expandtab:cindent:cinoptions=\:0
*/
