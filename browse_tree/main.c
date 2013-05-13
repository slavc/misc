#include <sys/types.h>

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "tree.h"
#include "xmem.h"
#include "str.h"
#include "file.h"

#define DELIM		"."
#define MAX_LINE	8192

/*
 * "Model" -- the tree, "view" -- the graphical widgets which represent information.
 * load_*() functions  -- update the view with values from the model.
 * get_*() functions   -- retrieve the current value from the view.
 * store_*() functions -- update the model with current value from the view.
 */

static GtkWidget	*create_left_pane(const char *filename);
static GtkWidget	*create_right_pane(void);
static void		 cb_expand_clicked(GtkWidget *widget, gpointer data);
static void		 cb_collapse_clicked(GtkWidget *widget, gpointer data);
static GtkWidget	*create_view_and_model(const char *filename);
static void		 cb_save_clicked(GtkWidget *widget, gpointer user_data);
static void		 cb_entry_changed(GtkWidget *widget, gpointer user_data);
static gboolean		 cb_is_visible(GtkTreeModel *model, GtkTreeIter *iter, gpointer data);
static void              cb_tree_selection_changed(GtkTreeSelection *sel, gpointer data);
static gboolean		 cb_descr_view_focus_out(GtkWidget *descr_view, GdkEvent *event, gpointer data);
static GtkTreeModel	*parse(const char *filename);
static char		*decode_descr(char *s);
static void		 load_descr(void);
static char		*get_descr(void);
static void		 store_descr(void);
static void		 load_path(void);


static void		 chop_space(char *s);
static void		 tree_clear_flags(TreeNode *node, enum TreeNodeFlags flags);
static void		 tree_set_flags(TreeNode *node, enum TreeNodeFlags flags);
static void		 update_visibility(TreeNode *node, const char *pattern);

static char		*filter_pattern;

GtkWidget	*tree_view;
GtkWidget	*path_entry;
GtkWidget	*descr_view;

TreeNode	*current_node;

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
	GtkWidget	*bbox;
	GtkWidget	*expand;
	GtkWidget	*collapse;

	box       = gtk_vbox_new(FALSE, FALSE);
	entry     = gtk_entry_new();
	swin      = gtk_scrolled_window_new(NULL, NULL);
	tree_view = create_view_and_model(filename);
	bbox      = gtk_hbutton_box_new();
	expand    = gtk_button_new_with_label("Expand");
	collapse  = gtk_button_new_with_label("Collapse");

	gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), swin,  TRUE,  TRUE,  0);
	gtk_box_pack_start(GTK_BOX(box), bbox,  FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(swin), tree_view);
	gtk_container_add(GTK_CONTAINER(bbox), expand);
	gtk_container_add(GTK_CONTAINER(bbox), collapse);

	g_signal_connect(entry,    "changed", G_CALLBACK(cb_entry_changed),    tree_view);
	g_signal_connect(expand,   "clicked", G_CALLBACK(cb_expand_clicked),   NULL);
	g_signal_connect(collapse, "clicked", G_CALLBACK(cb_collapse_clicked), NULL);

	return box;
}

static GtkWidget *
create_right_pane(void)
{
	GtkWidget	*box;
	GtkWidget	*lblpath;
	GtkWidget	*lbldescr;

	box        = gtk_vbox_new(FALSE, 0);
	lblpath    = gtk_label_new("Path");
	path_entry = gtk_entry_new();
	lbldescr   = gtk_label_new("Description");
	descr_view = gtk_text_view_new();

	gtk_box_pack_start(GTK_BOX(box), lblpath,    FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), path_entry, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), lbldescr,   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), descr_view, TRUE,  TRUE,  0);

	g_signal_connect(descr_view, "focus-out-event", G_CALLBACK(cb_descr_view_focus_out), NULL);

	return box;
}

