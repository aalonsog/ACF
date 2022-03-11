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

size_t operator-(Int64Iterator i, Int64Iterator j) {
  return i.payload.first - j.payload.first;
}

Int64Iterator operator+(Int64Iterator i, size_t j) {
  return Int64Iterator(i.payload.first + j);
}

template <int kTagBits, int Alpha, typename T, typename U>
size_t TestRecoverEither(T& dt, const size_t kPopulation, U universeBegin, U universeEnd) {
redo:

  size_t current_pop = 0;

  for (auto i = universeBegin; i != universeBegin + 100 * kPopulation; ++i) {
    try {
      dt.Insert(i->first, i->second);
      const double filled = 100.0 * (i - universeBegin) / dt.Capacity();
      if (filled >= 95.0) {
        // cout << "filled " << filled << '%' << endl;
        current_pop = (i - universeBegin) + 1;
        // cout << "kPopulation " << kPopulation << endl;
        // cout << "dt.data_.size() " << dt.data_.size() << endl;
        break;
      }
    } catch (...) {
      // cout << "failed " << (100.0 * i / dt.data_.size() / 4) << '%' << endl;
      // // TODO: how to undo this?
      // kPopulation = 1ul << 20;
      cout << "fill failed; retrying " << (100.0 * (i - universeBegin) / dt.Capacity())
           << endl;
      throw 'r';
      //dt.Clear();
      goto redo;
    }
  }

  auto vv = GetPositives(dt, universeBegin, universeEnd);
  vector<KeyCode<decltype(vv.first[0]->first)>> v, w;
  for (auto& xx : vv.first)
    v.push_back(KeyCode<decltype(xx->first)>{xx->first, xx->second});

  auto e = RainbowExtract<kTagBits, decltype(dt.h), decltype(v[0].key), Alpha>(dt, v);
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
  cout << vv.second * 100.0 / (universeEnd - universeBegin - current_pop) << ",";
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
    return TestRecoverEither<kTagBits, Alpha>(dt, kPopulation, Int64Iterator(0),
                                              Int64Iterator(universe));
  } else {
    DebugTable<uint64_t, kTagBits, decltype(g)> dt(
        g, 1ul << static_cast<int>(ceil(log2(max(2.0, 1.5 * kPopulation / 4)))), x, y, z);
    return TestRecoverEither<kTagBits, Alpha>(dt, kPopulation, Int64Iterator(0),
                                              Int64Iterator(universe));
  }
};

int lexiShift = 11;

template <int kTagBits, int Alpha, bool protect>
size_t TestRecover24(const size_t kPopulation, vector<pair<string, size_t>> lexicon) {
  auto x = MS64::FromDevice(), y = MS64::FromDevice();
  auto z = MS128::FromDevice();
  auto h = [](uint32_t x) { return (0x87ad153f * x) >> lexiShift; };
  auto g = [](uint32_t x) { return x; };
  if (protect) {
    DebugTable<string, kTagBits, decltype(h)> dt(
        h, 1ul << static_cast<int>(ceil(log2(max(2.0, 1.5 * kPopulation / 4)))), x, y, z);
    return TestRecoverEither<kTagBits, Alpha>(dt, kPopulation, lexicon.begin(),
                                              lexicon.end());
  } else {
    DebugTable<string, kTagBits, decltype(g)> dt(
        g, 1ul << static_cast<int>(ceil(log2(max(2.0, 1.5 * kPopulation / 4)))), x, y, z);
    return TestRecoverEither<kTagBits, Alpha>(dt, kPopulation, lexicon.begin(),
                                              lexicon.end());
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
    return TestRecoverEither<kTagBits, Alpha>(dt, kPopulation, Int64Iterator(0),
                                              Int64Iterator(universe));
  } else {
    FourOne<uint64_t, kTagBits, Alpha, decltype(g)> dt(
        g, 1ul << static_cast<int>(ceil(log2(max(2.0, 1.5 * kPopulation / 4)))), xy, z);
    return TestRecoverEither<kTagBits, Alpha>(dt, kPopulation, Int64Iterator(0),
                                              Int64Iterator(universe));
  }
};

template <int kTagBits, int Alpha, bool protect>
size_t TestRecover41(const size_t kPopulation,  vector<pair<string, size_t>> lexicon) {
  MS64 xy[4] = {MS64::FromDevice(), MS64::FromDevice(), MS64::FromDevice(),
                MS64::FromDevice()};
  auto z = MS128::FromDevice();
  auto h = [](uint32_t x) { return (0x87ad153f * x) >> lexiShift; };
  auto g = [](uint32_t x) { return x; };
  if (protect) {
    FourOne<string, kTagBits, Alpha, decltype(h)> dt(
        h, 1ul << static_cast<int>(ceil(log2(max(2.0, 1.5 * kPopulation / 4)))), xy, z);
    return TestRecoverEither<kTagBits, Alpha>(dt, kPopulation, lexicon.begin(),
                                              lexicon.end());
  } else {
    FourOne<string, kTagBits, Alpha, decltype(g)> dt(
        g, 1ul << static_cast<int>(ceil(log2(max(2.0, 1.5 * kPopulation / 4)))), xy, z);
    return TestRecoverEither<kTagBits, Alpha>(dt, kPopulation, lexicon.begin(),
                                              lexicon.end());
  }
};

// read strings from stdin, put them in a Rainbow, then print all keys that can be
// recovered
int main(int argc, char ** argv) {
  constexpr bool protect = true;
  constexpr size_t universe = 1ul << 32;
  istringstream s(argv[1]);
  int f_b = 0;
  s >> f_b;

  vector<pair<string, size_t>> lexicon;
  LazyTabHash lth;
  if (argc > 2) {
    if (argc != 4) throw []() { string("lexi argc"); };
    if (string(argv[2]) != "dict") throw string("dict");
    istringstream s(argv[3]);
    s >> lexiShift;
    string word;
    while (cin >> word) {
      lexicon.push_back({word, lth(word.c_str(), word.size())});
    }
  }
  const unsigned long count = 1ul << 14;
  cout << "recovered24,positives24,fpp24,recovered411,positives411,fpp411,recovered412,"
          "positives412,fpp412,recovered413,positives413,fpp412,fill_count"
       << endl;
#define effbee(F_B, U)                                \
  case F_B:                                           \
    TestRecover24<F_B, 0, protect>(count, U);         \
    TestRecover41<F_B, 1, protect>(count, U);         \
    TestRecover41<F_B, 2, protect>(count, U);         \
    cout << TestRecover41<F_B, 3, protect>(count, U); \
    cout << endl;                                     \
    return 0;

#define all_effbee(U) \
  switch (f_b) {      \
    effbee(4, U);     \
    effbee(5, U);     \
    effbee(6, U);     \
    effbee(7, U);     \
    effbee(8, U);     \
    effbee(9, U);     \
    effbee(10, U);    \
    effbee(11, U);    \
    effbee(12, U);    \
    effbee(13, U);    \
    effbee(14, U);    \
    effbee(15, U);    \
    effbee(16, U);    \
    default:          \
      throw 0;        \
  }

  if (not lexicon.empty()) {
    all_effbee(lexicon);
  } else {
    all_effbee(universe);
  }
}
