/*********************************************************************
*
*  $Id: rgxtools.h,v 1.3 2011/02/14 15:06:54 woodbri Exp $
*
*  TigerToShape - t2s
*  Copyright 2001, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
*********************************************************************
*
*  rgxtools.h
*
*  $Log: rgxtools.h,v $
*  Revision 1.3  2011/02/14 15:06:54  woodbri
*  Added guard to prevent multiple inclusions.
*
*  Revision 1.2  2002/08/16 14:55:41  woodbri
*  *** empty log message ***
*
*  Revision 1.1  2001/12/30 23:45:28  woodbri
*  Initial revision
*
*
*  2001-12-29 sew, created     ver 0.1
*
*********************************************************************
*
*  routines to do search and replace type functions using regex
*
*********************************************************************/

#ifndef RGXTOOLS_H
#define RGXTOOLS_H

#include <regex.h>

typedef struct structRXcache {

  int         rclass;
  int         rindex;
  int         flags;
  int         num_subs;
  char       *pat;
  char       *replace;
  regmatch_t *pmatch;
  regex_t    *preg;

} RXcache;


// Internal private functions

// int count_subs(char *pat);
// int sizeofsub(regmatch_t *pmatch, int npmatch, int n);
// regex_t *compreg(char *pat, int icase, int subs);

// Public functions

int match(char *str, char *pat, int icase);
char *replace(char *str, char *pat, char *replace, int icase);
RXcache *buildRXcache(char *pat, char *replace, int icase);
void freeRXcache(RXcache *c);
int cmatch(RXcache *c, char *str);
char *creplace(RXcache *c, char *str);

#endif
