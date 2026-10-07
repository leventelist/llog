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

/*
 * WSJT-X UDP listener.
 *
 * WSJT-X sends Qt QDataStream encoded datagrams (big endian) to the
 * "UDP Server" set in its Reporting settings. See NetworkMessage.hpp in
 * the WSJT-X sources for the format. Only the Status (1) and QSO Logged (5)
 * messages are used, everything else is ignored.
 */

#include <stdio.h>
#include <string.h>
#include <gio/gio.h>

#include "wsjtx_client.h"

#define WSJTX_MAGIC 0xadbccbda
#define WSJTX_MSG_STATUS 1
#define WSJTX_MSG_QSO_LOGGED 5
#define WSJTX_DGRAM_SIZE 4096
#define WSJTX_NULL_STRING 0xffffffff

/*Julian day number of 1970-01-01, as Qt stores QDate*/
#define JULIAN_DAY_UNIX_EPOCH 2440588

enum qt_time_spec {
  qt_local_time = 0,
  qt_utc,
  qt_offset_from_utc,
  qt_time_zone
};

typedef struct {
  const guint8 *p;
  gsize left;
  gboolean err;       /*Set once a read runs past the end; later reads return zero*/
} reader_t;

static GSocket *wsjtx_socket = NULL;
static GSource *wsjtx_source = NULL;
static wsjtx_status_cb_t wsjtx_status_cb = NULL;
static wsjtx_qso_cb_t wsjtx_qso_cb = NULL;


static gboolean rd_take(reader_t *r, gsize n) {
  if (r->err || r->left < n) {
    r->err = TRUE;
    return FALSE;
  }
  return TRUE;
}

static guint8 rd_u8(reader_t *r) {
  guint8 v;

  if (!rd_take(r, 1)) {
    return 0;
  }
  v = r->p[0];
  r->p += 1;
  r->left -= 1;
  return v;
}

static guint32 rd_u32(reader_t *r) {
  guint32 v;

  if (!rd_take(r, 4)) {
    return 0;
  }
  memcpy(&v, r->p, 4);
  r->p += 4;
  r->left -= 4;
  return GUINT32_FROM_BE(v);
}

static guint64 rd_u64(reader_t *r) {
  guint64 v;

  if (!rd_take(r, 8)) {
    return 0;
  }
  memcpy(&v, r->p, 8);
  r->p += 8;
  r->left -= 8;
  return GUINT64_FROM_BE(v);
}

/*A QByteArray: length, then UTF-8 bytes. Truncated to fit dst.*/
static void rd_utf8(reader_t *r, char *dst, gsize dst_len) {
  guint32 len;
  gsize n;

  dst[0] = '\0';
  len = rd_u32(r);
  if (r->err || len == WSJTX_NULL_STRING) {
    return;
  }
  if (!rd_take(r, len)) {
    return;
  }
  n = MIN((gsize)len, dst_len - 1);
  memcpy(dst, r->p, n);
  dst[n] = '\0';
  r->p += len;
  r->left -= len;
}

/*A QDateTime: Julian day, ms since midnight, time spec (and offset for spec 2)*/
static time_t rd_datetime(reader_t *r) {
  gint64 julian_day;
  guint32 ms;
  guint8 spec;
  time_t t;
  struct tm bdt;

  julian_day = (gint64)rd_u64(r);
  ms = rd_u32(r);
  spec = rd_u8(r);

  if (ms == WSJTX_NULL_STRING) {
    ms = 0;
  }
  t = (time_t)((julian_day - JULIAN_DAY_UNIX_EPOCH) * 86400 + ms / 1000);

  switch (spec) {
  case qt_utc:
    break;

  case qt_offset_from_utc:
    t -= (gint32)rd_u32(r);
    break;

  case qt_local_time:
    gmtime_r(&t, &bdt);
    bdt.tm_isdst = -1;
    t = mktime(&bdt);
    break;

  default:
    /*A serialized QTimeZone follows, which WSJT-X does not send. Give up on the rest.*/
    r->err = TRUE;
    break;
  }

  return t;
}


