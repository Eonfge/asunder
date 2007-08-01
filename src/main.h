/*
Asunder

Copyright(C) 2005 Eric Lathrop <eric@ericlathrop.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

For more details see the file COPYING
*/
#include <gtk/gtk.h>
#include <cddb/cddb.h>

enum
{
	COL_RIPTRACK,
	COL_TRACKNUM,
	COL_TRACKARTIST,
	COL_TRACKTITLE,
	COL_TRACKTIME,
	NUM_COLS
};

// scan the cdrom device for a disc
// if the tray has been opened since last time we were called
//     then we go to CDDB, and update the widgets
// if (force == TRUE)
//     then we updated everything no matter what
void check_disc(char * cdrom, int force);

// looks up the given cddb_disc_t in the online database, and fills in the values
void lookup_disc(cddb_disc_t * disc);

// creates a tree model that represents the data in the cddb_disc_t
GtkTreeModel * create_model_from_disc(cddb_disc_t * disc);

// updates all the necessary widgets with the data for the given cddb_disc_t
void update_tracklist(cddb_disc_t * disc);

extern cddb_disc_t * current_disc;
extern GList * disc_matches;

extern GtkWidget * win_main;
extern GtkWidget * win_prefs;
extern GtkWidget * win_ripping;
extern GtkWidget * win_about;

extern GtkWidget * tracklist;

//#define DEBUG
