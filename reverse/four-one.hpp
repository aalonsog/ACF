#pragma once

#include <cstdint>
#include <vector>
#include <array>

using namespace std;

#include "debug-table.hpp"

template<typename Z, int W, int S>
struct FourOne {
  struct Slot {
    bool full = false;
    uint16_t fingerprint : W;
    unsigned short alpha: S;
    uint64_t code;
    Z back;
  };

  vector<Slot> data_[4];
  MS64 hash_makers_[4];
  MS128 print_maker_;

  size_t Capacity() const { return data_[0].size() * 4; }

  FourOne(size_t bucket_count, const MS64 hash_makers[4], MS128 print_maker)
      : data_{vector<Slot>{bucket_count}, vector<Slot>{bucket_count},
              vector<Slot>{bucket_count}, vector<Slot>{bucket_count}},
        hash_makers_{hash_makers[0], hash_makers[1], hash_makers[2], hash_makers[3]},
        print_maker_(print_maker) {
    if (0 != (bucket_count & (bucket_count - 1))) throw bucket_count;
  }

  void Insert(Z key, uint64_t code) {
    int ttl = 500;
    for (int j = 0; true; j = ((j + 1) % 4)) {
      auto hash = hash_makers_[j](code) & (data_[j].size() - 1);
      if (not data_[j][hash].full) {
        data_[j][hash].full = true;
        data_[j][hash].fingerprint = print_maker_(code);
        // cout << code << " " << data_[j][hash].fingerprint << endl;
        data_[j][hash].alpha = 0;
        data_[j][hash].code = code;
        data_[j][hash].back = key;
        return;
      }
      using std::swap;
      swap(key, data_[j][hash].back);
      swap(code, data_[j][hash].code);
      //data_[j][hash].alpha += 1;
      //data_[j][hash].alpha = data_[j][hash].alpha % ((1 << S) - 1);
      data_[j][hash].fingerprint =
          print_maker_(data_[j][hash].code) >> (data_[j][hash].alpha * W);
      --ttl;
      if (ttl < 0) throw key;
    }
  }

  enum Found { TrueNegative = 0, TruePositive = 1, FalsePositive = 2 };
  Found AdaptiveFind(Z key, uint64_t code) {
    auto result = TrueNegative;
    for (int i = 0; i < 4; ++i) {
      auto hash = hash_makers_[i](code) & (data_[i].size() - 1);
      if (not data_[i][hash].full) continue;
      // cout << code << " " << data_[i][hash].fingerprint << " "
      //      << static_cast<uint64_t>(print_maker_(code) >> (data_[i][hash].alpha * W)) << endl;
      if (((print_maker_(code) >> (data_[i][hash].alpha * W)) & ((1ull << W) - 1)) ==
          data_[i][hash].fingerprint) {
        if (key == data_[i][hash].back) return TruePositive;
        result = FalsePositive;
        data_[i][hash].alpha += 1;
        data_[i][hash].alpha = data_[i][hash].alpha & ((1 << S) - 1);
        data_[i][hash].fingerprint =
          print_maker_(data_[i][hash].code) >> (data_[i][hash].alpha * W);
      }
    }
    return result;
  }
};
