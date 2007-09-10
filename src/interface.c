/*
Asunder

Copyright(C) 2005 Eric Lathrop <eric@ericlathrop.com>
Copyright(C) 2007 Andrew Smith <http://littlesvr.ca/misc/contactandrew.php>

Any code in this file may be redistributed or modified under the terms of
the GNU General Public Licence as published by the Free Software 
Foundation; version 2 of the licence.

*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "main.h"
#include "prefs.h"
#include "callbacks.h"
#include "interface.h"
#include "support.h"

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)

GtkWidget*
create_main (void)
{
  GtkWidget *main_win;
  GdkPixbuf *main_icon_pixbuf;
  GtkWidget *vbox1;
  GtkWidget *toolbar1;
  gint tmp_toolbar_icon_size;
  GtkWidget *refresh;
  GtkWidget *preferences;
  GtkWidget *separatortoolitem1;
  GtkWidget *table2;
  GtkWidget *album_artist;
  GtkWidget *album_title;
  GtkWidget *pick_disc;
  GtkWidget *disc;
  GtkWidget *artist_label;
  GtkWidget *title_label;
  GtkWidget *single_artist;
  GtkWidget *scrolledwindow1;
  GtkWidget *tracklist;
  GtkWidget *alignment4;
  GtkWidget *rip_button;
  GtkWidget *alignment3;
  GtkWidget *hbox4;
  GtkWidget *image1;
  GtkWidget *label8;

  main_win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (main_win), "Asunder");
  
  gtk_window_set_default_size (GTK_WINDOW (main_win), global_prefs->main_window_width, global_prefs->main_window_height);
  main_icon_pixbuf = create_pixbuf ("asunder.png");
  if (main_icon_pixbuf)
  {
    gtk_window_set_icon (GTK_WINDOW (main_win), main_icon_pixbuf);
    gdk_pixbuf_unref (main_icon_pixbuf);
  }

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (main_win), vbox1);

  toolbar1 = gtk_toolbar_new ();
  gtk_widget_show (toolbar1);
  gtk_box_pack_start (GTK_BOX (vbox1), toolbar1, FALSE, FALSE, 0);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar1), GTK_TOOLBAR_BOTH_HORIZ);
  tmp_toolbar_icon_size = gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar1));

  refresh = (GtkWidget*) gtk_tool_button_new_from_stock ("gtk-refresh");
  gtk_widget_show (refresh);
  gtk_container_add (GTK_CONTAINER (toolbar1), refresh);
  gtk_tool_item_set_is_important (GTK_TOOL_ITEM (refresh), TRUE);

  preferences = (GtkWidget*) gtk_tool_button_new_from_stock ("gtk-preferences");
  gtk_widget_show (preferences);
  gtk_container_add (GTK_CONTAINER (toolbar1), preferences);
  gtk_tool_item_set_is_important (GTK_TOOL_ITEM (preferences), TRUE);

  separatortoolitem1 = (GtkWidget*) gtk_separator_tool_item_new ();
  gtk_widget_show (separatortoolitem1);
  gtk_container_add (GTK_CONTAINER (toolbar1), separatortoolitem1);

#if GTK_MINOR_VERSION >= 6
  GtkWidget *about;
  about = (GtkWidget*) gtk_tool_button_new_from_stock ("gtk-about");
  gtk_widget_show (about);
  gtk_container_add (GTK_CONTAINER (toolbar1), about);
  gtk_tool_item_set_is_important (GTK_TOOL_ITEM (about), TRUE);
#endif
  
  table2 = gtk_table_new (3, 3, FALSE);
  gtk_widget_show (table2);
  gtk_box_pack_start (GTK_BOX (vbox1), table2, FALSE, TRUE, 3);

  album_artist = gtk_entry_new ();
  gtk_widget_show (album_artist);
  gtk_table_attach (GTK_TABLE (table2), album_artist, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  album_title = gtk_entry_new ();
  gtk_widget_show (album_title);
  gtk_table_attach (GTK_TABLE (table2), album_title, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  pick_disc = gtk_combo_box_new ();
  gtk_table_attach (GTK_TABLE (table2), pick_disc, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  disc = gtk_label_new (_("Disc:"));
  gtk_table_attach (GTK_TABLE (table2), disc, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 3, 0);
  gtk_misc_set_alignment (GTK_MISC (disc), 0, 0.49);

  artist_label = gtk_label_new (_("Album artist:"));
  gtk_misc_set_alignment (GTK_MISC (artist_label), 0, 0);
  gtk_widget_show (artist_label);
  gtk_table_attach (GTK_TABLE (table2), artist_label, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 3, 0);

  title_label = gtk_label_new (_("Album title:"));
  gtk_misc_set_alignment (GTK_MISC (title_label), 0, 0);
  gtk_widget_show (title_label);
  gtk_table_attach (GTK_TABLE (table2), title_label, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 3, 0);

  single_artist = gtk_check_button_new_with_mnemonic (_("Single artist"));
  gtk_widget_show (single_artist);
  gtk_table_attach (GTK_TABLE (table2), single_artist, 2, 3, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 3, 0);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolledwindow1);
  gtk_box_pack_start (GTK_BOX (vbox1), scrolledwindow1, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  tracklist = gtk_tree_view_new ();
  gtk_widget_show (tracklist);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), tracklist);
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tracklist), TRUE);
  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (tracklist), FALSE);

  alignment4 = gtk_alignment_new (1, 0.5, 0, 1);
  gtk_widget_show (alignment4);
  gtk_box_pack_start (GTK_BOX (vbox1), alignment4, FALSE, FALSE, 0);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment4), 5, 5, 5, 5);

  rip_button = gtk_button_new ();
  gtk_widget_show (rip_button);
  gtk_container_add (GTK_CONTAINER (alignment4), rip_button);

  alignment3 = gtk_alignment_new (0.5, 0.5, 0, 0);
  gtk_widget_show (alignment3);
  gtk_container_add (GTK_CONTAINER (rip_button), alignment3);

  hbox4 = gtk_hbox_new (FALSE, 2);
  gtk_widget_show (hbox4);
  gtk_container_add (GTK_CONTAINER (alignment3), hbox4);

  image1 = gtk_image_new_from_stock ("gtk-cdrom", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show (image1);
  gtk_box_pack_start (GTK_BOX (hbox4), image1, FALSE, FALSE, 0);

  label8 = gtk_label_new_with_mnemonic (_("Rip"));
  gtk_widget_show (label8);
  gtk_box_pack_start (GTK_BOX (hbox4), label8, FALSE, FALSE, 0);

  g_signal_connect ((gpointer) main_win, "delete_event",
                    G_CALLBACK (on_window_close),
                    NULL);
  g_signal_connect ((gpointer) refresh, "clicked",
                    G_CALLBACK (on_refresh_clicked),
                    NULL);
  g_signal_connect ((gpointer) preferences, "clicked",
                    G_CALLBACK (on_preferences_clicked),
                    NULL);
#if GTK_MINOR_VERSION >= 6
  g_signal_connect ((gpointer) about, "clicked",
                    G_CALLBACK (on_about_clicked),
                    NULL);
#endif
  g_signal_connect ((gpointer) album_artist, "focus_out_event",
                    G_CALLBACK (on_album_artist_focus_out_event),
                    NULL);
  g_signal_connect ((gpointer) album_title, "focus_out_event",
                    G_CALLBACK (on_album_title_focus_out_event),
                    NULL);
  g_signal_connect ((gpointer) pick_disc, "changed",
                    G_CALLBACK (on_pick_disc_changed),
                    NULL);
  g_signal_connect ((gpointer) single_artist, "toggled",
                    G_CALLBACK (on_single_artist_toggled),
                    NULL);
  g_signal_connect ((gpointer) rip_button, "clicked",
                    G_CALLBACK (on_rip_button_clicked),
                    NULL);

  /* KEYBOARD accelerators */
  GtkAccelGroup* accelGroup;
  guint accelKey;
  GdkModifierType accelModifier;
  GClosure *closure = NULL;
  
  accelGroup = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(main_win), accelGroup);
  
  gtk_accelerator_parse("<Control>W", &accelKey, &accelModifier);
  closure = g_cclosure_new(G_CALLBACK(on_window_close), NULL, NULL);
  gtk_accel_group_connect(accelGroup, accelKey, accelModifier, GTK_ACCEL_VISIBLE, closure);
  
  gtk_accelerator_parse("<Control>Q", &accelKey, &accelModifier);
  closure = g_cclosure_new(G_CALLBACK(on_window_close), NULL, NULL);
  gtk_accel_group_connect(accelGroup, accelKey, accelModifier, GTK_ACCEL_VISIBLE, closure);
  /* END KEYBOARD accelerators */

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (main_win, main_win, "main");
  GLADE_HOOKUP_OBJECT (main_win, vbox1, "vbox1");
  GLADE_HOOKUP_OBJECT (main_win, toolbar1, "toolbar1");
  GLADE_HOOKUP_OBJECT (main_win, refresh, "refresh");
  GLADE_HOOKUP_OBJECT (main_win, preferences, "preferences");
  GLADE_HOOKUP_OBJECT (main_win, separatortoolitem1, "separatortoolitem1");
