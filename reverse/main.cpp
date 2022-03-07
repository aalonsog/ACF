#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "four-one.hpp"
#include "rainbow-four-one.hpp"
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

template <typename S, typename T>
pair<vector<T>, size_t> GetPositives(S& dt, T universeBegin, T universeEnd) {
  constexpr int kRepeats = 500;  // max(800 / W, 50);
  vector<T> v;
  size_t fpp = 0;
  for (auto i = universeBegin; i != universeEnd; ++i) {
    auto finder = dt.AdaptiveFind(i->first, i->second);
    if (finder != S::TrueNegative) {
      bool found = true;
      if (finder == S::FalsePositive) {
        ++fpp;
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
  return {v, fpp};
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


// template <int kTagBits, int Alpha>
// void TestRecover(const size_t kPopulation, size_t universe) {
// redo:
//   auto /*w = MS128::FromDevice(), */
//       x = MS64::FromDevice(),
//       y = MS64::FromDevice();
//   MS64 xy[4] = {MS64::FromDevice(), MS64::FromDevice(), MS64::FromDevice(),
//                 MS64::FromDevice()};
//   auto z = MS128::FromDevice();
//   // uint64_t kMaxUniverse = 1ul << 25;
//   size_t current_pop = 0;

//   DebugTable<uint64_t, kTagBits> dt(
//       1ul << static_cast<int>(ceil(log2(max(2.0, 1.5 * kPopulation / 4)))), x, y, z);
//   FourOne<uint64_t, kTagBits, Alpha> fo(
//       2ul << static_cast<int>(ceil(log2(max(2.0, 1.5 * kPopulation / 4)))), xy, z);
//   for (uint32_t i = 0; i < 100 * kPopulation; ++i) {
//     try {
//       dt.Insert(i, i);
//       fo.Insert(i, i);
//       const double filled = 100.0 * i / dt.Capacity();
//       if (filled >= 95.0) {
//         // cout << "filled " << filled << '%' << endl;
//         current_pop = i + 1;
//         // cout << "kPopulation " << kPopulation << endl;
//         // cout << "dt.data_.size() " << dt.data_.size() << endl;
//         break;
//       }
//     } catch (...) {
//       // cout << "failed " << (100.0 * i / dt.data_.size() / 4) << '%' << endl;
//       // // TODO: how to undo this?
//       // kPopulation = 1ul << 20;
//       cout << "fill failed; retrying " << (i+1) << " " << (100.0 * i / dt.Capacity()) << endl;
//       throw 'r';
//       goto redo;
//     }
//   }

//   auto vv = GetPositives(dt, Int64Iterator(0), Int64Iterator(universe));
//   auto ww = GetPositives(fo, Int64Iterator(0), Int64Iterator(universe));
//   vector<KeyCode<uint64_t>> v, w;
//   for (auto& xx : vv) v.push_back(KeyCode<uint64_t>{xx->first, xx->second});
//   for (auto& xx : ww) w.push_back(KeyCode<uint64_t>{xx->first, xx->second});

//   //cout << v.size() << endl;
//   // Rainbow<uint64_t, kTagBits> s(dt.Capacity(), v, x, y, z);
//   //  auto e = s.Extract();
//   auto e = RainbowExtract<kTagBits, Alpha>(dt, v);
//   //RainbowFourOne<uint64_t, kTagBits, Alpha> r(fo.Capacity(), w, xy, z);
//   //auto f = r.Extract();
//   auto f = RainbowExtract<kTagBits, Alpha>(fo, w);
//   bool ok = true;
//   for (auto i : e) {
//     if (i >= current_pop) {
//       //cout << "invalid extraction " << i << '\t' << current_pop << endl;
//       ok = false;
//     }
//   }
//   for (auto i : f) {
//     if (i >= current_pop) {
//       //cout << "invalid extraction " << i << '\t' << current_pop << endl;
//       ok = false;
//     }
//   }
//   //vector<uint64_t> f(e.begin(), e.end());
//   //sort(f.begin(), f.end());

//   //cout << "f_b = " << kTagBits << "\t";
//   cout << "recovered_24,positives_24,truepop_24,recovered_41,positives_41,truepop_41\n";
//   cout << e.size() << ",";
//   cout << vv.size() << ",";
//   cout << current_pop << ",";
//   cout << f.size() << ",";
//   cout << ww.size() << ",";
//   cout << current_pop << endl;
//   if (not ok) throw "invalid";
// }

template <int kTagBits, int Alpha, typename T>
size_t TestRecoverEither(T& dt, const size_t kPopulation, size_t universe) {
redo:
  // auto /*w = MS128::FromDevice(), */
  //     x = MS64::FromDevice(),
  //     y = MS64::FromDevice();
  // MS64 xy[4] = {MS64::FromDevice(), MS64::FromDevice(), MS64::FromDevice(),
  //               MS64::FromDevice()};
  // auto z = MS128::FromDevice();
  // uint64_t kMaxUniverse = 1ul << 25;

  // auto h = [](uint32_t x) { return x / 2; };
  // auto h = [](uint32_t x) { return x; };

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
      throw 'r';
      goto redo;
    }
  }

  auto vv = GetPositives(dt, Int64Iterator(0), Int64Iterator(universe));
  vector<KeyCode<uint64_t>> v, w;
  for (auto& xx : vv.first) v.push_back(KeyCode<uint64_t>{xx->first, xx->second});

  auto e = RainbowExtract<kTagBits, decltype(dt.h), Alpha>(dt, v);
  bool ok = true;
  for (auto i : e) {
    if (i >= current_pop) {
      //cout << "invalid extraction " << i << '\t' << current_pop << endl;
      ok = false;
    }
  }
  (void)ok;
  //vector<uint64_t> f(e.begin(), e.end());
  //sort(f.begin(), f.end());

  //cout << "f_b = " << kTagBits << "\t";
  cout << e.size() << ",";
  cout << vv.first.size() << ",";
  cout << vv.second * 100.0 / (universe - current_pop) << ",";
  cout.flush();
  if (not is_same<DebugTable<uint64_t, kTagBits, decltype(dt.h)>, T>::value) {
    //if (not ok) throw "invalid";
  }
  return current_pop;
}

template <int kTagBits, int Alpha, bool protect>
size_t TestRecover24(const size_t kPopulation, size_t universe) {
  auto x = MS64::FromDevice(), y = MS64::FromDevice();
  auto z = MS128::FromDevice();
  auto h = [](uint32_t x) { return (0x87ad153f * x) >> 1; };
  auto g = [](uint32_t x) { return x; };
  if (protect) {
    DebugTable<uint64_t, kTagBits, decltype(h)> dt(
        h, 1ul << static_cast<int>(ceil(log2(max(2.0, 1.5 * kPopulation / 4)))), x, y, z);
    return TestRecoverEither<kTagBits, Alpha>(dt, kPopulation, universe);
  } else {
    DebugTable<uint64_t, kTagBits, decltype(g)> dt(
        g, 1ul << static_cast<int>(ceil(log2(max(2.0, 1.5 * kPopulation / 4)))), x, y, z);
    return TestRecoverEither<kTagBits, Alpha>(dt, kPopulation, universe);
  }
};

template <int kTagBits, int Alpha, bool protect>
size_t TestRecover41(const size_t kPopulation, size_t universe) {
  MS64 xy[4] = {MS64::FromDevice(), MS64::FromDevice(), MS64::FromDevice(),
                MS64::FromDevice()};
  auto z = MS128::FromDevice();
  auto h = [](uint32_t x) { return (0x87ad153f * x) >> 1; };
  auto g = [](uint32_t x) { return x; };
  if (protect) {
    FourOne<uint64_t, kTagBits, Alpha, decltype(h)> dt(
        h, 1ul << static_cast<int>(ceil(log2(max(2.0, 1.5 * kPopulation / 4)))), xy, z);
    return TestRecoverEither<kTagBits, Alpha>(dt, kPopulation, universe);
  } else {
    FourOne<uint64_t, kTagBits, Alpha, decltype(g)> dt(
        g, 1ul << static_cast<int>(ceil(log2(max(2.0, 1.5 * kPopulation / 4)))), xy, z);
    return TestRecoverEither<kTagBits, Alpha>(dt, kPopulation, universe);
  }
};

// read strings from stdin, put them in a Rainbow, then print all keys that can be
// recovered
int main(int, char ** argv) {
  constexpr bool protect = true;
  constexpr size_t universe = 1ul << 32;
  istringstream s(argv[1]);
  int f_b = 0;
  s >> f_b;
  const unsigned long count = 1ul << 14;
  cout << "recovered24,positives24,fpp24,recovered411,positives411,fpp411,recovered412,"
          "positives412,fpp412,recovered413,positives413,fpp412,fill_count"
       << endl;
  switch (f_b) {
#define effbee(F_B)                                          \
  case F_B:                                                  \
    TestRecover24<F_B, 0, protect>(count, universe);         \
    TestRecover41<F_B, 1, protect>(count, universe);         \
    TestRecover41<F_B, 2, protect>(count, universe);         \
    cout << TestRecover41<F_B, 3, protect>(count, universe); \
    cout << endl;                                            \
    return 0;

    effbee(4);
    effbee(5);
    effbee(6);
    effbee(7);
    effbee(8);
    effbee(9);
    effbee(10);
    effbee(11);
    effbee(12);
    effbee(13);
    effbee(14);
    effbee(15);
    effbee(16);

    default:
      throw 0;
  }
}
