/*
Asunder

Copyright(C) 2005 Eric Lathrop <eric@ericlathrop.com>
Copyright(C) 2007 Andrew Smith <http://littlesvr.ca/misc/contactandrew.php>

Any code in this file may be redistributed or modified under the terms of
the GNU General Public Licence as published by the Free Software 
Foundation; version 2 of the licence.

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "prefs.h"
#include "main.h"
#include "util.h"
#include "support.h"
#include "interface.h"

prefs * global_prefs = NULL;

// allocate memory for a new prefs struct
// and make sure everything is set to NULL
prefs * new_prefs()
{
    prefs * p = malloc(sizeof(prefs));
    if (p == NULL)
        fatalError("malloc(sizeof(prefs)) failed. Out of memory.");
    memset(p, 0, sizeof(prefs));
    
    return p;
}

void clear_prefs(prefs * p)
{
    if (p->cdrom != NULL) 
        free(p->cdrom);
    p->cdrom = NULL;

    if (p->music_dir != NULL) 
        free(p->music_dir);
    p->music_dir = NULL;

    if (p->format_music != NULL) 
        free(p->format_music);
    p->format_music = NULL;

    if (p->format_playlist != NULL) 
        free(p->format_playlist);
    p->format_playlist = NULL;

    if (p->format_albumdir != NULL) 
        free(p->format_albumdir);
    p->format_albumdir = NULL;
    
    if (p->server_name != NULL) 
        free(p->server_name);
    p->server_name = NULL;
}

// free memory allocated for prefs struct
// also frees any strings pointed to in the struct
void delete_prefs(prefs * p)
{
    clear_prefs(p);
    
    free(p);
}

// returns a new prefs struct with all members set to nice default values
// this struct must be freed with delete_prefs()
prefs * get_default_prefs()
{
    prefs * p = new_prefs();
    
    p->cdrom = malloc(sizeof(char) * 11);
    if (p->cdrom == NULL)
        fatalError("malloc(sizeof(char) * 11) failed. Out of memory.");
    strncpy(p->cdrom, "/dev/cdrom", 11);
    
    p->music_dir = strdup(getenv("HOME"));
    p->make_playlist = 1;
    
    p->format_music = malloc(sizeof(char) * 13);
    if (p->format_music == NULL)
        fatalError("malloc(sizeof(char) * 8) failed. Out of memory.");
    strncpy(p->format_music, "%N - %A - %T", 13);
    
    p->format_playlist = malloc(sizeof(char) * 8);
    if (p->format_playlist == NULL)
        fatalError("malloc(sizeof(char) * 8) failed. Out of memory.");
    strncpy(p->format_playlist, "%A - %L", 8);
    
    p->format_albumdir = malloc(sizeof(char) * 8);
    if (p->format_albumdir == NULL)
        fatalError("malloc(sizeof(char) * 8) failed. Out of memory.");
    strncpy(p->format_albumdir, "%A - %L", 8);
    
    p->rip_wav = 0;
    p->rip_mp3 = 0;
    p->rip_ogg = 1;
    p->rip_flac = 0;
    p->rip_wavpack = 0;
    p->mp3_vbr = 1;
    p->mp3_bitrate = 10;
    p->ogg_quality = 6;
    p->flac_compression = 5;
    p->wavpack_compression = 1;
    p->wavpack_hibrid = 1;
    p->wavpack_bitrate = 3;
    
    p->main_window_width = 600;
    p->main_window_height = 450;
    
    p->eject_on_done = 0;
    
    p->do_cddb_updates = 1;
    
    p->use_proxy = 0;
    
    p->do_log = 0;
    
    p->server_name = malloc(sizeof(char) * (strlen("10.0.0.1") + 1));
    if (p->server_name == NULL)
        fatalError("malloc(sizeof(char) * (strlen(\"10.0.0.1\") + 1)) failed. Out of memory.");
    strcpy(p->server_name, "10.0.0.1");
    
    p->port_number = DEFAULT_PROXY_PORT;
    
    return p;
}

// sets up all of the widgets in the preferences dialog to
// match the given prefs struct
void set_widgets_from_prefs(prefs * p)
{
    char tempStr[10];
    
    gtk_entry_set_text(GTK_ENTRY(lookup_widget(win_prefs, "cdrom")), p->cdrom);
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(lookup_widget(win_prefs, "music_dir")), prefs_get_music_dir(p));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "make_playlist")), p->make_playlist);
    gtk_entry_set_text(GTK_ENTRY(lookup_widget(win_prefs, "format_music")), p->format_music);
    gtk_entry_set_text(GTK_ENTRY(lookup_widget(win_prefs, "format_playlist")), p->format_playlist);
    gtk_entry_set_text(GTK_ENTRY(lookup_widget(win_prefs, "format_albumdir")), p->format_albumdir);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_wav")), p->rip_wav);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_mp3")), p->rip_mp3);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_ogg")), p->rip_ogg);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_flac")), p->rip_flac);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_wavpack")), p->rip_wavpack);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "mp3_vbr")), p->mp3_vbr);
    gtk_range_set_value(GTK_RANGE(lookup_widget(win_prefs, "mp3bitrate")), p->mp3_bitrate);
    gtk_range_set_value(GTK_RANGE(lookup_widget(win_prefs, "oggquality")), p->ogg_quality);
    gtk_range_set_value(GTK_RANGE(lookup_widget(win_prefs, "flaccompression")), p->flac_compression);
    gtk_range_set_value(GTK_RANGE(lookup_widget(win_prefs, "wavpack_compression")), p->wavpack_compression);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "wavpack_hibrid")), p->wavpack_hibrid);
    gtk_range_set_value(GTK_RANGE(lookup_widget(win_prefs, "wavpack_bitrate_slider")), p->wavpack_bitrate);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "eject_on_done")), p->eject_on_done);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "do_cddb_updates")), p->do_cddb_updates);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "use_proxy")), p->use_proxy);
    gtk_entry_set_text(GTK_ENTRY(lookup_widget(win_prefs, "server_name")), p->server_name);
    snprintf(tempStr, 10, "%d", p->port_number);
    gtk_entry_set_text(GTK_ENTRY(lookup_widget(win_prefs, "port_number")), tempStr);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "do_log")), p->do_log);
    
    /* disable widgets if needed */
    if ( !(p->rip_mp3) )
        disable_mp3_widgets();
    if ( !(p->rip_ogg) )
        disable_ogg_widgets();
    if ( !(p->rip_flac) )
        disable_flac_widgets();
    if ( !(p->rip_wavpack) )
        disable_wavpack_widgets();
    else
        enable_wavpack_widgets(); /* need this to potentially disable hibrid widgets */
}

