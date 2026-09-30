/*	This is llog, a minimalist HAM logging software.
 *	Copyright (C) 2013-2026  Levente Kovacs
 *
 *	This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * http://levente.logonex.eu
 * ha5ogl.levente@gmail.com
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>
#include <curl/curl.h>
#include <json-c/json.h>

#include "wrl_client.h"
#include "band.h"

#define WRL_BASE_URL "https://api.worldradioleague.com"
#define WRL_TIMEOUT_S 30L
#define WRL_NOTES_MAX 2000   /*maxLength of `notes` in the API*/
#define WRL_GRID_MAX 8       /*The API accepts 2, 4, 6 or 8 character locators*/
#define HEADER_LEN 512

typedef struct {
  char *data;
  size_t len;
} response_buffer_t;


static size_t wrl_write_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {
  response_buffer_t *buf = (response_buffer_t *)userdata;
  size_t n = size * nmemb;

  char *p = g_realloc(buf->data, buf->len + n + 1);
  buf->data = p;
  memcpy(buf->data + buf->len, ptr, n);
  buf->len += n;
  buf->data[buf->len] = '\0';

  return n;
}


void wrl_client_init(void) {
  static gsize initialized = 0;

  if (g_once_init_enter(&initialized)) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    g_once_init_leave(&initialized, 1);
  }
}


/* Performs one HTTP request. Returns false on a transport error, with the reason in result->message.
 * On success the caller owns *response (may be NULL) and frees it with g_free().
 */
static bool wrl_request(const char *method, const char *path, const char *api_key,
                        const char *body, wrl_result_t *result, char **response) {
  CURL *curl;
  CURLcode res;
  struct curl_slist *headers = NULL;
  response_buffer_t buf = { NULL, 0 };
  char url[HEADER_LEN];
  char header[HEADER_LEN];
  char errbuf[CURL_ERROR_SIZE] = "";
  curl_off_t retry_after = 0;
  bool ok = true;

  *response = NULL;

  curl = curl_easy_init();
  if (curl == NULL) {
    snprintf(result->message, WRL_MSG_LEN, "Could not initialise libcurl");
    return false;
  }

  snprintf(url, sizeof(url), "%s%s", WRL_BASE_URL, path);
  snprintf(header, sizeof(header), "Authorization: Bearer %s", api_key);
  headers = curl_slist_append(headers, header);
  headers = curl_slist_append(headers, "Accept: application/json");
  if (body != NULL) {
    headers = curl_slist_append(headers, "Content-Type: application/json");
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, PROGRAM_NAME "/" VERSION);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, WRL_TIMEOUT_S);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, wrl_write_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
  if (body != NULL) {
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
  }

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    snprintf(result->message, WRL_MSG_LEN, "Network error: %s",
             errbuf[0] != '\0' ? errbuf : curl_easy_strerror(res));
    g_free(buf.data);
    ok = false;
    goto out;
  }

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result->http_status);
  curl_easy_getinfo(curl, CURLINFO_RETRY_AFTER, &retry_after);
  result->retry_after = (long)retry_after;
  *response = buf.data;

out:
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  return ok;
}


static const char *wrl_json_get_string(json_object *obj, const char *key) {
  json_object *value;

  if (obj == NULL || !json_object_object_get_ex(obj, key, &value)) {
    return NULL;
  }
  if (!json_object_is_type(value, json_type_string)) {
    return NULL;
  }

  return json_object_get_string(value);
}


/* Parses the response envelope. Returns the root object (caller puts it) and sets *data to its
 * `data` member. Fills result->code and result->message from `error`, if there is one.
 */
static json_object *wrl_parse_envelope(const char *response, json_object **data, wrl_result_t *result) {
  json_object *root;
  json_object *error;

  *data = NULL;

  if (response == NULL) {
    return NULL;
  }

  root = json_tokener_parse(response);
  if (root == NULL) {
    snprintf(result->message, WRL_MSG_LEN, "HTTP %ld, unparseable response", result->http_status);
    return NULL;
  }

  json_object_object_get_ex(root, "data", data);

  if (json_object_object_get_ex(root, "error", &error) && json_object_is_type(error, json_type_object)) {
    const char *code = wrl_json_get_string(error, "code");
    const char *message = wrl_json_get_string(error, "message");
    const char *field = wrl_json_get_string(error, "field");
    const char *hint = wrl_json_get_string(error, "hint");

    snprintf(result->code, WRL_CODE_LEN, "%s", code != NULL ? code : "");
    snprintf(result->field, WRL_CODE_LEN, "%s", field != NULL ? field : "");
    snprintf(result->message, WRL_MSG_LEN, "%s%s%s%s%s%s",
             message != NULL ? message : "Unknown error",
             field != NULL ? " (field: " : "", field != NULL ? field : "", field != NULL ? ")" : "",
             hint != NULL ? " " : "", hint != NULL ? hint : "");
  }

  return root;
}


