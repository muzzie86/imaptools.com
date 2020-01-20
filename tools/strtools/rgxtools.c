/*********************************************************************
*
*  $Id: rgxtools.c,v 1.4 2008/02/04 03:38:59 woodbri Exp $
*
*  TigerToShape - t2s
*  Copyright 2001, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
*********************************************************************
*
*  rgxtools.c
*
*  $Log: rgxtools.c,v $
*  Revision 1.4  2008/02/04 03:38:59  woodbri
*  Updating cvs local changes.
*
*  Revision 1.3  2003/04/19 15:29:45  woodbri
*  *** empty log message ***
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "rgxtools.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif



int count_subs(char *pat) 
{ 
  int i, cnt, inbracket; 

  cnt      = 0; 
  inbracket= 0; 

  for (i=0; i<strlen(pat); i++) 
    switch (pat[i]) { 
      case '\\': 
        if (!inbracket && (pat[i+1] == '(' || pat[i+1] == '[')) 
          i++; 
        break; 
      case '[': 
        inbracket = 1; 
        break; 
      case ']': 
        inbracket = 0; 
        break; 
      case '(': 
        cnt++; 
        break; 
    } 
  return cnt; 
} 


int sizeofsub(regmatch_t *pmatch, int npmatch, int n)
{

  if (n >= npmatch) return 0;
  if (pmatch[n].rm_so == -1) return 0;
  return pmatch[n].rm_eo - pmatch[n].rm_so;
}


regex_t *compreg(char *pat, int icase, int subs) 
{ 
  int      err, esize, flags; 
  char    *errmsg; 
  regex_t *preg; 

  preg = (regex_t *) malloc(sizeof(regex_t)); 
  assert(preg); 

  flags = REG_EXTENDED; 
  if (icase) 
    flags |= REG_ICASE; 
  if (!subs) 
    flags |= REG_NOSUB; 

  if (err = regcomp(preg, pat, flags)) { 
    esize = regerror(err, preg, NULL, 0); 
    errmsg = (char *) malloc(esize); 
    assert(errmsg); 
    regerror(err, preg, errmsg, esize); 
    printf("%s\n", errmsg); 
    exit(1); 
  } 

  return preg; 
} 


int match(char *str, char *pat, int icase) 
{ 
  regex_t    *preg; 
  int         ret;

  preg = compreg(pat, icase, 0); 

  ret = regexec(preg, str, 0, NULL, 0); 
  regfree(preg);
  free(preg);

  return (!ret);
} 


char *replace(char *str, char *pat, char *replace, int icase) 
{ 
  regmatch_t *pmatch; 
  regex_t    *preg; 
  char       *ret; 
  int         num_sub, isub; 
  int         cnt, i, j, pos; 
  int         nomatch;

  num_sub = count_subs(pat) + 1; 

  pmatch = (regmatch_t *) malloc(num_sub * sizeof(regmatch_t)); 
  assert(pmatch); 

  preg = compreg(pat, icase, 1); 

  nomatch = regexec(preg, str, num_sub, pmatch, 0);
  regfree(preg);
  free(preg);

  if(nomatch) {
    free(pmatch);
    return NULL;  // string did not match 
  }

  // count how large return string will be 

  cnt = strlen(str) - sizeofsub(pmatch, num_sub, 0);
  for (i=0; i<strlen(replace); i++) 
    switch (replace[i]) { 
      case '\\': 
        if (replace[i+1] == '$') 
          i++; 
        cnt++; 
        break; 
      case '$': 
        if (isdigit(replace[i+1])) { 
          cnt += sizeofsub(pmatch, num_sub, replace[i+1]-'0'); 
          i++; 
        } else 
          cnt++; 
        break; 
      default: 
        cnt++; 
    } 

  // get space for return string 

  ret = (char *)calloc(cnt+1, 1); 
  assert(ret); 

  // build the return string 

  // copy any chars before the matched part

  if (pmatch[0].rm_so)
    strncpy(ret, str, pmatch[0].rm_so);

  // next copy matched part with replacements

  pos = pmatch[0].rm_so;
  for (i=0; i<strlen(replace); i++) 
    switch (replace[i]) { 
      case '\\': 
        if (replace[i+1] == '$') { 
          ret[pos++] = '$'; 
          i++; 
        } else 
          ret[pos++] = replace[i]; 
        break; 
      case '$': 
        if (isdigit(replace[i+1])) { 
          isub = replace[i+1] - '0'; 
          if (!isub) isub = 10; 
          if (isub < num_sub && pmatch[isub].rm_so > -1) { 
            for (j=pmatch[isub].rm_so; j<pmatch[isub].rm_eo; j++) 
              ret[pos++] = str[j]; 
          } 
          i++; 
        } else 
          ret[pos++] = replace[i]; 
        break; 
      default: 
        ret[pos++] = replace[i]; 
    }

  // then copy any part remaining after the matched part

  ret[pos] = '\0'; 
  if (strlen(str) > pmatch[0].rm_eo)
    strcat(ret, str + pmatch[0].rm_eo);

  if (pmatch) free(pmatch); 
  return ret; 
}


/*
** The following routines buildRXcache, freeRXcache, cmatch, creplace
** are designed to cache the compilied regex so it can be called repeatedly
** without the overhead of compiling the same expression every time.
** This would be more efficent where you were matching every line in a file
** for example.
*/

