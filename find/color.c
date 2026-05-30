/* color.c -- terminal color support for bfind output.
   Copyright (C) 2024-2026 Free Software Foundation, Inc.

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
*/

#include <config.h>

#include "color.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

fu_colors_t fu_colors;
fu_color_when_t fu_color_when = FU_COLOR_AUTO;

static const fu_colors_t fu_colors_off = {
  .enabled = 0,
  .reset = "",
  .bd_mtime = "",
  .bd_atime = "",
  .bd_ctime = "",
  .bd_birth = "",
  .bd_mtime_frac = "",
  .bd_atime_frac = "",
  .bd_ctime_frac = "",
  .bd_birth_frac = "",
  .path = "",
  .path_dim = "",
  .symlink = "",
};

static const fu_colors_t fu_colors_ansi_default = {
  .enabled = 1,
  .reset = "\x1b[0m",
  .bd_mtime = "\x1b[1;36m",
  .bd_atime = "\x1b[1;32m",
  .bd_ctime = "\x1b[1;33m",
  .bd_birth = "\x1b[1;35m",
  .bd_mtime_frac = "\x1b[37m",
  .bd_atime_frac = "\x1b[37m",
  .bd_ctime_frac = "\x1b[37m",
  .bd_birth_frac = "\x1b[37m",
  .path = "\x1b[1m",
  .path_dim = "\x1b[2m",
  .symlink = "\x1b[33m",
};

static const fu_colors_t fu_colors_ansi_bright = {
  .enabled = 1,
  .reset = "\x1b[0m",
  .bd_mtime = "\x1b[1;96m",
  .bd_atime = "\x1b[1;92m",
  .bd_ctime = "\x1b[1;93m",
  .bd_birth = "\x1b[1;95m",
  .bd_mtime_frac = "\x1b[97m",
  .bd_atime_frac = "\x1b[97m",
  .bd_ctime_frac = "\x1b[97m",
  .bd_birth_frac = "\x1b[97m",
  .path = "\x1b[1;97m",
  .path_dim = "\x1b[2m",
  .symlink = "\x1b[93m",
};

static const fu_colors_t fu_colors_truecolor_default = {
  .enabled = 1,
  .reset = "\x1b[0m",
  .bd_mtime = "\x1b[1;38;2;80;200;220m",
  .bd_atime = "\x1b[1;38;2;120;220;140m",
  .bd_ctime = "\x1b[1;38;2;240;200;80m",
  .bd_birth = "\x1b[1;38;2;200;120;220m",
  .bd_mtime_frac = "\x1b[38;2;200;204;214m",
  .bd_atime_frac = "\x1b[38;2;200;204;214m",
  .bd_ctime_frac = "\x1b[38;2;200;204;214m",
  .bd_birth_frac = "\x1b[38;2;200;204;214m",
  .path = "\x1b[1;38;2;240;240;240m",
  .path_dim = "\x1b[2;38;2;128;128;128m",
  .symlink = "\x1b[38;2;240;200;80m",
};

static const fu_colors_t fu_colors_truecolor_bright = {
  .enabled = 1,
  .reset = "\x1b[0m",
  .bd_mtime = "\x1b[1;38;2;0;220;255m",
  .bd_atime = "\x1b[1;38;2;80;255;120m",
  .bd_ctime = "\x1b[1;38;2;255;220;80m",
  .bd_birth = "\x1b[1;38;2;255;120;255m",
  .bd_mtime_frac = "\x1b[38;2;210;214;224m",
  .bd_atime_frac = "\x1b[38;2;210;214;224m",
  .bd_ctime_frac = "\x1b[38;2;210;214;224m",
  .bd_birth_frac = "\x1b[38;2;210;214;224m",
  .path = "\x1b[1;38;2;255;255;255m",
  .path_dim = "\x1b[2;38;2;160;160;160m",
  .symlink = "\x1b[38;2;255;220;80m",
};

static int
strcaseeq (const char *a, const char *b)
{
  for (; *a && *b; a++, b++)
    {
      if (tolower ((unsigned char) *a) != tolower ((unsigned char) *b))
        return 0;
    }
  return *a == *b;
}

