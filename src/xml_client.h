
typedef enum {
  xml_client_stat_ok = 0,
  xml_client_stat_err
} xml_client_error_t;

enum xml_client_result_type {
  xml_client_result_type_double = 0,
  xml_client_result_type_int,
  xml_client_result_type_string
};

typedef struct {
  uint64_t result_type;
  union {
    double result_d;
    int64_t result_i;
    char *result_s;
  } result;
} xml_client_result_t;


int xml_client_init(const char *host, u_int64_t port);
int xml_client_fldigi_get_frequency(double *frequency);
int xml_client_fldigi_get_call(char *call);
int xml_client_fldigi_get_name(char *name);
int xml_client_fldigi_get_rxrst(char *rxrst);
int xml_client_fldigi_get_txrst(char *txrst);
int xml_client_fldigi_get_qth(char *qth);
int xml_client_fldigi_get_qra(char *qra);
int xml_client_fldigi_get_utc(char *utc);
void xml_client_shutdown(void);
