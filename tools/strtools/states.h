/*********************************************************************
*
*  $Id: states.h,v 2.0 2001/12/03 03:48:27 woodbri Exp $
*
*  GeoCoder
*  Copyright 2001, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  states.h
*
*  $Log: states.h,v $
*  Revision 2.0  2001/12/03 03:48:27  woodbri
*  Changes to use metaphone keyed indexes.
*
*  2001-10-25 sew, rewrite     ver 0.2
*  2001-10-18 sew, created     ver 0.1
*
*
**********************************************************************/

#define ST_NUM 79

extern char *st_name[ST_NUM];
extern char *st_abbr[ST_NUM];

char *st_name_to_abbr(char *name);
char *st_abbr_to_name(char *abbr);
int st_abbr_to_fips(char *abbr);

