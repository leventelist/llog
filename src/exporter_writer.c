/*	This is llog, a minimalist HAM logging software.
 *	Copyright (C) 2013-2025  Levente Kovacs
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
#include <libxml/xmlwriter.h>

#include "exporter_writer.h"
#include "band.h"

#include "llog.h"

#define ENCODING "UTF-8"


static xmlTextWriterPtr writer;
static FILE *output_txt_file;
static adif_writer_export_format_t export_format;


static int exporter_write_adx_header(const char *filename);
static int exporter_write_adi_header(const char *filename);
static int exporter_write_csv_header(const char *filename);
static int exporter_add_adx_qso(log_entry_t *qso, station_entry_t *station);
static int exporter_add_adi_qso(log_entry_t *qso, station_entry_t *station);
static int exporter_add_csv_qso(log_entry_t *qso, station_entry_t *station);
static void exporter_adx_close(void);
static void exporter_file_close(void);





/*Interface functions*/

int exporter_write_header(const char *filename, adif_writer_export_format_t format) {
  export_format = format;

  int ret;

  switch (format) {
  case export_format_adi:
    ret = exporter_write_adi_header(filename);
    break;

  case export_format_adx:
    ret = exporter_write_adx_header(filename);
    break;

  case export_format_csv:
    ret = exporter_write_csv_header(filename);
    break;

  default:
    ret = export_status_err;
    break;
  }

  return ret;
}


int exporter_add_qso(log_entry_t *entry, station_entry_t *station) {
  int ret;
  switch (export_format) {
  case export_format_adi:
    ret = exporter_add_adi_qso(entry, station);
    break;

  case export_format_adx:
    ret = exporter_add_adx_qso(entry, station);
    break;

  case export_format_csv:
    ret = exporter_add_csv_qso(entry, station);
    break;

  default:
    ret = export_status_err;
    break;
  }

  return ret;
}


void exporter_close(void) {
  switch (export_format) {
  case export_format_adi:
    exporter_file_close();
    break;

  case export_format_adx:
    exporter_adx_close();
    break;

  case export_format_csv:
    exporter_file_close();
    break;

  default:
    break;
  }
}

/*ADX format handlers*/

static int exporter_write_adx_header(const char *filename) {
  int ret = export_status_ok;
  int ret_val;
  writer = xmlNewTextWriterFilename(filename, 0);


  if (writer == NULL) {
    fprintf(stderr, "Error creating XML writer\n");
    ret = export_status_err;
    goto out;
  }

  // Start the document
  if (xmlTextWriterStartDocument(writer, NULL, ENCODING, NULL) < 0) {
    fprintf(stderr, "Error starting XML document\n");
    xmlFreeTextWriter(writer);
    ret = export_status_err;
    goto out;
  }

  // Start the ADIF root element
  ret_val = xmlTextWriterStartElement(writer, BAD_CAST "ADX");
  if (ret_val < 0) {
    fprintf(stderr, "Error writing XML element\n");
    ret = export_status_err;
    goto out;
  }

  // Add the header
  ret_val = xmlTextWriterStartElement(writer, BAD_CAST "HEADER");
  if (ret_val < 0) {
    fprintf(stderr, "Error writing XML element\n");
    ret = export_status_err;
    goto out;
  }
  ret_val = xmlTextWriterWriteElement(writer, BAD_CAST "ADIF_VER", BAD_CAST "3.0.5");
  if (ret_val < 0) {
    fprintf(stderr, "Error writing XML element\n");
    ret = export_status_err;
    goto out;
  }
  ret_val = xmlTextWriterWriteElement(writer, BAD_CAST "PROGRAMID", BAD_CAST PROGRAM_NAME);
  if (ret_val < 0) {
    fprintf(stderr, "Error writing XML element\n");
    ret = export_status_err;
    goto out;
  }
  ret_val = xmlTextWriterWriteElement(writer, BAD_CAST "PROGRAMVERSION", BAD_CAST VERSION);
  if (ret_val < 0) {
    fprintf(stderr, "Error writing XML element\n");
    ret = export_status_err;
    goto out;
  }
  ret_val = xmlTextWriterEndElement(writer);   // Close HEADER
  if (ret_val < 0) {
    fprintf(stderr, "Error writing XML element\n");
    ret = export_status_err;
    goto out;
  }

  // Start the records
  ret_val = xmlTextWriterStartElement(writer, BAD_CAST "RECORDS");
  if (ret_val < 0) {
    fprintf(stderr, "Error writing XML element\n");
    ret = export_status_err;
    goto out;
  }

  out:
  return ret;
}


