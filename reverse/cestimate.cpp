#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>

#include "four-one.hpp"

using namespace std;

int main() {
  // variables from the paper:
  // the number of distinct false positives
  size_t C = 256ul << 10;
  // the number of queries of these false positives
  size_t N = 25 * C;
  // one quarter the size of the filter
  size_t b = 4096;
  // the number of fingerprint bits
  constexpr int f = 15;
  // the percent occupancy of the filter, expressed as a number between 0 and 1
  double o = 0.95;
  // the number of runs
  int r = 100;

  random_device d;
  auto Rand = [&d]() { return (static_cast<uint64_t>(d()) << 32) | d(); };
    // Don't need a hash function when the keys are fully random!
  auto Id = [](uint64_t x) { return x; };
  MS64 hash_makers[4] = {MS64::FromDevice(), MS64::FromDevice(), MS64::FromDevice(),
                         MS64::FromDevice()};
  MS128 fingerprint_maker = MS128::FromDevice();
  auto MagicMod = [](uint64_t x, uint64_t y) {
    return (static_cast<unsigned __int128>(x) * static_cast<unsigned __int128>(y)) >> 64;
  };

  for (int i = 0; i < r; ++i) {
    // the keys that we use. keys[k] = false if a key is not in the map, true otherwise
    unordered_map<uint64_t, bool> keys;
    for (size_t i = 0; i < C; ++i) {
      keys[Rand()] = false;
    }

    FourOne<uint64_t, f, 1 /* adaptivity bits */, decltype(Id)> filter(Id, b, hash_makers,
                                                                       fingerprint_maker);
    for (size_t i = 0; i < b * 4 * o; ++i) {
      auto key = Rand();
      keys[key] = true;
      // fill the filter
      filter.Insert(key, key);
    }

    vector<uint64_t> false_positives;
    for (const auto& x : keys) {
      if (not x.second) false_positives.push_back(x.first);
    }

    for (size_t i = 0; i < N; ++i) {
      auto index = MagicMod(Rand(), false_positives.size());
      filter.AdaptiveFind(index, index);
    }

    // print the estimate
    cout << b * (1 << 15) *
                log(1.0 / (1.0 - 1.0 * filter.AdaptedCount() / filter.Count()));
    cout << "\t" << false_positives.size() << endl;
  }
}
