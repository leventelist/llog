/*	This is llog, a minimalist HAM logging software.
 *	Copyright (C) 2013-2019  Levente Kovacs
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
#include "csv.h"
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
#include "getch.h"
#include "db_sqlite.h"

static void printver(void);
static void printhelp(void);

static void reset_values(llog_t *log, logEntryT *entry);
static void reset_values_static(llog_t *log, logEntryT *entry);
static void set_default_rst(llog_t *data);
static int dup_check(llog_t *data);
static int get_data(const char *prompt, char *data);
static void print_log_data(llog_t *data);
static int fwrite_log_data(llog_t *data);
static void strupper(char *s);
static int llog_setup(llog_t *data);
static int print_local_values(llog_t *data, int n);
static void printver(void);



int main(int argc, char *argv[]) {

	int opt;
	int ret;
	char *line;
	char f_line[LINE_LEN];
	const char *prompt=PROMPT;

	FILE *fp;

	llog_t llog;
	logEntryT logEntry;
	stationEntryT station;


/*defaults*/

	strcpy(llog.logfileFn, "log.sqlite");



	ret=0;

	while ((opt = getopt(argc, argv, "f:s:hv")) !=-1) {
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
			ret=1;
		break;
		case 'v':
			printver();
			return(OK);
		case '?':
		case ':':
		default:
			printf("Error parsing the command line arguemts\n");
			printhelp();
			return(CMD_LINE_ERR);
			break;
		}
	}
	/*Try to open the logfile. Just for test.*/
	ret = db_sqlite_init(&llog);

	if (ret != SQLITE_OK) {
		fprintf(stderr, "Could not open log file '%s'\n", llog.logfileFn);
		return FILE_ERR;
	}


	fprintf(stderr, "Using logfile '%s'\n", llog.logfileFn);
//	print_local_values(&log_variables, 0);


/*get the maximum TX number*/



//	fprintf(stderr, "\tTX serial number: %04d\n", log_variables.tx_nr);

	opt='0';

	while (1) {
		if (opt=='q') {
			break;
		}
//		print_log_data(&log_variables);
//		dup_check(&log_variables);
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
				ret=get_data("QRG: ", logEntry.QRG);
			break;
			case 'm':
				ret=get_data("Mode: ", logEntry.mode);
				strupper(logEntry.mode);
				set_default_rst(&logEntry);
			break;
			case 'p':
				ret=get_data("Power: ", logEntry.pwr);
			break;
			case 'e':
				ret=get_data("Comment: ", logEntry.comment);
			break;
			case 'w':
				if (*logEntry.call=='\0') {
					break;
				}
				ret=fwrite_log_data(&logEntry);
				if (ret==OK) {
					logEntry.tx_nr++;
					reset_values(&llog, &logEntry);
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
			break;
			case 's':
				llog_setup(&logEntry);
			break;
		}
	}

	return ret;
}

static void reset_values(llog_t *log, logEntryT *entry) {

	*entry->QTH='\0';
	*entry->QRA='\0';
	*entry->call='\0';
	*entry->name='\0';
	strcpy(entry->rxrst, log->defaultRst);
	strcpy(entry->txrst, log->defaultRst);
	strcpy(entry->comment, "73 DX!");
	entry->rx_nr=0;
	*entry->rx_x='\0';

	return;
}


static void reset_values_static(llog_t *log, logEntryT *entry) {

	*entry->QRG='\0';
	*entry->mode='\0';
	*entry->pwr='\0';
	entry->tx_nr=1;
//	strcpy(entry->default_rst, "599");
	*entry->tx_x='\0';

	return;
}


static void set_default_rst(llog_t *data) {

	if (strstr(data->mode, "FM") || strstr(data->mode, "USB") || strstr(data->mode, "LSB") || strstr(data->mode, "SSB")) {
		strcpy(data->default_rst, "59");
	} else {
		strcpy(data->default_rst, "599");
	}
	return;
}


static int dup_check(llog_t *data) {

	FILE *fp;
	char *line;
	char f_line[LINE_LEN];
	char *csv_list[CSV_LIST_LEN];

	if (*data->call=='\0') {
		return OK;
	}

	fp=fopen(data->logfile, "r");
	if (fp==NULL) {
		fprintf(stderr, "Could not open log file '%s'\n", data->logfile);
		return FILE_ERR;
	}
	while (1) {
		line=fgets(f_line, LINE_LEN, fp);
		if (line==NULL) {
			break;
		}
		csv_parse(f_line, csv_list, CSV_LIST_LEN);
		if (strcasestr(csv_list[CSV_CALL_POS], data->call)!=NULL) {
			fprintf(stderr, "DUP QSO on %s at %s; RXNR: %s TXNR: %s \n", csv_list[CSV_DATE_POS], csv_list[CSV_TIME_POS], csv_list[CSV_RXNR_POS], csv_list[CSV_TXNR_POS]);
		}
	}
	fclose(fp);
	return OK;
}


static int get_data(const char *prompt, char *data) {

	char *line;
	int ret;

	ret=LLOG_ERR;
	line=readline(prompt);
	if (line==NULL) {
		*data='\0';
		ret=LLOG_EOF;
	} else if (*line=='\0') {
		ret=LLOG_CANCEL;
	} else {
		strcpy(data, line);
		ret=OK;
	}
	if (line!=NULL) {
		free(line);
	}

	return ret;
}


