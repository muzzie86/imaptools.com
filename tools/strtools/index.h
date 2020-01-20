/*********************************************************************
*
*  $Id: index.h,v 2.4 2005/12/22 04:49:07 woodbri Exp $
*
*  TigerToShape - t2s                                                 
*  Copyright 2001, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
**********************************************************************
*
*  index.h
*
*  $Log: index.h,v $
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
*********************************************************************/          

#ifdef WIN32
#define VOID char
#else
#define VOID void
#endif

typedef struct structIndex {

  FILE       *fH;
  int        (*comp)();  // pointer to compare function
  VOID       *id;     // pointer to start of index entries
  int         num;    // num of entries in the index
  int         kSize;  // sizeof(key)
  int         eSize;  // sizeof(entry struct)
  int         ready;  // true when index is usable, false while building
  int         max;    // max number of entries allocated
  VOID       *p;      // pointer to next entry while building

} Index;



/*  Index Functions  */

Index *InitIndex(FILE *h, int s_key, int s_entry, int (*comp)());
void AddToIndex(Index *idx, VOID *entry);
void ReadyIndex(Index *idx);
VOID *FetchFromIndex(Index *idx, VOID *key);
VOID *FirstIndex(Index *idx, VOID *key);
VOID *NextIndex(Index *idx, VOID *last);
int CountIndex(Index *idx, VOID *key);
void FreeIndex(Index *idx);
int IntComp(const VOID *e1, const VOID *e2);
int StrComp(const VOID *e1, const VOID *e2);

void DumpIndex(Index *idx, char *fname, char * (*EntryToString)());

