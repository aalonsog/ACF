#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
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
  unsigned __int128 m, a;
  static MS64 FromDevice() {
    random_device d;
    MS64 result;
    for (int i = 0; i < 8; ++i) {
      reinterpret_cast<uint32_t*>(&result)[i] = d();
    }
    return result;
  }
  uint64_t operator()(uint64_t x) const {
    static_assert(std::is_same<decltype(x * m), decltype(m)>::value, "64 * 128 = 128");
    return ((x * m) + a) >> 64;
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


template<typename Z, int W>
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

  DebugTable(size_t bucket_count, MS64 hash_maker, MS64 xor_maker, MS128 print_makers)
      : data_(bucket_count),
        hash_maker_(hash_maker),
        xor_maker_(xor_maker),
        print_makers_(print_makers) {
    if (0 != (bucket_count & (bucket_count - 1))) throw bucket_count;
    srand(0);
  }

  void Insert(Z key, uint64_t code) {
    auto hash = hash_maker_(code) & (data_.size() - 1);
    int ttl = 100;
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
      int i = rand() % 4;
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
