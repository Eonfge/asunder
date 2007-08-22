#include <stdbool.h>

GtkWidget* create_main (void);
GtkWidget* create_prefs (void);
GtkWidget* create_ripping (void);
#if GTK_MINOR_VERSION >= 6
void show_aboutbox (void);
#endif
void show_completed_dialog(int numOk, int numFailed);
