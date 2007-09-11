/*
  Copyright (c) 2006-2007 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2006-2007 Center for Bioinformatics, University of Hamburg

  Permission to use, copy, modify, and distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgtcore/cstr.h>
#include <libgtcore/fileutils.h>
#include <libgtcore/splitter.h>
#include <libgtcore/xansi.h>
#include <libgtcore/xposix.h>

bool file_exists(const char *path)
{
  FILE *file;
  if ((file = fopen(path, "r")) == NULL)
    return false;
  xfclose(file);
  return true;
}

bool file_is_newer(const char *a, const char *b)
{
  struct stat stat_a, stat_b;
  assert(a && b);
  xstat(a, &stat_a);
  xstat(b, &stat_b);
  if (stat_a.st_mtime > stat_b.st_mtime ||
      (stat_a.st_mtime == stat_b.st_mtime &&
       stat_a.st_mtime > stat_b.st_mtime)) {
    return true;
  }
  return false;
}

unsigned long file_number_of_lines(FILE *fp)
{
  unsigned long number_of_lines = 0;
  fpos_t current_pos;
  int cc;
  assert(fp);
  xfgetpos(fp, &current_pos);
  xfseek(fp, SEEK_SET, 0);
  while ((cc = xfgetc(fp)) != EOF)
    if (cc == '\n') number_of_lines++;
  xfsetpos(fp, &current_pos);
  return number_of_lines;
}

void file_dirname(Str *path, const char *file, Env *env)
{
  long i;
  str_reset(path);
  for (i = strlen(file) - 1; i >= 0; i--) {
    if (file[i] == '/')
      break;
  }
  if (i > 0)
    str_append_cstr_nt(path, file, i, env);
}

int file_find_in_path(Str *path, const char *file, Env *env)
{
  char *pathvariable, *pathcomponent = NULL;
  Splitter *splitter = NULL;
  unsigned long i;
  int had_err = 0;

  env_error_check(env);
  assert(file);

  /* check if 'file' has dirname */
  file_dirname(path, file, env);
  if (str_length(path))
    return had_err;
  /* 'file' has no dirname -> scan $PATH */
  pathvariable = getenv("PATH");
  if (pathvariable)
    pathvariable = cstr_dup(pathvariable, env); /* make writeable copy */
  else {
    env_error_set(env, "environment variable $PATH is not defined");
    had_err = -1;
  }

  if (!had_err) {
    splitter = splitter_new(env);
    splitter_split(splitter, pathvariable, strlen(pathvariable), ':', env);
    for (i = 0; i < splitter_size(splitter); i++) {
      pathcomponent = splitter_get_token(splitter, i);
      str_set_length(path, 0);
      str_append_cstr(path, pathcomponent, env);
      str_append_char(path, '/', env);
      str_append_cstr(path, file, env);
      if (file_exists(str_get(path)))
        break;
    }
    if (i < splitter_size(splitter)) {
      /* file found in path */
      str_set_length(path, 0);
      str_append_cstr(path, pathcomponent, env);
    }
    else {
      /* file not found in path  */
      str_reset(path);
    }
  }

  /* free */
  env_ma_free(pathvariable, env);
  splitter_delete(splitter, env);

  return had_err;
}
