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

#include "eqsl_client.h"
#include "band.h"

#define EQSL_URL "https://www.eqsl.cc/qslcard/ImportADIF.cfm"
#define EQSL_TIMEOUT_S 60L

typedef struct {
  char *data;
  size_t len;
} response_buffer_t;


static size_t eqsl_write_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {
  response_buffer_t *buf = (response_buffer_t *)userdata;
  size_t n = size * nmemb;

  char *p = g_realloc(buf->data, buf->len + n + 1);
  buf->data = p;
  memcpy(buf->data + buf->len, ptr, n);
  buf->len += n;
  buf->data[buf->len] = '\0';

  return n;
}


void eqsl_client_init(void) {
  static gsize initialized = 0;

  if (g_once_init_enter(&initialized)) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    g_once_init_leave(&initialized, 1);
  }
}


static void eqsl_add_field(GString *adif, const char *name, const char *value) {
  if (value != NULL && value[0] != '\0') {
    g_string_append_printf(adif, "<%s:%zu>%s\n", name, strlen(value), value);
  }
}


/* Builds a one record ADIF file. Returns NULL if the QSO lacks a field eQSL.cc requires. */
static GString *eqsl_build_adif(const char *qth_nickname, const log_entry_t *entry,
                                const char *super_mode, eqsl_result_t *result) {
  char date[16];
  char time[8];
  char freq[32];
  size_t n = 0;
  int year, month, day;
  const char *band;

  if (entry->call[0] == '\0') {
    snprintf(result->message, EQSL_MSG_LEN, "No callsign");
    return NULL;
  }

  if (entry->mode.name[0] == '\0') {
    snprintf(result->message, EQSL_MSG_LEN, "No mode");
    return NULL;
  }

  if (sscanf(entry->date, "%4d-%2d-%2d", &year, &month, &day) != 3) {
    snprintf(result->message, EQSL_MSG_LEN, "Invalid date '%s'", entry->date);
    return NULL;
  }
  snprintf(date, sizeof(date), "%04d%02d%02d", year, month, day);

  for (const char *p = entry->utc; *p != '\0' && n < 6; p++) {
    if (isdigit((unsigned char)*p)) {
      time[n++] = *p;
    }
  }
  time[n] = '\0';
  if (n != 4 && n != 6) {
    snprintf(result->message, EQSL_MSG_LEN, "Invalid time '%s'", entry->utc);
    return NULL;
  }

  band = band_find(entry->qrg);
  if (strcmp(band, "Unknown") == 0) {
    snprintf(result->message, EQSL_MSG_LEN, "Frequency %.6f MHz is not in a known band", entry->qrg);
    return NULL;
  }

  snprintf(freq, sizeof(freq), "%.6f", entry->qrg);
  for (char *p = freq + strlen(freq) - 1; *p == '0' && *(p - 1) != '.'; p--) {
    *p = '\0';
  }

  GString *adif = g_string_new(NULL);

  eqsl_add_field(adif, "ADIF_VER", "3.0.5");
  eqsl_add_field(adif, "PROGRAMID", PROGRAM_NAME);
  eqsl_add_field(adif, "PROGRAMVERSION", VERSION);
  g_string_append(adif, "<EOH>\n");

  eqsl_add_field(adif, "CALL", entry->call);
  eqsl_add_field(adif, "QSO_DATE", date);
  eqsl_add_field(adif, "TIME_ON", time);
  eqsl_add_field(adif, "BAND", band);
  eqsl_add_field(adif, "FREQ", freq);

  /*llog stores submodes (e.g. USB, "OLIVIA 8/250") as the mode; ADIF wants them as SUBMODE*/
  if (super_mode != NULL && super_mode[0] != '\0' && g_ascii_strcasecmp(super_mode, entry->mode.name) != 0) {
    eqsl_add_field(adif, "MODE", super_mode);
    eqsl_add_field(adif, "SUBMODE", entry->mode.name);
  } else {
    eqsl_add_field(adif, "MODE", entry->mode.name);
  }

  eqsl_add_field(adif, "RST_SENT", entry->txrst);
  eqsl_add_field(adif, "TX_PWR", entry->power);
  eqsl_add_field(adif, "APP_EQSL_QTH_NICKNAME", qth_nickname);
  g_string_append(adif, "<EOR>\n");

  return adif;
}


/* Performs the upload. Returns false on a transport error, with the reason in result->message.
 * On success the caller owns *response (may be NULL) and frees it with g_free().
 */
