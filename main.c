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
	char *line;
	const char *prompt=": ";
	char substr[512];
	char output[1024];

	struct timeval tv;
	struct tm bdt;
	FILE *fp;	

	llog_t log_variables;


	while ((opt = getopt(argc, argv, "f:d:l:")) !=-1) {
		switch (opt) {
		case 'm':
//			strncpy(port, optarg, 40);
			break;
		case 'd': /*debug*/
//			debug_level=atoi(optarg);
			break;
		case 'f':
//			debug_fn=optarg;
			break;
		case '?':
		case ':':
		default:
			printf("Error parsing the command line arguemts\n");
			return(1);
			break;
		}
	}

	fp=fopen("log.csv", "a");

	strcpy(log_variables.default_rst, "59");

	reset_values(&log_variables);
	reset_values_static(&log_variables);

	while (1) {

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
		if (log_variables.call=='\0') {
			continue;
		}

		printf("TXRST [%s]", log_variables.txrst);
		line=readline(prompt);
		if (line==NULL) {
			break;
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
		if (*line!='\0') {
			strcpy(log_variables.rxrst, line);
		}
		free(line);

		printf("Name [%s]", log_variables.name);
		line=readline(prompt);
		if (line==NULL) {
			break;
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
		if (*line!='\0') {
			strcpy(log_variables.QTH, line);
		}
		free(line);

		printf("QRA [%s]", log_variables.QRA);
		line=readline(prompt);
		if (line==NULL) {
			break;
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
		if (*line!='\0') {
			log_variables.rx_nr = strtoul(line, NULL, 0);
		}
		free(line);

		printf("TX NR [%04u]", log_variables.tx_nr);
		line=readline(prompt);
		if (line==NULL) {
			break;
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
		if (*line!='\0') {
			strcpy(log_variables.QRG, line);
		}
		free(line);

		printf("MODE [%s]", log_variables.mode);
		line=readline(prompt);
		if (line==NULL) {
			break;
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
			free(line);
			continue;
		}

		*output='\0';
		gettimeofday(&tv, NULL);
		gmtime_r(&(tv.tv_sec), &bdt);
		sprintf(substr, "%d-%02d-%02d,%02d:%02d,", 1900+bdt.tm_year, 1+bdt.tm_mon, bdt.tm_mday, bdt.tm_hour, bdt.tm_min);
		strcat(output, substr);
		sprintf(substr, "%s,%s,%s,", log_variables.call, log_variables.rxrst, log_variables.txrst);
		strcat(output, substr);
		sprintf(substr, "%s,%s,%s,", log_variables.QTH, log_variables.QRA, log_variables.name);
		strcat(output, substr);
		sprintf(substr, "%s,%s,", log_variables.QRG, log_variables.mode);
		strcat(output, substr);
		sprintf(substr, "%04u,%04u,", log_variables.rx_nr, log_variables.tx_nr);
		strcat(output, substr);
		sprintf(substr, "%s\n", log_variables.comment);
		strcat(output, substr);
		printf("%s", output);
		fprintf(fp, "%s", output);
		fflush(fp);
		log_variables.tx_nr++;
		reset_values(&log_variables);
		
	}


	printf("\n");

	fclose(fp);

	if(line!=NULL) {
		free(line);
	}

	return 0;
}

void reset_values(llog_t *data) {

	*data->QTH='\0';
	*data->QRA='\0';
	*data->call='\0';
	*data->name='\0';
	strcpy(data->rxrst, data->default_rst);
	strcpy(data->txrst, data->default_rst);
	data->rx_nr=0;

	return;
}

void reset_values_static(llog_t *data) {

	*data->QRG='\0';
	*data->mode='\0';
	strcpy(data->comment, "TNX for nice QSO! 73 es DX!");
	data->tx_nr=1;

	return;
}

