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

