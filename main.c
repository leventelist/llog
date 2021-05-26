/*	This is llog, a minimalist HAM logging software.
 *	Copyright (C) 2013-2021  Levente Kovacs
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
 */

#define _GNU_SOURCE
#include "conf.h"
#include "llog.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdint.h>
#include <inttypes.h>
#include "getch.h"
#include "db_sqlite.h"

#include "main_window.h"

static void printver(void);
static void printhelp(void);

static void reset_values(log_entry_t *entry);
static void reset_values_static(log_entry_t *entry);
static int get_data(const char *prompt, char *data);
static void strupper(char *s);
static void printver(void);
static int get_mode(const char *prompt, op_mode_t *data);


//op_mode_t modes[MONDE_N] = {{"CW", "599"}, {"RTTY", "599"}, {"PSK31", "599"}, {"PSK63", "599"},
//							{"OLIVIA", "599"}, {"MFSK16", "599"}, {"FT8", "599"}, {"FT4", "599"},
//							{"USB", "59"}, {"LSB", "59"}, {"AM", "59"}, {"FM", "59"},
//							{NULL, NULL}};

int main(int argc, char *argv[]) {

	int opt;
	int ret;
	int mode;
	char f_line[LINE_LEN];
	const char *prompt=PROMPT;
	char buff[256];

//	llog_t llog;
	log_entry_t log_entry;
	station_entry_t station;


	/*Initialize main data structures*/
	llog_init("log.sqlite");

	ret = 0;
	mode = LLOG_MODE_N;
//	strncpy(llog.station, "1", STATION_LEN);
/*Parse command line arguments*/

	while ((opt = getopt(argc, argv, "f:s:lhv")) !=-1) {
		switch (opt) {
		case 'h': /*print help*/
			printhelp();
			return(OK);
		break;

		case 'f':
			llog_init(optarg);
			llog_open_db();
		break;
		case 's':
//			strncpy(llog.station, optarg, STATION_LEN);
		break;
		case 'v':
			printver();
			return(OK);
		break;
		case 'l':
			mode = LLOG_MODE_L;
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

	exit(EXIT_SUCCESS);

	if (mode == LLOG_MODE_L) {
//		list_stations(&llog);
		return(0);
	}

//	ret = lookupStation(&llog, &station);

	log_entry.stationId = station.id;

	reset_values_static(&log_entry);
	reset_values(&log_entry);


/*get the maximum TX number*/

//	getMaxNr(&llog, &log_entry);

//	fprintf(stderr, "\tTX serial number: %04d\n", log_variables.txnr);

	opt='0';

	while (1) {
		if (opt=='q') {
			break;
		}
//		print_log_data(&log_entry);
//		checkDupQSO(&llog, &log_entry);
		printf(prompt);
		fflush(stdout);
		opt=getch();
		switch (opt) {
			case 'c':
				ret = get_data("Call: ", log_entry.call);

				strupper(log_entry.call);
			break;
			case 'o':
				ret=get_data("Operator name: ", log_entry.name);
			break;
			case 'r':
				ret=get_data("RXRST: ", log_entry.rxrst);
			break;
			case 'R':
				ret=get_data("TXRST: ", log_entry.txrst);
			break;
			case 'n':
				ret=get_data("RXNR: ", f_line);
				log_entry.rxnr=strtoul(f_line, NULL, 10);;
			break;
			case 'N':
				ret=get_data("TXNR: ", f_line);
				log_entry.txnr=strtoul(f_line, NULL, 10);;
			break;
			case 't':
				ret=get_data("QTH: ", log_entry.qth);
			break;
			case 'a':
				ret=get_data("QRA: ", log_entry.qra);
				strupper(log_entry.qra);
			break;
			case 'g':
				ret = get_data("QRG: ", buff);
				log_entry.qrg = strtod(buff, NULL);
			break;
			case 'm':
				ret = get_mode("Mode: ", &log_entry.mode);
				strcpy(log_entry.rxrst, log_entry.mode.default_rst);
				strcpy(log_entry.txrst, log_entry.mode.default_rst);
			break;
			case 'p':
				ret = get_data("Power: ", log_entry.power);
			break;
			case 'e':
				ret = get_data("Comment: ", log_entry.comment);
			break;
			case 'w':
				if (*log_entry.call=='\0') {
					break;
				}
//				ret = db_set_log_entry(&llog, &log_entry);
				if (ret==OK) {
					log_entry.txnr++;
					reset_values(&log_entry);
					printf("\nWritten OK.\n");
				} else {
					printf("\nError writing record.\n");
				}
			break;
			case 'x':
				ret=get_data("RX extra: ", log_entry.rxextra);
			break;
			case 'X':
				ret=get_data("TX extra: ", log_entry.txextra);
			break;
			case 'q':
				printf("\n");
//				db_sqlite_close(&llog);
			break;
			case 's':
//				list_stations(&llog);
//				ret = get_data("Station ID or name: ", llog.station);
//				lookupStation(&llog, &station);
				log_entry.stationId = station.id;
			break;
		}
	}

	return ret;
}


static void reset_values(log_entry_t *entry) {

	*entry->qth = '\0';
	*entry->qra = '\0';
	*entry->call = '\0';
	*entry->name = '\0';
	strcpy(entry->rxrst, entry->mode.default_rst);
	strcpy(entry->txrst, entry->mode.default_rst);
	strcpy(entry->comment, "73 DX!");
	entry->rxnr = 0;
	*entry->rxextra = '\0';

	return;
}


static void reset_values_static(log_entry_t *entry) {

	entry->qrg = 0;
//	entry->mode = modes[0];
	*entry->power = '\0';
	entry->txnr = 1;
	*entry->txextra = '\0';

	return;
}


static int get_data(const char *prompt, char *data) {

	char *line;
	int ret;

	ret = LLOG_ERR;
	line = readline(prompt);
	if (line == NULL) {
		*data = '\0';
		ret = LLOG_EOF;
	} else if (*line == '\0') {
		ret = LLOG_CANCEL;
	} else {
		strcpy(data, line);
		ret = OK;
	}
	if (line != NULL) {
		free(line);
	}

	return ret;
}


static int get_mode(const char *prompt, op_mode_t *data) {

	uint32_t i, j;
	int ret = OK;
	char *line;

	printf("\n\nSelect mode. If none is selected, 0 will be used.\n\n");

	for (i = 0; i < MONDE_N; i++) {
//		if (modes[i].name == NULL) {
//			break;
//		}
//		printf("\t%5d: %-10s", i, modes[i].name);
		if ( i%2 == 1) {
			printf("\n");
		}
	}
	printf("\n");
	line = readline(prompt);

	j = strtoul(line, NULL, 0);
	if (j > i) {
		j = 0;
		ret = LLOG_ERR;
	}
	free(line);
//	*data = modes[j];

	return ret;
}





static void printver(void) {

	printf("\nThis is llog, a minimalist HAM log software.\n");
	printf("\nLicense: GNU 2.0.\n");
	printf("Version: %s.\n", VERSION);
	printf("Author: ha5ogl@logonex.eu.\n\n");
}


static void printhelp(void) {

	printver();
	printf("\nCommand line options\n\n");
	printf("\t-s STATIOM\t\tSelect station. Id or name.\n");
	printf("\t-f FILE\t\tWrite output to logfile FILE.\n");
	printf("\t-n NR\t\tSet number to be transmitted to NR.\n");
	printf("\t-l List available stations.\n");
	printf("\t-h\t\tGet help.\n");
	printf("\t-v\t\tPrint version information.\n\n");
}
