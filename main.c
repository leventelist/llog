/*	This is llog, a minimalist HAM logging software.
 *	Copyright (C) 2013-2014  Levente Kovacs
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

int main(int argc, char *argv[]) {

	int opt;
	int ret;
	char *line;
	char f_line[LINE_LEN];
	const char *prompt=PROMPT;

	FILE *fp;

	char *csv_list[CSV_LIST_LEN];

	llog_t log_variables;

/*defaults*/

	strcpy(log_variables.logfile, "log.csv");
	strcpy(log_variables.qsl_stat, "TX0RX0");
	*log_variables.my_call='\0';
	*log_variables.my_QRA='\0';
	*log_variables.my_QTH='\0';
	*log_variables.my_RIG='\0';
	*log_variables.my_ANT='\0';
	*log_variables.my_alt='\0';

	reset_values_static(&log_variables);
	reset_values(&log_variables);

/*confuguration storage*/
	ConfigAttribute config_attributes[] = {
		{"my_call", CONFIG_String, log_variables.my_call},
		{"my_QRA", CONFIG_String, log_variables.my_QRA},
		{"my_QTH", CONFIG_String, log_variables.my_QTH},
		{"my_RIG", CONFIG_String, log_variables.my_RIG},
		{"my_ANT", CONFIG_String, log_variables.my_ANT},
		{"my_ALT", CONFIG_String, log_variables.my_alt},
		{"my_PWR", CONFIG_String, log_variables.pwr},
		{"logfile", CONFIG_String, log_variables.logfile},
		{NULL, CONFIG_Unused, NULL}
	};

	fprintf(stderr, "\n");

/*read config file(s)*/
	ret=config_file_read("~/"CONFIG_FILE_NAME, config_attributes);
	if (ret==CONF_OK){
		fprintf(stderr, "Config file read in '%s'\n", "~/"CONFIG_FILE_NAME);
	}
	ret=config_file_read("./"CONFIG_FILE_NAME, config_attributes);
	if (ret==CONF_OK){
		fprintf(stderr, "Config file read in '%s'\n", "./"CONFIG_FILE_NAME);
	}

	ret=0;

	while ((opt = getopt(argc, argv, "q:r:R:f:a:n:c:l:p:")) !=-1) {
		switch (opt) {
		case 'l':
			strncpy(log_variables.my_alt, optarg, ALT_LEN);
		break;
		case 'p':
			strncpy(log_variables.pwr, optarg, PWR_LEN);
		break;
		case 'c':
			strncpy(log_variables.my_call, optarg, CALL_LEN);
		break;
		case 'f':
			strncpy(log_variables.logfile, optarg, LOGFILE_LEN);
		break;
		case 'q':
			strncpy(log_variables.my_QTH, optarg, QTH_LEN);
		break;
		case 'r':
			strncpy(log_variables.my_QRA, optarg, QRA_LEN);
		break;
		case 'a':
			strncpy(log_variables.my_ANT, optarg, ANT_LEN);
		break;
		case 'R':
			strncpy(log_variables.my_RIG, optarg, RIG_LEN);
		break;
		case 'n':
			log_variables.tx_nr=strtoul(optarg, NULL, 10);
			ret=1;
		break;
		case '?':
		case ':':
		default:
			printf("Error parsing the command line arguemts\n");
			return(CMD_LINE_ERR);
			break;
		}
	}
	/*Try to open the logfile. Just for test.*/
	fp=fopen(log_variables.logfile, "a+");
	if (fp==NULL) {
		fprintf(stderr, "Could not open log file '%s'\n", log_variables.logfile);
		return FILE_ERR;
	}
	fclose (fp);

	fprintf(stderr, "Using logfile '%s'\n", log_variables.logfile);
	print_local_values(&log_variables, 0);

	if (ret==0) {
		fp=fopen(log_variables.logfile, "r");
		if (fp==NULL) {
			fprintf(stderr, "\nCould not open log file '%s'\n", log_variables.logfile);
			return FILE_ERR;
		}
		while (1) {
			line=fgets(f_line, LINE_LEN, fp);
			if (line==NULL) {
				break;
			}
			if (csv_parse(f_line, csv_list, CSV_LIST_LEN)<CSV_LIST_LEN) {
				continue;
			}
			log_variables.tx_nr=1 + strtoul(csv_list[CSV_TXNR_POS], NULL, 10);
		}
		fclose(fp);
	}

	fprintf(stderr, "\tTX serial number: %04d\n", log_variables.tx_nr);

	opt='c';

	while (1) {
		if (opt=='q') {
			break;
		}
		print_log_data(&log_variables);
		dup_check(&log_variables);
		if (opt!='c') {
			printf(prompt);
			fflush(stdout);
			opt=getch();
		}
		switch (opt) {
			case 'c':
				ret=get_data("Call: ", log_variables.call);
				gettimeofday(&log_variables.tv, NULL);
				strupper(log_variables.call);
				opt=0;
			break;
			case 'o':
				ret=get_data("Operator name: ", log_variables.name);
			break;
			case 'r':
				ret=get_data("RXRST: ", log_variables.rxrst);
			break;
			case 'R':
				ret=get_data("TXRST: ", log_variables.txrst);
			break;
			case 'n':
				ret=get_data("RXNR: ", f_line);
				log_variables.rx_nr=strtoul(f_line, NULL, 10);;
			break;
			case 'N':
				ret=get_data("TXNR: ", f_line);
				log_variables.tx_nr=strtoul(f_line, NULL, 10);;
			break;
			case 't':
				ret=get_data("QTH: ", log_variables.QTH);
			break;
			case 'a':
				ret=get_data("QRA: ", log_variables.QRA);
				strupper(log_variables.QRA);
			break;
			case 'g':
				ret=get_data("QRG: ", log_variables.QRG);
			break;
			case 'm':
				ret=get_data("Mode: ", log_variables.mode);
				strupper(log_variables.mode);
			break;
			case 'p':
				ret=get_data("Power: ", log_variables.pwr);
			break;
			case 'e':
				ret=get_data("Comment: ", log_variables.comment);
			break;
			case 'w':
				if (*log_variables.call=='\0') {
					break;
				}
				ret=fwrite_log_data(&log_variables);
				if (ret==OK) {
					log_variables.tx_nr++;
					reset_values(&log_variables);
					printf("\nWritten OK.\n");
				} else {
					printf("\nError writing record.\n");
				}
				opt='c';
			break;
			case 'x':
				ret=get_data("RX extra: ", log_variables.rx_x);
			break;
			case 'X':
				ret=get_data("TX extra: ", log_variables.tx_x);
			break;
			case 'q':
				printf("\n");
			break;
			case 's':
				llog_setup(&log_variables);
				opt='c';
			break;
		}
	}

	return ret;
}

