/*
** autogenerated content - DO NOT EDIT
*/
/*
** Copyright (C) 2007 Thomas Jahns <Thomas.Jahns@gmx.net>
**
** See LICENSE file or http://genometools.org/license.html for license details.
**
*/
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <time.h>
#include <sys/time.h>

#include "libgtcore/bitpackstring.h"
#include "libgtcore/env.h"
#include "libgtcore/ensure.h"

enum {
/*   MAX_RND_NUMS = 10, */
  MAX_RND_NUMS = 100000,
};

static inline int
icmp(unsigned a, unsigned b)
{
  if (a > b)
    return 1;
  else if (a < b)
    return -1;
  else /* if (a == b) */
    return 0;
}

#define freeResourcesAndReturn(retval) \
  do {                                 \
    env_ma_free(randSrc, env);         \
    env_ma_free(randCmp, env);         \
    env_ma_free(bitStore, env);        \
    env_ma_free(bitStoreCopy, env);    \
    return retval;                     \
  } while (0)

int
bitPackStringInt_unit_test(Env *env)
{
  BitString bitStore = NULL;
  BitString bitStoreCopy = NULL;
  unsigned *randSrc = NULL; /*< create random ints here for input as bit
                                *  store */
  unsigned *randCmp = NULL; /*< used for random ints read back */
  size_t i, numRnd;
  BitOffset offsetStart, offset;
  unsigned long seedval;
  int had_err = 0;
  {
    struct timeval seed;
    gettimeofday(&seed, NULL);
    seedval = seed.tv_sec + seed.tv_usec;
    srandom(seedval);
  }
  offset = offsetStart = random()%(sizeof (unsigned) * CHAR_BIT);
  numRnd = random() % (MAX_RND_NUMS + 1);
  env_log_log(env, "seedval = %lu, offset=%lu, numRnd=%lu\n", seedval,
              (long unsigned)offsetStart, (long unsigned)numRnd);
  {
    BitOffset numBits = sizeof (unsigned) * CHAR_BIT * numRnd + offsetStart;
    randSrc = env_ma_malloc(env, sizeof (unsigned)*numRnd);
    bitStore = env_ma_malloc(env, bitElemsAllocSize(numBits)
                             * sizeof (BitElem));
    bitStoreCopy = env_ma_calloc(env, bitElemsAllocSize(numBits),
                                 sizeof (BitElem));
    randCmp = env_ma_malloc(env, sizeof (unsigned)*numRnd);
  }
  /* first test unsigned types */
  env_log_log(env, "bsStoreUInt/bsGetUInt: ");
  for (i = 0; i < numRnd; ++i)
  {
#if 32 > 32 && LONG_BIT < 32
    unsigned v = randSrc[i] = (unsigned)random() << 32 | random();
#else /* 32 > 32 && LONG_BIT < 32 */
    unsigned v = randSrc[i] = random();
#endif /* 32 > 32 && LONG_BIT < 32 */
    int bits = requiredUIntBits(v);
    bsStoreUInt(bitStore, offset, bits, v);
    offset += bits;
  }
  offset = offsetStart;
  for (i = 0; i < numRnd; ++i)
  {
    unsigned v = randSrc[i];
    int bits = requiredUIntBits(v);
    unsigned r = bsGetUInt(bitStore, offset, bits);
    ensure(had_err, r == v);
    if (had_err)
    {
      env_log_log(env, "Expected %""u"", got %""u"
                  ", seed = %lu, i = %lu\n",
                  v, r, seedval, (unsigned long)i);
      freeResourcesAndReturn(had_err);
    }
    offset += bits;
  }
  env_log_log(env, "passed\n");
  if (numRnd > 0)
  {
    unsigned v = randSrc[0], r = 0;
    unsigned numBits = requiredUIntBits(v);
    BitOffset i = offsetStart + numBits;
    unsigned mask = ~(unsigned)0;
    if (numBits < 32)
      mask = ~(mask << numBits);
    env_log_log(env, "bsSetBit, bsClearBit, bsToggleBit, bsGetBit: ");
    while (v)
    {
      int lowBit = v & 1;
      v >>= 1;
      ensure(had_err, lowBit == (r = bsGetBit(bitStore, --i)));
      if (had_err)
      {
        env_log_log(env, "Expected %d, got %d, seed = %lu, i = %llu\n",
                    lowBit, (int)r, seedval, (unsigned long long)i);
        freeResourcesAndReturn(had_err);
      }
    }
    i = offsetStart + numBits;
    bsClear(bitStoreCopy, offsetStart, numBits, random()&1);
    v = randSrc[0];
    while (v)
    {
      int lowBit = v & 1;
      v >>= 1;
      if (lowBit)
        bsSetBit(bitStoreCopy, --i);
      else
        bsClearBit(bitStoreCopy, --i);
    }
    v = randSrc[0];
    r = bsGetUInt(bitStoreCopy, offsetStart, numBits);
    ensure(had_err, r == v);
    if (had_err)
    {
      env_log_log(env, "Expected %""u"", got %""u"
                  ", seed = %lu\n", v, r, seedval);
      freeResourcesAndReturn(had_err);
    }
    for (i = 0; i < numBits; ++i)
      bsToggleBit(bitStoreCopy, offsetStart + i);
    r = bsGetUInt(bitStoreCopy, offsetStart, numBits);
    ensure(had_err, r == (v = (~v & mask)));
    if (had_err)
    {
      env_log_log(env, "Expected %""u"", got %""u"
                  ", seed = %lu\n", v, r, seedval);
      freeResourcesAndReturn(had_err);
    }
    env_log_log(env, "passed\n");
  }
  if (numRnd > 1)
  {
    env_log_log(env, "bsCompare: ");
    {
      unsigned v0 = randSrc[0];
      int bits0 = requiredUIntBits(v0);
      unsigned r0;
      offset = offsetStart;
      r0 = bsGetUInt(bitStore, offset, bits0);
      for (i = 1; i < numRnd; ++i)
      {
        unsigned v1 = randSrc[i];
        int bits1 = requiredUIntBits(v1);
        unsigned r1 = bsGetUInt(bitStore, offset + bits0, bits1);
        int result = -2;   /*< -2 is not a return value of bsCompare, thus
                            *   if it is displayed, there was an earlier
                            *   error. */
        ensure(had_err, r0 == v0 && r1 == v1);
        ensure(had_err, icmp(v0, v1) ==
               (result = bsCompare(bitStore, offset, bits0,
                                   bitStore, offset + bits0, bits1)));
        if (had_err)
        {
          env_log_log(env, "Expected v0 %s v1, got v0 %s v1,\n for v0=%"
                      "u"" and v1=%""u"",\n"
                      "seed = %lu, i = %lu, bits0=%u, bits1=%u\n",
                      (v0 > v1?">":(v0 < v1?"<":"==")),
                      (result > 0?">":(result < 0?"<":"==")), v0, v1,
                      seedval, (unsigned long)i, bits0, bits1);
          freeResourcesAndReturn(had_err);
        }
        offset += bits0;
        bits0 = bits1;
        v0 = v1;
        r0 = r1;
      }
    }
    env_log_log(env, "passed\n");
  }
  env_log_log(env, "bsStoreUniformUIntArray/bsGetUInt: ");
  {
    unsigned numBits = random()%32 + 1;
    unsigned mask = ~(unsigned)0;
    if (numBits < 32)
      mask = ~(mask << numBits);
    offset = offsetStart;
    bsStoreUniformUIntArray(bitStore, offset, numBits, numRnd, randSrc);
    for (i = 0; i < numRnd; ++i)
    {
      unsigned v = randSrc[i] & mask;
      unsigned r = bsGetUInt(bitStore, offset, numBits);
      ensure(had_err, r == v);
      if (had_err)
      {
        env_log_log(env, "Expected %""u"", got %""u"",\n"
                    "seed = %lu, i = %lu, bits=%u\n",
                    v, r, seedval, (unsigned long)i, numBits);
        freeResourcesAndReturn(had_err);
      }
      offset += numBits;
    }
    env_log_log(env, "passed\n");
    env_log_log(env,
                "bsStoreUniformUIntArray/bsGetUniformUIntArray: ");
    bsGetUniformUIntArray(bitStore, offset = offsetStart,
                               numBits, numRnd, randCmp);
    for (i = 0; i < numRnd; ++i)
    {
      unsigned v = randSrc[i] & mask;
      unsigned r = randCmp[i];
      ensure(had_err, r == v);
      if (had_err)
      {
        env_log_log(env,
                    "Expected %""u"", got %""u"",\n seed = %lu,"
                    " i = %lu, bits=%u\n",
                    v, r, seedval, (unsigned long)i, numBits);
        freeResourcesAndReturn(had_err);
      }
    }
    if (numRnd > 1)
    {
      unsigned v = randSrc[0] & mask;
      unsigned r;
      bsGetUniformUIntArray(bitStore, offsetStart,
                                 numBits, 1, &r);
      if (r != v)
      {
        env_log_log(env,
                    "Expected %""u"", got %""u"", seed = %lu,"
                    " one value extraction\n",
                    v, r, seedval);
        freeResourcesAndReturn(had_err);
      }
    }
    env_log_log(env, " passed\n");
  }
  /* int types */
  env_log_log(env, "bsStoreInt/bsGetInt: ");
  for (i = 0; i < numRnd; ++i)
  {
    int v = (int)randSrc[i];
    unsigned bits = requiredIntBits(v);
    bsStoreInt(bitStore, offset, bits, v);
    offset += bits;
  }
  offset = offsetStart;
  for (i = 0; i < numRnd; ++i)
  {
    int v = randSrc[i];
    unsigned bits = requiredIntBits(v);
    int r = bsGetInt(bitStore, offset, bits);
    ensure(had_err, r == v);
    if (had_err)
    {
      env_log_log(env, "Expected %""d"", got %""d"",\n"
                  "seed = %lu, i = %lu, bits=%u\n",
                  v, r, seedval, (unsigned long)i, bits);
      freeResourcesAndReturn(had_err);
    }
    offset += bits;
  }
  env_log_log(env, "passed\n");
  env_log_log(env, "bsStoreUniformIntArray/bsGetInt: ");
  {
    unsigned numBits = random()%32 + 1;
    int mask = ~(int)0;
    if (numBits < 32)
      mask = ~(mask << numBits);
    offset = offsetStart;
    bsStoreUniformIntArray(bitStore, offset, numBits, numRnd,
                                (int *)randSrc);
    for (i = 0; i < numRnd; ++i)
    {
      int m = (int)1 << (numBits - 1);
      int v = (int)((randSrc[i] & mask) ^ m) - m;
      int r = bsGetInt(bitStore, offset, numBits);
      ensure(had_err, r == v);
      if (had_err)
      {
        env_log_log(env, "Expected %""d"", got %""d"",\n"
                    "seed = %lu, i = %lu, numBits=%u\n",
                    v, r, seedval, (unsigned long)i, numBits);
        freeResourcesAndReturn(had_err);
      }
      offset += numBits;
    }
    env_log_log(env, "passed\n");
    env_log_log(env, "bsStoreUniformIntArray/bsGetUniformIntArray: ");
    bsGetUniformIntArray(bitStore, offset = offsetStart,
                              numBits, numRnd, (int *)randCmp);
    for (i = 0; i < numRnd; ++i)
    {
      int m = (int)1 << (numBits - 1);
      int v = (int)((randSrc[i] & mask) ^ m) - m;
      int r = randCmp[i];
      ensure(had_err, r == v);
      if (had_err)
      {
        env_log_log(env, "Expected %""d"", got %""d"
                    ", seed = %lu, i = %lu\n",
                    v, r, seedval, (unsigned long)i);
        freeResourcesAndReturn(had_err);
      }
    }
    if (numRnd > 0)
    {
      int m = (int)1 << (numBits - 1);
      int v = (int)((randSrc[0] & mask) ^ m) - m;
      int r;
      bsGetUniformIntArray(bitStore, offsetStart,
                                numBits, 1, &r);
      ensure(had_err, r == v);
      if (had_err)
      {
        env_log_log(env, "Expected %""d"", got %""d"
                    ", seed = %lu, one value extraction\n",
                    v, r, seedval);
        freeResourcesAndReturn(had_err);
      }
    }
    env_log_log(env, "passed\n");
  }
  if (numRnd > 0)
  {
    env_log_log(env, "bsCopy: ");
    {
      /* first decide how many of the values to use and at which to start */
      size_t numValueCopies, copyStart;
      BitOffset numCopyBits = 0, destOffset;
      unsigned numBits = random()%32 + 1;
      unsigned mask = ~(unsigned)0;
      if (numBits < 32)
        mask = ~(mask << numBits);
      if (random()&1)
      {
        numValueCopies = random()%(numRnd + 1);
        copyStart = random()%(numRnd - numValueCopies + 1);
      }
      else
      {
        copyStart = random() % numRnd;
        numValueCopies = random()%(numRnd - copyStart) + 1;
      }
      assert(copyStart + numValueCopies <= numRnd);
      offset = offsetStart + (BitOffset)copyStart * numBits;
      bsStoreUniformUIntArray(bitStore, offset, numBits, numValueCopies,
                                    randSrc);
      destOffset = random()%(offsetStart + 32
                             * (BitOffset)(numRnd - numValueCopies) + 1);
      numCopyBits = (BitOffset)numBits * numValueCopies;
      /* the following bsCopy should be equivalent to:
       * bsStoreUniformUIntArray(bitStoreCopy, destOffset,
       *                              numBits, numValueCopies, randSrc); */
      bsCopy(bitStore, offset, bitStoreCopy, destOffset, numCopyBits);
      ensure(had_err,
             bsCompare(bitStore, offset, numCopyBits,
                       bitStoreCopy, destOffset, numCopyBits) == 0);
      if (had_err)
      {
        env_log_log(env, "Expected equality on bitstrings\n"
                    "seed = %lu, offset = %llu, destOffset = %llu,"
                    " numCopyBits=%llu\n",
                    seedval, (unsigned long long)offset,
                    (unsigned long long)destOffset,
                    (unsigned long long)numCopyBits);
        /* FIXME: implement bitstring output function */
        freeResourcesAndReturn(had_err);
      }
      env_log_log(env, "passed\n");
    }
  }
  if (numRnd > 0)
  {
    env_log_log(env, "bsClear: ");
    {
      /* first decide how many of the values to use and at which to start */
      size_t numResetValues, resetStart;
      BitOffset numResetBits = 0;
      unsigned numBits = random()%32 + 1;
      int bitVal = random()&1;
      int cmpVal = bitVal?-1:0;
      unsigned mask = ~(unsigned)0;
      if (numBits < 32)
        mask = ~(mask << numBits);
      if (random()&1)
      {
        numResetValues = random()%(numRnd + 1);
        resetStart = random()%(numRnd - numResetValues + 1);
      }
      else
      {
        resetStart = random() % numRnd;
        numResetValues = random()%(numRnd - resetStart) + 1;
      }
      assert(resetStart + numResetValues <= numRnd);
      offset = offsetStart;
      bsStoreUniformIntArray(bitStore, offset, numBits, numRnd,
                                    (int *)randSrc);
      numResetBits = (BitOffset)numBits * numResetValues;
      /* the following bsCopy should be equivalent to:
       * bsStoreUniformUIntArray(bitStoreCopy, destOffset,
       *                              numBits, numResetValues, randSrc); */
      bsClear(bitStore, offset + (BitOffset)resetStart * numBits,
              numResetBits, bitVal);
      {
        int m = (int)1 << (numBits - 1);
        for (i = 0; i < resetStart; ++i)
        {
          int v = (int)((randSrc[i] & mask) ^ m) - m;
          int r = bsGetInt(bitStore, offset, numBits);
          ensure(had_err, r == v);
          if (had_err)
          {
            env_log_log(env, "Expected %""d"", got %""d"",\n"
                        "seed = %lu, i = %lu, numBits=%u\n",
                        v, r, seedval, (unsigned long)i, numBits);
            freeResourcesAndReturn(had_err);
          }
          offset += numBits;
        }
        for (; i < resetStart + numResetValues; ++i)
        {
          int r = bsGetInt(bitStore, offset, numBits);
          ensure(had_err, r == cmpVal);
          if (had_err)
          {
            env_log_log(env, "Expected %""d"", got %""d"",\n"
                        "seed = %lu, i = %lu, numBits=%u\n",
                        cmpVal, r, seedval, (unsigned long)i, numBits);
            freeResourcesAndReturn(had_err);
          }
          offset += numBits;
        }
        for (; i < numRnd; ++i)
        {
          int v = (int)((randSrc[i] & mask) ^ m) - m;
          int r = bsGetInt(bitStore, offset, numBits);
          ensure(had_err, r == v);
          if (had_err)
          {
            env_log_log(env, "Expected %""d"", got %""d"",\n"
                        "seed = %lu, i = %lu, numBits=%u\n",
                        v, r, seedval, (unsigned long)i, numBits);
            freeResourcesAndReturn(had_err);
          }
          offset += numBits;
        }
      }
      env_log_log(env, "passed\n");
    }
  }
  freeResourcesAndReturn(had_err);
}
