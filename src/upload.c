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

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include "llog.h"
#include "db_sqlite.h"
#include "upload.h"


GtkWidget *upload_add_row(GtkWidget *grid, int row, const char *label, GtkWidget *entry) {
  GtkWidget *lbl = gtk_label_new(label);
  gtk_widget_set_halign(lbl, GTK_ALIGN_START);
  gtk_grid_attach(GTK_GRID(grid), lbl, 0, row, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), entry, 1, row, 1, 1);
  gtk_widget_set_hexpand(entry, TRUE);
  return entry;
}


void upload_append_text(GtkWidget *text_view, const char *text) {
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
  GtkTextIter end;

  gtk_text_buffer_get_end_iter(buffer, &end);
  gtk_text_buffer_insert(buffer, &end, text, -1);
  gtk_text_buffer_insert(buffer, &end, "\n", -1);

  GtkTextMark *mark = gtk_text_buffer_create_mark(buffer, NULL, &end, FALSE);
  gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(text_view), mark);
  gtk_text_buffer_delete_mark(buffer, mark);
}


/* Normalises the "From date" field to YYYY-MM-DD, the format of the log's date column.
 * An empty field gives an empty string (no filter). Returns false if the date is malformed.
 */
bool upload_parse_from_date(const char *text, char *from_date, size_t len) {
  char buf[UPLOAD_DATE_LEN];
  int year, month, day;

  g_strlcpy(buf, text, sizeof(buf));
  g_strstrip(buf);

  if (buf[0] == '\0') {
    from_date[0] = '\0';
    return true;
  }

  if (sscanf(buf, "%4d-%2d-%2d", &year, &month, &day) != 3 ||
      month < 1 || month > 12 || day < 1 || day > 31) {
    return false;
  }

  snprintf(from_date, len, "%04d-%02d-%02d", year, month, day);
  return true;
}


/* Appends the QSOs (upload_qso_t, newest first) made on or after from_date that have not
 * been uploaded to the service yet. Main thread only.
 */
int upload_collect_qsos(llog_t *llog, const char *service, const char *from_date,
                        GArray *qsos, guint *already_uploaded) {
  upload_qso_t qso;

  *already_uploaded = 0;

  if (llog->log_db == NULL) {
    return llog_stat_err;
  }

  qso.entry.data_stat = db_data_init;
  for (;;) {
    db_get_log_entry_with_station(llog, &qso.entry, &qso.station);
    if (qso.entry.data_stat != db_data_valid) {
      break;
    }
    if (from_date[0] != '\0' && strcmp(qso.entry.date, from_date) < 0) {
      continue;
    }
    if (db_is_uploaded(llog, qso.entry.id, service)) {
      (*already_uploaded)++;
      continue;
    }
    db_get_super_mode(llog, qso.entry.mode.name, qso.super_mode, sizeof(qso.super_mode));
    g_array_append_val(qsos, qso);
  }

  return qso.entry.data_stat == db_data_last ? llog_stat_ok : llog_stat_err;
}
