#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

//#include <gtk/gtk.h>

#include "llog.h"
#define LLOG_COLUMNS 16
#define LLOG_ENTRIES 32


int main_window_draw(void);
int main_window_add_log_entry_to_list(log_entry_t *entry);
int main_window_add_station_entry_to_list(station_entry_t *station);
int main_window_add_mode_entry_to_list(mode_entry_t *mode);
void main_window_clear_log_list(void);
void main_window_clear_station_list(void);
void main_window_clear_modes_list(void);

#endif
