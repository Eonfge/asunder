#include <gtk/gtk.h>
#include <cddb/cddb.h>
#include <stdbool.h>

#define LOG_FILE "/tmp/asunder.log"

enum
{
    COL_RIPTRACK,
    COL_TRACKNUM,
    COL_TRACKARTIST,
    COL_TRACKTITLE,
    COL_TRACKTIME,
    COL_GENRE,
    COL_YEAR,
    NUM_COLS
};

// scan the cdrom device for a disc
// returns True if a disc is present and
//   is different from the last time this was called
bool check_disc(char * cdrom);

void clear_widgets();

// creates a tree model that represents the data in the cddb_disc_t
GtkTreeModel * create_model_from_disc(cddb_disc_t * disc);

// open/close the drive's tray
void eject_disc(char * cdrom);

// looks up the given cddb_disc_t in the online database, and fills in the values
GList * lookup_disc(cddb_disc_t * disc);

// reads the TOC of a cdrom into a CDDB struct
// returns the filled out struct
// so we can send it over the internet to lookup the disc
cddb_disc_t * read_disc(char * cdrom);

// the main logic for scanning the discs
void refresh(char * cdrom, int force);

// updates all the necessary widgets with the data for the given cddb_disc_t
void update_tracklist(cddb_disc_t * disc);

extern GList * disc_matches;

extern GtkWidget * win_main;
extern GtkWidget * win_prefs;
extern GtkWidget * win_ripping;
extern GtkWidget * win_about;

extern GtkWidget * tracklist;

extern int gbl_null_fd;

//#define DEBUG