static int
env_truthy (const char *name)
{
  const char *v = getenv (name);

  if (v == NULL || v[0] == '\0')
    return 0;
  return strcmp (v, "0") != 0 && !strcaseeq (v, "false");
}

static int
no_color_requested (void)
{
  const char *clicolor = getenv ("CLICOLOR");

  if (getenv ("NO_COLOR") != NULL)
    return 1;
  if (clicolor != NULL
      && (strcmp (clicolor, "0") == 0 || strcaseeq (clicolor, "false")))
    return 1;
  return 0;
}

static int
term_supports_truecolor (void)
{
  const char *colorterm = getenv ("COLORTERM");
  const char *term = getenv ("TERM");

  if (colorterm != NULL)
    {
      if (strcaseeq (colorterm, "truecolor") || strcaseeq (colorterm, "24bit"))
        return 1;
    }
  if (term != NULL && strstr (term, "truecolor") != NULL)
    return 1;
  return 0;
}

static const fu_colors_t *
palette_ansi (fu_color_scheme_t scheme)
{
  return scheme == FU_SCHEME_BRIGHT ?
    &fu_colors_ansi_bright : &fu_colors_ansi_default;
}

static const fu_colors_t *
palette_truecolor (fu_color_scheme_t scheme)
{
  return scheme == FU_SCHEME_BRIGHT ?
    &fu_colors_truecolor_bright : &fu_colors_truecolor_default;
}

static void
fu_colors_resolve (fu_color_when_t when, fu_color_scheme_t scheme)
{
  fu_color_when = when;

  if (when == FU_COLOR_NEVER)
    {
      fu_colors = fu_colors_off;
      return;
    }

  if (when == FU_COLOR_AUTO && no_color_requested ())
    {
      fu_colors = fu_colors_off;
      return;
    }

  if (when == FU_COLOR_ANSI)
    {
      fu_colors = *palette_ansi (scheme);
      return;
    }

  if (when == FU_COLOR_TRUECOLOR)
    {
      fu_colors = *palette_truecolor (scheme);
      return;
    }

  if (when == FU_COLOR_ALWAYS || when == FU_COLOR_AUTO)
    {
      if (when == FU_COLOR_AUTO
          && !isatty (fileno (stdout)) && !env_truthy ("CLICOLOR_FORCE"))
        {
          fu_colors = fu_colors_off;
          return;
        }
      if (term_supports_truecolor ())
        fu_colors = *palette_truecolor (scheme);
      else
        fu_colors = *palette_ansi (scheme);
      return;
    }

  fu_colors = fu_colors_off;
}

int
fu_color_parse_when (const char *value, fu_color_when_t *out)
{
  if (value == NULL || value[0] == '\0')
    return -1;

  if (strcaseeq (value, "auto"))
    *out = FU_COLOR_AUTO;
  else if (strcaseeq (value, "always") || strcaseeq (value, "on")
           || strcaseeq (value, "yes"))
    *out = FU_COLOR_ALWAYS;
  else if (strcaseeq (value, "never") || strcaseeq (value, "off")
           || strcaseeq (value, "no"))
    *out = FU_COLOR_NEVER;
  else if (strcaseeq (value, "ansi") || strcaseeq (value, "16"))
    *out = FU_COLOR_ANSI;
  else if (strcaseeq (value, "truecolor") || strcaseeq (value, "24bit")
           || strcaseeq (value, "rgb"))
    *out = FU_COLOR_TRUECOLOR;
  else
    return -1;

  return 0;
}

int
fu_color_parse_scheme (const char *value, fu_color_scheme_t *out)
{
  if (value == NULL || value[0] == '\0')
    return -1;

  if (strcaseeq (value, "default"))
    *out = FU_SCHEME_DEFAULT;
  else if (strcaseeq (value, "bright"))
    *out = FU_SCHEME_BRIGHT;
  else
    return -1;

  return 0;
}

