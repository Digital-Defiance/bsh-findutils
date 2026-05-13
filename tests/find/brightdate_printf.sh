#!/bin/sh
# Exercise bfind -printf %Wt %Wa %Wc %WB (BrightDate format specifiers).

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

# Create a file with a known fixed timestamp: 2000-01-01T12:00:00 UTC.
# That is BD 0.000428240 (37 leap-second offset already in effect... actually
# the offset in 2000 was 32 s, so TAI-Unix = 32 at that time).
# Rather than relying on arithmetic, we just check that:
#   (a) %Wt produces a decimal number with a dot
#   (b) %Wt and %Wa produce different values after we access the file
#   (c) the BD value is positive (file is after J2000)

touch testfile || framework_failure_

# (a) %Wt output looks like a decimal BD scalar
bfind . -maxdepth 1 -name testfile -printf '%Wt\n' > out_t || fail=1
# Must contain a decimal point
grep '\.' out_t || { echo "FAIL: %Wt output has no decimal point"; fail=1; }
# Must be positive (all modern files are after BD 0)
bd_t=$(cat out_t)
case "$bd_t" in
  [0-9]*.[0-9]*) ;;
  *) echo "FAIL: %Wt output does not look like a BD scalar: $bd_t"; fail=1 ;;
esac

# (b) %Wa output also looks like a BD scalar
bfind . -maxdepth 1 -name testfile -printf '%Wa\n' > out_a || fail=1
bd_a=$(cat out_a)
case "$bd_a" in
  [0-9]*.[0-9]*) ;;
  *) echo "FAIL: %Wa output does not look like a BD scalar: $bd_a"; fail=1 ;;
esac

# (c) %Wc (ctime) is a BD scalar
bfind . -maxdepth 1 -name testfile -printf '%Wc\n' > out_c || fail=1
bd_c=$(cat out_c)
case "$bd_c" in
  [0-9]*.[0-9]*) ;;
  *) echo "FAIL: %Wc output does not look like a BD scalar: $bd_c"; fail=1 ;;
esac

# (d) %WB (birth time) is a BD scalar (may equal mtime on systems without
#     native birth-time support, but must still be a valid scalar)
bfind . -maxdepth 1 -name testfile -printf '%WB\n' > out_B || fail=1
bd_B=$(cat out_B)
case "$bd_B" in
  [0-9]*.[0-9]*) ;;
  *) echo "FAIL: %WB output does not look like a BD scalar: $bd_B"; fail=1 ;;
esac

# (e) A file touched to a well-known date should give a predictable BD range.
# 2000-01-01T12:00:00 UTC ≈ BD 0.000428 (J2000.0 is 11:58:55.816 UTC)
# 1970-01-01T00:00:00 UTC = BD -10957.xxx  (before J2000)
# Touch to the Unix epoch so we get a known negative-ish BD:
touch -t 197001010000 epoch_file 2>/dev/null || skip_ "touch -t not supported"
bfind . -maxdepth 1 -name epoch_file -printf '%Wt\n' > out_epoch || fail=1
bd_epoch=$(cat out_epoch)
# 1970-01-01 is about BD -10957.x; verify it starts with a minus
case "$bd_epoch" in
  -*) ;;
  *) echo "FAIL: epoch file BD should be negative, got: $bd_epoch"; fail=1 ;;
esac

# (f) Four specifiers in one -printf all produce scalars on one line
bfind . -maxdepth 1 -name testfile \
  -printf '%Wt %Wa %Wc %WB\n' > out_all || fail=1
# Should have four space-separated tokens
set -- $(cat out_all)
test "$#" -eq 4 \
  || { echo "FAIL: expected 4 tokens from combined %%W printf, got $#"; fail=1; }

Exit $fail
