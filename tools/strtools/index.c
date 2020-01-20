/*********************************************************************
*
*  $Id: index.c,v 2.4 2005/12/22 04:49:07 woodbri Exp $
*
*  TigerToShape - t2s
*  Copyright 2001, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
*********************************************************************
*
*  index.c
*
*  $Log: index.c,v $
*  Revision 2.4  2005/12/22 04:49:07  woodbri
*  Added new function to CountIndex() to count the number of index entries
*  for a given key.
*
*  Revision 2.3  2003/04/19 15:29:45  woodbri
*  *** empty log message ***
*
*  Revision 2.2  2002/08/16 14:55:41  woodbri
*  *** empty log message ***
*
*  Revision 2.1  2001/11/29 18:13:14  woodbri
*  Promote stable 1.x to 2.0 baseline
*
*  2001-11-09 sew, created     ver 0.1
*
*********************************************************************
*
*  routines for working with indexes in tiger to shape
*
*********************************************************************/

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "index.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

typedef struct DummyIEstruct {

  char  *key;
  char   dummy[8];

} DummyIE;

#define INDEX_CHUNK 1000

Index *InitIndex(FILE *h, int s_key, int s_entry, int (*comp)())
{
  Index   *this;
  char    *err;
  char     line[512];

  this = (Index *) malloc(sizeof(Index));
  assert(this != NULL);

  this->fH    = h;
  this->comp  = comp;
  this->ready = 0;
  this->p     = 0L;
  this->num   = 0;
  this->kSize = s_key;
  this->eSize = s_entry;

  if (h) {
    rewind(h);
    err = fgets(line, 511, h);
    assert(err != NULL);
    assert(line[strlen(line)-1] == '\n');

    fseek(h, 0L, SEEK_END);
    this->max = ftell(h) / strlen(line);
  } else {
    this->max = INDEX_CHUNK;
  }

  this->id = (VOID *) malloc(this->max * s_entry);
  assert(this->id != NULL);

  this->p = this->id;

  return this;
}



void AddToIndex(Index *idx, VOID *entry)
{
  int offset;

  assert(idx);
  assert(idx->ready == 0);

  if (idx->num >= idx->max) {
    offset = idx->p - idx->id;
    idx->max += INDEX_CHUNK;
    idx->id = (VOID *) realloc(idx->id, idx->max * idx->eSize);
    assert(idx->id);
    idx->p = idx->id + offset;
  }

  memcpy(idx->p, entry, idx->eSize);
  idx->p += idx->eSize;
  idx->num++;
}



void ReadyIndex(Index *idx)
{

  if (!idx) return;

  idx->ready = 1;
  qsort(idx->id, (size_t) idx->num, idx->eSize, idx->comp);

}



VOID *FetchFromIndex(Index *idx, VOID *key)
{
  if (!idx) return NULL;

  return bsearch(key, idx->id, idx->num, idx->eSize, idx->comp);
}



VOID *FirstIndex(Index *idx, VOID *key)
{
  VOID *p;

  if (!idx) return NULL;

  p = bsearch(key, idx->id, idx->num, idx->eSize, idx->comp);

  while (p && ((p - idx->eSize) >= idx->id) && ! idx->comp(p - idx->eSize, key))
    p -= idx->eSize;

  return p;
}



VOID *NextIndex(Index *idx, VOID *last)
{
  if (!idx) return NULL;

  if ( last && ((last - idx->id)/idx->eSize < idx->num -1) &&
       !idx->comp(last + idx->eSize, last) )
    return last + idx->eSize;
  else
    return NULL;
}

int CountIndex(Index *idx, VOID *key)
{
  VOID *p;
  int cnt = 1;

  if (!idx) return 0;  /* none here if no index defined */

  p = bsearch(key, idx->id, idx->num, idx->eSize, idx->comp);

  while (p && ((p - idx->eSize) >= idx->id) && ! idx->comp(p - idx->eSize, key))    p -= idx->eSize;

  if (!p) return 0;  /* none here if bsearch fails */

  while (p && ((p - idx->id)/idx->eSize < idx->num -1) &&
         !idx->comp(p + idx->eSize, p) ) {
    cnt++;
    p = p + idx->eSize;
  }
  return cnt;  /* return how many we counted */
}


void DumpIndex(Index *idx, char *fname, char * (*EntryToString)())
{
  int   i;
  FILE *h = NULL;

  if (fname)
    h = fopen(fname, "w");

  for (i=0; i<idx->num; i++)
    if (h)
      fprintf(h, "%s\n", EntryToString(idx->id + i*idx->eSize));
    else
      printf("%s\n", EntryToString(idx->id + i*idx->eSize));

  if (h) fclose(h);
}


void FreeIndex(Index *idx)
{
  if (!idx) return;
  if (idx->id) free(idx->id);
  free(idx);
}


int IntComp(const VOID *e1, const VOID *e2)
{
  int v1 = *(int *)e1;
  int v2 = *(int *)e2;
  return (v1<v2) ? -1 : (v1>v2) ? 1 : 0;
}


int StrComp(const VOID *e1, const VOID *e2)
{
  DummyIE *v1 = (DummyIE *)e1;
  DummyIE *v2 = (DummyIE *)e2;
  return strcmp(v1->key, v2->key);
}
