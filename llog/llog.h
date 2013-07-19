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

#define OK 0
#define FILE_ERR 1
#define CMD_LINE_ERR 2

#define CANCEL_SEQ ":c"
#define PROMPT ": "

/*CSV stuff*/
#define CSV_LIST_LEN 17
/*CSV field positions*/
#define CSV_DATE_POS 0
#define CSV_TIME_POS 1
#define CSV_CALL_POS 2
#define CSV_TXRST_POS 3
#define CSV_RXRST_POS 4
#define CSV_QTH_POS 5
#define CSV_QRA_POS 6
#define CSV_NAME_POS 7
#define CSV_QRG_POS 8
#define CSV_MODE_POS 9
#define CSV_RXNR_POS 10
#define CSV_TXNR_POS 11
#define CSV_COMMENT_POS 12
#define CSV_LOCAL_QTH_POS 13
#define CSV_LOCAL_QRA_POS 14
#define CSV_LOCAL_RIG_POS 15
#define CSV_LOCAL_ANT_POS 16

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