static int exporter_add_adx_qso(log_entry_t *qso, station_entry_t *station) {
  (void)station;
  const char *band_str;
  int ret;
  int ret_val;

  ret = export_status_ok;

  // Find the band based on the frequency
  band_str = band_find(qso->qrg);

  ret_val = xmlTextWriterStartElement(writer, BAD_CAST "RECORD");
  if (ret_val < 0) {
    fprintf(stderr, "Error writing XML element\n");
    ret = export_status_err;
    goto out;
  }
  ret_val = xmlTextWriterWriteElement(writer, BAD_CAST "CALL", BAD_CAST qso->call);
  if (ret_val < 0) {
    fprintf(stderr, "Error writing XML element\n");
    ret = export_status_err;
    goto out;
  }
  ret_val = xmlTextWriterWriteElement(writer, BAD_CAST "BAND", BAD_CAST band_str);
  if (ret_val < 0) {
    fprintf(stderr, "Error writing XML element\n");
    ret = export_status_err;
    goto out;
  }
  ret_val = xmlTextWriterWriteElement(writer, BAD_CAST "MODE", BAD_CAST qso->mode.name);
  if (ret_val < 0) {
    fprintf(stderr, "Error writing XML element\n");
    ret = export_status_err;
    goto out;
  }
  ret_val = xmlTextWriterWriteElement(writer, BAD_CAST "QSO_DATE", BAD_CAST qso->date);
  if (ret_val < 0) {
    fprintf(stderr, "Error writing XML element\n");
    ret = export_status_err;
    goto out;
  }
  ret_val = xmlTextWriterWriteElement(writer, BAD_CAST "TIME_ON", BAD_CAST qso->utc);
  if (ret_val < 0) {
    fprintf(stderr, "Error writing XML element\n");
    ret = export_status_err;
    goto out;
  }
  ret_val = xmlTextWriterWriteElement(writer, BAD_CAST "RST_RCVD", BAD_CAST qso->rxrst);
  if (ret_val < 0) {
    fprintf(stderr, "Error writing XML element\n");
    ret = export_status_err;
    goto out;
  }
  ret_val = xmlTextWriterWriteElement(writer, BAD_CAST "RST_SENT", BAD_CAST qso->txrst);
  if (ret_val < 0) {
    fprintf(stderr, "Error writing XML element\n");
    ret = export_status_err;
    goto out;
  }
  ret_val = xmlTextWriterEndElement(writer);     // Close RECORD
  if (ret_val < 0) {
    fprintf(stderr, "Error writing XML element\n");
    ret = export_status_err;
    goto out;
  }

out:
  return ret;
}


static void exporter_adx_close(void) {
  xmlTextWriterEndElement(writer);   // Close RECORDS
  xmlTextWriterEndElement(writer);   // Close ADIF

  // End the document
  if (xmlTextWriterEndDocument(writer) < 0) {
    fprintf(stderr, "Error ending XML document\n");
  }

  // Clean up
  xmlFreeTextWriter(writer);
}


/*ADI format handlers*/

static int exporter_write_adi_header(const char *filename) {
// Open the file for writing
  output_txt_file = fopen(filename, "w");
  char *buffer;
  int ret = export_status_ok;
  int num_bytes;

  buffer = (char *)malloc(1024);

  if (buffer == NULL) {
    perror("Failed to allocate memory");
    goto out;
  }

  if (!output_txt_file) {
    perror("Failed to open file");
    return export_status_file_err;
  }

  num_bytes = fprintf(output_txt_file, "<adif_ver:5>3.0.5\n");

  if (num_bytes < 0) {
    perror("Failed to write to file");
    ret = export_status_file_err;
    goto out;
  } else {
    ret = export_status_ok;
  }

  sprintf(buffer, "<programid:%lu>%s\n", strlen(PROGRAM_NAME), PROGRAM_NAME);
  num_bytes = fprintf(output_txt_file, buffer);

  if (num_bytes < 0) {
    perror("Failed to write to file");
    ret = export_status_file_err;
    goto out;
  } else {
    ret = export_status_ok;
  }

  sprintf(buffer, "<programversion:%lu>%s\n", strlen(VERSION), VERSION);
  num_bytes = fprintf(output_txt_file, buffer);

  if (num_bytes < 0) {
    perror("Failed to write to file");
    ret = export_status_file_err;
    goto out;
  } else {
    ret = export_status_ok;
  }

  num_bytes = fprintf(output_txt_file, "<EOH>\n");   // End of Header

  if (num_bytes < 0) {
    perror("Failed to write to file");
    ret = export_status_file_err;
  } else {
    ret = export_status_ok;
  }

out:

  free(buffer);
  return ret;
}

