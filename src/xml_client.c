#include <xmlrpc-c/client.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <glib.h>

#include "llog.h"
#include "xml_client.h"

/*FLdigi XML methods*/

#define FLDIGI_XML_METHOD_CONF_DIR "fldigi.config_dir"
#define FLDIGI_XML_METHOD_CONF_DIR "fldigi.config_dir"
#define FLDIGI_XML_METHOD_LIST "fldigi.list"
#define FLDIGI_XML_METHOD_NAME "fldigi.name"
#define FLDIGI_XML_METHOD_NAME_VERSION "fldigi.name_version"
#define FLDIGI_XML_METHOD_TERMINATE "fldigi.terminate"
#define FLDIGI_XML_METHOD_VERSION "fldigi.version"
#define FLDIGI_XML_METHOD_VERSION_STRUCT "fldigi.version_struct"
#define FLDIGI_XML_METHOD_IO_ENABLE_ARQ "io.enable_arq"
#define FLDIGI_XML_METHOD_IO_ENABLE_KISS "io.enable_kiss"
#define FLDIGI_XML_METHOD_IO_IN_USE "io.in_use"
#define FLDIGI_XML_METHOD_LOG_CLEAR "log.clear"
#define FLDIGI_XML_METHOD_LOG_GET_AZ "log.get_az"
#define FLDIGI_XML_METHOD_LOG_GET_BAND "log.get_band"
#define FLDIGI_XML_METHOD_LOG_GET_CALL "log.get_call"
#define FLDIGI_XML_METHOD_LOG_GET_COUNTRY "log.get_country"
#define FLDIGI_XML_METHOD_LOG_GET_EXCHANGE "log.get_exchange"
#define FLDIGI_XML_METHOD_LOG_GET_FREQUENCY "log.get_frequency"
#define FLDIGI_XML_METHOD_LOG_GET_LOCATOR "log.get_locator"
#define FLDIGI_XML_METHOD_LOG_GET_NAME "log.get_name"
#define FLDIGI_XML_METHOD_LOG_GET_NOTES "log.get_notes"
#define FLDIGI_XML_METHOD_LOG_GET_PROVINCE "log.get_province"
#define FLDIGI_XML_METHOD_LOG_GET_QTH "log.get_qth"
#define FLDIGI_XML_METHOD_LOG_GET_RST_IN "log.get_rst_in"
#define FLDIGI_XML_METHOD_LOG_GET_RST_OUT "log.get_rst_out"
#define FLDIGI_XML_METHOD_LOG_GET_SERIAL_NUMBER "log.get_serial_number"
#define FLDIGI_XML_METHOD_LOG_GET_SERIAL_NUMBER_SENT "log.get_serial_number_sent"
#define FLDIGI_XML_METHOD_LOG_GET_STATE "log.get_state"
#define FLDIGI_XML_METHOD_LOG_GET_TIME_OFF "log.get_time_off"
#define FLDIGI_XML_METHOD_LOG_GET_TIME_ON "log.get_time_on"
#define FLDIGI_XML_METHOD_LOG_SET_CALL "log.set_call"
#define FLDIGI_XML_METHOD_LOG_SET_EXCHANGE "log.set_exchange"
#define FLDIGI_XML_METHOD_LOG_SET_LOCATOR "log.set_locator"
#define FLDIGI_XML_METHOD_LOG_SET_NAME "log.set_name"
#define FLDIGI_XML_METHOD_LOG_SET_QTH "log.set_qth"
#define FLDIGI_XML_METHOD_LOG_SET_RST_IN "log.set_rst_in"
#define FLDIGI_XML_METHOD_LOG_SET_RST_OUT "log.set_rst_out"
#define FLDIGI_XML_METHOD_LOG_SET_SERIAL_NUMBER "log.set_serial_number"
#define FLDIGI_XML_METHOD_MAIN_ABORT "main.abort"
#define FLDIGI_XML_METHOD_MAIN_GET_AFC "main.get_afc"
#define FLDIGI_XML_METHOD_MAIN_GET_CHAR_RATES "main.get_char_rates"
#define FLDIGI_XML_METHOD_MAIN_GET_CHAR_TIMING "main.get_char_timing"
#define FLDIGI_XML_METHOD_MAIN_GET_FREQUENCY "main.get_frequency"
#define FLDIGI_XML_METHOD_MAIN_GET_LOCK "main.get_lock"
#define FLDIGI_XML_METHOD_MAIN_GET_MAX_MACRO_ID "main.get_max_macro_id"
#define FLDIGI_XML_METHOD_MAIN_GET_REVERSE "main.get_reverse"
#define FLDIGI_XML_METHOD_MAIN_GET_RSID "main.get_rsid"
#define FLDIGI_XML_METHOD_MAIN_GET_TXID "main.get_txid"
#define FLDIGI_XML_METHOD_MAIN_GET_SQUELCH "main.get_squelch"
#define FLDIGI_XML_METHOD_MAIN_GET_SQUELCH_LEVEL "main.get_squelch_level"
#define FLDIGI_XML_METHOD_MAIN_GET_STATUS1 "main.get_status1"
#define FLDIGI_XML_METHOD_MAIN_GET_STATUS2 "main.get_status2"
#define FLDIGI_XML_METHOD_MAIN_GET_TRX_STATE "main.get_trx_state"
#define FLDIGI_XML_METHOD_MAIN_GET_TRX_STATUS "main.get_trx_status"
#define FLDIGI_XML_METHOD_MAIN_GET_TX_TIMING "main.get_tx_timing"
#define FLDIGI_XML_METHOD_MAIN_GET_WF_SIDEBAND "main.get_wf_sideband"
#define FLDIGI_XML_METHOD_MAIN_INC_FREQUENCY "main.inc_frequency"
#define FLDIGI_XML_METHOD_MAIN_INC_SQUELCH_LEVEL "main.inc_squelch_level"
#define FLDIGI_XML_METHOD_MAIN_RUN_MACRO "main.run_macro"
#define FLDIGI_XML_METHOD_MAIN_RX "main.rx"
#define FLDIGI_XML_METHOD_MAIN_RX_ONLY "main.rx_only"
#define FLDIGI_XML_METHOD_MAIN_RX_TX "main.rx_tx"
#define FLDIGI_XML_METHOD_MAIN_SET_AFC "main.set_afc"
#define FLDIGI_XML_METHOD_MAIN_SET_FREQUENCY "main.set_frequency"
#define FLDIGI_XML_METHOD_MAIN_SET_LOCK "main.set_lock"
#define FLDIGI_XML_METHOD_MAIN_SET_REVERSE "main.set_reverse"
#define FLDIGI_XML_METHOD_MAIN_SET_RSID "main.set_rsid"
#define FLDIGI_XML_METHOD_MAIN_SET_TXID "main.set_txid"
#define FLDIGI_XML_METHOD_MAIN_SET_SQUELCH "main.set_squelch"
#define FLDIGI_XML_METHOD_MAIN_SET_SQUELCH_LEVEL "main.set_squelch_level"
#define FLDIGI_XML_METHOD_MAIN_SET_WF_SIDEBAND "main.set_wf_sideband"
#define FLDIGI_XML_METHOD_MAIN_TOGGLE_AFC "main.toggle_afc"
#define FLDIGI_XML_METHOD_MAIN_TOGGLE_LOCK "main.toggle_lock"
#define FLDIGI_XML_METHOD_MAIN_TOGGLE_REVERSE "main.toggle_reverse"
#define FLDIGI_XML_METHOD_MAIN_TOGGLE_RSID "main.toggle_rsid"
#define FLDIGI_XML_METHOD_MAIN_TOGGLE_TXID "main.toggle_txid"
#define FLDIGI_XML_METHOD_MAIN_TOGGLE_SQUELCH "main.toggle_squelch"
#define FLDIGI_XML_METHOD_MAIN_TUNE "main.tune"
#define FLDIGI_XML_METHOD_MAIN_TX "main.tx"
#define FLDIGI_XML_METHOD_MODEM_GET_AFC_SEARCH_RANGE "modem.get_afc_search_range"
#define FLDIGI_XML_METHOD_MODEM_GET_BANDWIDTH "modem.get_bandwidth"
#define FLDIGI_XML_METHOD_MODEM_GET_CARRIER "modem.get_carrier"
#define FLDIGI_XML_METHOD_MODEM_GET_ID "modem.get_id"
#define FLDIGI_XML_METHOD_MODEM_GET_MAX_ID "modem.get_max_id"
#define FLDIGI_XML_METHOD_MODEM_GET_NAME "modem.get_name"
#define FLDIGI_XML_METHOD_MODEM_GET_NAMES "modem.get_names"
#define FLDIGI_XML_METHOD_MODEM_GET_QUALITY "modem.get_quality"
#define FLDIGI_XML_METHOD_MODEM_INC_AFC_SEARCH_RANGE "modem.inc_afc_search_range"
#define FLDIGI_XML_METHOD_MODEM_INC_BANDWIDTH "modem.inc_bandwidth"
#define FLDIGI_XML_METHOD_MODEM_INC_CARRIER "modem.inc_carrier"
#define FLDIGI_XML_METHOD_MODEM_OLIVIA_GET_BANDWIDTH "modem.olivia.get_bandwidth"
#define FLDIGI_XML_METHOD_MODEM_OLIVIA_GET_TONES "modem.olivia.get_tones"
#define FLDIGI_XML_METHOD_MODEM_OLIVIA_SET_BANDWIDTH "modem.olivia.set_bandwidth"
#define FLDIGI_XML_METHOD_MODEM_OLIVIA_SET_TONES "modem.olivia.set_tones"
#define FLDIGI_XML_METHOD_MODEM_SEARCH_DOWN "modem.search_down"
#define FLDIGI_XML_METHOD_MODEM_SEARCH_UP "modem.search_up"
#define FLDIGI_XML_METHOD_MODEM_SET_AFC_SEARCH_RANGE "modem.set_afc_search_range"
#define FLDIGI_XML_METHOD_MODEM_SET_BANDWIDTH "modem.set_bandwidth"
#define FLDIGI_XML_METHOD_MODEM_SET_BY_ID "modem.set_by_id"
#define FLDIGI_XML_METHOD_MODEM_SET_BY_NAME "modem.set_by_name"
#define FLDIGI_XML_METHOD_MODEM_SET_CARRIER "modem.set_carrier"
#define FLDIGI_XML_METHOD_NAVTEX_GET_MESSAGE "navtex.get_message"
#define FLDIGI_XML_METHOD_NAVTEX_SEND_MESSAGE "navtex.send_message"
#define FLDIGI_XML_METHOD_RIG_GET_BANDWIDTH "rig.get_bandwidth"
#define FLDIGI_XML_METHOD_RIG_GET_BANDWIDTHS "rig.get_bandwidths"
#define FLDIGI_XML_METHOD_RIG_GET_MODE "rig.get_mode"
#define FLDIGI_XML_METHOD_RIG_GET_MODES "rig.get_modes"
#define FLDIGI_XML_METHOD_RIG_GET_NAME "rig.get_name"
#define FLDIGI_XML_METHOD_RIG_GET_NOTCH "rig.get_notch"
#define FLDIGI_XML_METHOD_RIG_RELEASE_CONTROL "rig.release_control"
#define FLDIGI_XML_METHOD_RIG_SET_BANDWIDTH "rig.set_bandwidth"
#define FLDIGI_XML_METHOD_RIG_SET_BANDWIDTHS "rig.set_bandwidths"
#define FLDIGI_XML_METHOD_RIG_SET_FREQUENCY "rig.set_frequency"
#define FLDIGI_XML_METHOD_RIG_SET_MODE "rig.set_mode"
#define FLDIGI_XML_METHOD_RIG_SET_MODES "rig.set_modes"
#define FLDIGI_XML_METHOD_RIG_SET_NAME "rig.set_name"
#define FLDIGI_XML_METHOD_RIG_SET_PWRMETER "rig.set_pwrmeter"
#define FLDIGI_XML_METHOD_RIG_SET_SMETER "rig.set_smeter"
#define FLDIGI_XML_METHOD_RIG_TAKE_CONTROL "rig.take_control"
#define FLDIGI_XML_METHOD_RX_GET_DATA "rx.get_data"
#define FLDIGI_XML_METHOD_RXTX_GET_DATA "rxtx.get_data"
#define FLDIGI_XML_METHOD_SPOT_GET_AUTO "spot.get_auto"
#define FLDIGI_XML_METHOD_SPOT_PSKREP_GET_COUNT "spot.pskrep.get_count"
#define FLDIGI_XML_METHOD_SPOT_SET_AUTO "spot.set_auto"
#define FLDIGI_XML_METHOD_SPOT_TOGGLE_AUTO "spot.toggle_auto"
#define FLDIGI_XML_METHOD_TEXT_ADD_TX "text.add_tx"
#define FLDIGI_XML_METHOD_TEXT_ADD_TX_BYTES "text.add_tx_bytes"
#define FLDIGI_XML_METHOD_TEXT_CLEAR_RX "text.clear_rx"
#define FLDIGI_XML_METHOD_TEXT_CLEAR_TX "text.clear_tx"
#define FLDIGI_XML_METHOD_TEXT_GET_RX "text.get_rx"
#define FLDIGI_XML_METHOD_TEXT_GET_RX_LENGTH "text.get_rx_length"
#define FLDIGI_XML_METHOD_TX_GET_DATA "tx.get_data"
#define FLDIGI_XML_METHOD_WEFAX_END_RECEPTION "wefax.end_reception"
#define FLDIGI_XML_METHOD_WEFAX_GET_RECEIVED_FILE "wefax.get_received_file"
#define FLDIGI_XML_METHOD_WEFAX_SEND_FILE "wefax.send_file"
#define FLDIGI_XML_METHOD_WEFAX_SET_ADIF_LOG "wefax.set_adif_log"
#define FLDIGI_XML_METHOD_WEFAX_SET_MAX_LINES "wefax.set_max_lines"
#define FLDIGI_XML_METHOD_WEFAX_SET_TX_ABORT_FLAG "wefax.set_tx_abort_flag"
#define FLDIGI_XML_METHOD_WEFAX_SKIP_APT "wefax.skip_apt"
#define FLDIGI_XML_METHOD_WEFAX_SKIP_PHASING "wefax.skip_phasing"
#define FLDIGI_XML_METHOD_WEFAX_STATE_STRING "wefax.state_string"


