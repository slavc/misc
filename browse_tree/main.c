#include <sys/types.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gtk/gtk.h>

#include "tree.h"
#include "xmem.h"

#define DELIM		"."
#define MAX_LINE	8192

static GtkWidget	*create_left_pane(const char *filename);
static GtkWidget	*create_right_pane(void);
static void		 cb_expand_clicked(GtkWidget *widget, gpointer data);
static void		 cb_collapse_clicked(GtkWidget *widget, gpointer data);
static GtkWidget	*create_view_and_model(const char *filename);
static void		 cb_save_clicked(GtkWidget *widget, gpointer user_data);
static void		 cb_entry_changed(GtkWidget *widget, gpointer user_data);
static gboolean		 cb_is_visible(GtkTreeModel *model, GtkTreeIter *iter, gpointer data);
static void              cb_tree_selection_changed(GtkTreeSelection *sel, gpointer data);
static GtkTreeModel	*parse(const char *filename);

static void		 chop_space(char *s);
static void		 tree_clear_flags(TreeNode *node, enum TreeNodeFlags flags);
static void		 tree_set_flags(TreeNode *node, enum TreeNodeFlags flags);
static void		 update_visibility(TreeNode *node, const char *pattern);

static char		*filter_pattern;

int
main(int argc, char **argv)
{
	GtkWidget	*window;
	GtkWidget	*paned;
	GtkWidget	*left;
	GtkWidget	*right;

	if (argc < 2)
		return 1;

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size (GTK_WINDOW(window), 640, 480);
	g_signal_connect(window, "delete_event", gtk_main_quit, NULL);

	paned = gtk_hpaned_new();
	left  = create_left_pane(argv[1]);
	right = create_right_pane();

	gtk_paned_add1(GTK_PANED(paned), left);
	gtk_paned_add2(GTK_PANED(paned), right);
	gtk_container_add(GTK_CONTAINER(window), paned);

	gtk_widget_show_all(window);

	gtk_main();

	return 0;
}

static GtkWidget *
create_left_pane(const char *filename)
{
	GtkWidget	*box;
	GtkWidget	*entry;
	GtkWidget	*swin;
	GtkWidget	*view;
	GtkWidget	*bbox;
	GtkWidget	*expand;
	GtkWidget	*collapse;

	box      = gtk_vbox_new(FALSE, FALSE);
	entry    = gtk_entry_new();
	swin     = gtk_scrolled_window_new(NULL, NULL);
	view     = create_view_and_model(filename);
	bbox     = gtk_hbutton_box_new();
	expand   = gtk_button_new_with_label("Expand");
	collapse = gtk_button_new_with_label("Collapse");

	gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), swin,   TRUE,  TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), bbox,  FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(swin), view);
	gtk_container_add(GTK_CONTAINER(bbox), expand);
	gtk_container_add(GTK_CONTAINER(bbox), collapse);

	g_signal_connect(entry,    "changed", G_CALLBACK(cb_entry_changed),    view);
	g_signal_connect(expand,   "clicked", G_CALLBACK(cb_expand_clicked),   NULL);
	g_signal_connect(collapse, "clicked", G_CALLBACK(cb_collapse_clicked), NULL);

	return box;
}

static GtkWidget *
create_right_pane(void)
{
	GtkWidget	*box;
	GtkWidget	*lblpath;
	GtkWidget	*path;
	GtkWidget	*lbldescr;
	GtkWidget	*descr;

	box      = gtk_vbox_new(FALSE, 0);
	lblpath  = gtk_label_new("Path");
	path     = gtk_entry_new();
	lbldescr = gtk_label_new("Description");
	descr    = gtk_text_view_new();

	gtk_box_pack_start(GTK_BOX(box), lblpath,  FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), path,     FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), lbldescr, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), descr,    TRUE,  TRUE,  0);

	return box;
}

static void
cb_expand_clicked(GtkWidget *widget, gpointer data)
{
	printf("expand\n");
}

static void
cb_collapse_clicked(GtkWidget *widget, gpointer data)
{
	printf("collapse\n");
}

