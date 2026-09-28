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

/* Upload window for ARRL Logbook of The World.
 *
 * LoTW only accepts logs signed with the operator's callsign certificate, so llog writes the QSOs
 * to a temporary ADIF file and lets TQSL sign and upload it:
 *
 *   tqsl -x -d -u -a compliant -l <station location> <file>
 *
 * TQSL reports one result for the whole file, through its exit code.
 */

#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <string.h>
#include <unistd.h>
#include "llog.h"
#include "db_sqlite.h"
#include "band.h"
#include "exporter_writer.h"
#include "upload.h"
#include "lotw_window.h"

#define LOTW_SERVICE "LOTW"

typedef struct lotw_run lotw_run_t;

typedef struct {
  GtkWidget *window;
  GtkWidget *tqsl_path_entry;
  GtkWidget *location_entry;
  GtkWidget *from_date_entry;
  GtkWidget *text_view;
  GtkWidget *button_upload;
  GtkWidget *button_stop;
  GtkWidget *button_close;
  llog_t *llog;
} lotw_widgets_t;

/*One TQSL run. Outlives the window if it is closed while TQSL is still running.*/
struct lotw_run {
  lotw_widgets_t *widgets;         /*NULL once the window is gone*/
  llog_t *llog;
  char log_file_name[FILE_LEN];    /*The log the QSOs came from*/
  char *adif_path;
  GSubprocess *process;
  GArray *qsos;                    /*upload_qso_t in the ADIF file*/
};

static lotw_widgets_t *widgets = NULL;
static lotw_run_t *running = NULL;

static void on_lotw_window_destroy(GtkWidget *widget, gpointer data);
static void on_button_upload_clicked(GtkWidget *widget, gpointer data);
static void on_button_stop_clicked(GtkWidget *widget, gpointer data);
static void on_button_close_clicked(GtkWidget *widget, gpointer data);


void on_upload_lotw_window_activate(GtkWidget *widget, gpointer data) {
  (void)widget;

  if (widgets != NULL) {
    gtk_window_present(GTK_WINDOW(widgets->window));
    return;
  }

  widgets = g_malloc0(sizeof(lotw_widgets_t));
  widgets->llog = (llog_t *)data;

  widgets->window = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(widgets->window), "Upload to LoTW");
  gtk_window_set_default_size(GTK_WINDOW(widgets->window), 700, 450);
  g_signal_connect(widgets->window, "destroy", G_CALLBACK(on_lotw_window_destroy), widgets);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(box, 10);
  gtk_widget_set_margin_end(box, 10);
  gtk_widget_set_margin_top(box, 10);
  gtk_widget_set_margin_bottom(box, 10);
  gtk_window_set_child(GTK_WINDOW(widgets->window), box);

  GtkWidget *grid = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
  gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
  gtk_box_append(GTK_BOX(box), grid);

  widgets->tqsl_path_entry = upload_add_row(grid, 0, "TQSL program:", gtk_entry_new());
  gtk_editable_set_text(GTK_EDITABLE(widgets->tqsl_path_entry), widgets->llog->tqsl_path);

  widgets->location_entry = upload_add_row(grid, 1, "Station location:", gtk_entry_new());
  gtk_entry_set_placeholder_text(GTK_ENTRY(widgets->location_entry), "Name of the station location in TQSL");
  gtk_editable_set_text(GTK_EDITABLE(widgets->location_entry), widgets->llog->tqsl_station_location);

  widgets->from_date_entry = upload_add_row(grid, 2, "From date:", gtk_entry_new());
  gtk_entry_set_placeholder_text(GTK_ENTRY(widgets->from_date_entry), "YYYY-MM-DD, empty: all QSOs");

  GtkWidget *hint = gtk_label_new("TQSL signs the QSOs with your callsign certificate and uploads them. "
                                  "Set up the certificate and the station location in TQSL first.\n"
                                  "QSOs already uploaded from this log are not sent again.");
  gtk_widget_set_halign(hint, GTK_ALIGN_START);
  gtk_label_set_wrap(GTK_LABEL(hint), TRUE);
  gtk_grid_attach(GTK_GRID(grid), hint, 0, 3, 2, 1);

  GtkWidget *scrolled = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scrolled, TRUE);
  widgets->text_view = gtk_text_view_new();
  gtk_text_view_set_editable(GTK_TEXT_VIEW(widgets->text_view), FALSE);
  gtk_text_view_set_monospace(GTK_TEXT_VIEW(widgets->text_view), TRUE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(widgets->text_view), GTK_WRAP_WORD_CHAR);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), widgets->text_view);
  gtk_box_append(GTK_BOX(box), scrolled);

  GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_append(GTK_BOX(box), button_box);

  widgets->button_upload = gtk_button_new_with_label("Upload");
  widgets->button_stop = gtk_button_new_with_label("Stop");
  widgets->button_close = gtk_button_new_with_label("Close");
  gtk_box_append(GTK_BOX(button_box), widgets->button_upload);
  gtk_box_append(GTK_BOX(button_box), widgets->button_stop);
  gtk_box_append(GTK_BOX(button_box), widgets->button_close);

  /*A run started from a window that has since been closed may still be going; take it over*/
  gtk_widget_set_sensitive(widgets->button_upload, running == NULL);
  gtk_widget_set_sensitive(widgets->button_stop, running != NULL);

  g_signal_connect(widgets->button_upload, "clicked", G_CALLBACK(on_button_upload_clicked), widgets);
  g_signal_connect(widgets->button_stop, "clicked", G_CALLBACK(on_button_stop_clicked), widgets);
  g_signal_connect(widgets->button_close, "clicked", G_CALLBACK(on_button_close_clicked), widgets);

  if (running != NULL) {
    running->widgets = widgets;
    upload_append_text(widgets->text_view, "TQSL is still running from a previous upload.");
  }

  gtk_window_present(GTK_WINDOW(widgets->window));
}


