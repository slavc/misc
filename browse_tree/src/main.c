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
#include <libgen.h>
#include <zlib.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "tree.h"
#include "descr.h"
#include "util.h"
#include "xmem.h"
#include "str.h"
#include "file.h"

#define DELIM		"."

struct node_expand_arg {
	gboolean	do_expand;
	gboolean	expand_all;
};

static char		*current_filename;
static TreeNode		*current_node;
static GtkWidget	*descr_view;
static char		*filter_pattern;
static int		 only_show_nodes_with_descriptions_enabled;
static GtkWidget	*path_entry;
const char		*program_name;
static int		 search_by_descriptions_enabled;
static GtkTreeModel	*tree_model;
static GtkTreeModel	*tree_model_filter;
static GtkWidget	*tree_view;
static GtkWidget	*window;
static int		 unsaved_changes_exist;

static const char	*base_name(const char *path);
static void		 cb_collapse_clicked(GtkWidget *widget, gpointer data);
static gboolean		 cb_descr_view_focus_out(GtkWidget *descr_view, GdkEvent *event, gpointer data);
static void		 cb_entry_changed(GtkWidget *widget, gpointer user_data);
static void		 cb_expand_clicked(GtkWidget *widget, gpointer data);
static void		 cb_expand_fully_clicked(GtkWidget *widget, gpointer data);
static gboolean		 cb_is_visible(GtkTreeModel *model, GtkTreeIter *iter, gpointer data);
static void		 cb_only_show_nodes_with_descrs_toggled(GtkWidget *widget, gpointer data);
static void		 cb_row_activated(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *col, gpointer data);
static void		 cb_save_clicked(GtkWidget *widget, gpointer user_data);
static void		 cb_search_descriptions_toggled(GtkWidget *widget, gpointer data);
static void		 cb_set_node_expansion(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data);
static void		 cb_tree_selection_changed(GtkTreeSelection *sel, gpointer data);
static gboolean		 cb_window_delete(GtkWidget *widget, GdkEvent *event, gpointer data);
static void		 cb_window_destroy(GtkWidget *widget, gpointer data);
static void		 chop_space(char *s);
static GtkWidget	*create_left_pane(void);
static GtkWidget	*create_menu_bar(void);
static GtkWidget	*create_right_pane(void);
static GtkWidget	*create_toolbar(void);
static GtkWidget	*create_view_and_model(void);
static void		 display_descr(void);
static void		 display_path(void);
static void		 filter_nodes(void);
static char		*fmt_node_path(TreeNode *node);
static char		*get_descr(void);
static int		 load_file(const char *filename);
static int		 save_file(const char *filename);
static void		 store_descr(void);
static void		 tree_clear_flags(TreeNode *node, enum TreeNodeFlags flags);
static void		 tree_set_flags(TreeNode *node, enum TreeNodeFlags flags);
static void		 update_title(void);
static void		 update_visibility(TreeNode *node, const char *pattern);
static void		 usage(void);

int
main(int argc, char **argv)
{
	GtkWidget	*vbox;
	GtkWidget	*toolbar;
	GtkWidget	*paned;
	GtkWidget	*left;
	GtkWidget	*right;
	GtkWidget	*menu_bar;
	int		 ch;

	while ((ch = getopt(argc, argv, "h")) != -1) {
		switch (ch) {
		case 'h':
			usage();
			return 0;
		case '?':
			usage();
			return 1;
		}
	}

	program_name = base_name(argv[0]);
	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);
	g_signal_connect(window, "delete_event", G_CALLBACK(cb_window_delete), NULL);
	g_signal_connect(window, "destroy", G_CALLBACK(cb_window_destroy), NULL);

	vbox = gtk_vbox_new(FALSE, 0);
	toolbar = create_toolbar();
	paned = gtk_hpaned_new();
	left = create_left_pane();
	right = create_right_pane();
	menu_bar = create_menu_bar();

	gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), paned, TRUE, TRUE, 0);
	gtk_paned_add1(GTK_PANED(paned), left);
	gtk_paned_add2(GTK_PANED(paned), right);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	gtk_widget_show_all(window);

	if (argc > 1) {
		if (load_file(argv[1]) != 0)
			g_warning("%s: failed to load file", argv[1]);
	}

	update_title();

	gtk_main();

	return 0;
}

