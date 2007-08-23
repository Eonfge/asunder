/*
Asunder

Copyright(C) 2005 Eric Lathrop <eric@ericlathrop.com>
Copyright(C) 2007 Andrew Smith <http://littlesvr.ca/misc/contactandrew.php>

Any code in this file may be redistributed or modified under the terms of
the GNU General Public Licence as published by the Free Software 
Foundation; version 2 of the licence.

*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include "threads.h"
#include "main.h"
#include "prefs.h"
#include "util.h"
#include "wrappers.h"
#include "support.h"
#include "interface.h"

static GMutex * barrier = NULL;
static GCond * available = NULL;
static int counter;

static FILE * playlist_wav = NULL;
static FILE * playlist_mp3 = NULL;
static FILE * playlist_ogg = NULL;
static FILE * playlist_flac = NULL;

int aborted = 0;

static GThread * ripper;
static GThread * encoder;
static GThread * tracker;

static int tracks_to_rip;
static double rip_percent;
static double mp3_percent;
static double ogg_percent;
static double flac_percent;
static int rip_tracks_completed;
static int encode_tracks_completed;

// aborts ripping- stops all the threads and return to normal execution
void abort_threads()
{
    aborted = 1;

    if (cdparanoia_pid != 0) 
        kill(cdparanoia_pid, SIGKILL);
    if (lame_pid != 0) 
        kill(lame_pid, SIGKILL);
    if (oggenc_pid != 0) 
        kill(oggenc_pid, SIGKILL);
    if (flac_pid != 0) 
        kill(flac_pid, SIGKILL);
    
    /* wait until all the worker threads are done */
    while (cdparanoia_pid != 0 || lame_pid != 0 || oggenc_pid != 0 || flac_pid != 0)
    {
#ifdef DEBUG
        printf("w1");
#endif
        usleep(100000);
    }
    
    g_cond_signal(available);
    
#ifdef DEBUG
    printf("Aborting: 1\n");
#endif
    g_thread_join(ripper);
#ifdef DEBUG
    printf("Aborting: 2\n");
#endif
    g_thread_join(encoder);
#ifdef DEBUG
    printf("Aborting: 3\n");
#endif
    g_thread_join(tracker);
#ifdef DEBUG
    printf("Aborting: 4 (All threads joined)\n");
#endif
    
    gtk_widget_hide(win_ripping);
    gdk_flush();
    show_completed_dialog(numCdparanoiaOk + numLameOk + numOggOk + numFlacOk,
                          numCdparanoiaFailed + numLameFailed + numOggFailed + numFlacFailed);
}