/*Messages for TQSL's exit codes, from the TQSL command line documentation*/
static const char *lotw_exit_message(int code) {
  switch (code) {
  case 0:  return "All QSOs were signed and uploaded.";
  case 1:  return "Cancelled.";
  case 2:  return "Rejected by LoTW.";
  case 3:  return "Unexpected response from the TQSL server.";
  case 4:  return "TQSL error.";
  case 5:  return "TQSLlib error.";
  case 6:  return "TQSL could not open the input file.";
  case 7:  return "TQSL could not open the output file.";
  case 8:  return "No QSOs uploaded: all were duplicates or outside the certificate's date range.";
  case 9:  return "Some QSOs were skipped as duplicates or outside the certificate's date range; "
                  "the rest were uploaded.";
  case 10: return "TQSL command syntax error.";
  case 11: return "Could not connect to LoTW.";
  default: return "Unknown TQSL exit code.";
  }
}


static void lotw_run_free(lotw_run_t *run) {
  if (run->adif_path != NULL) {
    g_unlink(run->adif_path);
    g_free(run->adif_path);
  }
  g_clear_object(&run->process);
  g_array_free(run->qsos, TRUE);
  g_free(run);
}


static void lotw_log(lotw_run_t *run, const char *text) {
  if (run->widgets != NULL) {
    upload_append_text(run->widgets->text_view, text);
  }
}


