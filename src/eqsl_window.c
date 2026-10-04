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

/* Upload window for eQSL.cc.
 *
 * Every QSO is uploaded in its own request, so each one gets its own result. The requests run in
 * a worker thread, which never touches GTK or the log database: it posts every result to the main
 * thread with g_idle_add(), and the main thread records uploaded QSOs and updates the window.
 */

#include <gtk/gtk.h>
#include <string.h>
#include "llog.h"
#include "db_sqlite.h"
#include "eqsl_client.h"
#include "upload.h"
#include "eqsl_window.h"

typedef struct eqsl_ctx eqsl_ctx_t;

typedef struct {
  GtkWidget *window;
  GtkWidget *user_entry;
  GtkWidget *password_entry;
  GtkWidget *nickname_entry;
  GtkWidget *from_date_entry;
  GtkWidget *progress_bar;
  GtkWidget *text_view;
  GtkWidget *button_upload;
  GtkWidget *button_stop;
  GtkWidget *button_close;
  eqsl_ctx_t *running;   /*Worker in progress, or NULL*/
  llog_t *llog;
} eqsl_widgets_t;

/*Shared between the window and the worker. Reference counted with g_atomic_rc_box.*/
struct eqsl_ctx {
  eqsl_widgets_t *widgets;         /*NULL once the window is gone; only used on the main thread*/
  llog_t *llog;
  char log_file_name[FILE_LEN];    /*The log the QSOs came from*/
  char user[CALL_LEN];
  char password[API_KEY_LEN];
  char qth_nickname[STATION_LEN];
  GArray *qsos;                    /*upload_qso_t, newest first*/
  gint cancel;
  guint n_ok;
  guint n_duplicate;
  guint n_failed;
};

typedef enum {
  eqsl_msg_log,
  eqsl_msg_uploaded,   /*Also sent for duplicates: eQSL.cc has the QSO either way*/
  eqsl_msg_failed,
  eqsl_msg_done
} eqsl_msg_kind_t;

typedef struct {
  eqsl_ctx_t *ctx;
  eqsl_msg_kind_t kind;
  guint done;              /*QSOs processed so far*/
  uint64_t log_id;
  char *text;
} eqsl_msg_t;

static eqsl_widgets_t *widgets = NULL;
static guint active_workers = 0;   /*Includes workers of a closed window that are still stopping*/

static void on_eqsl_window_destroy(GtkWidget *widget, gpointer data);
static void on_button_upload_clicked(GtkWidget *widget, gpointer data);
static void on_button_stop_clicked(GtkWidget *widget, gpointer data);
static void on_button_close_clicked(GtkWidget *widget, gpointer data);
static gpointer eqsl_worker(gpointer data);