// populates a prefs struct from the current state of the widgets
void get_prefs_from_widgets(prefs * p)
{
    gchar * tocopy = NULL;
    const gchar * tocopyc = NULL;

    clear_prefs(p);
    
    tocopyc = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "cdrom")));
    p->cdrom = malloc(sizeof(char) * (strlen(tocopyc) + 1));
    if (p->cdrom == NULL)
        fatalError("malloc(sizeof(char) * (strlen(tocopyc) + 1)) failed. Out of memory.");
    strncpy(p->cdrom, tocopyc, strlen(tocopyc)+1);
    
    tocopy = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(lookup_widget(win_prefs, "music_dir")));
    if ((p->music_dir = malloc(sizeof(char) * (strlen(tocopy) + 1))) == NULL)
        fatalError("malloc(sizeof(char) * (strlen(tocopy) + 1)) failed. Out of memory.");
    strncpy(p->music_dir, tocopy, strlen(tocopy)+1);
    g_free(tocopy);
    
    p->make_playlist = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "make_playlist")));
    
    tocopyc = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "format_music")));
    p->format_music = malloc(sizeof(char) * (strlen(tocopyc) + 1));
    if (p->format_music == NULL)
        fatalError("malloc(sizeof(char) * (strlen(tocopyc) + 1)) failed. Out of memory.");
    strncpy(p->format_music, tocopyc, strlen(tocopyc)+1);
    
    tocopyc = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "format_playlist")));
    p->format_playlist = malloc(sizeof(char) * (strlen(tocopyc) + 1));
    if (p->format_playlist == NULL)
        fatalError("malloc(sizeof(char) * (strlen(tocopyc) + 1)) failed. Out of memory.");
    strncpy(p->format_playlist, tocopyc, strlen(tocopyc)+1);
    
    tocopyc = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "format_albumdir")));
    if ((p->format_albumdir = malloc(sizeof(char) * (strlen(tocopyc) + 1))) == NULL)
        fatalError("malloc(sizeof(char) * (strlen(tocopyc) + 1)) failed. Out of memory.");
    strncpy(p->format_albumdir, tocopyc, strlen(tocopyc)+1);

    p->rip_wav = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_wav")));
    p->rip_mp3 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_mp3")));
    p->rip_ogg = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_ogg")));
    p->rip_flac = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_flac")));
    p->rip_wavpack = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_wavpack")));
    p->mp3_vbr = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "mp3_vbr")));
    p->mp3_bitrate = (int)gtk_range_get_value(GTK_RANGE(lookup_widget(win_prefs, "mp3bitrate")));
    p->ogg_quality = (int)gtk_range_get_value(GTK_RANGE(lookup_widget(win_prefs, "oggquality")));
    p->flac_compression = (int)gtk_range_get_value(GTK_RANGE(lookup_widget(win_prefs, "flaccompression")));
    p->wavpack_compression = (int)gtk_range_get_value(GTK_RANGE(lookup_widget(win_prefs, "wavpack_compression")));
    p->wavpack_hibrid = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "wavpack_hibrid")));
    p->wavpack_bitrate = (int)gtk_range_get_value(GTK_RANGE(lookup_widget(win_prefs, "wavpack_bitrate_slider")));
    
    p->eject_on_done = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "eject_on_done")));
    
    p->do_cddb_updates = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "do_cddb_updates")));
    
    p->use_proxy = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "use_proxy")));
    
    tocopyc = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "server_name")));
    p->server_name = malloc(sizeof(char) * (strlen(tocopyc) + 1));
    if (p->server_name == NULL)
        fatalError("malloc(sizeof(char) * (strlen(tocopyc) + 1)) failed. Out of memory.");
    strncpy(p->server_name, tocopyc, strlen(tocopyc) + 1);
    
    tocopyc = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "port_number")));
    
    p->do_log = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "do_log")));
}