// spawn needed threads and begin ripping
void dorip()
{
    aborted = 0;
    counter = 0;
    barrier = g_mutex_new();
    available = g_cond_new();
    rip_percent = 0.0;
    mp3_percent = 0.0;
    ogg_percent = 0.0;
    flac_percent = 0.0;
    rip_tracks_completed = 0;
    encode_tracks_completed = 0;

    const char * albumartist = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_main, "album_artist")));
    const char * albumtitle = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_main, "album_title")));
    char * albumdir = parse_format(global_prefs->format_albumdir, 0, albumartist, albumtitle, NULL);
    char * playlist = parse_format(global_prefs->format_playlist, 0, albumartist, albumtitle, NULL);

    // make sure there's at least one format to rip to
    if (!global_prefs->rip_wav && !global_prefs->rip_mp3 && !global_prefs->rip_ogg && !global_prefs->rip_flac)
    {
        GtkWidget * dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "No ripping/encoding method selected. Please enable one from the \"preferences\" menu.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    // make sure there's some tracks to rip
    GtkListStore * store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(win_main, "tracklist"))));
    GtkTreeIter iter;
    gboolean rowsleft = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
    tracks_to_rip = 0;
    int riptrack;
    while(rowsleft)
    {
        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
            COL_RIPTRACK, &riptrack,
            -1);
        if (riptrack) tracks_to_rip++;
        rowsleft = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
    }
    if (tracks_to_rip == 0)
    {
        GtkWidget * dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "No tracks were selected for ripping/encoding. Please select at least one track.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    
    if (global_prefs->make_albumdir)
    {
#ifdef DEBUG
        printf("Making album directory\n");
#endif
        char * dirpath = make_filename(prefs_get_music_dir(global_prefs), albumdir, NULL, NULL);
        
        if ((mkdir(dirpath, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) != 0) && (errno != EEXIST))
        {
            GtkWidget * dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Unable to create directory \"%s\": %s", dirpath, strerror(errno));
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
        
            free(dirpath);
            return;
        }
        
        free(dirpath);
    }
    if (global_prefs->make_playlist)
    {
#ifdef DEBUG
        printf("Creating playlists\n");
#endif
        if (global_prefs->rip_wav)
        {
            char * filename = make_filename(prefs_get_music_dir(global_prefs), albumdir, playlist, "wav.m3u");
            playlist_wav = fopen(filename, "w");
            
            if (playlist_wav == NULL)
            {
                GtkWidget * dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Unable to create WAV playlist \"%s\": %s", filename, strerror(errno));
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
            } else {
                fprintf(playlist_wav, "#EXTM3U\n");
            }

            free(filename);
        }
        if (global_prefs->rip_mp3)
        {
            char * filename = make_filename(prefs_get_music_dir(global_prefs), albumdir, playlist, "mp3.m3u");
            playlist_mp3 = fopen(filename, "w");
            
            if (playlist_mp3 == NULL)
            {
                GtkWidget * dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Unable to create MP3 playlist \"%s\": %s", filename, strerror(errno));
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
            } else {
                fprintf(playlist_mp3, "#EXTM3U\n");
            }

            free(filename);
        }
        if (global_prefs->rip_ogg)
        {
            char * filename = make_filename(prefs_get_music_dir(global_prefs), albumdir, playlist, "ogg.m3u");
            playlist_ogg = fopen(filename, "w");
            
            if (playlist_ogg == NULL)
            {
                GtkWidget * dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Unable to create OGG playlist \"%s\": %s", filename, strerror(errno));
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
            } else {
                fprintf(playlist_ogg, "#EXTM3U\n");
            }

            free(filename);
        }
        if (global_prefs->rip_flac)
        {
            char * filename = make_filename(prefs_get_music_dir(global_prefs), albumdir, playlist, "flac.m3u");
            playlist_flac = fopen(filename, "w");
            
            if (playlist_flac == NULL)
            {
                GtkWidget * dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Unable to create FLAC playlist \"%s\": %s", filename, strerror(errno));
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
            } else {
                fprintf(playlist_flac, "#EXTM3U\n");
            }

            free(filename);
        }
    }

    free(albumdir);
    free(playlist);
    
    gtk_widget_show(win_ripping);
    
    numCdparanoiaFailed = 0;
    numLameFailed = 0;
    numOggFailed = 0;
    numFlacFailed = 0;
    
    numCdparanoiaOk = 0;
    numLameOk = 0;
    numOggOk = 0;
    numFlacOk = 0;
    
    ripper = g_thread_create(rip, NULL, TRUE, NULL);
    encoder = g_thread_create(encode, NULL, TRUE, NULL);
    tracker = g_thread_create(track, NULL, TRUE, NULL);
}

// the thread that handles ripping tracks to WAV files
gpointer rip(gpointer data)
{
    GtkTreeIter iter;

    int riptrack;
    int tracknum;
    const char * trackartist;
    const char * tracktitle;
    
    char * albumdir = NULL;
    char * musicfilename = NULL;
    char * wavfilename = NULL;

    gdk_threads_enter();
        GtkListStore * store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(win_main, "tracklist"))));
        gboolean single_artist = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_main, "single_artist")));
        const char * albumartist = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_main, "album_artist")));
        const char * albumtitle = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_main, "album_title")));

        gboolean rowsleft = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
    gdk_threads_leave();
    while(rowsleft)
    {
        gdk_threads_enter();
        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
            COL_RIPTRACK, &riptrack,
            COL_TRACKNUM, &tracknum,
            COL_TRACKARTIST, &trackartist,
            COL_TRACKTITLE, &tracktitle,
            -1);
        gdk_threads_leave();
        
        if (single_artist)
        {
            trackartist = albumartist;
        }

        if (riptrack)
        {
            if (global_prefs->make_albumdir)
            {
                albumdir = parse_format(global_prefs->format_albumdir, 0, albumartist, albumtitle, NULL);
            }
            musicfilename = parse_format(global_prefs->format_music, tracknum, trackartist, albumtitle, tracktitle);
            gdk_threads_enter();
            wavfilename = make_filename(prefs_get_music_dir(global_prefs), albumdir, musicfilename, "wav");
            gdk_threads_leave();

#ifdef DEBUG
            printf("Ripping track %d to \"%s\"\n", tracknum, wavfilename);
#endif
            if (aborted) g_thread_exit(NULL);
            cdparanoia(global_prefs->cdrom, tracknum, wavfilename, &rip_percent);

            free(albumdir);
            free(musicfilename);
            free(wavfilename);
            
            rip_percent = 0.0;
            rip_tracks_completed++;
        }

        g_mutex_lock(barrier);
        counter++;
        g_mutex_unlock(barrier);
        g_cond_signal(available);
        
        if (aborted) g_thread_exit(NULL);
        
        gdk_threads_enter();
        rowsleft = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
        gdk_threads_leave();
    }
    
    // no more tracks to rip, safe to eject
    if (global_prefs->eject_on_done)
    {
        eject_disc(global_prefs->cdrom);
    }
    
    return NULL;
}

