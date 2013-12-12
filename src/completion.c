#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#include <gtk/gtk.h>
#include <glib.h>

#include "util.h"

#define COMPLETION_MAX 4000
#define COMPLETION_NAME_KEY "completion_name"

static char * completion_filename(const char * name, int create_dirs)
{
	const char * cache = g_getenv("XDG_CACHE_HOME");
	char * file;
	if (cache == NULL) {
		file = g_strdup_printf("%s/.asunder_%s", g_getenv("HOME"), name);
	}
	else {
		file = g_strdup_printf("%s/asunder/%s", cache, name);
		if (create_dirs) {
			char * dir = g_strdup_printf("%s/asunder", cache);
			recursive_mkdir(dir, S_IRWXU|S_IRWXG|S_IRWXO);
			g_free(dir);
		}
	}
	debugLog("using completion file name: %s\n", file);
	return file;
}

static void
read_completion_file(GtkListStore * list, const char * name)
{
    char buf[1024];
    char * file;
    char * ptr;
    FILE * data;
    GtkTreeIter iter;
    int i;

    file = completion_filename(name, false);
    if (file == NULL)
      fatalError("g_strdup_printf() failed. Out of memory.");

    debugLog("Reading completion data\n");

    data = fopen(file, "r");
    if (data == NULL) {
      g_free(file);
      return;
    }

    for (i = 0; i < COMPLETION_MAX; i++) {
      ptr = fgets(buf, sizeof(buf), data);
      if (ptr == NULL)
	break;

      g_strstrip(buf);

      if (buf[0] == '\0')
	continue;

      if (g_utf8_validate(buf, -1, NULL)) {
	gtk_list_store_append(list, &iter);
	gtk_list_store_set(list, &iter, 0, buf, -1);
      }
    }

    fclose(data);
    g_free(file);
}


void
create_completion(GtkWidget * entry, const char * name)
{
    GtkEntryCompletion * compl;
    GtkListStore * list;
    gchar * str;

    list = gtk_list_store_new(1, G_TYPE_STRING);
    compl = gtk_entry_completion_new();
    gtk_entry_completion_set_model(compl, GTK_TREE_MODEL(list));
    gtk_entry_completion_set_inline_completion(compl, FALSE);
    gtk_entry_completion_set_popup_completion(compl, TRUE);
    gtk_entry_completion_set_popup_set_width(compl, TRUE);
    gtk_entry_completion_set_text_column(compl, 0);

    gtk_entry_set_completion(GTK_ENTRY(entry), compl);

    str = g_strdup(name);
    g_object_set_data(G_OBJECT(entry), COMPLETION_NAME_KEY, str);

    read_completion_file(list, name);
}

static gboolean
save_history_cb(GtkTreeModel * model, GtkTreePath * path, GtkTreeIter * iter, gpointer data)
{
    FILE * fp;
    char * str;

    fp = (FILE *) data;

    gtk_tree_model_get(model, iter, 0, &str, -1);
    if (str) {
      fprintf(fp, "%s\n", str);
      g_free(str);
    }

    return FALSE;
}

static GtkTreeModel *
get_tree_model(GtkWidget * entry)
{
    GtkEntryCompletion * compl;
    GtkTreeModel * model;

    compl = gtk_entry_get_completion(GTK_ENTRY(entry));
    if (compl == NULL)
      return NULL;

    model = gtk_entry_completion_get_model(compl);

    return model;
}

void
save_completion(GtkWidget * entry)
{
    GtkTreeModel * model;
    const gchar * name;
    char * file;
    FILE * data;

    model = get_tree_model(entry);
    if (model == NULL)
      return;

    name = g_object_get_data(G_OBJECT(entry), COMPLETION_NAME_KEY);
    if (name == NULL)
      return;

    file = completion_filename(name, 1);
    if (file == NULL)
      fatalError("g_strdup_printf() failed. Out of memory.");

    debugLog("Saving completion data\n");

    data = fopen(file, "w");
    if (data) {
      gtk_tree_model_foreach(model, save_history_cb, data);
      fclose(data);
    }

    g_free(file);
}

void
add_completion(GtkWidget * entry)
{
    GtkTreeModel * model;
    GtkTreeIter iter;
    gboolean found;
    char * saved_str = NULL;
    const char * str;

    str = gtk_entry_get_text(GTK_ENTRY(entry));
    if (str == NULL || strlen(str) == 0)
      return;

    model = get_tree_model(entry);
    if (model == NULL)
      return;

    found = gtk_tree_model_get_iter_first(model, &iter);
    while (found) {
      gtk_tree_model_get(model, &iter, 0, &saved_str, -1);
      if (saved_str) {
	if (strcmp(str, saved_str) == 0) {
	  gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
	  g_free(saved_str);
	  break;
	}
	g_free(saved_str);
      }
      found = gtk_tree_model_iter_next(model, &iter);
    }
    gtk_list_store_prepend(GTK_LIST_STORE(model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, str, -1);
}
