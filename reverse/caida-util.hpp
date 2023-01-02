#include <cassert>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <random>
#include <unordered_set>

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

  bool operator!=(const Line & that) const { return not (*this == that); }
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