// the thread that handles encoding WAV files to all other formats
gpointer encode(gpointer data)
{
    GtkTreeIter iter;

    int riptrack;
    int tracknum;
    char * trackartist = NULL;
    char * tracktitle = NULL;
    char * tracktime = NULL;
    int min;
    int sec;
    int i;
    
    char * album_artist = NULL;
    char * album_title = NULL;
    
    char * albumdir = NULL;
    char * musicfilename = NULL;
    char * wavfilename = NULL;
    char * mp3filename = NULL;
    char * oggfilename = NULL;
    char * flacfilename = NULL;

    gdk_threads_enter();
        GtkListStore * store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(win_main, "tracklist"))));
        gboolean single_artist = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_main, "single_artist")));
        
        const char * temp_album_artist = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_main, "album_artist")));
        album_artist = malloc(sizeof(char) * (strlen(temp_album_artist)+1));
        if (album_artist == NULL)
        {
            fprintf(stderr, "malloc(sizeof(char) * (strlen(temp_album_artist)+1)) failed\n");
            exit(-1);
        }
        strncpy(album_artist, temp_album_artist, strlen(temp_album_artist)+1);
        
        const char * temp_album_title = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_main, "album_title")));
        album_title = malloc(sizeof(char) * (strlen(temp_album_title)+1));
        if (album_title == NULL)
        {
            fprintf(stderr, "malloc(sizeof(char) * (strlen(temp_album_artist)+1)) failed\n");
            exit(-1);
        }
        strncpy(album_title, temp_album_title, strlen(temp_album_title)+1);

        gboolean rowsleft = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
    gdk_threads_leave();
    while(rowsleft)
    {
        g_mutex_lock(barrier);
        while ((counter < 1) && (!aborted))
        {
            g_cond_wait(available, barrier);
        }
        counter--;
        g_mutex_unlock(barrier);
        if (aborted) g_thread_exit(NULL);

        gdk_threads_enter();
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
                COL_RIPTRACK, &riptrack,
                COL_TRACKNUM, &tracknum,
                COL_TRACKARTIST, &trackartist,
                COL_TRACKTITLE, &tracktitle,
                COL_TRACKTIME, &tracktime,
                -1);
        gdk_threads_leave();
        sscanf(tracktime, "%d:%d", &min, &sec);
        
        if (single_artist)
        {
            trackartist = album_artist;
        }

        if (riptrack)
        {
            if (global_prefs->make_albumdir)
            {
                albumdir = parse_format(global_prefs->format_albumdir, 0, album_artist, album_title, NULL);
            }
            musicfilename = parse_format(global_prefs->format_music, tracknum, trackartist, album_title, tracktitle);
            gdk_threads_enter();
                if (global_prefs->make_albumdir)
                {
                    wavfilename = make_filename(prefs_get_music_dir(global_prefs), albumdir, musicfilename, "wav");
                    mp3filename = make_filename(prefs_get_music_dir(global_prefs), albumdir, musicfilename, "mp3");
                    oggfilename = make_filename(prefs_get_music_dir(global_prefs), albumdir, musicfilename, "ogg");
                    flacfilename = make_filename(prefs_get_music_dir(global_prefs), albumdir, musicfilename, "flac");
                } else {
                    wavfilename = make_filename(prefs_get_music_dir(global_prefs), NULL, musicfilename, "wav");
                    mp3filename = make_filename(prefs_get_music_dir(global_prefs), NULL, musicfilename, "mp3");
                    oggfilename = make_filename(prefs_get_music_dir(global_prefs), NULL, musicfilename, "ogg");
                    flacfilename = make_filename(prefs_get_music_dir(global_prefs), NULL, musicfilename, "flac");
                }
            gdk_threads_leave();
            
            if (global_prefs->rip_mp3)
            {
#ifdef DEBUG
                printf("Encoding track %d to \"%s\"\n", tracknum, mp3filename);
#endif
                if (aborted) g_thread_exit(NULL);
                lame(tracknum, trackartist, album_title, tracktitle, wavfilename, mp3filename, global_prefs->mp3_vbr, int_to_bitrate(global_prefs->mp3_bitrate), &mp3_percent);
                if (aborted) g_thread_exit(NULL);

                if (playlist_mp3)
                {
                    fprintf(playlist_mp3, "#EXTINF:%d,%s - %s\n", (min*60)+sec, trackartist, tracktitle);
                    fprintf(playlist_mp3, "%s\n", basename(mp3filename));
                    fflush(playlist_mp3);
                }
            }
            if (global_prefs->rip_ogg)
            {
#ifdef DEBUG
                printf("Encoding track %d to \"%s\"\n", tracknum, oggfilename);
#endif
                if (aborted) g_thread_exit(NULL);
                oggenc(tracknum, trackartist, album_title, tracktitle, wavfilename, oggfilename, global_prefs->ogg_quality, &ogg_percent);
                if (aborted) g_thread_exit(NULL);

                if (playlist_ogg)
                {
                    fprintf(playlist_ogg, "#EXTINF:%d,%s - %s\n", (min*60)+sec, trackartist, tracktitle);
                    fprintf(playlist_ogg, "%s\n", basename(oggfilename));
                    fflush(playlist_ogg);
                }
            }
            if (global_prefs->rip_flac)
            {
#ifdef DEBUG
                printf("Encoding track %d to \"%s\"\n", tracknum, flacfilename);
#endif
                if (aborted) g_thread_exit(NULL);
                flac(tracknum, trackartist, album_title, tracktitle, wavfilename, flacfilename, global_prefs->flac_compression, &flac_percent);
                if (aborted) g_thread_exit(NULL);
                
                if (playlist_flac)
                {
                    for (i=strlen(flacfilename); ((i>0) && (flacfilename[i] != '/')); i--);
                    fprintf(playlist_flac, "#EXTINF:%d,%s - %s\n", (min*60)+sec, trackartist, tracktitle);
                    fprintf(playlist_flac, "%s\n", basename(flacfilename));
                    fflush(playlist_flac);
                }
            }
            if (!global_prefs->rip_wav)
            {
#ifdef DEBUG
                printf("Removing track %d WAV file\n", tracknum);
#endif
                if (unlink(wavfilename) != 0)
                {
                    GtkWidget * dialog;
                    dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), 
                                        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
                                        "Unable to delete WAV file \"%s\": %s", wavfilename, strerror(errno));
                    gtk_dialog_run(GTK_DIALOG(dialog));
                    gtk_widget_destroy(dialog);
                }
            } else {
                if (playlist_wav)
                {
                    for (i=strlen(wavfilename); ((i>0) && (wavfilename[i] != '/')); i--);
                    fprintf(playlist_wav, "#EXTINF:%d,%s - %s\n", (min*60)+sec, trackartist, tracktitle);
                    fprintf(playlist_wav, "%s\n", basename(wavfilename));
                    fflush(playlist_wav);
                }
            }

            free(albumdir);
            free(musicfilename);
            free(wavfilename);
            free(mp3filename);
            free(oggfilename);
            free(flacfilename);
            
            mp3_percent = 0.0;
            ogg_percent = 0.0;
            flac_percent = 0.0;
            encode_tracks_completed++;
        }
        
        if (aborted) g_thread_exit(NULL);
        
        gdk_threads_enter();
            rowsleft = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
        gdk_threads_leave();
    }
    
    free(album_artist);
    free(album_title);
    
    if (playlist_wav) fclose(playlist_wav);
    playlist_wav = NULL;
    if (playlist_mp3) fclose(playlist_mp3);
    playlist_mp3 = NULL;
    if (playlist_ogg) fclose(playlist_ogg);
    playlist_ogg = NULL;
    if (playlist_flac) fclose(playlist_flac);
    playlist_flac = NULL;
    
    g_mutex_free(barrier);
    barrier = NULL;
    g_cond_free(available);
    available = NULL;
    
    /* wait until all the worker threads are done */
    while (cdparanoia_pid != 0 || lame_pid != 0 || oggenc_pid != 0 || flac_pid != 0)
    {
#ifdef DEBUG
        printf("w2");
#endif
        usleep(100000);
    }
    
    aborted = 1; // so the tracker thread will exit
    
    gdk_threads_enter();
        gtk_widget_hide(win_ripping);
        gdk_flush();
        show_completed_dialog(numCdparanoiaOk + numLameOk + numOggOk + numFlacOk,
                              numCdparanoiaFailed + numLameFailed + numOggFailed + numFlacFailed);
    gdk_threads_leave();
    
    return NULL;
}

