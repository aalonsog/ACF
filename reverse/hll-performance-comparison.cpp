#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "caida-util.hpp"
#include "four-one-separate.hpp"
#include "hash-table.hpp"
// #include "local-hll.hpp"
#include "hll.hpp"

using namespace std;

int main(int argc, char ** argv) {
  assert(2 == argc);
  size_t b = 1 << 16;

  FILE* file = fopen(argv[1], "r");


  vector<Line> trace;

  while (not feof(file)) {
    Line l{file};
    trace.push_back(l);
  }
  printf("Trace ready; benchmarking beginning\n");

  
  MS64 ms64[4] = {MS64::FromDevice(), MS64::FromDevice(), MS64::FromDevice(),
                  MS64::FromDevice()};
  for (int i = 0; i < 10; ++i) {
    LineHasher lh = LineHasher::FromDevice();
    FourOneACF<Line, 8, 1, LineHasher> acf15(lh, b, ms64, MS128::FromDevice());
    //HLL<5> hll(1 << 10);
    datasketches::hll_sketch hll{10, datasketches::target_hll_type::HLL_6};

    auto LineMatch = [](const Line& l) {return l.ip_from_[0] >> 5 == 0;};
    chrono::steady_clock clock;
    auto before = clock.now();
    size_t count = 0;
    size_t j = 0;
    for (; j < trace.size(); ++j) {
      auto h = lh(trace[j]);
      const auto found = acf15.AdaptiveFind(trace[j], h);
      if (found != decltype(acf15)::TruePositive && LineMatch(trace[j])) {
        try {
          acf15.Insert(trace[j], h);
          ++count;
        } catch (...) {
          cout << "full\n";
          break;
        }
      }
      if (j % (trace.size() / 60) == 0) {
        auto C_occupancy = acf15.EstimateNegativesCardinality();
        fprintf(stdout, "acf cardinality estimate: %f, with RSE: %f\n", C_occupancy.first,
                acf15.RSE(C_occupancy));
      }
    }
    auto after = clock.now();
    printf("seconds: %f\n", (after - before).count() / 1000.0 / 1000.0 / 1000.0);
    printf("num inserted into ACF: %zd, num looked up: %zd", count, j);
    fflush(stdout);

    HashSet<Line, LineHasher> hashset(16, lh);
    for (size_t k = 0; k < j; ++k) {
      hashset.insert(trace[k]);
    }
    cout << ", num distinct: " << hashset.size() << endl;

    acf15 = decltype(acf15)(lh, b, ms64, MS128::FromDevice());
    before = clock.now();
    count = 0;
    j = 0;
    for (; j < trace.size(); ++j) {
      auto h = lh(trace[j]);
      const auto found = acf15.AdaptiveFind(trace[j], h);
      if (found != decltype(acf15)::TruePositive && LineMatch(trace[j])) {
        try {
          acf15.Insert(trace[j], h);
          ++count;
        } catch (...) {
          cout << "full\n";
          break;
        }
      }
      if (found != decltype(acf15)::TruePositive) {
        hll.update(h);
      }
      if (j % (trace.size()/60) == 0) {
        fprintf(stdout, "acf cardinality estimate: %f, HLL cardinality estimate: %f, HLL RSE: %f\n",
                acf15.EstimateNegativesCardinality().first, hll.get_estimate(), 1.04 / (1 << 5));
      }
    }
    after = clock.now();
    printf("seconds: %f\n", (after - before).count() / 1000.0 / 1000.0 / 1000.0);
    printf("num inserted into ACF: %zd, num looked up: %zd\n", count, j);
  }
}
