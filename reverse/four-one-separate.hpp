#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
#include <cstdio>

using namespace std;

#include "debug-table.hpp"

// 4-1 adaptive cuckoo filters with separately stored backing data to reduce cache pollution
template <typename Z, int W, int S, typename H>
struct FourOneACF {
  static_assert(S == 1);
  struct Slot {
    uint16_t fingerprint : W; // never zero and full
    unsigned short alpha: S;
  };

  vector<Z> back_[4];
  vector<Slot> data_[4];
  MS64 hash_makers_[4];
  MS128 print_maker_;
  H h_;

  size_t Capacity() const { return data_[0].size() * 4; }

  FourOneACF(H h, size_t bucket_count, const MS64 hash_makers[4], MS128 print_maker)
      : back_{vector<Z>{bucket_count}, vector<Z>{bucket_count},
              vector<Z>{bucket_count}, vector<Z>{bucket_count}},
        data_{vector<Slot>{bucket_count}, vector<Slot>{bucket_count},
              vector<Slot>{bucket_count}, vector<Slot>{bucket_count}},
        hash_makers_{hash_makers[0], hash_makers[1], hash_makers[2], hash_makers[3]},
        print_maker_(print_maker),
        h_(h) {
    if (0 != (bucket_count & (bucket_count - 1))) throw bucket_count;
  }

  void Insert(Z key, uint64_t code /* h_(key) */) {
    int ttl = 500;
    for (int j = 0; true; j = ((j + 1) % 4)) {
      auto hash = hash_makers_[j](code) & (data_[j].size() - 1);
      if (data_[j][hash].fingerprint == 0) {
        data_[j][hash].fingerprint = print_maker_(code);
        if (data_[j][hash].fingerprint == 0) data_[j][hash].fingerprint = 1;
        back_[j][hash] = key;
        // cout << "ttl: " << 500 - ttl << endl;
        return;
      }
      using std::swap;
      swap(key, back_[j][hash]);

      //data_[j][hash].alpha += 1;
      //data_[j][hash].alpha = data_[j][hash].alpha % ((1 << S) - 1);
      data_[j][hash].fingerprint = print_maker_(code) >> (data_[j][hash].alpha * W);
      code = h_(key);
      if (data_[j][hash].fingerprint == 0) data_[j][hash].fingerprint = 1;
      --ttl;
      if (ttl < 0) throw key;
    }
  }

  enum Found { TrueNegative = 0, TruePositive = 1, FalsePositive = 2 };
  Found AdaptiveFind(Z key, uint64_t code /* h_(key) */) {
    auto result = TrueNegative;
    for (int i = 0; i < 4; ++i) {
      auto hash = hash_makers_[i](code) & (data_[i].size() - 1);
      if (data_[i][hash].fingerprint == 0) continue;
      // cout << code << " " << data_[i][hash].fingerprint << " "
      //      << static_cast<uint64_t>(print_maker_(code) >> (data_[i][hash].alpha * W)) << endl;
      auto fp = (print_maker_(code) >> (data_[i][hash].alpha * W)) & ((1ull << W) - 1);
      fp = fp ? fp : 1;
      if (key == back_[i][hash]) return TruePositive;
      if (fp == data_[i][hash].fingerprint) {
        result = FalsePositive;
        data_[i][hash].alpha += 1;
        // data_[i][hash].alpha = data_[i][hash].alpha & ((1 << S) - 1);
        data_[i][hash].fingerprint =
            print_maker_(h_(back_[i][hash])) >> (data_[i][hash].alpha * W);
        if (data_[i][hash].fingerprint == 0) data_[i][hash].fingerprint = 1;
      }
    }
    return result;
  }

  pair<double, uint64_t> EstimateNegativesCardinality() const {
    uint64_t count = 0, occupancy = 0;
    for (int i = 0; i < 4; ++i) {
      for (size_t j = 0; j < data_[i].size(); ++j) {
        count += data_[i][j].alpha;
        occupancy += (data_[i][j].fingerprint != 0);
      }
    }
    double p1hat = count / 1.0 / occupancy;
    printf("p1hat: %f\n", p1hat);
    return {(data_[0].size() << (W - 1)) * -log(1.0 - 2 * p1hat), occupancy};
  }

  double RSE(pair<double, uint64_t> C_occupancy) const {
    auto phi = [](double x) { return sqrt(exp(4 * x) - 1) / (2 * x); };
    auto [C, occupancy] = C_occupancy;
    auto b = data_[0].size();
    auto o = occupancy / 1.0 / b / 4;
    auto d = 4;
    auto f = W;
    return phi(C / (b * (1ul << f))) / sqrt(b * d * o);
  }
};
