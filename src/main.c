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
#include <sys/types.h>
#include <cddb/cddb.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>

#include "main.h"
#include "interface.h"
#include "support.h"
#include "prefs.h"
#include "callbacks.h"
#include "util.h"
#include "wrappers.h"


GList * disc_matches = NULL;
gboolean track_format[100];

GtkWidget * win_main = NULL;
GtkWidget * win_prefs = NULL;
GtkWidget * win_ripping = NULL;
GtkWidget * win_about = NULL;

GtkWidget * album_artist;
GtkWidget * album_title;
GtkWidget * tracklist;
GtkWidget * pick_disc;


int main(int argc, char *argv[])
{
    GtkTreeViewColumn *col;
    GtkCellRenderer *renderer;
    
#ifdef ENABLE_NLS
    /* initialize gettext */
    bindtextdomain("asunder", PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset("asunder", "UTF-8"); /* so that gettext() returns UTF-8 strings */
    textdomain("asunder");
#endif
    
    /* SET UP signal handler for children */
    struct sigaction signalHandler;
    sigset_t blockedSignals;
    
    bzero(&signalHandler, sizeof(signalHandler));
    signalHandler.sa_handler = sigchld;
    //~ signalHandler.sa_flags = SA_RESTART;
    sigemptyset(&blockedSignals);
    sigaddset(&blockedSignals, SIGCHLD);
    signalHandler.sa_mask = blockedSignals;
    
    sigaction(SIGCHLD, &signalHandler, NULL);
    /* END SET UP signal handler for children */
    
    //gtk_set_locale();
    g_thread_init(NULL);
    gdk_threads_init();
    gtk_init(&argc, &argv);
    
    add_pixmap_directory(PACKAGE_DATA_DIR "/pixmaps");
#ifdef DEBUG
    printf("Pixmap dir: " PACKAGE_DATA_DIR "/pixmaps\n");
#endif
    
    global_prefs = get_default_prefs();
    load_prefs(global_prefs);
    
    win_main = create_main();
    album_artist = lookup_widget(win_main, "album_artist");
    album_title = lookup_widget(win_main, "album_title");
    tracklist = lookup_widget(win_main, "tracklist");
    pick_disc = lookup_widget(win_main, "pick_disc");
    
    // set up all the columns for the track listing widget
    renderer = gtk_cell_renderer_toggle_new();
    g_object_set(renderer, "activatable", TRUE, NULL);
    g_signal_connect(renderer, "toggled", (GCallback) rip_toggled, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tracklist), -1, 
                    _("Rip"), renderer, "active", COL_RIPTRACK, NULL);

    col = gtk_tree_view_column_new();
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tracklist), -1, 
                    _("Track"), renderer, "text", COL_TRACKNUM, NULL);

    col = gtk_tree_view_column_new();
    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "editable", TRUE, NULL);
    g_signal_connect(renderer, "edited", (GCallback) artist_edited, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tracklist), -1, 
                    _("Artist"), renderer, "text", COL_TRACKARTIST, NULL);

    col = gtk_tree_view_column_new();
    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "editable", TRUE, NULL);
    g_signal_connect(renderer, "edited", (GCallback) title_edited, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tracklist), -1, 
                    _("Title"), renderer, "text", COL_TRACKTITLE, NULL);

    col = gtk_tree_view_column_new();
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tracklist), -1, 
                    _("Time"), renderer, "text", COL_TRACKTIME, NULL);

    // disable the "rip" button
    // it will be enabled when check_disc() finds a disc in the drive
    gtk_widget_set_sensitive(lookup_widget(win_main, "rip_button"), FALSE);
    
    win_ripping = create_ripping();

    if (!program_exists("cdparanoia"))
    {
        GtkWidget * dialog;

        dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
                _("'cdparanoia' was not found in your path. Asunder requires cdparanoia to rip CDs."));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        exit(-1);
    }
    
    gtk_widget_show(win_main);
    
    // set up recurring timeout to automatically re-scan the cdrom once every second
    g_timeout_add(1000, idle, (void *)1);
    // add an idle event to scan the cdrom drive ASAP
    g_idle_add(idle, NULL);

    gdk_threads_enter();
    gtk_main();
    gdk_threads_leave();
    return 0;
}


