/**********************************************************************/
/*                                                                    */
/*  Copyright 2001, Stephen Woodbridge, All rights Reserved           */
/*  Redistribition without written permission strictly prohibited     */
/*                                                                    */
/**********************************************************************/
/*                                                                    */
/*  strtools.h                                                        */
/*                                                                    */
/*  2001-11-09 sew, created     ver 0.1                               */
/*  2001-11-24 sew, created     ver 0.2                               */
/*      put in separate dir, add ReplaceVars stuff                    */
/*  2004-01-30 sew, added lev(a,b) to compute Levenshtein distance    */
/*                  added strcmpic(a,b,c)                             */
/*                                                                    */
/*                                                                    */
/**********************************************************************/


#define VARS_CHUNK 10

typedef struct structVar {

  char   *name;
  char   *val;
  int    free;

} Var;

typedef struct structVars {

  int  num;
  int  nchunk;
  Var *v;

} Vars;


typedef struct structList {

  void *item;
  struct structList *next;

} List;


void chomp(char *str);
char *trim(char *str);
int  toint(int c);
int cmpfld(char *buf, int pos, int len, int icase, char *match);

#if defined(UNIX) && !defined(__CYGWIN__)
int strncmpi(const char *a, const char *b, int n);
int strcmpi(const char *a, const char *b);
#endif

int strcmpic(char *a, char *b, char c);
void strmcpy(char *d, const char *s, int maxc);
void sstrcpy(char *d, const char *s);
int imt_strpos(const char *s, char c);

Vars *NewVars();
Var *GetAVar(Vars *v, char *name);
Vars *SetAVar(Vars *v, char *name, char *val, int dofree);
void ClearVars(Vars *v);
char *ReplaceVars(Vars *v, char *template, int encode);

char *xml_encode(char *str);
char *url_encode(char *str);

List *AddToList(List *p, void *str);
int ListStrComp(void *e1, void *e2);
List *SortList(List *p, int (*comp)());
void FreeList(List *p);
int inStrList(List *p, char *n);

char *expand_path(char *p);
int lev(char a[], char b[], int icase);

