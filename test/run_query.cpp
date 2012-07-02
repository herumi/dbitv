/*-----------------------------------------------------------------------------
 *  run_benchmark.cpp - A benchmark for SuccinctBitVector.hpp
 *
 *  Coding-Style: google-styleguide
 *      https://code.google.com/p/google-styleguide/
 *
 *  Copyright 2012 Takeshi Yamamuro <linguin.m.s_at_gmail.com>
 *-----------------------------------------------------------------------------
 */

#include <cstdio>
#include <memory>
#include <algorithm>
#include <vector>

#include "SuccinctBitVector.hpp"
#include "rank.hpp"
#include "xbyak/xbyak_util.h"
#include "util.hpp"

namespace {

typedef std::vector<double> DoubleVec;
typedef std::vector<uint32_t> Uint32Vec;

const double BIT_DENSITY = 0.50;
const size_t NTRIALS = 11;

void show_result(DoubleVec& tv, int count, const char *msg)
{
  /* Get the value of median */
  std::sort(tv.begin(), tv.end());
  double t = tv[tv.size() / 2];

  printf("%s\n", msg);
  printf(" Total Time(count:%d): %lf\n", count, t);
  printf(" Throughputs(query/sec): %lf\n", count / t);
  printf(" Unit Speed(sec/query): %lfclk\n", t / count);
}

} /* namespace */

size_t getRank(const succinct::dense::SuccinctRank& r, size_t pos)
{
  return r.rank1(pos + 1);
}

size_t getRank(const mie::SuccinctBitVector& sb, size_t pos)
{
  return sb.rank1(pos);
}

template<class T>
void bench(const T& t, const Uint32Vec& rkwk, int nloop, const char *msg)
{
  uint32_t chk = 0;
  DoubleVec rtv;

  for (size_t i = 0; i < NTRIALS; i++) {
    Xbyak::util::Clock clk;
    clk.begin();

    for (int j = 0; j < nloop; j++) {
      chk += getRank(t, rkwk[j]);
    }

    clk.end();
    rtv.push_back((double)clk.getClock());
  }
  show_result(rtv, nloop, msg);
  printf("chk=%u\n", chk);
}

int main(int argc, char **argv)
{
  int nloop = 10000000;
  int bsz = 1000000;

  argc--, argv++;
  while (argc > 0) {
    if (argc > 1 && strcmp(*argv, "-l") == 0) {
      argc--, argv++;
      nloop = atoi(*argv);
    } else
    if (argc > 1 && strcmp(*argv, "-b") == 0) {
      argc--, argv++;
      bsz = atoi(*argv);
    } else
    {
      fprintf(stderr, "run_query [-l <loop num>] [-b <bit size>]\n");
      return 1;
    }
    argc--, argv++;
  }

//  bsz &= ~255; // restriction of rank.hpp
  printf("loop %d, bit size %d\n", nloop, bsz);

  /* Generate a sequence of bits */
  succinct::dense::SuccinctBitVector bv;
  bv.init(bsz);
  mie::BitVector my_bv;
  my_bv.resize(bsz);
  XorShift128 r;

  uint32_t nbits = 0;
  uint64_t thres = (1ULL << 32) * BIT_DENSITY;
  for (int i = 0; i < bsz; i++) {
    if (r.get() < thres) {
      nbits++;
      bv.set_bit(1, i);
      my_bv.set(i, true);
    }
  }

  bv.build();
  mie::SuccinctBitVector my_sbv;
  my_sbv.init(my_bv.getBlock(), my_bv.getBlockSize());

  /* Generate test data-set */
  Uint32Vec rkwk(nloop);

  for (int i = 0; i < nloop; i++) {
    rkwk[i] = r.get() % bsz;
  }

  bench(bv.getRank(), rkwk, nloop, "org rank");
  bench(my_sbv, rkwk, nloop, "mie rank");
  bench(bv.getRank(), rkwk, nloop, "org rank");
  bench(my_sbv, rkwk, nloop, "mie rank");
}

