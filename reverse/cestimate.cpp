#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>

#include "four-one.hpp"

using namespace std;

int main() {
  size_t C = 256ul << 10;
  size_t N = 25 * C;
  size_t b = 4096;

  random_device d;
  auto Rand = [&d]() { return (static_cast<uint64_t>(d()) << 32) | d(); };
  unordered_map<uint64_t, bool> keys;
  for (size_t i = 0; i < C; ++i) {
    keys[Rand()] = false;
  }
  auto Id = [](uint64_t x) { return x; };
  MS64 hash_makers[4] = {MS64::FromDevice(), MS64::FromDevice(), MS64::FromDevice(),
                         MS64::FromDevice()};
  FourOne<uint64_t, 15, 1, decltype(Id)> filter(Id, b, hash_makers,
                                                MS128::FromDevice());
  for (size_t i = 0; i < b * 4 * 0.95; ++i) {
    auto key = Rand();
    keys[key] = true;
    filter.Insert(key, key);
  }
  vector<uint64_t> false_positives;
  for (const auto& x : keys) {
    if (not x.second) false_positives.push_back(x.first);
  }
  auto MagicMod = [](uint64_t x, uint64_t y) {
    return (static_cast<unsigned __int128>(x) * static_cast<unsigned __int128>(y)) >> 64;
  };
  for (size_t i = 0; i < N; ++i) {
    auto index = MagicMod(Rand(), false_positives.size());
    filter.AdaptiveFind(index, index);
  }
  cout << b * (1 << 15) * log(1.0 / (1.0 - 1.0 * filter.AdaptedCount() / filter.Count()));
  cout << "\t" << false_positives.size() << endl;
}
