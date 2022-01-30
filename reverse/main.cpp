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
  constexpr int kRepeats = 100;  // max(800 / W, 50);
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


template <int kTagBits, int Alpha>
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

  DebugTable<uint64_t, kTagBits> dt(
      1ul << static_cast<int>(ceil(log2(max(2.0, 1.5 * kPopulation / 4)))), x, y, z);
  FourOne<uint64_t, kTagBits, Alpha> fo(
      2ul << static_cast<int>(ceil(log2(max(2.0, 1.5 * kPopulation / 4)))), xy, z);
  for (uint32_t i = 0; i < 100 * kPopulation; ++i) {
    try {
      dt.Insert(i, i);
      fo.Insert(i, i);
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
  auto ww = GetPositives(fo, Int64Iterator(0), Int64Iterator(universe));
  vector<KeyCode<uint64_t>> v, w;
  for (auto& xx : vv) v.push_back(KeyCode<uint64_t>{xx->first, xx->second});
  for (auto& xx : ww) w.push_back(KeyCode<uint64_t>{xx->first, xx->second});

  //cout << v.size() << endl;
  // Rainbow<uint64_t, kTagBits> s(dt.Capacity(), v, x, y, z);
  //  auto e = s.Extract();
  auto e = RainbowExtract<kTagBits, Alpha>(dt, v);
  //RainbowFourOne<uint64_t, kTagBits, Alpha> r(fo.Capacity(), w, xy, z);
  //auto f = r.Extract();
  auto f = RainbowExtract<kTagBits, Alpha>(fo, w);
  bool ok = true;
  for (auto i : e) {
    if (i >= current_pop) {
      //cout << "invalid extraction " << i << '\t' << current_pop << endl;
      ok = false;
    }
  }
  for (auto i : f) {
    if (i >= current_pop) {
      //cout << "invalid extraction " << i << '\t' << current_pop << endl;
      ok = false;
    }
  }
  //vector<uint64_t> f(e.begin(), e.end());
  //sort(f.begin(), f.end());

  //cout << "f_b = " << kTagBits << "\t";
  cout << "recovered_24,positives_24,truepop_24,recovered_41,positives_41,truepop_41\n";
  cout << e.size() << ",";
  cout << vv.size() << ",";
  cout << current_pop << ",";
  cout << f.size() << ",";
  cout << ww.size() << ",";
  cout << current_pop << endl;
  if (not ok) throw "invalid";
}

template <int kTagBits, int Alpha, typename T>
void TestRecoverEither(T& dt, const size_t kPopulation, size_t universe) {
redo:
  // auto /*w = MS128::FromDevice(), */
  //     x = MS64::FromDevice(),
  //     y = MS64::FromDevice();
  // MS64 xy[4] = {MS64::FromDevice(), MS64::FromDevice(), MS64::FromDevice(),
  //               MS64::FromDevice()};
  // auto z = MS128::FromDevice();
  // uint64_t kMaxUniverse = 1ul << 25;
  size_t current_pop = 0;

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
  vector<KeyCode<uint64_t>> v, w;
  for (auto& xx : vv) v.push_back(KeyCode<uint64_t>{xx->first, xx->second});

  auto e = RainbowExtract<kTagBits, Alpha>(dt, v);
  bool ok = true;
  for (auto i : e) {
    if (i >= current_pop) {
      //cout << "invalid extraction " << i << '\t' << current_pop << endl;
      ok = false;
    }
  }
  //vector<uint64_t> f(e.begin(), e.end());
  //sort(f.begin(), f.end());

  //cout << "f_b = " << kTagBits << "\t";
  cout << e.size() << ",";
  cout << vv.size() << ",";
  cout.flush();
  if (not ok) throw "invalid";
}


template <int kTagBits, int Alpha>
void TestRecover24(const size_t kPopulation, size_t universe) {
  auto x = MS64::FromDevice(), y = MS64::FromDevice();
  auto z = MS128::FromDevice();
  DebugTable<uint64_t, kTagBits> dt(
      1ul << static_cast<int>(ceil(log2(max(2.0, 1.5 * kPopulation / 4)))), x, y, z);
  return TestRecoverEither<kTagBits, Alpha>(dt, kPopulation, universe);
};

template <int kTagBits, int Alpha>
void TestRecover41(const size_t kPopulation, size_t universe) {
  MS64 xy[4] = {MS64::FromDevice(), MS64::FromDevice(), MS64::FromDevice(),
                MS64::FromDevice()};
  auto z = MS128::FromDevice();
  FourOne<uint64_t, kTagBits, Alpha> dt(
      1ul << static_cast<int>(ceil(log2(max(2.0, 1.5 * kPopulation / 4)))), xy, z);
  return TestRecoverEither<kTagBits, Alpha>(dt, kPopulation, universe);
};

// read strings from stdin, put them in a Rainbow, then print all keys that can be
// recovered
int main(int, char ** argv) {
  constexpr size_t universe = 1ul << 20;
  istringstream s(argv[1]);
  int f_b = 0;
  s >> f_b;
  const unsigned long count = 1ul << 17;
  switch (f_b) {
#define effbee(F_B)                                                           \
  case F_B:                                                                   \
    TestRecover<F_B, 1>(count, universe);                                     \
    return 0;                                                                 \
    cout << "recovered24,positives24,recovered411,positives411,recovered412," \
            "positives412,recovered413,positives413"                          \
         << endl;                                                             \
    TestRecover24<F_B, 0>(count, universe);                                   \
    TestRecover41<F_B, 1>(count, universe);                                   \
    TestRecover41<F_B, 2>(count, universe);                                   \
    TestRecover41<F_B, 3>(count, universe);                                   \
    cout << endl;                                                             \
    return 0;

    effbee(4);
    effbee(6);
    effbee(8);
    effbee(10);
    effbee(12);
    effbee(14);
    effbee(16);

    default:
      throw 0;
  }
}