typedef struct xmlrpc_res_s {
  int intval;
  const char    *stringval;
  const unsigned char *byteval;
} xmlrpc_res;

typedef enum {
  xml_client_initailized = 0,
  xml_client_not_initialized
} xml_client_init_state_t;



static xmlrpc_env env;
static char *xmlrpc_url;
xmlrpc_server_info *xml_client_server_info = NULL;
pthread_mutex_t xmlrpc_mutex = PTHREAD_MUTEX_INITIALIZER;
uint32_t xml_client_initialized = xml_client_not_initialized;



static char *xml_client_assemble_url(const char *host, u_int64_t port);
static int xml_client_query(xmlrpc_res *local_result, xmlrpc_env *local_env,
                            char *methodname, char *format, ...);


int xml_client_init(const char *host, u_int64_t port) {
  int ret = xml_client_stat_ok;

  xmlrpc_url = xml_client_assemble_url(host, port);

  printf("Creating connection to URL: %s\n", xmlrpc_url);

  xmlrpc_client_init2(&env, XMLRPC_CLIENT_NO_FLAGS, PROGRAM_NAME, VERSION, NULL, 0);

  if (env.fault_occurred != 0) {
    printf("Error initializing xml client\n");
    ret = xml_client_stat_err;
    goto out;
  }

  xml_client_server_info = xmlrpc_server_info_new(&env, xmlrpc_url);
  if (env.fault_occurred != 0) {
    xml_client_server_info = NULL;
    xml_client_initialized = xml_client_not_initialized;
    ret = xml_client_stat_err;
  }

  xml_client_initialized = xml_client_initailized;

out:
  pthread_mutex_unlock(&xmlrpc_mutex);
  return ret;
}


