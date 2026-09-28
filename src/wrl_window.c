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

/* Upload window for World Radio League.
 *
 * The HTTP requests run in a worker thread. The worker never touches GTK or the log database:
 * it posts every result to the main thread with g_idle_add(), and the main thread records
 * uploaded QSOs in the `upload` table and updates the window.
 */

#include <gtk/gtk.h>
#include <string.h>
#include "llog.h"
#include "db_sqlite.h"
#include "wrl_client.h"
#include "upload.h"
#include "wrl_window.h"

#define WRL_MIN_INTERVAL_US G_USEC_PER_SEC   /*The API allows 60 writes per minute*/
#define WRL_MAX_WAIT_S 300                   /*Longer waits mean the daily quota is gone*/
#define WRL_MAX_RETRIES 3

typedef struct upload_ctx upload_ctx_t;

typedef struct {
  GtkWidget *window;
  GtkWidget *api_key_entry;
  GtkWidget *logbook_entry;
  GtkWidget *from_date_entry;
  GtkWidget *progress_bar;
  GtkWidget *text_view;
  GtkWidget *button_check;
  GtkWidget *button_upload;
  GtkWidget *button_stop;
  GtkWidget *button_close;
  upload_ctx_t *running;   /*Worker in progress, or NULL*/
  llog_t *llog;
} upload_widgets_t;

typedef enum {
  upload_job_check,
  upload_job_upload
} upload_job_t;

/*Shared between the window and the worker. Reference counted with g_atomic_rc_box.*/
struct upload_ctx {
  upload_job_t job;
  upload_widgets_t *widgets;       /*NULL once the window is gone; only used on the main thread*/
  llog_t *llog;
  char log_file_name[FILE_LEN];    /*The log the QSOs came from*/
  char api_key[API_KEY_LEN];
  char logbook_id[LOGBOOK_ID_LEN];
  GArray *qsos;                    /*upload_qso_t, newest first*/
  gint cancel;
  guint n_ok;
  guint n_failed;
};

typedef enum {
  upload_msg_log,
  upload_msg_uploaded,
  upload_msg_failed,
  upload_msg_done
} upload_msg_kind_t;

typedef struct {
  upload_ctx_t *ctx;
  upload_msg_kind_t kind;
  guint done;              /*QSOs processed so far*/
  uint64_t log_id;
  char remote_id[WRL_ID_LEN];
  char *text;
} upload_msg_t;

static upload_widgets_t *widgets = NULL;
static guint active_workers = 0;   /*Includes workers of a closed window that are still stopping*/

static void on_upload_window_destroy(GtkWidget *widget, gpointer data);
static void on_button_check_clicked(GtkWidget *widget, gpointer data);
static void on_button_upload_clicked(GtkWidget *widget, gpointer data);
static void on_button_stop_clicked(GtkWidget *widget, gpointer data);
static void on_button_close_clicked(GtkWidget *widget, gpointer data);
static gpointer upload_worker(gpointer data);



void on_upload_wrl_window_activate(GtkWidget *widget, gpointer data) {
  (void)widget;

  if (widgets != NULL) {
    gtk_window_present(GTK_WINDOW(widgets->window));
    return;
  }

  wrl_client_init();

  widgets = g_malloc0(sizeof(upload_widgets_t));
  widgets->llog = (llog_t *)data;

  widgets->window = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(widgets->window), "Upload to World Radio League");
  gtk_window_set_default_size(GTK_WINDOW(widgets->window), 700, 450);
  g_signal_connect(widgets->window, "destroy", G_CALLBACK(on_upload_window_destroy), widgets);

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

  widgets->api_key_entry = upload_add_row(grid, 0, "API key:", gtk_password_entry_new());
  gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(widgets->api_key_entry), TRUE);
  gtk_editable_set_text(GTK_EDITABLE(widgets->api_key_entry), widgets->llog->wrl_api_key);

  widgets->logbook_entry = upload_add_row(grid, 1, "Logbook ID:", gtk_entry_new());
  gtk_entry_set_placeholder_text(GTK_ENTRY(widgets->logbook_entry), "Empty: your default logbook");
  gtk_editable_set_text(GTK_EDITABLE(widgets->logbook_entry), widgets->llog->wrl_logbook_id);

  widgets->from_date_entry = upload_add_row(grid, 2, "From date:", gtk_entry_new());
  gtk_entry_set_placeholder_text(GTK_ENTRY(widgets->from_date_entry), "YYYY-MM-DD, empty: all QSOs");

  GtkWidget *hint = gtk_label_new("Generate the API key in World Radio League under Integrations → Developer API.\n"
                                  "QSOs already uploaded from this log are not sent again.");
  gtk_widget_set_halign(hint, GTK_ALIGN_START);
  gtk_label_set_wrap(GTK_LABEL(hint), TRUE);
  gtk_grid_attach(GTK_GRID(grid), hint, 0, 3, 2, 1);

  widgets->progress_bar = gtk_progress_bar_new();
  gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(widgets->progress_bar), TRUE);
  gtk_progress_bar_set_text(GTK_PROGRESS_BAR(widgets->progress_bar), "Idle");
  gtk_box_append(GTK_BOX(box), widgets->progress_bar);

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

  widgets->button_check = gtk_button_new_with_label("Check key");
  widgets->button_upload = gtk_button_new_with_label("Upload");
  widgets->button_stop = gtk_button_new_with_label("Stop");
  widgets->button_close = gtk_button_new_with_label("Close");
  gtk_box_append(GTK_BOX(button_box), widgets->button_check);
  gtk_box_append(GTK_BOX(button_box), widgets->button_upload);
  gtk_box_append(GTK_BOX(button_box), widgets->button_stop);
  gtk_box_append(GTK_BOX(button_box), widgets->button_close);
  gtk_widget_set_sensitive(widgets->button_stop, FALSE);

  g_signal_connect(widgets->button_check, "clicked", G_CALLBACK(on_button_check_clicked), widgets);
  g_signal_connect(widgets->button_upload, "clicked", G_CALLBACK(on_button_upload_clicked), widgets);
  g_signal_connect(widgets->button_stop, "clicked", G_CALLBACK(on_button_stop_clicked), widgets);
  g_signal_connect(widgets->button_close, "clicked", G_CALLBACK(on_button_close_clicked), widgets);

  gtk_window_present(GTK_WINDOW(widgets->window));
}



