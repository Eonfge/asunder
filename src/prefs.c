/*
Asunder

Copyright(C) 2005 Eric Lathrop <eric@ericlathrop.com>
Copyright(C) 2007 Andrew Smith <http://littlesvr.ca/misc/contactandrew.php>

Any code in this file may be redistributed or modified under the terms of
the GNU General Public Licence as published by the Free Software 
Foundation; version 2 of the licence.

*/

#include "prefs.h"
#include "main.h"
#include "util.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

prefs * global_prefs = NULL;

// allocate memory for a new prefs struct
// and make sure everything is set to NULL
prefs * new_prefs()
{
    prefs * p = malloc(sizeof(prefs));
    if (p == NULL)
    {
        fprintf(stderr, "malloc() failed, out of memory\n");
        exit(-1);
    }
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

    if (p->invalid_chars != NULL) 
        free(p->invalid_chars);
    p->invalid_chars = NULL;
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
    {
        fprintf(stderr, "malloc() failed, out of memory\n");
        exit(-1);
    }
    strncpy(p->cdrom, "/dev/cdrom", 11);
    
    p->music_dir = strdup(getenv("HOME"));
    p->make_playlist = 1;
    p->make_albumdir = 1;
    p->format_music = malloc(sizeof(char) * 8);
    if (p->format_music == NULL)
    {
        fprintf(stderr, "malloc() failed, out of memory\n");
        exit(-1);
    }
    strncpy(p->format_music, "%A - %T", 8);
    
    p->format_playlist = malloc(sizeof(char) * 8);
    if (p->format_playlist == NULL)
    {
        fprintf(stderr, "malloc() failed, out of memory\n");
        exit(-1);
    }
    strncpy(p->format_playlist, "%A - %L", 8);
    
    p->format_albumdir = malloc(sizeof(char) * 8);
    if (p->format_albumdir == NULL)
    {
        fprintf(stderr, "malloc() failed, out of memory\n");
        exit(-1);
    }
    strncpy(p->format_albumdir, "%A - %L", 8);
    
    p->rip_wav = 0;
    p->rip_mp3 = 0;
    p->rip_ogg = 1;
    p->rip_flac = 0;
    p->mp3_vbr = 1;
    p->mp3_bitrate = 10;
    p->ogg_quality = 6;
    p->flac_compression = 8;
    
    p->invalid_chars = malloc(sizeof(char) * 2);
    if (p->invalid_chars == NULL)
    {
        fprintf(stderr, "malloc() failed, out of memory\n");
        exit(-1);
    }
    strncpy(p->invalid_chars, "/", 2);
    
    p->main_window_width = 600;
    p->main_window_height = 450;
    
    return p;
}

// sets up all of the widgets in the preferences dialog to
// match the given prefs struct
void set_widgets_from_prefs(prefs * p)
{
    gtk_entry_set_text(GTK_ENTRY(lookup_widget(win_prefs, "cdrom")), p->cdrom);
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(lookup_widget(win_prefs, "music_dir")), prefs_get_music_dir(p));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "make_playlist")), p->make_playlist);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "make_albumdir")), p->make_albumdir);
    gtk_entry_set_text(GTK_ENTRY(lookup_widget(win_prefs, "format_music")), p->format_music);
    gtk_entry_set_text(GTK_ENTRY(lookup_widget(win_prefs, "format_playlist")), p->format_playlist);
    gtk_entry_set_text(GTK_ENTRY(lookup_widget(win_prefs, "format_albumdir")), p->format_albumdir);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_wav")), p->rip_wav);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_mp3")), p->rip_mp3);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_ogg")), p->rip_ogg);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_flac")), p->rip_flac);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "mp3_vbr")), p->mp3_vbr);
    gtk_range_set_value(GTK_RANGE(lookup_widget(win_prefs, "mp3bitrate")), p->mp3_bitrate);
    gtk_range_set_value(GTK_RANGE(lookup_widget(win_prefs, "oggquality")), p->ogg_quality);
    gtk_range_set_value(GTK_RANGE(lookup_widget(win_prefs, "flaccompression")), p->flac_compression);
    gtk_entry_set_text(GTK_ENTRY(lookup_widget(win_prefs, "invalid_chars")), p->invalid_chars);
}