bool check_disc(char * cdrom)
{
    int fd;
    bool ret = false;
    int status;
    
    // open the device
    fd = open(cdrom, O_RDONLY | O_NONBLOCK);
    if (fd < 0)
    {
        fprintf(stderr, "Error: Couldn't open %s\n", cdrom);
        return false;
    }
    
    /* this was the original (Eric's 0.1 and post 0.0.1) checking code,
    * but it never worked properly for me. Removed 21 aug 2007. */
    //~ static bool newdisc = true;
    //~ // read the drive status info
    //~ if (ioctl(fd, CDROM_DRIVE_STATUS, CDSL_CURRENT) == CDS_DISC_OK)
    //~ {
        //~ if (newdisc)
        //~ {
            //~ newdisc = false;
            
            //~ status = ioctl(fd, CDROM_DISC_STATUS, CDSL_CURRENT);
            //~ if ((status == CDS_AUDIO) || (status == CDS_MIXED))
            //~ {
                //~ ret = true;
            //~ }printf("status %d vs %d\n", status, CDS_NO_INFO);
        //~ }
    //~ } else {
        //~ newdisc = true;
        //~ clear_widgets();
    //~ }
    
    static bool alreadyKnowGood = false; /* check when program just started */
    static bool alreadyCleared = true; /* no need to clear when program just started */
    
    status = ioctl(fd, CDROM_DISC_STATUS, CDSL_CURRENT);
    if (status == CDS_AUDIO || status == CDS_MIXED)
    {
        if (!alreadyKnowGood)
        {
            ret = true;
            alreadyKnowGood = true; /* don't return true again for this disc */
            alreadyCleared = false; /* clear when disc is removed */
        }
    }
    else
    {
        alreadyKnowGood = false; /* return true when good disc inserted */
        if (!alreadyCleared)
        {
            alreadyCleared = true;
            clear_widgets();
        }
    }

    close(fd);
    return ret;
}


void clear_widgets()
{
    // hide the widgets for multiple albums
    gtk_widget_hide(lookup_widget(win_main, "disc"));
    gtk_widget_hide(lookup_widget(win_main, "pick_disc"));
    
    // clear the textboxes
    gtk_entry_set_text(GTK_ENTRY(album_artist), "");
    gtk_entry_set_text(GTK_ENTRY(album_title), "");
    
    // clear the tracklist
    gtk_tree_view_set_model(GTK_TREE_VIEW(tracklist), NULL);
    
    // disable the "rip" button
    gtk_widget_set_sensitive(lookup_widget(win_main, "rip_button"), FALSE);
}


GtkTreeModel * create_model_from_disc(cddb_disc_t * disc)
{
    GtkListStore * store;
    GtkTreeIter iter;
    cddb_track_t * track;
    int seconds;
    char time[6];
    char * track_artist;
    char * track_title;
    
    store = gtk_list_store_new(NUM_COLS, G_TYPE_BOOLEAN, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    
    for (track = cddb_disc_get_track_first(disc); track != NULL; track = cddb_disc_get_track_next(disc))
    {
        seconds = cddb_track_get_length(track);
        snprintf(time, 6, "%02d:%02d", seconds/60, seconds%60);
        
        track_artist = (char*)cddb_track_get_artist(track);
        trim_chars(track_artist, global_prefs->invalid_chars);
        trim_whitespace(track_artist);
        
        track_title = (char*)cddb_track_get_title(track); //!! this returns const char*
        trim_chars(track_title, global_prefs->invalid_chars);
        trim_whitespace(track_title);
        
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
            COL_RIPTRACK, track_format[cddb_track_get_number(track)],
            COL_TRACKNUM, cddb_track_get_number(track),
            COL_TRACKARTIST, track_artist,
            COL_TRACKTITLE, track_title,
            COL_TRACKTIME, time,
            -1);
    }
    
    return GTK_TREE_MODEL(store);
}