static void on_tqsl_finished(GObject *source, GAsyncResult *result, gpointer data) {
  lotw_run_t *run = (lotw_run_t *)data;
  GSubprocess *process = G_SUBPROCESS(source);
  GError *error = NULL;
  char *output = NULL;
  char *text;

  if (!g_subprocess_communicate_utf8_finish(process, result, &output, NULL, &error)) {
    text = g_strdup_printf("Error reading TQSL output: %s", error->message);
    lotw_log(run, text);
    g_free(text);
    g_clear_error(&error);
  }

  if (output != NULL && output[0] != '\0') {
    g_strchomp(output);
    lotw_log(run, output);
  }
  g_free(output);

  if (!g_subprocess_get_if_exited(process)) {
    lotw_log(run, "TQSL was stopped. Nothing was recorded as uploaded; "
                  "LoTW ignores QSOs it already has if you upload again.");
  } else {
    int code = g_subprocess_get_exit_status(process);
    text = g_strdup_printf("TQSL exit code %d: %s", code, lotw_exit_message(code));
    lotw_log(run, text);
    g_free(text);

    /*TQSL reports per file, not per QSO. Skipped QSOs are duplicates or outside the certificate's
     *date range; sending them again would not change that, so the whole file counts as done.*/
    if (code == 0 || code == 8 || code == 9) {
      guint recorded = 0;

      if (strcmp(run->llog->log_file_name, run->log_file_name) == 0) {
        for (guint i = 0; i < run->qsos->len; i++) {
          upload_qso_t *qso = &g_array_index(run->qsos, upload_qso_t, i);
          if (db_set_uploaded(run->llog, qso->entry.id, LOTW_SERVICE, NULL) == llog_stat_ok) {
            recorded++;
          }
        }
      }

      if (recorded == run->qsos->len) {
        text = g_strdup_printf("%u QSOs recorded as uploaded to LoTW.", recorded);
      } else {
        text = g_strdup_printf("WARNING: only %u of %u QSOs could be recorded as uploaded; "
                               "the rest may be sent again.", recorded, run->qsos->len);
      }
      lotw_log(run, text);
      g_free(text);
    }
  }

  if (run->widgets != NULL) {
    gtk_widget_set_sensitive(run->widgets->button_upload, TRUE);
    gtk_widget_set_sensitive(run->widgets->button_stop, FALSE);
  }

  running = NULL;
  lotw_run_free(run);
}


/* Writes the QSOs LoTW can take to an ADIF file, and keeps only those in run->qsos. */
static bool lotw_write_adif(lotw_run_t *run, GArray *qsos) {
  GError *error = NULL;
  int fd;

  fd = g_file_open_tmp("llog-lotw-XXXXXX.adi", &run->adif_path, &error);
  if (fd < 0) {
    char *text = g_strdup_printf("Could not create a temporary file: %s", error->message);
    lotw_log(run, text);
    g_free(text);
    g_clear_error(&error);
    return false;
  }
  close(fd);

  if (exporter_write_header(run->adif_path, export_format_adi) != export_status_ok) {
    lotw_log(run, "Could not write the ADIF file.");
    return false;
  }

  bool ok = true;
  for (guint i = 0; i < qsos->len; i++) {
    upload_qso_t *qso = &g_array_index(qsos, upload_qso_t, i);
    log_entry_t *e = &qso->entry;

    /*TQSL rejects the whole file if a record lacks these*/
    if (e->call[0] == '\0' || e->mode.name[0] == '\0' || strcmp(band_find(e->qrg), "Unknown") == 0) {
      char *text = g_strdup_printf("SKIPPED %s %s %-12s missing callsign, mode or valid frequency",
                                   e->date, e->utc, e->call);
      lotw_log(run, text);
      g_free(text);
      continue;
    }

    if (exporter_add_qso(e, &qso->station) != export_status_ok) {
      lotw_log(run, "Could not write the ADIF file.");
      ok = false;
      break;
    }
    g_array_append_val(run->qsos, *qso);
  }

  exporter_close();
  return ok;
}