void on_upload_eqsl_window_activate(GtkWidget *widget, gpointer data) {
  (void)widget;

  if (widgets != NULL) {
    gtk_window_present(GTK_WINDOW(widgets->window));
    return;
  }

  eqsl_client_init();

  widgets = g_malloc0(sizeof(eqsl_widgets_t));
  widgets->llog = (llog_t *)data;

  widgets->window = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(widgets->window), "Upload to eQSL.cc");
  gtk_window_set_default_size(GTK_WINDOW(widgets->window), 700, 450);
  g_signal_connect(widgets->window, "destroy", G_CALLBACK(on_eqsl_window_destroy), widgets);

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

  widgets->user_entry = upload_add_row(grid, 0, "User:", gtk_entry_new());
  gtk_entry_set_placeholder_text(GTK_ENTRY(widgets->user_entry), "Your eQSL.cc user name (callsign)");
  gtk_editable_set_text(GTK_EDITABLE(widgets->user_entry), widgets->llog->eqsl_user);

  widgets->password_entry = upload_add_row(grid, 1, "Password:", gtk_password_entry_new());
  gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(widgets->password_entry), TRUE);
  gtk_editable_set_text(GTK_EDITABLE(widgets->password_entry), widgets->llog->eqsl_password);

  widgets->nickname_entry = upload_add_row(grid, 2, "QTH nickname:", gtk_entry_new());
  gtk_entry_set_placeholder_text(GTK_ENTRY(widgets->nickname_entry), "Optional, if your account has several QTHs");
  gtk_editable_set_text(GTK_EDITABLE(widgets->nickname_entry), widgets->llog->eqsl_qth_nickname);

  widgets->from_date_entry = upload_add_row(grid, 3, "From date:", gtk_entry_new());
  gtk_entry_set_placeholder_text(GTK_ENTRY(widgets->from_date_entry), "YYYY-MM-DD, empty: all QSOs");

  GtkWidget *hint = gtk_label_new("The password is your eQSL.cc login password.\n"
                                  "QSOs already uploaded from this log are not sent again.");
  gtk_widget_set_halign(hint, GTK_ALIGN_START);
  gtk_label_set_wrap(GTK_LABEL(hint), TRUE);
  gtk_grid_attach(GTK_GRID(grid), hint, 0, 4, 2, 1);

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

  widgets->button_upload = gtk_button_new_with_label("Upload");
  widgets->button_stop = gtk_button_new_with_label("Stop");
  widgets->button_close = gtk_button_new_with_label("Close");
  gtk_box_append(GTK_BOX(button_box), widgets->button_upload);
  gtk_box_append(GTK_BOX(button_box), widgets->button_stop);
  gtk_box_append(GTK_BOX(button_box), widgets->button_close);
  gtk_widget_set_sensitive(widgets->button_stop, FALSE);

  g_signal_connect(widgets->button_upload, "clicked", G_CALLBACK(on_button_upload_clicked), widgets);
  g_signal_connect(widgets->button_stop, "clicked", G_CALLBACK(on_button_stop_clicked), widgets);
  g_signal_connect(widgets->button_close, "clicked", G_CALLBACK(on_button_close_clicked), widgets);

  gtk_window_present(GTK_WINDOW(widgets->window));
}


static void eqsl_set_running(eqsl_widgets_t *w, eqsl_ctx_t *ctx) {
  w->running = ctx;
  gtk_widget_set_sensitive(w->button_upload, ctx == NULL);
  gtk_widget_set_sensitive(w->button_stop, ctx != NULL);
}


static void eqsl_ctx_free(gpointer data) {
  eqsl_ctx_t *ctx = (eqsl_ctx_t *)data;

  if (ctx->qsos != NULL) {
    g_array_free(ctx->qsos, TRUE);
  }
}


/*Main thread: handles one message from the worker*/
static gboolean eqsl_msg_cb(gpointer data) {
  eqsl_msg_t *msg = (eqsl_msg_t *)data;
  eqsl_ctx_t *ctx = msg->ctx;
  eqsl_widgets_t *w = ctx->widgets;

  /*Record the upload even if the window has been closed, but only in the log it came from*/
  if (msg->kind == eqsl_msg_uploaded) {
    if (strcmp(ctx->llog->log_file_name, ctx->log_file_name) != 0 ||
        db_set_uploaded(ctx->llog, msg->log_id, EQSL_SERVICE, NULL) != llog_stat_ok) {
      char *text = g_strdup_printf("%s\n  WARNING: could not record the upload in the log; "
                                   "this QSO may be uploaded again.", msg->text);
      g_free(msg->text);
      msg->text = text;
    }
  }

  if (msg->kind == eqsl_msg_done) {
    active_workers--;
  }

  if (w != NULL) {
    if (msg->text != NULL) {
      upload_append_text(w->text_view, msg->text);
    }

    if (msg->kind == eqsl_msg_uploaded || msg->kind == eqsl_msg_failed) {
      char progress[64];
      snprintf(progress, sizeof(progress), "%u / %u", msg->done, ctx->qsos->len);
      gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(w->progress_bar), (double)msg->done / ctx->qsos->len);
      gtk_progress_bar_set_text(GTK_PROGRESS_BAR(w->progress_bar), progress);
    }

    if (msg->kind == eqsl_msg_done) {
      eqsl_set_running(w, NULL);
    }
  }

  g_free(msg->text);
  g_atomic_rc_box_release_full(ctx, eqsl_ctx_free);
  g_free(msg);

  return G_SOURCE_REMOVE;
}


