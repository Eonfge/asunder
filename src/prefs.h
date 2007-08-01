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
} prefs;

extern prefs * global_prefs;

// allocate memory for a new prefs struct
// and make sure everything is set to NULL
prefs * new_prefs();

// free memory allocated for prefs struct
// also frees any strings pointed to in the struct
void delete_prefs(prefs * p);

// returns a new prefs struct with all members set to nice default values
// this struct must be freed with delete_prefs()
prefs * get_default_prefs();

// sets up all of the widgets in the preferences dialog to
// match the given prefs struct
void set_widgets_from_prefs(prefs * p);

// makes a prefs struct from the current state of the widgets
prefs * get_prefs_from_widgets();

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

