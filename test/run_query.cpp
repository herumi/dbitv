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
#include <cstdarg>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <algorithm>
#include <vector>

#include "SuccinctBitVector.hpp"
#include "rank.hpp"
#include "xbyak/xbyak_util.h"
#include "util.hpp"

namespace {


const double BIT_DENSITY = 0.50;
const size_t NTRIALS = 11;

void show_result(std::vector<double>& tv, int count, const char *msg)
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

int main(int argc, char **argv)
{
  int nloop = 10000000;
  int bsz = 1000000 & ~255; // restriction of rank.hpp

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

  bsz &= ~255; // restriction of rank.hpp

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
  std::vector<uint32_t> rkwk(nloop), stwk(nloop);

  for (int i = 0; i < nloop; i++) {
    rkwk[i] = r.get() % bsz;
  }

  for (int i = 0; i < nloop; i++) {
    stwk[i] = r.get() % nbits;
  }

  /* Start benchmarking rank & select */
  {
    std::vector<double> rtv;
    std::vector<double> stv;

    /* A benchmark for rank */
    uint32_t chk = 0;
    for (size_t i = 0; i < NTRIALS; i++) {
      Xbyak::util::Clock clk;
      clk.begin();

      for (int j = 0; j < nloop; j++) {
        chk += bv.getRank().rank1(rkwk[j] + 1);
      }
      clk.end();
      rtv.push_back((double)clk.getClock());
    }

    show_result(rtv, nloop, "--rank");
    printf("   chk=%u\n", chk);

#if 1
    chk = 0;
    rtv.clear();
    for (size_t i = 0; i < NTRIALS; i++) {
      Xbyak::util::Clock clk;
      clk.begin();

      for (int j = 0; j < nloop; j++) {
        chk += my_sbv.rank1(rkwk[j]);
      }

      clk.end();
      rtv.push_back((double)clk.getClock());
    }
    show_result(rtv, nloop, "--my_rank");
    printf("my chk=%u\n", chk);

    chk = 0;
    for (size_t i = 0; i < NTRIALS; i++) {
      Xbyak::util::Clock clk;
      clk.begin();

      for (int j = 0; j < nloop; j++) {
        chk += bv.rank(rkwk[j], 1);
      }
      clk.end();
      rtv.push_back((double)clk.getClock());
    }

    show_result(rtv, nloop, "--rank");
    printf("   chk=%u\n", chk);
#endif

  }
}
