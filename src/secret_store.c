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
#include <glib.h>

#include "secret_store.h"

#ifdef HAVE_LIBSECRET

#include <libsecret/secret.h>

#define SECRET_LABEL_LEN 128

static const SecretSchema llog_secret_schema = {
  .name = "eu.logonex.llog.Secret",
  .flags = SECRET_SCHEMA_NONE,
  .attributes = {
    { "key", SECRET_SCHEMA_ATTRIBUTE_STRING },
    { NULL, 0 },
  },
};


bool secret_store_load(const char *key, char *value, size_t len) {
  GError *error = NULL;
  gchar *secret;

  secret = secret_password_lookup_sync(&llog_secret_schema, NULL, &error, "key", key, NULL);
  if (error != NULL) {
    fprintf(stderr, "Keyring lookup of `%s` failed: %s\n", key, error->message);
    g_error_free(error);
    return false;
  }

  if (secret == NULL) {
    value[0] = '\0';
  } else {
    g_strlcpy(value, secret, len);
    secret_password_free(secret);
  }
  return true;
}


bool secret_store_save(const char *key, const char *value) {
  GError *error = NULL;
  char label[SECRET_LABEL_LEN];

  if (value[0] == '\0') {
    secret_password_clear_sync(&llog_secret_schema, NULL, &error, "key", key, NULL);
  } else {
    snprintf(label, sizeof(label), "llog %s", key);
    secret_password_store_sync(&llog_secret_schema, SECRET_COLLECTION_DEFAULT, label, value,
                               NULL, &error, "key", key, NULL);
  }

  if (error != NULL) {
    fprintf(stderr, "Keyring store of `%s` failed: %s\n", key, error->message);
    g_error_free(error);
    return false;
  }
  return true;
}

#else

bool secret_store_load(const char *key, char *value, size_t len) {
  (void)key;
  (void)value;
  (void)len;
  return false;
}


bool secret_store_save(const char *key, const char *value) {
  (void)key;
  (void)value;
  return false;
}

#endif
