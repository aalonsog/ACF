#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "rainbow.hpp"

string itoa(uint32_t i) {
  if (0 == i) return "0";
  string result;
  while (i) {
    result += ((i % 10) + '0');
    i = i / 10;
  }
  reverse(result.begin(), result.end());
  return result;
}

template <typename U, int W, typename T>
vector<T> GetPositives(DebugTable<U, W>& dt, T universeBegin, T universeEnd) {
  constexpr int kRepeats = 50;
  vector<T> v;
  for (auto i = universeBegin; i != universeEnd; ++i) {
    auto finder = dt.AdaptiveFind(i->first, i->second);
    if (finder != DebugTable<U, W>::TrueNegative) {
      bool found = true;
      if (finder == DebugTable<U, W>::FalsePositive) {
        for (int j = 0; j < kRepeats; ++j) {
          auto inner_finder = dt.AdaptiveFind(i->first, i->second);
          if (inner_finder == DebugTable<U, W>::TruePositive) break;
          if (inner_finder == DebugTable<U, W>::TrueNegative) {
            found = false;
            break;
          }
        }
      }
      if (found) v.push_back(i);
    }
  }
  return v;
}

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

  for (uint64_t kPopulation = 1 << 0; kPopulation < (1 << 20); kPopulation *= 2) {
    cout << "kPopulation " << kPopulation << endl;
    uint64_t kMaxUniverse = 1 << 25;
    constexpr auto kTagBits = 5;
    DebugTable<string, kTagBits> dt(
        1ul << static_cast<int>(ceil(log2(max(2.0, 1.5 * kPopulation / 4)))), x, y, z);
    vector<pair<string,uint32_t>> universe;
    for (uint32_t i = 0; i < kPopulation; ++i) {
      dt.Insert(itoa(i), i);
    }

    for (uint32_t i = 0; i < kMaxUniverse; ++i) {
      universe.push_back(make_pair(itoa(i), i));
    }

    auto vv = GetPositives(dt, universe.begin(), universe.end());
    vector<KeyCode<string>> v;
    for (auto& ww: vv) {
      v.push_back(KeyCode<string>{ww->first, ww->second});
    }

    Rainbow<string, kTagBits> r(dt.data_.size() - 1, v, x, y, z);
    auto e = r.Extract();
    vector<uint64_t> f(e.begin(), e.end());
    sort(f.begin(), f.end());

    if (f.size() != kPopulation) {
      cout << "dt.data_.size() " << dt.data_.size() << endl;
      cout << "v.size()        " << v.size() << endl;
      cout << "kPopulation     " << kPopulation << endl;
      cout << "e.size()        " << e.size() << endl;
      // for (auto x : v) {
      //   if (atoi(x.key.c_str()) >= kPopulation) cout << "uhoh before: " << x.key << endl;
      // }
      // for (auto x : e) {
      //   if (x >= kPopulation) cout << "uhoh after:  " << x << endl;
      // }
    }
    // assert(e.size() <= kPopulation);
    // assert(e.size() >= kPopulation);

    for (auto& t : f) {
      // cout << v[t].key << endl;
    }
    for (uint64_t i = 0; i < f.size(); ++i) {
      // assert(i < f.size());
      assert(atoi(v[f[i]].key.c_str()) >= i);
    }
  }
}
