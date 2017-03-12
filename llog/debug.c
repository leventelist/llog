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
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/time.h>
#include <time.h>

#include "debug.h"


int debug_init(debug_t *dbg) {

	int ret;

	dbg->fd=fopen(dbg->fn, "a");
	if (dbg->fn==NULL) {
		ret=DEBUG_RET_ERR;
	} else {
		ret=DEBUG_RET_OK;
	}

	return ret;
}


int debug(debug_t *dbg, int score, const char *fmt, ...) {

	struct timeval tv;
	struct tm bdt;

	gettimeofday(&tv, NULL);

	if (dbg->level>=score && dbg->fd!=NULL) {

		localtime_r(&(tv.tv_sec), &bdt);
		fprintf(dbg->fd, "%d-%02d-%02d %02d:%02d:%02d | ", 1900+bdt.tm_year, 1+bdt.tm_mon, bdt.tm_mday, bdt.tm_hour, bdt.tm_min, bdt.tm_sec);
		va_list args;
		va_start(args, fmt);
		vfprintf(dbg->fd, fmt, args);
		va_end(args);
		fflush(dbg->fd);
		return DEBUG_WRITTEN;
	}

	return DEBUG_NOT_WRITTEN;
}

int debug_close(debug_t *dbg) {

	int ret=0;

	if (dbg->fd!=NULL) {
		ret=fclose(dbg->fd);
		dbg->fd=NULL;
	}
		return ret;
}