// store the given prefs struct to the config file
void save_prefs(prefs * p)
{
    char * home = getenv("HOME");
    int homelen = strlen(home);
    char * file;
    
    file = malloc(sizeof(char) * (homelen + 10));
    if (file == NULL)
        fatalError("malloc(sizeof(char) * (homelen + 10)) failed. Out of memory.");
    strncpy(file, home, homelen);
    strncpy(&file[homelen], "/.asunder", 10);
        
    debugLog("Saving configuration\n");
    
    FILE * config = fopen(file, "w");
    if (config != NULL)
    {
        fprintf(config, "%s\n", p->cdrom);
        fprintf(config, "%s\n", p->music_dir);
        fprintf(config, "%d\n", p->make_playlist);
        fprintf(config, "%d\n", 1); /* used to be p->make_albumdir */
        fprintf(config, "%s\n", p->format_music);
        fprintf(config, "%s\n", p->format_playlist);
        fprintf(config, "%s\n", p->format_albumdir);
        fprintf(config, "%d\n", p->rip_wav);
        fprintf(config, "%d\n", p->rip_mp3);
        fprintf(config, "%d\n", p->rip_ogg);
        fprintf(config, "%d\n", p->rip_flac);
        fprintf(config, "%d\n", p->mp3_vbr);
        fprintf(config, "%d\n", p->mp3_bitrate);
        fprintf(config, "%d\n", p->ogg_quality);
        fprintf(config, "%d\n", p->flac_compression);
        fprintf(config, "%s\n", "unused"); /* used to be p->invalid_chars */
        fprintf(config, "%d\n", p->main_window_width);
        fprintf(config, "%d\n", p->main_window_height);
        fprintf(config, "%d\n", p->eject_on_done);
        fprintf(config, "%d\n", p->do_cddb_updates);
        fprintf(config, "%d\n", p->use_proxy);
        fprintf(config, "%s\n", p->server_name);
        fprintf(config, "%d\n", p->port_number);
        fprintf(config, "%d\n", p->rip_wavpack);
        fprintf(config, "%d\n", p->wavpack_compression);
        fprintf(config, "%d\n", p->wavpack_hibrid);
        fprintf(config, "%d\n", p->wavpack_bitrate);
        fprintf(config, "%d\n", p->do_log);
        
        fclose(config);
    } else {
        fprintf(stderr, "Warning: could not save config file: %s\n", strerror(errno));
    }
    free(file);
}

