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
#include "four-one.hpp"
#include "rainbow-four-one.hpp"

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

template <typename S, typename T>
vector<T> GetPositives(S& dt, T universeBegin, T universeEnd) {
  constexpr int kRepeats = 50;  // max(800 / W, 50);
  vector<T> v;
  for (auto i = universeBegin; i != universeEnd; ++i) {
    auto finder = dt.AdaptiveFind(i->first, i->second);
    if (finder != S::TrueNegative) {
      bool found = true;
      if (finder == S::FalsePositive) {
        for (int j = 0; j < kRepeats; ++j) {
          auto inner_finder = dt.AdaptiveFind(i->first, i->second);
          if (inner_finder == S::TruePositive) break;
          if (inner_finder == S::TrueNegative) {
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
void TestRecover(const size_t kPopulation, size_t universe) {
redo:
  auto /*w = MS128::FromDevice(), */
      x = MS64::FromDevice(),
      y = MS64::FromDevice();
  MS64 xy[4] = {MS64::FromDevice(), MS64::FromDevice(), MS64::FromDevice(),
                MS64::FromDevice()};
  auto z = MS128::FromDevice();
  // uint64_t kMaxUniverse = 1ul << 25;
  size_t current_pop = 0;

  // DebugTable<uint64_t, kTagBits> dt(
  //     1ul << static_cast<int>(ceil(log2(max(2.0, 1.5 * kPopulation / 4)))), x, y, z);
  FourOne<uint64_t, kTagBits, 3> dt(
      1ul << static_cast<int>(ceil(log2(max(2.0, 1.5 * kPopulation / 4)))), xy, z);
  for (uint32_t i = 0; i < 100 * kPopulation; ++i) {
    try {
      dt.Insert(i, i);
      const double filled = 100.0 * i / dt.Capacity();
      if (filled >= 95.0) {
        // cout << "filled " << filled << '%' << endl;
        current_pop = i + 1;
        // cout << "kPopulation " << kPopulation << endl;
        // cout << "dt.data_.size() " << dt.data_.size() << endl;
        break;
      }
    } catch (...) {
      // cout << "failed " << (100.0 * i / dt.data_.size() / 4) << '%' << endl;
      // // TODO: how to undo this?
      // kPopulation = 1ul << 20;
      cout << "fill failed; retrying " << (i+1) << " " << (100.0 * i / dt.Capacity()) << endl;
      goto redo;
    }
  }

  auto vv = GetPositives(dt, Int64Iterator(0), Int64Iterator(universe));
  vector<KeyCode<uint64_t>> v;
  for (auto& ww : vv) {
    v.push_back(KeyCode<uint64_t>{ww->first, ww->second});
  }
  //cout << v.size() << endl;
  RainbowFourOne<uint64_t> r(dt.Capacity(), v, xy, z);
  auto e = r.Extract();
  bool ok = true;
  for (auto i : e) {
    if (i >= current_pop) {
      //cout << "invalid extraction " << i << '\t' << current_pop << endl;
      ok = false;
    }
  }
  //vector<uint64_t> f(e.begin(), e.end());
  //sort(f.begin(), f.end());

  cout << "f_b = " << kTagBits << "\t";
  cout << "recovered " << 100.0 * e.size() / current_pop << "%\t";
  cout << "unrecovered " << 100.0 - 100.0 * e.size() / current_pop << "%\t";
  cout << "positives " << vv.size() << "\t";
  cout << "true pop = " << current_pop << endl;
  if (not ok) throw "invalid";
}

// read strings from stdin, put them in a Rainbow, then print all keys that can be
// recovered
int main(int, char ** argv) {
  constexpr size_t universe = 1ul << 28;
  istringstream s(argv[1]);
  int f_b;
  s >> f_b;
  const unsigned long count = 1ul << 16;
  switch (f_b) {
  case 4:
    TestRecover<4>(count, universe);
    break;
  case 5:
    TestRecover<5>(count, universe);
    break;
  case 6:
    TestRecover<6>(count, universe);
    break;
  case 7:
    TestRecover<7>(count, universe);
    break;
  case 8:
    TestRecover<8>(count, universe);
    break;
  case 10:
    TestRecover<10>(count, universe);
    break;
  case 12:
    TestRecover<12>(count, universe);
    break;
  case 14:
    TestRecover<14>(count, universe);
    break;
  case 15:
    TestRecover<15>(count, universe);
    break;
  case 16:
    TestRecover<16>(count, universe);
    break;
  // case 17:
  //   TestRecover<17>(count, universe);
  //   break;
  default:
    throw 0;
  }
}
