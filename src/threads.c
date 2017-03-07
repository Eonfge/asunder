/*
Asunder

Copyright(C) 2005 Eric Lathrop <eric@ericlathrop.com>
Copyright(C) 2007 Andrew Smith <http://littlesvr.ca/contact.php>

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
#include "completion.h"

static GMutex * barrier = NULL;
static GCond * available = NULL;
static int counter;

static FILE * playlist_wav = NULL;
static FILE * playlist_mp3 = NULL;
static FILE * playlist_ogg = NULL;
static FILE * playlist_opus = NULL;
static FILE * playlist_flac = NULL;
static FILE * playlist_wavpack = NULL;
static FILE * playlist_monkey = NULL;
static FILE * playlist_musepack = NULL;
static FILE * playlist_aac = NULL;

/* ripping or encoding, so that can know not to clear the tracklist on eject */
bool working;
/* for canceling */
bool aborted;
/* for stopping the tracking thread */
bool allDone;

static GThread * ripper;
static GThread * encoder;
static GThread * tracker;

static int tracks_to_rip;
static double rip_percent;
static double mp3_percent;
static double ogg_percent;
static double opus_percent;
static double flac_percent;
static double wavpack_percent;
static double monkey_percent;
static double musepack_percent;
static double aac_percent;
static int rip_tracks_completed;
static int encode_tracks_completed;

extern bool overwriteAll;
extern bool overwriteNone;

// aborts ripping- stops all the threads and return to normal execution
void abort_threads()
{
    aborted = true;

    if (cdparanoia_pid != 0) 
        kill(cdparanoia_pid, SIGKILL);
    if (lame_pid != 0) 
        kill(lame_pid, SIGKILL);
    if (oggenc_pid != 0) 
        kill(oggenc_pid, SIGKILL);
    if (opusenc_pid !=0)
        kill(opusenc_pid, SIGKILL);
    if (flac_pid != 0) 
        kill(flac_pid, SIGKILL);
    if (wavpack_pid != 0) 
        kill(wavpack_pid, SIGKILL);
    if (monkey_pid != 0) 
        kill(monkey_pid, SIGKILL);
    if (musepack_pid != 0) 
        kill(musepack_pid, SIGKILL);
    if (aac_pid != 0) 
        kill(aac_pid, SIGKILL);
    
    /* wait until all the worker threads are done */
    while (cdparanoia_pid != 0 || lame_pid != 0 || oggenc_pid != 0 || 
           opusenc_pid != 0 || flac_pid != 0 || wavpack_pid != 0 || monkey_pid != 0 ||
           musepack_pid != 0 || aac_pid != 0)
    {
        debugLog("w1");
        usleep(100000);
    }
    
    g_cond_signal(available);
    
    debugLog("Aborting: 1\n");
    g_thread_join(ripper);
    debugLog("Aborting: 2\n");
    g_thread_join(encoder);
    debugLog("Aborting: 3\n");
    g_thread_join(tracker);
    debugLog("Aborting: 4 (All threads joined)\n");
    
    gtk_window_set_title(GTK_WINDOW(win_main), "Asunder");
    
    gtk_widget_hide(win_ripping);
    gdk_flush();
    working = false;
    
    show_completed_dialog(numCdparanoiaOk + numLameOk + numOggOk + numOpusOk + numFlacOk + numWavpackOk + numMonkeyOk + numMusepackOk + numAacOk,
                          numCdparanoiaFailed + numLameFailed + numOggFailed + numOpusFailed + numFlacFailed + numWavpackFailed + numMonkeyFailed + numMusepackFailed + numAacFailed);
}

