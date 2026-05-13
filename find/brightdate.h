/* brightdate.h -- BrightDate time system support for bfind/blocate.
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
   along with this program.  If not, see <https://www.gnu.org/licenses/>.

   BrightDate is decimal SI days since J2000.0 (2000-01-01T12:00:00 TT),
   measured on the TAI timescale.  Leap seconds only appear at UTC
   boundaries; the BrightDate timeline itself has no discontinuities.

   Conversion formulae:
     bd          = (tai_unix_s - J2000_TAI_UNIX_S) / BD_SECS_PER_DAY
     tai_unix_s  = bd * BD_SECS_PER_DAY + J2000_TAI_UNIX_S
     utc_unix_s  = tai_unix_s - tai_utc_offset(utc_unix_s)

   Example: bd 9628.4 ≈ 2026-05-08 (approx.)
*/

#ifndef INC_BRIGHTDATE_H
#define INC_BRIGHTDATE_H 1

#include <stdbool.h>
#include <time.h>

/* J2000.0 expressed in TAI Unix seconds (2000-01-01T11:59:27.816 TAI). */
#define J2000_TAI_UNIX_S   946727967.816

/* Seconds per SI day. */
#define BD_SECS_PER_DAY    86400.0

/* Convert a BrightDate scalar (decimal days since J2000.0) to a UTC
   struct timespec.  The result is a standard POSIX/UTC Unix timestamp
   suitable for comparison with struct stat timestamps. */
struct timespec bd_to_timespec (double bd);

/* Convert a UTC struct timespec to a BrightDate scalar. */
double timespec_to_bd (struct timespec ts);

/* Return true if STR looks like a bare BrightDate numeric literal:
   an optional leading sign (+/-), at least one ASCII digit, an optional
   decimal point followed by more digits, and nothing else.  Strings with
   letters, colons, dashes, or any other character that would be present
   in an ISO 8601 / RFC 3339 date string return false. */
bool is_brightdate_literal (const char *str);

/* Parse STR as a BrightDate literal and store the result in *RESULT.
   Returns true on success; false if STR is not a valid BD literal. */
bool parse_brightdate_literal (const char *str, double *result);

/* Return the current wall-clock time as a BrightDate scalar. */
double brightdate_now (void);

#endif /* INC_BRIGHTDATE_H */