static void upload_set_running(upload_widgets_t *w, upload_ctx_t *ctx) {
  w->running = ctx;
  gtk_widget_set_sensitive(w->button_check, ctx == NULL);
  gtk_widget_set_sensitive(w->button_upload, ctx == NULL);
  gtk_widget_set_sensitive(w->button_stop, ctx != NULL);
}


static void upload_ctx_free(gpointer data) {
  upload_ctx_t *ctx = (upload_ctx_t *)data;

  if (ctx->qsos != NULL) {
    g_array_free(ctx->qsos, TRUE);
  }
}


/*Main thread: handles one message from the worker*/
static gboolean upload_msg_cb(gpointer data) {
  upload_msg_t *msg = (upload_msg_t *)data;
  upload_ctx_t *ctx = msg->ctx;
  upload_widgets_t *w = ctx->widgets;

  /*Record the upload even if the window has been closed, but only in the log it came from*/
  if (msg->kind == upload_msg_uploaded) {
    if (strcmp(ctx->llog->log_file_name, ctx->log_file_name) != 0 ||
        db_set_uploaded(ctx->llog, msg->log_id, WRL_SERVICE, msg->remote_id) != llog_stat_ok) {
      char *text = g_strdup_printf("%s\n  WARNING: could not record the upload in the log; "
                                   "this QSO may be uploaded again.", msg->text);
      g_free(msg->text);
      msg->text = text;
    }
  }

  if (msg->kind == upload_msg_done) {
    active_workers--;
  }

  if (w != NULL) {
    if (msg->text != NULL) {
      upload_append_text(w->text_view, msg->text);
    }

    if (msg->kind == upload_msg_uploaded || msg->kind == upload_msg_failed) {
      char progress[64];
      snprintf(progress, sizeof(progress), "%u / %u", msg->done, ctx->qsos->len);
      gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(w->progress_bar), (double)msg->done / ctx->qsos->len);
      gtk_progress_bar_set_text(GTK_PROGRESS_BAR(w->progress_bar), progress);
    }

    if (msg->kind == upload_msg_done) {
      upload_set_running(w, NULL);
    }
  }

  g_free(msg->text);
  g_atomic_rc_box_release_full(ctx, upload_ctx_free);
  g_free(msg);

  return G_SOURCE_REMOVE;
}


/*Worker thread: queues a message for the main thread*/
static void upload_post(upload_ctx_t *ctx, upload_msg_kind_t kind, guint done,
                        uint64_t log_id, const char *remote_id, char *text) {
  upload_msg_t *msg = g_malloc0(sizeof(upload_msg_t));

  msg->ctx = g_atomic_rc_box_acquire(ctx);
  msg->kind = kind;
  msg->done = done;
  msg->log_id = log_id;
  if (remote_id != NULL) {
    g_strlcpy(msg->remote_id, remote_id, sizeof(msg->remote_id));
  }
  msg->text = text;   /*Takes ownership*/

  g_idle_add(upload_msg_cb, msg);
}