// the thread that calculates the progress of the other threads and updates the progress bars
gpointer track(gpointer data)
{
    int parts = 1;
    if (global_prefs->rip_mp3) parts++;
    if (global_prefs->rip_ogg) parts++;
    if (global_prefs->rip_flac) parts++;
    
    gdk_threads_enter();
        GtkProgressBar * progress_total = GTK_PROGRESS_BAR(lookup_widget(win_ripping, "progress_total"));
        GtkProgressBar * progress_rip = GTK_PROGRESS_BAR(lookup_widget(win_ripping, "progress_rip"));
        GtkProgressBar * progress_encode = GTK_PROGRESS_BAR(lookup_widget(win_ripping, "progress_encode"));
        
        gtk_progress_bar_set_fraction(progress_total, 0.0);
        gtk_progress_bar_set_text(progress_total, _("Waiting..."));
        gtk_progress_bar_set_fraction(progress_rip, 0.0);
        gtk_progress_bar_set_text(progress_rip, _("Waiting..."));
        if (parts > 1)
        {
            gtk_progress_bar_set_fraction(progress_encode, 0.0);
            gtk_progress_bar_set_text(progress_encode, _("Waiting..."));
        } else {
            gtk_progress_bar_set_fraction(progress_encode, 1.0);
            gtk_progress_bar_set_text(progress_encode, "100% (0/0)");
        }
    gdk_threads_leave();
    
    double prip;
    char srip[13];
    double pencode;
    char sencode[13];
    double ptotal;
    char stotal[5];

    while (!aborted)
    {
#ifdef DEBUG
        printf("completed tracks %d, rip %.2f%%; encoded tracks %d, mp3 %.2f%% ogg %.2f%% flac %.2f%%\n", rip_tracks_completed, rip_percent, encode_tracks_completed, mp3_percent, ogg_percent, flac_percent);
#endif
        prip = (rip_tracks_completed+rip_percent) / tracks_to_rip;
        snprintf(srip, 13, "%d%% (%d/%d)", (int)(prip*100), rip_tracks_completed, tracks_to_rip);
        if (parts > 1)
        {
            pencode = ((double)encode_tracks_completed/(double)tracks_to_rip) + ((mp3_percent+ogg_percent+flac_percent)/(parts-1)/tracks_to_rip);
            snprintf(sencode, 13, "%d%% (%d/%d)", (int)(pencode*100), encode_tracks_completed, tracks_to_rip);
            ptotal = prip/parts + pencode*(parts-1)/parts;
        } else {
            ptotal = prip;
        }
        snprintf(stotal, 5, "%d%%", (int)(ptotal*100));
        
        gdk_threads_enter();
            gtk_progress_bar_set_fraction(progress_rip, prip);
            gtk_progress_bar_set_text(progress_rip, srip);
            if (parts > 1)
            {
                gtk_progress_bar_set_fraction(progress_encode, pencode);
                gtk_progress_bar_set_text(progress_encode, sencode);
            }
            gtk_progress_bar_set_fraction(progress_total, ptotal);
            gtk_progress_bar_set_text(progress_total, stotal);
        gdk_threads_leave();
        
        usleep(100000);
    }
    
    return NULL;
}

