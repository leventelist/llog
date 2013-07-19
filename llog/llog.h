#ifndef LLOG_H
#define LLOG_H

#include <stdint.h>

#define CONFIG_FILE_NAME "llog.conf"
#define LOGFILE_LEN 100
#define QTH_LEN 100
#define QRA_LEN 20
#define RIG_LEN 50
#define ANT_LEN 20
#define RST_LEN 10
#define CALL_LEN 40
#define LIST_LEN 50
#define LINE_LEN 1024
#define SUBSTR_LEN 512
#define CSV_LIST_LEN 3

#define OK 0
#define FILE_ERR 1
#define CMD_LINE_ERR 2

#define CANCEL_SEQ ":c"
#define PROMPT ": "

/*CSV field positions*/
#define CSV_DATE_POS 0
#define CSV_TIME_POS 1
#define CSV_CALL_POS 2

typedef struct {

	char call[CALL_LEN];
	char rxrst[RST_LEN];
	char txrst[RST_LEN];
	char QTH[QTH_LEN];
	char name[100];
	char comment[200];
	char default_rst[RST_LEN];
	char QRA[QRA_LEN];
	char QRG[20];
	char mode[20];
	uint32_t tx_nr;
	uint32_t rx_nr;

} llog_t;

void reset_values(llog_t *data);
void reset_values_static(llog_t *data);

#endif

