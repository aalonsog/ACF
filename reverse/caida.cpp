#include <cassert>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <unordered_set>

#include "caida-util.hpp"
#include "four-one.hpp"
#include "hash-table.hpp"

using namespace std;

int main(int argc, char** argv) {
  assert(2 == argc);
  size_t b = 1024;

  for (int i = 0; i < 1; ++i) {
    FILE* file = fopen(argv[1], "r");
    MS64 ms64[4] = {MS64::FromDevice(), MS64::FromDevice(), MS64::FromDevice(),
                    MS64::FromDevice()};

    auto id = [](uint64_t x) { return x; };
    FourOne<Line, 14, 1, decltype(id)> acf14(id, b, ms64, MS128::FromDevice());
    FourOne<Line, 16, 1, decltype(id)> acf16(id, b, ms64, MS128::FromDevice());
    LineHasher lh = LineHasher::FromDevice();

    while (not feof(file)) {
      Line l{file};
      auto h = lh(l);
#define INSERT(ACF)                                    \
  {                                                    \
    const auto found = ACF.AdaptiveFind(l, h);         \
    if (found != decltype(ACF)::TruePositive) {        \
      ACF.Insert(l, h);                                \
      if (ACF.Count() >= ACF.Capacity() * 0.95) break; \
    }                                                  \
  }
      INSERT(acf14);
      INSERT(acf16);
    }
    rewind(file);
    size_t lines_seen = 0;
    HashSet<Line, LineHasher> exact(1024, lh);
    //unordered_set<Line, LineHasher> exact2(1024, lh);
    while (not feof(file)) {
      Line l{file};
      const auto h = lh(l);
      acf14.AdaptiveFind(l, h);
      acf16.AdaptiveFind(l, h);
      ++lines_seen;
      // exact.Insert(l);
      if (0 == i) exact.insert(l);
      if ((lines_seen & (lines_seen - 1)) == 0) {
        // cout << b * (1 << f) * log(1.0 / sqrt(1.0 - 2.0 * acf.AdaptedCount() /
        // acf.Count()))
        //      << endl;
      }
    }
    cout << acf14.AdaptedCount() << ' ';
    cout << acf16.AdaptedCount() << ' ';
    // cout << exact.Size();
    if (0 == i) cout << exact.size() << ' ' << lines_seen;
    cout << endl;
  }
}