static void parse_status(reader_t *r) {
  wsjtx_status_t status;
  char skip[CALL_LEN];

  memset(&status, 0, sizeof(status));

  rd_utf8(r, skip, sizeof(skip));                            /*Id*/
  status.qrg = rd_u64(r) / 1e6;                              /*Dial frequency, Hz*/
  rd_utf8(r, status.mode, sizeof(status.mode));
  rd_utf8(r, status.dx_call, sizeof(status.dx_call));
  rd_utf8(r, status.report, sizeof(status.report));
  rd_utf8(r, skip, sizeof(skip));                            /*Tx mode*/
  rd_u8(r);                                                  /*Tx enabled*/
  rd_u8(r);                                                  /*Transmitting*/
  rd_u8(r);                                                  /*Decoding*/
  rd_u32(r);                                                 /*Rx DF*/
  rd_u32(r);                                                 /*Tx DF*/
  rd_utf8(r, skip, sizeof(skip));                            /*DE call*/
  rd_utf8(r, skip, sizeof(skip));                            /*DE grid*/
  rd_utf8(r, status.dx_grid, sizeof(status.dx_grid));
  rd_u8(r);                                                  /*Tx watchdog*/
  rd_utf8(r, status.sub_mode, sizeof(status.sub_mode));

  /*Older WSJT-X versions stop earlier. What was read up to there is still good.*/
  if (wsjtx_status_cb != NULL && status.mode[0] != '\0') {
    wsjtx_status_cb(&status);
  }
}


static void parse_qso_logged(reader_t *r) {
  wsjtx_qso_t *qso;
  char skip[CALL_LEN];

  /*Too big for the stack with the comment and exchange buffers*/
  qso = g_malloc0(sizeof(wsjtx_qso_t));

  rd_utf8(r, skip, sizeof(skip));                            /*Id*/
  qso->time_off = rd_datetime(r);
  rd_utf8(r, qso->dx_call, sizeof(qso->dx_call));
  rd_utf8(r, qso->dx_grid, sizeof(qso->dx_grid));
  qso->qrg = rd_u64(r) / 1e6;                                /*Tx frequency, Hz*/
  rd_utf8(r, qso->mode, sizeof(qso->mode));
  rd_utf8(r, qso->report_sent, sizeof(qso->report_sent));
  rd_utf8(r, qso->report_rcvd, sizeof(qso->report_rcvd));
  rd_utf8(r, qso->power, sizeof(qso->power));
  rd_utf8(r, qso->comment, sizeof(qso->comment));
  rd_utf8(r, qso->name, sizeof(qso->name));
  qso->time_on = rd_datetime(r);
  if (r->err) {
    /*Time on is a later addition: fall back to time off*/
    qso->time_on = qso->time_off;
  }
  rd_utf8(r, skip, sizeof(skip));                            /*Operator call*/
  rd_utf8(r, skip, sizeof(skip));                            /*My call*/
  rd_utf8(r, skip, sizeof(skip));                            /*My grid*/
  rd_utf8(r, qso->exchange_sent, sizeof(qso->exchange_sent));
  rd_utf8(r, qso->exchange_rcvd, sizeof(qso->exchange_rcvd));

  if (wsjtx_qso_cb != NULL && qso->dx_call[0] != '\0') {
    wsjtx_qso_cb(qso);
  }
  g_free(qso);
}


static void parse_datagram(const guint8 *buf, gsize len) {
  reader_t r = { buf, len, FALSE };
  guint32 magic;
  guint32 type;

  magic = rd_u32(&r);
  rd_u32(&r);                                                /*Schema*/
  type = rd_u32(&r);

  if (r.err || magic != WSJTX_MAGIC) {
    return;
  }

  switch (type) {
  case WSJTX_MSG_STATUS:
    parse_status(&r);
    break;

  case WSJTX_MSG_QSO_LOGGED:
    parse_qso_logged(&r);
    break;

  default:
    break;
  }
}


