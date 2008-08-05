/*
  Copyright (c) 2007 Sascha Steinbiss <ssteinbiss@stud.zbh.uni-hamburg.de>,
  Copyright (c) 2007 Malte Mader <mmader@stud.zbh.uni-hamburg.de>,
  Copyright (c) 2007 Christin Schaerfer <cschaerfer@stud.zbh.uni-hamburg.de>
  Copyright (c) 2007 Center for Bioinformatics, University of Hamburg

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

#include <string.h>
#include "libgtcore/ensure.h"
#include "libgtcore/hashtable.h"
#include "libgtcore/interval_tree.h"
#include "libgtcore/ma.h"
#include "libgtcore/minmax.h"
#include "libgtcore/range.h"
#include "libgtcore/undef.h"
#include "libgtcore/unused.h"
#include "libgtext/feature_type_factory_builtin.h"
#include "libgtext/genome_node.h"
#include "libgtview/feature_index.h"

struct FeatureIndex {
  Hashtable *regions;
  char *firstseqid;
  unsigned int nof_sequence_regions,
               reference_count;
};

typedef struct {
  IntervalTree *features;
  SequenceRegion *region;
  Range dyn_range;
} RegionInfo;

static void region_info_delete(RegionInfo *info)
{
  interval_tree_delete(info->features);
  if (info->region)
    genome_node_delete((GenomeNode*)info->region);
  ma_free(info);
}

FeatureIndex* feature_index_new(void)
{
  FeatureIndex *fi;
  fi = ma_calloc(1, sizeof (FeatureIndex));
  fi->regions = hashtable_new(HASH_STRING, NULL, (FreeFunc) region_info_delete);
  return fi;
}

FeatureIndex* feature_index_ref(FeatureIndex *fi)
{
  assert(fi);
  fi->reference_count++;
  return fi;
}

void feature_index_add_sequence_region(FeatureIndex *fi, SequenceRegion *sr)
{
  char *seqid;
  RegionInfo *info;
  assert(fi && sr);
  seqid = str_get(genome_node_get_seqid((GenomeNode*) sr));
  if (!hashtable_get(fi->regions, seqid)) {
    info = ma_malloc(sizeof (RegionInfo));
    info->region = (SequenceRegion*) genome_node_ref((GenomeNode*) sr);
    info->features = interval_tree_new();
    info->dyn_range.start = ~0UL;
    info->dyn_range.end   = 0;
    hashtable_add(fi->regions, seqid, info);
    if (fi->nof_sequence_regions++ == 0)
      fi->firstseqid = seqid;
  }
}

void feature_index_add_genome_feature(FeatureIndex *fi, GenomeFeature *gf)
{
  GenomeNode *gn;
  GenomeFeature *gf_new;
  char* seqid;
  Range node_range;
  RegionInfo *info;

  assert(fi && gf);

  gn = genome_node_rec_ref((GenomeNode*) gf);
  gf_new = (GenomeFeature*) gn;
  /* get information about seqid and range */
  node_range = genome_node_get_range(gn);
  seqid = str_get(genome_node_get_seqid(gn));
  /* entry for the seqid must already exist */
  assert(feature_index_has_seqid(fi, seqid));
  info = (RegionInfo*) hashtable_get(fi->regions, seqid);
  /* add node to the appropriate array in the hashtable */
  IntervalTreeNode *new_node = interval_tree_node_new(gn, node_range.start,
                                                      node_range.end,
                                                      (FreeFunc) genome_node_rec_delete);
  interval_tree_insert(info->features, new_node);
  /* update dynamic range */
  info->dyn_range.start = MIN(info->dyn_range.start, node_range.start);
  info->dyn_range.end = MAX(info->dyn_range.end, node_range.end);
}

static int collect_features_from_itree(IntervalTreeNode *node, void *data)
{
  Array *a = (Array*) data;
  GenomeNode *gn = (GenomeNode*) interval_tree_node_get_data(node);
  array_add(a, gn);
  return 0;
}

Array* feature_index_get_features_for_seqid(FeatureIndex *fi, const char *seqid)
{
  RegionInfo *ri;
  int had_err = 0;
  Array *a;
  assert(fi && seqid);
  a = array_new(sizeof (GenomeFeature*));
  ri = (RegionInfo*) hashtable_get(fi->regions, seqid);
  had_err = interval_tree_traverse(ri->features,
                                   collect_features_from_itree,
                                   a);
  assert(!had_err);   /* collect_features_from_itree() is sane */
  return a;
}

