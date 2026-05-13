# bright-findutils

A fork of [GNU findutils 4.10.0](https://www.gnu.org/software/findutils/) that
replaces Unix timestamps with **BrightDate** scalars throughout `bfind`,
`blocate`, `bupdatedb`, and `bxargs`.

All tools are renamed with a `b` prefix so they coexist with standard
`find`/`locate`/`xargs` installations.

---

## Table of Contents

- [What is BrightDate?](#what-is-brightdate)
- [Tools](#tools)
- [Building](#building)
- [BrightDate in bfind](#brightdate-in-bfind)
  - [-after and -before](#-after-and--before)
  - [%W printf format](#w-printf-format)
  - [-daystart](#-daystart)
  - [Time predicates accepting BD scalars](#time-predicates-accepting-bd-scalars)
- [BrightDate in blocate / bupdatedb](#brightdate-in-blocate--bupdatedb)
- [BrightDate API (C)](#brightdate-api-c)
- [Tests](#tests)
- [Original findutils README](#original-findutils-readme)

---

## What is BrightDate?

BrightDate is a **monotonic decimal day counter** rooted at the J2000.0
astronomical epoch (2000-01-01T11:58:55.816 UTC, or exactly
2000-01-01T12:00:00.000 TAI).

### Formula

$$
\text{BD} = \frac{t_\text{TAI} - 946{,}727{,}967.816}{86400}
$$

where $t_\text{TAI}$ is the TAI Unix timestamp (seconds since 1970-01-01
00:00:00 TAI).  The inverse is:

$$
t_\text{TAI} = \text{BD} \times 86400 + 946{,}727{,}967.816
$$

Converting TAI→UTC subtracts the cumulative leap-second offset (currently **37
seconds** since 2017-01-01).

### Key properties

| Property | Value |
|---|---|
| Epoch | J2000.0 = 2000-01-01T12:00:00.000 TAI |
| Unit | 1 BD = 86 400 SI seconds (one solar day) |
| Substrate | TAI (no leap-second discontinuities) |
| Format | Decimal, e.g. `9628.501783` |
| BD 0.0 | 2000-01-01T11:58:55.816 UTC |
| BD 9628 | ≈ 2026-05-12 (this fork was written) |

Because BD is TAI-based, it never jumps or repeats at a leap second.
A difference of `1.0` is always exactly 86 400 seconds.

### Reference implementation

The canonical Rust implementation is in [`brightdate-rust/`](brightdate-rust/).
The C implementation used by this fork lives in
[`find/brightdate.c`](find/brightdate.c) and
[`find/brightdate.h`](find/brightdate.h).

---

## Tools

| Binary | Replaces | Description |
|---|---|---|
| `bfind` | `find` | File search with BrightDate time predicates and `%W` format |
| `blocate` | `locate` | Database-backed file search; prints DB mtime as BD |
| `bupdatedb` | `updatedb` | Database builder; uses `bfind` internally |
| `bxargs` | `xargs` | Argument list builder (renamed; no BD-specific changes) |

---

## Building

### Prerequisites

- C compiler (GCC or Clang)
- GNU Autotools: `autoconf`, `automake ≥ 1.16`, `m4`

### Steps

```sh
git clone https://github.com/Digital-Defiance/bright-findutils.git
cd bright-findutils
autoreconf -fi          # regenerate Makefile.in from Makefile.am
./configure --prefix=/usr/local
make
sudo make install
```

To build only `bfind` without installing:

```sh
autoreconf -fi
./configure
make -C gl              # gnulib support library (generates headers)
make -C lib             # findutils support library
make -C find            # produces find/bfind
```

---

## BrightDate in bfind

### -after and -before

Two new primary predicates accept a **BrightDate scalar** (or any datetime
string accepted by `--newermt`):

```sh
# Files modified after BD 9628.0 (≈ 2026-05-12 00:00 UTC)
bfind /path -after 9628.0

# Files modified before BD 9000.0 (≈ 2024-08-17)
bfind /path -before 9000.0

# ISO 8601 strings still work too
bfind /path -after "2026-01-01"
```

`-after` is equivalent to `-newer` but accepts BD scalars directly.
`-before` is the logical inverse — it matches files whose mtime is strictly
less than the reference time.

### %W printf format

`-printf` gains a new **`%W`** format letter, analogous to `%T` (mtime as
strftime) but outputting BrightDate decimals:

| Specifier | Timestamp | Example output |
|---|---|---|
| `%Wt` | modification time | `9628.501783612` |
| `%Wa` | access time | `9628.501780234` |
| `%Wc` | status-change (inode) time | `9628.498579048` |
| `%WB` | birth (creation) time | `9628.480661705` |

```sh
# Print BD mtime of every .c file
bfind . -name '*.c' -printf '%Wt %p\n'

# Print all four timestamps
bfind . -maxdepth 1 -printf 't=%Wt a=%Wa c=%Wc B=%WB  %f\n'
```

The output precision is 9 decimal places (≈ 86.4 µs resolution).

### -daystart

`-daystart` is redefined for BrightDate: it sets the reference time to
`floor(BD_now)` — the start of the current BD day — rather than the local
calendar midnight.  Because BD days are exactly 86 400 seconds, this is
unambiguous even during a leap second.

```sh
# Files modified in the current BD day
bfind . -daystart -mtime 0
```

### Time predicates accepting BD scalars

The `-newerXt` family (`-newerat`, `-newerct`, `-newermt`, `-newerBt`) will
try to parse its argument as a BrightDate scalar first, then fall back to the
`parse_datetime` string parser.

```sh
# -newermt with a BD scalar
bfind . -newermt 9628.0 -printf '%Wt %f\n'
```

---

## BrightDate in blocate / bupdatedb

`blocate --statistics` now displays the database modification time as a
BrightDate value instead of a localtime string:

```
Database was last modified at BD 9628.501783612
```

`bupdatedb` is otherwise identical to `updatedb`, but calls `bfind`
internally to build the file database.

---

## BrightDate API (C)

Declared in [`find/brightdate.h`](find/brightdate.h), implemented in
[`find/brightdate.c`](find/brightdate.c).

```c
/* Convert a BrightDate scalar to a struct timespec (UTC wall clock). */
struct timespec bd_to_timespec (double bd);

/* Convert a struct timespec to a BrightDate scalar. */
double timespec_to_bd (struct timespec ts);

/* Return the current time as a BrightDate scalar. */
double brightdate_now (void);

/* Return true iff str looks like a BD literal (digits + optional dot). */
bool is_brightdate_literal (const char *str);

/* Parse a BD literal into *result.  Returns false if not a valid literal. */
bool parse_brightdate_literal (const char *str, double *result);
```

### Leap-second table

`brightdate.c` embeds a static leap-second table (28 entries, current through
2028-06-28).  The table is sourced from IERS Bulletin C / IANA
`leap-seconds.list`.  Update the table when new leap seconds are announced.

---

## Tests

BrightDate-specific tests live in [`tests/find/`](tests/find/):

| Test | What it covers |
|---|---|
| `brightdate_printf.sh` | `%Wt`, `%Wa`, `%Wc`, `%WB` format specifiers |
| `brightdate_after_before.sh` | `-after` / `-before` BD scalar predicates |
| `brightdate_daystart.sh` | `-daystart` BD floor behaviour |

Run all tests with:

```sh
make check
```

Run a single test:

```sh
make check TESTS=tests/find/brightdate_printf.sh
```

---

## Original findutils README

This package contains the GNU find, xargs, and locate programs.  find
and xargs comply with POSIX 1003.2.  They also support a large number
of additional options, some borrowed from Unix and some unique to GNU.

See [NEWS](NEWS) for a list of major changes in the current release.

See [COPYING](COPYING) for copying conditions.

See [INSTALL](INSTALL) for compilation and installation instructions.

The latest full release of upstream findutils is available at
<http://ftp.gnu.org/gnu/findutils>.

Bug reports and patches for the upstream project:
<https://savannah.gnu.org/bugs/?group=findutils>

Bug reports for this fork:
<https://github.com/Digital-Defiance/bright-findutils/issues>