static gboolean on_socket_readable(GSocket *socket, GIOCondition condition, gpointer user_data) {
  (void)condition;
  (void)user_data;
  guint8 buf[WSJTX_DGRAM_SIZE];
  gssize n;
  GError *error = NULL;

  for (;;) {
    n = g_socket_receive(socket, (gchar *)buf, sizeof(buf), NULL, &error);
    if (n < 0) {
      if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK)) {
        g_printerr("WSJT-X: receive failed: %s\n", error->message);
      }
      g_error_free(error);
      break;
    }
    parse_datagram(buf, n);
  }

  return G_SOURCE_CONTINUE;
}


int wsjtx_client_init(const char *addr, uint64_t port, wsjtx_status_cb_t status_cb, wsjtx_qso_cb_t qso_cb) {
  GInetAddress *inet_addr;
  GInetAddress *bind_addr;
  GSocketAddress *sock_addr;
  GError *error = NULL;
  gboolean multicast;
  int ret = wsjtx_client_stat_err;

  wsjtx_client_shutdown();

  inet_addr = g_inet_address_new_from_string(addr);
  if (inet_addr == NULL) {
    g_printerr("WSJT-X: invalid address: %s\n", addr);
    return ret;
  }

  multicast = g_inet_address_get_is_multicast(inet_addr);

  /*A multicast group is received on the wildcard address*/
  if (multicast) {
    bind_addr = g_inet_address_new_any(g_inet_address_get_family(inet_addr));
  } else {
    bind_addr = g_object_ref(inet_addr);
  }

  wsjtx_socket = g_socket_new(g_inet_address_get_family(inet_addr), G_SOCKET_TYPE_DATAGRAM,
                              G_SOCKET_PROTOCOL_UDP, &error);
  if (wsjtx_socket == NULL) {
    g_printerr("WSJT-X: can't create socket: %s\n", error->message);
    g_error_free(error);
    goto out;
  }
  g_socket_set_blocking(wsjtx_socket, FALSE);

  /*Allow reuse, so others (GridTracker, JTAlert) can listen on the same port*/
  sock_addr = g_inet_socket_address_new(bind_addr, port);
  if (!g_socket_bind(wsjtx_socket, sock_addr, TRUE, &error)) {
    g_printerr("WSJT-X: can't listen on %s:%" G_GUINT64_FORMAT ": %s\n", addr, port, error->message);
    g_error_free(error);
    g_object_unref(sock_addr);
    g_clear_object(&wsjtx_socket);
    goto out;
  }
  g_object_unref(sock_addr);

  if (multicast) {
    /*WSJT-X sends multicast on the loopback interface by default, so join there as well*/
    gboolean joined = g_socket_join_multicast_group(wsjtx_socket, inet_addr, FALSE, NULL, NULL);
    joined = g_socket_join_multicast_group(wsjtx_socket, inet_addr, FALSE, "lo", NULL) || joined;
    if (!joined) {
      g_printerr("WSJT-X: can't join multicast group %s\n", addr);
    }
  }

  wsjtx_status_cb = status_cb;
  wsjtx_qso_cb = qso_cb;

  /*Attached to the default context, so the callbacks run on the GTK main thread*/
  wsjtx_source = g_socket_create_source(wsjtx_socket, G_IO_IN, NULL);
  g_source_set_callback(wsjtx_source, G_SOURCE_FUNC(on_socket_readable), NULL, NULL);
  g_source_attach(wsjtx_source, NULL);

  printf("WSJT-X: listening on %s:%" G_GUINT64_FORMAT "\n", addr, port);
  ret = wsjtx_client_stat_ok;

out:
  g_object_unref(bind_addr);
  g_object_unref(inet_addr);
  return ret;
}


void wsjtx_client_shutdown(void) {
  if (wsjtx_source != NULL) {
    g_source_destroy(wsjtx_source);
    g_source_unref(wsjtx_source);
    wsjtx_source = NULL;
  }
  if (wsjtx_socket != NULL) {
    g_socket_close(wsjtx_socket, NULL);
    g_clear_object(&wsjtx_socket);
  }
}