static GtkWidget *
create_view_and_model(const char *filename)
{
	GtkTreeViewColumn	*col;
	GtkCellRenderer		*renderer;
	GtkWidget		*view;
	GtkTreeModel		*model;
	GtkTreeSelection	*sel;

	model = parse(filename);
	view = gtk_tree_view_new_with_model(model);

	g_object_unref(model); /* destroy store automatically with view */

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new();

	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", TREE_COL_DATA);
	gtk_tree_view_column_set_title (col, filename);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	gtk_tree_selection_set_mode(sel, GTK_SELECTION_MULTIPLE);

	g_signal_connect(sel, "changed", G_CALLBACK(cb_tree_selection_changed), NULL);

	return view;
}

static void
cb_tree_selection_changed(GtkTreeSelection *sel, gpointer data)
{
}

static void
cb_save_clicked(GtkWidget *widget, gpointer user_data)
{
	g_print("save clicked\n");
}

static void
cb_entry_changed(GtkWidget *widget, gpointer data)
{
	GtkTreeModel	*model;
	Tree		*tree;

	if (filter_pattern != NULL)
		free(filter_pattern);
	filter_pattern = xstrdup(gtk_entry_get_text(GTK_ENTRY(widget)));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data));
	tree = (Tree *) gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(model));
	if (*filter_pattern == '\0') {
		tree_set_flags(tree->root, TREE_NODE_VISIBLE);
	} else {
		tree_clear_flags(tree->root, ~0);
		update_visibility(tree->root, filter_pattern);
	}
	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(model));
}

static GtkTreeModel *
parse(const char *filename)
{
	FILE		*fp;
	char		 buf[MAX_LINE];
	const size_t	 bufsize = sizeof buf;
	char		*tok;
        Tree		*tree;
	TreeNode	*parent, *child;
	GtkTreeModel	*filter;

	fp = fopen(filename, "r");
	if (fp == NULL)
		err(1, "%s", filename);

        tree = tree_new();
	while (fgets(buf, bufsize, fp) != NULL) {
		chop_space(buf);
		if (*buf == '\0') /* skip empty lines */
			continue;
		parent = tree->root;
		tok = strtok(buf, DELIM);
		while (tok != NULL) {
			child = tree_vivify_child(parent, tok);
			parent = child;
			tok = strtok(NULL, DELIM);
		}
	}
	fclose(fp);
	tree_set_parents(tree->root);

	filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(tree), NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filter), cb_is_visible, tree, NULL);

	return (GtkTreeModel *) filter;
}

static void
tree_clear_flags(TreeNode *node, enum TreeNodeFlags flags)
{
	size_t	 i;

	for (i = 0; i < node->nchildren; ++i)
		tree_clear_flags(node->children + i, flags);
	node->flags &= ~flags;
}

static void
tree_set_flags(TreeNode *node, enum TreeNodeFlags flags)
{
	size_t	 i;

	node->flags |= flags;
	for (i = 0; i < node->nchildren; ++i)
		tree_set_flags(node->children + i, flags);
}

static void
update_visibility(TreeNode *node, const char *pattern)
{
	TreeNode	*parent;
	size_t		 i;

	if (node->flags & TREE_NODE_WALKED)
		return;
	node->flags |= TREE_NODE_WALKED;
	if (strstr(node->data, pattern) != NULL) {
		for (parent = node->parent; parent != NULL; parent = parent->parent)
			parent->flags |= TREE_NODE_WALKED | TREE_NODE_VISIBLE;
		tree_set_flags(node, TREE_NODE_WALKED | TREE_NODE_VISIBLE);
		return;
	}
	for (i = 0; i < node->nchildren; ++i)
		update_visibility(node->children + i, pattern);
}

static gboolean
cb_is_visible(GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	TreeNode	*node;

	node = iter->user_data;
	if (node->flags & TREE_NODE_VISIBLE)
		return TRUE;
	else
		return FALSE;
}

static void
chop_space(char *s)
{
	char *p;
	p = strchr(s, '\0') - 1;
	while (p >= s && isspace(*p))
		*p++ = '\0';
}