static gboolean
cb_window_delete(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	GtkWidget	*dialog;
	gint		 response;

	store_descr();
	if (unsaved_changes_exist) {
		dialog = gtk_dialog_new_with_buttons("Save changes?", GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_YES, GTK_RESPONSE_ACCEPT, GTK_STOCK_NO, GTK_RESPONSE_REJECT, NULL);
		response = gtk_dialog_run(GTK_DIALOG(dialog));
		if (response == GTK_RESPONSE_ACCEPT)
			save_file(current_filename);
	}
	
	return FALSE;
}

static void
cb_window_destroy(GtkWidget *widget, gpointer data)
{
	gtk_main_quit();
}

static const char *
base_name(const char *path)
{
	const char	*p;

	p = strrchr(path, '/');
	if (p == NULL)
		return path;
	else
		return p + 1;
}

static void
update_title(void)
{
	char	 buf[1024];

	if (current_filename == NULL) {
		gtk_window_set_title(GTK_WINDOW(window), program_name);
	} else {
		if (snprintf(buf, sizeof buf, "%s: %s", program_name, base_name(current_filename)) > 0)
			gtk_window_set_title(GTK_WINDOW(window), buf);
		else
			warn("snprintf");
	}
}

static void
usage(void)
{
	g_print("usage: browse_tree [<filename>]\n");
}

static GtkWidget *
create_left_pane(void)
{
	GtkWidget	*box;
	GtkWidget	*vbox;
	GtkWidget	*entry;
	GtkWidget	*check1;
	GtkWidget	*check2;
	GtkWidget	*swin;
	GtkWidget	*bbox;
	GtkWidget	*expand_fully;
	GtkWidget	*expand;
	GtkWidget	*collapse;

	box          = gtk_vbox_new(FALSE, FALSE);
	vbox         = gtk_vbox_new(FALSE, FALSE);
	check1       = gtk_check_button_new_with_label("Search descriptions");
	check2       = gtk_check_button_new_with_label("Only show nodes with descriptions");
	entry        = gtk_entry_new();
	swin         = gtk_scrolled_window_new(NULL, NULL);
	tree_view    = create_view_and_model();
	bbox         = gtk_hbutton_box_new();
	expand_fully = gtk_button_new_with_label("Expand fully");
	expand       = gtk_button_new_with_label("Expand");
	collapse     = gtk_button_new_with_label("Collapse");

	gtk_box_pack_start(GTK_BOX(vbox), entry,  FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), check1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), check2, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box),  vbox,   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box),  swin,   TRUE,  TRUE,  0);
	gtk_box_pack_start(GTK_BOX(box),  bbox,   FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(swin), tree_view);
	gtk_container_add(GTK_CONTAINER(bbox), expand);
	gtk_container_add(GTK_CONTAINER(bbox), expand_fully);
	gtk_container_add(GTK_CONTAINER(bbox), collapse);

	g_signal_connect(entry,        "changed",       G_CALLBACK(cb_entry_changed),                       tree_view);
	g_signal_connect(expand,       "clicked",       G_CALLBACK(cb_expand_clicked),                      NULL);
	g_signal_connect(expand_fully, "clicked",       G_CALLBACK(cb_expand_fully_clicked),                NULL);
	g_signal_connect(collapse,     "clicked",       G_CALLBACK(cb_collapse_clicked),                    NULL);
	g_signal_connect(check1,       "toggled",       G_CALLBACK(cb_search_descriptions_toggled),         NULL);
	g_signal_connect(check2,       "toggled",       G_CALLBACK(cb_only_show_nodes_with_descrs_toggled), NULL);
	g_signal_connect(tree_view,    "row-activated", G_CALLBACK(cb_row_activated),                       NULL);

	gtk_entry_set_icon_from_stock(GTK_ENTRY(entry), GTK_ENTRY_ICON_PRIMARY, GTK_STOCK_FIND);

	return box;
}

