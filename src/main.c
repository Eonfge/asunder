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

#include "main.h"
#include "interface.h"
#include "support.h"
#include "prefs.h"
#include "callbacks.h"


cddb_disc_t * current_disc = NULL;
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


// scan the cdrom device for a disc
// if the tray has been opened since last time we were called
//     then we go to CDDB, and update the widgets
// if (force == TRUE)
//     then we updated everything no matter what
void check_disc(char * cdrom, int force)
{
	static int newdisc = 1;

	int fd;
	int status;
	struct cdrom_tochdr th;
	struct cdrom_tocentry te;
	int i;
	
	cddb_disc_t * disc = NULL;
	cddb_track_t * track = NULL;

	char trackname[9];

	if (force)
	{
		newdisc = 1;
	}
	
	// open the device
	fd = open(cdrom, O_RDONLY | O_NONBLOCK);
	if (fd < 0)
	{
		fprintf(stderr, "Error: Couldn't open %s\n", cdrom);
		return;
	}
	// read the drive status info
	if (ioctl(fd, CDROM_DRIVE_STATUS, CDSL_CURRENT) == CDS_DISC_OK)
	{
		if (newdisc)
		{
			newdisc = 0;

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
					
					close(fd);
					
					gtk_widget_hide(lookup_widget(win_main, "disc"));
					gtk_widget_hide(lookup_widget(win_main, "pick_disc"));
					gtk_entry_set_text(GTK_ENTRY(album_artist), "Unknown Artist");
					gtk_entry_set_text(GTK_ENTRY(album_title), "Unknown Album");
					
					// show the temporary info
					update_tracklist(disc);
					gtk_widget_set_sensitive(lookup_widget(win_main, "rip_button"), TRUE);

					lookup_disc(disc);
					cddb_disc_destroy(disc);
				} else {
					close(fd);
				}
			} else {
				close(fd);
			}
		} else {
			close(fd);
		}
	} else {
		newdisc = 1;
		close(fd);
		
		gtk_entry_set_text(GTK_ENTRY(album_artist), "");
		gtk_entry_set_text(GTK_ENTRY(album_title), "");
		gtk_tree_view_set_model(GTK_TREE_VIEW(tracklist), NULL);
		gtk_widget_set_sensitive(lookup_widget(win_main, "rip_button"), FALSE);
	}
}


// looks up the given cddb_disc_t in the online database, and fills in the values
void lookup_disc(cddb_disc_t * disc)
{
	cddb_conn_t * conn = NULL;
	int num_matches;
	int i;

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
		disc_matches = g_list_append(disc_matches, possible_match);
		
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
	
	if (num_matches == 1) {
		// update the current disc
		if (current_disc)
		{
			cddb_disc_destroy(current_disc);
		}
		current_disc = g_list_first(disc_matches)->data;
		g_list_free(disc_matches);
		disc_matches = NULL;
		
		// fill with match's data
		if (!cddb_read(conn, current_disc))
		{
			cddb_error_print(cddb_errno(conn));
			exit(-1);
		}

		// update the main window
		update_tracklist(current_disc);
	} else if (num_matches > 1) {
		GtkListStore * store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
		GtkCellRenderer * renderer;
		GtkTreeIter iter;
		GList * curr;
		cddb_disc_t * tempdisc;

		renderer = gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(pick_disc), renderer, TRUE);
		gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(pick_disc), renderer,
                                "text", 0,
                                NULL);
		renderer = gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(pick_disc), renderer, TRUE);
		gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(pick_disc), renderer,
                                "text", 1,
                                NULL);

								
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
		gtk_combo_box_set_active(GTK_COMBO_BOX(pick_disc), 0);
		
		gtk_widget_show(lookup_widget(win_main, "disc"));
		gtk_widget_show(lookup_widget(win_main, "pick_disc"));
	}

	cddb_destroy(conn);
}


// creates a tree model that represents the data in the cddb_disc_t
GtkTreeModel * create_model_from_disc(cddb_disc_t * disc)
{
	GtkListStore * store;
	GtkTreeIter iter;
	cddb_track_t * track;
	int seconds;
	char time[6];
	const char * track_artist;
	char * track_title;
	
	store = gtk_list_store_new(NUM_COLS, G_TYPE_BOOLEAN, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	
	for (track = cddb_disc_get_track_first(disc); track != NULL; track = cddb_disc_get_track_next(disc))
	{
		seconds = cddb_track_get_length(track);
		snprintf(time, 6, "%02d:%02d", seconds/60, seconds%60);
		
		track_artist = cddb_track_get_artist(track);
		trim_chars(track_artist, global_prefs->invalid_chars);
		trim_whitespace(track_artist);
		
		track_title = cddb_track_get_title(track); //!! this returns const char*
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


// updates all the necessary widgets with the data for the given cddb_disc_t
void update_tracklist(cddb_disc_t * disc)
{
	GtkTreeModel * model;
	char * disc_artist = cddb_disc_get_artist(disc); //!! this returns const char*
	char * disc_title = cddb_disc_get_title(disc); //!! this returns const char*
	cddb_track_t * track;
	int singleartist;

	if (disc_artist != NULL)
	{
		trim_chars(disc_artist, global_prefs->invalid_chars);
		trim_whitespace(disc_artist);
		gtk_entry_set_text(GTK_ENTRY(album_artist), disc_artist);
		
		singleartist = 1;
		for (track = cddb_disc_get_track_first(disc); track != NULL; track = cddb_disc_get_track_next(disc))
		{
			if (strcmp(disc_artist, cddb_track_get_artist(track)) != 0)
			{
				singleartist = 0;
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


int main(int argc, char *argv[])
{
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	cddb_disc_t * disc;
	
	gtk_set_locale();
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
					"Rip", renderer, "active", COL_RIPTRACK, NULL);

	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tracklist), -1, 
					"Track", renderer, "text", COL_TRACKNUM, NULL);

	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited", (GCallback) artist_edited, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tracklist), -1, 
					"Artist", renderer, "text", COL_TRACKARTIST, NULL);

	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited", (GCallback) title_edited, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tracklist), -1, 
					"Title", renderer, "text", COL_TRACKTITLE, NULL);

	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tracklist), -1, 
					"Time", renderer, "text", COL_TRACKTIME, NULL);

	// disable the "rip" button
	// it will be enabled when check_disc() finds a disc in the drive
	gtk_widget_set_sensitive(lookup_widget(win_main, "rip_button"), FALSE);
	
	win_ripping = create_ripping();

	if (!program_exists("cdparanoia"))
	{
		GtkWidget * dialog;

		dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
				"\"cdparanoia\" was not found in your path.\n\n"
				PACKAGE" requires cdparanoia to rip CDs. Please download cdparanoia "
				"from http://www.xiph.org/paranoia/ and install it.");
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