int xml_client_fldigi_get_frequency(double *frequency) {
  xmlrpc_res result;
  int ret;
  xmlrpc_env env;

  ret = xml_client_query(&result, &env, FLDIGI_XML_METHOD_LOG_GET_FREQUENCY, "");

  if (ret == xml_client_stat_ok) {
    *frequency = strtod(result.stringval, NULL) / 1000;
    printf("Frequency: %f\n", *frequency);
  }

  return ret;
}

int xml_client_fldigi_get_call(char *call) {
  xmlrpc_res result;
  int ret;
  xmlrpc_env env;
  char *up_result;

  ret = xml_client_query(&result, &env, FLDIGI_XML_METHOD_LOG_GET_CALL, "");

  if (ret == xml_client_stat_ok) {
    strncpy(call, result.stringval, CALL_LEN);
    up_result = g_utf8_strup(result.stringval, -1);
    strcpy(call, up_result);
    printf("Call: %s\n", call);
    g_free(up_result);
  }

  return ret;
}

int xml_client_fldigi_get_name(char *name) {
  xmlrpc_res result;
  int ret;
  xmlrpc_env env;

  ret = xml_client_query(&result, &env, FLDIGI_XML_METHOD_LOG_GET_NAME, "");

  if (ret == xml_client_stat_ok) {
    strncpy(name, result.stringval, NAME_LEN);
    printf("Name: %s\n", name);
  }

  return ret;
}