void eject_disc(char * cdrom)
{
    int fd;
    
    // open the device
    fd = open(cdrom, O_RDONLY | O_NONBLOCK);
    if (fd < 0)
    {
        fprintf(stderr, "Error: Couldn't open %s\n", cdrom);
        return;
    }

    //~ if (ioctl(fd, CDROM_DRIVE_STATUS, CDSL_CURRENT) == CDS_TRAY_OPEN)
    //~ {
        //~ ioctl(fd, CDROMCLOSETRAY, CDSL_CURRENT);
    //~ } else {
            ioctl(fd, CDROMEJECT, CDSL_CURRENT);
    //~ }
    
    close(fd);
}


GList * lookup_disc(cddb_disc_t * disc)
{
    cddb_conn_t * conn = NULL;
    int num_matches;
    int i;
    GList * matches = NULL;
    
    // set up the connection to the cddb server
    conn = cddb_new();
    if (conn == NULL)
    {
        fprintf(stderr, "out of memory, unable to create connection structure");
        exit(-1);
    }

    // query cddb to find similar discs
    cddb_cache_disable(conn);
    num_matches = cddb_query(conn, disc);
    cddb_cache_enable(conn);
    
    // make a list of all the matches
    for (i=0; i<num_matches; i++)
    {
        cddb_disc_t * possible_match = cddb_disc_clone(disc);
        if (!cddb_read(conn, possible_match))
        {
            cddb_error_print(cddb_errno(conn));
            exit(-1);
        }
        matches = g_list_append(matches, possible_match);
        
        // move to next match
        if (i < num_matches-1)
        {
            if (!cddb_query_next(conn, disc))
            {
                fprintf(stderr, "query index out of bounds");
                exit(-1);
            }
        }
    }

    cddb_destroy(conn);
    
    return matches;
}


cddb_disc_t * read_disc(char * cdrom)
{
    int fd;
    int status;
    struct cdrom_tochdr th;
    struct cdrom_tocentry te;
    int i;
    
    cddb_disc_t * disc = NULL;
    cddb_track_t * track = NULL;

    char trackname[9];

    // open the device
    fd = open(cdrom, O_RDONLY | O_NONBLOCK);
    if (fd < 0)
    {
        fprintf(stderr, "Error: Couldn't open %s\n", cdrom);
        return NULL;
    }

    // read disc status info
    status = ioctl(fd, CDROM_DISC_STATUS, CDSL_CURRENT);
    if ((status == CDS_AUDIO) || (status == CDS_MIXED))
    {
        // see if we can read the disc's table of contents (TOC).
        if (ioctl(fd, CDROMREADTOCHDR, &th) == 0)
        {
#ifdef DEBUG
            printf("starting track: %d\n", th.cdth_trk0);
            printf("ending track: %d\n", th.cdth_trk1);
#endif
            disc = cddb_disc_new();
            if (disc == NULL)
            {
                fprintf(stderr, "out of memory, unable to create disc");
                exit(-1);
            }
            
            te.cdte_format = CDROM_LBA;
            for (i=th.cdth_trk0; i<=th.cdth_trk1; i++)
            {
                te.cdte_track = i;
                if (ioctl(fd, CDROMREADTOCENTRY, &te) == 0)
                {
                    if (te.cdte_ctrl & CDROM_DATA_TRACK)
                    {
                        // track is a DATA track. make sure its "rip" box is not checked by default
                        track_format[i] = FALSE;
                    } else {
                        track_format[i] = TRUE;
                    }

                    track = cddb_track_new();
                    if (track == NULL)
                    {
                        fprintf(stderr, "out of memory, unable to create track");
                        exit(-1);
                    }
                
                    cddb_track_set_frame_offset(track, te.cdte_addr.lba+SECONDS_TO_FRAMES(2));
                    snprintf(trackname, 9, "Track %d", i);
                    cddb_track_set_title(track, trackname);
                    cddb_track_set_artist(track, "Unknown Artist");
                    cddb_disc_add_track(disc, track);
                }
            }
            te.cdte_track = CDROM_LEADOUT;
            if (ioctl(fd, CDROMREADTOCENTRY, &te) == 0)
            {
                cddb_disc_set_length(disc, (te.cdte_addr.lba+SECONDS_TO_FRAMES(2))/SECONDS_TO_FRAMES(1));
            }
        }
    }
    close(fd);

    return disc;
}


