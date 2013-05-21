#include <stdlib.h>
#include <string.h>

#include "tree.h"
#include "xmem.h"

#define INDENT_STR	"  "

/* boring declarations of local functions */

static void         tree_init(Tree *);
static void         tree_class_init(TreeClass *);
static void         tree_model_init(GtkTreeModelIface *);
static void         tree_finalize(GObject *);
static GtkTreeModelFlags tree_get_flags(GtkTreeModel *);
static gint         tree_get_n_columns(GtkTreeModel *);
static GType        tree_get_column_type(GtkTreeModel *, gint);
static gboolean     tree_get_iter(GtkTreeModel *, GtkTreeIter *, GtkTreePath *);
static GtkTreePath *tree_get_path(GtkTreeModel *, GtkTreeIter *);
static void         tree_get_value(GtkTreeModel *, GtkTreeIter *, gint, GValue *);
static gboolean     tree_iter_next(GtkTreeModel *, GtkTreeIter *);
static gboolean     tree_iter_children(GtkTreeModel *, GtkTreeIter *, GtkTreeIter *);
static gboolean     tree_iter_has_child(GtkTreeModel *, GtkTreeIter *);
static gint         tree_iter_n_children(GtkTreeModel *, GtkTreeIter *);
static gboolean     tree_iter_nth_child(GtkTreeModel *, GtkTreeIter *, GtkTreeIter *, gint);
static gboolean     tree_iter_parent(GtkTreeModel *, GtkTreeIter *, GtkTreeIter *);


static GObjectClass *parent_class = NULL;  /* GObject stuff - nothing to worry about */


/*****************************************************************************
 *
 *  custom_list_get_type: here we register our new type and its interfaces
 *                        with the type system. If you want to implement
 *                        additional interfaces like GtkTreeSortable, you
 *                        will need to do it here.
 *
 *****************************************************************************/

GType
tree_get_type(void)
{
  static GType TreeNodeype = 0;

  /* Some boilerplate type registration stuff */
  if (TreeNodeype == 0)
  {
    static const GTypeInfo tree_info =
    {
      sizeof (TreeClass),
      NULL,                                         /* base_init */
      NULL,                                         /* base_finalize */
      (GClassInitFunc) tree_class_init,
      NULL,                                         /* class finalize */
      NULL,                                         /* class_data */
      sizeof (Tree),
      0,                                           /* n_preallocs */
      (GInstanceInitFunc) tree_init
    };
    static const GInterfaceInfo tree_model_info =
    {
      (GInterfaceInitFunc) tree_model_init,
      NULL,
      NULL
    };

    /* First register the new derived type with the GObject type system */
    TreeNodeype = g_type_register_static (G_TYPE_OBJECT, "Tree",
                                               &tree_info, (GTypeFlags)0);

    /* Now register our GtkTreeModel interface with the type system */
    g_type_add_interface_static (TreeNodeype, GTK_TYPE_TREE_MODEL, &tree_model_info);
  }

  return TreeNodeype;
}


/*****************************************************************************
 *
 *  custom_list_class_init: more boilerplate GObject/GType stuff.
 *                          Init callback for the type system,
 *                          called once when our new class is created.
 *
 *****************************************************************************/

static void
tree_class_init(TreeClass *klass)
{
  GObjectClass *object_class;

  parent_class = (GObjectClass*) g_type_class_peek_parent (klass);
  object_class = (GObjectClass*) klass;

  object_class->finalize = tree_finalize;
}

/*****************************************************************************
 *
 *  custom_list_tree_model_init: init callback for the interface registration
 *                               in custom_list_get_type. Here we override
 *                               the GtkTreeModel interface functions that
 *                               we implement.
 *
 *****************************************************************************/

static void
tree_model_init(GtkTreeModelIface *iface)
{
  iface->get_flags       = tree_get_flags;
  iface->get_n_columns   = tree_get_n_columns;
  iface->get_column_type = tree_get_column_type;
  iface->get_iter        = tree_get_iter;
  iface->get_path        = tree_get_path;
  iface->get_value       = tree_get_value;
  iface->iter_next       = tree_iter_next;
  iface->iter_children   = tree_iter_children;
  iface->iter_has_child  = tree_iter_has_child;
  iface->iter_n_children = tree_iter_n_children;
  iface->iter_nth_child  = tree_iter_nth_child;
  iface->iter_parent     = tree_iter_parent;
}


