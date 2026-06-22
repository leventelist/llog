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
typedef enum {
  xml_client_stat_ok = 0,
  xml_client_stat_err
} xml_client_error_t;

enum xml_client_result_type {
  xml_client_result_type_double = 0,
  xml_client_result_type_int,
  xml_client_result_type_string
};

typedef struct {
  uint64_t result_type;
  union {
    double result_d;
    int64_t result_i;
    char *result_s;
  } result;
} xml_client_result_t;


int xml_client_init(const char *host, u_int64_t port);
int xml_client_fldigi_get_frequency(double *frequency);
int xml_client_fldigi_get_call(char *call);
int xml_client_fldigi_get_name(char *name);
int xml_client_fldigi_get_rxrst(char *rxrst);
int xml_client_fldigi_get_txrst(char *txrst);
int xml_client_fldigi_get_qth(char *qth);
int xml_client_fldigi_get_qra(char *qra);
int xml_client_fldigi_get_utc(char *utc);
void xml_client_shutdown(void);
