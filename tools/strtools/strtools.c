/**********************************************************************/
/*                                                                    */
/*  Copyright 2001, Stephen Woodbridge, All rights Reserved           */
/*  Redistribition without written permission strictly prohibited     */
/*                                                                    */
/**********************************************************************/
/*                                                                    */
/*  strtools.c                                                        */
/*                                                                    */
/*  2001-11-09 sew, created     ver 0.1                               */
/*  2001-11-24 sew, created     ver 0.2                               */
/*      put in separate dir, add ReplaceVars stuff                    */
/*                                                                    */
/*                                                                    */
/**********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "strtools.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif



void chomp(char *str)
{
  int i;

  if (!str || !strlen(str)) return;

  for (i=strlen(str)-1; i>-1; i--)
    if (str[i] == '\r' || str[i] == '\n')
      str[i] = '\0';
    else
      break;

}


char *trim(char *str)
{
  int i;

  if (!str || !strlen(str)) return str;

  for (i=strlen(str)-1; i>-1; i--)
    if (isspace(str[i]))
      str[i] = '\0';
    else
      break;

  return &str[strspn(str, "\t\r\n\v\f ")];

}

int toint( int c )
{

  if (c >= '0' && c<= '9') return c - '0';
  if (c >= 'A' && c<= 'F') return c - 'A' + 10;
  if (c >= 'a' && c<= 'f') return c - 'a' + 10;
  /* c is not a hexadecimal digit */
  return 0;

}

int cmpfld(char *buf, int pos, int len, int icase, char *match)
{
  int i, j;
  int length;

  if (!match || strlen(match) == 0) return 1;

  if (strlen(match) < len)
    length = strlen(match);
  else
    length = len;

  for (i=0; i<length; i++) {
    if (icase) 
      j = tolower(buf[pos+i]) - tolower(match[i]);
    else
      j = buf[pos+i] - match[i];

    if (j) return j;
  }

  if (len < strlen(match)) return -1;

  return 0;
}


#if defined(UNIX) && !defined(__CYGWIN__)

int strncmpi(const char *a, const char *b, int n)
{
    return strncasecmp(a, b, n);
}

/**************************************************
int strncmpi(const char *a, const char *b, int n)
{
    int i, j, al, bl, len;

    bl = strlen(b);
    if (bl > n) bl = n;
    if (bl == 0) return 1;

    al = strlen(a);
    if (al > n) al = n;

    len = n;
    if (al < len) len = al;
    if (bl < len) len = bl;

    for (i=0; i<len; i++) {
      j = tolower(a[i]) - tolower(b[i]);
      if (j) return j;
    }

    if (al == bl)
      return 0;
    else
      if (al < bl)
        return -1;
      else
        return 1;
}
*********************************************************/


int strcmpi(const char *a, const char *b)
{
    return strcasecmp(a, b);
}    
/*****************************************
int strcmpi(const char *a, const char *b)
    int i, j, len;

    // next line is a bug because if both a and b are null
    // we should return 0
    if (strlen(b) == 0) return 1;

    if (strlen(a) < strlen(b))
      len = strlen(a);
    else
      len = strlen(b);

    for (i=0; i<len; i++) {
      j = tolower(a[i]) - tolower(b[i]);
      if (j) return j;
    }

    if (strlen(a) == strlen(b))
      return 0;
    else
      if (strlen(a) < strlen(b))
        return -1;
      else
        return 1;
}
*******************************************/

#endif

int strcmpic(char *a, char *b, char c)
{
  int i, j, len;

  if (strlen(b) == 0) return 1;
  if (strlen(a) < strlen(b))
    len = strlen(a);
  else
    len = strlen(b);

  for (i=0; i<len; i++) {
    j = tolower(a[i]) - tolower(b[i]);
    if (j) {
      if ((tolower(a[i]) == c && tolower(b[i]) == ' ') ||
          (tolower(b[i]) == c && tolower(a[i]) == ' ')) 
        continue;
      else
        return j;
    }
  }

  if (strlen(a) == strlen(b))
    return 0;
  else
    if (strlen(a) < strlen(b))
      return -1;
    else
      return 1;
}




/*
*   strmcpy copies a maximum of maxc chars and then adds '\0' after them
*/

void strmcpy(char *d, const char *s, int maxc)
{
  int i;

  for (i=0; i<maxc && *s != '\0'; i++, s++, d++)
    *d = *s;
  *d = '\0';
}

void sstrcpy(char *d, const char *s)
{
  if (s)
    strcpy(d, s);
  else
    strcpy(d, "");
}


int imt_strpos(const char *s, char c)
{
    char *p;

    p = strchr(s, c);
    if (p)
      return p-s;
    else
      return -1;
}


