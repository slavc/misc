#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include <unistd.h>
#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
/*#include <sys/limits.h>*/
#include <dirent.h>
#include <libgen.h>

#include <gtk/gtk.h>

#include "fm.h"
#include "util.h"

GdkPixbuf *get_icon(fsnode_t *node);
GtkWidget *create_icon_view(void);
GtkWidget *create_main_view(void);
GtkWidget *create_scrolled_window(GtkWidget *child);
int        change_dir(const char *path);
void       handle_icon_view_item_activated(GtkWidget *icon_view, GtkTreePath *tree_path, gpointer unused);
void       handle_main_window_destroy(GtkWidget *widget, gpointer unused);
void       load_default_icons(void);
void       populate_icon_view(GtkWidget *icon_view, fsnode_t *node);

struct fsnode *current_fsnode; /* the directory which we are displaying currently */
GdkPixbuf     *directory_icon,
              *regular_file_icon,
              *chr_device_icon,
              *blk_device_icon;
GtkWidget     *icon_view,
              *path_entry;

enum icon_view_columns {
    COL_NAME,
    COL_ICON,
    N_COLS
};

int
main(int argc, char **argv) {
    char cwd[PATH_MAX];
    GtkWidget *main_window;

    gtk_init(&argc, &argv);
    gtk_rc_parse("fm.rc");

    load_default_icons();
    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(G_OBJECT(main_window), "destroy", G_CALLBACK(handle_main_window_destroy), NULL);
    gtk_container_add(GTK_CONTAINER(main_window), create_main_view());

    if (getcwd(cwd, sizeof cwd) == NULL)
        err(1, "getcwd");
    change_dir(cwd);

    gtk_widget_show_all(main_window);
    gtk_main();

    return 0;
}

char *
strip_space(const char *s) {
    char *buf, *p, *q;

    while (isspace(*s))
        ++s;

    buf = p = strdup(s);

    p = strrchr(buf, '\0');
    while (p > buf && isspace(*p))
        *p-- = '\0';

    return buf;
}

/* backwards strchr */
char *
rstrchr(const char *str_start, const char *search_start, const char c) {
    const char *p;

    for (p = search_start; p >= str_start; --p)
        if (*p == c)
            return (char *) p;

    return NULL;
}

char *
normalize_path(const char *path) {
    size_t        nslashes;
    char         *buf,
                 *dotdot,
                 *p;
    int           do_loop;
    const char   *dotdot_str = "..",
                 *slash_dotdot_str = "/..";
    const size_t  dotdot_str_len       = sizeof(dotdot_str) - 1,
                  slash_dotdot_str_len = sizeof(slash_dotdot_str) - 1;

    assert(path);

    buf = strip_space(path);
    if (*buf == '\0') {
        free(buf);
        return strdup("/");
    }

    do {
        do_loop = 0;

        /* first, get rid of slases in the end,
         * as well as repeating slashes
         */
        p = strrchr(buf, '\0');
        while (*--p == '/' && p > buf) /* we handled the "" case above, so it's ok to *--p */
            *p = '\0';
        for (p = buf; p = strchr(p, '/'); ++p) {
            for (nslashes = 1; *(p + nslashes) == '/'; ++nslashes)
                /* empty */;
            if (nslashes > 1)
                memmove(p, p + nslashes - 1, strlen(p + nslashes - 1) + 1);
        }

        /* now take care of ".." */
        while (dotdot = strstr(buf, slash_dotdot_str)) {
            /* while dealing with .. we might introduce
             * repeating slashes, so loop the outer loop
             * once more...
             */
            do_loop = 1;
            if (dotdot == buf) {
                free(buf);
                return strdup("/");
            } else if (p = rstrchr(buf, dotdot - 1, '/')) {
                memmove(p, dotdot + slash_dotdot_str_len, strlen(dotdot + slash_dotdot_str_len) + 1);
            } else {
                free(buf);
                return strdup("/");
            }
        }

        for (dotdot = buf; dotdot = strstr(dotdot, dotdot_str); dotdot += dotdot_str_len) {
            if (dotdot == buf || *(dotdot - 1) != '/') {
                free(buf);
                return strdup("/");
            }
        }

        if (!strchr(buf, '/')) {
            free(buf);
            return strdup("/");
        }

    } while (do_loop);

    return buf;
}

