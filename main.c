/*	This is llog, a minimalist HAM logging software.
 *	Copyright (C) 2013  Levente Kovacs
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


int main(int argc, char *argv[]) {

	int opt;
	int ret;
	char *line;
	const char *prompt=PROMPT;
	char substr[SUBSTR_LEN];
	char f_line[LINE_LEN];

	char logfile[LOGFILE_LEN];

	char my_QRA[QRA_LEN];
	char my_QTH[QTH_LEN];
	char my_RIG[RIG_LEN];
	char my_ANT[ANT_LEN];
	char my_call[CALL_LEN];
	char my_alt[ALT_LEN];
	char qsl_stat[QSL_LEN];
	struct timeval tv;
	struct tm bdt;
	FILE *fp;

	char *csv_list[CSV_LIST_LEN];

	llog_t log_variables;

/*defaults*/

	strcpy(logfile, "log.csv");
	strcpy(qsl_stat, "TX0RX0");
	*my_call='\0';
	*my_QRA='\0';
	*my_QTH='\0';
	*my_RIG='\0';
	*my_ANT='\0';
	*my_alt='\0';

	reset_values_static(&log_variables);
	reset_values(&log_variables);

/*confuguration storage*/
	ConfigAttribute config_attributes[] = {
		{"my_call", CONFIG_String, my_call},
		{"my_QRA", CONFIG_String, my_QRA},
		{"my_QTH", CONFIG_String, my_QTH},
		{"my_RIG", CONFIG_String, my_RIG},
		{"my_ANT", CONFIG_String, my_ANT},
		{"my_ALT", CONFIG_String, my_alt},
		{"my_PWR", CONFIG_String, log_variables.pwr},
		{"logfile", CONFIG_String, logfile},
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
			strncpy(my_alt, optarg, ALT_LEN);
		break;
		case 'p':
			strncpy(log_variables.pwr, optarg, PWR_LEN);
		break;
		case 'c':
			strncpy(my_call, optarg, CALL_LEN);
		break;
		case 'f':
			strncpy(logfile, optarg, LOGFILE_LEN);
		break;
		case 'q':
			strncpy(my_QTH, optarg, QTH_LEN);
		break;
		case 'r':
			strncpy(my_QRA, optarg, QRA_LEN);
		break;
		case 'a':
			strncpy(my_ANT, optarg, ANT_LEN);
		break;
		case 'R':
			strncpy(my_RIG, optarg, RIG_LEN);
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

	fp=fopen(logfile, "a+");

	if (fp==NULL) {
		fprintf(stderr, "Could not open log file '%s'\n", logfile);
		return FILE_ERR;
	}

	fprintf(stderr, "Using logfile '%s'\n", logfile);
	fprintf(stderr, "Using local settings: \n\n");
	fprintf(stderr, "\tCall: %s\n\tQTH: %s\n\tQRA: %s\n\tRIG: %s\n\tANT: %s\n", my_call, my_QTH, my_QRA, my_RIG, my_ANT);
	fprintf(stderr, "\tAltitude: %s\n\tPower: %s\n", my_alt, log_variables.pwr);

	if (ret==0) {
		rewind(fp);
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
		fseek(fp, 0L, SEEK_END);
	}
	line=NULL;

	fprintf(stderr, "\tTX serial number: %04d\n", log_variables.tx_nr);

	while (1) {

		free(line);
		/*input the data*/

		printf("\nCall [%s]", log_variables.call);
		line=readline(prompt);
		if (line==NULL) {
			break;
		}
		if (*line!='\0') {
			strncpy(log_variables.call, line, CALL_LEN);
		}
		free(line);
		line=NULL;
		if (*log_variables.call=='\0') {
			continue;
		}
		/*get the time*/
		gettimeofday(&tv, NULL);
		/*check for duplicate QSOs*/
		rewind(fp);
		while (1) {
			line=fgets(f_line, LINE_LEN, fp);
			if (line==NULL) {
				break;
			}
			csv_parse(f_line, csv_list, CSV_LIST_LEN);
			if (strcasestr(csv_list[CSV_CALL_POS], log_variables.call)!=NULL) {
				fprintf(stderr, "DUP QSO on %s at %s\n", csv_list[CSV_DATE_POS], csv_list[CSV_TIME_POS]);
			}
		}
		fseek(fp, 0L, SEEK_END);

		printf("TX_RST [%s]", log_variables.txrst);
		line=readline(prompt);
		if (line==NULL) {
			break;
		}
		if (strcmp(line, CANCEL_SEQ)==0) {
			reset_values(&log_variables);
			continue;
		}
		if (*line!='\0') {
			strncpy(log_variables.txrst, line, RST_LEN);
		}
		free(line);

		printf("RX_RST [%s]", log_variables.rxrst);
		line=readline(prompt);
		if (line==NULL) {
			break;
		}
		if (strcmp(line, CANCEL_SEQ)==0) {
			reset_values(&log_variables);
			continue;
		}
		if (*line!='\0') {
			strncpy(log_variables.rxrst, line, RST_LEN);
		}
		free(line);

		printf("Name [%s]", log_variables.name);
		line=readline(prompt);
		if (line==NULL) {
			break;
		}
		if (strcmp(line, CANCEL_SEQ)==0) {
			reset_values(&log_variables);
			continue;
		}
		if (*line!='\0') {
			strncpy(log_variables.name, line, NAME_LEN);
		}
		free(line);

		printf("QTH [%s]", log_variables.QTH);
		line=readline(prompt);
		if (line==NULL) {
			break;
		}
		if (strcmp(line, CANCEL_SEQ)==0) {
			reset_values(&log_variables);
			continue;
		}
		if (*line!='\0') {
			strncpy(log_variables.QTH, line, QTH_LEN);
		}
		free(line);

		printf("QRA [%s]", log_variables.QRA);
		line=readline(prompt);
		if (line==NULL) {
			break;
		}
		if (strcmp(line, CANCEL_SEQ)==0) {
			reset_values(&log_variables);
			continue;
		}
		if (*line!='\0') {
			strncpy(log_variables.QRA, line, QRA_LEN);
		}
		free(line);

		printf("RX_NR [%04u]", log_variables.rx_nr);
		line=readline(prompt);
		if (line==NULL) {
			break;
		}
		if (strcmp(line, CANCEL_SEQ)==0) {
			reset_values(&log_variables);
			continue;
		}
		if (*line!='\0') {
			log_variables.rx_nr = strtoul(line, NULL, 0);
		}
		free(line);

		printf("TX_NR [%04u]", log_variables.tx_nr);
		line=readline(prompt);
		if (line==NULL) {
			break;
		}
		if (strcmp(line, CANCEL_SEQ)==0) {
			reset_values(&log_variables);
			continue;
		}
		if (*line!='\0') {
			log_variables.tx_nr = strtoul(line, NULL, 0);
		}
		free(line);

		printf("QRG [%s]", log_variables.QRG);
		line=readline(prompt);
		if (line==NULL) {
			break;
		}
		if (strcmp(line, CANCEL_SEQ)==0) {
			reset_values(&log_variables);
			continue;
		}
		if (*line!='\0') {
			strncpy(log_variables.QRG, line, QRG_LEN);
		}
		free(line);

		printf("MODE [%s]", log_variables.mode);
		line=readline(prompt);
		if (line==NULL) {
			break;
		}
		if (strcmp(line, CANCEL_SEQ)==0) {
			reset_values(&log_variables);
			continue;
		}
		if (*line!='\0') {
			strncpy(log_variables.mode, line, MODE_LEN);
		}
		free(line);

		printf("PWR [%s]", log_variables.pwr);
		line=readline(prompt);
		if (line==NULL) {
			break;
		}
		if (strcmp(line, CANCEL_SEQ)==0) {
			reset_values(&log_variables);
			continue;
		}
		if (*line!='\0') {
			strncpy(log_variables.pwr, line, PWR_LEN);
		}
		free(line);

		printf("Comment [%s]", log_variables.comment);
		line=readline(prompt);
		if (line==NULL) {
			break;
		}
		if (strcmp(line, CANCEL_SEQ)==0) {
			reset_values(&log_variables);
			continue;
		}
		if (*line!='\0') {
			strncpy(log_variables.comment, line, COMMENT_LEN);
		}
		free(line);

		printf("Confirm? [Y|n]");
		line=readline(prompt);
		if (line==NULL) {
			break;
		}
		if (*line!='\0' && *line!='y' && *line!='Y') {
			continue;
		}

		*f_line='\0';
		gmtime_r(&(tv.tv_sec), &bdt);
		sprintf(substr, "%d-%02d-%02d,%02d:%02d,", 1900+bdt.tm_year, 1+bdt.tm_mon, bdt.tm_mday, bdt.tm_hour, bdt.tm_min);
		strcat(f_line, substr);
		sprintf(substr, "%s,%s,%s,", log_variables.call, log_variables.rxrst, log_variables.txrst);
		strcat(f_line, substr);
		sprintf(substr, "%s,%s,%s,", log_variables.QTH, log_variables.QRA, log_variables.name);
		strcat(f_line, substr);
		sprintf(substr, "%s,%s,", log_variables.QRG, log_variables.mode);
		strcat(f_line, substr);
		sprintf(substr, "%04u,%04u,", log_variables.rx_nr, log_variables.tx_nr);
		strcat(f_line, substr);
		sprintf(substr, "\"%s\",%s,", log_variables.comment, qsl_stat);
		strcat(f_line, substr);
		sprintf(substr, "%s,%s,%s,%s,%s,%s,%s\n", my_call, my_QTH, my_QRA, my_alt, my_RIG, log_variables.pwr, my_ANT);
		strcat(f_line, substr);
		printf("%s", f_line);
		fprintf(fp, "%s", f_line);
		fflush(fp);
		log_variables.tx_nr++;
		reset_values(&log_variables);
		
	}

	printf("\n");
	fclose(fp);
	free(line);

	return OK;
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

	return;
}

void reset_values_static(llog_t *data) {

	*data->QRG='\0';
	*data->mode='\0';
	*data->pwr='\0';
	data->tx_nr=1;
	strcpy(data->default_rst, "59");

	return;
}