// spawn needed threads and begin ripping
void dorip()
{
    char logStr[1024];
    working = true;
    aborted = false;
    allDone = false;
    counter = 0;
    barrier = g_mutex_new();
    available = g_cond_new();
    rip_percent = 0.0;
    mp3_percent = 0.0;
    ogg_percent = 0.0;
    opus_percent = 0.0;
    flac_percent = 0.0;
    wavpack_percent = 0.0;
    monkey_percent = 0.0;
    musepack_percent = 0.0;
    aac_percent = 0.0;
    rip_tracks_completed = 0;
    encode_tracks_completed = 0;

    GtkTreeIter iter;
    GtkListStore * store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(win_main, "tracklist"))));
    gboolean rowsleft = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);

    const char * albumartist = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_main, "album_artist")));
    const char * albumtitle = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_main, "album_title")));
    const char * albumyear = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_main, "album_year")));
    const char * albumgenre = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_main, "album_genre")));
    
    //Trimmed for use in filenames	//mrpl
    char * albumartist_trimmed = strdup(albumartist);
    trim_chars(albumartist_trimmed, BADCHARS);
    char * albumtitle_trimmed = strdup(albumtitle);
    trim_chars(albumtitle_trimmed, BADCHARS);
    char * albumgenre_trimmed = strdup(albumgenre);
    trim_chars(albumgenre_trimmed, BADCHARS);

    //char * albumdir = parse_format(global_prefs->format_albumdir, 0, albumyear, albumartist, albumtitle, albumgenre, NULL);
    //char * playlist = parse_format(global_prefs->format_playlist, 0, albumyear, albumartist, albumtitle, albumgenre, NULL);
    char * albumdir = parse_format(global_prefs->format_albumdir, 0, albumyear, albumartist_trimmed, albumtitle_trimmed, albumgenre_trimmed, NULL);
    char * playlist = parse_format(global_prefs->format_playlist, 0, albumyear, albumartist_trimmed, albumtitle_trimmed, albumgenre_trimmed, NULL);
    
    overwriteAll = false;
    overwriteNone = false;
    
    // make sure there's at least one format to rip to
    if (!global_prefs->rip_wav && !global_prefs->rip_mp3 && !global_prefs->rip_ogg && !global_prefs->rip_opus &&
        !global_prefs->rip_flac && !global_prefs->rip_wavpack && !global_prefs->rip_monkey &&
        !global_prefs->rip_musepack && !global_prefs->rip_aac)
    {
        GtkWidget * dialog;
        dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), 
                                        GTK_DIALOG_DESTROY_WITH_PARENT, 
                                        GTK_MESSAGE_ERROR, 
                                        GTK_BUTTONS_OK, 
                                        _("No ripping/encoding method selected. Please enable one from the "
                                        "'Preferences' menu."));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        free(albumartist_trimmed);
        free(albumtitle_trimmed);
        free(albumgenre_trimmed);
        free(albumdir);
        free(playlist);
        return;
    }
    
    // make sure there's some tracks to rip
    rowsleft = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
    tracks_to_rip = 0;
    int riptrack;
    while(rowsleft)
    {
        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
                           COL_RIPTRACK, &riptrack,
                           -1);
        if (riptrack) 
            tracks_to_rip++;
        rowsleft = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
    }
    if (tracks_to_rip == 0)
    {
        GtkWidget * dialog;
        dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), 
                                        GTK_DIALOG_DESTROY_WITH_PARENT, 
                                        GTK_MESSAGE_ERROR, 
                                        GTK_BUTTONS_OK, 
                                        _("No tracks were selected for ripping/encoding. "
                                        "Please select at least one track."));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        free(albumdir);
        free(playlist);
        free(albumartist_trimmed);
        free(albumtitle_trimmed);
        free(albumgenre_trimmed);
        return;
    }
    
    /* CREATE the album directory */
    char * dirpath = make_filename(prefs_get_music_dir(global_prefs), albumdir, NULL, NULL);
    
    snprintf(logStr, 1024, "Making album directory '%s'\n", dirpath);
    debugLog(logStr);
    
    if ( recursive_mkdir(dirpath, S_IRWXU|S_IRWXG|S_IRWXO) != 0 && 
         errno != EEXIST )
    {
        GtkWidget * dialog;
        dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), 
                                        GTK_DIALOG_DESTROY_WITH_PARENT, 
                                        GTK_MESSAGE_ERROR, 
                                        GTK_BUTTONS_OK, 
                                        "Unable to create directory '%s': %s", 
                                        dirpath, strerror(errno));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        free(dirpath);
        free(albumdir);
        free(playlist);
        free(albumartist_trimmed);
        free(albumtitle_trimmed);
        free(albumgenre_trimmed);
        return;
    }
    
    free(dirpath);
    /* END CREATE the album directory */
    
    if (global_prefs->make_playlist)
    {
        debugLog("Creating playlists\n");
        
        if (global_prefs->rip_wav)
        {
            char * filename = make_filename(prefs_get_music_dir(global_prefs), albumdir, playlist, "wav.m3u");
            
            make_playlist(filename, &playlist_wav);
            
            free(filename);
        }
        if (global_prefs->rip_mp3)
        {
            char * filename = make_filename(prefs_get_music_dir(global_prefs), albumdir, playlist, "mp3.m3u");
            
            make_playlist(filename, &playlist_mp3);
            
            free(filename);
        }
        if (global_prefs->rip_ogg)
        {
            char * filename = make_filename(prefs_get_music_dir(global_prefs), albumdir, playlist, "ogg.m3u");
            
            make_playlist(filename, &playlist_ogg);
            
            free(filename);
        }
        if (global_prefs->rip_opus)
        {
            char * filename = make_filename(prefs_get_music_dir(global_prefs), albumdir, playlist, "opus.m3u");
            
            make_playlist(filename, &playlist_opus);
            
            free(filename);
        }
        if (global_prefs->rip_flac)
        {
            char * filename = make_filename(prefs_get_music_dir(global_prefs), albumdir, playlist, "flac.m3u");
            
            make_playlist(filename, &playlist_flac);
            
            free(filename);
        }
        if (global_prefs->rip_wavpack)
        {
            char * filename = make_filename(prefs_get_music_dir(global_prefs), albumdir, playlist, "wv.m3u");
            
            make_playlist(filename, &playlist_wavpack);
            
            free(filename);
        }
        if (global_prefs->rip_monkey)
        {
            char * filename = make_filename(prefs_get_music_dir(global_prefs), albumdir, playlist, "ape.m3u");
            
            make_playlist(filename, &playlist_monkey);
            
            free(filename);
        }
        if (global_prefs->rip_musepack)
        {
            char * filename = make_filename(prefs_get_music_dir(global_prefs), albumdir, playlist, "mpc.m3u");
            
            make_playlist(filename, &playlist_monkey);
            
            free(filename);
        }
        if (global_prefs->rip_aac)
        {
            char * filename = make_filename(prefs_get_music_dir(global_prefs), albumdir, playlist, "m4a.m3u");
            
            make_playlist(filename, &playlist_monkey);
            
            free(filename);
        }
    }
    
    free(albumdir);
    free(playlist);
    free(albumartist_trimmed);
    free(albumtitle_trimmed);
    free(albumgenre_trimmed);
    
    gtk_widget_show(win_ripping);
    
    numCdparanoiaFailed = 0;
    numLameFailed = 0;
    numOggFailed = 0;
    numOpusFailed = 0;
    numFlacFailed = 0;
    numWavpackFailed = 0;
    numMonkeyFailed = 0;
    numMusepackFailed = 0;
    numAacFailed = 0;
    
    numCdparanoiaOk = 0;
    numLameOk = 0;
    numOggOk = 0;
    numOpusOk = 0;
    numFlacOk = 0;
    numWavpackOk = 0;
    numMonkeyOk = 0;
    numMusepackOk = 0;
    numAacOk = 0;
    
    ripper = g_thread_create(rip, NULL, TRUE, NULL);
    encoder = g_thread_create(encode, NULL, TRUE, NULL);
    tracker = g_thread_create(track, NULL, TRUE, NULL);
}

