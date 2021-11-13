#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <sstream>
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

struct Int64Iterator {
  pair<uint64_t, uint64_t> payload;
  explicit Int64Iterator(uint64_t x) : payload{x, x} {}
  bool operator!=(const Int64Iterator& x) { return payload.first != x.payload.second; }
  Int64Iterator& operator++() {
    ++payload.first;
    ++payload.second;
    return *this;
  }
  pair<uint64_t, uint64_t>* operator->() { return &payload; }
};


template <int kTagBits>
void TestRecover(size_t kPopulation, size_t universe) {
redo:
  auto /*w = MS128::FromDevice(), */
      x = MS64::FromDevice(),
      y = MS64::FromDevice();
  auto z = MS128::FromDevice();
  // uint64_t kMaxUniverse = 1ul << 25;

  DebugTable<uint64_t, kTagBits> dt(
      1ul << static_cast<int>(ceil(log2(max(2.0, 1.5 * kPopulation / 4)))), x, y, z);
  for (uint32_t i = 0; i < 100 * kPopulation; ++i) {
    try {
      dt.Insert(i, i);
      const double filled = 100.0 * i / dt.data_.size() / 4;
      if (filled >= 95.0) {
        // cout << "filled " << filled << '%' << endl;
        kPopulation = i + 1;
        // cout << "kPopulation " << kPopulation << endl;
        // cout << "dt.data_.size() " << dt.data_.size() << endl;
        break;
      }
    } catch (...) {
      // cout << "failed " << (100.0 * i / dt.data_.size() / 4) << '%' << endl;
      // // TODO: how to undo this?
      // kPopulation = 1ul << 20;
      // cout << "kPopulation " << kPopulation << endl;
      goto redo;
    }
  }

  auto vv = GetPositives(dt, Int64Iterator(0), Int64Iterator(universe));
  vector<KeyCode<uint64_t>> v;
  for (auto& ww : vv) {
    v.push_back(KeyCode<uint64_t>{ww->first, ww->second});
  }

  Rainbow<uint64_t, kTagBits> r(dt.data_.size() - 1, v, x, y, z);
  auto e = r.Extract();
  vector<uint64_t> f(e.begin(), e.end());
  sort(f.begin(), f.end());

  cout << "f_b = " << kTagBits << "\t";
  cout << "recovered " << 100.0 * e.size() / kPopulation << "%\t";
  cout << "unrecovered " << 100.0 - 100.0 * e.size() / kPopulation << '%' << endl;
}

// read strings from stdin, put them in a Rainbow, then print all keys that can be
// recovered
int main(int, char ** argv) {
  constexpr size_t universe = 1ul << 32;
  istringstream s(argv[1]);
  int f_b;
  s >> f_b;
  switch (f_b) {
  case 4:
    TestRecover<4>(1ul << 18, universe);
    break;
  case 6:
    TestRecover<6>(1ul << 18, universe);
    break;
  case 8:
    TestRecover<8>(1ul << 18, universe);
    break;
  case 10:
    TestRecover<10>(1ul << 18, universe);
    break;
  case 12:
    TestRecover<12>(1ul << 18, universe);
    break;
  case 14:
    TestRecover<14>(1ul << 18, universe);
    break;
  case 16:
    TestRecover<16>(1ul << 18, universe);
    break;
  default:
    throw 0;
  }
}
