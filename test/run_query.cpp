/*
  modified by herumi
  require https://github.com/herumi/opti, https://github.com/herumi/xbyak

  extra memory[bit]  rate
  maropu  192/256      6
  tbl512  128/512      2
  tbl1Ki  128/1024     1

  benchmark
  dbitv% foreach i (1000000 5000000 10000000 50000000 100000000 500000000 1000000000)


        clk @Core i7-2600(L3 8MiB) 3.4GHz + gcc 4.6.3
            1e6   5e6   1e7   5e7   1e8    5e8    1e9
   maropu 15.78 18.84 22.85 70.11 83.82 101.82 110.48
   tbl512 13.20 16.30 17.00 47.59 66.62  84.01  96.09
   tbl1Ki 20.39 24.12 25.11 53.38 89.02 113.30 125.10

        clk @Xeon X5650(L3 12MiB) + gcc 4.6.3
            1e6   5e6   1e7   5e7   1e8    5e8    1e9
   maropu 16.34 19.99 21.70 39.53 57.83  72.94  73.37
   tbl512 13.39 19.04 20.61 26.58 47.76  68.21  72.69
   tbl1Ki 20.49 25.86 27.03 32.38 56.52  88.40  92.96
*/
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

