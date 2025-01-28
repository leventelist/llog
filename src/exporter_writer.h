/*	This is llog, a minimalist HAM logging software.
 *	Copyright (C) 2013-2025  Levente Kovacs
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

#ifndef ADIF_WRITER_H
#define ADIF_WRITER_H

#include "llog.h"

typedef enum {
  export_format_adi = 0,
  export_format_adx,
  export_format_csv
} adif_writer_export_format_t;


typedef enum {
  export_status_ok = 0,
  export_status_err,
  export_status_file_err
} adif_writer_status_t;

int exporter_write_header(const char *filename, adif_writer_export_format_t format);
int exporter_add_qso(log_entry_t *entry, station_entry_t *station);
void exporter_close(void);

#endif