/*Worker thread: queues a message for the main thread*/
static void eqsl_post(eqsl_ctx_t *ctx, eqsl_msg_kind_t kind, guint done, uint64_t log_id, char *text) {
  eqsl_msg_t *msg = g_malloc0(sizeof(eqsl_msg_t));

  msg->ctx = g_atomic_rc_box_acquire(ctx);
  msg->kind = kind;
  msg->done = done;
  msg->log_id = log_id;
  msg->text = text;   /*Takes ownership*/

  g_idle_add(eqsl_msg_cb, msg);
}


static gpointer eqsl_worker(gpointer data) {
  eqsl_ctx_t *ctx = (eqsl_ctx_t *)data;
  eqsl_result_t result;
  eqsl_status_t status;
  guint done = 0;

  /*The array is newest first; upload in the order the QSOs were made*/
  for (guint i = ctx->qsos->len; i > 0; i--) {
    upload_qso_t *qso = &g_array_index(ctx->qsos, upload_qso_t, i - 1);
    log_entry_t *e = &qso->entry;

    if (g_atomic_int_get(&ctx->cancel)) {
      break;
    }

    status = eqsl_upload_qso(ctx->user, ctx->password, ctx->qth_nickname, e, qso->super_mode, &result);
    done++;

    switch (status) {
    case eqsl_stat_ok:
      ctx->n_ok++;
      eqsl_post(ctx, eqsl_msg_uploaded, done, e->id,
                g_strdup_printf("OK      %s %s %-12s %s", e->date, e->utc, e->call, result.message));
      break;

    case eqsl_stat_duplicate:
      ctx->n_duplicate++;
      eqsl_post(ctx, eqsl_msg_uploaded, done, e->id,
                g_strdup_printf("DUP     %s %s %-12s already on eQSL.cc", e->date, e->utc, e->call));
      break;

    case eqsl_stat_skipped:
    case eqsl_stat_rejected:
      ctx->n_failed++;
      eqsl_post(ctx, eqsl_msg_failed, done, e->id,
                g_strdup_printf("FAILED  %s %s %-12s %s", e->date, e->utc, e->call, result.message));
      break;

    default:   /*eqsl_stat_fatal*/
      ctx->n_failed++;
      eqsl_post(ctx, eqsl_msg_failed, done, e->id,
                g_strdup_printf("FAILED  %s %s %-12s %s\nUpload stopped.", e->date, e->utc, e->call,
                                result.message));
      g_atomic_int_set(&ctx->cancel, 1);
      break;
    }
  }

  eqsl_post(ctx, eqsl_msg_log, done, 0,
            g_strdup_printf("%s: %u uploaded, %u already on eQSL.cc, %u failed, %u not attempted.",
                            done < ctx->qsos->len ? "Stopped" : "Finished",
                            ctx->n_ok, ctx->n_duplicate, ctx->n_failed, ctx->qsos->len - done));
  eqsl_post(ctx, eqsl_msg_done, done, 0, NULL);
  g_atomic_rc_box_release_full(ctx, eqsl_ctx_free);

  return NULL;
}


