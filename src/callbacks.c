/*
Asunder

Copyright(C) 2005 Eric Lathrop <eric@ericlathrop.com>
Copyright(C) 2007 Andrew Smith <http://littlesvr.ca/misc/contactandrew.php>

Any code in this file may be redistributed or modified under the terms of
the GNU General Public Licence as published by the Free Software 
Foundation; version 2 of the licence.

*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "main.h"
#include "prefs.h"
#include "threads.h"
#include "util.h"

gboolean
for_each_row_deselect                  (GtkTreeModel *model,
                                        GtkTreePath *path,
                                        GtkTreeIter *iter,
                                        gpointer data)
{
    gtk_list_store_set(GTK_LIST_STORE(model), iter, COL_RIPTRACK, 0, -1);
    
    return FALSE;
}

gboolean
for_each_row_select                    (GtkTreeModel *model,
                                        GtkTreePath *path,
                                        GtkTreeIter *iter,
                                        gpointer data)
{
    gtk_list_store_set(GTK_LIST_STORE(model), iter, COL_RIPTRACK, 1, -1);
    
    return FALSE;
}

gboolean
idle(gpointer data)
{
    refresh(global_prefs->cdrom, 0);
    return (data != NULL);
}

void
on_about_clicked                       (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{
    show_aboutbox();
}

void
on_aboutbox_response                   (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data)
{
    gtk_widget_hide(GTK_WIDGET(dialog));
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

gboolean
on_album_artist_focus_out_event        (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
    const gchar * ctext = gtk_entry_get_text(GTK_ENTRY(widget));
    gchar * text = malloc(sizeof(gchar) * (strlen(ctext) + 1));
    if (text == NULL)
        fatalError("malloc(sizeof(gchar) * (strlen(ctext) + 1)) failed. Out of memory.");
    strncpy(text, ctext, strlen(ctext)+1);
    
    trim_chars(text, global_prefs->invalid_chars);
    trim_whitespace(text);
    gtk_entry_set_text(GTK_ENTRY(widget), text);
    
    free(text);
    return FALSE;
}

gboolean
on_album_title_focus_out_event         (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
    const gchar * ctext = gtk_entry_get_text(GTK_ENTRY(widget));
    gchar * text = malloc(sizeof(gchar) * (strlen(ctext) + 1));
    if (text == NULL)
        fatalError("malloc(sizeof(gchar) * (strlen(ctext) + 1)) failed. Out of memory.");
    strncpy(text, ctext, strlen(ctext)+1);
    
    trim_chars(text, global_prefs->invalid_chars);
    trim_whitespace(text);
    gtk_entry_set_text(GTK_ENTRY(widget), text);
    
    free(text);
    return FALSE;
}

void
on_artist_edited                    (GtkCellRendererText *cell,
                                     gchar               *path_string,
                                     gchar               *new_text,
                                     gpointer             user_data)
{
    GtkListStore * store = GTK_LIST_STORE(gtk_tree_view_get_model(
                    GTK_TREE_VIEW(lookup_widget(win_main, "tracklist"))));
    GtkTreeIter iter;
    
    trim_chars(new_text, global_prefs->invalid_chars);
    trim_whitespace(new_text);
    gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter, path_string);
    gtk_list_store_set(store, &iter,
            COL_TRACKARTIST, new_text,
            -1);
}

void
on_cancel_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
    abort_threads();
}

void
on_deselect_all_click                  (GtkMenuItem *menuitem, 
                                        gpointer data)
{
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(win_main, "tracklist")));
    
    gtk_tree_model_foreach(model, for_each_row_deselect, NULL);
}

void
on_mp3bitrate_value_changed            (GtkRange        *range,
                                        gpointer         user_data)
{
    char bitrate[8];
    snprintf(bitrate, 8, "%dKbps", int_to_bitrate((int)gtk_range_get_value(range)));
    gtk_label_set_text(GTK_LABEL(lookup_widget(win_prefs, "bitrate_lbl_2")), bitrate);
}

void
on_pick_disc_changed                   (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    cddb_disc_t * disc = g_list_nth_data(disc_matches, gtk_combo_box_get_active(combobox));
    update_tracklist(disc);
}

void
on_preferences_clicked                 (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{
    win_prefs = create_prefs();
    gtk_widget_show(win_prefs);
}

void
on_prefs_response                      (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data)
{
    gtk_widget_hide(GTK_WIDGET(dialog));

    if (response_id == GTK_RESPONSE_OK)
    {
        get_prefs_from_widgets(global_prefs);
        save_prefs(global_prefs);
    }
    
    gtk_widget_destroy(GTK_WIDGET(dialog));
    
}

void
on_prefs_show                          (GtkWidget       *widget,
                                        gpointer         user_data)
{
    set_widgets_from_prefs(global_prefs);
}

void 
on_press_f2                       (void)
{
    GtkWidget* treeView;
    GtkTreePath* treePath;
    GtkTreeViewColumn* focusColumn;
    
    treeView = lookup_widget(win_main, "tracklist");
    
    if (!GTK_WIDGET_HAS_FOCUS(treeView))
        return;
    
    gtk_tree_view_get_cursor(GTK_TREE_VIEW(treeView), &treePath, &focusColumn);
    
    if (treePath == NULL || focusColumn == NULL)
        return;
    
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(treeView), treePath, focusColumn, TRUE);
}

void
on_refresh_clicked                     (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{
    refresh(global_prefs->cdrom, 1);
}

void
on_rip_button_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
    if (!global_prefs->rip_wav && !global_prefs->rip_mp3 && !global_prefs->rip_ogg && !global_prefs->rip_flac)
    {
        GtkWidget * dialog;
        dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), 
                                        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
                                        _("No ripping/encoding method selected. Please enable one from the "
                                        "'Preferences' menu."));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    GtkListStore * store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(win_main, "tracklist"))));
    if (store == NULL)
    {
        GtkWidget * dialog;
        dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), 
                                        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
                                        "No CD is inserted. Please insert a CD into the CD-ROM drive.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    GtkTreeIter iter;
    gboolean rowsleft = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
    int tracks_to_rip = 0;
    int riptrack;
    while(rowsleft)
    {
        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
                           COL_RIPTRACK, &riptrack, -1);
        if (riptrack) 
            tracks_to_rip++;
        rowsleft = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
    }
    if (tracks_to_rip == 0)
    {
        GtkWidget * dialog;
        dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), 
                                        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
                                        _("No tracks were selected for ripping/encoding. "
                                        "Please select at least one track."));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    dorip();
}

void
on_rip_mp3_toggled                     (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton) && !program_exists("lame"))
    {
        GtkWidget * dialog;
        
        dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), 
                                        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
                                        _("%s was not found in your path. Asunder requires it to create %s files. "
                                        "All %s functionality is disabled."),
                                        "'lame'", "MP3", "MP3");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        global_prefs->rip_mp3 = 0;
        gtk_toggle_button_set_active(togglebutton, global_prefs->rip_mp3);
    }
    
    if (!gtk_toggle_button_get_active(togglebutton))
        disable_mp3_widgets();
    else
        enable_mp3_widgets();
}

void
on_rip_flac_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton) && !program_exists("flac"))
    {
        GtkWidget * dialog;
        dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), 
                                        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
                                        _("%s was not found in your path. Asunder requires it to create %s files. "
                                        "All %s functionality is disabled."),
                                        "'flac'", "FLAC", "FLAC");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        global_prefs->rip_flac = 0;
        gtk_toggle_button_set_active(togglebutton, global_prefs->rip_flac);
    }
    
    if (!gtk_toggle_button_get_active(togglebutton))
        disable_flac_widgets();
    else
        enable_flac_widgets();
}

void
on_rip_ogg_toggled                     (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton) && !program_exists("oggenc"))
    {
        GtkWidget * dialog;
        dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), 
                                        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
                                        _("%s was not found in your path. Asunder requires it to create %s files. "
                                        "All %s functionality is disabled."),
                                        "'oggenc'", "OGG", "OGG");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        global_prefs->rip_ogg = 0;
        gtk_toggle_button_set_active(togglebutton, global_prefs->rip_ogg);
    }
    
    if (!gtk_toggle_button_get_active(togglebutton))
        disable_ogg_widgets();
    else
        enable_ogg_widgets();
}

void
on_rip_toggled                       (GtkCellRendererToggle *cell,
                                      gchar                 *path_string,
                                      gpointer               user_data)
{
    GtkListStore * store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(win_main, "tracklist"))));
    GtkTreeIter iter;
    int toggled;

    gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter, path_string);
    gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
            COL_RIPTRACK, &toggled,
            -1);
    gtk_list_store_set(store, &iter,
            COL_RIPTRACK, !toggled,
            -1);
}

void
on_select_all_click                    (GtkMenuItem *menuitem, 
                                        gpointer data)
{
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(win_main, "tracklist")));
    
    gtk_tree_model_foreach(model, for_each_row_select, NULL);
}

void
on_single_artist_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    GtkTreeViewColumn * col = gtk_tree_view_get_column(GTK_TREE_VIEW(tracklist), 2);
    gtk_tree_view_column_set_visible(col, !gtk_toggle_button_get_active(togglebutton));
}

void
on_title_edited                    (GtkCellRendererText *cell,
                                    gchar               *path_string,
                                    gchar               *new_text,
                                    gpointer             user_data)
{
    GtkListStore * store = GTK_LIST_STORE(gtk_tree_view_get_model(
                    GTK_TREE_VIEW(lookup_widget(win_main, "tracklist"))));
    GtkTreeIter iter;
    
    trim_chars(new_text, global_prefs->invalid_chars);
    trim_whitespace(new_text);
    gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter, path_string);
    gtk_list_store_set(store, &iter,
            COL_TRACKTITLE, new_text,
            -1);
}

gboolean
on_tracklist_mouse_click               (GtkWidget* treeView, 
                                        GdkEventButton* event, 
                                        gpointer user_data)
{
    if( event->type == GDK_BUTTON_PRESS && event->button == 3 &&
        GTK_WIDGET_SENSITIVE(lookup_widget(win_main, "rip_button")) )
    {
        GtkWidget* menu;
        GtkWidget* menuItem;
        
        menu = gtk_menu_new();
        
        menuItem = gtk_menu_item_new_with_label(_("Select all for ripping"));
        g_signal_connect(menuItem, "activate", 
                         G_CALLBACK(on_select_all_click), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem);
        gtk_widget_show_all(menu);
        
        menuItem = gtk_menu_item_new_with_label(_("Deselect all for ripping"));
        g_signal_connect(menuItem, "activate", 
                         G_CALLBACK(on_deselect_all_click), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem);
        gtk_widget_show_all(menu);
        
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
                       event->button, gdk_event_get_time((GdkEvent*)event));
        
        /* no need for signal to propagete */
        return TRUE;
    }
    
    return FALSE;
}

void
on_window_close                        (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
    gtk_window_get_size(GTK_WINDOW(win_main), 
            &global_prefs->main_window_width, 
            &global_prefs->main_window_height);
    
    save_prefs(global_prefs);

    gtk_main_quit();
}    
