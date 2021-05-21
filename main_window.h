#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

//#include <gtk/gtk.h>

#include "llog.h"
#define LLOG_COLUMNS 16


int main_window_draw(void);
int main_window_add_log_entry_to_list(log_entry_t *entry);
void main_window_clear_log_list(void);

#endif