static void
cb_row_activated(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *col, gpointer data)
{
	if (gtk_tree_view_row_expanded(view, path))
		gtk_tree_view_collapse_row(view, path);
	else
		gtk_tree_view_expand_row(view, path, FALSE);
}

static void
cb_only_show_nodes_with_descrs_toggled(GtkWidget *widget, gpointer data)
{
	only_show_nodes_with_descriptions_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	filter_nodes();
}

static void
cb_search_descriptions_toggled(GtkWidget *widget, gpointer data)
{
	search_by_descriptions_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	filter_nodes();
}

static void
create_tree_model(GtkTreeModel **model, GtkTreeModel **model_filter)
{
	*model = (GtkTreeModel *) tree_new();
	*model_filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(*model), NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(*model_filter), cb_is_visible, *model, NULL);
}

static int
load_file(const char *filename)
{
	f_file_t	 f;
	char		*buf;
	char		*path;
	char		*descr;
	char		*tok;
	TreeNode	*parent;
	TreeNode	*child;

	unsaved_changes_exist = 0;
	f = f_open(filename, "r");
	if (f == NULL)
		return -1;
	current_node = NULL;
	create_tree_model(&tree_model, &tree_model_filter);
	while ((buf = f_gets(f)) != NULL) {
		chop_space(buf);
		if (*buf == '\0') /* skip empty lines */
			continue;
		path = buf;
		descr = strchr(buf, ' ');
		if (descr != NULL)
			*descr++ = '\0';
		parent = TREE(tree_model)->root;
		child = NULL;
		tok = strtok(path, DELIM);
		while (tok != NULL) {
			child = tree_vivify_child(parent, tok);
			parent = child;
			tok = strtok(NULL, DELIM);
		}
		if (child != NULL) {
			if (descr != NULL && *descr != '\0')
				child->descr = descr_decode(descr);
			else
				child->descr = NULL;
		}
		xfree(buf);
	}
	f_close(f);
	tree_set_parents(TREE(tree_model)->root);
	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(tree_model_filter));
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree_view), GTK_TREE_MODEL(tree_model_filter));
	g_object_unref(tree_model);
	g_object_unref(tree_model_filter);

	free(current_filename);
	current_filename = xstrdup(filename);

	return 0;
}

static GtkWidget *
create_right_pane(void)
{
	GtkWidget	*box;
	GtkWidget	*hbox1;
	GtkWidget	*hbox2;
	GtkWidget	*lblpath;
	GtkWidget	*lbldescr;

	box        = gtk_vbox_new(FALSE, 0);
	hbox1      = gtk_hbox_new(FALSE, 0);
	hbox2      = gtk_hbox_new(FALSE, 0);
	lblpath    = gtk_label_new("Path");
	path_entry = gtk_entry_new();
	lbldescr   = gtk_label_new("Description");
	descr_view = gtk_text_view_new();

	gtk_box_pack_start(GTK_BOX(hbox1), lblpath, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox2), lbldescr, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(box), hbox1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), path_entry, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), hbox2, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), descr_view, TRUE, TRUE,  0);

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

	modelf = gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view));
	model = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(modelf));
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	list = gtk_tree_selection_get_selected_rows(sel, NULL);
	if (list == NULL)
		return NULL;
	first = g_list_first(list);
	path = first->data;

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
display_descr(void)
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
	if (*tmp == '\0') {
		g_free(tmp);
		return NULL;
	} else
		return tmp;
}

static void
store_descr(void)
{
	char	*descr;

	if (current_node == NULL)
		return;
	descr = get_descr();
	if ((descr != NULL && current_node->descr != NULL && strcmp(descr, current_node->descr) != 0) || ((descr == NULL) ^ (current_node->descr == NULL)))
		unsaved_changes_exist = 1;
	xfree(current_node->descr);
	current_node->descr = descr;
}