static bool eqsl_request(const char *user, const char *password, const GString *adif,
                         eqsl_result_t *result, char **response) {
  CURL *curl;
  CURLcode res;
  curl_mime *mime;
  curl_mimepart *part;
  response_buffer_t buf = { NULL, 0 };
  char errbuf[CURL_ERROR_SIZE] = "";
  bool ok = true;

  *response = NULL;

  curl = curl_easy_init();
  if (curl == NULL) {
    snprintf(result->message, EQSL_MSG_LEN, "Could not initialise libcurl");
    return false;
  }

  mime = curl_mime_init(curl);

  part = curl_mime_addpart(mime);
  curl_mime_name(part, "EQSL_USER");
  curl_mime_data(part, user, CURL_ZERO_TERMINATED);

  part = curl_mime_addpart(mime);
  curl_mime_name(part, "EQSL_PSWD");
  curl_mime_data(part, password, CURL_ZERO_TERMINATED);

  /*eQSL.cc refuses uploads without a file name extension*/
  part = curl_mime_addpart(mime);
  curl_mime_name(part, "Filename");
  curl_mime_filename(part, "llog.adi");
  curl_mime_type(part, "text/plain");
  curl_mime_data(part, adif->str, adif->len);

  curl_easy_setopt(curl, CURLOPT_URL, EQSL_URL);
  curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, PROGRAM_NAME "/" VERSION);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, EQSL_TIMEOUT_S);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, eqsl_write_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    snprintf(result->message, EQSL_MSG_LEN, "Network error: %s",
             errbuf[0] != '\0' ? errbuf : curl_easy_strerror(res));
    g_free(buf.data);
    ok = false;
    goto out;
  }

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result->http_status);
  *response = buf.data;

out:
  curl_mime_free(mime);
  curl_easy_cleanup(curl);
  return ok;
}


/* The response is an HTML page with one status line per message, e.g.
 *   Result: 1 out of 1 records added
 *   Warning: Y=2026 M=10 D=04 Bad record: Duplicate
 *   Error: No match on eQSL_User/eQSL_Pswd
 */
static eqsl_status_t eqsl_parse_response(const char *response, eqsl_result_t *result) {
  GString *text = g_string_new(NULL);
  GString *msg = g_string_new(NULL);
  bool in_tag = false;
  bool has_result = false;
  bool duplicate = false;
  bool fatal = false;
  bool rejected = false;
  int added = 0, total = 0;

  /*Drop the markup; every tag ends a line*/
  for (const char *p = response != NULL ? response : ""; *p != '\0'; p++) {
    if (*p == '<') {
      in_tag = true;
      g_string_append_c(text, '\n');
    } else if (*p == '>') {
      in_tag = false;
    } else if (!in_tag) {
      g_string_append_c(text, *p == '\r' ? '\n' : *p);
    }
  }

  gchar **lines = g_strsplit(text->str, "\n", -1);
  for (gchar **l = lines; *l != NULL; l++) {
    char *line = g_strstrip(*l);

    if (g_str_has_prefix(line, "Result:")) {
      has_result = sscanf(line, "Result: %d out of %d", &added, &total) == 2;
      continue;
    }

    if (g_str_has_prefix(line, "Error:")) {
      /*The account doesn't cover this QSO's date; other QSOs may still match*/
      if (strstr(line, " for date ") != NULL) {
        rejected = true;
      } else {
        fatal = true;
      }
    } else if (g_str_has_prefix(line, "Warning:")) {
      if (strstr(line, "Duplicate") != NULL) {
        duplicate = true;
      }
    } else if (g_str_has_prefix(line, "Information:")) {
      /*Only keep notes that tell the user something new, like a mode mapping*/
      if (strstr(line, " was mapped to ") == NULL && strstr(line, "multiple accounts") == NULL) {
        continue;
      }
    } else if (!g_str_has_prefix(line, "Caution:")) {
      continue;
    }

    if (msg->len > 0) {
      g_string_append(msg, " ");
    }
    g_string_append(msg, line);
  }
  g_strfreev(lines);
  g_string_free(text, TRUE);

  eqsl_status_t ret;
  if (result->http_status != 200 || fatal) {
    ret = eqsl_stat_fatal;
  } else if (rejected) {
    ret = eqsl_stat_rejected;
  } else if (has_result && added > 0) {
    ret = eqsl_stat_ok;
  } else if (duplicate) {
    ret = eqsl_stat_duplicate;
  } else if (has_result) {
    ret = eqsl_stat_rejected;
  } else {
    ret = eqsl_stat_fatal;
    if (msg->len == 0) {
      g_string_append(msg, "Unexpected response from eQSL.cc");
    }
  }

  if (result->http_status != 200 && msg->len == 0) {
    g_string_printf(msg, "HTTP %ld", result->http_status);
  }

  snprintf(result->message, EQSL_MSG_LEN, "%s", msg->str);
  g_string_free(msg, TRUE);

  return ret;
}


eqsl_status_t eqsl_upload_qso(const char *user, const char *password, const char *qth_nickname,
                              const log_entry_t *entry, const char *super_mode, eqsl_result_t *result) {
  char *response;
  eqsl_status_t ret;

  memset(result, 0, sizeof(*result));

  GString *adif = eqsl_build_adif(qth_nickname, entry, super_mode, result);
  if (adif == NULL) {
    return eqsl_stat_skipped;
  }

  bool sent = eqsl_request(user, password, adif, result, &response);
  g_string_free(adif, TRUE);

  if (!sent) {
    return eqsl_stat_fatal;
  }

  ret = eqsl_parse_response(response, result);
  g_free(response);

  return ret;
}