// load the prefereces from the config file into the given prefs struct
void load_prefs(prefs * p)
{
    char * home = getenv("HOME");
    int homelen = strlen(home);
    char * file;
    
    file = malloc(sizeof(char) * (homelen + 10));
    if (file == NULL)
        fatalError("malloc(sizeof(char) * (homelen + 10)) failed. Out of memory.");
    strncpy(file, home, homelen);
    strncpy(&file[homelen], "/.asunder", 10);
    
    debugLog("Loading configuration\n");
    
    int fd = open(file, O_RDONLY);
    if (fd > -1)
    {
        int anInt;
        char* aCharPtr;
        
        aCharPtr = read_line(fd);
        if (aCharPtr != NULL)
        {
            if (p->cdrom != NULL)
                free(p->cdrom);
            p->cdrom = aCharPtr;
        }
        
        aCharPtr = read_line(fd);
        if (aCharPtr != NULL)
        {
            if (p->music_dir != NULL)
                free(p->music_dir);
            p->music_dir = aCharPtr;
        }
        
        // this one can be 0
        p->make_playlist = read_line_num(fd);
        
        // used to be p->make_albumdir, but no longer used
        p->make_albumdir = 1;
        read_line_num(fd);
        
        aCharPtr = read_line(fd);
        if (aCharPtr != NULL)
        {
            if (p->format_music != NULL)
                free(p->format_music);
            p->format_music = aCharPtr;
        }
        
        aCharPtr = read_line(fd);
        if (aCharPtr != NULL)
        {
            if (p->format_playlist != NULL)
                free(p->format_playlist);
            p->format_playlist = aCharPtr;
        }
        
        aCharPtr = read_line(fd);
        if (aCharPtr != NULL)
        {
            if (p->format_albumdir != NULL)
                free(p->format_albumdir);
            p->format_albumdir = aCharPtr;
        }
        
        // this one can be 0
        p->rip_wav = read_line_num(fd);
        
        // this one can be 0
        p->rip_mp3 = read_line_num(fd);
        
        // this one can be 0
        p->rip_ogg = read_line_num(fd);
        
        // this one can be 0
        p->rip_flac = read_line_num(fd);
        
        // this one can be 0
        p->mp3_vbr = read_line_num(fd);
        
        anInt = read_line_num(fd);
        if (anInt != 0)
            p->mp3_bitrate = anInt;
        
        // this one can be 0
        p->ogg_quality = read_line_num(fd);
        
        // this one can be 0
        p->flac_compression = read_line_num(fd);
        
        /* used to be p->invalid_chars, but no longer used */
        read_line(fd);
        
        anInt = read_line_num(fd);
        if (anInt != 0)
            p->main_window_width = anInt;
        
        anInt = read_line_num(fd);
        if (anInt != 0)
            p->main_window_height = anInt;
        
        // this one can be 0
        p->eject_on_done = read_line_num(fd);
        
        // this one can be 0
        p->do_cddb_updates = read_line_num(fd);
        
        // this one can be 0
        p->use_proxy = read_line_num(fd);
        
        aCharPtr = read_line(fd);
        if (aCharPtr != NULL)
        {
            if (p->server_name != NULL)
                free(p->server_name);
            p->server_name = aCharPtr;
        }
        
        // this one can be 0
        p->port_number = read_line_num(fd);
        if (!is_valid_port_number(p->port_number))
        {
            printf("bad port number read from config file, using %d instead\n", DEFAULT_PROXY_PORT);
            p->port_number = DEFAULT_PROXY_PORT;
        }
        
        // this one can be 0
        p->rip_wavpack = read_line_num(fd);
        
        // this one can be 0
        p->wavpack_compression = read_line_num(fd);
        
        // this one can be 0
        p->wavpack_hibrid = read_line_num(fd);
        
        // this one can be 0
        p->wavpack_bitrate = read_line_num(fd);
        
        // this one can be 0
        p->do_log = read_line_num(fd);
        
        close(fd);
    } else {
        fprintf(stderr, "Warning: could not load config file: %s\n", strerror(errno));
    }
    free(file);
}

