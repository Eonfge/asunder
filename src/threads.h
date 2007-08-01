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

extern int aborted;

// aborts ripping- stops all the threads and return to normal execution
void abort_threads();

// spawn needed threads and begin ripping
void dorip();

// the thread that handles ripping tracks to WAV files
gpointer rip(gpointer data);

// the thread that handles encoding WAV files to all other formats
gpointer encode(gpointer data);

// the thread that calculates the progress of the other threads and updates the progress bars
gpointer track(gpointer data);