static void on_button_upload_clicked(GtkWidget *widget, gpointer data) {
  (void)widget;
  lotw_widgets_t *w = (lotw_widgets_t *)data;
  llog_t *llog = w->llog;
  char from_date[UPLOAD_DATE_LEN];
  guint already_uploaded = 0;
  GError *error = NULL;
  char *text;

  if (running != NULL) {
    return;
  }

  char *path = g_strstrip(g_strdup(gtk_editable_get_text(GTK_EDITABLE(w->tqsl_path_entry))));
  char *location = g_strstrip(g_strdup(gtk_editable_get_text(GTK_EDITABLE(w->location_entry))));
  g_strlcpy(llog->tqsl_path, path[0] != '\0' ? path : "tqsl", sizeof(llog->tqsl_path));
  g_strlcpy(llog->tqsl_station_location, location, sizeof(llog->tqsl_station_location));
  g_free(path);
  g_free(location);
  llog_save_config_file();

  if (llog->tqsl_station_location[0] == '\0') {
    upload_append_text(w->text_view, "Enter the name of your station location in TQSL.");
    return;
  }

  if (!upload_parse_from_date(gtk_editable_get_text(GTK_EDITABLE(w->from_date_entry)),
                              from_date, sizeof(from_date))) {
    upload_append_text(w->text_view, "From date must be YYYY-MM-DD, or empty for all QSOs.");
    return;
  }

  if (llog->log_db == NULL) {
    upload_append_text(w->text_view, "No log file is open.");
    return;
  }

  GArray *qsos = g_array_new(FALSE, FALSE, sizeof(upload_qso_t));
  if (upload_collect_qsos(llog, LOTW_SERVICE, from_date, qsos, &already_uploaded) != llog_stat_ok) {
    upload_append_text(w->text_view, "Error reading the log.");
    g_array_free(qsos, TRUE);
    return;
  }

  if (qsos->len == 0) {
    text = g_strdup_printf("Nothing to upload (%u QSOs already uploaded).", already_uploaded);
    upload_append_text(w->text_view, text);
    g_free(text);
    g_array_free(qsos, TRUE);
    return;
  }

  lotw_run_t *run = g_malloc0(sizeof(lotw_run_t));
  run->widgets = w;
  run->llog = llog;
  g_strlcpy(run->log_file_name, llog->log_file_name, sizeof(run->log_file_name));
  run->qsos = g_array_new(FALSE, FALSE, sizeof(upload_qso_t));

  bool written = lotw_write_adif(run, qsos);
  g_array_free(qsos, TRUE);

  if (!written || run->qsos->len == 0) {
    lotw_run_free(run);
    return;
  }

  const char *argv[] = {
    llog->tqsl_path, "-x", "-d", "-u", "-a", "compliant",
    "-l", llog->tqsl_station_location, run->adif_path, NULL
  };

  run->process = g_subprocess_newv(argv, G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_MERGE, &error);
  if (run->process == NULL) {
    text = g_strdup_printf("Could not run TQSL: %s\nInstall TQSL (Debian: trustedqsl) or set its path above.",
                           error->message);
    upload_append_text(w->text_view, text);
    g_free(text);
    g_clear_error(&error);
    lotw_run_free(run);
    return;
  }

  text = g_strdup_printf("Signing and uploading %u QSOs with TQSL (%u already uploaded are skipped)...",
                         run->qsos->len, already_uploaded);
  upload_append_text(w->text_view, text);
  g_free(text);

  running = run;
  gtk_widget_set_sensitive(w->button_upload, FALSE);
  gtk_widget_set_sensitive(w->button_stop, TRUE);

  g_subprocess_communicate_utf8_async(run->process, NULL, NULL, on_tqsl_finished, run);
}


static void on_button_stop_clicked(GtkWidget *widget, gpointer data) {
  (void)widget;
  lotw_widgets_t *w = (lotw_widgets_t *)data;

  if (running != NULL) {
    g_subprocess_force_exit(running->process);
    gtk_widget_set_sensitive(w->button_stop, FALSE);
  }
}


static void on_button_close_clicked(GtkWidget *widget, gpointer data) {
  (void)widget;

  lotw_widgets_t *w = (lotw_widgets_t *)data;
  gtk_window_close(GTK_WINDOW(w->window));
}


static void on_lotw_window_destroy(GtkWidget *widget, gpointer data) {
  (void)widget;

  /*Let a running TQSL finish, so its result is still recorded in the log*/
  if (running != NULL) {
    running->widgets = NULL;
  }

  widgets = NULL;
  g_free(data);
}