static void print_log_data(llog_t *data) {

	printf("\nc: Call [%s]\no: Operator's name: [%s]\n", data->call, data->name);
	printf("r: RXRST [%s]\nR: TXRST [%s]\n", data->rxrst, data->txrst);
	printf("t: QTH [%s]\na: QRA [%s]\n", data->QTH, data->QRA);
	printf("g: QRG [%s]\nm: mode [%s]\np: Power: [%s]\n", data->QRG, data->mode, data->pwr);
	printf("n: RXNR [%04u]\nN: TXNR [%04u]\n", data->rx_nr, data->tx_nr);
	printf("x: RX_EXTRA [%s]\nX: TX_EXTRA [%s]\n", data->rx_x, data->tx_x);
	printf("e: Comment [%s]\n\n", data->comment);
	printf("w: Write!\tq: QRT\t\ts: Setup\n");
	return;
}


static int fwrite_log_data(llog_t *data) {

	char substr[SUBSTR_LEN];
	char f_line[LINE_LEN];
	struct tm bdt;
	FILE *fp;

	*f_line='\0';
	gmtime_r(&(data->tv.tv_sec), &bdt);
	sprintf(substr, "%d-%02d-%02d,%02d%02d,", 1900+bdt.tm_year, 1+bdt.tm_mon, bdt.tm_mday, bdt.tm_hour, bdt.tm_min);
	strcat(f_line, substr);
	sprintf(substr, "%s,%s,%s,", data->call, data->rxrst, data->txrst);
	strcat(f_line, substr);
	sprintf(substr, "%s,%s,%s,", data->QTH, data->QRA, data->name);
	strcat(f_line, substr);
	sprintf(substr, "%s,%s,", data->QRG, data->mode);
	strcat(f_line, substr);
	sprintf(substr, "%04u,%04u,", data->rx_nr, data->tx_nr);
	strcat(f_line, substr);
	sprintf(substr, "%s,%s,", data->rx_x, data->tx_x);
	strcat(f_line, substr);
	sprintf(substr, "\"%s\",%s,", data->comment, data->qsl_stat);
	strcat(f_line, substr);
	sprintf(substr, "%s,%s,%s,%s,%s,%s,%s\n", data->my_call, data->my_QTH, data->my_QRA, data->my_alt, data->my_RIG, data->pwr, data->my_ANT);
	strcat(f_line, substr);
	fp=fopen(data->logfile, "a+");
	if (fp==NULL) {
		fprintf(stderr, "Could not open log file '%s'\n", data->logfile);
		return FILE_ERR;
	}
	fprintf(fp, "%s", f_line);
	fflush(fp);
	fclose(fp);

	return OK;
}


static void strupper(char *s) {
	while (*s) {
		if ((*s >= 'a' ) && (*s <= 'z')) {
			*s -= ('a'-'A');
		}
		s++;
	}
}

int print_local_values(llog_t *data, int n) {

	if (n==0) {
	fprintf(stderr, "Using local settings: \n\n");
	fprintf(stderr, "\tCall: %s\n\tQTH: %s\n\tQRA: %s\n\tRIG: %s\n\tANT: %s\n", data->my_call, data->my_QTH, data->my_QRA, data->my_RIG, data->my_ANT);
	fprintf(stderr, "\tAltitude: %s\n\tPower: %s\n", data->my_alt, data->pwr);
	} else {
	fprintf(stderr, "\nc: Call [%s]\nt: QTH: [%s]\na: QRA [%s]\nr: RIG [%s]\nn: ANT [%s]\nl: Altitude [%s]\n", data->my_call, data->my_QTH, data->my_QRA, data->my_RIG, data->my_ANT, data->my_alt);
	fprintf(stderr, "\nq: Quit\t\tw: Write!\n");
	}
	return OK;
}

int llog_setup(llog_t *data) {

	char c;
	const char *prompt=PROMPT;
	c=0;

	while (1) {
		if (c=='q'){
			break;
		}
		print_local_values(data, 1);
		printf(prompt);
		fflush(stdout);
		c=getch();
		switch (c) {
			case 'c':
				get_data("CALL: ", data->my_call);
				strupper(data->my_call);
			break;
			case 't':
				get_data("QTH: ", data->my_QTH);
			break;
			case 'a':
				get_data("QRA: ", data->my_QRA);
				strupper(data->my_QRA);
			break;
			case 'r':
				get_data("RIG: ", data->my_RIG);
			break;
			case 'n':
				get_data("ANT: ", data->my_ANT);
			break;
			case 'l':
				get_data("ALT: ", data->my_alt);
			break;
			case 'w':
				write_local_values(data);
			break;
		}
	}

	return OK;
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
	printf("\t-c FILE\t\tUse config file FILE.\n");
	printf("\t-f FILE\t\tWrite output to logfile FILE.\n");
	printf("\t-l CALL\t\tUse callsigne CALL.\n");
	printf("\t-r QRA\t\tUse QRA.\n");
	printf("\t-q QTH\t\tUse QTH.\n");
	printf("\t-t ALT\t\tUse altitude ALT.\n");
	printf("\t-p PWR\t\tSet output power to PWR.\n");
	printf("\t-a ANT\t\tUse ANT as antenna.\n");
	printf("\t-R RIG\t\tUse RIG.\n");
	printf("\t-n NR\t\tSet number to be transmitted to NR.\n");
	printf("\t-x INFO\t\tSet information to be transmitted to INFO.\n");
	printf("\t-h\t\tGet help.\n");
	printf("\t-v\t\tPrint version information.\n\n");
}

