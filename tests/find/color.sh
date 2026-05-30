#!/bin/sh
# Exercise bfind terminal colorization.

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

fail=0

has_ansi () { LC_ALL=C grep -q $'\033' "$1"; }
strip_ansi () { sed $'s/\033\\[[0-9;]*m//g'; }
grep_fixed () { LC_ALL=C grep -qF "$2" "$1"; }

expect_exit_2 () {
  _cmd=$1
  _err=$2
  _msg=$3
  eval "$_cmd" > /dev/null 2> "$_err"
  _status=$?
  if test "$_status" -ne 2; then
    echo "FAIL: expected exit 2 for $_msg, got $_status"
    fail=1
  fi
}

touch testfile || framework_failure_

# ── invalid color settings must fail cleanly (exit 2) ────────────────────────

expect_exit_2 \
  'bfind --color=nope . -maxdepth 0' \
  err_bad \
  '--color=nope'
grep 'invalid color setting' err_bad > /dev/null \
  || { echo "FAIL: missing invalid color message for --color=nope"; fail=1; }

expect_exit_2 \
  'bfind --color-scheme=garbage . -maxdepth 0' \
  err_scheme \
  '--color-scheme=garbage'
grep 'invalid color scheme' err_scheme > /dev/null \
  || { echo "FAIL: missing invalid color scheme message"; fail=1; }

expect_exit_2 \
  'BFIND_COLOR=nope bfind . -maxdepth 0' \
  err_env_color \
  'BFIND_COLOR=nope'
grep 'invalid color setting' err_env_color > /dev/null \
  || { echo "FAIL: missing invalid color message for BFIND_COLOR"; fail=1; }

expect_exit_2 \
  'BFIND_COLOR_SCHEME=garbage bfind . -maxdepth 0' \
  err_env_scheme \
  'BFIND_COLOR_SCHEME=garbage'
grep 'invalid color scheme' err_env_scheme > /dev/null \
  || { echo "FAIL: missing invalid color scheme message for BFIND_COLOR_SCHEME"; fail=1; }

# ── color disabled ─────────────────────────────────────────────────────────

bfind --no-color . -maxdepth 1 -name testfile -printf '%Wt %p\n' > out_no_color \
  || fail=1
if has_ansi out_no_color; then
  echo "FAIL: --no-color still emitted ANSI escapes"
  fail=1
fi

# Piped output with default (auto) color must stay plain.
bfind . -maxdepth 1 -name testfile -printf '%Wt %p\n' > out_auto_pipe \
  || fail=1
if has_ansi out_auto_pipe; then
  echo "FAIL: auto color emitted ANSI escapes on a non-TTY stdout"
  fail=1
fi

NO_COLOR=1 bfind . -maxdepth 1 -name testfile -printf '%p\n' > out_no_color_env \
  || fail=1
if has_ansi out_no_color_env; then
  echo "FAIL: NO_COLOR did not suppress auto colorization"
  fail=1
fi

# ── explicit color modes ───────────────────────────────────────────────────

bfind --color=ansi . -maxdepth 1 -name testfile -printf '%Wt %p\n' > out_ansi \
  || fail=1
if ! has_ansi out_ansi; then
  echo "FAIL: --color=ansi did not emit ANSI escapes"
  fail=1
fi

bfind --color=truecolor . -maxdepth 1 -name testfile -printf '%Wt\n' > out_truecolor \
  || fail=1
if ! grep_fixed out_truecolor '38;2;'; then
  echo "FAIL: --color=truecolor did not emit 24-bit color sequences"
  fail=1
fi
if grep_fixed out_ansi '38;2;'; then
  echo "FAIL: --color=ansi emitted truecolor sequences"
  fail=1
fi

bfind --color . -maxdepth 1 -name testfile -printf '%p\n' > out_color_flag \
  || fail=1
if ! has_ansi out_color_flag; then
  echo "FAIL: bare --color did not enable colorization"
  fail=1
fi

# Explicit --color=ansi overrides NO_COLOR (same as btime/iputils).
NO_COLOR=1 bfind --color=ansi . -maxdepth 1 -name testfile -printf '%p\n' > out_force \
  || fail=1
if ! has_ansi out_force; then
  echo "FAIL: --color=ansi did not override NO_COLOR"
  fail=1
fi

# ── color must not change semantic output ──────────────────────────────────