/* Maps the HTTP status and error code of a failed request onto what the uploader should do. */
static wrl_status_t wrl_classify_error(wrl_result_t *result) {
  long http = result->http_status;

  if (http == 429) {
    if (result->retry_after <= 0) {
      result->retry_after = 60;
    }
    return wrl_stat_rate_limited;
  }

  /*Problems with the key, the account or the logbook affect every contact*/
  if (http == 401 || http == 403 || http == 404 || http == 409 || http >= 500 ||
      strcmp(result->code, "LOGBOOK_REQUIRED") == 0) {
    return wrl_stat_fatal;
  }

  return wrl_stat_rejected;
}


wrl_status_t wrl_check_key(const char *api_key, wrl_result_t *result) {
  char *response;
  json_object *root, *data, *obj;
  GString *msg;
  wrl_status_t ret;

  memset(result, 0, sizeof(*result));

  if (!wrl_request("GET", "/v1/me", api_key, NULL, result, &response)) {
    return wrl_stat_fatal;
  }

  root = wrl_parse_envelope(response, &data, result);
  g_free(response);

  if (result->http_status != 200) {
    ret = wrl_classify_error(result);
    if (result->message[0] == '\0') {
      snprintf(result->message, WRL_MSG_LEN, "HTTP %ld", result->http_status);
    }
    json_object_put(root);
    return ret == wrl_stat_rate_limited ? ret : wrl_stat_fatal;
  }

  msg = g_string_new("API key OK.");

  const char *tier = wrl_json_get_string(data, "membershipTier");
  const char *env = wrl_json_get_string(data, "environment");
  if (tier != NULL || env != NULL) {
    g_string_append_printf(msg, " Membership: %s, environment: %s.",
                           tier != NULL ? tier : "?", env != NULL ? env : "?");
  }

  if (json_object_object_get_ex(data, "defaultLogbook", &obj)) {
    const char *id = wrl_json_get_string(obj, "logbookId");
    const char *resolution = wrl_json_get_string(obj, "resolution");
    if (id != NULL) {
      snprintf(result->logbook_id, WRL_ID_LEN, "%s", id);
      g_string_append_printf(msg, "\nDefault logbook: %s (%s).", id, resolution != NULL ? resolution : "?");
    } else {
      g_string_append_printf(msg, "\nNo default logbook (%s): set a logbook ID below, "
                             "or choose a default in World Radio League.",
                             resolution != NULL ? resolution : "none");
    }
  }
  json_object_put(root);

  /*List the logbooks so the user can pick an ID. Failing here doesn't make the key bad.*/
  wrl_result_t lb_result;
  memset(&lb_result, 0, sizeof(lb_result));
  if (wrl_request("GET", "/v1/logbooks", api_key, NULL, &lb_result, &response)) {
    root = wrl_parse_envelope(response, &data, &lb_result);
    g_free(response);
    if (lb_result.http_status == 200 && json_object_is_type(data, json_type_array)) {
      size_t n = json_object_array_length(data);
      g_string_append_printf(msg, "\nLogbooks (%zu):", n);
      for (size_t i = 0; i < n; i++) {
        json_object *lb = json_object_array_get_idx(data, i);
        const char *id = wrl_json_get_string(lb, "id");
        const char *name = wrl_json_get_string(lb, "name");
        g_string_append_printf(msg, "\n  %s  %s", id != NULL ? id : "?", name != NULL ? name : "");
      }
    }
    json_object_put(root);
  }

  snprintf(result->message, WRL_MSG_LEN, "%s", msg->str);
  g_string_free(msg, TRUE);

  return wrl_stat_ok;
}


/* 2, 4, 6 or 8 character Maidenhead locator, as the API validates it. */
static bool wrl_valid_grid(const char *grid) {
  size_t len = strlen(grid);

  if (len != 2 && len != 4 && len != 6 && len != 8) {
    return false;
  }

  for (size_t i = 0; i < len; i++) {
    char c = toupper((unsigned char)grid[i]);
    switch (i) {
    case 0: case 1:
      if (c < 'A' || c > 'R') return false;
      break;
    case 4: case 5:
      if (c < 'A' || c > 'X') return false;
      break;
    default:
      if (!isdigit((unsigned char)c)) return false;
      break;
    }
  }

  return true;
}


