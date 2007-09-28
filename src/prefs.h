#include <stdbool.h>

typedef struct _prefs
{
    char * cdrom;
    char * music_dir;
    int make_playlist;
    int make_albumdir;
    char * format_music;
    char * format_playlist;
    char * format_albumdir;
    int rip_wav;
    int rip_mp3;
    int rip_ogg;
    int rip_flac;
    int mp3_vbr;
    int mp3_bitrate;
    int ogg_quality;
    int flac_compression;
    char * invalid_chars;
    int main_window_width;
    int main_window_height;
    int eject_on_done;
    int do_cddb_updates;
    int use_proxy;
    char * server_name;
    int port_number;
    
} prefs;

#define DEFAULT_PROXY_PORT 8080

extern prefs * global_prefs;

// allocate memory for a new prefs struct
// and make sure everything is set to NULL
prefs * new_prefs();

void clear_prefs(prefs * p);

// free memory allocated for prefs struct
// also frees any strings pointed to in the struct
void delete_prefs(prefs * p);

// returns a new prefs struct with all members set to nice default values
// this struct must be freed with delete_prefs()
prefs * get_default_prefs();

// sets up all of the widgets in the preferences dialog to
// match the given prefs struct
void set_widgets_from_prefs(prefs * p);

void get_prefs_from_widgets(prefs * p);

// store the given prefs struct to the config file
void save_prefs(prefs * p);

// load the prefereces from the config file into the given prefs struct
void load_prefs(prefs * p);

// use this method when reading the "music_dir" field of a prefs struct
// it will make sure it always points to a nice directory
char * prefs_get_music_dir(prefs * p);

// converts a gtk slider's integer range to a meaningful bitrate
//
// NOTE: i grabbed these bitrates from the list in the LAME man page
int int_to_bitrate(int i);

int is_valid_port_number(int number);

bool prefs_are_valid(void);

bool string_has_slashes(const char* string);
