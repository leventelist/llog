/*	This is llog, a minimalist HAM logging software.
 *	Copyright (C) 2013  Levente Kovacs
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

int csv_parse ( char *line, char *list[], int size )
{
  char *p;
  char *dp;
  int inquote;
  int na;
  char prevc = ',';

  dp = NULL;
  inquote = 0;
  na = 0;
  prevc = ',';
  for ( p = line; *p != '\0'; prevc = *p, p++ ) {
    if ( prevc == ',' && !inquote ) {
      if ( dp != NULL )
        *dp = '\0';
      if ( na >= size )
        return na;
      list[na++] = p;
      dp = p;
      if ( *p == '"' ) {
        inquote = 1;
        continue;
      }
    }
    if ( inquote && *p == '"' ) {
      if ( p[1] != '"' )
        inquote = 0;
      p++;
    }
    if ( *p != ',' || inquote )
      *dp++ = *p;
  }
  if ( dp != NULL )
    *dp = '\0';
  if ( na < size )
    list[na] = NULL;

  return na;
}

