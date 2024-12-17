/*	This is a configuration utility.
 *	Copyright (C) 2007-2021 Levente Kovacs
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "conf.h"
static int parse_config_line(char *line, char *attr, char *value);
static int config_lookup(config_attribute_t *ca, char *option);
static int config_set(config_attribute_t *ca, char *option, char *value);

static int parse_config_line(char *line, char *option, char *value) {
  char *s, *ss;
  int n = 0;

  s = line;
  while (*s == ' ' || *s == '\t')
    ++s;
  if (!*s || *s == '\n' || *s == '#' || *s == '[') {
    return n;
  }
  ss = s;
  while (*ss && *ss != '=' && *ss != ':')
    ++ss;
  n = ss - s;
  if (n > OPTSIZE) {
    return CONF_OUT_OF_MEM_ERR;
  }
  strncpy(option, s, n);
  option[n] = '\0';
  while (*ss == ' ' || *ss == '\t' || *ss == ':' || *ss == '=' || *ss == '"')
    ++ss;
  s = ss;
  while (*s && *s != '\n' && *s != '"' && *s != '#')
    ++s;
  n = s - ss;
  if (n > VALUESIZE) {
    return CONF_OUT_OF_MEM_ERR;
  }
  strncpy(value, ss, n);
  value[n] = '\0';
  return n;
}

int config_file_read(char *path, config_attribute_t *ca) {
  FILE *fp;
  char buff[BUFFER_SIZE], option[OPTSIZE], value[VALUESIZE];

  fp = fopen(path, "r");
  if (fp == NULL) {
#ifdef DEBUG
    sprintf(buff, "can't open config file: %s", path);
    perror(buff);
#endif
    return CONF_READ_ERR;
  }
  while (fgets(buff, sizeof(buff), fp)) {
    if (parse_config_line(buff, option, value) > 0) {
      config_set(ca, option, value);
    }
  }
  fclose(fp);
  return CONF_OK;
}

static int config_lookup(config_attribute_t *ca, char *option) {
  int i;

  for (i = 0; ca[i].name != NULL; ++i) {
    if (strstr(option, ca[i].name) != NULL)
      return i;
  }
  return CONF_LOOKUP_NO_MATCH;
}

static int config_set(config_attribute_t *ca, char *option, char *value) {
  int n;

  if ((n = config_lookup(ca, option)) == CONF_LOOKUP_NO_MATCH) {
    return CONF_LOOKUP_NO_MATCH;
  }
  switch ((ca[n]).type) {
  case CONFIG_String:
    strcpy((char *)ca[n].value, value);
    break;

  case CONFIG_Boolean:
    *(int *)ca[n].value = strstr(value, "true") ? 1 : 0;
    break;

  case CONFIG_Integer:
    *(int *)ca[n].value = atoi(value);
    break;

  case CONFIG_ULInteger:
    *(uint32_t *)ca[n].value = strtoul(value, NULL, 0);
    break;

  case CONFIG_ULLInteger:
    *(uint64_t *)ca[n].value = strtoull(value, NULL, 0);
    break;

  case CONFIG_Real:
    *(double *)ca[n].value = atof(value);
    break;

  case CONFIG_Unused:
    break;
  }
  return CONF_OK;
}

int config_print(config_attribute_t *ca) {
  int ret;

  ret = config_print_file("/dev/stdout", ca);

  return ret;
}

int config_print_file(char *dest_filename, config_attribute_t *ca) {
  int ret;
  FILE *dest_fd;
  int i;

  ret = CONF_OK;

  dest_fd = fopen(dest_filename, "w");

  if (dest_fd != NULL) {
    for (i = 0; ca[i].name != NULL; ++i) {
      switch ((ca[i]).type) {
      case CONFIG_String:
        fprintf(dest_fd, "%s=%s\n", ca[i].name, (char *)ca[i].value);
        break;

      case CONFIG_Boolean:
        fprintf(dest_fd, "%s=%d\n", ca[i].name, *(int *)ca[i].value ? 1 : 0);
        break;

      case CONFIG_Integer:
        fprintf(dest_fd, "%s=%d\n", ca[i].name, *(int *)ca[i].value);
        break;

      case CONFIG_ULInteger:
        fprintf(dest_fd, "%s=%lu\n", ca[i].name, *(unsigned long int *)ca[i].value);
        break;

      case CONFIG_ULLInteger:
        fprintf(dest_fd, "%s=%" PRIu64 "\n", ca[i].name, *(uint64_t *)ca[i].value);
        break;

      case CONFIG_Real:
        fprintf(dest_fd, "%s=%f\n", ca[i].name, *(double *)ca[i].value);
        break;

      case CONFIG_Unused:
        break;
      }
    }
    fprintf(dest_fd, "\n");
    fclose(dest_fd);
  } else {
    ret = CONF_WRITE_ERR;
  }


  return ret;
}
