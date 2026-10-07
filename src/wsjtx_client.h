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

#pragma once

#include <stdint.h>
#include <time.h>

#include "llog.h"

/*WSJT-X sends its UDP messages here by default*/
#define WSJTX_DEFAULT_ADDR "127.0.0.1"
#define WSJTX_DEFAULT_PORT 2237

typedef enum {
  wsjtx_client_stat_ok = 0,
  wsjtx_client_stat_err
} wsjtx_client_error_t;

/*Status message (type 1): what WSJT-X is doing right now*/
typedef struct {
  double qrg;                 /*Dial frequency in MHz*/
  char mode[MODE_LEN];
  char sub_mode[MODE_LEN];
  char dx_call[CALL_LEN];
  char dx_grid[QRA_LEN];
  char report[RST_LEN];       /*Report to send*/
} wsjtx_status_t;

/*QSO Logged message (type 5)*/
typedef struct {
  time_t time_on;
  time_t time_off;
  double qrg;                 /*Tx frequency in MHz*/
  char dx_call[CALL_LEN];
  char dx_grid[QRA_LEN];
  char mode[MODE_LEN];
  char report_sent[RST_LEN];
  char report_rcvd[RST_LEN];
  char power[PWR_LEN];
  char comment[COMMENT_LEN];
  char name[NAME_LEN];
  char exchange_sent[X_LEN];
  char exchange_rcvd[X_LEN];
} wsjtx_qso_t;

typedef void (*wsjtx_status_cb_t)(const wsjtx_status_t *status);
typedef void (*wsjtx_qso_cb_t)(const wsjtx_qso_t *qso);

/*Listens for WSJT-X on the main loop. A multicast address joins that group.*/
int wsjtx_client_init(const char *addr, uint64_t port, wsjtx_status_cb_t status_cb, wsjtx_qso_cb_t qso_cb);
void wsjtx_client_shutdown(void);