// the thread that handles ripping tracks to WAV files
gpointer rip(gpointer data)
{
    char logStr[1024];
    GtkTreeIter iter;
    
    int riptrack;
    int tracknum;
    const char * trackartist;
    const char * tracktitle;
    char * trackartist_trimmed;
    char * tracktitle_trimmed;
    
    char * albumdir = NULL;
    char * musicfilename = NULL;
    char * wavfilename = NULL;

    gdk_threads_enter();
        GtkListStore * store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(win_main, "tracklist"))));
        gboolean single_artist = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_main, "single_artist")));
        const char * albumartist = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_main, "album_artist")));
        const char * albumtitle = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_main, "album_title")));
        const char * albumyear = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_main, "album_year")));
        const char * albumgenre = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_main, "album_genre")));
        
        gboolean rowsleft = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
    gdk_threads_leave();
    
    //Trimmed for use in filenames	//mrpl
    char * albumartist_trimmed = strdup(albumartist);
    trim_chars(albumartist_trimmed, BADCHARS);
    char * albumtitle_trimmed = strdup(albumtitle);
    trim_chars(albumtitle_trimmed, BADCHARS);
    char * albumgenre_trimmed = strdup(albumgenre);
    trim_chars(albumgenre_trimmed, BADCHARS);
    
    while(rowsleft)
    {
        const char * trackyear = 0;
        gdk_threads_enter();
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
                COL_RIPTRACK, &riptrack,
                COL_TRACKNUM, &tracknum,
                COL_TRACKARTIST, &trackartist,
                COL_TRACKTITLE, &tracktitle,
                COL_YEAR, &trackyear,
                -1);
        gdk_threads_leave();
        
        if (single_artist)
        {
            trackartist = albumartist;
        }
        
        if (riptrack)
        {
            //Trimmed for use in filenames	//mrpl
            trackartist_trimmed = strdup(trackartist);
            trim_chars(trackartist_trimmed, BADCHARS);
            tracktitle_trimmed = strdup(tracktitle);
            trim_chars(tracktitle_trimmed, BADCHARS);
            
            //~albumdir = parse_format(global_prefs->format_albumdir, 0, albumyear, albumartist, albumtitle, albumgenre, NULL);
            //~musicfilename = parse_format(global_prefs->format_music, tracknum, trackyear, trackartist, albumtitle, tracktitle);
            //~musicfilename = parse_format(global_prefs->format_music, tracknum, albumyear, trackartist, albumtitle, albumgenre, tracktitle);            
            albumdir = parse_format(global_prefs->format_albumdir, 0, albumyear, albumartist_trimmed, albumtitle_trimmed, albumgenre_trimmed, NULL);
            musicfilename = parse_format(global_prefs->format_music, tracknum, albumyear, trackartist_trimmed, albumtitle_trimmed, albumgenre_trimmed, tracktitle_trimmed);
            wavfilename = make_filename(prefs_get_music_dir(global_prefs), albumdir, musicfilename, "wav");
            
            snprintf(logStr, 1024, "Ripping track %d to \"%s\"\n", tracknum, wavfilename);
            debugLog(logStr);
            
            if (aborted) g_thread_exit(NULL);
            
            struct stat statStruct;
            int rc;
            bool doRip;
            
            rc = stat(wavfilename, &statStruct);
            if(rc == 0)
            {
                gdk_threads_enter();
                    if(confirmOverwrite(wavfilename))
                        doRip = true;
                    else
                        doRip = false;
                gdk_threads_leave();
            }
            else
                doRip = true;
            
            if(doRip)
                cdparanoia(global_prefs->cdrom, tracknum, wavfilename, &rip_percent);
            
            free(albumdir);
            free(musicfilename);
            free(wavfilename);
            free(trackartist_trimmed);
            free(tracktitle_trimmed);
            
            rip_percent = 0.0;
            rip_tracks_completed++;
        }
        
        debugLog("rip() waiting for barrier\n");
        g_mutex_lock(barrier);
        counter++;
        debugLog("rip() done waiting\n");
        g_mutex_unlock(barrier);
        debugLog("rip() signaling available\n");
        g_cond_signal(available);
        
        if (aborted)
            g_thread_exit(NULL);
        
        gdk_threads_enter();
            rowsleft = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
        gdk_threads_leave();
    }
    
    free(albumartist_trimmed);
    free(albumtitle_trimmed);
    free(albumgenre_trimmed);
    
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
    char logStr[1024];
    GtkTreeIter iter;

    int riptrack;
    int tracknum;
    char* trackartist = NULL;
    char* tracktitle = NULL;
    char* tracktime = NULL;
    char* genre = NULL;
    char* trackyear;
    int min;
    int sec;
    int rc;
    
    char* album_artist = NULL;
    char* album_title = NULL;
    char* album_genre = NULL;		// lnr
    char* album_year = NULL;
    
    char* album_artist_trimmed = NULL;
    char* album_title_trimmed = NULL;
    char* album_genre_trimmed = NULL;
    char* trackartist_trimmed = NULL;
    char* tracktitle_trimmed = NULL;
    char* genre_trimmed = NULL;
    
    char* albumdir = NULL;
    char* musicfilename = NULL;
    char* wavfilename = NULL;
    char* mp3filename = NULL;
    char* oggfilename = NULL;
    char* opusfilename = NULL;
    char* flacfilename = NULL;
    char* wavpackfilename = NULL;
    char* wavpackfilename2 = NULL;
    char* monkeyfilename = NULL;
    char* musepackfilename = NULL;
    char* aacfilename = NULL;
    struct stat statStruct;
    bool doEncode;
    
    gdk_threads_enter();
        GtkListStore * store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(win_main, "tracklist"))));
        gboolean single_artist = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_main, "single_artist")));
        gboolean single_genre  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_main, "single_genre")));	// lnr

        GtkWidget * album_artist_widget = lookup_widget(win_main, "album_artist");
        const char * temp_album_artist = gtk_entry_get_text(GTK_ENTRY(album_artist_widget));
        album_artist = malloc(sizeof(char) * (strlen(temp_album_artist)+1));
        if (album_artist == NULL)
            fatalError("malloc(sizeof(char) * (strlen(temp_album_artist)+1)) failed. Out of memory.");
        strncpy(album_artist, temp_album_artist, strlen(temp_album_artist)+1);
        add_completion(album_artist_widget);
        save_completion(album_artist_widget);
        
        const char * temp_year = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_main, "album_year")));
        album_year = malloc(sizeof(char) * (strlen(temp_year)+1));
        if (album_year == NULL)
            fatalError("malloc(sizeof(char) * (strlen(temp_year)+1)) failed. Out of memory.");
        strncpy(album_year, temp_year, strlen(temp_year)+1);
        
        GtkWidget * album_title_widget = lookup_widget(win_main, "album_title");
        const char * temp_album_title = gtk_entry_get_text(GTK_ENTRY(album_title_widget));
        album_title = malloc(sizeof(char) * (strlen(temp_album_title)+1));
        if (album_title == NULL)
            fatalError("malloc(sizeof(char) * (strlen(temp_album_title)+1)) failed. Out of memory.");
        strncpy(album_title, temp_album_title, strlen(temp_album_title)+1);
        add_completion(album_title_widget);
        save_completion(album_title_widget);

        GtkWidget * album_genre_widget = lookup_widget(win_main, "album_genre");
        const char * temp_album_genre = gtk_entry_get_text(GTK_ENTRY(album_genre_widget));      // lnr
        album_genre = malloc(sizeof(char) * (strlen(temp_album_genre)+1));
        if (album_genre == NULL)
            fatalError("malloc(sizeof(char) * (strlen(temp_album_genre)+1)) failed. Out of memory.");
        strcpy( album_genre, temp_album_genre );
        add_completion(album_genre_widget);
        save_completion(album_genre_widget);
        
        gboolean rowsleft = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
    gdk_threads_leave();
    
    //Trimmed for use in filenames	//mrpl
    album_artist_trimmed = strdup(album_artist);
    trim_chars(album_artist_trimmed, BADCHARS);
    album_title_trimmed = strdup(album_title);
    trim_chars(album_title_trimmed, BADCHARS);
    album_genre_trimmed = strdup(album_genre);
    trim_chars(album_genre_trimmed, BADCHARS);
    
    while(rowsleft)
    {
        debugLog("encode() waiting for 'barrier'\n");
        g_mutex_lock(barrier);
        while ((counter < 1) && (!aborted))
        {
            debugLog("encode() waiting for 'available'\n");
            g_cond_wait(available, barrier);
        }
        counter--;
        snprintf(logStr, 1024, "encode() done waiting, counter is now %d\n", counter);
        debugLog(logStr);
        g_mutex_unlock(barrier);
        if (aborted) g_thread_exit(NULL);
        
        gdk_threads_enter();
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
                COL_RIPTRACK, &riptrack,
                COL_TRACKNUM, &tracknum,
                COL_TRACKARTIST, &trackartist,
                COL_TRACKTITLE, &tracktitle,
                COL_TRACKTIME, &tracktime,
                COL_GENRE, &genre,
                COL_YEAR, &trackyear,
                -1);
        gdk_threads_leave();
        sscanf(tracktime, "%d:%d", &min, &sec);
        
        if (single_artist)
        {
            trackartist = album_artist;
        }

        if ( single_genre )				// lnr
            genre	= album_genre;
        
        if (riptrack)
        {
            //Trimmed for use in filenames	//mrpl
            trackartist_trimmed = strdup(trackartist);
            trim_chars(trackartist_trimmed, BADCHARS);
            tracktitle_trimmed = strdup(tracktitle);
            trim_chars(tracktitle_trimmed, BADCHARS);
            genre_trimmed = strdup(genre);
            trim_chars(genre_trimmed, BADCHARS);
            
            //~albumdir = parse_format(global_prefs->format_albumdir, 0, album_year, album_artist, album_title, genre, NULL);
            //~musicfilename = parse_format(global_prefs->format_music, tracknum, trackyear, trackartist, album_title, tracktitle);
            //~musicfilename = parse_format(global_prefs->format_music, tracknum, album_year, trackartist, album_title, genre, tracktitle);
            albumdir = parse_format(global_prefs->format_albumdir, 0, album_year, album_artist_trimmed, album_title_trimmed, genre_trimmed, NULL);
            musicfilename = parse_format(global_prefs->format_music, tracknum, album_year, trackartist_trimmed, album_title_trimmed, genre_trimmed, tracktitle_trimmed);
            
            wavfilename = make_filename(prefs_get_music_dir(global_prefs), albumdir, musicfilename, "wav");
            mp3filename = make_filename(prefs_get_music_dir(global_prefs), albumdir, musicfilename, "mp3");
            oggfilename = make_filename(prefs_get_music_dir(global_prefs), albumdir, musicfilename, "ogg");
            opusfilename = make_filename(prefs_get_music_dir(global_prefs), albumdir, musicfilename, "opus");
            flacfilename = make_filename(prefs_get_music_dir(global_prefs), albumdir, musicfilename, "flac");
            wavpackfilename = make_filename(prefs_get_music_dir(global_prefs), albumdir, musicfilename, "wv");
            wavpackfilename2 = make_filename(prefs_get_music_dir(global_prefs), albumdir, musicfilename, "wvc");
            monkeyfilename = make_filename(prefs_get_music_dir(global_prefs), albumdir, musicfilename, "ape");
            musepackfilename = make_filename(prefs_get_music_dir(global_prefs), albumdir, musicfilename, "mpc");
            aacfilename = make_filename(prefs_get_music_dir(global_prefs), albumdir, musicfilename, "m4a");
            
            if (global_prefs->rip_mp3)
            {
                snprintf(logStr, 1024, "Encoding track %d to \"%s\"\n", tracknum, mp3filename);
                debugLog(logStr);
                
                if (aborted) g_thread_exit(NULL);
                
                rc = stat(mp3filename, &statStruct);
                if(rc == 0)
                {
                    gdk_threads_enter();
                        if(confirmOverwrite(mp3filename))
                            doEncode = true;
                        else
                            doEncode = false;
                    gdk_threads_leave();
                }
                else
                    doEncode = true;

                if(doEncode)
                    //~ lame(tracknum, trackartist, album_title, tracktitle, genre, trackyear, wavfilename, mp3filename, 
                         //~ global_prefs->mp3_vbr, global_prefs->mp3_bitrate, &mp3_percent);
                    lame(tracknum, trackartist, album_title, tracktitle, genre, album_year, wavfilename, mp3filename, 
                         global_prefs->mp3_vbr, global_prefs->mp3_bitrate, &mp3_percent);
                
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
                snprintf(logStr, 1024, "Encoding track %d to \"%s\"\n", tracknum, oggfilename);
                debugLog(logStr);
                
                if (aborted) g_thread_exit(NULL);
                
                rc = stat(oggfilename, &statStruct);
                if(rc == 0)
                {
                    gdk_threads_enter();
                        if(confirmOverwrite(oggfilename))
                            doEncode = true;
                        else
                            doEncode = false;
                    gdk_threads_leave();
                }
                else
                    doEncode = true;
                
                if(doEncode)
                {
                    oggenc(tracknum, trackartist, album_title, tracktitle, album_year, genre, wavfilename, 
                           oggfilename, global_prefs->ogg_quality, &ogg_percent);
                }
                if (aborted) g_thread_exit(NULL);

                if (playlist_ogg)
                {
                    fprintf(playlist_ogg, "#EXTINF:%d,%s - %s\n", (min*60)+sec, trackartist, tracktitle);
                    fprintf(playlist_ogg, "%s\n", basename(oggfilename));
                    fflush(playlist_ogg);
                }
            }
            if (global_prefs->rip_opus)
            {
                snprintf(logStr, 1024, "Encoding track %d to \"%s\"\n", tracknum, opusfilename);
                debugLog(logStr);

                if (aborted) g_thread_exit(NULL);

                rc = stat(opusfilename, &statStruct);
                if(rc == 0)
                {
                    gdk_threads_enter();
                        if(confirmOverwrite(opusfilename))
                            doEncode = true;
                        else
                            doEncode = false;
                    gdk_threads_leave();
                }
                else
                    doEncode = true;

                if(doEncode)
                    opusenc(tracknum, trackartist, album_title, tracktitle, album_year, genre, wavfilename,
                           opusfilename, global_prefs->opus_bitrate, &opus_percent);

                if (aborted) g_thread_exit(NULL);

                if (playlist_opus)
                {
                    fprintf(playlist_opus, "#EXTINF:%d,%s - %s\n", (min*60)+sec, trackartist, tracktitle);
                    fprintf(playlist_opus, "%s\n", basename(opusfilename));
                    fflush(playlist_opus);
                }
            }
            if (global_prefs->rip_flac)
            {
                snprintf(logStr, 1024, "Encoding track %d to \"%s\"\n", tracknum, flacfilename);
                debugLog(logStr);
                
                if (aborted) g_thread_exit(NULL);
                
                rc = stat(flacfilename, &statStruct);
                if(rc == 0)
                {
                    gdk_threads_enter();
                        if(confirmOverwrite(flacfilename))
                            doEncode = true;
                        else
                            doEncode = false;
                    gdk_threads_leave();
                }
                else
                    doEncode = true;
                
                if(doEncode)
                    //~ flac(tracknum, trackartist, album_title, tracktitle, genre, trackyear, wavfilename, 
                         //~ flacfilename, global_prefs->flac_compression, &flac_percent);
                    flac(tracknum, trackartist, album_title, tracktitle, genre, album_year, wavfilename, 
                         flacfilename, global_prefs->flac_compression, &flac_percent);
                
                if (aborted) g_thread_exit(NULL);
                
                if (playlist_flac)
                {
                    fprintf(playlist_flac, "#EXTINF:%d,%s - %s\n", (min*60)+sec, trackartist, tracktitle);
                    fprintf(playlist_flac, "%s\n", basename(flacfilename));
                    fflush(playlist_flac);
                }
            }
            if (global_prefs->rip_wavpack)
            {
                snprintf(logStr, 1024, "Encoding track %d to \"%s\"\n", tracknum, wavpackfilename);
                debugLog(logStr);
                
                if (aborted) g_thread_exit(NULL);
                
                rc = stat(wavpackfilename, &statStruct);
                int rc2 = stat(wavpackfilename2, &statStruct);
                if(rc == 0 || rc2 == 0)
                {
                    gdk_threads_enter();
                        if(confirmOverwrite(wavpackfilename))
                            doEncode = true;
                        else
                            doEncode = false;
                    gdk_threads_leave();
                }
                else
                    doEncode = true;
                
                if(doEncode)
                {
                    wavpack(tracknum, wavfilename, 
                            global_prefs->wavpack_compression, global_prefs->wavpack_hybrid, 
                            int_to_wavpack_bitrate(global_prefs->wavpack_bitrate), &wavpack_percent);
                }
                
                if (aborted) g_thread_exit(NULL);
                
                if (playlist_wavpack)
                {
                    fprintf(playlist_wavpack, "#EXTINF:%d,%s - %s\n", (min*60)+sec, trackartist, tracktitle);
                    fprintf(playlist_wavpack, "%s\n", basename(wavpackfilename));
                    fflush(playlist_wavpack);
                }
            }
            if (global_prefs->rip_monkey)
            {
                snprintf(logStr, 1024, "Encoding track %d to \"%s\"\n", tracknum, monkeyfilename);
                debugLog(logStr);
                
                if (aborted) g_thread_exit(NULL);
                
                rc = stat(monkeyfilename, &statStruct);
                if(rc == 0)
                {
                    gdk_threads_enter();
                        if(confirmOverwrite(monkeyfilename))
                            doEncode = true;
                        else
                            doEncode = false;
                    gdk_threads_leave();
                }
                else
                    doEncode = true;
                
                if(doEncode)
                {
                    mac(wavfilename, monkeyfilename,
                        int_to_monkey_int(global_prefs->monkey_compression), 
                        &monkey_percent);
                }
                
                if (aborted) g_thread_exit(NULL);
                
                if (playlist_monkey)
                {
                    fprintf(playlist_monkey, "#EXTINF:%d,%s - %s\n", (min*60)+sec, trackartist, tracktitle);
                    fprintf(playlist_monkey, "%s\n", basename(monkeyfilename));
                    fflush(playlist_monkey);
                }
            }
            if (global_prefs->rip_musepack)
            {
                snprintf(logStr, 1024, "Encoding track %d to \"%s\"\n", tracknum, musepackfilename);
                debugLog(logStr);
                
                if (aborted) g_thread_exit(NULL);
                
                rc = stat(musepackfilename, &statStruct);
                if(rc == 0)
                {
                    gdk_threads_enter();
                        if(confirmOverwrite(musepackfilename))
                            doEncode = true;
                        else
                            doEncode = false;
                    gdk_threads_leave();
                }
                else
                    doEncode = true;
                
                if(doEncode)
                {
                    musepack(wavfilename, musepackfilename,
                             int_to_musepack_int(global_prefs->musepack_bitrate), 
                             &musepack_percent);
                }
                
                if (aborted) g_thread_exit(NULL);
                
                if (playlist_musepack)
                {
                    fprintf(playlist_musepack, "#EXTINF:%d,%s - %s\n", (min*60)+sec, trackartist, tracktitle);
                    fprintf(playlist_musepack, "%s\n", basename(musepackfilename));
                    fflush(playlist_musepack);
                }
            }
            if (global_prefs->rip_aac)
            {
                snprintf(logStr, 1024, "Encoding track %d to \"%s\"\n", tracknum, aacfilename);
                debugLog(logStr);
                
                if (aborted) g_thread_exit(NULL);
                
                rc = stat(aacfilename, &statStruct);
                if(rc == 0)
                {
                    gdk_threads_enter();
                        if(confirmOverwrite(aacfilename))
                            doEncode = true;
                        else
                            doEncode = false;
                    gdk_threads_leave();
                }
                else
                    doEncode = true;
                
                if(doEncode)
                {
                    aac(tracknum, trackartist, album_title, tracktitle, genre, album_year,
                        wavfilename, aacfilename,
                        global_prefs->aac_quality, 
                        &aac_percent);
                }
                
                if (aborted) g_thread_exit(NULL);
                
                if (playlist_aac)
                {
                    fprintf(playlist_aac, "#EXTINF:%d,%s - %s\n", (min*60)+sec, trackartist, tracktitle);
                    fprintf(playlist_aac, "%s\n", basename(aacfilename));
                    fflush(playlist_aac);
                }
            }
            if (!global_prefs->rip_wav)
            {
                snprintf(logStr, 1024, "Removing track %d WAV file\n", tracknum);
                debugLog(logStr);
                
                if (unlink(wavfilename) != 0)
                {
                    snprintf(logStr, 1024, "Unable to delete WAV file \"%s\": %s\n", wavfilename, strerror(errno));
                    debugLog(logStr);
                }
            } else {
                if (playlist_wav)
                {
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
            free(opusfilename);
            free(flacfilename);
            free(wavpackfilename);
            free(wavpackfilename2);
            free(monkeyfilename);
            free(musepackfilename);
            free(aacfilename);
            free(trackartist_trimmed);
            free(tracktitle_trimmed);
            free(genre_trimmed);
            
            mp3_percent = 0.0;
            ogg_percent = 0.0;
            opus_percent = 0.0;
            flac_percent = 0.0;
            wavpack_percent = 0.0;
            monkey_percent = 0.0;
            musepack_percent = 0.0;
            aac_percent = 0.0;
            encode_tracks_completed++;
        }
        
        if (aborted) g_thread_exit(NULL);
        
        gdk_threads_enter();
            rowsleft = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
        gdk_threads_leave();
    }
    
    free(album_artist);
    free(album_title);
    free(album_genre);
    free(album_year);
    free(album_artist_trimmed);
    free(album_title_trimmed);
    free(album_genre_trimmed);
    
    if (playlist_wav) fclose(playlist_wav);
    playlist_wav = NULL;
    if (playlist_mp3) fclose(playlist_mp3);
    playlist_mp3 = NULL;
    if (playlist_ogg) fclose(playlist_ogg);
    playlist_ogg = NULL;
    if (playlist_opus) fclose (playlist_opus);
    playlist_opus = NULL;
    if (playlist_flac) fclose(playlist_flac);
    playlist_flac = NULL;
    if (playlist_wavpack) fclose(playlist_wavpack);
    playlist_wavpack = NULL;
    if (playlist_monkey) fclose(playlist_monkey);
    playlist_monkey = NULL;
    if (playlist_musepack) fclose(playlist_musepack);
    playlist_musepack = NULL;
    if (playlist_aac) fclose(playlist_aac);
    playlist_aac = NULL;
    
    g_mutex_free(barrier);
    barrier = NULL;
    g_cond_free(available);
    available = NULL;
    
    /* wait until all the worker threads are done */
    while (cdparanoia_pid != 0 || lame_pid != 0 || oggenc_pid != 0 || 
           opusenc_pid != 0 || flac_pid != 0 || wavpack_pid != 0 || monkey_pid != 0 ||
           musepack_pid != 0 || aac_pid != 0)
    {
        debugLog("w2");
        usleep(100000);
    }
    
    allDone = true; // so the tracker thread will exit
    working = false;
    
    gdk_threads_enter();
        gtk_widget_hide(win_ripping);
        gdk_flush();
        
        show_completed_dialog(numCdparanoiaOk + numLameOk + numOggOk + numOpusOk + numFlacOk + numWavpackOk + numMonkeyOk + numMusepackOk + numAacOk,
                              numCdparanoiaFailed + numLameFailed + numOggFailed + numOpusFailed + numFlacFailed + numWavpackFailed + numMonkeyFailed + numMusepackFailed + numAacFailed);
    gdk_threads_leave();
    
    return NULL;
}

