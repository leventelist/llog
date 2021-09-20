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

#ifndef CONF_H
#define CONF_H

/*Error codes*/
#define CONF_OUT_OF_MEM_ERR -1
#define CONF_READ_ERR -2
#define CONF_WRITE_ERR -3
#define CONF_LOOKUP_NO_MATCH -4
#define CONF_OK 0

/*Internals*/

#define BUFFER_SIZE 255
#define OPTSIZE 64
#define VALUESIZE 255

enum ConfigType {
	CONFIG_Boolean,
	CONFIG_Integer,
	CONFIG_ULInteger,
	CONFIG_ULLInteger,
	CONFIG_Real,
	CONFIG_String,
	CONFIG_Unused
};

typedef struct {
	char *name;
	enum ConfigType type;
	void *value;
} config_attribute_t;

int config_file_read(char *path, config_attribute_t *ca);
int config_print_file(char *dest_filename, config_attribute_t *ca);
int config_print(config_attribute_t *ca);
#endif
