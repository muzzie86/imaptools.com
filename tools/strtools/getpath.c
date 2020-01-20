/*********************************************************************
*
*  $Id: getpath.c,v 1.2 2006/04/19 14:18:54 woodbri Exp $
*
*  Copyright 2002, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  getpath.c
*
*  $Log: getpath.c,v $
*  Revision 1.2  2006/04/19 14:18:54  woodbri
*  Modify local copy of strpos to be imt_strpos to avoid conflicts with other libraries.
*
*  Revision 1.1  2002/08/16 14:55:41  woodbri
*  Initial revision
*
*
*  2002-08-15 sew, created     ver 0.1
*
*
**********************************************************************/

#include <pwd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif


char *expand_path(const char *path)
{
    char *newpath;
    char *pfx, *temp;
    int   pos;
    struct passwd *pw;

    if (!path || strlen(path) == 0) return NULL;

    if (path[0] != '~') return strdup(path);

    pos = imt_strpos(path, '/');

    if (strlen(path) == 1 || pos == 1) {
        pfx = getenv("HOME");
        if (!pfx) {
            // Punt. We're trying to expand ~/ but HOME isn't set
            pw = getpwuid(getuid());
            if (pw)
                pfx = pw->pw_dir;
        }
    } else {
        if (pos < 0)
            pos = strlen(path);

        temp = (char *) malloc(pos);
        strncpy(temp, &path[1], pos-1);
        temp[pos-1] = '\0';

        pw = getpwnam(temp);
        if (pw)
            pfx = pw->pw_dir;
        free(temp);
    }

    // if we failed to find an expansion, return the original path

    if (!pfx) return strdup(path);

    newpath = malloc(strlen(pfx) + strlen(&path[pos]) + 2);
    strcpy(newpath, pfx);
    if (newpath[strlen(newpath)-1] != '/' && path[pos] != '/')
        strcat(newpath, "/");
    if (newpath[strlen(newpath)-1] == '/' && path[pos] == '/')
        pos++;
    strcat(newpath, &path[pos]);

    return newpath;
}