static void
cb_expand_fully_clicked(GtkWidget *widget, gpointer data)
{
	GtkTreeSelection	*sel;
	struct node_expand_arg	 arg = {
		TRUE,
		TRUE,
	};

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	gtk_tree_selection_selected_foreach(sel, cb_set_node_expansion, &arg);
}

static void
cb_expand_clicked(GtkWidget *widget, gpointer data)
{
	GtkTreeSelection	*sel;
	struct node_expand_arg	 arg = {
		TRUE,
		FALSE,
	};

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	gtk_tree_selection_selected_foreach(sel, cb_set_node_expansion, &arg);
}

static void
cb_collapse_clicked(GtkWidget *widget, gpointer data)
{
	GtkTreeSelection	*sel;
	struct node_expand_arg	 arg = {
		FALSE,
		FALSE,
	};

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	gtk_tree_selection_selected_foreach(sel, cb_set_node_expansion, &arg);
}

static void
cb_set_node_expansion(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	struct node_expand_arg	*arg = data;

	if (arg->do_expand)
		gtk_tree_view_expand_row(GTK_TREE_VIEW(tree_view), path, arg->expand_all);
	else
		gtk_tree_view_collapse_row(GTK_TREE_VIEW(tree_view), path);
}

static GtkWidget *
create_view_and_model(void)
{
	GtkTreeViewColumn	*col;
	GtkCellRenderer		*renderer;
	GtkTreeSelection	*sel;
	GtkWidget		*view;

	create_tree_model(&tree_model, &tree_model_filter);
	view = gtk_tree_view_new_with_model(tree_model_filter);
	g_object_unref(tree_model);
	g_object_unref(tree_model_filter);
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new();
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));

	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", TREE_COL_DATA);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);
	gtk_tree_selection_set_mode(sel, GTK_SELECTION_MULTIPLE);

	g_signal_connect(sel, "changed", G_CALLBACK(cb_tree_selection_changed), NULL);

	return view;
}

static void
cb_tree_selection_changed(GtkTreeSelection *sel, gpointer data)
{
	store_descr();
	current_node = get_first_selected_node();
	display_descr();
	display_path();
}

static void
display_path(void)
{
	char		*path;

	if (current_node == NULL) {
		gtk_entry_set_text(GTK_ENTRY(path_entry), "");
		return;
	}

	path = fmt_node_path(current_node);
	gtk_entry_set_text(GTK_ENTRY(path_entry), path);
	xfree(path);
}

static char *
fmt_node_path(TreeNode *node)
{
	int	 i;
	char	*s = NULL;

	for (i = 0; node != NULL && node->parent != NULL; ++i, node = node->parent)
		s = str_prepend(s, i == 0 ? "%s" : "%s.", node->data);
	return s;
}

static gboolean
cb_write_node_to_file(GtkTreeModel *model, GtkTreePath *treepath, GtkTreeIter *iter, gpointer data)
{
	f_file_t	 f = data;
	TreeNode	*node = iter->user_data;
	char		*path = NULL;
	char		*descr = NULL;

	path = fmt_node_path(node);
	f_write(f, path, strlen(path));
	if (node->descr != NULL) {
		descr = descr_encode(node->descr);
		f_write(f, " ", 1);
		f_write(f, descr, strlen(descr));
	}
	f_write(f, "\n", 1);
	xfree(path);
	xfree(descr);
	return FALSE;
}

static int
save_file(const char *filename)
{
	f_file_t	 f;

        if (filename == NULL)
            return -1;

	f = f_open(filename, "w");
	if (f == NULL)
		return -1;
	gtk_tree_model_foreach(GTK_TREE_MODEL(tree_model), cb_write_node_to_file, f);
	f_close(f);
	return 0;
}

static void
cb_save_clicked(GtkWidget *unused1, gpointer unused2)
{
	GtkWidget	*dialog;
	char		*filename;

	if (current_filename == NULL)
		return;
	store_descr();
	save_file(current_filename);
	unsaved_changes_exist = 0;
}

