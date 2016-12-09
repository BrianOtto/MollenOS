/*
* gmtime_r.c
* Original Author: Adapted from tzcode maintained by Arthur David Olson.
* Modifications:
* - Changed to mktm_r and added __tzcalc_limits - 04/10/02, Jeff Johnston
* - Fixed bug in mday computations - 08/12/04, Alex Mogilnikov <alx@intellectronika.ru>
* - Fixed bug in __tzcalc_limits - 08/12/04, Alex Mogilnikov <alx@intellectronika.ru>
* - Move code from _mktm_r() to gmtime_r() - 05/09/14, Freddie Chopin <freddie_chopin@op.pl>
* - Fixed bug in calculations for dates after year 2069 or before year 1901. Ideas for
*   solution taken from musl's __secs_to_tm() - 07/12/2014, Freddie Chopin
*   <freddie_chopin@op.pl>
* - Use faster algorithm from civil_from_days() by Howard Hinnant - 12/06/2014,
* Freddie Chopin <freddie_chopin@op.pl>
*
* Converts the calendar time pointed to by tim_p into a broken-down time
* expressed as local time. Returns a pointer to a structure containing the
* broken-down time.
*/

/*
FUNCTION
<<gmtime>>---convert time to UTC traditional form

INDEX
	gmtime
INDEX
	gmtime_r

ANSI_SYNOPSIS
	#include <time.h>
	struct tm *gmtime(const time_t *<[clock]>);
	struct tm *gmtime_r(const time_t *<[clock]>, struct tm *<[res]>);

TRAD_SYNOPSIS
	#include <time.h>
	struct tm *gmtime(<[clock]>)
	const time_t *<[clock]>;
	struct tm *gmtime_r(<[clock]>, <[res]>)
	const time_t *<[clock]>;
	struct tm *<[res]>;

DESCRIPTION
<<gmtime>> takes the time at <[clock]> representing the number
of elapsed seconds since 00:00:00 on January 1, 1970, Universal
Coordinated Time (UTC, also known in some countries as GMT,
Greenwich Mean time) and converts it to a <<struct tm>>
representation.

<<gmtime>> constructs the traditional time representation in static
storage; each call to <<gmtime>> or <<localtime>> will overwrite the
information generated by previous calls to either function.

RETURNS
A pointer to the traditional time representation (<<struct tm>>).

PORTABILITY
ANSI C requires <<gmtime>>.

<<gmtime>> requires no supporting OS subroutines.
*/

/* Includes */
#include <os/thread.h>
#include <stdlib.h>
#include "local.h"

#define _GMT_OFFSET 0

/* Move epoch from 01.01.1970 to 01.03.0000 (yes, Year 0) - this is the first
* day of a 400-year long "era", right after additional day of leap year.
* This adjustment is required only for date calculation, so instead of
* modifying time_t value (which would require 64-bit operations to work
* correctly) it's enough to adjust the calculated number of days since epoch.
*/
#define EPOCH_ADJUSTMENT_DAYS	719468L
/* year to which the adjustment was made */
#define ADJUSTED_EPOCH_YEAR	0
/* 1st March of year 0 is Wednesday */
#define ADJUSTED_EPOCH_WDAY	3
/* there are 97 leap years in 400-year periods. ((400 - 97) * 365 + 97 * 366) */
#define DAYS_PER_ERA		146097L
/* there are 24 leap years in 100-year periods. ((100 - 24) * 365 + 24 * 366) */
#define DAYS_PER_CENTURY	36524L
/* there is one leap year every 4 years */
#define DAYS_PER_4_YEARS	(3 * 365 + 366)
/* number of days in a non-leap year */
#define DAYS_PER_YEAR		365
/* number of days in January */
#define DAYS_IN_JANUARY		31
/* number of days in non-leap February */
#define DAYS_IN_FEBRUARY	28
/* number of years per era */
#define YEARS_PER_ERA		400

/* Reentrency version of gmtime 
 * Modified implementation by newlib */
struct tm *gmtime_r(__CRT_CONST time_t *__restrict tim_p, struct tm *__restrict res)
{
	long days, rem;
	__CRT_CONST time_t lcltime = *tim_p;
	int era, weekday, year;
	unsigned erayear, yearday, month, day;
	unsigned long eraday;

	days = ((long)lcltime) / SECSPERDAY + EPOCH_ADJUSTMENT_DAYS;
	rem = ((long)lcltime) % SECSPERDAY;
	if (rem < 0)
	{
		rem += SECSPERDAY;
		--days;
	}

	/* compute hour, min, and sec */
	res->tm_hour = (int)(rem / SECSPERHOUR);
	rem %= SECSPERHOUR;
	res->tm_min = (int)(rem / SECSPERMIN);
	res->tm_sec = (int)(rem % SECSPERMIN);

	/* compute day of week */
	if ((weekday = ((ADJUSTED_EPOCH_WDAY + days) % DAYSPERWEEK)) < 0)
		weekday += DAYSPERWEEK;
	res->tm_wday = weekday;

	/* compute year, month, day & day of year */
	/* for description of this algorithm see
	* http://howardhinnant.github.io/date_algorithms.html#civil_from_days */
	era = (days >= 0 ? days : days - (DAYS_PER_ERA - 1)) / DAYS_PER_ERA;
	eraday = days - era * DAYS_PER_ERA;	/* [0, 146096] */
	erayear = (eraday - eraday / (DAYS_PER_4_YEARS - 1) + eraday / DAYS_PER_CENTURY -
		eraday / (DAYS_PER_ERA - 1)) / 365;	/* [0, 399] */
	yearday = eraday - (DAYS_PER_YEAR * erayear + erayear / 4 - erayear / 100);	/* [0, 365] */
	month = (5 * yearday + 2) / 153;	/* [0, 11] */
	day = yearday - (153 * month + 2) / 5 + 1;	/* [1, 31] */
	month += month < 10 ? 2 : -10;
	year = ADJUSTED_EPOCH_YEAR + erayear + era * YEARS_PER_ERA + (month <= 1);

	res->tm_yday = yearday >= DAYS_PER_YEAR - DAYS_IN_JANUARY - DAYS_IN_FEBRUARY ?
		yearday - (DAYS_PER_YEAR - DAYS_IN_JANUARY - DAYS_IN_FEBRUARY) :
		yearday + DAYS_IN_JANUARY + DAYS_IN_FEBRUARY + isleap(erayear);
	res->tm_year = year - YEAR_BASE;
	res->tm_mon = month;
	res->tm_mday = day;

	res->tm_isdst = 0;

	return (res);
}

/* Normal version of gmtime 
 * Modified implementation by newlib */
struct tm *gmtime(__CRT_CONST time_t *tim_p)
{
	/* Get the TLS buffer for
	* time-modification */
	struct tm *gmbuf = &(TLSGetCurrent()->TmBuffer);

	/* Reset the buffer */
	memset(gmbuf, 0, sizeof(struct tm));

	/* Return the reentrency version */
	return gmtime_r(tim_p, gmbuf);
}
