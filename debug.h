/*	Debug facilities.
 *
 *	Copyright (C) 2012-2016 Levente Kovacs
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * http://levente.logonex.eu
 * leventelist@gmail.com
 *
 */

#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>
#include <stdio.h>

/*constants*/
#define DEBUG_MAX_MSG 2048
#define DEBUG_FILE_LEN 512

enum {
	DEBUG_WRITTEN=0,
	DEBUG_NOT_WRITTEN

};

enum {
	DEBUG_QUIET=0,
	DEBUG_ERR,
	DEBUG_INFO,
	DEBUG_ALL
};

enum {
	DEBUG_RET_OK=0,
	DEBUG_RET_ERR=1
};

/*This is needed for SWIG.*/
struct Bind_debug {
	FILE *fd;
	int level;
	char fn[DEBUG_FILE_LEN];
};

/*do the normal type for C*/
typedef struct Bind_debug debug_t;

/*function prototypes*/

int debug_init(debug_t *dbg);
int debug(debug_t *dbg, int score, const char *fmt, ...);
int debug_close(debug_t *dbg);


#endif