/*****************************************************************************
 *
 *  custom_list_init: this is called everytime a new custom list object
 *                    instance is created (we do that in custom_list_new).
 *                    Initialise the list structure's fields here.
 *
 *****************************************************************************/

static void
tree_init(Tree *t)
{
	t->root = xcalloc(1, sizeof(TreeNode));
	t->root->data = xstrdup("root");
	t->stamp = g_random_int();  /* Random int to check whether an iter belongs to our model */
}


/*****************************************************************************
 *
 *  custom_list_finalize: this is called just before a custom list is
 *                        destroyed. Free dynamically allocated memory here.
 *
 *****************************************************************************/

/* Note that it doesn't free the root TreeNode */
static void
tree_free(TreeNode *node)
{
	size_t	 i;

	for (i = 0; i < node->nchildren; ++i)
		tree_free(node->children + i);
	free(node->data);
	free(node->descr);
	free(node->children);
}

static void
tree_finalize(GObject *object)
{
	Tree	*tree;

	tree = TREE(object);
/*  CustomList *custom_list = CUSTOM_LIST(object); */

  /* free all records and free all memory used by the list */
	tree_free(tree->root);
	free(tree->root);


  /* must chain up - finalize parent */
  (*parent_class->finalize) (object);
}


/*****************************************************************************
 *
 *  custom_list_get_flags: tells the rest of the world whether our tree model
 *                         has any special characteristics. In our case,
 *                         we have a list model (instead of a tree), and each
 *                         tree iter is valid as long as the row in question
 *                         exists, as it only contains a pointer to our struct.
 *
 *****************************************************************************/

static GtkTreeModelFlags
tree_get_flags(GtkTreeModel *model)
{
  g_return_val_if_fail (IS_TREE(model), (GtkTreeModelFlags)0);

  return 0;
}


/*****************************************************************************
 *
 *  custom_list_get_n_columns: tells the rest of the world how many data
 *                             columns we export via the tree model interface
 *
 *****************************************************************************/

static gint
tree_get_n_columns(GtkTreeModel *model)
{
  g_return_val_if_fail (IS_TREE(model), 0);

  return TREE_N_COLUMNS;
}


/*****************************************************************************
 *
 *  custom_list_get_column_type: tells the rest of the world which type of
 *                               data an exported model column contains
 *
 *****************************************************************************/

static GType
tree_get_column_type(GtkTreeModel *model, gint index)
{
  g_return_val_if_fail (IS_TREE(model), G_TYPE_INVALID);
  g_return_val_if_fail (index == 0, G_TYPE_INVALID);

  return G_TYPE_STRING;
}


/*****************************************************************************
 *
 *  custom_list_get_iter: converts a tree path (physical position) into a
 *                        tree iter structure (the content of the iter
 *                        fields will only be used internally by our model).
 *                        We simply store a pointer to our CustomRecord
 *                        structure that represents that row in the tree iter.
 *
 *****************************************************************************/

static gboolean
tree_get_iter(GtkTreeModel *model, GtkTreeIter *iter, GtkTreePath *path)
{
	TreeNode	*node;
	gint		 depth;
	gint		*idx;

	g_assert(IS_TREE(model));
	g_assert(path != NULL);
	g_assert(iter != NULL);

	node = TREE(model)->root;
	idx = gtk_tree_path_get_indices(path);
	depth = gtk_tree_path_get_depth(path);
	while (depth--) {
		if (*idx >= node->nchildren)
			return FALSE;
		node = node->children + *idx++;
	}
	iter->stamp = TREE(model)->stamp;
	iter->user_data = node;
	return TRUE;
}


/*****************************************************************************
 *
 *  custom_list_get_path: converts a tree iter into a tree path (ie. the
 *                        physical position of that row in the list).
 *
 *****************************************************************************/

static GtkTreePath *
tree_get_path(GtkTreeModel *model, GtkTreeIter *iter)
{
	TreeNode	*t, *children;
	size_t		 i, nchildren;
	GtkTreePath	*path;

	g_return_val_if_fail (IS_TREE(model), NULL);
	g_return_val_if_fail (iter != NULL,               NULL);
	g_return_val_if_fail (iter->user_data != NULL,    NULL);

	t = iter->user_data;
	path = gtk_tree_path_new();

	while (t->parent != NULL) {
		nchildren = t->parent->nchildren;
		children = t->parent->children;
		for (i = 0; i < nchildren && t != (children + i); ++i)
			/* empty */;
		g_assert(i < nchildren);
		gtk_tree_path_prepend_index(path, i);
		t = t->parent;
	}

	return path;
}




