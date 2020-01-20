/*********************************************************************
*
*  $Id: states.c,v 2.2 2003/04/19 15:29:45 woodbri Exp $
*
*  GeoCoder
*  Copyright 2001, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  states.c
*
*  $Log: states.c,v $
*  Revision 2.2  2003/04/19 15:29:45  woodbri
*  *** empty log message ***
*
*  Revision 2.1  2002/08/16 14:55:41  woodbri
*  *** empty log message ***
*
*  Revision 2.0  2001/12/03 03:48:27  woodbri
*  Changes to use metaphone keyed indexes.
*
*  2001-10-25 sew, rewrite     ver 0.2
*  2001-10-18 sew, created     ver 0.1
*
*
**********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "states.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define UPPER(x)  (((x) > 0x60 && (x) < 0x7b)?(x)-32:(x))


char *st_name[ST_NUM] = {

  /* 0 */ "",
  /* 1 */ "Alabama",
  /* 2 */ "Alaska",
  /* 3 */ "",
  /* 4 */ "Arizona",
  /* 5 */ "Arkansas",
  /* 6 */ "California",
  /* 7 */ "",
  /* 8 */ "Colorado",
  /* 9 */ "Connecticut",
  /* 10 */ "Delaware",
  /* 11 */ "District of Columbia",
  /* 12 */ "Florida",
  /* 13 */ "Georgia",
  /* 14 */ "",
  /* 15 */ "Hawaii",
  /* 16 */ "Idaho",
  /* 17 */ "Illinois",
  /* 18 */ "Indiana",
  /* 19 */ "Iowa",
  /* 20 */ "Kansas",
  /* 21 */ "Kentucky",
  /* 22 */ "Louisiana",
  /* 23 */ "Maine",
  /* 24 */ "Maryland",
  /* 25 */ "Massachusetts",
  /* 26 */ "Michigan",
  /* 27 */ "Minnesota",
  /* 28 */ "Mississippi",
  /* 29 */ "Missouri",
  /* 30 */ "Montana",
  /* 31 */ "Nebraska",
  /* 32 */ "Nevada",
  /* 33 */ "New Hampshire",
  /* 34 */ "New Jersey",
  /* 35 */ "New Mexico",
  /* 36 */ "New York",
  /* 37 */ "North Carolina",
  /* 38 */ "North Dakota",
  /* 39 */ "Ohio",
  /* 40 */ "Oklahoma",
  /* 41 */ "Oregon",
  /* 42 */ "Pennsylvania",
  /* 43 */ "",
  /* 44 */ "Rhode Island",
  /* 45 */ "South Carolina",
  /* 46 */ "South Dakota",
  /* 47 */ "Tennessee",
  /* 48 */ "Texas",
  /* 49 */ "Utah",
  /* 50 */ "Vermont",
  /* 51 */ "Virginia",
  /* 52 */ "",
  /* 53 */ "Washington",
  /* 54 */ "West Virginia",
  /* 55 */ "Wisconsin",
  /* 56 */ "Wyoming",
  /* 57 */ "",
  /* 58 */ "",
  /* 59 */ "",
  /* 60 */ "American Samoa",
  /* 61 */ "",
  /* 62 */ "",
  /* 63 */ "",
  /* 64 */ "Federated States of Micronesia",
  /* 65 */ "",
  /* 66 */ "Guam",
  /* 67 */ "",
  /* 68 */ "Marshall Islands",
  /* 69 */ "Northern Mariana Islands",
  /* 70 */ "Palau",
  /* 71 */ "",
  /* 72 */ "Puerto Rico",
  /* 73 */ "",
  /* 74 */ "U.S. Minor Outlying Islands",
  /* 75 */ "",
  /* 76 */ "",
  /* 77 */ "",
  /* 78 */ "Virgin Islands"
};

char *st_abbr[ST_NUM] = {

  /* 0 */ "",
  /* 1 */ "AL",
  /* 2 */ "AK",
  /* 3 */ "",
  /* 4 */ "AZ",
  /* 5 */ "AR",
  /* 6 */ "CA",
  /* 7 */ "",
  /* 8 */ "CO",
  /* 9 */ "CT",
  /* 10 */ "DE",
  /* 11 */ "DC",
  /* 12 */ "FL",
  /* 13 */ "GA",
  /* 14 */ "",
  /* 15 */ "HI",
  /* 16 */ "ID",
  /* 17 */ "IL",
  /* 18 */ "IN",
  /* 19 */ "IA",
  /* 20 */ "KS",
  /* 21 */ "KY",
  /* 22 */ "LA",
  /* 23 */ "ME",
  /* 24 */ "MD",
  /* 25 */ "MA",
  /* 26 */ "MI",
  /* 27 */ "MN",
  /* 28 */ "MS",
  /* 29 */ "MO",
  /* 30 */ "MT",
  /* 31 */ "NE",
  /* 32 */ "NV",
  /* 33 */ "NH",
  /* 34 */ "NJ",
  /* 35 */ "NM",
  /* 36 */ "NY",
  /* 37 */ "NC",
  /* 38 */ "ND",
  /* 39 */ "OH",
  /* 40 */ "OK",
  /* 41 */ "OR",
  /* 42 */ "PA",
  /* 43 */ "",
  /* 44 */ "RI",
  /* 45 */ "SC",
  /* 46 */ "SD",
  /* 47 */ "TN",
  /* 48 */ "TX",
  /* 49 */ "UT",
  /* 50 */ "VT",
  /* 51 */ "VA",
  /* 52 */ "",
  /* 53 */ "WA",
  /* 54 */ "WV",
  /* 55 */ "WI",
  /* 56 */ "WY",
  /* 57 */ "",
  /* 58 */ "",
  /* 59 */ "",
  /* 60 */ "AS",
  /* 61 */ "",
  /* 62 */ "",
  /* 63 */ "",
  /* 64 */ "FM",
  /* 65 */ "",
  /* 66 */ "GU",
  /* 67 */ "",
  /* 68 */ "MH",
  /* 69 */ "MP",
  /* 70 */ "PW",
  /* 71 */ "",
  /* 72 */ "PR",
  /* 73 */ "",
  /* 74 */ "",
  /* 75 */ "",
  /* 76 */ "",
  /* 77 */ "",
  /* 78 */ "VI"
};

int st_abbr_to_fips(char *abbr)
{
  int i;

  if (abbr && strlen(abbr) == 2) {

    for (i=0; i<ST_NUM; i++)
      if (!strcmpi(abbr, st_abbr[i]))
        return i;
  }
  return -1;
}


char *st_name_to_abbr(char *name)
{
  int i;

  if (name) {
    for (i=0; i<ST_NUM; i++)
      if (!strcmpi(name, st_name[i]))
        return st_abbr[i];
  }
  return NULL;
}


char *st_abbr_to_name(char *abbr)
{
  int i;

  i = st_abbr_to_fips(abbr);
  if (i<0)
    return NULL;
  else
    return st_name[i];
}