#if GTK_MINOR_VERSION >= 6
  GLADE_HOOKUP_OBJECT (main_win, about, "about");
#endif
  GLADE_HOOKUP_OBJECT (main_win, table2, "table2");
  GLADE_HOOKUP_OBJECT (main_win, album_artist, "album_artist");
  GLADE_HOOKUP_OBJECT (main_win, album_title, "album_title");
  GLADE_HOOKUP_OBJECT (main_win, pick_disc, "pick_disc");
  GLADE_HOOKUP_OBJECT (main_win, disc, "disc");
  GLADE_HOOKUP_OBJECT (main_win, artist_label, "artist_label");
  GLADE_HOOKUP_OBJECT (main_win, title_label, "title_label");
  GLADE_HOOKUP_OBJECT (main_win, single_artist, "single_artist");
  GLADE_HOOKUP_OBJECT (main_win, scrolledwindow1, "scrolledwindow1");
  GLADE_HOOKUP_OBJECT (main_win, tracklist, "tracklist");
  GLADE_HOOKUP_OBJECT (main_win, alignment4, "alignment4");
  GLADE_HOOKUP_OBJECT (main_win, rip_button, "rip_button");
  GLADE_HOOKUP_OBJECT (main_win, alignment3, "alignment3");
  GLADE_HOOKUP_OBJECT (main_win, hbox4, "hbox4");
  GLADE_HOOKUP_OBJECT (main_win, image1, "image1");
  GLADE_HOOKUP_OBJECT (main_win, label8, "label8");

  return main_win;
}