/*
int
main(int argc, char **argv) {
    int i;
    char *p;

    const char *tests[] = {
        "",
        "/",
        "//",
        "///",
        "/qwe",
        "/qwe/",
        "/qwe/asd",
        "/qwe/asd/",
        "/qwe//asd",
        "   /qwe//asd/",
        "/qwe//123//",
        "/..",
        "///qwe/..",
        "/qwe/../asd",
        "/qwe/../asd/..//888/../../..",
        "xzc",
        "zxc..ccc",
        "/zxc..vvv",
        "/asd//qwe/..",
        "/home/s/src/svn/my/..",
        NULL
    };

    for (i = 0; tests[i]; ++i) {
        printf("test %-30s: \"%s\"\n", tests[i], p = normalize_path(tests[i]));
        free(p);
    }

    for (i = 1; i < argc; ++i) {
        printf("\"%s\"\n", p = normalize_path(argv[i]));
        free(p);
    }

    return 0;
}
*/

void
set_address_bar(const char *s) {
    if (current_fsnode)
        gtk_entry_set_text(GTK_ENTRY(path_entry), current_fsnode->path);
    gtk_widget_grab_focus(icon_view);
}

/* as in start showing this dir on the screen,
 * not actually chdir(2) to it
 */
int
change_dir(const char *path) {
    char *npath;

    assert(path);

    if (!current_fsnode)
        current_fsnode = xcalloc(1, sizeof(*current_fsnode));

    npath = normalize_path(path);

    list_dir(npath, current_fsnode);
    sort_fsnodes_children(current_fsnode);
    populate_icon_view(icon_view, current_fsnode);
    set_address_bar(current_fsnode->path);

    free(npath);

    return 0;
}

void
handle_back_clicked(GtkWidget *widget, gpointer unused) {
}

void
handle_forward_clicked(GtkWidget *widget, gpointer unused) {
}

void
handle_home_clicked(GtkWidget *widget, gpointer unused) {
    change_dir(get_home_dir());
}

void
handle_up_clicked(GtkWidget *widget, gpointer unused) {
    char buf[PATH_MAX];

    if (current_fsnode) {
        snprintf(buf, sizeof buf, "%s/..", current_fsnode->path);
        change_dir(buf);
    }
}

void
handle_path_entered(GtkWidget *widget, gchar *text, gpointer unused) {
    const gchar *path;

    path = gtk_entry_get_text(GTK_ENTRY(widget));
    change_dir(path);
}

GtkWidget *
create_main_view(void) {
    int i;
    GtkWidget *vbox, *hbox;
    GtkWidget *tool_button;
    GtkWidget *scrolled_window;
    struct {
        const gchar *stock_id;
        void *click_handler;
    } tool_buttons[] = {
        { GTK_STOCK_GO_BACK,    handle_back_clicked    },
        { GTK_STOCK_GO_FORWARD, handle_forward_clicked },
        { GTK_STOCK_HOME,       handle_home_clicked    },
        { GTK_STOCK_GO_UP,      handle_up_clicked      }
    };

    vbox = gtk_vbox_new(FALSE, 0);
    hbox = gtk_hbox_new(FALSE, 0);
    path_entry = gtk_entry_new();
    icon_view = create_icon_view();
    scrolled_window = create_scrolled_window(icon_view);

    g_signal_connect(G_OBJECT(path_entry), "insert-at-cursor", G_CALLBACK(handle_path_entered), NULL);

    for (i = 0; i < NELEMS(tool_buttons); ++i) {
        tool_button = (GtkWidget *) gtk_tool_button_new_from_stock(tool_buttons[i].stock_id);
        g_signal_connect(G_OBJECT(tool_button), "clicked", G_CALLBACK(tool_buttons[i].click_handler), NULL);
        gtk_box_pack_start(GTK_BOX(hbox), tool_button, FALSE, FALSE, 0);
    }

    gtk_box_pack_start(GTK_BOX(hbox), path_entry, TRUE, TRUE, 0);
    
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    return vbox;
}

