#include <stdint.h>

typedef struct {

	char call[40];
	char rxrst[10];
	char txrst[10];
	char QTH[100];
	char name[100];
	char comment[200];
	char default_rst[10];
	char QRA[20];
	char QRG[20];
	char mode[20];
	uint32_t tx_nr;
	uint32_t rx_nr;

} llog_t;

void reset_values(llog_t *data);
void reset_values_static(llog_t *data);