GtkWidget*
create_prefs (void)
{
  GtkWidget *prefs;
  GtkWidget *dialog_vbox1;
  GtkWidget *notebook1;
  GtkWidget *vbox5;
  GtkWidget *music_dir;
  GtkWidget *label15;
  GtkWidget *make_playlist;
  GtkWidget *make_albumdir;
  GtkWidget *hbox12;
  GtkWidget *label28;
  GtkWidget *cdrom;
  GtkWidget *label4;
  GtkWidget *vbox7;
  GtkWidget *frame2;
  GtkWidget *vbox10;
  GtkWidget *label14;
  GtkWidget *table1;
  GtkWidget *label11;
  GtkWidget *label12;
  GtkWidget *label17;
  GtkWidget *format_music;
  GtkWidget *format_albumdir;
  GtkWidget *format_playlist;
  GtkWidget *label20;
  GtkWidget *hbox13;
  GtkWidget *label29;
  GtkWidget *invalid_chars;
  GtkWidget *label19;
  GtkWidget *vbox8;
  GtkWidget *rip_wav;
  GtkWidget *frame3;
  GtkWidget *alignment8;
  GtkWidget *vbox9;
  GtkWidget *mp3_vbr;
  GtkWidget *hbox9;
  GtkWidget *label23;
  GtkWidget *mp3bitrate;
  GtkWidget *mp3_bitrate;
  GtkWidget *rip_mp3;
  GtkWidget *frame4;
  GtkWidget *alignment9;
  GtkWidget *hbox10;
  GtkWidget *label24;
  GtkWidget *oggquality;
  GtkWidget *rip_ogg;
  GtkWidget *frame5;
  GtkWidget *alignment10;
  GtkWidget *hbox11;
  GtkWidget *label25;
  GtkWidget *flaccompression;
  GtkWidget *rip_flac;
  GtkWidget *label6;
  GtkWidget *dialog_action_area1;
  GtkWidget *cancelbutton1;
  GtkWidget *okbutton1;
  GtkWidget *eject_on_done;
  GtkTooltips *tooltips;
  GtkTooltips *tooltips1;
  GtkTooltips *tooltips2;
  GtkTooltips *tooltips3;
  GtkTooltips *tooltips4;
  
  tooltips = gtk_tooltips_new ();

  prefs = gtk_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW(prefs), GTK_WINDOW(win_main));
  gtk_window_set_title (GTK_WINDOW (prefs), _("Preferences"));
  gtk_window_set_modal (GTK_WINDOW (prefs), TRUE);
  gtk_window_set_type_hint (GTK_WINDOW (prefs), GDK_WINDOW_TYPE_HINT_DIALOG);
  
  dialog_vbox1 = GTK_DIALOG (prefs)->vbox;
  gtk_widget_show (dialog_vbox1);

  notebook1 = gtk_notebook_new ();
  gtk_widget_show (notebook1);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), notebook1, TRUE, TRUE, 0);

  vbox5 = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox5), 5);
  gtk_widget_show (vbox5);
  gtk_container_add (GTK_CONTAINER (notebook1), vbox5);

  label15 = gtk_label_new (_("Destination folder"));
  gtk_misc_set_alignment(GTK_MISC(label15), 0, 0);
  gtk_widget_show (label15);
  gtk_box_pack_start (GTK_BOX (vbox5), label15, FALSE, FALSE, 0);
  gtk_label_set_use_markup (GTK_LABEL (label15), TRUE);

  music_dir = gtk_file_chooser_button_new(_("Destination folder"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  gtk_widget_show (music_dir);
  gtk_box_pack_start (GTK_BOX (vbox5), music_dir, FALSE, FALSE, 0);
  
  make_playlist = gtk_check_button_new_with_mnemonic (_("Create M3U playlist"));
  gtk_widget_show (make_playlist);
  gtk_box_pack_start (GTK_BOX (vbox5), make_playlist, FALSE, FALSE, 0);

  make_albumdir = gtk_check_button_new_with_mnemonic (_("Create directory for each album"));
  gtk_widget_show (make_albumdir);
  gtk_box_pack_start (GTK_BOX (vbox5), make_albumdir, FALSE, FALSE, 0);

  hbox12 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox12);
  gtk_box_pack_start (GTK_BOX (vbox5), hbox12, FALSE, FALSE, 0);

  label28 = gtk_label_new (_("CD-ROM device: "));
  gtk_widget_show (label28);
  gtk_box_pack_start (GTK_BOX (hbox12), label28, FALSE, FALSE, 0);

  cdrom = gtk_entry_new ();
  gtk_widget_show (cdrom);
  gtk_box_pack_start (GTK_BOX (hbox12), cdrom, TRUE, TRUE, 0);
  
  eject_on_done = gtk_check_button_new_with_mnemonic (_("Eject disc when finished"));
  gtk_widget_show (eject_on_done);
  gtk_box_pack_start (GTK_BOX (vbox5), eject_on_done, FALSE, FALSE, 5);

  GtkWidget* hboxFill;
  hboxFill = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hboxFill);
  gtk_box_pack_start (GTK_BOX (vbox5), hboxFill, TRUE, TRUE, 0);
  
  label4 = gtk_label_new (_("General"));
  gtk_widget_show (label4);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 0), label4);

  vbox7 = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox7), 5);
  gtk_widget_show (vbox7);
  gtk_container_add (GTK_CONTAINER (notebook1), vbox7);

  frame2 = gtk_frame_new (NULL);
  gtk_widget_show (frame2);
  gtk_box_pack_start (GTK_BOX (vbox7), frame2, FALSE, FALSE, 0);
  
  vbox10 = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox10), 5);
  gtk_widget_show (vbox10);
  gtk_container_add (GTK_CONTAINER (frame2), vbox10);
  
  label14 = gtk_label_new (_("%A - Artist\n%L - Album\n%N - Track number (2-digit)\n%T - Song title"));
  gtk_widget_show (label14);
  gtk_box_pack_start (GTK_BOX (vbox10), label14, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label14), 0, 0);
  gtk_misc_set_padding (GTK_MISC (label14), 0, 7);

  table1 = gtk_table_new (3, 2, FALSE);
  gtk_widget_show (table1);
  gtk_box_pack_start (GTK_BOX (vbox10), table1, TRUE, TRUE, 0);
  
  label11 = gtk_label_new (_("Music file: "));
  gtk_widget_show (label11);
  gtk_table_attach (GTK_TABLE (table1), label11, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label11), 0, 0);

  label12 = gtk_label_new (_("Playlist file: "));
  gtk_widget_show (label12);
  gtk_table_attach (GTK_TABLE (table1), label12, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label12), 0, 0);

  label17 = gtk_label_new (_("Album directory: "));
  gtk_widget_show (label17);
  gtk_table_attach (GTK_TABLE (table1), label17, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label17), 0, 0);

  format_music = gtk_entry_new ();
  gtk_widget_show (format_music);
  gtk_table_attach (GTK_TABLE (table1), format_music, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  format_albumdir = gtk_entry_new ();
  gtk_widget_show (format_albumdir);
  gtk_table_attach (GTK_TABLE (table1), format_albumdir, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  format_playlist = gtk_entry_new ();
  gtk_widget_show (format_playlist);
  gtk_table_attach (GTK_TABLE (table1), format_playlist, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label20 = gtk_label_new (_("Filename formats"));
  gtk_widget_show (label20);
  gtk_frame_set_label_widget (GTK_FRAME (frame2), label20);
  gtk_label_set_use_markup (GTK_LABEL (label20), TRUE);

  hbox13 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox13);
  gtk_box_pack_start (GTK_BOX (vbox7), hbox13, FALSE, TRUE, 0);

  label29 = gtk_label_new (_("Invalid characters"));
  gtk_widget_show (label29);
  gtk_box_pack_start (GTK_BOX (hbox13), label29, FALSE, FALSE, 0);

  invalid_chars = gtk_entry_new ();
  gtk_widget_show (invalid_chars);
  gtk_box_pack_start (GTK_BOX (hbox13), invalid_chars, TRUE, TRUE, 3);
  gtk_tooltips_set_tip (tooltips, invalid_chars, _("These characters will be removed from all filenames."), NULL);

  label19 = gtk_label_new (_("Filenames"));
  gtk_widget_show (label19);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 1), label19);

  vbox8 = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox8), 5);
  gtk_widget_show (vbox8);
  gtk_container_add (GTK_CONTAINER (notebook1), vbox8);

  rip_wav = gtk_check_button_new_with_mnemonic (_("WAV (uncompressed)"));
  gtk_widget_show (rip_wav);
  gtk_box_pack_start (GTK_BOX (vbox8), rip_wav, FALSE, FALSE, 0);

  frame3 = gtk_frame_new (NULL);
  gtk_widget_show (frame3);
  gtk_box_pack_start (GTK_BOX (vbox8), frame3, TRUE, TRUE, 0);

  alignment8 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_show (alignment8);
  gtk_container_add (GTK_CONTAINER (frame3), alignment8);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment8), 0, 0, 12, 0);

  vbox9 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox9);
  gtk_container_add (GTK_CONTAINER (alignment8), vbox9);

  mp3_vbr = gtk_check_button_new_with_mnemonic (_("Variable bit rate (VBR)"));
  gtk_widget_show (mp3_vbr);
  gtk_box_pack_start (GTK_BOX (vbox9), mp3_vbr, FALSE, FALSE, 0);

  tooltips4 = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltips4, mp3_vbr, _("Better quality for the same size."), NULL);
  
  hbox9 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox9);
  gtk_box_pack_start (GTK_BOX (vbox9), hbox9, TRUE, TRUE, 0);

  label23 = gtk_label_new (_("Bitrate"));
  gtk_widget_show (label23);
  gtk_box_pack_start (GTK_BOX (hbox9), label23, FALSE, FALSE, 0);

  mp3bitrate = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 14, 1, 1, 1)));
  gtk_widget_show (mp3bitrate);
  gtk_box_pack_start (GTK_BOX (hbox9), mp3bitrate, TRUE, TRUE, 5);
  gtk_scale_set_draw_value (GTK_SCALE (mp3bitrate), FALSE);
  gtk_scale_set_digits (GTK_SCALE (mp3bitrate), 0);
  
  tooltips1 = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltips1, mp3bitrate, _("Higher bitrate is better quality but also bigger file. Most people use 192Kbps."), NULL);
  
  mp3_bitrate = gtk_label_new (_("32Kbps"));
  gtk_widget_show (mp3_bitrate);
  gtk_box_pack_start (GTK_BOX (hbox9), mp3_bitrate, FALSE, FALSE, 0);

  rip_mp3 = gtk_check_button_new_with_mnemonic (_("MP3 (lossy compression)"));
  gtk_widget_show (rip_mp3);
  gtk_frame_set_label_widget (GTK_FRAME (frame3), rip_mp3);

  frame4 = gtk_frame_new (NULL);
  gtk_widget_show (frame4);
  gtk_box_pack_start (GTK_BOX (vbox8), frame4, TRUE, TRUE, 0);

  alignment9 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_show (alignment9);
  gtk_container_add (GTK_CONTAINER (frame4), alignment9);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment9), 0, 0, 12, 0);

  hbox10 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox10);
  gtk_container_add (GTK_CONTAINER (alignment9), hbox10);

  label24 = gtk_label_new (_("Quality"));
  gtk_widget_show (label24);
  gtk_box_pack_start (GTK_BOX (hbox10), label24, FALSE, FALSE, 0);

  oggquality = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (6, 0, 11, 1, 1, 1)));
  gtk_widget_show (oggquality);
  gtk_box_pack_start (GTK_BOX (hbox10), oggquality, TRUE, TRUE, 5);
  gtk_scale_set_digits (GTK_SCALE (oggquality), 0);

  tooltips2 = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltips2, oggquality, _("Higher quality means bigger file. Default is 6."), NULL);
  
  rip_ogg = gtk_check_button_new_with_mnemonic (_("OGG Vorbis (lossy compression)"));
  gtk_widget_show (rip_ogg);
  gtk_frame_set_label_widget (GTK_FRAME (frame4), rip_ogg);

  frame5 = gtk_frame_new (NULL);
  gtk_widget_show (frame5);
  gtk_box_pack_start (GTK_BOX (vbox8), frame5, TRUE, TRUE, 0);

  alignment10 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_show (alignment10);
  gtk_container_add (GTK_CONTAINER (frame5), alignment10);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment10), 0, 0, 12, 0);

  hbox11 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox11);
  gtk_container_add (GTK_CONTAINER (alignment10), hbox11);

  label25 = gtk_label_new (_("Compression level"));
  gtk_widget_show (label25);
  gtk_box_pack_start (GTK_BOX (hbox11), label25, FALSE, FALSE, 0);

  flaccompression = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 9, 1, 1, 1)));
  gtk_widget_show (flaccompression);
  gtk_box_pack_start (GTK_BOX (hbox11), flaccompression, TRUE, TRUE, 5);
  gtk_scale_set_digits (GTK_SCALE (flaccompression), 0);

  tooltips3 = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltips3, flaccompression, _("This does not affect quality. Higher number means smaller file."), NULL);
  
  rip_flac = gtk_check_button_new_with_mnemonic (_("FLAC (lossless compression)"));
  gtk_widget_show (rip_flac);
  gtk_frame_set_label_widget (GTK_FRAME (frame5), rip_flac);

  label6 = gtk_label_new (_("Encode"));
  gtk_widget_show (label6);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 2), label6);

  dialog_action_area1 = GTK_DIALOG (prefs)->action_area;
  gtk_widget_show (dialog_action_area1);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_END);

  cancelbutton1 = gtk_button_new_from_stock ("gtk-cancel");
  gtk_widget_show (cancelbutton1);
  gtk_dialog_add_action_widget (GTK_DIALOG (prefs), cancelbutton1, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS (cancelbutton1, GTK_CAN_DEFAULT);

  okbutton1 = gtk_button_new_from_stock ("gtk-ok");
  gtk_widget_show (okbutton1);
  gtk_dialog_add_action_widget (GTK_DIALOG (prefs), okbutton1, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS (okbutton1, GTK_CAN_DEFAULT);

  g_signal_connect ((gpointer) prefs, "response",
                    G_CALLBACK (on_prefs_response),
                    NULL);
  g_signal_connect ((gpointer) prefs, "show",
                    G_CALLBACK (on_prefs_show),
                    NULL);
  g_signal_connect ((gpointer) mp3bitrate, "value_changed",
                    G_CALLBACK (on_mp3bitrate_value_changed),
                    NULL);
  g_signal_connect ((gpointer) rip_mp3, "toggled",
                    G_CALLBACK (on_rip_mp3_toggled),
                    NULL);
  g_signal_connect ((gpointer) rip_ogg, "toggled",
                    G_CALLBACK (on_rip_ogg_toggled),
                    NULL);
  g_signal_connect ((gpointer) rip_flac, "toggled",
                    G_CALLBACK (on_rip_flac_toggled),
                    NULL);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (prefs, prefs, "prefs");
  GLADE_HOOKUP_OBJECT_NO_REF (prefs, dialog_vbox1, "dialog_vbox1");
  GLADE_HOOKUP_OBJECT (prefs, notebook1, "notebook1");
  GLADE_HOOKUP_OBJECT (prefs, vbox5, "vbox5");
  GLADE_HOOKUP_OBJECT (prefs, music_dir, "music_dir");
  GLADE_HOOKUP_OBJECT (prefs, label15, "label15");
  GLADE_HOOKUP_OBJECT (prefs, make_playlist, "make_playlist");
  GLADE_HOOKUP_OBJECT (prefs, make_albumdir, "make_albumdir");
  GLADE_HOOKUP_OBJECT (prefs, hbox12, "hbox12");
  GLADE_HOOKUP_OBJECT (prefs, label28, "label28");
  GLADE_HOOKUP_OBJECT (prefs, cdrom, "cdrom");
  GLADE_HOOKUP_OBJECT (prefs, eject_on_done, "eject_on_done");
  GLADE_HOOKUP_OBJECT (prefs, label4, "label4");
  GLADE_HOOKUP_OBJECT (prefs, vbox7, "vbox7");
  GLADE_HOOKUP_OBJECT (prefs, frame2, "frame2");
  GLADE_HOOKUP_OBJECT (prefs, vbox10, "vbox10");
  GLADE_HOOKUP_OBJECT (prefs, label14, "label14");
  GLADE_HOOKUP_OBJECT (prefs, table1, "table1");
  GLADE_HOOKUP_OBJECT (prefs, label11, "label11");
  GLADE_HOOKUP_OBJECT (prefs, label12, "label12");
  GLADE_HOOKUP_OBJECT (prefs, label17, "label17");
  GLADE_HOOKUP_OBJECT (prefs, format_music, "format_music");
  GLADE_HOOKUP_OBJECT (prefs, format_albumdir, "format_albumdir");
  GLADE_HOOKUP_OBJECT (prefs, format_playlist, "format_playlist");
  GLADE_HOOKUP_OBJECT (prefs, label20, "label20");
  GLADE_HOOKUP_OBJECT (prefs, hbox13, "hbox13");
  GLADE_HOOKUP_OBJECT (prefs, label29, "label29");
  GLADE_HOOKUP_OBJECT (prefs, invalid_chars, "invalid_chars");
  GLADE_HOOKUP_OBJECT (prefs, label19, "label19");
  GLADE_HOOKUP_OBJECT (prefs, vbox8, "vbox8");
  GLADE_HOOKUP_OBJECT (prefs, rip_wav, "rip_wav");
  GLADE_HOOKUP_OBJECT (prefs, frame3, "frame3");
  GLADE_HOOKUP_OBJECT (prefs, alignment8, "alignment8");
  GLADE_HOOKUP_OBJECT (prefs, vbox9, "vbox9");
  GLADE_HOOKUP_OBJECT (prefs, mp3_vbr, "mp3_vbr");
  GLADE_HOOKUP_OBJECT (prefs, hbox9, "hbox9");
  GLADE_HOOKUP_OBJECT (prefs, label23, "label23");
  GLADE_HOOKUP_OBJECT (prefs, mp3bitrate, "mp3bitrate");
  GLADE_HOOKUP_OBJECT (prefs, mp3_bitrate, "mp3_bitrate");
  GLADE_HOOKUP_OBJECT (prefs, rip_mp3, "rip_mp3");
  GLADE_HOOKUP_OBJECT (prefs, frame4, "frame4");
  GLADE_HOOKUP_OBJECT (prefs, alignment9, "alignment9");
  GLADE_HOOKUP_OBJECT (prefs, hbox10, "hbox10");
  GLADE_HOOKUP_OBJECT (prefs, label24, "label24");
  GLADE_HOOKUP_OBJECT (prefs, oggquality, "oggquality");
  GLADE_HOOKUP_OBJECT (prefs, rip_ogg, "rip_ogg");
  GLADE_HOOKUP_OBJECT (prefs, frame5, "frame5");
  GLADE_HOOKUP_OBJECT (prefs, alignment10, "alignment10");
  GLADE_HOOKUP_OBJECT (prefs, hbox11, "hbox11");
  GLADE_HOOKUP_OBJECT (prefs, label25, "label25");
  GLADE_HOOKUP_OBJECT (prefs, flaccompression, "flaccompression");
  GLADE_HOOKUP_OBJECT (prefs, rip_flac, "rip_flac");
  GLADE_HOOKUP_OBJECT (prefs, label6, "label6");
  GLADE_HOOKUP_OBJECT_NO_REF (prefs, dialog_action_area1, "dialog_action_area1");
  GLADE_HOOKUP_OBJECT (prefs, cancelbutton1, "cancelbutton1");
  GLADE_HOOKUP_OBJECT (prefs, okbutton1, "okbutton1");
  GLADE_HOOKUP_OBJECT_NO_REF (prefs, tooltips, "tooltips");
  GLADE_HOOKUP_OBJECT_NO_REF (prefs, tooltips1, "tooltips1");
  GLADE_HOOKUP_OBJECT_NO_REF (prefs, tooltips2, "tooltips2");
  GLADE_HOOKUP_OBJECT_NO_REF (prefs, tooltips3, "tooltips3");
  GLADE_HOOKUP_OBJECT_NO_REF (prefs, tooltips4, "tooltips3");
  
  return prefs;
}

GtkWidget*
create_ripping (void)
{
  GtkWidget *ripping;
  GtkWidget *dialog_vbox2;
  GtkWidget *table3;
  GtkWidget *progress_total;
  GtkWidget *progress_rip;
  GtkWidget *progress_encode;
  GtkWidget *label25;
  GtkWidget *label26;
  GtkWidget *label27;
  GtkWidget *dialog_action_area2;
  GtkWidget *cancel;

  ripping = gtk_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW(ripping), GTK_WINDOW(win_main));
  gtk_window_set_title (GTK_WINDOW (ripping), _("Ripping"));
  gtk_window_set_modal (GTK_WINDOW (ripping), TRUE);
  gtk_window_set_type_hint (GTK_WINDOW (ripping), GDK_WINDOW_TYPE_HINT_DIALOG);

  dialog_vbox2 = GTK_DIALOG (ripping)->vbox;
  gtk_widget_show (dialog_vbox2);

  table3 = gtk_table_new (3, 2, FALSE);
  gtk_widget_show (table3);
  gtk_box_pack_start (GTK_BOX (dialog_vbox2), table3, TRUE, TRUE, 0);

  progress_total = gtk_progress_bar_new ();
  gtk_widget_show (progress_total);
  gtk_table_attach (GTK_TABLE (table3), progress_total, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  progress_rip = gtk_progress_bar_new ();
  gtk_widget_show (progress_rip);
  gtk_table_attach (GTK_TABLE (table3), progress_rip, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  progress_encode = gtk_progress_bar_new ();
  gtk_widget_show (progress_encode);
  gtk_table_attach (GTK_TABLE (table3), progress_encode, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label25 = gtk_label_new (_("Total progress"));
  gtk_widget_show (label25);
  gtk_table_attach (GTK_TABLE (table3), label25, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 5, 0);
  gtk_misc_set_alignment (GTK_MISC (label25), 0, 0.5);

  label26 = gtk_label_new (_("Ripping"));
  gtk_widget_show (label26);
  gtk_table_attach (GTK_TABLE (table3), label26, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 5, 0);
  gtk_misc_set_alignment (GTK_MISC (label26), 0, 0.5);

  label27 = gtk_label_new (_("Encoding"));
  gtk_widget_show (label27);
  gtk_table_attach (GTK_TABLE (table3), label27, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 5, 0);
  gtk_misc_set_alignment (GTK_MISC (label27), 0, 0.5);

  dialog_action_area2 = GTK_DIALOG (ripping)->action_area;
  gtk_widget_show (dialog_action_area2);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area2), GTK_BUTTONBOX_END);

  cancel = gtk_button_new_from_stock ("gtk-cancel");
  gtk_widget_show (cancel);
  gtk_dialog_add_action_widget (GTK_DIALOG (ripping), cancel, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS (cancel, GTK_CAN_DEFAULT);

  g_signal_connect ((gpointer) cancel, "clicked",
                    G_CALLBACK (on_cancel_clicked),
                    NULL);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (ripping, ripping, "ripping");
  GLADE_HOOKUP_OBJECT_NO_REF (ripping, dialog_vbox2, "dialog_vbox2");
  GLADE_HOOKUP_OBJECT (ripping, table3, "table3");
  GLADE_HOOKUP_OBJECT (ripping, progress_total, "progress_total");
  GLADE_HOOKUP_OBJECT (ripping, progress_rip, "progress_rip");
  GLADE_HOOKUP_OBJECT (ripping, progress_encode, "progress_encode");
  GLADE_HOOKUP_OBJECT (ripping, label25, "label25");
  GLADE_HOOKUP_OBJECT (ripping, label26, "label26");
  GLADE_HOOKUP_OBJECT (ripping, label27, "label27");
  GLADE_HOOKUP_OBJECT_NO_REF (ripping, dialog_action_area2, "dialog_action_area2");
  GLADE_HOOKUP_OBJECT (ripping, cancel, "cancel");

  return ripping;
}

#if GTK_MINOR_VERSION >= 6
static const char* 
GBLprogramName = "Asunder 0.8.1";

static const char* 
GBLauthors[2] = {
"Many thanks to all the following people:\n"
"\n"
"Andrew Smith\n"
"http://littlesvr.ca/misc/contactandrew.php\n"
"Summer 2005 - Summer 2007\n"
"- maintainer\n"
"\n"
"Eric Lathrop\n"
"http://ericlathrop.com/\n"
"- original author\n"
"- 'eject when finished' feature\n"
"\n"
"Packages:\n"
"\n"
"Toni Graffy\n"
"Maintainer of many SuSE packages at PackMan\n"
"- SuSE packages of Asunder, versions 0.1 - 0.8\n"
"\n"
"Trent Weddington\n"
"http://rtfm.insomnia.org/~qg/\n"
"- Debian packages of Asunder, version 0.8\n"
"\n"
"vktgz\n"
"http://www.vktgz.homelinux.net/\n"
"- Gentoo ebuild for Asunder, version 0.8\n"
"\n"
,
NULL};

static const char* 
GBLtranslators = 
"Schmaki\n"
"- cs (Czeck) translation of Asunder version 0.8\n"
"\n"
"Rene Schmidt\n"
"- de (German) translation of Asunder version 0.8\n"
"\n"
"Mike Kranidis\n"
"- el (Greek) translation of Asunder version 0.8\n"
"\n"
"Juan Garcia-Murga Monago\n"
"- es (Spanish) translation of Asunder version 0.8\n"
"\n"
"Eero Salokannel\n"
"- fi (Finnish) translation of Asunder version 0.8\n"
"\n"
"Christophe Legras\n"
"- fr (French) translation of Asunder version 0.8\n"
"\n"
"Valerio Guaglianone\n"
"- it (Italian) translation of Asunder version 0.8\n"
"\n"
"Robert Groenning\n"
"- nb (Norwegian Bokmal) translation of Asunder version 0.8\n"
"\n"
"Stephen Brandt\n"
"- nl (Dutch) translation of Asunder version 0.8\n"
"\n"
"Robert Groenning\n"
"- nn (Norwegian Nynorsk) translation of Asunder version 0.8\n"
"\n"
"Alexey Sivakov\n"
"- ru (Russian) translation of Asunder version 0.8\n"
"\n"
"Daniel Nylander\n"
"- sv (Swedish) translation of Asunder version 0.8\n"
"\n"
"Besnik Bleta\n"
"- sq (Albanian) translation of Asunder version 0.8\n"
"\n"
"Cheng-Wei Chien\n"
"- zh_TW (Chineese/Taiwan) translation of Asunder version 0.8\n"
"\n";

static const char* 
GBLcomments = 
"An application to save tracks from an Audio CD \n"
"as WAV, MP3, OGG, and/or FLAC.";

static const char* 
GBLcopyright = 
"Copyright 2005 Eric Lathrop\n"
"Copyright 2007 Andrew Smith";

static const char* 
GBLwebsite = "http://littlesvr.ca/asunder/";

static const char* 
GBLlicense = 
"Asunder is distributed under the GNU General Public Licence\n"
"version 2, please see COPYING file for the complete text\n";

void
show_aboutbox (void)
{
    gtk_show_about_dialog(GTK_WINDOW(lookup_widget(win_main, "main")), 
                          "name", GBLprogramName,
                          "authors", GBLauthors,
                          "translator-credits", GBLtranslators,
                          "comments", GBLcomments,
                          "copyright", GBLcopyright,
                          "license", GBLlicense,
                          "website", GBLwebsite,
                          NULL);
}
#endif

void show_completed_dialog(int numOk, int numFailed)
{
    GtkWidget* dialog;
    
    if (numFailed > 0)
    {
        dialog = gtk_message_dialog_new(GTK_WINDOW(win_main),
                                      GTK_DIALOG_DESTROY_WITH_PARENT,
                                      GTK_MESSAGE_WARNING,
                                      GTK_BUTTONS_CLOSE,
                                      "%d file(s) created successfully\n"
                                      "There was an error creating %d file(s)",
                                      numOk, numFailed);
    }
    else
    {
        dialog = gtk_message_dialog_new(GTK_WINDOW(win_main),
                                      GTK_DIALOG_DESTROY_WITH_PARENT,
                                      GTK_MESSAGE_INFO,
                                      GTK_BUTTONS_CLOSE,
                                      "All %d file(s) created successfully",
                                      numOk);
    }
    
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}