// the thread that calculates the progress of the other threads and updates the progress bars
gpointer track(gpointer data)
{
    char logStr[1024];
    int parts = 1;
    if(global_prefs->rip_mp3) 
        parts++;
    if(global_prefs->rip_ogg) 
        parts++;
    if(global_prefs->rip_opus)
        parts++;
    if(global_prefs->rip_flac) 
        parts++;
    if(global_prefs->rip_wavpack) 
        parts++;
    if(global_prefs->rip_monkey) 
        parts++;
    if(global_prefs->rip_musepack) 
        parts++;
    if(global_prefs->rip_aac) 
        parts++;
    
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
    double pencode = 0;
    char sencode[13];
    double ptotal;
    char stotal[5];
    char windowTitle[15]; /* "Asunder - 100%" */
    
    while (!allDone)
    {
        if (aborted) g_thread_exit(NULL);
        
        snprintf(logStr, 1024, "completed tracks %d, rip %.2lf%%; encoded tracks %d, "
                 "mp3 %.2lf%% ogg %.2lf%% opus %.2lf%% flac %.2lf%% wavpack %.2lf%% "
                 "monkey %.2lf%% musepack %.2lf%% aac %.2lf%%\n", 
                 rip_tracks_completed, rip_percent*100, encode_tracks_completed, 
                 mp3_percent*100, ogg_percent*100, opus_percent*100, flac_percent*100, wavpack_percent*100,
                 monkey_percent*100,musepack_percent*100,aac_percent*100);
        debugLog(logStr);
        
        prip = (rip_tracks_completed+rip_percent) / tracks_to_rip;
        snprintf(srip, 13, "%d%% (%d/%d)", (int)(prip*100),
                 (rip_tracks_completed < tracks_to_rip)
                     ? (rip_tracks_completed + 1)
                     : tracks_to_rip,
                 tracks_to_rip);
        if (parts > 1)
        {
            pencode = ((double)encode_tracks_completed/(double)tracks_to_rip) + 
                       ((mp3_percent+ogg_percent+flac_percent+wavpack_percent+monkey_percent
                         +opus_percent+musepack_percent+aac_percent) /
                        (parts-1) / tracks_to_rip);
            snprintf(sencode, 13, "%d%% (%d/%d)", (int)(pencode*100),
                     (encode_tracks_completed < tracks_to_rip)
                         ? (encode_tracks_completed + 1)
                         : tracks_to_rip,
                     tracks_to_rip);
            ptotal = prip/parts + pencode*(parts-1)/parts;
        } else {
            ptotal = prip;
        }
        snprintf(stotal, 5, "%d%%", (int)(ptotal*100));
        
        strcpy(windowTitle, "Asunder - ");
        strcat(windowTitle, stotal);
        
        if (aborted) g_thread_exit(NULL);
        
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
            
            gtk_window_set_title(GTK_WINDOW(win_main), windowTitle);
        gdk_threads_leave();
        
        usleep(100000);
    }
    
    gdk_threads_enter();
        gtk_window_set_title(GTK_WINDOW(win_main), "Asunder");
    gdk_threads_leave();
    
    return NULL;
}
