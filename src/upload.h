/*	This is llog, a minimalist HAM logging software.
 *	Copyright (C) 2013-2026  Levente Kovacs
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

/* Helpers shared by the upload windows (Upload menu). */

#ifndef UPLOAD_H
#define UPLOAD_H

#include <gtk/gtk.h>
#include "llog.h"

#define UPLOAD_DATE_LEN 16

typedef struct {
  log_entry_t entry;
  station_entry_t station;
  char super_mode[MODE_LEN];   /*ADIF mode of entry.mode, e.g. OLIVIA for "OLIVIA 8/250"*/
} upload_qso_t;

GtkWidget *upload_add_row(GtkWidget *grid, int row, const char *label, GtkWidget *entry);
void upload_append_text(GtkWidget *text_view, const char *text);
bool upload_parse_from_date(const char *text, char *from_date, size_t len);
int upload_collect_qsos(llog_t *llog, const char *service, const char *from_date,
                        GArray *qsos, guint *already_uploaded);

#endif
