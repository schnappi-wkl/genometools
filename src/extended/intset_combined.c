/*
  Copyright (c) 2014 Dirk Willrodt <willrodt@zbh.uni-hamburg.de>
  Copyright (c) 2014 Center for Bioinformatics, University of Hamburg

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

#include <stdio.h>

#include "core/assert_api.h"
#include "core/error_api.h"
#include "extended/intset.h"
#include "extended/intset_16.h"
#include "extended/intset_32.h"
#include "extended/intset_8.h"
#include "extended/xansi_io.h"

GtIntset *gt_intset_best_new(GtUword maxelement, GtUword num_of_elems)
{
  size_t s8, s16, s32;
  gt_assert(GT_BITS_FOR_TYPE(GtUword) > GT_BITS_FOR_TYPE(uint8_t));
  s8 = gt_intset_8_size(maxelement, num_of_elems);
  s16 = GT_BITS_FOR_TYPE(GtUword) > GT_BITS_FOR_TYPE(uint16_t) ?
    gt_intset_16_size(maxelement, num_of_elems) :
    s8;
  s32 = GT_BITS_FOR_TYPE(GtUword) > GT_BITS_FOR_TYPE(uint32_t) ?
    gt_intset_32_size(maxelement, num_of_elems) :
    s8;
  if (s8 <= s16) {
    if (s8 <= s32)
      return gt_intset_8_new(maxelement, num_of_elems);
  }
  else {
    if (s16 <= s32)
      return gt_intset_16_new(maxelement, num_of_elems);
  }
  return gt_intset_32_new(maxelement, num_of_elems);
}

static GtUword gt_intset_read_type_rewind(FILE *fp)
{
  fpos_t pos;
  GtUword type;
  if (fgetpos(fp, &pos) != 0) {
    perror("fgetpos failed");
    exit(EXIT_FAILURE);
  }
  gt_xansi_io_xfread_one(type, fp);
  if (fsetpos(fp, &pos) != 0) {
    perror("fsetpos failed");
    exit(EXIT_FAILURE);
  }
  return type;
}

GtIntset *gt_intset_new_from_file(FILE *fp, GtError *err)
{
  GtUword type;
  GtIntset *intset = NULL;
  type = gt_intset_read_type_rewind(fp);
  if (gt_intset_8_file_is_type(type))
    intset = gt_intset_8_new_from_file(fp, err);
  else {
    if (gt_intset_16_file_is_type(type))
      intset = gt_intset_16_new_from_file(fp, err);
    else {
      if (gt_intset_32_file_is_type(type))
        intset = gt_intset_32_new_from_file(fp, err);
      else
        gt_error_set(err, "could not identify intset type from file");
    }
  }
  return intset;
}

GtIntset *gt_intset_io(GtIntset *intset, FILE *fp, GtError *err,
                       GtXansiIOFunc io_func)
{
  GtIntsetIOFunc intset_io;
  GtUword type;
  if (intset == NULL) {
    type = gt_intset_read_type_rewind(fp);
    if (gt_intset_8_file_is_type(type))
      intset_io = gt_intset_8_io;
    else {
      if (gt_intset_16_file_is_type(type))
        intset_io = gt_intset_16_io;
      else {
        if (gt_intset_32_file_is_type(type))
          intset_io = gt_intset_32_io;
        else {
          gt_error_set(err, "could not identify intset type from file");
          return NULL;
        }
      }
    }
  }
  else {
    gt_assert(intset->c_class != NULL);
    gt_assert(intset->c_class->io_func != NULL);
    intset_io = intset->c_class->io_func;
  }
  return intset_io(intset, fp, err, io_func);
}

int gt_intset_unit_test(GtError *err)
{
  int had_err = 0;
  gt_error_check(err);
  if (GT_BITS_FOR_TYPE(GtUword) > GT_BITS_FOR_TYPE(uint8_t))
    had_err = gt_intset_8_unit_test(err);
  if (!had_err && GT_BITS_FOR_TYPE(GtUword) > GT_BITS_FOR_TYPE(uint16_t))
    had_err = gt_intset_16_unit_test(err);
  if (!had_err && GT_BITS_FOR_TYPE(GtUword) > GT_BITS_FOR_TYPE(uint32_t))
    had_err = gt_intset_32_unit_test(err);
  return had_err;
}