/*Worker thread: sleeps, waking up early if the user pressed Stop. Returns false if cancelled.*/
static bool upload_sleep_until(upload_ctx_t *ctx, gint64 deadline) {
  while (g_get_monotonic_time() < deadline) {
    if (g_atomic_int_get(&ctx->cancel)) {
      return false;
    }
    g_usleep(100 * 1000);
  }

  return !g_atomic_int_get(&ctx->cancel);
}


static void upload_run_check(upload_ctx_t *ctx) {
  wrl_result_t result;
  wrl_status_t status;

  status = wrl_check_key(ctx->api_key, &result);
  upload_post(ctx, upload_msg_log, 0, 0, NULL,
              status == wrl_stat_ok ? g_strdup(result.message) :
              g_strdup_printf("Key check failed: %s", result.message));
}


static void upload_run_upload(upload_ctx_t *ctx) {
  wrl_result_t result;
  wrl_status_t status;
  gint64 next_request = 0;
  guint done = 0;
  bool stop = false;

  /*The array is newest first; upload in the order the QSOs were made*/
  for (guint i = ctx->qsos->len; i > 0 && !stop; i--) {
    upload_qso_t *qso = &g_array_index(ctx->qsos, upload_qso_t, i - 1);
    log_entry_t *e = &qso->entry;
    int retries = 0;

    for (;;) {
      if (!upload_sleep_until(ctx, next_request)) {
        stop = true;
        break;
      }
      next_request = g_get_monotonic_time() + WRL_MIN_INTERVAL_US;

      status = wrl_post_contact(ctx->api_key, ctx->logbook_id, e, &qso->station, &result);

      if (status != wrl_stat_rate_limited) {
        break;
      }

      if (result.retry_after > WRL_MAX_WAIT_S || ++retries > WRL_MAX_RETRIES) {
        upload_post(ctx, upload_msg_log, done, 0, NULL,
                    g_strdup_printf("Rate limit reached: %s Try again later.", result.message));
        stop = true;
        break;
      }

      upload_post(ctx, upload_msg_log, done, 0, NULL,
                  g_strdup_printf("Rate limited, waiting %ld s...", result.retry_after));
      next_request = g_get_monotonic_time() + result.retry_after * G_USEC_PER_SEC;
    }

    if (stop) {
      break;
    }

    done++;

    switch (status) {
    case wrl_stat_ok:
      ctx->n_ok++;
      upload_post(ctx, upload_msg_uploaded, done, e->id, result.id,
                  g_strdup_printf("OK      %s %s %-12s %s%s", e->date, e->utc, e->call,
                                  result.message[0] != '\0' ? "  " : "", result.message));
      break;

    case wrl_stat_skipped:
    case wrl_stat_rejected:
      ctx->n_failed++;
      upload_post(ctx, upload_msg_failed, done, e->id, NULL,
                  g_strdup_printf("FAILED  %s %s %-12s %s%s%s", e->date, e->utc, e->call,
                                  result.code, result.code[0] != '\0' ? ": " : "", result.message));
      break;

    default:   /*wrl_stat_fatal*/
      ctx->n_failed++;
      upload_post(ctx, upload_msg_failed, done, e->id, NULL,
                  g_strdup_printf("FAILED  %s %s %-12s %s%s%s\nUpload stopped.", e->date, e->utc, e->call,
                                  result.code, result.code[0] != '\0' ? ": " : "", result.message));
      stop = true;
      break;
    }
  }

  upload_post(ctx, upload_msg_log, done, 0, NULL,
              g_strdup_printf("%s: %u uploaded, %u failed, %u not attempted.",
                              g_atomic_int_get(&ctx->cancel) ? "Stopped" : "Finished",
                              ctx->n_ok, ctx->n_failed, ctx->qsos->len - done));
}


static gpointer upload_worker(gpointer data) {
  upload_ctx_t *ctx = (upload_ctx_t *)data;

  if (ctx->job == upload_job_check) {
    upload_run_check(ctx);
  } else {
    upload_run_upload(ctx);
  }

  upload_post(ctx, upload_msg_done, ctx->qsos->len, 0, NULL, NULL);
  g_atomic_rc_box_release_full(ctx, upload_ctx_free);

  return NULL;
}


