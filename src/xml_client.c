#include <xmlrpc-c/client.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "llog.h"
#include "xml_client.h"


static xmlrpc_env env;
static char *xmlrpc_url;


static char *xml_cli_assemble_url(const char *host, u_int64_t port);



int xml_cli_init(const char *host, u_int64_t port) {
  xmlrpc_env_init(&env);

  xmlrpc_url = xml_cli_assemble_url(host, port);

  printf("Creating connection to URL: %s\n", xmlrpc_url);

  xmlrpc_client_init2(&env, XMLRPC_CLIENT_NO_FLAGS, PROGRAM_NAME, VERSION, NULL, 0);

  if (env.fault_occurred) {
    printf("Error initializing xml client\n");
  }


  return 0;
}


int xml_cli_fldigi_get_frequency(double *frequency) {
  xml_client_result_t result;
  int ret;

  ret = xml_cli_get_call(FLDIGI_XML_METHOD_MAIN_GET_FREQUENCY, "", &result);

  if (ret == xml_client_stat_ok) {
    *frequency = result.result.result_d;
  }

  return ret;
}


int xml_cli_get_call(const char *method_name, char *params, xml_client_result_t *result) {
  xmlrpc_value *response;
  xmlrpc_type response_type;
  int ret;

  ret = xml_client_stat_ok;

  response = xmlrpc_client_call(&env, xmlrpc_url, method_name, params);

  if (env.fault_occurred) {
    ret = xml_client_stat_err;
    goto out;
  }

  response_type = xmlrpc_value_type(response);


  switch (response_type) {
  case XMLRPC_TYPE_DEAD:
    goto out;
    break;

  case XMLRPC_TYPE_INT:
    xmlrpc_parse_value(&env, response, "i", result->result.result_i);
    result->result_type = xml_client_result_type_int;
    break;

  case XMLRPC_TYPE_DOUBLE:
    xmlrpc_parse_value(&env, response, "d", result->result.result_d);
    result->result_type = xml_client_result_type_double;
    break;

  case XMLRPC_TYPE_STRING:
    xmlrpc_parse_value(&env, response, "s", result->result.result_s);
    result->result_type = xml_client_result_type_string;
    break;
  case XMLRPC_TYPE_BASE64:
    break;
  default:
    break;
  }

out:
  return ret;
}


void xml_cli_close(void) {
  printf("Shutting down xml client\n");
  xmlrpc_client_cleanup();
  xmlrpc_env_clean(&env);
  free(xmlrpc_url);
}


static char *xml_cli_assemble_url(const char *host, u_int64_t port) {
  size_t url_size = strlen(host) + 20; // 20 to accommodate port and extra characters
  char *url = malloc(url_size);

  if (url == NULL) {
    return NULL;
  }
  snprintf(url, url_size, "http://%s:%lu/RPC2", host, port);
  return url;
}
