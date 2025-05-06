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

#ifndef POSITION_H
#define POSITION_H

#include <gps.h>
#include "llog.h"

#define POSITION_TIMEOUT_USEC 5000000UL

typedef struct gps_data_t gps_data_real_t;

typedef void (*position_callback_t)(position_t *);

/*Init the GPSd interface*/
int position_init(char *host, uint64_t port, position_callback_t callback);

/*Get the current position*/
void position_get(position_t *out_pos);

/*Calculate the distance between two positions*/
void position_distance_and_heading(position_t *pos1, position_t *pos2, double *distance, double *heading);

/*Convert a position to a QRA locator*/
void position_to_qra(position_t *pos, char *qra_locator);

/*Stop the GPSd interface*/
void position_stop(void);

#endif