// use this method when reading the "music_dir" field of a prefs struct
// it will make sure it always points to a nice directory
char * prefs_get_music_dir(prefs * p)
{
    struct stat s;
    char * home;
    GtkWidget * dialog;
    
    if (stat(p->music_dir, &s) != 0)
    {
        home = getenv("HOME");
        
        dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), 
                                        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
                                        "The music directory '%s' does not exist.\n\n"
                                        "The music directory will be reset to '%s'.", 
                                        p->music_dir, home);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        
        free(p->music_dir);
        p->music_dir = malloc(sizeof(char) * (strlen(home)+1));
        if (p->music_dir == NULL)
            fatalError("malloc(sizeof(char) * (strlen(home)+1)) failed. Out of memory.");
        
        strncpy(p->music_dir, home, strlen(home)+1);
        
        save_prefs(p);
    }
    return p->music_dir;
}

int is_valid_port_number(int number)
{
    if(number >= 0 && number <= 65535)
        return 1;
    else
        return 0;
}

bool prefs_are_valid(void)
{
    GtkWidget * warningDialog;
    bool somethingWrong = false;
    
    // playlistfile
    if(string_has_slashes(gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "format_playlist")))))
    {
        warningDialog = gtk_message_dialog_new(GTK_WINDOW(win_main), GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
                                               _("Invalid characters in the '%s' field"),
                                               _("Playlist file: "));
        gtk_dialog_run(GTK_DIALOG(warningDialog));
        gtk_widget_destroy(warningDialog);
        somethingWrong = true;
    }
    
    // musicfile
    if(string_has_slashes(gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "format_music")))))
    {
        warningDialog = gtk_message_dialog_new(GTK_WINDOW(win_main), GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
                                               _("Invalid characters in the '%s' field"),
                                               _("Music file: "));
        gtk_dialog_run(GTK_DIALOG(warningDialog));
        gtk_widget_destroy(warningDialog);
        somethingWrong = true;
    }
    if(strlen(gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "format_music")))) == 0)
    {
        warningDialog = gtk_message_dialog_new(GTK_WINDOW(win_main), GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
                                               _("'%s' cannot be empty"),
                                               _("Music file: "));
        gtk_dialog_run(GTK_DIALOG(warningDialog));
        gtk_widget_destroy(warningDialog);
        somethingWrong = true;
    }
    
    // proxy port
    int rc;
    int port_number;
    rc = sscanf(gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "port_number"))), "%d", &port_number);
    if (rc != 1 || !is_valid_port_number(port_number))
    {
        warningDialog = gtk_message_dialog_new(GTK_WINDOW(win_main), GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
                                               _("Invalid proxy port number"));
        gtk_dialog_run(GTK_DIALOG(warningDialog));
        gtk_widget_destroy(warningDialog);
        somethingWrong = true;
    }
    
    if(somethingWrong)
        return false;
    else
        return true;
}

bool string_has_slashes(const char* string)
{
    int count;
    
    for(count = strlen(string) - 1; count >= 0; count--)
    {
        if(string[count] == '/')
            return true;
    }
    
    return false;
}
