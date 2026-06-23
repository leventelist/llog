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
#include <stdbool.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include "conf.h"


#define CONFIG_FILE "llog.cf"

#define PATH_LEN 1024
#define BASE_LEN 512


static char config_file_path[PATH_LEN];

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


static FILE *open_config_file(char *app_name) {
    const char *home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "open_config_file: $HOME is not set\n");
        return NULL;
    }

    /* Build ~/.config/<APP_NAME> */
    char dir_path[BASE_LEN];
    int n = snprintf(dir_path, BASE_LEN,
                     "%s/.config/%s", home, app_name);
    if (n < 0 || (size_t)n >= BASE_LEN) {
        fprintf(stderr, "open_config_file: path too long\n");
        errno = ENAMETOOLONG;
        return NULL;
    }

    /* Build the full file path */

    n = snprintf(config_file_path, PATH_LEN,
                 "%s/%s", dir_path, CONFIG_FILE);
    if (n < 0 || (size_t)n >= PATH_LEN) {
        fprintf(stderr, "open_config_file: path too long\n");
        errno = ENAMETOOLONG;
        return NULL;
    }

    /* Try to open the config file for reading + writing */
    FILE *fp = fopen(config_file_path, "r+");
    if (fp) {
        printf("open_config_file: opened existing config: %s\n", config_file_path);
        return fp;
    }

    if (errno != ENOENT) {
        /* Something unexpected – permissions, I/O error, etc. */
        perror("open_config_file: fopen");
        return NULL;
    }

    /* File doesn't exist; make sure the directory exists first */
    struct stat st;
    if (stat(dir_path, &st) != 0) {
        if (errno != ENOENT) {
            perror("open_config_file: stat dir");
            return NULL;
        }

        /*
         * Directory is missing – create ~/.config first (it may already
         * exist), then the app subdirectory.
         */
        char base_config[BASE_LEN];
        snprintf(base_config, BASE_LEN, "%s/.config", home);

        /* mkdir returns EEXIST for a pre-existing dir – that's fine */
        if (mkdir(base_config, 0700) != 0 && errno != EEXIST) {
            perror("open_config_file: mkdir ~/.config");
            return NULL;
        }
        if (mkdir(dir_path, 0700) != 0 && errno != EEXIST) {
            perror("open_config_file: mkdir app dir");
            return NULL;
        }

        printf("open_config_file: created directory: %s\n", dir_path);
    } else {
        /* Directory already exists */
        printf("open_config_file: directory exists, config file missing: %s\n",
               dir_path);
    }

    /* Create the config file */
    fp = fopen(config_file_path, "w+");
    if (!fp) {
        perror("open_config_file: fopen (create)");
        return NULL;
    }

    /* Write a minimal default config so the file isn't completely empty */
    fprintf(fp, "# %s configuration\n", app_name);
    fprintf(fp, "# Created automatically\n\n");
    rewind(fp);   /* reposition to the beginning for the caller */

    printf("open_config_file: created new config: %s\n", config_file_path);
    return fp;
}


int config_file_read(config_attribute_t *ca, char *app_name) {
  FILE *fp;
  char buff[BUFFER_SIZE], option[OPTSIZE], value[VALUESIZE];

  fp = open_config_file(app_name);
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


char *get_config_file_path(void) {
  return config_file_path;
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
    *(bool *)ca[n].value = strstr(value, "true") ? true : false;
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

  ret = config_print_file(ca);

  return ret;
}

int config_print_file(config_attribute_t *ca) {
  int ret;
  FILE *dest_fd;
  int i;

  ret = CONF_OK;

  dest_fd = fopen(config_file_path, "w");

  if (dest_fd != NULL) {
    for (i = 0; ca[i].name != NULL; ++i) {
      switch ((ca[i]).type) {
      case CONFIG_String:
        fprintf(dest_fd, "%s=%s\n", ca[i].name, (char *)ca[i].value);
        break;

      case CONFIG_Boolean:
        fprintf(dest_fd, "%s=%s\n", ca[i].name, *(bool *)ca[i].value ? "true" : "false");
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
