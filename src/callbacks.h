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


void
on_rip_button_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_prefs_response                      (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data);

void
on_pick_match_changed                  (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_matches_response                    (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data);

void
on_browse_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_pick_disc_changed                   (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_single_artist_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_mp3_bitrate_value_changed           (GtkRange        *range,
                                        gpointer         user_data);

void
on_mp3bitrate_value_changed            (GtkRange        *range,
                                        gpointer         user_data);

void
rip_toggled                       (GtkCellRendererToggle *cell,
                                   gchar                 *path_string,
                                   gpointer               user_data);
void
artist_edited                    (GtkCellRendererText *cell,
                                  gchar               *path_string,
                                  gchar               *new_text,
                                  gpointer             user_data);

void
title_edited                     (GtkCellRendererText *cell,
                                  gchar               *path_string,
                                  gchar               *new_text,
                                  gpointer             user_data);


void
on_prefs_show                          (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_cancel_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_rip_mp3_toggled                     (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_rip_ogg_toggled                     (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_rip_flac_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data);
										
gboolean
idle(gpointer data);


void
on_about_clicked                       (GtkToolButton   *toolbutton,
                                        gpointer         user_data);

void
on_preferences_clicked                 (GtkToolButton   *toolbutton,
                                        gpointer         user_data);

void
on_refresh_clicked                     (GtkToolButton   *toolbutton,
                                        gpointer         user_data);

gboolean
on_album_artist_focus_out_event        (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

gboolean
on_album_title_focus_out_event         (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);


void
on_aboutbox_response                   (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data);