static TreeNode *
get_first_selected_node(void)
{
	GtkTreeModel		*modelf;
	GtkTreeModel		*model;
	GtkTreeSelection	*sel;
	GList			*list;
	GList			*first;
	GtkTreePath		*path;
	GtkTreeIter		 iter;
	TreeNode		*node;

	char			*tmp;

	modelf = gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view));
	model = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(modelf));
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	list = gtk_tree_selection_get_selected_rows(sel, NULL);
	if (list == NULL)
		return NULL;
	first = g_list_first(list);
	path = first->data;

	tmp = gtk_tree_path_to_string(path);
	g_free(tmp);

	if (tree_get_iter_visible(model, &iter, path))
		node = iter.user_data;
	else
		node = NULL;
	g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(list);
	return node;
}

static gboolean
cb_descr_view_focus_out(GtkWidget *descr_view, GdkEvent *event, gpointer data)
{
	store_descr();
	return FALSE;
}

static void
load_descr(void)
{
	GtkTextBuffer	*tbuf;

	tbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(descr_view));
	if (current_node == NULL || current_node->descr == NULL) {
		gtk_text_buffer_set_text(tbuf, "", -1);
		return;
	}
	gtk_text_buffer_set_text(tbuf, current_node->descr, -1);
}

static char *
get_descr(void)
{
	GtkTextBuffer	*buf;
	gchar		*tmp;
	GtkTextIter	 start;
	GtkTextIter	 end;

	buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(descr_view));
	gtk_text_buffer_get_start_iter(buf, &start);
	gtk_text_buffer_get_end_iter(buf, &end);
	tmp = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
	return tmp;
}

static void
store_descr(void)
{
	char	*descr;

	if (current_node == NULL)
		return;
	descr = get_descr();
	xfree(current_node->descr);
	current_node->descr = descr;
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
	store_descr();
	current_node = get_first_selected_node();
	load_descr();
	load_path();
}

static void
load_path(void)
{
	TreeNode	*node = current_node;
	int		 i;
	char		*s = NULL;

	if (current_node == NULL) {
		gtk_entry_set_text(GTK_ENTRY(path_entry), "");
		return;
	}

	for (i = 0; node != NULL && node->parent != NULL; ++i, node = node->parent) {
		if (i == 0)
			s = xstrdup(node->data);
		else
			s = str_prepend(s, "%s.", node->data);
	}
	gtk_entry_set_text(GTK_ENTRY(path_entry), s);
	xfree(s);
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
	char		*buf;
	char		*path;
	char		*descr;
	char		*tok;
        Tree		*tree;
	TreeNode	*parent, *child;
	GtkTreeModel	*filter;

	fp = fopen(filename, "r");
	if (fp == NULL)
		err(1, "%s", filename);

        tree = tree_new();
	while ((buf = fp_gets(fp)) != NULL) {
		chop_space(buf);
		if (*buf == '\0') /* skip empty lines */
			continue;
		path = buf;
		descr = strchr(buf, ' ');
		if (descr != NULL)
			*descr++ = '\0';
		parent = tree->root;
		tok = strtok(path, DELIM);
		while (tok != NULL) {
			child = tree_vivify_child(parent, tok);
			parent = child;
			tok = strtok(NULL, DELIM);
		}
		if (descr != NULL)
			child->descr = decode_descr(descr);
		else
			child->descr = NULL;
		free(buf);
	}
	fclose(fp);
	tree_set_parents(tree->root);

	filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(tree), NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filter), cb_is_visible, tree, NULL);

	return (GtkTreeModel *) filter;
}

static inline char
esc2char(char c)
{
	switch (c) {
	case 'n':
		return '\n';
	case 'r':
		return '\r';
	default:
		return c;
	}
}

static char *
decode_descr(char *s)
{
	char	*buf;
	char	*p, *q;
	size_t	 slen;

	slen = strlen(s);
	buf = xmalloc(slen + 1);

	s[slen - 1] = '\0'; // remove the last quote
	for (p = s + 1, q = buf; *p != '\0'; ++p, ++q) {
		if (*p == '\\') {
			++p;
			*q = esc2char(*p);
		} else {
			*q = *p;
		}
	}

	return buf;
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
	if (node == NULL || !(node->flags & TREE_NODE_VISIBLE))
		return FALSE;
	else
		return TRUE;
}

static void
chop_space(char *s)
{
	char *p;
	p = strchr(s, '\0') - 1;
	while (p >= s && isspace(*p))
		*p++ = '\0';
}

