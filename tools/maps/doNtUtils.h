
#define FWID_RTE_NUM 8
#define FWID_RTE_P   3

#define UC(a) (((a) > '\140' && (a) < '\173')?((a)-'\40'):\
               ((a) > '\337' && (a) < '\376')?((a)-'\40'):(a))
#define LC(a) (((a) > '\100' && (a) < '\133')?((a)+'\40'):\
               ((a) > '\277' && (a) < '\336')?((a)+'\40'):(a))
#define MIN(a,b) ((a)<(b)?(a):(b))

#include "rgxtools.h"

typedef struct {
    double number;
    long   index;
} sortStruct;

void mkFileName(char *base, char *ext, char *buf);
const char* wordCase(const char *in);
const char* u_wordCase(const char *in);
int cmp_num_desc(const void *i, const void *j);
int cmp_num_asc(const void *i, const void *j);
sortStruct* sortDBF(DBFHandle dbfH, char *fldname, int (*compar)(const void *e1, const void *e2));
int extractRouteNum(int region, char *rte_n, char *rte_p, const char *name);
char *getStyle(int region, int fc, int tolls, char *extra, char *pre);
char *getShield(int region, int rt, int fc, char *pre, int tc);
