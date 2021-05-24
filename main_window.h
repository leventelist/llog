#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

//#include <gtk/gtk.h>

#include "llog.h"
#define LLOG_COLUMNS 16


int main_window_draw(void);
int main_window_add_log_entry_to_list(log_entry_t *entry);
int main_window_add_station_entry_to_list(station_entry_t *station);
void main_window_clear_log_list(void);
void main_window_clear_station_list(void);

#endif