void reset_values(llog_t *data) {

	*data->QTH='\0';
	*data->QRA='\0';
	*data->call='\0';
	*data->name='\0';
	strcpy(data->rxrst, data->default_rst);
	strcpy(data->txrst, data->default_rst);
	strcpy(data->comment, "73 DX!");
	data->rx_nr=0;
	*data->rx_x='\0';

	return;
}

void reset_values_static(llog_t *data) {

	*data->QRG='\0';
	*data->mode='\0';
	*data->pwr='\0';
	data->tx_nr=1;
	strcpy(data->default_rst, "59");
	*data->tx_x='\0';

	return;
}

int dup_check(llog_t *data) {

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

int get_data(const char *prompt, char *data) {

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

void print_log_data(llog_t *data) {

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

int fwrite_log_data(llog_t *data) {

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

void strupper(char *s) {
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
				get_data("QTH: ", data->my_call);
			break;
			case 't':
				get_data("QTH: ", data->my_QTH);
			break;
			case 'a':
				get_data("QRA: ", data->my_QRA);
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

int write_local_values(llog_t *data) {

	FILE *fp;

	fp=fopen(CONFIG_FILE_NAME, "w");

	if (fp==NULL) {
		fprintf(stderr, "\nCould not open config file '%s'\n", CONFIG_FILE_NAME);
		return FILE_ERR;
	}

	if (*data->my_call!='\0') {
		fprintf(fp, "my_call = %s\n", data->my_call);
	}
	if (*data->my_QTH!='\0') {
		fprintf(fp, "my_QTH = %s\n", data->my_QTH);
	}
	if (*data->my_QRA!='\0') {
		fprintf(fp, "my_QRA = %s\n", data->my_QRA);
	}
	if (*data->my_RIG!='\0') {
		fprintf(fp, "my_RIG = %s\n", data->my_RIG);
	}
	if (*data->my_ANT!='\0') {
		fprintf(fp, "my_ANT = %s\n", data->my_ANT);
	}
	if (*data->my_alt!='\0') {
		fprintf(fp, "my_ALT = %s\n", data->my_alt);
	}
	if (*data->pwr!='\0') {
		fprintf(fp, "my_PWR = %s\n", data->pwr);
	}
	if (*data->logfile!='\0') {
		fprintf(fp, "logfile = %s\n\n", data->logfile);
	}
	fflush(fp);
	fclose(fp);
	fprintf(stderr, "\nConfiguration written to file '%s'\n", CONFIG_FILE_NAME);

	return OK;
}