/*****************************************************************************
 *
 *  custom_list_get_value: Returns a row's exported data columns
 *                         (_get_value is what gtk_tree_model_get uses)
 *
 *****************************************************************************/

static void
tree_get_value(GtkTreeModel *model, GtkTreeIter *iter, gint column, GValue *value)
{
	TreeNode	*t;

	g_return_if_fail (IS_TREE (model));
	g_return_if_fail (iter != NULL);
	g_return_if_fail (column == 0);

	g_value_init (value, G_TYPE_STRING);

	t = iter->user_data;

	g_return_if_fail ( t != NULL );

	g_value_set_string(value, t->data);
}


/*****************************************************************************
 *
 *  custom_list_iter_next: Takes an iter structure and sets it to point
 *                         to the next row.
 *
 *****************************************************************************/

static gboolean
tree_iter_next(GtkTreeModel *model, GtkTreeIter *iter)
{
	TreeNode	*t, *next, *parent;

	g_return_val_if_fail (IS_TREE (model), FALSE);

	t = iter->user_data;
	parent = t->parent;
	if (parent == NULL)
		return FALSE;
	next = t + 1;
	if (next < (parent->children + parent->nchildren)) {
		iter->user_data = next;
		iter->stamp = TREE(model)->stamp;
		return TRUE;
	} else
		return FALSE;
}


/*****************************************************************************
 *
 *  custom_list_iter_children: Returns TRUE or FALSE depending on whether
 *                             the row specified by 'parent' has any children.
 *                             If it has children, then 'iter' is set to
 *                             point to the first child. Special case: if
 *                             'parent' is NULL, then the first top-level
 *                             row should be returned if it exists.
 *
 *****************************************************************************/

static gboolean
tree_iter_children(GtkTreeModel *model, GtkTreeIter *iter, GtkTreeIter *parent_iter)
{
	Tree	*tree;
	TreeNode	*parent;

	tree = TREE(model);

	if (parent_iter == NULL)
		parent = tree->root;
	else
		parent = parent_iter->user_data;

	if (parent->nchildren == 0)
		return FALSE;

	iter->user_data = parent->children;
	iter->stamp = tree->stamp;

	return TRUE;
}


/*****************************************************************************
 *
 *  custom_list_iter_has_child: Returns TRUE or FALSE depending on whether
 *                              the row specified by 'iter' has any children.
 *                              We only have a list and thus no children.
 *
 *****************************************************************************/

static gboolean
tree_iter_has_child(GtkTreeModel *model, GtkTreeIter *iter)
{
	TreeNode	*t;

	t = iter->user_data;
	if (t->nchildren > 0)
		return TRUE;
	else
		return FALSE;
}


/*****************************************************************************
 *
 *  custom_list_iter_n_children: Returns the number of children the row
 *                               specified by 'iter' has. This is usually 0,
 *                               as we only have a list and thus do not have
 *                               any children to any rows. A special case is
 *                               when 'iter' is NULL, in which case we need
 *                               to return the number of top-level nodes,
 *                               ie. the number of rows in our list.
 *
 *****************************************************************************/

static gint
tree_iter_n_children(GtkTreeModel *model, GtkTreeIter *iter)
{
	TreeNode	*t;

	if (iter == NULL) {
		return TREE(model)->root->nchildren;
	} else {
		t = iter->user_data;
		return t->nchildren;
	}
}


/*****************************************************************************
 *
 *  custom_list_iter_nth_child: If the row specified by 'parent' has any
 *                              children, set 'iter' to the n-th child and
 *                              return TRUE if it exists, otherwise FALSE.
 *                              A special case is when 'parent' is NULL, in
 *                              which case we need to set 'iter' to the n-th
 *                              row if it exists.
 *
 *****************************************************************************/

static gboolean
tree_iter_nth_child(GtkTreeModel *model, GtkTreeIter *iter, GtkTreeIter *parent_iter, gint n)
{
	Tree	*tree;
	TreeNode	*parent;

	tree = TREE(model);

	if (parent_iter == NULL)
		parent = tree->root;
	else
		parent = parent_iter->user_data;

	if (n < parent->nchildren) {
		iter->user_data = parent->children + n;
		iter->stamp = tree->stamp;
		return TRUE;
	} else
		return FALSE;

}


