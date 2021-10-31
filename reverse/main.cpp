// Performs key inference on an adaptive cuckoo filter

#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "debug-table.hpp"

using namespace std;

// // Three strong typedefs to avoid accidentally confusing one integer for another

// struct Fingerprints {
//   uint64_t fingerprints;
// };

// bool operator==(const Fingerprints& x, const Fingerprints& y) {
//   return x.fingerprints == y.fingerprints;
// }

// template <>
// struct hash<Fingerprints> {
//   size_t operator()(Fingerprints x) const {
//     hash<uint64_t> h;
//     return h(x.fingerprints);
//   }
// };

// struct Index {
//   uint64_t index;
// };

// bool operator==(const Index& x, const Index& y) {
//   return x.index == y.index;
// }

// template <>
// struct hash<Index> {
//   size_t operator()(Index x) const {
//     hash<uint64_t> h;
//     return h(x.index);
//   }
// };

// struct KeyIndex {
//   uint64_t key_index;
// };

// bool operator==(const KeyIndex& x, const KeyIndex& y) {
//   return x.key_index == y.key_index;
// }

// template <>
// struct hash<KeyIndex> {
//   size_t operator()(KeyIndex x) const {
//     hash<uint64_t> h;
//     return h(x.key_index);
//   }
// };

template <typename T>
struct KeyCode {
  T key;
  uint64_t code;
};

// Rainbow is a table structure to organize input keys by their fingerprint values, then
// their index values (location in the ACF).
//
// To use, the caller needs to know the identities of the keys, the hash function
// producing fingerprints (here as four 16-bit values in on uint64_t), and the two hash
// functions producing indexes into the ACF.
template<typename T, int W>
struct Rainbow {
  static_assert(W <= 16, "W <= 16");
  // fingerprint -> index_into_filter -> index_into_key_vector
  uint64_t mask;
  unordered_map<uint64_t, unordered_map<uint64_t, unordered_set<uint64_t>>> table;
  const vector<KeyCode<T>>& keys;
  MS64 hash_maker;
  MS64 xor_maker;

  template <typename U>
  Rainbow(uint64_t mask, const vector<KeyCode<T>>& keys, MS64 hash_maker, MS64 xor_maker,
          U fingerprinter)
      : mask(mask), keys(keys), hash_maker(hash_maker), xor_maker(xor_maker) {
    size_t n = keys.size();
    for(uint64_t i = 0; i < n; ++i) {
      for (uint64_t index :
           {hash_maker(keys[i].code) & mask,
            (hash_maker(keys[i].code) ^ xor_maker(keys[i].code)) & mask}) {
        table[fingerprinter(keys[i].code) & ((1ul << (4 * W % 64)) - 1)][index].insert(i);
      }
    }
  }

  unordered_set<uint64_t> Extract() {
    unordered_set<uint64_t> result;
    for (auto& bucket : table) {
      for (auto& index_map : bucket.second) {
        if (index_map.second.size() > 1) {
          // cerr << "unrecoverable\n";
          continue;
        }
        auto ki = *index_map.second.begin();
        bool unique = true;
        for (uint64_t index :
             {hash_maker(keys[ki].code) & mask,
              (hash_maker(keys[ki].code) ^ xor_maker(keys[ki].code)) & mask}) {
          if (bucket.second.find(index)->second.size() > 1) {
            // cerr << "unrecoverable\n";
            unique = false;
            break;
          }
        }
        if (unique) result.insert(ki);
      }
    }
    return result;
  }
};