/*Main thread: saves the settings and prepares a worker context. Returns NULL if there's no key.*/
static upload_ctx_t *upload_ctx_new(upload_widgets_t *w, upload_job_t job) {
  llog_t *llog = w->llog;
  char *key = g_strstrip(g_strdup(gtk_editable_get_text(GTK_EDITABLE(w->api_key_entry))));
  char *logbook = g_strstrip(g_strdup(gtk_editable_get_text(GTK_EDITABLE(w->logbook_entry))));

  g_strlcpy(llog->wrl_api_key, key, sizeof(llog->wrl_api_key));
  g_strlcpy(llog->wrl_logbook_id, logbook, sizeof(llog->wrl_logbook_id));
  g_free(key);
  g_free(logbook);
  llog_save_config_file();

  if (llog->wrl_api_key[0] == '\0') {
    upload_append_text(w->text_view, "Enter your World Radio League API key.");
    return NULL;
  }

  upload_ctx_t *ctx = g_atomic_rc_box_new0(upload_ctx_t);
  ctx->job = job;
  ctx->widgets = w;
  ctx->llog = llog;
  g_strlcpy(ctx->log_file_name, llog->log_file_name, sizeof(ctx->log_file_name));
  g_strlcpy(ctx->api_key, llog->wrl_api_key, sizeof(ctx->api_key));
  g_strlcpy(ctx->logbook_id, llog->wrl_logbook_id, sizeof(ctx->logbook_id));
  ctx->qsos = g_array_new(FALSE, FALSE, sizeof(upload_qso_t));

  return ctx;
}


static void upload_start(upload_widgets_t *w, upload_ctx_t *ctx) {
  active_workers++;
  upload_set_running(w, ctx);
  /*The worker owns the initial reference*/
  g_thread_unref(g_thread_new("wrl-upload", upload_worker, ctx));
}


static void on_button_check_clicked(GtkWidget *widget, gpointer data) {
  (void)widget;
  upload_widgets_t *w = (upload_widgets_t *)data;

  upload_ctx_t *ctx = upload_ctx_new(w, upload_job_check);
  if (ctx == NULL) {
    return;
  }

  upload_append_text(w->text_view, "Checking API key...");
  upload_start(w, ctx);
}


static void on_button_upload_clicked(GtkWidget *widget, gpointer data) {
  (void)widget;
  upload_widgets_t *w = (upload_widgets_t *)data;
  char from_date[UPLOAD_DATE_LEN];
  guint already_uploaded = 0;

  if (!upload_parse_from_date(gtk_editable_get_text(GTK_EDITABLE(w->from_date_entry)),
                              from_date, sizeof(from_date))) {
    upload_append_text(w->text_view, "From date must be YYYY-MM-DD, or empty for all QSOs.");
    return;
  }

  if (active_workers > 0) {
    /*A stopped upload may still have a request in flight; collecting now could send that QSO twice*/
    upload_append_text(w->text_view, "The previous upload is still stopping, try again in a moment.");
    return;
  }

  if (w->llog->log_db == NULL) {
    upload_append_text(w->text_view, "No log file is open.");
    return;
  }

  upload_ctx_t *ctx = upload_ctx_new(w, upload_job_upload);
  if (ctx == NULL) {
    return;
  }

  /*Collect the QSOs here: the database is only used from the main thread*/
  if (upload_collect_qsos(w->llog, WRL_SERVICE, from_date, ctx->qsos, &already_uploaded) != llog_stat_ok) {
    upload_append_text(w->text_view, "Error reading the log.");
    g_atomic_rc_box_release_full(ctx, upload_ctx_free);
    return;
  }

  if (ctx->qsos->len == 0) {
    char *text = g_strdup_printf("Nothing to upload (%u QSOs already uploaded).", already_uploaded);
    upload_append_text(w->text_view, text);
    g_free(text);
    g_atomic_rc_box_release_full(ctx, upload_ctx_free);
    return;
  }

  char *text = g_strdup_printf("Uploading %u QSOs (%u already uploaded are skipped). "
                               "This takes about %u seconds.",
                               ctx->qsos->len, already_uploaded, ctx->qsos->len);
  upload_append_text(w->text_view, text);
  g_free(text);

  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(w->progress_bar), 0.0);
  upload_start(w, ctx);
}


static void on_button_stop_clicked(GtkWidget *widget, gpointer data) {
  (void)widget;
  upload_widgets_t *w = (upload_widgets_t *)data;

  if (w->running != NULL) {
    g_atomic_int_set(&w->running->cancel, 1);
    gtk_widget_set_sensitive(w->button_stop, FALSE);
    upload_append_text(w->text_view, "Stopping...");
  }
}


static void on_button_close_clicked(GtkWidget *widget, gpointer data) {
  (void)widget;

  upload_widgets_t *w = (upload_widgets_t *)data;
  gtk_window_close(GTK_WINDOW(w->window));
}


static void on_upload_window_destroy(GtkWidget *widget, gpointer data) {
  (void)widget;
  upload_widgets_t *w = (upload_widgets_t *)data;

  /*A running worker is stopped; the QSOs it already uploaded are still recorded*/
  if (w->running != NULL) {
    g_atomic_int_set(&w->running->cancel, 1);
    w->running->widgets = NULL;
  }

  widgets = NULL;
  g_free(w);
}