RXcache *buildRXcache(char *pat, char *replace, int icase)
{
  RXcache *c;
  char    *errmsg;
  int      err, esize;

  if (!pat) return NULL;

  c = (RXcache *) calloc(1, sizeof(RXcache));
  assert(c);

  c->pat = strdup(pat);
  c->replace = replace?strdup(replace):NULL;

  c->rclass = -1;  // set it to undefined
  c->rindex = -1;

  c->flags = REG_EXTENDED;
  if (icase)
    c->flags |= REG_ICASE;

  c->num_subs = count_subs(pat);
  if (!c->num_subs && !replace) {
    c->flags |= REG_NOSUB;
    c->pmatch = NULL;
  } else {
    c->num_subs++;
    c->pmatch = (regmatch_t *) malloc(c->num_subs * sizeof(regmatch_t));
    assert(c->pmatch);
  }

  c->preg = (regex_t *) malloc(sizeof(regex_t));
  assert(c->preg);

  if (err = regcomp(c->preg, c->pat, c->flags)) {
    esize = regerror(err, c->preg, NULL, 0);
    errmsg = (char *) malloc(esize);
    assert(errmsg);
    regerror(err, c->preg, errmsg, esize);
    printf("%s\n", errmsg);
    printf("pattern: \"%s\"\n", c->pat);
    free(errmsg);
    abort();
  }


  return c;
}



void freeRXcache(RXcache *c)
{
  if (!c) return;
  free(c->pat);
  if (c->replace)
    free(c->replace);
  regfree(c->preg);
  free(c->preg);
  if (c->pmatch)
    free(c->pmatch);
  free(c);
}



int cmatch(RXcache *c, char *str)
{
  int ret;
  ret = regexec(c->preg, str, 0, NULL, 0);
  return (!ret);
}



char *creplace(RXcache *c, char *str)
{
  char       *ret;
  int         isub, cnt, i, j, pos;


  if (regexec(c->preg, str, c->num_subs, c->pmatch, 0))
    return NULL;  // string did not match

  if (!c->pmatch || c->pmatch[0].rm_so == 0 && c->pmatch[0].rm_eo == 0)
    return NULL;

  // count how large return string will be

  cnt = strlen(str) - sizeofsub(c->pmatch, c->num_subs, 0);
  for (i=0; i<strlen(c->replace); i++)
    switch (c->replace[i]) {
      case '\\':
        if (c->replace[i+1] == '$')
          i++;
        cnt++;
        break;
      case '$':
        if (isdigit(c->replace[i+1])) {
          cnt += sizeofsub(c->pmatch, c->num_subs, c->replace[i+1]-'0');
          i++;
        } else
          cnt++;
        break;
      default:
        cnt++;
    }

  // get space for return string

  ret = (char *)calloc(cnt+1, 1);
  assert(ret);

  // build the return string

  // copy any chars before the matched part

  if (c->pmatch[0].rm_so)
    strncpy(ret, str, c->pmatch[0].rm_so);

  // next copy matched part with replacements

  pos = c->pmatch[0].rm_so;
  for (i=0; i<strlen(c->replace); i++)
    switch (c->replace[i]) {
      case '\\':
        if (c->replace[i+1] == '$') {
          ret[pos++] = '$';
          i++;
        } else
          ret[pos++] = c->replace[i];
        break;
      case '$':
        if (isdigit(c->replace[i+1])) {
          isub = c->replace[i+1] - '0';
          if (!isub) isub = 10;
          if (isub <= c->num_subs && c->pmatch[isub].rm_so > -1) {
            for (j=c->pmatch[isub].rm_so; j<c->pmatch[isub].rm_eo; j++)
              ret[pos++] = str[j];
          }
          i++;
        } else
          ret[pos++] = c->replace[i];
        break;
      default:
        ret[pos++] = c->replace[i];
    }

  // then copy any part remaining after the matched part

  ret[pos] = '\0';
  if (strlen(str) > c->pmatch[0].rm_eo)
    strcat(ret, str + c->pmatch[0].rm_eo);

  return ret;
}


