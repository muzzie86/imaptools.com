#define ERR_DBFOPEN_FAILED  -1
#define ERR_DBF_UNKNOWN_COL -2
#define ERR_DBF_SQLEXEC     -3
#define ERR_DBF_SQLPREP     -4
#define ERR_DBF_SQLBIND     -5
#define ERR_DBF_SQLSTEP     -6
#define ERR_DBF_SQLRESET    -7
#define ERR_DBF_SQLFINALIZE -8
#define ERR_DBF_SQLCOMMIT   -9

int loadDBF(sqlite3 *db, char *dbfname, char *tablename, int ncol, const char **colnames, char *recno, int drop);
int loadDBFrename(sqlite3 *db, char *dbfname, char *tablename, int ncol, const char **colsin, const char **colsout, char *recno, int drop);
