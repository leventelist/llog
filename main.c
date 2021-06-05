/*	This is llog, a minimalist HAM logging software.
 *	Copyright (C) 2013-2021  Levente Kovacs
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

#define _GNU_SOURCE
#include "llog.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "main_window.h"

static void printver(void);
static void printhelp(void);

static void printver(void);


int main(int argc, char *argv[]) {

	int opt;

	/*Initialize main data structures*/
	llog_init("log.sqlite");


	/*Parse command line arguments*/

	while ((opt = getopt(argc, argv, "f:s:hv")) !=-1) {
		switch (opt) {
		case 'h': /*print help*/
			printhelp();
			return(OK);
		break;

		case 'f':
			llog_init(optarg);
			llog_open_db();
		break;
//		case 's':
//			strncpy(llog.station, optarg, STATION_LEN);
		break;
		case 'v':
			printver();
			return(OK);
		break;
		case '?':
		case ':':
		default:
			printf("Error parsing the command line arguemts\n");
			printhelp();
			return(CMD_LINE_ERR);
			break;
		}
	}

	/*Draw main window*/
	main_window_draw();

	llog_shutdown();

	return EXIT_SUCCESS;
}


static void printver(void) {

	printf("\nThis is llog, a minimalist HAM log software.\n");
	printf("\nLicense: GNU 3.0.\n");
	printf("Version: %s.\n", VERSION);
	printf("Author: ha5ogl.levente@gmail.com.\n\n");
}


static void printhelp(void) {

	printver();
	printf("\nCommand line options\n\n");
//	printf("\t-s STATION\t\tSelect station. Id or name.\n");
	printf("\t-f FILE\t\tWrite output to logfile FILE.\n");
	printf("\t-h\t\tGet help.\n");
	printf("\t-v\t\tPrint version information.\n\n");
}