bfind --no-color . -maxdepth 1 -name testfile -printf '%Wt %p\n' > plain_printf \
  || fail=1
bfind --color=ansi . -maxdepth 1 -name testfile -printf '%Wt %p\n' \
  | strip_ansi > stripped_printf \
  || fail=1
compare plain_printf stripped_printf \
  || { echo "FAIL: colored -printf text differs after stripping ANSI"; fail=1; }

bfind --no-color . -maxdepth 1 -name testfile > plain_print \
  || fail=1
bfind --color=ansi . -maxdepth 1 -name testfile | strip_ansi > stripped_print \
  || fail=1
compare plain_print stripped_print \
  || { echo "FAIL: colored -print text differs after stripping ANSI"; fail=1; }

# Literal text glued to %p must survive (and stays uncolored when not plain %s).
bfind --no-color . -maxdepth 1 -name testfile -printf 'before%p\n' > plain_prefix \
  || fail=1
bfind --color=ansi . -maxdepth 1 -name testfile -printf 'before%p\n' \
  | strip_ansi > stripped_prefix \
  || fail=1
compare plain_prefix stripped_prefix \
  || { echo "FAIL: before%%p output changed with color enabled"; fail=1; }

# ── per-field ANSI palette (default scheme, 16-color) ──────────────────────

bfind --color=ansi . -maxdepth 1 -name testfile -printf '%Wt\n' > out_wt \
  || fail=1
grep_fixed out_wt $'\033[1;36m' \
  || { echo "FAIL: %%Wt missing bold mtime cyan"; fail=1; }
grep_fixed out_wt $'\033[37m' \
  || { echo "FAIL: %%Wt missing neutral fractional gray"; fail=1; }
grep_fixed out_wt $'\033[37m.' \
  || { echo "FAIL: %%Wt fractional part should use gray on the dot"; fail=1; }

bfind --color=ansi . -maxdepth 1 -name testfile -printf '%Wa\n' > out_wa \
  || fail=1
grep_fixed out_wa $'\033[1;32m' \
  || { echo "FAIL: %%Wa missing bold atime green"; fail=1; }
grep_fixed out_wa $'\033[37m' \
  || { echo "FAIL: %%Wa missing neutral fractional gray"; fail=1; }

bfind --color=ansi . -maxdepth 1 -name testfile -printf '%Wc\n' > out_wc \
  || fail=1
grep_fixed out_wc $'\033[1;33m' \
  || { echo "FAIL: %%Wc missing bold ctime yellow"; fail=1; }
grep_fixed out_wc $'\033[37m' \
  || { echo "FAIL: %%Wc missing neutral fractional gray"; fail=1; }

bfind --color=ansi . -maxdepth 1 -name testfile -printf '%WB\n' > out_wB \
  || fail=1
grep_fixed out_wB $'\033[1;35m' \
  || { echo "FAIL: %%WB missing bold birthtime magenta"; fail=1; }
grep_fixed out_wB $'\033[37m' \
  || { echo "FAIL: %%WB missing neutral fractional gray"; fail=1; }

bfind --color=ansi . -maxdepth 1 -name testfile -printf '%p\n' > out_path \
  || fail=1
grep_fixed out_path $'\033[1m' \
  || { echo "FAIL: %%p missing path highlight"; fail=1; }

# ── color scheme ───────────────────────────────────────────────────────────

bfind --color=ansi --color-scheme=default . -maxdepth 1 -name testfile \
  -printf '%Wt\n' > out_scheme_def \
  || fail=1
grep_fixed out_scheme_def $'\033[1;36m' \
  || { echo "FAIL: default scheme missing 36m mtime color"; fail=1; }

bfind --color=ansi --color-scheme=bright . -maxdepth 1 -name testfile \
  -printf '%Wt\n' > out_scheme_bright \
  || fail=1
grep_fixed out_scheme_bright $'\033[1;96m' \
  || { echo "FAIL: bright scheme missing 96m mtime color"; fail=1; }
if grep_fixed out_scheme_bright $'\033[1;36m'; then
  echo "FAIL: bright scheme still used default 36m mtime color"
  fail=1
fi

# ── color options are consumed, not passed to find as paths ────────────────

bfind -H --color=ansi . -maxdepth 1 -name testfile -print > out_leading \
  || fail=1
if ! has_ansi out_leading; then
  echo "FAIL: --color=ansi after -H did not colorize -print output"
  fail=1
fi

Exit $fail
