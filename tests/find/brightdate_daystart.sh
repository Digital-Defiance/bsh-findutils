#!/bin/sh
# Exercise bfind -daystart when using BrightDate.
#
# With BrightDate semantics, -daystart sets the reference time to the start
# of the current BD day (floor of brightdate_now()), i.e., the most recent
# whole BD integer boundary.  One BD day = exactly 86400 seconds.

# Copyright (C) 2024-2026 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

. "${srcdir=.}/tests/init.sh"; fu_path_prepend_
print_ver_ bfind

# Strategy: create two files —
#   today_file:     touched just now (should be within the current BD day)
#   yesterday_file: touched 2 BD days ago (86400*2 seconds ago)
# With -daystart -mtime 0, only today_file should match.
# With -daystart -mtime 0, only today_file should match.
# With -daystart -mtime +365, only the old_file (years ago) should match.

touch today_file || framework_failure_

# old_file is touched to 2020-01-01 00:00 (definitely > 365 BD days ago).
touch -t 202001010000 old_file 2>/dev/null \
  || skip_ "touch -t not supported on this system"

# -daystart -mtime 0: files modified in the current BD day
bfind . -maxdepth 1 \( -name today_file -o -name old_file \) \
  -daystart -mtime 0 -printf '%f\n' | sort > out_today || fail=1

printf '%s\n' today_file > exp_today
compare exp_today out_today \
  || { echo "FAIL: -daystart -mtime 0 did not match only today_file"; fail=1; }

# -daystart -mtime +365: old_file (years ago) should match; today_file must not
bfind . -maxdepth 1 \( -name today_file -o -name old_file \) \
  -daystart -mtime +365 -printf '%f\n' | sort > out_old || fail=1

printf '%s\n' old_file > exp_old
compare exp_old out_old \
  || { echo "FAIL: -daystart -mtime +365 did not match only old_file"; fail=1; }

# Sanity: -daystart alone should not crash or produce errors
bfind . -maxdepth 1 -daystart -name today_file > /dev/null || fail=1

Exit $fail