bool
fu_color_stream_active (bool dest_is_tty)
{
  if (!fu_colors.enabled)
    return false;
  if (dest_is_tty)
    return true;
  if (fu_color_when == FU_COLOR_ALWAYS
      || fu_color_when == FU_COLOR_ANSI
      || fu_color_when == FU_COLOR_TRUECOLOR)
    return true;
  if (env_truthy ("CLICOLOR_FORCE"))
    return true;
  return false;
}

const char *
fu_color_bd_style (char subchar)
{
  switch (subchar)
    {
    case 'a':
      return FU (&fu_colors, bd_atime);
    case 'c':
      return FU (&fu_colors, bd_ctime);
    case 'B':
      return FU (&fu_colors, bd_birth);
    case 't':
    default:
      return FU (&fu_colors, bd_mtime);
    }
}

const char *
fu_color_bd_frac_style (char subchar)
{
  switch (subchar)
    {
    case 'a':
      return FU (&fu_colors, bd_atime_frac);
    case 'c':
      return FU (&fu_colors, bd_ctime_frac);
    case 'B':
      return FU (&fu_colors, bd_birth_frac);
    case 't':
    default:
      return FU (&fu_colors, bd_mtime_frac);
    }
}

static void
color_usage (const char *prog)
{
  fprintf (stderr,
           "%s: invalid color setting (expected auto, always, never, ansi, or truecolor)\n",
           prog);
}

static void
scheme_usage (const char *prog)
{
  fprintf (stderr,
           "%s: invalid color scheme (expected default or bright)\n",
           prog);
}

static int
parse_color_arg (const char *prog, const char *arg, fu_color_when_t *when)
{
  const char *value = arg;

  if (strncmp (arg, "--color=", 8) == 0)
    value = arg + 8;
  else if (strcmp (arg, "--color") == 0)
    value = "always";

  if (fu_color_parse_when (value, when) != 0)
    {
      color_usage (prog);
      exit (2);
    }
  return 1;
}

static int
parse_scheme_arg (const char *prog, const char *arg, fu_color_scheme_t *scheme)
{
  const char *value = arg;

  if (strncmp (arg, "--color-scheme=", 15) == 0)
    value = arg + 15;

  if (fu_color_parse_scheme (value, scheme) != 0)
    {
      scheme_usage (prog);
      exit (2);
    }
  return 1;
}

static void
compact_argv (int *argc, char **argv, int remove_at)
{
  int i;

  for (i = remove_at; i + 1 < *argc; i++)
    argv[i] = argv[i + 1];
  (*argc)--;
  argv[*argc] = NULL;
}

void
fu_color_init (int *argc, char **argv)
{
  fu_color_when_t when = FU_COLOR_AUTO;
  fu_color_scheme_t scheme = FU_SCHEME_DEFAULT;
  const char *prog = (argv && argv[0]) ? argv[0] : "bfind";
  const char *env_color = getenv ("BFIND_COLOR");
  const char *env_scheme = getenv ("BFIND_COLOR_SCHEME");
  int i = 1;

  if (env_color != NULL && env_color[0] != '\0')
    {
      if (fu_color_parse_when (env_color, &when) != 0)
        {
          color_usage (prog);
          exit (2);
        }
    }
  if (env_scheme != NULL && env_scheme[0] != '\0')
    {
      if (fu_color_parse_scheme (env_scheme, &scheme) != 0)
        {
          scheme_usage (prog);
          exit (2);
        }
    }

  while (i < *argc)
    {
      if (strcmp (argv[i], "--no-color") == 0)
        {
          when = FU_COLOR_NEVER;
          compact_argv (argc, argv, i);
          continue;
        }
      if (strncmp (argv[i], "--color=", 8) == 0
          || strcmp (argv[i], "--color") == 0)
        {
          parse_color_arg (prog, argv[i], &when);
          compact_argv (argc, argv, i);
          continue;
        }
      if (strncmp (argv[i], "--color-scheme=", 15) == 0)
        {
          parse_scheme_arg (prog, argv[i], &scheme);
          compact_argv (argc, argv, i);
          continue;
        }
      i++;
    }

  fu_colors_resolve (when, scheme);
}
