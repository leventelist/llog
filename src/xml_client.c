#include <xmlrpc-c/client.h>


#define SERVER_URL "http://localhost:7362/RPC2"
#define METHOD_NAME "rig.get_frequency"


int xml_cli_init(xmlrpc_env *env, xmlrpc_client **clientP) {
  xmlrpc_client *client;

  xmlrpc_env_init(env);

  xmlrpc_client_init2(env, XMLRPC_CLIENT_NO_FLAGS, clientName, clientVersion, NULL, 0);

  client = xmlrpc_client_new(env, 0, NULL);
  if (env->fault_occurred) {
    return -1;
  }

  xmlrpc_client_set_server_url(env, client, SERVER_URL);
  if (env->fault_occurred) {
    return -1;
  }

  *clientP = client;
  return 0;
}