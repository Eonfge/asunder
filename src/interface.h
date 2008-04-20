#include <stdbool.h>

GtkWidget* create_main (void);
GtkWidget* create_prefs (void);
GtkWidget* create_ripping (void);
void disable_all_main_widgets(void);
void enable_all_main_widgets(void);
void disable_flac_widgets(void);
void enable_flac_widgets(void);
void disable_mp3_widgets(void);
void enable_mp3_widgets(void);
void disable_ogg_widgets(void);
void enable_ogg_widgets(void);
void disable_wavpack_widgets(void);
void enable_wavpack_widgets(void);
#if GTK_MINOR_VERSION >= 6
void show_aboutbox (void);
#endif
void show_completed_dialog(int numOk, int numFailed);