// read strings from stdin, put them in a Rainbow, then print all keys that can be
// recovered
int main() {
  auto w = MS64::FromDevice(), x = MS64::FromDevice(), y = MS64::FromDevice(),
       z = MS64::FromDevice();

  {
    uint64_t aa[16] = {
        0x5e1a75e3d05242b5, 0xe3474a91193544da, 0xbedf0f3064fd4f74, 0x38588d84069f4bb9,
        0x7deb04d287ab49b7, 0xec734743a4c84a15, 0x096dadf136c74bad, 0x94fbb085d0f245b1,
        0xa9cfc872640f4743, 0x50871f18f028408d, 0x3fffa4125d334bb0, 0x8a6748239a2a48ab,
        0x21fb2e9b7c3340b0, 0x95b94d1f03f84a3a, 0x175bd55ef7764f54, 0x1e4328f8e88a4428};
    w.m = (static_cast<unsigned __int128>(aa[0]) << 64) | aa[1];
    x.m = (static_cast<unsigned __int128>(aa[2]) << 64) | aa[3];
    y.m = (static_cast<unsigned __int128>(aa[4]) << 64) | aa[5];
    z.m = (static_cast<unsigned __int128>(aa[6]) << 64) | aa[7];
    w.a = (static_cast<unsigned __int128>(aa[8]) << 64) | aa[9];
    x.a = (static_cast<unsigned __int128>(aa[10]) << 64) | aa[11];
    y.a = (static_cast<unsigned __int128>(aa[12]) << 64) | aa[13];
    z.a = (static_cast<unsigned __int128>(aa[14]) << 64) | aa[15];
  }
  // cout << w.m[0] << endl;
  // if (0) {
  //   vector<KeyCode<string>> v;
  //   string s;
  //   while (cin >> s) v.push_back(KeyCode<string>{s, w(hash<string>()(s))});
  //   Rainbow<string, 16> r(0, v, x, y, [z](const string& s) { return z(hash<string>()(s)); });
  //   auto e = r.Extract();
  //   for (auto& t : e) {
  //     cout << v[t].key << endl;
  //   }
  // }

  //  if (0)

  {
    constexpr auto kRepeats = 100;
    for (uint64_t kPopulation = 2; kPopulation <= 770; ++kPopulation) {
      uint64_t kMaxUniverse = 1 << 20; // 18544 << 0;
      constexpr auto kTagBits = 4;
      DebugTable<uint32_t, kTagBits> dt(
          1ul << static_cast<int>(ceil(log2(1.15 * kPopulation))), x, y, z);
      for (uint32_t i = 0; i < kPopulation; ++i) {
        dt.Insert(i, w(i));
      }

      vector<KeyCode<uint32_t>> v;
      for (uint64_t i = 0; i < kMaxUniverse; ++i) {
        //if (0 == (i & (i - 1))) cerr << i << " " << v.size() << endl;

        auto finder = dt.AdaptiveFind(i, w(i));
        if (finder != DebugTable<uint32_t, kTagBits>::TrueNegative) {
          bool found = true;
          if (finder == DebugTable<uint32_t, kTagBits>::FalsePositive) {
            for (int j = 0; j < kRepeats; ++j) {
              if (DebugTable<uint32_t, kTagBits>::TrueNegative ==
                  dt.AdaptiveFind(i, w(i))) {
                found = false;
                break;
              }
            }
          }
          if (found) v.push_back(KeyCode<uint32_t>{static_cast<uint32_t>(i), w(i)});
        }
      }
      // assert(v.size() == kPopulation && "v.size == kPopulation");

      Rainbow<uint32_t, kTagBits> r(dt.data_.size() - 1, v, x, y, z);
      auto e = r.Extract();
      cout << "v.size()    " << v.size() << endl;
      cout << "kPopulation " << kPopulation << endl;
      cout << "e.size()    " << e.size() << endl;
      assert(e.size() <= kPopulation);
      //assert(e.size() >= kPopulation);
      vector<uint32_t> f(e.begin(), e.end());
      sort(f.begin(), f.end());

      for (auto& t : f) {
        // cout << v[t].key << endl;
      }
      for (uint64_t i = 0; i < f.size(); ++i) {
        // assert(i < f.size());
        assert(v[f[i]].key >= i);
      }
    }
  }
}
