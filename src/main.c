/*	This is llog, a minimalist HAM logging software.
 *	Copyright (C) 2013-2024  Levente Kovacs
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
#include <getopt.h>
#include <signal.h>

#include "main_window.h"

static void print_ver(void);
static void print_help(void);
static void signal_handler(int sig);

static llog_t *llog;


int main(int argc, char *argv[]) {

	int opt;


	struct sigaction sa;
	sa.sa_handler = signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGINT, &sa, NULL) == -1) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}

	if (sigaction(SIGTERM, &sa, NULL) == -1) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}

	/*Initialize main data structures*/
	llog = llog_init();

	/*Parse command line arguments*/

	while ((opt = getopt(argc, argv, "c:f:hv")) !=-1) {
		switch (opt) {
		case 'h': /*print help*/
			print_help();
			return(llog_stat_ok);
		break;
		case 'c':
			llog_set_config_file(optarg);
		break;
		case 'f':
			llog_set_log_file(optarg, true);
			llog_save_config_file();
		break;
		case 'v':
			print_ver();
			return(llog_stat_ok);
		break;
		case '?':
		case ':':
		default:
			printf("Error parsing the command line arguments\n");
			print_help();
			return(llog_cmd_line_err);
			break;
		}
	}

	/*This will also open the database for us.*/
	llog_parse_config_file();

	/*Draw main window*/
	main_window_set_llog(llog);
	main_window_draw(argc, argv);

	llog_shutdown();

	return EXIT_SUCCESS;
}


static void print_ver(void) {

	printf("\nThis is llog, a minimalist HAM log software.\n");
	printf("\nLicense: GNU 3.0.\n");
	printf("Version: %s.\n", VERSION);
	printf("Author: ha5ogl.levente@gmail.com.\n\n");
}


static void print_help(void) {

	print_ver();
	printf("\nCommand line options\n\n");
	printf("\t-c FILE\t\tRead configuration from FILE.\n");
	printf("\t-f FILE\t\tSet logfile to FILE.\n");
	printf("\t-h\t\tGet help.\n");
	printf("\t-v\t\tPrint version information.\n\n");
}


static void signal_handler(int sig) {
	printf("Caught signal %d, shutting down.\n", sig);
	llog_shutdown();
	exit(EXIT_SUCCESS);
}
