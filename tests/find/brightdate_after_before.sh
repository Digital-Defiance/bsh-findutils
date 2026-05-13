#!/bin/sh
# Exercise bfind -after / -before (BrightDate scalar predicates).

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

# Strategy:
# 1. Create two files with fixed known timestamps:
#    - "old_file": touched to 2001-01-01 00:00 UTC  → BD ≈ 366.5
#    - "new_file": touched to 2010-01-01 00:00 UTC  → BD ≈ 3653.5
# 2. Use a threshold BD between them (BD 2000.0 ≈ 2005-06-18) to verify that
#    -after 2000.0 finds only new_file, and -before 2000.0 finds only old_file.

touch -t 200101010000 old_file 2>/dev/null \
  || skip_ "touch -t not supported on this system"
touch -t 201001010000 new_file

# -after 2000.0 should match new_file but NOT old_file
bfind . -maxdepth 1 \( -name old_file -o -name new_file \) \
  -after 2000.0 -printf '%f\n' | sort > out_after || fail=1

printf '%s\n' new_file > exp_after
compare exp_after out_after || { echo "FAIL: -after 2000.0 mismatch"; fail=1; }

# -before 2000.0 should match old_file but NOT new_file
bfind . -maxdepth 1 \( -name old_file -o -name new_file \) \
  -before 2000.0 -printf '%f\n' | sort > out_before || fail=1

printf '%s\n' old_file > exp_before
compare exp_before out_before || { echo "FAIL: -before 2000.0 mismatch"; fail=1; }

# Boundary: -after X on a file exactly AT X should NOT match (strictly greater)
# Touch a file to correspond to BD 2000.0:
#   BD 2000.0 → TAI_unix = 2000*86400 + 946727967.816 = 1119927967.816
#   UTC offset in 2005 = 32 s → UTC_unix = 1119927935.816 ≈ 1119927936
# touch -d @N is a GNU/BSD extension; skip this sub-test if unsupported.
if touch -d @1119927936 boundary_file 2>/dev/null; then
  # -after 2000.0 should NOT match a file AT BD 2000.0 (strictly greater)
  bfind . -maxdepth 1 -name boundary_file -after 2000.0 \
    -printf '%f\n' > out_boundary_after || fail=1
  test -s out_boundary_after \
    && { echo "FAIL: -after 2000.0 matched file AT BD 2000.0"; fail=1; }

  # -before 2000.0 SHOULD match a file slightly before BD 2000.0
  bfind . -maxdepth 1 -name boundary_file -before 2000.0001 \
    -printf '%f\n' > out_boundary_before || fail=1
  printf '%s\n' boundary_file > exp_boundary
  compare exp_boundary out_boundary_before \
    || { echo "FAIL: -before 2000.0001 did not match file at BD ~2000.0"; fail=1; }
fi

# Verify that -after / -before also accept ISO datetime strings (fall-through)
bfind . -maxdepth 1 \( -name old_file -o -name new_file \) \
  -after "2005-01-01" -printf '%f\n' | sort > out_iso || fail=1
compare exp_after out_iso \
  || { echo "FAIL: -after with ISO date mismatch"; fail=1; }

Exit $fail