int xml_client_fldigi_get_rxrst(char *rxrst) {
  xmlrpc_res result;
  int ret;
  xmlrpc_env env;

  ret = xml_client_query(&result, &env, FLDIGI_XML_METHOD_LOG_GET_RST_IN, "");

  if (ret == xml_client_stat_ok) {
    strncpy(rxrst, result.stringval, RST_LEN);
    printf("RXRST: %s\n", rxrst);
  }

  return ret;
}


int xml_client_fldigi_get_txrst(char *txrst) {
  xmlrpc_res result;
  int ret;
  xmlrpc_env env;

  ret = xml_client_query(&result, &env, FLDIGI_XML_METHOD_LOG_GET_RST_OUT, "");

  if (ret == xml_client_stat_ok) {
    strncpy(txrst, result.stringval, RST_LEN);
    printf("TXRST: %s\n", txrst);
  }

  return ret;
}


int xml_client_fldigi_get_qth(char *qth) {
  xmlrpc_res result;
  int ret;
  xmlrpc_env env;

  ret = xml_client_query(&result, &env, FLDIGI_XML_METHOD_LOG_GET_QTH, "");

  if (ret == xml_client_stat_ok) {
    strncpy(qth, result.stringval, QTH_LEN);
    printf("QTH: %s\n", qth);
  }

  return ret;
}


