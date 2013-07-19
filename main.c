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

	struct timeval tv;
	struct tm bdt;
	FILE *fp;

	char *csv_list[CSV_LIST_LEN];

	llog_t log_variables;

/*defaults*/

	strcpy(logfile, "log.csv");
	*my_QRA='\0';
	*my_QTH='\0';
	*my_RIG='\0';
	*my_ANT='\0';

/*confuguration storage*/
	ConfigAttribute config_attributes[] = {
		{"my_QRA", CONFIG_String, my_QRA},
		{"my_QTH", CONFIG_String, my_QTH},
		{"my_RIG", CONFIG_String, my_RIG},
		{"my_ANT", CONFIG_String, my_ANT},
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


	while ((opt = getopt(argc, argv, "q:r:R:f:a:")) !=-1) {
		switch (opt) {
		case 'f':
			strncpy(logfile, optarg, LOGFILE_LEN);
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
	fprintf(stderr, "\tQTH: %s\n\tQRA: %s\n\tRIG: %s\n\tANT: %s\n", my_QTH, my_QRA, my_RIG, my_ANT);

	strcpy(log_variables.default_rst, "59");

	reset_values(&log_variables);
	reset_values_static(&log_variables);

	line=NULL;

	while (1) {

		free(line);
		/*input the data*/

		printf("\nCall [%s]", log_variables.call);
		line=readline(prompt);
		if (line==NULL) {
			break;
		}
		if (*line!='\0') {
			strcpy(log_variables.call, line);
		}
		free(line);
		line=NULL;
		if (*log_variables.call=='\0') {
			continue;
		}

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

		printf("TXRST [%s]", log_variables.txrst);
		line=readline(prompt);
		if (line==NULL) {
			break;
		}
		if (strcmp(line, CANCEL_SEQ)==0) {
			reset_values(&log_variables);
			continue;
		}
		if (*line!='\0') {
			strcpy(log_variables.txrst, line);
		}
		free(line);

		printf("RXRST [%s]", log_variables.rxrst);
		line=readline(prompt);
		if (line==NULL) {
			break;
		}
		if (strcmp(line, CANCEL_SEQ)==0) {
			reset_values(&log_variables);
			continue;
		}
		if (*line!='\0') {
			strcpy(log_variables.rxrst, line);
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
			strcpy(log_variables.name, line);
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
			strcpy(log_variables.QTH, line);
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
			strcpy(log_variables.QRA, line);
		}
		free(line);

		printf("RX NR [%04u]", log_variables.rx_nr);
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

		printf("TX NR [%04u]", log_variables.tx_nr);
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
			strcpy(log_variables.QRG, line);
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
			strcpy(log_variables.mode, line);
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
			strcpy(log_variables.comment, line);
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
		gettimeofday(&tv, NULL);
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
		sprintf(substr, "%s,", log_variables.comment);
		strcat(f_line, substr);
		sprintf(substr, "%s,%s,%s,%s\n", my_QTH, my_QRA, my_RIG, my_ANT);
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
	strcpy(data->comment, "TNX for nice QSO! 73 es DX!");
	data->rx_nr=0;

	return;
}

void reset_values_static(llog_t *data) {

	*data->QRG='\0';
	*data->mode='\0';
	data->tx_nr=1;

	return;
}