Vars *NewVars()
{
  Vars *this;

  this = (Vars *) calloc(1, sizeof(Vars));
  this->nchunk = 10;

  return this;
}



Var *GetAVar(Vars *v, char *name)
{
  int  i;

  assert(v);
  for (i=0; i<v->num; i++)
    if (!strcmp(name, (&v->v[i])->name)) return &(v->v[i]);

  return NULL;
}



Vars *SetAVar(Vars *v, char *name, char *val, int dofree)
{
  int   i;

  assert(v);
  for (i=0; i<v->num; i++)
    if (!strcmp(name, (&v->v[i])->name)) {
      if ((&v->v[i])->free) free((&v->v[i])->val);
      (&v->v[i])->val  = val;
      (&v->v[i])->free = dofree;
      return v;
    }

  // need to add a new var

  if (!v->v || (v->num+1) >= v->nchunk*VARS_CHUNK) {
    v->nchunk++;
    v->v = (Var *) realloc(v->v, v->nchunk*VARS_CHUNK*sizeof(Var));
  }

  assert(v->v);

  (&v->v[v->num])->name = (char *) malloc(strlen(name)+1);
  strcpy((&v->v[v->num])->name, name);
  (&v->v[v->num])->val  = val;
  (&v->v[i])->free = dofree;
  v->num++;

  return v;
}



void ClearVars(Vars *v)
{
  int   i;

  assert(v);
  for (i=0; i<v->num; i++) {
    free((&v->v[i])->name);
    if ((&v->v[i])->free) free((&v->v[i])->val);
  }

  free(v->v);

  v->num    = 0;
  v->nchunk = 0;
}



char *ReplaceVars(Vars *v, char *template, int encode)
{
  enum estate {STRING, OPENBRACE} state;

  Var  *var;
  char *p;
  char *buf, *b, *n, *val;
  char  name[32];
  int size, len, len2;

  assert(v);

  state = STRING;
  p = template;

  size = strlen(template) * 3;
  size = (size/256 + 1) * 256;
  buf  = (char *) calloc(size, 1);
  b    = buf;

  while (p < p + strlen(p)) {
    switch (state) {
      case STRING:
        if (!strncmp("{{", p, 2)) {
          *b = *p;
          b++;
          p++;
        } else if (*p == '{') {
          state = OPENBRACE;
          n = name;
        } else {
          *b = *p;
          b++;
        }
        len = b-buf;
        if (len+2 > size) {
          size += 256;
          buf = (char *) realloc(buf, size);
          b = buf + len;
          memset(b, 0, size-len);
        }
        break;

      case OPENBRACE:
        if (*p == '}') {
          state = STRING;
          if (!(var = GetAVar(v, name))) break;

          if (encode == 1)
            val = url_encode(var->val);
          else if (encode == 2)
            val = xml_encode(var->val);
          else
            val = var->val;

          len = b-buf;
          len2 = len + strlen(val);

          if (len2+2 > size) {
            size += len2+2;
            size = (size/256 + 1) * 256;
            buf = (char *) realloc(buf, size);
            b = buf + len;
            memset(b, 0, size-len);
          }

          sprintf(b, "%s", val);
          if (encode) free(val);
          b = b + strlen(b);
        } else {
          *n = *p;
          if ((n - name) < 32) {
            n++;
            *n = '\0';
          }          
        }
        break;
    } // end of switch (state)

    p++;

  } // end of while

  *b = '\0';
  return buf;
}


char *xml_encode(char *str)
{
    int   i, j, k;
    int   len, newlen;
    char *this;
    char *p;
    char *g;
    char  bad[] = "<>&'\"";
    char *good[] = {"&lt;", "&gt;", "&amp;", "&apos;", "&quot;"};
    int   grow[] = {3,3,4,6,6};

    len = newlen = strlen(str);
    for (i=0; i<len; i++) {
        p = strchr(bad, str[i]);
        if (p)
            newlen += grow[p-bad];
    }
    
    this = (char *) malloc(newlen + 1);
    assert(this);

    for (i=0, j=0; i<len; i++) {
        p = strchr(bad, str[i]);
        if (p) {
            g = good[p-bad];
            for (k=0; k<strlen(g); k++) {
                this[j] = g[k];
                j++;
            }
        } else {
            this[j] = str[i];
            j++;
        }
    }
    this[j] = '\0';
    
    return this;
}


int urlsafe(int c)
{
  int safe[] = {'!', '$', 27, '(', ')', '*', ',', '-', '.', '_', ' '};
  int i;

  if (isalnum(c)) return 1;
  for (i=0; i<11; i++)
    if (safe[i] == c) return 1;

  return 0;
}