int xml_client_fldigi_get_qra(char *qra) {
  xmlrpc_res result;
  int ret;
  xmlrpc_env env;

  ret = xml_client_query(&result, &env, FLDIGI_XML_METHOD_LOG_GET_LOCATOR, "");

  if (ret == xml_client_stat_ok) {
    strncpy(qra, result.stringval, QRA_LEN);
    printf("QRA: %s\n", qra);
  }

  return ret;
}


int xml_client_fldigi_get_utc(char *utc) {
  xmlrpc_res result;
  int ret;
  xmlrpc_env env;

  ret = xml_client_query(&result, &env, FLDIGI_XML_METHOD_LOG_GET_TIME_ON, "");

  if (ret == xml_client_stat_ok) {
    strncpy(utc, result.stringval, NAME_LEN);
    printf("UTC: %s\n", utc);
  }

  return ret;
}

void xml_client_shutdown(void) {
  printf("Shutting down xml client\n");
  xmlrpc_server_info_free(xml_client_server_info);
  xmlrpc_client_cleanup();
  xmlrpc_env_clean(&env);
  free(xmlrpc_url);
}


static char *xml_client_assemble_url(const char *host, u_int64_t port) {
  size_t url_size = strlen(host) + 20; // 20 to accommodate port and extra characters
  char *url = malloc(url_size);

  if (url == NULL) {
    return NULL;
  }
  snprintf(url, url_size, "http://%s:%lu/RPC2", host, port);
  return url;
}


