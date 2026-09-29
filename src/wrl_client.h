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

/* Client for the World Radio League API.
 * https://worldradioleague.com/developer/
 *
 * The functions here do not touch GTK or the log database, so they can be
 * called from a worker thread.
 */

#ifndef WRL_CLIENT_H
#define WRL_CLIENT_H

#include "llog.h"

#define WRL_SERVICE "WRL"
#define WRL_ID_LEN 64
#define WRL_CODE_LEN 64
#define WRL_MSG_LEN 2048

typedef enum {
  wrl_stat_ok = 0,
  wrl_stat_rejected,     /*The server refused this contact; the next one may still succeed*/
  wrl_stat_rate_limited, /*Wait retry_after seconds, then try again*/
  wrl_stat_fatal,        /*Bad key, no logbook, network error... stop uploading*/
  wrl_stat_skipped       /*Not sent: the QSO lacks data WRL requires*/
} wrl_status_t;

typedef struct {
  long http_status;
  char id[WRL_ID_LEN];         /*ID of the created contact*/
  char code[WRL_CODE_LEN];     /*error.code, e.g. VALIDATION_ERROR*/
  char field[WRL_CODE_LEN];    /*error.field, the request field the error is about*/
  char message[WRL_MSG_LEN];   /*Human readable error, or warnings on success*/
  long retry_after;            /*Seconds, set on wrl_stat_rate_limited*/
} wrl_result_t;

void wrl_client_init(void);
wrl_status_t wrl_check_key(const char *api_key, wrl_result_t *result);
wrl_status_t wrl_post_contact(const char *api_key, const char *logbook_id,
                              const log_entry_t *entry, const char *mode, const station_entry_t *station,
                              wrl_result_t *result);

#endif