static void on_button_upload_clicked(GtkWidget *widget, gpointer data) {
  (void)widget;
  eqsl_widgets_t *w = (eqsl_widgets_t *)data;
  llog_t *llog = w->llog;
  char from_date[UPLOAD_DATE_LEN];
  guint already_uploaded = 0;
  char *text;

  char *user = g_strstrip(g_strdup(gtk_editable_get_text(GTK_EDITABLE(w->user_entry))));
  char *nickname = g_strstrip(g_strdup(gtk_editable_get_text(GTK_EDITABLE(w->nickname_entry))));
  g_strlcpy(llog->eqsl_user, user, sizeof(llog->eqsl_user));
  g_strlcpy(llog->eqsl_password, gtk_editable_get_text(GTK_EDITABLE(w->password_entry)),
            sizeof(llog->eqsl_password));
  g_strlcpy(llog->eqsl_qth_nickname, nickname, sizeof(llog->eqsl_qth_nickname));
  g_free(user);
  g_free(nickname);
  llog_save_config_file();

  if (llog->eqsl_user[0] == '\0' || llog->eqsl_password[0] == '\0') {
    upload_append_text(w->text_view, "Enter your eQSL.cc user name and password.");
    return;
  }

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

  if (llog->log_db == NULL) {
    upload_append_text(w->text_view, "No log file is open.");
    return;
  }

  eqsl_ctx_t *ctx = g_atomic_rc_box_new0(eqsl_ctx_t);
  ctx->widgets = w;
  ctx->llog = llog;
  g_strlcpy(ctx->log_file_name, llog->log_file_name, sizeof(ctx->log_file_name));
  g_strlcpy(ctx->user, llog->eqsl_user, sizeof(ctx->user));
  g_strlcpy(ctx->password, llog->eqsl_password, sizeof(ctx->password));
  g_strlcpy(ctx->qth_nickname, llog->eqsl_qth_nickname, sizeof(ctx->qth_nickname));
  ctx->qsos = g_array_new(FALSE, FALSE, sizeof(upload_qso_t));

  /*Collect the QSOs here: the database is only used from the main thread*/
  if (upload_collect_qsos(llog, EQSL_SERVICE, from_date, ctx->qsos, &already_uploaded) != llog_stat_ok) {
    upload_append_text(w->text_view, "Error reading the log.");
    g_atomic_rc_box_release_full(ctx, eqsl_ctx_free);
    return;
  }

  if (ctx->qsos->len == 0) {
    text = g_strdup_printf("Nothing to upload (%u QSOs already uploaded).", already_uploaded);
    upload_append_text(w->text_view, text);
    g_free(text);
    g_atomic_rc_box_release_full(ctx, eqsl_ctx_free);
    return;
  }

  text = g_strdup_printf("Uploading %u QSOs (%u already uploaded are skipped)...",
                         ctx->qsos->len, already_uploaded);
  upload_append_text(w->text_view, text);
  g_free(text);

  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(w->progress_bar), 0.0);
  active_workers++;
  eqsl_set_running(w, ctx);
  /*The worker owns the initial reference*/
  g_thread_unref(g_thread_new("eqsl-upload", eqsl_worker, ctx));
}


static void on_button_stop_clicked(GtkWidget *widget, gpointer data) {
  (void)widget;
  eqsl_widgets_t *w = (eqsl_widgets_t *)data;

  if (w->running != NULL) {
    g_atomic_int_set(&w->running->cancel, 1);
    gtk_widget_set_sensitive(w->button_stop, FALSE);
    upload_append_text(w->text_view, "Stopping...");
  }
}


static void on_button_close_clicked(GtkWidget *widget, gpointer data) {
  (void)widget;

  eqsl_widgets_t *w = (eqsl_widgets_t *)data;
  gtk_window_close(GTK_WINDOW(w->window));
}


static void on_eqsl_window_destroy(GtkWidget *widget, gpointer data) {
  (void)widget;
  eqsl_widgets_t *w = (eqsl_widgets_t *)data;

  /*A running worker is stopped; the QSOs it already uploaded are still recorded*/
  if (w->running != NULL) {
    g_atomic_int_set(&w->running->cancel, 1);
    w->running->widgets = NULL;
  }

  widgets = NULL;
  g_free(w);
}
