#include <cassert>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <unordered_set>

#include "four-one.hpp"
#include "linear-hashing.hpp"

using namespace std;

struct Line {
  uint8_t ip_from_[4];
  uint8_t ip_to_[4];
  uint16_t port_from_;
  uint16_t port_to_;
  uint16_t protocol_;

  Line() {}
  Line(FILE *stream) {
    const int scanned =
        fscanf(stream,
               "%" SCNu8 ".%" SCNu8 ".%" SCNu8 ".%" SCNu8 " %" SCNu8 ".%" SCNu8 ".%" SCNu8
               ".%" SCNu8 " %" SCNu16 " %" SCNu16 " %" SCNu16 "\n",
               &ip_from_[0], &ip_from_[1], &ip_from_[2], &ip_from_[3], &ip_to_[0],
               &ip_to_[1], &ip_to_[2], &ip_to_[3], &port_from_, &port_to_, &protocol_);
    assert(11 == scanned);
  }

  bool operator==(const Line& that) const {
    return ip_from_[0] == that.ip_from_[0] && ip_from_[1] == that.ip_from_[1] &&
           ip_from_[2] == that.ip_from_[2] && ip_from_[3] == that.ip_from_[3] &&
           ip_to_[0] == that.ip_to_[0] && ip_to_[1] == that.ip_to_[1] &&
           ip_to_[2] == that.ip_to_[2] && ip_to_[3] == that.ip_to_[3] &&
           port_from_ == that.port_from_ && port_to_ == that.port_to_ &&
           protocol_ == that.protocol_;
  }
};

istream& operator>>(istream& in, Line& l) {
  char separator;
  in >> l.ip_from_[0] >> separator >> l.ip_from_[1] >> separator >> l.ip_from_[2] >>
      separator >> l.ip_from_[3];
  in >> separator;
  in >> l.ip_to_[0] >> separator >> l.ip_to_[1] >> separator >> l.ip_to_[2] >>
      separator >> l.ip_to_[3];
  in >> separator;
  in >> l.port_from_;
  in >> separator;
  in >> l.port_to_;
  in >> separator;
  in >> l.protocol_;
  return in;
}

ostream& operator<<(ostream& out, Line& l) {
  out << static_cast<int>(l.ip_from_[0]) << '.' << static_cast<int>(l.ip_from_[1]) << '.'
      << static_cast<int>(l.ip_from_[2]) << '.' << static_cast<int>(l.ip_from_[3]);
  out << ' ';
  out << static_cast<int>(l.ip_to_[0]) << '.' << static_cast<int>(l.ip_to_[1]) << '.'
      << static_cast<int>(l.ip_to_[2]) << '.' << static_cast<int>(l.ip_to_[3]);
  out << ' ';
  out << l.port_from_;
  out << ' ';
  out << l.port_to_;
  out << ' ';
  out << l.protocol_;
  return out;
}

struct LineHasher {
  uint32_t NH_[4];
  unsigned __int128 ms_;

  uint64_t operator()(const Line& l) const {
    uint32_t a, b;
    memcpy(&a, l.ip_from_, 4);
    memcpy(&b, l.ip_to_, 4);
    uint64_t accum = static_cast<uint64_t>(a + NH_[0]) * (b + NH_[1]);
    a = (static_cast<uint32_t>(l.port_from_) << 16) | l.port_to_;
    b = l.protocol_;
    accum += static_cast<uint64_t>(a + NH_[2]) * (b + NH_[3]);
    return (ms_ * accum) >> 64;
  }

  static LineHasher FromDevice() {
    LineHasher result;
    random_device d;
    for (int i = 0; i < 4; ++i) result.NH_[i] = d();
    result.ms_ = d();
    result.ms_ <<= 32;
    result.ms_ |= d();
    result.ms_ <<= 32;
    result.ms_ |= d();
    result.ms_ <<= 32;
    result.ms_ |= d();
    return result;
  }
};

int main(int argc, char** argv) {
  assert(2 == argc);
  size_t b = 1024;

  for (int i = 0; i < 100; ++i) {
    FILE* file = fopen(argv[1], "r");
    MS64 ms64[4] = {MS64::FromDevice(), MS64::FromDevice(), MS64::FromDevice(),
                    MS64::FromDevice()};

    auto id = [](uint64_t x) { return x; };
    FourOne<Line, 9, 1, decltype(id)> acf09(id, b, ms64, MS128::FromDevice());
    FourOne<Line, 10, 1, decltype(id)> acf10(id, b, ms64, MS128::FromDevice());
    FourOne<Line, 11, 1, decltype(id)> acf11(id, b, ms64, MS128::FromDevice());
    FourOne<Line, 12, 1, decltype(id)> acf12(id, b, ms64, MS128::FromDevice());
    FourOne<Line, 13, 1, decltype(id)> acf13(id, b, ms64, MS128::FromDevice());
    LineHasher lh = LineHasher::FromDevice();

    while (not feof(file)) {
      Line l{file};
      auto h = lh(l);
#define INSERT(ACF)                                    \
  {                                                    \
    const auto found = ACF.AdaptiveFind(l, h);         \
    if (found != decltype(ACF)::TruePositive) {        \
      ACF.Insert(l, h);                                \
      if (ACF.Count() >= ACF.Capacity() * 0.95) break; \
    }                                                  \
  }
      INSERT(acf09);
      INSERT(acf10);
      INSERT(acf11);
      INSERT(acf12);
      INSERT(acf13);
    }
    rewind(file);
    size_t lines_seen = 0;
    // HashSet<Line, LineHasher> exact(lh, 1024);
    unordered_set<Line, LineHasher> exact2(1024, lh);
    while (not feof(file)) {
      Line l{file};
      const auto h = lh(l);
      acf09.AdaptiveFind(l, h);
      acf10.AdaptiveFind(l, h);
      acf11.AdaptiveFind(l, h);
      acf12.AdaptiveFind(l, h);
      acf13.AdaptiveFind(l, h);
      ++lines_seen;
      // exact.Insert(l);
      if (0 == i) exact2.insert(l);
      if ((lines_seen & (lines_seen - 1)) == 0) {
        // cout << b * (1 << f) * log(1.0 / sqrt(1.0 - 2.0 * acf.AdaptedCount() /
        // acf.Count()))
        //      << endl;
      }
    }
    cout << acf09.AdaptedCount() << ' ';
    cout << acf10.AdaptedCount() << ' ';
    cout << acf11.AdaptedCount() << ' ';
    cout << acf12.AdaptedCount() << ' ';
    cout << acf13.AdaptedCount() << ' ';
    // cout << exact.Size();
    if (0 == i) cout << exact2.size() << ' ' << lines_seen;
    cout << endl;
  }
}
