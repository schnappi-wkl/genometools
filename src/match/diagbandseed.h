/*
  Copyright (c) 2015-2016 Joerg Winkler <joerg.winkler@studium.uni-hamburg.de>
  Copyright (c) 2015-2016 Center for Bioinformatics, University of Hamburg

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

#ifndef DIAGBANDSEED_H
#define DIAGBANDSEED_H
#include <stdbool.h>
#include "core/encseq_api.h"
#include "core/error_api.h"
#include "core/range_api.h"
#include "core/types_api.h"
#include "match/seed-extend.h"

typedef struct GtDiagbandseedInfo GtDiagbandseedInfo;
typedef struct GtDiagbandseedExtendParams GtDiagbandseedExtendParams;

/* Run the whole algorithm. */
int gt_diagbandseed_run(const GtDiagbandseedInfo *arg,
                        const GtRange *aseqranges,
                        const GtRange *bseqranges,
                        GtUword anumseqranges,
                        GtUword bnumseqranges,
                        GtError *err);

/* The constructor */
GtDiagbandseedInfo *gt_diagbandseed_info_new(GtEncseq *aencseq,
                                             GtEncseq *bencseq,
                                             GtUword maxfreq,
                                             GtUword memlimit,
                                             unsigned int seedlength,
                                             bool norev,
                                             bool nofwd,
                                             bool overlappingseeds,
                                             bool verify,
                                             bool verbose,
                                             bool debug_kmer,
                                             bool debug_seedpair,
                                             bool extend_last,
                                             GtDiagbandseedExtendParams *extp);

/* The destructor */
void gt_diagbandseed_info_delete(GtDiagbandseedInfo *info);

GtDiagbandseedExtendParams *gt_diagbandseed_extend_params_new(
                              GtUword errorpercentage,
                              GtUword userdefinedleastlength,
                              GtUword logdiagbandwidth,
                              GtUword mincoverage,
                              bool seed_display,
                              bool use_apos,
                              GtGreedyextendmatchinfo *extendgreedyinfo,
                              GtXdropmatchinfo *extendxdropinfo,
                              GtQuerymatchoutoptions *querymatchoutopt);

void gt_diagbandseed_extend_params_delete(GtDiagbandseedExtendParams *extp);
#endif