void update_tracklist(cddb_disc_t * disc)
{
    GtkTreeModel * model;
    char * disc_artist = (char*)cddb_disc_get_artist(disc);
    char * disc_title = (char*)cddb_disc_get_title(disc);
    cddb_track_t * track;
    bool singleartist;
    
    if (disc_artist != NULL)
    {
        trim_chars(disc_artist, global_prefs->invalid_chars);
        trim_whitespace(disc_artist);
        gtk_entry_set_text(GTK_ENTRY(album_artist), disc_artist);
        
        singleartist = true;
        for (track = cddb_disc_get_track_first(disc); track != NULL; track = cddb_disc_get_track_next(disc))
        {
            if (strcmp(disc_artist, cddb_track_get_artist(track)) != 0)
            {
                singleartist = false;
                break;
            }
        }
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_main, "single_artist")), singleartist);
    }
    if (disc_title != NULL)
    {
        trim_chars(disc_title, global_prefs->invalid_chars);
        trim_whitespace(disc_title);
        gtk_entry_set_text(GTK_ENTRY(album_title), disc_title);
    }
    
    model = create_model_from_disc(disc);
    gtk_tree_view_set_model(GTK_TREE_VIEW(tracklist), model);
    g_object_unref(model);
}


void refresh(char * cdrom, int force)
{
    cddb_disc_t * disc;
    GList * curr;

    if (check_disc(cdrom) || force)
    {
        //~ clear_widgets();
        gtk_widget_set_sensitive(lookup_widget(win_main, "rip_button"), TRUE);
        
        disc = read_disc(cdrom);
        if (disc == NULL)
            return;
        
        // show the temporary info
        gtk_entry_set_text(GTK_ENTRY(album_artist), "Unknown Artist");
        gtk_entry_set_text(GTK_ENTRY(album_title), "Unknown Album");
        update_tracklist(disc);

        // clear out the previous list of matches
        for (curr = g_list_first(disc_matches); curr != NULL; curr = g_list_next(curr))
        {
            cddb_disc_destroy((cddb_disc_t *)curr->data);
        }
        g_list_free(disc_matches);
        
        disc_matches = lookup_disc(disc);
        cddb_disc_destroy(disc);
        
        if (disc_matches == NULL)
            return;
        
        if (g_list_length(disc_matches) > 1)
        {
            // fill in and show the album drop-down box
            GtkListStore * store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
            GtkTreeIter iter;
            GList * curr;
            cddb_disc_t * tempdisc;
            
            for (curr = g_list_first(disc_matches); curr != NULL; curr = g_list_next(curr))
            {
                tempdisc = (cddb_disc_t *)curr->data;
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter,
                    0, cddb_disc_get_artist(tempdisc),
                    1, cddb_disc_get_title(tempdisc),
                    -1);
            }
            gtk_combo_box_set_model(GTK_COMBO_BOX(pick_disc), GTK_TREE_MODEL(store));
            gtk_combo_box_set_active(GTK_COMBO_BOX(pick_disc), 1);
            gtk_combo_box_set_active(GTK_COMBO_BOX(pick_disc), 0);
            
            gtk_widget_show(lookup_widget(win_main, "disc"));
            gtk_widget_show(lookup_widget(win_main, "pick_disc"));
        }
        
        update_tracklist((cddb_disc_t *)g_list_nth_data(disc_matches, 0));
    }
}
