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

#ifndef LLOG_H
#define LLOG_H

#include <stdint.h>

#define CONFIG_FILE_NAME "llog.conf"
#define LOGFILE_LEN 100
#define QTH_LEN 100
#define QRA_LEN 20
#define QRG_LEN 20
#define MODE_LEN 20
#define RIG_LEN 50
#define ANT_LEN 20
#define RST_LEN 10
#define CALL_LEN 40
#define NAME_LEN 100
#define COMMENT_LEN 200
#define QSL_LEN 10
#define ALT_LEN 20
#define PWR_LEN 20
#define LIST_LEN 50
#define LINE_LEN 1024
#define SUBSTR_LEN 512

#define OK 0
#define FILE_ERR 1
#define CMD_LINE_ERR 2

#define CANCEL_SEQ ":c"
#define PROMPT ": "

/*CSV stuff*/
#define CSV_LIST_LEN 21
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
#define CSV_QSL_POS 13
#define CSV_LOCAL_CALL_POS 14
#define CSV_LOCAL_QTH_POS 15
#define CSV_LOCAL_QRA_POS 16
#define CSV_LOCAL_ALT_POS 17
#define CSV_LOCAL_RIG_POS 18
#define CSV_LOCAL_PWR_POS 19
#define CSV_LOCAL_ANT_POS 20

typedef struct {

	char call[CALL_LEN];
	char rxrst[RST_LEN];
	char txrst[RST_LEN];
	char QTH[QTH_LEN];
	char name[NAME_LEN];
	char comment[COMMENT_LEN];
	char default_rst[RST_LEN];
	char QRA[QRA_LEN];
	char QRG[QRG_LEN];
	char mode[MODE_LEN];
	char pwr[PWR_LEN];
	uint32_t tx_nr;
	uint32_t rx_nr;

} llog_t;

void reset_values(llog_t *data);
void reset_values_static(llog_t *data);

#endif