static void
cb_entry_changed(GtkWidget *widget, gpointer data)
{
	if (filter_pattern != NULL)
		free(filter_pattern);
	filter_pattern = xstrdup(gtk_entry_get_text(GTK_ENTRY(widget)));
	filter_nodes();
}

static void
filter_nodes(void)
{
	tree_clear_flags(TREE(tree_model)->root, ~0);
	update_visibility(TREE(tree_model)->root, filter_pattern);
	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(tree_model_filter));
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

	node->flags |= TREE_NODE_WALKED;
	if ((pattern == NULL ||
	    strstr(node->data, pattern) != NULL ||
	    (search_by_descriptions_enabled && node->descr != NULL && strstr(node->descr, pattern) != NULL)
	    ) &&
	    (!only_show_nodes_with_descriptions_enabled || node->descr != NULL)) {
		for (parent = node->parent; parent != NULL; parent = parent->parent)
			parent->flags |= TREE_NODE_VISIBLE;
		tree_set_flags(node, TREE_NODE_VISIBLE);
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


static void
cb_open_clicked(GtkWidget *widget, gpointer data)
{
	GtkWidget	*dialog;
	char		*filename;

	dialog = gtk_file_chooser_dialog_new("Open File", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		if (load_file(filename) == 0)
			update_title();
		g_free(filename);
	}
	gtk_widget_destroy(dialog);
}

static GtkWidget *
create_toolbar(void)
{
	GtkWidget	*toolbar;
	GtkToolItem	*open_btn;
	GtkToolItem	*save_btn;

	toolbar = gtk_toolbar_new();
	open_btn = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
	save_btn = gtk_tool_button_new_from_stock(GTK_STOCK_SAVE);

	gtk_toolbar_set_orientation (GTK_TOOLBAR(toolbar), GTK_ORIENTATION_HORIZONTAL);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_tooltips(GTK_TOOLBAR(toolbar), TRUE);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), open_btn, -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), save_btn, -1);

	g_signal_connect(GTK_WIDGET(open_btn), "clicked", G_CALLBACK(cb_open_clicked), NULL);
	g_signal_connect(GTK_WIDGET(save_btn), "clicked", G_CALLBACK(cb_save_clicked), NULL);

	return toolbar;
}

static void
cb_about_item_activate(void)
{
	GtkWidget	*dialog;

	dialog = gtk_about_dialog_new();
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), program_name);
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), "This program loads and displays a tree from a file. "
		"The tree must be specified in dot notation, one node per line, similar to sysctls. "
		"You can browse and search the tree, as well as leave notes about particular nodes.");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static GtkWidget *
create_menu_bar(void)
{
	GtkWidget	*menu_bar;
	GtkWidget	*file_menu;
	GtkWidget	*help_menu;
	GtkWidget	*file_item;
	GtkWidget	*help_item;
	GtkWidget	*open_item;
	GtkWidget	*save_item;
	GtkWidget	*quit_item;
	GtkWidget	*about_item;

	menu_bar = gtk_menu_bar_new();

	file_menu = gtk_menu_new();
	open_item = gtk_menu_item_new_with_label("Open");
	save_item = gtk_menu_item_new_with_label("Save");
	quit_item = gtk_menu_item_new_with_label("Quit");
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), save_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), quit_item);
	file_item = gtk_menu_item_new_with_label("File");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);
	gtk_menu_bar_append(GTK_MENU_BAR(menu_bar), file_item);

	help_menu = gtk_menu_new();
	about_item = gtk_menu_item_new_with_label("About");
	gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), about_item);
	help_item = gtk_menu_item_new_with_label("Help");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_item), help_menu);
	gtk_menu_bar_append(GTK_MENU_BAR(menu_bar), help_item);

	g_signal_connect(open_item, "activate", G_CALLBACK(cb_open_clicked), NULL);
	g_signal_connect(save_item, "activate", G_CALLBACK(cb_save_clicked), NULL);
	g_signal_connect(quit_item, "activate", G_CALLBACK(gtk_main_quit), NULL);

	g_signal_connect(about_item, "activate", G_CALLBACK(cb_about_item_activate), NULL);

	return menu_bar;
}