GtkWidget *
create_icon_view(void) {
    GtkListStore *list_store;
    GtkWidget *widget;

    assert(list_store);

    list_store = gtk_list_store_new(N_COLS, G_TYPE_STRING, GDK_TYPE_PIXBUF);
    widget = gtk_icon_view_new_with_model(GTK_TREE_MODEL(list_store));
    gtk_icon_view_set_text_column(GTK_ICON_VIEW(widget), COL_NAME);
    gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(widget), COL_ICON);
    gtk_icon_view_set_selection_mode(GTK_ICON_VIEW(widget), GTK_SELECTION_MULTIPLE);
    gtk_icon_view_set_margin(GTK_ICON_VIEW(widget), 4);
    gtk_icon_view_set_spacing(GTK_ICON_VIEW(widget), 5);
    gtk_icon_view_set_item_width(GTK_ICON_VIEW(widget), 80);
    g_signal_connect(G_OBJECT(widget), "item-activated", G_CALLBACK(handle_icon_view_item_activated), NULL);

    return widget;
}

void
handle_icon_view_item_activated(GtkWidget *widget, GtkTreePath *tree_path, gpointer unused) {
    const char *opener = "/home/s/bin/op";
    char *argv[] = { NULL, NULL, NULL };
    char *new_path = NULL;
    char *file_path = NULL;
    gint *pidx;
    pid_t pid;
    fsnode_t *child;

    pidx = gtk_tree_path_get_indices(tree_path);
    /* they directly correlate to fsents in current_fsdir */
    if (*pidx < 0 || *pidx >= current_fsnode->nchildren)
        return;
    child = current_fsnode->children + *pidx;

    if (child->type == DIRECTORY) {
        if (!strcmp(child->name, ".."))
            new_path = ascend(current_fsnode->path);
        else
            new_path = str_glue(current_fsnode->path, slash(current_fsnode->path), child->name, NULL);
        change_dir(new_path);
        free(new_path);
    } else {
        pid = fork();

        if (pid > 0)
            return;
        else if (pid < 0) {
            warn("fork");
            return;
        } else {
            file_path = str_glue(current_fsnode->path, slash(current_fsnode->path), child->name, NULL);
            argv[0] = strdup(opener);
            argv[1] = strdup(file_path);
            if (execv("/home/s/bin/op", argv))
                err(1, "execlp");
        }
    }
}

GtkWidget *
create_scrolled_window(GtkWidget *child) {
    GtkWidget *scrolled_window;

    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    if (child)
        gtk_container_add(GTK_CONTAINER(scrolled_window), child);
    return scrolled_window;
}

void
populate_icon_view(GtkWidget *widget, fsnode_t *node) {
    GtkListStore *list_store;
    GtkTreeIter iter;
    size_t i;
    fsnode_t *child;

    assert(widget && node);

    list_store = (GtkListStore *) gtk_icon_view_get_model(GTK_ICON_VIEW(widget));
    gtk_list_store_clear(list_store);
    for (i = 0; i < node->nchildren; ++i) {
        child = node->children + i;
        gtk_list_store_append(list_store, &iter);
        gtk_list_store_set(list_store, &iter, COL_NAME, child->name, COL_ICON, get_icon(child), -1);
    }
}

void
load_default_icons(void) {
    GError *error = NULL;

    if ((directory_icon = gdk_pixbuf_new_from_file("directory.png", &error)) == NULL)
        warnx("failed to load directory icon: %s", error->message);
    error = NULL;
    if ((regular_file_icon = gdk_pixbuf_new_from_file("file.png", &error)) == NULL)
        warnx("failed to load folder icon: %s", error->message);
}

GdkPixbuf *
get_icon(fsnode_t *node) {
    assert(node);

    if (node->type == DIRECTORY) {
        return directory_icon;
    } else {
        return regular_file_icon;
    }
}

void
handle_main_window_destroy(GtkWidget *widget, gpointer unused) {
    gtk_main_quit();
}

