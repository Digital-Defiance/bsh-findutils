/* brightdate.c -- BrightDate time conversions for bfind/blocate.
   Copyright (C) 2024-2025 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* We always include config.h first. */
#include <config.h>

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "brightdate.h"

/* ---------------------------------------------------------------------------
 * Leap-second table.
 *
 * Source : IERS Bulletin C / IANA leap-seconds.list
 * Reviewed : 2026-05-10
 * Valid until: 2028-06-28T00:00:00Z (Unix s 1_845_129_600)
 *
 * Each entry is (utc_unix_seconds, cumulative_TAI−UTC_offset).
 * The entry is effective from utc_unix_seconds onward.
 * ---------------------------------------------------------------------------*/
static const struct
{
  long utc_s;
  int  offset;
} leap_table[] =
{
  {    63072000, 10 },  /* 1972-01-01 */
  {    78796800, 11 },  /* 1972-07-01 */
  {    94694400, 12 },  /* 1973-01-01 */
  {   126230400, 13 },  /* 1974-01-01 */
  {   157766400, 14 },  /* 1975-01-01 */
  {   189302400, 15 },  /* 1976-01-01 */
  {   220924800, 16 },  /* 1977-01-01 */
  {   252460800, 17 },  /* 1978-01-01 */
  {   283996800, 18 },  /* 1979-01-01 */
  {   315532800, 19 },  /* 1980-01-01 */
  {   362793600, 20 },  /* 1981-07-01 */
  {   394329600, 21 },  /* 1982-07-01 */
  {   425865600, 22 },  /* 1983-07-01 */
  {   489024000, 23 },  /* 1985-07-01 */
  {   567993600, 24 },  /* 1988-01-01 */
  {   631152000, 25 },  /* 1990-01-01 */
  {   662688000, 26 },  /* 1991-01-01 */
  {   709948800, 27 },  /* 1992-07-01 */
  {   741484800, 28 },  /* 1993-07-01 */
  {   773020800, 29 },  /* 1994-07-01 */
  {   820454400, 30 },  /* 1996-01-01 */
  {   867715200, 31 },  /* 1997-07-01 */
  {   915148800, 32 },  /* 1999-01-01 */
  {  1136073600, 33 },  /* 2006-01-01 */
  {  1230768000, 34 },  /* 2009-01-01 */
  {  1341100800, 35 },  /* 2012-07-01 */
  {  1435708800, 36 },  /* 2015-07-01 */
  {  1483228800, 37 },  /* 2017-01-01 */
};

#define LEAP_TABLE_LEN  (sizeof (leap_table) / sizeof (leap_table[0]))

/* Return the TAI−UTC offset (seconds) that applies at the given UTC Unix
   timestamp.  Uses binary search on the leap-second table (O(log n)). */
static int
get_tai_utc_offset (long utc_s)
{
  int lo, hi, mid;

  if (LEAP_TABLE_LEN == 0 || utc_s < leap_table[0].utc_s)
    return 10;  /* pre-1972: use the initial 10 s offset as an approximation */

  lo = 0;
  hi = (int)(LEAP_TABLE_LEN - 1);

  while (lo < hi)
    {
      mid = lo + (hi - lo + 1) / 2;
      if (leap_table[mid].utc_s <= utc_s)
        lo = mid;
      else
        hi = mid - 1;
    }

  return leap_table[lo].offset;
}

/* ---------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------------*/

/* Convert a BrightDate scalar to a UTC struct timespec.
 *
 * Steps:
 *   1. bd  → TAI Unix seconds: tai = bd * 86400 + J2000_TAI_UNIX_S
 *   2. TAI → UTC:  probe with the maximum known offset (37 s, post-2017)
 *      to get an approximate UTC second; look up the real offset at that
 *      approximate UTC; subtract it.  This gives the correct UTC Unix
 *      second for all normal (non-leap-second) instants.
 */
struct timespec
bd_to_timespec (double bd)
{
  struct timespec ts;
  double tai_unix_s, utc_s, whole, frac;
  long   probe_utc;
  int    offset;

  tai_unix_s = bd * BD_SECS_PER_DAY + J2000_TAI_UNIX_S;

  /* Probe with the largest known offset so the lookup lands in the
     right era of the table. */
  probe_utc = (long)(tai_unix_s - 37.0);
  offset    = get_tai_utc_offset (probe_utc);
  utc_s     = tai_unix_s - (double)offset;

  frac = modf (utc_s, &whole);
  ts.tv_sec  = (time_t)whole;
  ts.tv_nsec = (long)(frac * 1.0e9);

  /* Normalise: tv_nsec must be non-negative. */
  if (ts.tv_nsec < 0)
    {
      ts.tv_nsec += 1000000000L;
      ts.tv_sec--;
    }

  return ts;
}

/* Convert a UTC struct timespec to a BrightDate scalar.
 *
 * Steps:
 *   1. Look up TAI−UTC offset for the UTC second in ts.tv_sec.
 *   2. utc_s → TAI Unix seconds: tai = utc_s + offset
 *   3. TAI → bd: (tai - J2000_TAI_UNIX_S) / 86400
 */
double
timespec_to_bd (struct timespec ts)
{
  int    offset;
  double utc_s, tai_s;

  offset = get_tai_utc_offset ((long)ts.tv_sec);
  utc_s  = (double)ts.tv_sec + (double)ts.tv_nsec * 1.0e-9;
  tai_s  = utc_s + (double)offset;

  return (tai_s - J2000_TAI_UNIX_S) / BD_SECS_PER_DAY;
}

/* Return true if STR looks like a bare BrightDate numeric literal.
   Accepts an optional leading '+' or '-', followed by one or more ASCII
   decimal digits, optionally followed by a '.' and more decimal digits,
   with no trailing characters.  Rejects anything that contains letters,
   colons, or any other marker that would appear in an ISO 8601 date. */
bool
is_brightdate_literal (const char *str)
{
  if (str == NULL || *str == '\0')
    return false;

  if (*str == '+' || *str == '-')
    str++;

  if (!isdigit ((unsigned char)*str))
    return false;

  while (isdigit ((unsigned char)*str))
    str++;

  if (*str == '.')
    {
      str++;
      while (isdigit ((unsigned char)*str))
        str++;
    }

  return (*str == '\0');
}

/* Parse STR as a BrightDate literal and store the value in *RESULT.
   Returns true on success; false if STR is not a valid BD literal. */
bool
parse_brightdate_literal (const char *str, double *result)
{
  char  *endptr;
  double val;

  if (!is_brightdate_literal (str))
    return false;

  val = strtod (str, &endptr);
  if (endptr == str || *endptr != '\0')
    return false;

  *result = val;
  return true;
}

/* Return the current wall-clock time as a BrightDate scalar. */
double
brightdate_now (void)
{
  struct timespec ts;

#if defined HAVE_CLOCK_GETTIME
  clock_gettime (CLOCK_REALTIME, &ts);
#else
  ts.tv_sec  = time (NULL);
  ts.tv_nsec = 0;
#endif

  return timespec_to_bd (ts);
}