int feature_index_get_features_for_range(FeatureIndex *fi, Array *results,
                                         const char *seqid, Range qry_range,
                                         Error *err)
{
  RegionInfo *ri;
  error_check(err);
  assert(fi && results);

  ri = (RegionInfo*) hashtable_get(fi->regions, seqid);
  if (!ri) {
    error_set(err, "feature index does not contain the given sequence id");
    return -1;
  }
  interval_tree_find_all_overlapping(ri->features, qry_range.start,
                                     qry_range.end, results);
  return 0;
}

const char* feature_index_get_first_seqid(const FeatureIndex *fi)
{
  assert(fi);
  return fi->firstseqid;
}

int store_seqid(void *key, UNUSED void *value, void *data, UNUSED Error *err)
{
  StrArray *seqids = (StrArray*) data;
  const char *seqid = (const char*) key;
  assert(seqids && seqid);
  strarray_add_cstr(seqids, seqid);
  return 0;
}

StrArray* feature_index_get_seqids(const FeatureIndex *fi)
{
  StrArray* seqids;
  int rval;
  assert(fi);
  seqids = strarray_new();
  rval = hashtable_foreach_ao(fi->regions, store_seqid, seqids, NULL);
  assert(!rval); /* store_seqid() is sane */
  return seqids;
}

Range feature_index_get_range_for_seqid(FeatureIndex *fi, const char *seqid)
{
  Range ret;
  RegionInfo *info;
  assert(fi);
  info = (RegionInfo*) hashtable_get(fi->regions, seqid);
  assert(info);
  if (info && (info->dyn_range.start != ~0UL && info->dyn_range.end != 0)) {
    ret.start = info->dyn_range.start;
    ret.end = info->dyn_range.end;
  }
  else
    return genome_node_get_range((GenomeNode*) info->region);
  return ret;
}

void feature_index_get_rangeptr_for_seqid(FeatureIndex *fi, Range *range,
                                          const char *seqid)
{
  assert(fi && range);
  *range = feature_index_get_range_for_seqid(fi, seqid);
}

bool feature_index_has_seqid(const FeatureIndex *fi, const char *seqid)
{
  assert(fi);
  return (hashtable_get(fi->regions, seqid));
}

