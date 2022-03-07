#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace std;

struct MS32 {
  uint64_t m0, a0, m1, a1;
  uint64_t operator()(uint32_t x) {
    uint32_t result0 = ((x * m0) + a0) >> 32;
    uint32_t result1 = ((x * m1) + a1) >> 32;
    return result0 | (static_cast<uint64_t>(result1) << 32);
  }
};

struct MS64 {
  // unsigned __int128 m, a;
  uint64_t seed[8][256];
  static MS64 FromDevice() {
    random_device d;
    MS64 result;
    for (int i = 0; i < 8; ++i) {
      for (int j = 0; j < 256; ++j) {
        result.seed[i][j] = d();
        result.seed[i][j] |= static_cast<uint64_t>(d()) << 32;
      }
    }
    return result;
  }
  uint64_t operator()(uint64_t x) const {
    uint64_t result = 0;
    for (int i = 0; i < 8; ++i) {
      result ^= seed[i][(x >> (i * 8)) & 0xff];
    }
    return result;
  }
};

struct MS128 {
  MS64 p[2];
  static MS128 FromDevice() {
    MS128 x;
    x.p[0] = MS64::FromDevice();
    x.p[1] = MS64::FromDevice();
    return x;
  }
  unsigned __int128 operator()(uint64_t x) const {
    unsigned __int128 result = p[0](x);
    result = result << 64;
    result |= p[1](x);
    return result;
  }
};

struct LazyTabHash {
  vector<array<uint64_t, 8>> seed;
  uint64_t operator()(char* x, size_t n) {
    while (seed.size() < n) {
      random_device d;
      seed.push_back({});
      for (size_t i = 0; i < 256; ++i) {
        seed.back()[i] = d();
        seed.back()[i] |= static_cast<uint64_t>(d()) << 32;
      }
    }
    uint64_t result = 0;
    for (size_t i = 0; i < n; ++i) {
      result ^= seed[i][x[i]];
    }
    return result;
  }
};


template<typename Z, int W, typename H>
struct DebugTable {
  static_assert(W <= 32, "W <= 32");
  struct Slot {
    bool full = false;
    uint64_t fingerprint : W;
    uint64_t code;
    Z back;
  };

  vector<array<Slot, 4>> data_;
  MS64 hash_maker_;
  MS64 xor_maker_;
  MS128 print_makers_;
  H h;

  size_t Capacity() const { return data_.size() * 4; }

  DebugTable(const H& h, size_t bucket_count, MS64 hash_maker, MS64 xor_maker, MS128 print_makers)
      : data_(bucket_count),
        hash_maker_(hash_maker),
        xor_maker_(xor_maker),
        print_makers_(print_makers),
        h(h) {
    if (0 != (bucket_count & (bucket_count - 1))) throw bucket_count;
    srand(0);
  }

  void Insert(Z key, uint64_t code) {
    code = h(code);
    auto hash = hash_maker_(code) & (data_.size() - 1);
    int ttl = 500;
    random_device d;
    while (true) {
      for (int j = 0; j < 4; ++j) {
        if (data_[hash][j].full) {
          continue;
          // assert(data_[hash][j].fingerprint ==
          //            (print_makers_(data_[hash][j].code) >> (j * W)) & ((1 << W) - 1));
        }
        data_[hash][j].back = key;
        data_[hash][j].code = code;
        data_[hash][j].fingerprint = print_makers_(code) >> (j * W);
        data_[hash][j].full = true;
        return;
      }
      int i = d() % 4;
      using std::swap;
      swap(key, data_[hash][i].back);
      swap(code, data_[hash][i].code);
      data_[hash][i].fingerprint = print_makers_(data_[hash][i].code) >> (i * W);
      auto new_hash = hash_maker_(code) & (data_.size() - 1);
      if (new_hash == hash) new_hash = (new_hash ^ xor_maker_(code)) & (data_.size() - 1);
      hash = new_hash;
      --ttl;
      if (ttl < 0) throw key;
    }
  }

  enum Found { TrueNegative = 0, TruePositive = 1, FalsePositive = 2 };
  Found AdaptiveFind(const Z& key, uint64_t code) {
    code = h(code);
    bool debug = false;
    // if (code == 985567 or code == 995618) {
    //   debug = false;
    //   //cout << "here\n";
    // }
    auto hash = hash_maker_(code) & (data_.size() - 1);
    //cout << hex << print_makers_(code) << endl;
    for (int i = 0; i < 2; ++i) {
      for (int j = 0; j < 4; ++j) {
        if (not data_[hash][j].full) continue;

        // if (debug) {
        //   cout << code << " present " << data_[hash][j].code << " prints " << hex
        //        << (print_makers_(data_[hash][j].code) & ((1ul << 4 * W) - 1)) << dec
        //        << endl;
        // }
        if (data_[hash][j].fingerprint !=
            ((print_makers_(code) >> (j * W)) & ((1ul << W) - 1))) {
          continue;
        }
        if (key == data_[hash][j].back) return TruePositive;
        //cout << "fp " << j << " " << data_[hash][j].fingerprint << endl;
        int k = j;
        while (k == j) k = rand() % 4;
        if (debug) cout << j << " new " << k << endl;
        //cout << "swap " << k << " " << data_[hash][j].back << " " << data_[hash][k].back << endl;
        using std::swap;
        swap(data_[hash][j].full, data_[hash][k].full);
        swap(data_[hash][j].back, data_[hash][k].back);
        swap(data_[hash][j].code, data_[hash][k].code);
        for (auto n : {j, k}) {
          data_[hash][n].fingerprint = print_makers_(data_[hash][n].code) >> (n * W);
        }
        return FalsePositive;
      }
      hash = (hash ^ xor_maker_(code)) & (data_.size() - 1);
    }
    return TrueNegative;
  }
};
