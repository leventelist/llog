/*	This is llog, a minimalist HAM logging software.
 *	Copyright (C) 2013-2024  Levente Kovacs
 *
 *	This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * http://levente.logonex.eu
 * ha5ogl.levente@gmail.com
 */


#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include "llog.h"


int main_window_draw(int argc, char *argv[]);
void main_window_set_llog(llog_t *llog);
void main_window_add_log_entry_to_list(log_entry_t *entry);
void main_window_add_station_entry_to_list(station_entry_t *station);
void main_window_add_mode_entry_to_list(mode_entry_t *mode);
void main_window_clear_log_list(void);
void main_window_clear_station_list(void);
void main_window_clear_modes_list(void);

#endif
