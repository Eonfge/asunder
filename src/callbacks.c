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

gchar*
format_wavpack_bitrate                 (GtkScale *scale,
                                        gdouble   arg1,
                                        gpointer  user_data)
{
    return g_strdup_printf (_("%dKbps"), int_to_wavpack_bitrate((int)arg1));
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
    
    //trim_chars(text, BADCHARS);		// lnr	//Commented out by mrpl
    trim_whitespace(text);
    
    if(text[0] == '\0')
        gtk_entry_set_text(GTK_ENTRY(widget), "unknown");
    else
        gtk_entry_set_text(GTK_ENTRY(widget), text);
    
    free(text);
    return FALSE;
}

gboolean
on_year_focus_out_event        (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
    const gchar * ctext = gtk_entry_get_text(GTK_ENTRY(widget));
    gchar * text = malloc(5);
    if (text == NULL)
        fatalError("malloc(5) failed. Out of memory.");
    strncpy(text, ctext, 5);
    text[4] = '\0';
    
    if((text[0] != '1' && text[0] != '2') || text[1] < '0' || text[1] > '9' ||
        text[2] < '0' || text[2] > '9' || text[3] < '0' || text[3] > '9')
    {
        sprintf(text, "1900");
    }
    
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
    
    //trim_chars(text, BADCHARS);		// lnr	//Commented out by mrpl
    trim_whitespace(text);
    
    if(text[0] == '\0')
        gtk_entry_set_text(GTK_ENTRY(widget), "unknown");
    else
        gtk_entry_set_text(GTK_ENTRY(widget), text);
    
    free(text);
    return FALSE;
}

// lnr
gboolean
on_album_genre_focus_out_event         (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
    const gchar * ctext = gtk_entry_get_text(GTK_ENTRY(widget));

    gchar * text = malloc(sizeof(gchar) * (strlen(ctext) + 1));

    if (text == NULL)
        fatalError("malloc(sizeof(gchar) * (strlen(ctext) + 1)) failed. Out of memory.");

    strncpy(text, ctext, strlen(ctext)+1);
    
    //trim_chars(text, BADCHARS);		// lnr	//Commented out by mrpl
    trim_whitespace(text);
    
    if(text[0] == '\0')
        gtk_entry_set_text(GTK_ENTRY(widget), "Unknown");
    else
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
    
    //trim_chars(new_text, BADCHARS);		// lnr	//Commented out by mrpl
    trim_whitespace(new_text);
    
    gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter, path_string);
    
    if(new_text[0] == '\0')
        gtk_list_store_set(store, &iter, COL_TRACKARTIST, "unknown", -1);
    else
        gtk_list_store_set(store, &iter, COL_TRACKARTIST, new_text, -1);
}

// lnr
void
on_genre_edited                    (GtkCellRendererText *cell,
                                     gchar               *path_string,
                                     gchar               *new_text,
                                     gpointer             user_data)
{
    GtkListStore * store = GTK_LIST_STORE(gtk_tree_view_get_model(
                    GTK_TREE_VIEW(lookup_widget(win_main, "tracklist"))));
    GtkTreeIter iter;
    
    //trim_chars(new_text, BADCHARS);	//Commented out by mrpl
    trim_whitespace(new_text);
    
    gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter, path_string);
    
    if(new_text[0] == '\0')
        gtk_list_store_set(store, &iter, COL_GENRE, "Unknown", -1);
    else
        gtk_list_store_set(store, &iter, COL_GENRE, new_text, -1);
}

void
on_cancel_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
    abort_threads();
}

gboolean
on_cddb_update_closed         (GtkWidget *widget,
                               GdkEvent  *event,
                               GtkWidget**      update_window)
{
    *update_window = NULL;
    
    return FALSE;
}

void
on_deselect_all_click                  (GtkMenuItem *menuitem, 
                                        gpointer data)
{
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(win_main, "tracklist")));
    
    gtk_tree_model_foreach(model, for_each_row_deselect, NULL);
}