static void wrl_add_string(json_object *obj, const char *key, const char *value) {
  if (value != NULL && value[0] != '\0') {
    json_object_object_add(obj, key, json_object_new_string(value));
  }
}


/* Adds a locator, cutting 10 character locators down to the 8 the API accepts.
 * Returns false if the locator is present but invalid; it is then left out.
 */
static bool wrl_add_grid(json_object *obj, const char *key, const char *value) {
  char grid[WRL_GRID_MAX + 1];

  if (value == NULL || value[0] == '\0') {
    return true;
  }

  snprintf(grid, sizeof(grid), "%s", value);
  if (!wrl_valid_grid(grid)) {
    return false;
  }

  json_object_object_add(obj, key, json_object_new_string(grid));
  return true;
}


static void wrl_add_activity(json_object *array, const char *type, const char *ref) {
  if (ref[0] == '\0') {
    return;
  }

  json_object *activity = json_object_new_object();
  json_object_object_add(activity, "type", json_object_new_string(type));
  json_object_object_add(activity, "ref", json_object_new_string(ref));
  json_object_array_add(array, activity);
}


static void wrl_add_activities(json_object *obj, const char *key,
                               const char *sota, const char *pota, const char *wwff) {
  json_object *array = json_object_new_array();

  wrl_add_activity(array, "SOTA", sota);
  wrl_add_activity(array, "POTA", pota);
  wrl_add_activity(array, "WWFF", wwff);

  if (json_object_array_length(array) > 0) {
    json_object_object_add(obj, key, array);
  } else {
    json_object_put(array);
  }
}


/* Builds the ISO-8601 UTC timestamp from llog's YYYY-MM-DD date and HHMM[SS] time. */
static bool wrl_make_timestamp(const log_entry_t *entry, char *out, size_t len) {
  char digits[7] = "";
  size_t n = 0;
  int year, month, day;

  if (sscanf(entry->date, "%4d-%2d-%2d", &year, &month, &day) != 3) {
    return false;
  }

  for (const char *p = entry->utc; *p != '\0' && n < 6; p++) {
    if (isdigit((unsigned char)*p)) {
      digits[n++] = *p;
    }
  }

  if (n != 4 && n != 6) {
    return false;
  }

  snprintf(out, len, "%04d-%02d-%02dT%.2s:%.2s:%.2sZ",
           year, month, day, digits, digits + 2, n == 6 ? digits + 4 : "00");
  return true;
}


