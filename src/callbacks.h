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
void
on_window_close	                       (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);