static int xml_client_query(xmlrpc_res *local_result, xmlrpc_env *local_env,
                            char *methodname, char *format, ...) {
  xmlrpc_value *callresult;
  xmlrpc_value *pcall_array = NULL;
  xmlrpc_value *va_param = NULL;
  va_list argptr;
  int restype;
  size_t bytesize = 0;
  int ret = xml_client_stat_ok;


  pthread_mutex_lock(&xmlrpc_mutex);

  if (xml_client_initialized != xml_client_initailized) {
    printf("XML client not initialized\n");
    ret = xml_client_stat_err;
    goto out;
  }

  local_result->stringval = NULL;
  local_result->byteval = NULL;


  va_start(argptr, format);
  xmlrpc_env_init(local_env);
  pcall_array = xmlrpc_array_new(local_env);
  while (*format != '\0') {
    if (*format == 's') {
      char *s = va_arg(argptr, char *);
      va_param = xmlrpc_string_new(local_env, s);
      if (local_env->fault_occurred) {
        va_end(argptr);
        ret = xml_client_stat_err;
        goto out;
      }
      xmlrpc_array_append_item(local_env, pcall_array, va_param);
      if (local_env->fault_occurred) {
        va_end(argptr);
        ret = xml_client_stat_err;
        goto out;
      }
      xmlrpc_DECREF(va_param);
    } else if (*format == 'd') {
      int d = va_arg(argptr, int);
      va_param = xmlrpc_int_new(local_env, d);
      xmlrpc_array_append_item(local_env, pcall_array, va_param);
      if (local_env->fault_occurred) {
        va_end(argptr);
        ret = xml_client_stat_err;
        goto out;
      }
      xmlrpc_DECREF(va_param);
    } else if (*format == 'f') {
      double f = va_arg(argptr, double);
      va_param = xmlrpc_double_new(local_env, f);
      xmlrpc_array_append_item(local_env, pcall_array, va_param);
      if (local_env->fault_occurred) {
        va_end(argptr);
        ret = xml_client_stat_err;
        goto out;
      }
      xmlrpc_DECREF(va_param);
    }
    format++;
  }

  va_end(argptr);

  callresult = xmlrpc_client_call_server_params(local_env, xml_client_server_info,
                                                methodname, pcall_array);
  if (local_env->fault_occurred) {
    // error till xmlrpc_call
    xmlrpc_DECREF(pcall_array);
    xmlrpc_env_clean(local_env);
    ret = xml_client_stat_err;
    goto out;
  }

  restype = xmlrpc_value_type(callresult);

  local_result->intval = 0;

  switch (restype) {
  case XMLRPC_TYPE_DEAD:
    xmlrpc_DECREF(callresult);
    xmlrpc_DECREF(pcall_array);
    xmlrpc_env_clean(local_env);
    ret = xml_client_stat_err;
    goto out;
    break;

  // int
  case XMLRPC_TYPE_INT:
    xmlrpc_read_int(local_env, callresult,
                    &local_result->intval);
    break;

  // string
  case XMLRPC_TYPE_STRING:
    xmlrpc_read_string(local_env, callresult,
                       &local_result->stringval);
    break;

  // byte stream
  case XMLRPC_TYPE_BASE64:
    xmlrpc_read_base64(local_env, callresult,
                       &bytesize, &local_result->byteval);
    local_result->intval = (int)bytesize;
    break;
  }

  xmlrpc_DECREF(callresult);
  xmlrpc_DECREF(pcall_array);


out:
  pthread_mutex_unlock(&xmlrpc_mutex);
  return ret;
}