static json_object *wrl_build_contact(const char *logbook_id, const log_entry_t *entry, const char *mode,
                                      const station_entry_t *station, wrl_result_t *result) {
  char timestamp[32];
  char freq[32];
  const char *band;
  json_object *obj;

  if (entry->call[0] == '\0') {
    snprintf(result->message, WRL_MSG_LEN, "No callsign");
    return NULL;
  }

  if (mode[0] == '\0') {
    snprintf(result->message, WRL_MSG_LEN, "No mode");
    return NULL;
  }

  if (!wrl_make_timestamp(entry, timestamp, sizeof(timestamp))) {
    snprintf(result->message, WRL_MSG_LEN, "Invalid date/time '%s %s'", entry->date, entry->utc);
    return NULL;
  }

  band = band_find(entry->qrg);
  if (strcmp(band, "Unknown") == 0) {
    snprintf(result->message, WRL_MSG_LEN, "Frequency %.6f MHz is not in a known band", entry->qrg);
    return NULL;
  }
  if (strcmp(band, "2190m") == 0) {
    band = "2200m";   /*WRL names this band after the ITU designation*/
  }

  /*Serialise the frequency ourselves; json-c would print 14.074 as 14.074000000000000*/
  snprintf(freq, sizeof(freq), "%.6f", entry->qrg);
  for (char *p = freq + strlen(freq) - 1; *p == '0' && *(p - 1) != '.'; p--) {
    *p = '\0';
  }

  obj = json_object_new_object();

  json_object_object_add(obj, "programId", json_object_new_string(PROGRAM_NAME));
  wrl_add_string(obj, "logbookId", logbook_id);
  json_object_object_add(obj, "call", json_object_new_string(entry->call));
  json_object_object_add(obj, "timestamp", json_object_new_string(timestamp));
  json_object_object_add(obj, "freq", json_object_new_double_s(entry->qrg, freq));
  json_object_object_add(obj, "band", json_object_new_string(band));
  json_object_object_add(obj, "mode", json_object_new_string(mode));

  wrl_add_string(obj, "rstSent", entry->txrst);
  wrl_add_string(obj, "rstRcvd", entry->rxrst);
  wrl_add_string(obj, "txPwr", entry->power);
  wrl_add_string(obj, "name", entry->name);
  wrl_add_string(obj, "qth", entry->qth);

  if (entry->comment[0] != '\0') {
    if (g_utf8_validate(entry->comment, -1, NULL) && g_utf8_strlen(entry->comment, -1) > WRL_NOTES_MAX) {
      gchar *notes = g_utf8_substring(entry->comment, 0, WRL_NOTES_MAX);
      json_object_object_add(obj, "notes", json_object_new_string(notes));
      g_free(notes);
    } else {
      json_object_object_add(obj, "notes", json_object_new_string(entry->comment));
    }
  }

  /*The default station in a new log is called NOCALL; let WRL use the account callsign instead*/
  if (station->call[0] != '\0' && g_ascii_strcasecmp(station->call, "NOCALL") != 0) {
    json_object_object_add(obj, "stationCallsign", json_object_new_string(station->call));
  }

  if (!wrl_add_grid(obj, "gridsquare", entry->qra)) {
    g_strlcat(result->message, "Invalid locator left out. ", WRL_MSG_LEN);
  }
  if (!wrl_add_grid(obj, "myGridsquare", station->QRA)) {
    g_strlcat(result->message, "Invalid station locator left out. ", WRL_MSG_LEN);
  }

  wrl_add_activities(obj, "myActivities", entry->sota_ref, entry->pota_ref, entry->wwff_ref);
  wrl_add_activities(obj, "theirActivities", entry->s2s_ref, entry->p2p_ref, entry->w2w_ref);

  return obj;
}


wrl_status_t wrl_post_contact(const char *api_key, const char *logbook_id,
                              const log_entry_t *entry, const char *mode, const station_entry_t *station,
                              wrl_result_t *result) {
  json_object *contact, *root, *data, *meta, *warnings;
  char *response;
  char local_warnings[WRL_MSG_LEN];
  wrl_status_t ret;

  memset(result, 0, sizeof(*result));

  contact = wrl_build_contact(logbook_id, entry, mode, station, result);
  if (contact == NULL) {
    return wrl_stat_skipped;
  }

  /*Warnings found while building the payload; the request may overwrite result->message*/
  snprintf(local_warnings, sizeof(local_warnings), "%s", result->message);
  result->message[0] = '\0';

  bool sent = wrl_request("POST", "/v1/contacts", api_key,
                          json_object_to_json_string_ext(contact, JSON_C_TO_STRING_PLAIN | JSON_C_TO_STRING_NOSLASHESCAPE),
                          result, &response);
  json_object_put(contact);

  if (!sent) {
    return wrl_stat_fatal;
  }

  root = wrl_parse_envelope(response, &data, result);
  g_free(response);

  if (result->http_status != 201 && result->http_status != 200) {
    ret = wrl_classify_error(result);
    if (result->message[0] == '\0') {
      snprintf(result->message, WRL_MSG_LEN, "HTTP %ld", result->http_status);
    }
    json_object_put(root);
    return ret;
  }

  const char *id = wrl_json_get_string(data, "id");
  snprintf(result->id, WRL_ID_LEN, "%s", id != NULL ? id : "");

  /*Collect our own warnings and the server's meta.warnings into the message*/
  GString *msg = g_string_new(local_warnings);
  if (json_object_object_get_ex(root, "meta", &meta) &&
      json_object_object_get_ex(meta, "warnings", &warnings) &&
      json_object_is_type(warnings, json_type_array)) {
    for (size_t i = 0; i < json_object_array_length(warnings); i++) {
      json_object *w = json_object_array_get_idx(warnings, i);
      const char *text = json_object_is_type(w, json_type_string) ? json_object_get_string(w) :
                         wrl_json_get_string(w, "message");
      if (text == NULL) {
        text = json_object_to_json_string(w);
      }
      g_string_append_printf(msg, "%s ", text);
    }
  }
  snprintf(result->message, WRL_MSG_LEN, "%s", msg->str);
  g_string_free(msg, TRUE);

  json_object_put(root);
  return wrl_stat_ok;
}