void
on_vbr_toggled                         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    char bitrate[8];
    GtkRange* range;
    bool vbr;
    
    /* update the displayed vbr, as it's different for vbr and non-vbr */
    vbr = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton));
    range = GTK_RANGE(lookup_widget(win_prefs, "mp3bitrate"));
    snprintf(bitrate, 8, _("%dKbps"), int_to_bitrate((int)gtk_range_get_value(range), vbr));
    gtk_label_set_text(GTK_LABEL(lookup_widget(win_prefs, "bitrate_lbl_2")), bitrate);
}

void
on_hybrid_toggled                      (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "wavpack_hybrid"))))
    {
        gtk_widget_set_sensitive(lookup_widget(win_prefs, "wavpack_bitrate_lbl"), TRUE);
        gtk_widget_set_sensitive(lookup_widget(win_prefs, "wavpack_bitrate_slider"), TRUE);
    }
    else
    {
        gtk_widget_set_sensitive(lookup_widget(win_prefs, "wavpack_bitrate_lbl"), FALSE);
        gtk_widget_set_sensitive(lookup_widget(win_prefs, "wavpack_bitrate_slider"), FALSE);
    }
}

void
on_mp3bitrate_value_changed            (GtkRange        *range,
                                        gpointer         user_data)
{
    char bitrate[8];
    bool vbr;
    
    vbr = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "mp3_vbr")));
    snprintf(bitrate, 8, _("%dKbps"), int_to_bitrate((int)gtk_range_get_value(range), vbr));
    gtk_label_set_text(GTK_LABEL(lookup_widget(win_prefs, "bitrate_lbl_2")), bitrate);
}

void
on_opusrate_value_changed           (GtkRange   *range,
                                     gpointer   user_data)
{
    char bitrate[8];
    snprintf(bitrate, 8, _("%dKbps"), int_to_bitrate((int)gtk_range_get_value(range), FALSE));
    gtk_label_set_text(GTK_LABEL(lookup_widget(win_prefs, "bitrate_lbl_4")), bitrate);
}

void
on_musepackbitrate_value_changed            (GtkRange        *range,
                                             gpointer         user_data)
{
    char bitrate[8];
    
    snprintf(bitrate, 8, _("%dKbps"), int_to_musepack_bitrate((int)gtk_range_get_value(range)));
    gtk_label_set_text(GTK_LABEL(lookup_widget(win_prefs, "bitrate_lbl_3")), bitrate);
}

void
on_pick_disc_changed                   (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    cddb_disc_t * disc = g_list_nth_data(gbl_disc_matches, gtk_combo_box_get_active(combobox));
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
    //gtk_widget_hide(GTK_WIDGET(dialog));
    
    if (response_id == GTK_RESPONSE_OK)
    {
        if (!prefs_are_valid())
            return;
        
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
on_lookup_clicked                     (GtkToolButton   *toolbutton,
                                       gpointer         user_data)
{
    /* i need to lock myself in refresh()->lookup_disc() */
    /* another possible solution for this problem:
    static GThread *main_thread = NULL;
    
    void thread_helpers_init (void) {
       main_thread = g_thread_self ();
    }
    gboolean thread_helpers_in_main_thread (void) {
       return (main_thread == g_thread_self ());
    }
    void thread_helpers_lock_gdk (void) {
       if (!thread_helpers_in_main_thread ())  gdk_threads_enter ();
    }
    void thread_helpers_unlock_gdk (void) {
       if (!thread_helpers_in_main_thread ()) gdk_threads_leave ();
    }
    */
    gdk_threads_leave();
    refresh(global_prefs->cdrom, 1);
    gdk_threads_enter();
}

void
on_rip_button_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkListStore * store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(win_main, "tracklist"))));
    if (store == NULL)
    {
        GtkWidget * dialog;
        dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), 
                                        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
                                        _("No CD is inserted. Please insert a CD into the CD-ROM drive."));
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
on_rip_opus_toggled                     (GtkToggleButton *togglebutton,
                                        gpointer user_data)
{
    if (gtk_toggle_button_get_active(togglebutton) && !program_exists("opusenc"))
    {
        GtkWidget *dialog;
        dialog = gtk_message_dialog_new(GTK_WINDOW(win_main),
                                        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                        _("%s was not found in your path. Asunder requires it to create %s files."
                                          "All %s functionality is disabled."),
                                        "'opusenc'", "OPUS", "opus");
        gtk_dialog_run (GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        global_prefs->rip_opus=0;
        gtk_toggle_button_set_active(togglebutton, global_prefs->rip_opus);

    }

    if (!gtk_toggle_button_get_active(togglebutton))
        disable_opus_widgets();
    else
        enable_opus_widgets();
}

void
on_rip_wavpack_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton) && !program_exists("wavpack"))
    {
        GtkWidget * dialog;
        dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), 
                                        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
                                        _("%s was not found in your path. Asunder requires it to create %s files. "
                                        "All %s functionality is disabled."),
                                        "'wavpack'", "WV", "wavpack");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        global_prefs->rip_wavpack = 0;
        gtk_toggle_button_set_active(togglebutton, global_prefs->rip_wavpack);
    }
    
    if (!gtk_toggle_button_get_active(togglebutton))
        disable_wavpack_widgets();
    else
        enable_wavpack_widgets();
}