#define TOHEX(i) (((i)<10) ? ((i)+48) : ((i)+55))

char *url_encode(char *str)
{
  int   i;
  int   len, notsafe;
  char *this;
  char *p;

  notsafe = 0;
  len = strlen(str);
  for (i=0; i<len; i++)
    if (!urlsafe(str[i])) notsafe++;

  this = (char *) malloc(len + 1 + notsafe*2);
  assert(this);

  p = this;
  for (i=0; i<len; i++)
    if (str[i] == ' '){
      *p = '+';
      p++;
    } else if (urlsafe(str[i])) {
      *p = str[i];
      p++;
    } else {
      *p = '%';
      p++;
      *p = TOHEX((str[i] & 0x7f)>>4);
      p++;
      *p = TOHEX(str[i] & 0xf);
      p++;
    }

  *p = '\0';
  return this;
}



List *AddToList(List *p, void *item)
{
  List *this, *new;

  if (!item) return p;

  new = (List *) calloc(1, sizeof(List));
  assert(new);

  // if p == NULL then we return new as the start of a new chain

  new->item = item;
  if (!p) return new;

  // search for the end of the chain

  this = p;
  while (this->next)
    this = this->next;

  // add this string to the end of the chain

  this->next = new;

  return p;
}


int ListStrComp(void *e1, void *e2)
{
  List *a = *(List **)e1;
  List *b = *(List **)e2;

  return strcmp(a->item, b->item);
}


List *SortList(List *p, int (*comp)())
{
  int   cnt, i;
  List *next, **list;

  // count the length of the list

  cnt = 0;
  next = p;
  while (next) {
    cnt++;
    next = next->next;
  }

  if (cnt < 2) return p;

  // make and array of pointers to the list nodes

  list = (List **) malloc(cnt * sizeof(List *));
  assert(list);

  // initialize the list with pointers to the nodes

  cnt = 0;
  next = p;
  while (next) {
    list[cnt++] = next;
    next = next->next;
  }

  // order the pointers to the nodes based on the node->item

  qsort(list, cnt, sizeof(List *), comp);

  // relink the next pointers based on the sorted order

  for (i=0; i<cnt-1; i++)
    list[i]->next = list[i+1];
  list[cnt-1]->next = NULL;

  // save the head of the list to return it and free the array space

  next = list[0];
  free(list);
  return next;
}


List *CloneList(List *p, void *(*dupitem)())
{
  List *new  = NULL;
  List *last = NULL;
  List *this;

  while (p) {
    this = (List *) calloc(1, sizeof(List));
    if (dupitem)
      this->item = dupitem(p->item);
    else
      this->item = p->item;

    if (!new) new = this;
    if (last) last->next = this;
    last = this;
    p = p->next;
  }

  return new;
}


List *ReverseList(List *p)
{
  List *last, *next;

  last = NULL;
  while (p) {
    next = p->next;
    p->next = last;
    last = p;
    p = next;
  }
  return last;
}


void FreeListNotItems(List *p)
{
  List *next;

  while (p) {
    next = p->next;
    free(p);
    p = next;
  }
}


void FreeList(List *p)
{
  List *next;

  // walk the chain free all nodes as we go

  while (p) {
    next = p->next;
    if (p->item)
      free(p->item);
    free(p);
    p = next;
  }
}


int inStrList(List *p, char *n)
{
  while (p) {
    if (!strcmp(p->item, n)) return 1;
    p = p->next;
  }
  return 0;
}

/*
 * calculates Levenshtein distance between a and b
 */

#define MAXLINE 100

int lev(char a[], char b[], int icase)
{
  int arr[MAXLINE][MAXLINE];
  int i,j,l,m,n,add;
  int alen, blen, term;

  alen = (strlen(a)>MAXLINE)?MAXLINE:strlen(a);
  blen = (strlen(b)>MAXLINE)?MAXLINE:strlen(b);

  for (i=0;i<=alen;i++) {
    arr[0][i]=i;
  }
  for (j=0;j<=blen;j++) {
    arr[j][0]=j;
  }

  for (j=1;j<=blen;j++) {
    for (i=1;i<=alen;i++) {
      if (icase)
        term = tolower(a[i-1]) == tolower(b[j-1]);
      else
        term = a[i-1] == b[j-1];
      if (term)
        { add=0; } else { add=1; }
      m = 1+arr[j-1][i];
      l = 1+arr[j][i-1];
      n = add+arr[j-1][i-1];
      arr[j][i] = (m < l ? (m < n ? m : n): (l < n ? l : n));
    }
  }

  return arr[blen][alen];
} 
