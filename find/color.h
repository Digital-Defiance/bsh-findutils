#ifndef FIND_COLOR_H
#define FIND_COLOR_H

#include <stdbool.h>
#include <stdio.h>

typedef enum {
  FU_COLOR_AUTO,
  FU_COLOR_ALWAYS,
  FU_COLOR_NEVER,
  FU_COLOR_ANSI,
  FU_COLOR_TRUECOLOR,
} fu_color_when_t;

typedef enum {
  FU_SCHEME_DEFAULT,
  FU_SCHEME_BRIGHT,
} fu_color_scheme_t;

typedef struct {
  int enabled;
  const char *reset;
  const char *bd_mtime;
  const char *bd_atime;
  const char *bd_ctime;
  const char *bd_birth;
  const char *bd_mtime_frac;
  const char *bd_atime_frac;
  const char *bd_ctime_frac;
  const char *bd_birth_frac;
  const char *path;
  const char *path_dim;
  const char *symlink;
} fu_colors_t;

extern fu_colors_t fu_colors;
extern fu_color_when_t fu_color_when;

#define FU(c, field) ((c)->enabled ? (c)->field : "")

void fu_color_init (int *argc, char **argv);
int fu_color_parse_when (const char *value, fu_color_when_t *out);
int fu_color_parse_scheme (const char *value, fu_color_scheme_t *out);
bool fu_color_stream_active (bool dest_is_tty);
const char *fu_color_bd_style (char subchar);
const char *fu_color_bd_frac_style (char subchar);

#endif /* FIND_COLOR_H */