void
on_rip_monkey_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton) && !program_exists("mac"))
    {
        GtkWidget * dialog;
        dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), 
                                        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
                                        _("%s was not found in your path. Asunder requires it to create %s files. "
                                        "All %s functionality is disabled."),
                                        "'mac'", "APE", "Monkey's Audio");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        global_prefs->rip_monkey = 0;
        gtk_toggle_button_set_active(togglebutton, global_prefs->rip_monkey);
    }
    
    if (!gtk_toggle_button_get_active(togglebutton))
        disable_monkey_widgets();
    else
        enable_monkey_widgets();
}

void
on_rip_aac_toggled                  (GtkToggleButton *togglebutton,
                                     gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton) && !program_exists("neroAacEnc"))
    {
        GtkWidget * dialog;
        dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), 
                                        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
                                        _("%s was not found in your path. Asunder requires it to create %s files. "
                                        "All %s functionality is disabled."),
                                        "'neroAacEnc'", "MP4", "AAC");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        global_prefs->rip_aac = 0;
        gtk_toggle_button_set_active(togglebutton, global_prefs->rip_aac);
    }
    
    if (!gtk_toggle_button_get_active(togglebutton))
        disable_aac_widgets();
    else
        enable_aac_widgets();
}

void
on_rip_musepack_toggled                  (GtkToggleButton *togglebutton,
                                          gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton) && !program_exists("mpcenc"))
    {
        GtkWidget * dialog;
        dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), 
                                        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
                                        _("%s was not found in your path. Asunder requires it to create %s files. "
                                        "All %s functionality is disabled."),
                                        "'mpcenc'", "MPC", "Musepack");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        global_prefs->rip_musepack = 0;
        gtk_toggle_button_set_active(togglebutton, global_prefs->rip_musepack);
    }
    
    if (!gtk_toggle_button_get_active(togglebutton))
        disable_musepack_widgets();
    else
        enable_musepack_widgets();
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
//    GtkTreeViewColumn * col = gtk_tree_view_get_column(GTK_TREE_VIEW(tracklist), 2);
    GtkTreeViewColumn * col = gtk_tree_view_get_column(GTK_TREE_VIEW(tracklist), COL_TRACKARTIST ); //lnr
    gtk_tree_view_column_set_visible(col, !gtk_toggle_button_get_active(togglebutton));
}

// lnr
void
on_single_genre_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    GtkTreeViewColumn * col = gtk_tree_view_get_column(GTK_TREE_VIEW(tracklist), COL_GENRE );
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
    
    //trim_chars(new_text, BADCHARS);		// lnr	//Commented out by mrpl
    trim_whitespace(new_text);
    
    gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter, path_string);
    
    if(new_text[0] == '\0')
        gtk_list_store_set(store, &iter, COL_TRACKTITLE, "unknown", -1);
    else
        gtk_list_store_set(store, &iter, COL_TRACKTITLE, new_text, -1);
    
    
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
        
        /* no need for signal to propagate */
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