int feature_index_unit_test(Error *err)
{
  FeatureTypeFactory *feature_type_factory;
  GenomeFeatureType *type;
  /* first we have to create some objects that we can use for testing */
  GenomeNode *gn1, *gn2, *ex1, *ex2, *ex3, *cds1;
  FeatureIndex *fi;
  Range r1, r2, r3, r4, r5, check_range, rs;
  Str *seqid1, *seqid2;
  StrArray *seqids = NULL;
  SequenceRegion *sr1, *sr2;
  Array *features = NULL;
  int had_err = 0;
  error_check(err);

  feature_type_factory = feature_type_factory_builtin_new();

  /* generating some ranges */
  r1.start=100UL; r1.end=1000UL;
  r2.start=100UL; r2.end=300UL;
  r3.start=500UL; r3.end=1000UL;
  r4.start=600UL; r4.end=1200UL;
  r5.start=600UL; r5.end=1000UL;
  rs.start=100UL; rs.end=1200UL;

  /* generating sequnce ids as C-strings */
  seqid1 = str_new_cstr("test1");
  seqid2 = str_new_cstr("test2");

  sr1 = (SequenceRegion*) sequence_region_new(seqid1, rs, NULL, 0);
  sr2 = (SequenceRegion*) sequence_region_new(seqid2, rs, NULL, 0);

  /* generate a new genome_feature with the property gft_gene and the range r1
     ... */
  type = feature_type_factory_create_gft(feature_type_factory, gft_gene);
  gn1 = genome_feature_new(type, r1, STRAND_UNKNOWN, NULL, 0);
  /* ... and assign a sequence id to the new genome_feature-object. */
  genome_node_set_seqid(gn1, seqid1);

  gn2 = genome_feature_new(type, r4, STRAND_UNKNOWN, NULL, 0);
  genome_node_set_seqid(gn2, seqid2);

  type = feature_type_factory_create_gft(feature_type_factory, gft_exon);
  ex1 = genome_feature_new(type, r2, STRAND_UNKNOWN, NULL, 0);
  genome_node_set_seqid(ex1, seqid1);

  ex2 = genome_feature_new(type, r3, STRAND_UNKNOWN, NULL, 0);
  genome_node_set_seqid(ex2, seqid1);

  ex3 = genome_feature_new(type, r4, STRAND_UNKNOWN, NULL, 0);
  genome_node_set_seqid(ex3, seqid2);

  type = feature_type_factory_create_gft(feature_type_factory, gft_CDS);
  cds1 = genome_feature_new(type, r5, STRAND_UNKNOWN, NULL, 0);
  genome_node_set_seqid(cds1, seqid2);

  /* Determine the structure of our feature tree */
  genome_node_is_part_of_genome_node(gn1, ex1);
  genome_node_is_part_of_genome_node(gn1, ex2);
  genome_node_is_part_of_genome_node(gn2, ex3);
  genome_node_is_part_of_genome_node(gn2, cds1);

  /* create a new feature index on which we can perfom some tests */
  fi = feature_index_new();

  ensure(had_err, fi);
  ensure(had_err, !feature_index_has_seqid(fi, "test1"));
  ensure(had_err, !feature_index_has_seqid(fi, "test2"));

  /* add a sequence region to the feature index and test if it has really been
     added */
  feature_index_add_sequence_region(fi, sr1);
  ensure(had_err, feature_index_has_seqid(fi, "test1"));

  feature_index_add_sequence_region(fi, sr2);
  ensure(had_err, feature_index_has_seqid(fi, "test2"));

  /* tests if we get a empty data structure for every added sequence region*/
  if (!had_err)
    features = feature_index_get_features_for_seqid(fi, "test1");
  ensure(had_err, features);
  ensure(had_err, array_size(features) == 0);
  array_delete(features);
  features = NULL;

  if (!had_err)
    features = feature_index_get_features_for_seqid(fi, "test2");
  ensure(had_err, features);
  ensure(had_err, array_size(features) == 0);
  array_delete(features);
  features = NULL;

  /* add features to every sequence region and test if the according
     datastructures are not empty anymore. As we have added one genome_feature
     to every sequence region the size has to be one. */
  if (!had_err) {
    feature_index_add_genome_feature(fi, (GenomeFeature*) gn1);
    features = feature_index_get_features_for_seqid(fi, "test1");
  }
  ensure(had_err, array_size(features) == 1UL);
  array_delete(features);
  features = NULL;

  if (!had_err) {
    feature_index_add_genome_feature(fi, (GenomeFeature*) gn2);
    features = feature_index_get_features_for_seqid(fi, "test2");
  }
  ensure(had_err, array_size(features) == 1UL);
  array_delete(features);
  features = NULL;

  /* test feature_index_get_first_seqid() */
  ensure(had_err, feature_index_get_first_seqid(fi));
  ensure(had_err, strcmp("test1", feature_index_get_first_seqid(fi)) == 0);

  if (!had_err) {
    seqids = feature_index_get_seqids(fi);
    ensure(had_err, strarray_size(seqids) == 2);
    ensure(had_err, !strcmp(strarray_get(seqids, 0), "test1"));
    ensure(had_err, !strcmp(strarray_get(seqids, 1), "test2"));
  }

  check_range = feature_index_get_range_for_seqid(fi, "test1");
  ensure(had_err, check_range.start == 100UL && check_range.end == 1000UL);

  if (!had_err)
    features = feature_index_get_features_for_seqid(fi, "test1");
  ensure(had_err, features);
  array_delete(features);

  /* delete all generated objects */
  strarray_delete(seqids);
  feature_index_delete(fi);
  genome_node_rec_delete(gn1);
  genome_node_rec_delete(gn2);
  genome_node_rec_delete((GenomeNode*) sr1);
  genome_node_rec_delete((GenomeNode*) sr2);
  str_delete(seqid1);
  str_delete(seqid2);
  feature_type_factory_delete(feature_type_factory);
  return had_err;
}

void feature_index_delete(FeatureIndex *fi)
{
  if (!fi) return;
  if (fi->reference_count) {
    fi->reference_count--;
    return;
  }
  hashtable_delete(fi->regions);
  ma_free(fi);
}