static int exporter_add_adi_qso(log_entry_t *qso, station_entry_t *station) {
  const char *band_str;
  char freq_buffer[20];

  int ret;
  int num_bytes;

  ret = export_status_ok;

  (void)station;

  // Find the band based on the frequency
  band_str = band_find(qso->qrg);

  num_bytes = fprintf(output_txt_file, "<CALL:%lu>%s\n", strlen(qso->call), qso->call);

  if (num_bytes < 0) {
    perror("Failed to write to file");
    ret = export_status_file_err;
    goto out;
  }

  num_bytes = fprintf(output_txt_file, "<BAND:%lu>%s\n", strlen(band_str), band_str);
  if (num_bytes < 0) {
    perror("Failed to write to file");
    ret = export_status_file_err;
    goto out;
  }
  num_bytes = fprintf(output_txt_file, "<MODE:%lu>%s\n", strlen(qso->mode.name), qso->mode.name);
  if (num_bytes < 0) {
    perror("Failed to write to file");
    ret = export_status_file_err;
    goto out;
  }

  // Convert date from YYYY-MM-DD to YYYYMMDD format
  char formatted_date[9];
  snprintf(formatted_date, sizeof(formatted_date), "%.4s%.2s%.2s", qso->date, qso->date + 5, qso->date + 8);

  num_bytes = fprintf(output_txt_file, "<QSO_DATE:%lu>%s\n", strlen(formatted_date), formatted_date);
  if (num_bytes < 0) {
    perror("Failed to write to file");
    ret = export_status_file_err;
    goto out;
  }
  num_bytes = fprintf(output_txt_file, "<TIME_ON:%lu>%s\n", strlen(qso->utc), qso->utc);
  if (num_bytes < 0) {
    perror("Failed to write to file");
    ret = export_status_file_err;
    goto out;
  }
  num_bytes = fprintf(output_txt_file, "<RST_RCVD:%lu>%s\n", strlen(qso->rxrst), qso->rxrst);
  if (num_bytes < 0) {
    perror("Failed to write to file");
    ret = export_status_file_err;
    goto out;
  }
  num_bytes = fprintf(output_txt_file, "<RST_SENT:%lu>%s\n", strlen(qso->txrst), qso->txrst);
  if (num_bytes < 0) {
    perror("Failed to write to file");
    ret = export_status_file_err;
    goto out;
  }
  sprintf(freq_buffer, "%f", qso->qrg);
  num_bytes = fprintf(output_txt_file, "<FREQ:%lu>%s\n", strlen(freq_buffer), freq_buffer);
  if (num_bytes < 0) {
    perror("Failed to write to file");
    ret = export_status_file_err;
    goto out;
  }
  num_bytes = fprintf(output_txt_file, "<EOR>\n");   // End of Record
  if (num_bytes < 0) {
    perror("Failed to write to file");
    ret = export_status_file_err;
    goto out;
  }

out:

  return ret;
}


static void exporter_file_close(void) {
  // Close the file
  fclose(output_txt_file);
}


static int exporter_write_csv_header(const char *filename) {
  output_txt_file = fopen(filename, "w");

  if (!output_txt_file) {
    perror("Failed to open file");
    return export_status_file_err;
  }

  //fprintf(output_txt_file, "Date,Time,Band,Mode,Call,SOTA Ref,My SOTA Ref,Notes\n");

  return export_status_ok;
}


static int exporter_add_csv_qso(log_entry_t *qso, station_entry_t *station) {
  const char *band_str;

  // Find the band based on the frequency
  band_str = band_find(qso->qrg);

  fprintf(output_txt_file, "V2,%s,%s,%s,%s,%s,%s,%s,%s\n", station->call, qso->summit_ref, qso->date, qso->utc, band_str, qso->mode.name, qso->call, qso->s2s_ref);

  return export_status_ok;
}