// populates a prefs struct from the current state of the widgets
void get_prefs_from_widgets(prefs * p)
{
    gchar * tocopy = NULL;
    const gchar * tocopyc = NULL;
    
    clear_prefs(p);
    
    tocopyc = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "cdrom")));
    p->cdrom = malloc(sizeof(char) * (strlen(tocopyc)+1));
    if (p->cdrom == NULL)
    {
        fprintf(stderr, "malloc() failed, out of memory\n");
        exit(-1);
    }
    strncpy(p->cdrom, tocopyc, strlen(tocopyc)+1);
    
    tocopy = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(lookup_widget(win_prefs, "music_dir")));
    if ((p->music_dir = malloc(sizeof(char) * (strlen(tocopy)+1))) == NULL)
    {
        fprintf(stderr, "malloc() failed, out of memory\n");
        exit(-1);
    }
    strncpy(p->music_dir, tocopy, strlen(tocopy)+1);
    g_free(tocopy);
    
    p->make_playlist = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "make_playlist")));
    p->make_albumdir = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "make_albumdir")));

    tocopyc = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "format_music")));
    p->format_music = malloc(sizeof(char) * (strlen(tocopyc)+1));
    if (p->format_music == NULL)
    {
        fprintf(stderr, "malloc() failed, out of memory\n");
        exit(-1);
    }
    strncpy(p->format_music, tocopyc, strlen(tocopyc)+1);
    
    tocopyc = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "format_playlist")));
    p->format_playlist = malloc(sizeof(char) * (strlen(tocopyc)+1));
    if (p->format_playlist == NULL)
    {
        fprintf(stderr, "malloc() failed, out of memory\n");
        exit(-1);
    }
    strncpy(p->format_playlist, tocopyc, strlen(tocopyc)+1);
    
    tocopyc = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "format_albumdir")));
    if ((p->format_albumdir = malloc(sizeof(char) * (strlen(tocopyc)+1))) == NULL)
    {
        fprintf(stderr, "malloc() failed, out of memory\n");
        exit(-1);
    }
    strncpy(p->format_albumdir, tocopyc, strlen(tocopyc)+1);

    p->rip_wav = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_wav")));
    p->rip_mp3 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_mp3")));
    p->rip_ogg = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_ogg")));
    p->rip_flac = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_flac")));
    p->mp3_vbr = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "mp3_vbr")));
    p->mp3_bitrate = (int)gtk_range_get_value(GTK_RANGE(lookup_widget(win_prefs, "mp3bitrate")));
    p->ogg_quality = (int)gtk_range_get_value(GTK_RANGE(lookup_widget(win_prefs, "oggquality")));
    p->flac_compression = (int)gtk_range_get_value(GTK_RANGE(lookup_widget(win_prefs, "flaccompression")));
    
    tocopyc = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "invalid_chars")));
    if ((p->invalid_chars = malloc(sizeof(char) * (strlen(tocopyc)+1))) == NULL)
    {
        fprintf(stderr, "malloc() failed, out of memory\n");
        exit(-1);
    }
    strncpy(p->invalid_chars, tocopyc, strlen(tocopyc)+1);
}

// store the given prefs struct to the config file
void save_prefs(prefs * p)
{
    char * home = getenv("HOME");
    int homelen = strlen(home);
    char * file;
    
    file = malloc(sizeof(char) * (homelen + 10));
    if (file == NULL)
    {
        fprintf(stderr, "malloc(sizeof(char) * (homelen + 10)) failed\n");
        exit(-1);
    }
    strncpy(file, home, homelen);
    strncpy(&file[homelen], "/.asunder", 10);
    
#ifdef DEBUG
    printf("Saving configuration\n");
#endif
    
    FILE * config = fopen(file, "w");
    if (config != NULL)
    {
        fprintf(config, "%s\n", p->cdrom);
        fprintf(config, "%s\n", p->music_dir);
        fprintf(config, "%d\n", p->make_playlist);
        fprintf(config, "%d\n", p->make_albumdir);
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
        fprintf(config, "%s\n", p->invalid_chars);
        fprintf(config, "%d\n", p->main_window_width);
        fprintf(config, "%d\n", p->main_window_height);
        
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
    {
        fprintf(stderr, "malloc(sizeof(char) * (homelen + 10)) failed\n");
        exit(-1);
    }
    strncpy(file, home, homelen);
    strncpy(&file[homelen], "/.asunder", 10);

#ifdef DEBUG
    printf("Loading configuration\n");
#endif
    
    int fd = open(file, O_RDONLY);
    char * line;
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
        
        // this one can be 0
        p->make_albumdir = read_line_num(fd);
        
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
        
        aCharPtr = read_line(fd);
        if (aCharPtr != NULL)
        {
            if (p->invalid_chars != NULL)
                free(p->invalid_chars);
            p->invalid_chars = aCharPtr;
        }
        
        anInt = read_line_num(fd);
        if (anInt != 0)
            p->main_window_width = anInt;
        
        anInt = read_line_num(fd);
        if (anInt != 0)
            p->main_window_height = anInt;
        
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
        {
            fprintf(stderr, "malloc(sizeof(char) * (strlen(home)+1)) failed\n");
            exit(-1);
        }
        
        strncpy(p->music_dir, home, strlen(home)+1);
        
        save_prefs(p);
    }
    return p->music_dir;
}

// converts a gtk slider's integer range to a meaningful bitrate
//
// NOTE: i grabbed these bitrates from the list in the LAME man page
int int_to_bitrate(int i)
{
    switch (i)
    {
    case 0:
        return 32;
    case 1:
        return 40;
    case 2:
        return 48;
    case 3:
        return 56;
    case 4:
        return 64;
    case 5:
        return 80;
    case 6:
        return 96;
    case 7:
        return 112;
    case 8:
        return 128;
    case 9:
        return 160;
    case 10:
        return 192;
    case 11:
        return 224;
    case 12:
        return 256;
    case 13:
        return 320;
    }
}

