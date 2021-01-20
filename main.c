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

static void printver(void);
static void printhelp(void);

static void reset_values(logEntry_t *entry);
static void reset_values_static(logEntry_t *entry);
static int get_data(const char *prompt, char *data);
static void print_log_data(logEntry_t *entry);
static void strupper(char *s);
static void printver(void);
static int get_mode(const char *prompt, op_mode_t *data);


op_mode_t modes[MONDE_N]={ {"CW", "599"}, {"RTTY", "599"}, {"PSK31", "599"}, {"PSK63", "599"}, {"OLIVIA", "599"},
							{"FT8", "599"}, {"FT4", "599"}, {"USB", "59"}, {"LSB", "59"}, {"AM", "59"}, {"FM", "59"},
							{NULL, NULL}};

int main(int argc, char *argv[]) {

	int opt;
	int ret;
	int mode;
	char f_line[LINE_LEN];
	const char *prompt=PROMPT;
	char buff[256];

	llog_t llog;
	logEntry_t logEntry;
	stationEntry_t station;


/*defaults*/

	strcpy(llog.logfileFn, "log.sqlite");

	ret = 0;
	mode = LLOG_MODE_N;
	strncpy(llog.station, "1", STATION_LEN);
/*Parse command line arguments*/

	while ((opt = getopt(argc, argv, "f:s:lhv")) !=-1) {
		switch (opt) {
		case 'h': /*print help*/
			printhelp();
			return(OK);
		break;

		case 'f':
			strncpy(llog.logfileFn, optarg, LOGFILE_LEN);
		break;
		case 's':
			strncpy(llog.station, optarg, STATION_LEN);
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
	/*Try to open the log database.*/
	ret = db_sqlite_init(&llog);

	if (ret != SQLITE_OK) {
		fprintf(stderr, "Could not open log database '%s'\n", llog.logfileFn);
		return FILE_ERR;
	}


	fprintf(stderr, "Using log database '%s'\n", llog.logfileFn);

	if (mode == LLOG_MODE_L) {
		list_stations(&llog);
		return(0);
	}

	ret = lookupStation(&llog, &station);

	logEntry.stationId = station.id;

	reset_values_static(&logEntry);
	reset_values(&logEntry);


/*get the maximum TX number*/

	getMaxNr(&llog, &logEntry);

//	fprintf(stderr, "\tTX serial number: %04d\n", log_variables.tx_nr);

	opt='0';

	while (1) {
		if (opt=='q') {
			break;
		}
		print_log_data(&logEntry);
		checkDupQSO(&llog, &logEntry);
		printf(prompt);
		fflush(stdout);
		opt=getch();
		switch (opt) {
			case 'c':
				ret=get_data("Call: ", logEntry.call);
				gettimeofday(&logEntry.tv, NULL);
				strupper(logEntry.call);
			break;
			case 'o':
				ret=get_data("Operator name: ", logEntry.name);
			break;
			case 'r':
				ret=get_data("RXRST: ", logEntry.rxrst);
			break;
			case 'R':
				ret=get_data("TXRST: ", logEntry.txrst);
			break;
			case 'n':
				ret=get_data("RXNR: ", f_line);
				logEntry.rx_nr=strtoul(f_line, NULL, 10);;
			break;
			case 'N':
				ret=get_data("TXNR: ", f_line);
				logEntry.tx_nr=strtoul(f_line, NULL, 10);;
			break;
			case 't':
				ret=get_data("QTH: ", logEntry.QTH);
			break;
			case 'a':
				ret=get_data("QRA: ", logEntry.QRA);
				strupper(logEntry.QRA);
			break;
			case 'g':
				ret = get_data("QRG: ", buff);
				logEntry.QRG = strtod(buff, NULL);
			break;
			case 'm':
				ret = get_mode("Mode: ", &logEntry.mode);
				strcpy(logEntry.rxrst, logEntry.mode.default_rst);
				strcpy(logEntry.txrst, logEntry.mode.default_rst);
			break;
			case 'p':
				ret = get_data("Power: ", logEntry.pwr);
			break;
			case 'e':
				ret = get_data("Comment: ", logEntry.comment);
			break;
			case 'w':
				if (*logEntry.call=='\0') {
					break;
				}
				ret = setLogEntry(&llog, &logEntry);
				if (ret==OK) {
					logEntry.tx_nr++;
					reset_values(&logEntry);
					printf("\nWritten OK.\n");
				} else {
					printf("\nError writing record.\n");
				}
			break;
			case 'x':
				ret=get_data("RX extra: ", logEntry.rx_x);
			break;
			case 'X':
				ret=get_data("TX extra: ", logEntry.tx_x);
			break;
			case 'q':
				printf("\n");
				db_sqlite_close(&llog);
			break;
			case 's':
				list_stations(&llog);
				ret = get_data("Station ID or name: ", llog.station);
				lookupStation(&llog, &station);
				logEntry.stationId = station.id;
			break;
		}
	}

	return ret;
}


static void reset_values(logEntry_t *entry) {

	*entry->QTH = '\0';
	*entry->QRA = '\0';
	*entry->call = '\0';
	*entry->name = '\0';
	strcpy(entry->rxrst, entry->mode.default_rst);
	strcpy(entry->txrst, entry->mode.default_rst);
	strcpy(entry->comment, "73 DX!");
	entry->rx_nr = 0;
	*entry->rx_x = '\0';

	return;
}


static void reset_values_static(logEntry_t *entry) {

	entry->QRG = 0;
	entry->mode = modes[0];
	*entry->pwr = '\0';
	entry->tx_nr = 1;
	*entry->tx_x = '\0';

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
		if (modes[i].name == NULL) {
			break;
		}
		printf("\t%5d: %-10s", i, modes[i].name);
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
	*data = modes[j];

	return ret;
}


static void print_log_data(logEntry_t *entry) {

	printf("\nc: Call [%s]\no: Operator's name: [%s]\n", entry->call, entry->name);
	printf("r: RXRST [%s]\nR: TXRST [%s]\n", entry->rxrst, entry->txrst);
	printf("t: QTH [%s]\na: QRA [%s]\n", entry->QTH, entry->QRA);
	printf("g: QRG [%f]\nm: mode [%s]\np: Power: [%s]\n", entry->QRG, entry->mode.name, entry->pwr);
	printf("n: RXNR [%04"PRIu64"]\nN: TXNR [%04"PRIu64"]\n", entry->rx_nr, entry->tx_nr);
	printf("x: RX_EXTRA [%s]\nX: TX_EXTRA [%s]\n", entry->rx_x, entry->tx_x);
	printf("e: Comment [%s]\n\n", entry->comment);
	printf("w: Write!\ts: Select station\tq: QRT\n");
	return;
}


static void strupper(char *s) {
	while (*s) {
		if ((*s >= 'a' ) && (*s <= 'z')) {
			*s -= ('a'-'A');
		}
		s++;
	}
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