/*****************************************************************************
 *
 *  custom_list_iter_parent: Point 'iter' to the parent node of 'child'. As
 *                           we have a list and thus no children and no
 *                           parents of children, we can just return FALSE.
 *
 *****************************************************************************/

static gboolean
tree_iter_parent(GtkTreeModel *model, GtkTreeIter *iter, GtkTreeIter *child_iter)
{
	TreeNode	*child;

	g_assert(child_iter != NULL);

	child = child_iter->user_data;
	if (child->parent == NULL) {
		return FALSE;
	} else {
		iter->user_data = child->parent;
		iter->stamp = TREE(model)->stamp;
		return TRUE;
	}
}


Tree *
tree_new(void)
{
	Tree	*tree;

	tree = g_object_new(TYPE_TREE, NULL);
	g_assert(tree != NULL);

	return tree;
}

TreeNode *
tree_vivify_child(TreeNode *parent, const char *data)
{
	size_t		 pos, left, mid, right;
	int		 d;
	TreeNode	*child;

	/* binary search */
	left = 0;
	right = parent->nchildren;
	mid = (right - left) / 2;
	while (right > left) {
		d = strcmp(data, parent->children[mid].data);
		if (d < 0)
			right = mid;
		else if (d > 0)
			left = mid + 1;
		else
			return parent->children + mid;
		mid = left + (right - left) / 2;
	}
	/* We didn't find the droids we were looking for.
	 * But `left' now points to where the new child should be placed.
	 */
	pos = left;

	parent->children = xrealloc(parent->children, sizeof(TreeNode) * (parent->nchildren + 1));
	if (pos < parent->nchildren)
		memmove(parent->children + pos + 1, parent->children + pos, sizeof(TreeNode) * (parent->nchildren - pos));
	++parent->nchildren;
	child = parent->children + pos;

	memset(child, 0, sizeof(*child));
	child->data = xstrdup(data);
	child->flags |= TREE_NODE_VISIBLE;

	return child;
}

void
tree_set_parents(TreeNode *parent)
{
	size_t	 i;
	for (i = 0; i < parent->nchildren; ++i) {
		parent->children[i].parent = parent;
		tree_set_parents(parent->children + i);
	}
}

void
tree_print(TreeNode *node, size_t depth)
{
	size_t	 i;

	if (node == NULL)
		return;

	for (i = 0; i < depth; ++i)
		printf("%s", INDENT_STR);
	printf("%s (%p), parent=%p\n", node->data ? node->data : "(null)", node, node->parent);
	for (i = 0; i < node->nchildren; ++i)
		tree_print(node->children + i, depth + 1);
}

gboolean
tree_get_iter_visible(GtkTreeModel *model, GtkTreeIter *iter, GtkTreePath *path)
{
	TreeNode	*node;
	gint		 depth;
	gint		*idx;
	gint		 i;
	gint		 j;
	gint		 n;

	g_assert(IS_TREE(model));
	g_assert(path != NULL);
	g_assert(iter != NULL);

	node = TREE(model)->root;
	idx = gtk_tree_path_get_indices(path);
	depth = gtk_tree_path_get_depth(path);
	while (depth--) {
		n = node->nchildren;
		for (i = 0, j = *idx++; i < n; ++i) {
			if ((node->children[i].flags & TREE_NODE_VISIBLE) && j-- == 0) {
				node = node->children + i;
				break;
			}
		}
		if (i >= n)
			return FALSE;
	}
	iter->stamp = TREE(model)->stamp;
	iter->user_data = node;
	return TRUE;
}

void
tree_clear(Tree *tree)
{
	tree_free(tree->root);
	free(tree->root);
	tree_init(tree);
}

static int
foreach_leaf(TreeNode *node, tree_walker_func_t cb, void *data)
{
	int	 i;
	int	 rc;

	if (node->nchildren == 0)
		return cb(node, data);
	for (i = 0; i < node->nchildren; ++i) {
		rc = foreach_leaf(node->children + i, cb, data);
		if (rc != 0)
			return rc;
	}
	return 0;
}

int
tree_foreach_leaf(Tree *tree, tree_walker_func_t cb, void *data)
{
	return foreach_leaf(tree->root, cb, data);
}
