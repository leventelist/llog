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

/* Client for the eQSL.cc ADIF upload interface.
 * https://www.eqsl.cc/qslcard/ImportADIF.txt
 *
 * The functions here do not touch GTK or the log database, so they can be
 * called from a worker thread.
 */

#ifndef EQSL_CLIENT_H
#define EQSL_CLIENT_H

#include "llog.h"

#define EQSL_SERVICE "EQSL"
#define EQSL_MSG_LEN 2048

typedef enum {
  eqsl_stat_ok = 0,
  eqsl_stat_duplicate,   /*eQSL.cc already has this QSO*/
  eqsl_stat_rejected,    /*eQSL.cc refused this QSO; the next one may still succeed*/
  eqsl_stat_fatal,       /*Bad user or password, network error, maintenance... stop uploading*/
  eqsl_stat_skipped      /*Not sent: the QSO lacks data eQSL.cc requires*/
} eqsl_status_t;

typedef struct {
  long http_status;
  char message[EQSL_MSG_LEN];   /*Errors and warnings from eQSL.cc, or notes on success*/
} eqsl_result_t;

void eqsl_client_init(void);
eqsl_status_t eqsl_upload_qso(const char *user, const char *password, const char *qth_nickname,
                              const log_entry_t *entry, const char *super_mode, eqsl_result_t *result);

#endif
