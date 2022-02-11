#pragma once

#include <array>
#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <sstream>

#include "debug-table.hpp"
#include "rainbow.hpp"

using namespace std;

template<typename T, int W, int Alpha>
struct RainbowFourOne {
  unordered_map<unsigned __int128, unordered_map<uint64_t, uint64_t>> prints_to_code_to_index_;
  const vector<KeyCode<T>> keys_;
  uint64_t mask_;
  const MS64 hash_makers_[4];

  template <typename U>
  RainbowFourOne(uint64_t capacity, const vector<KeyCode<T>>& keys,
                 const MS64 hash_makers[4], U fingerprinter)
      : keys_(keys),
        mask_(capacity / 4 - 1),
        hash_makers_{hash_makers[0], hash_makers[1], hash_makers[2], hash_makers[3]} {
    for (const auto& kc : keys) {
      prints_to_code_to_index_[fingerprinter(kc.code) &
                               ((static_cast<unsigned __int128>(1) << (W << Alpha)) - 1)]
                              [kc.code] = &kc - &keys[0];
    }
  }

  unordered_set<uint64_t> Extract() const {
    unordered_set<uint64_t> result_index;
    for (const auto& s_print_code_index : prints_to_code_to_index_) {
      unordered_map<uint64_t, uint64_t> after_code_index = s_print_code_index.second;
      array<unordered_map<uint64_t, unordered_set<uint64_t>>, 4> dupe_hashes_code;
      for (const auto& code_index : s_print_code_index.second) {
        for(int i = 0; i < 4; ++i) {
          dupe_hashes_code[i][hash_makers_[i](code_index.first) & mask_].insert(
              code_index.first);
        }
      }
      for (const auto & x_hash_code : dupe_hashes_code) {
        for (const auto& y_hash_code : x_hash_code) {
          if (y_hash_code.second.size() > 1) {
            // cout << y_hash_code.second.size() << endl;
            for (const auto z_code : y_hash_code.second) {
              after_code_index.erase(z_code);
            }
          }
        }
      }
      //cout << after.size() << endl;
      for (const auto x_code_index : after_code_index) {
        result_index.insert(x_code_index.second);
      }
    }
    return result_index;
  }
};

string ShowHex128(unsigned __int128 x) {
  ostringstream os;
  os << hex << static_cast<uint64_t>(x >> 64) << ", " << static_cast<uint64_t>(x) << dec;
  return os.str();
}

template <int W, typename H, int Alpha>
unordered_set<uint64_t> RainbowExtract(FourOne<uint64_t, W, Alpha, H>& dt,
                                       vector<KeyCode<uint64_t>>& keys) {
  unordered_map<unsigned __int128, unordered_map<uint64_t, uint64_t>> prints_to_code_to_index_;
  uint64_t mask_ = dt.Capacity() / 4 - 1;
  const MS64 hash_makers_[4] = {dt.hash_makers_[0], dt.hash_makers_[1],
                                dt.hash_makers_[2], dt.hash_makers_[3]};
  auto fingerprinter = dt.print_maker_;
  static_assert((W << Alpha) < 129, "");
  unsigned __int128 print_mask = (W << Alpha) % 128;
  //cout << (int)print_mask << endl;
  print_mask = (static_cast<unsigned __int128>(1)) << print_mask;
  print_mask = print_mask - 1;
  print_mask = print_mask ? print_mask : -static_cast<unsigned __int128>(1);
  //cout << ShowHex128(print_mask) << endl;
  // for (size_t i = 0; i < keys.size(); ++i) keys[i].code = dt.h(keys[i].code);
  for (const auto& kc : keys) {
    prints_to_code_to_index_[fingerprinter(dt.h(kc.code)) & print_mask][kc.code] =
        &kc - &keys[0];
  }
  //cout << prints_to_code_to_index_.size() << endl;


  unordered_set<uint64_t> result_index;
  for (const auto& s_print_code_index : prints_to_code_to_index_) {
    if (s_print_code_index.second.size() > 1) {
      //cout << "fat " << s_print_code_index.second.size() << endl;
    }
    unordered_map<uint64_t, uint64_t> after_code_index = s_print_code_index.second;
    array<unordered_map<uint64_t, unordered_set<uint64_t>>, 4> dupe_hashes_code;
    for (const auto& code_index : s_print_code_index.second) {
      for (int i = 0; i < 4; ++i) {
        dupe_hashes_code[i][hash_makers_[i](dt.h(code_index.first)) & mask_].insert(
            code_index.first);
      }
    }
    for (const auto& x_hash_code : dupe_hashes_code) {
      for (const auto& y_hash_code : x_hash_code) {
        if (y_hash_code.second.size() > 1) {
          //cout << "wide " << y_hash_code.second.size() << endl;
          for (const auto z_code : y_hash_code.second) {
            after_code_index.erase(z_code);
          }
        }
      }
    }
    // cout << after.size() << endl;
    for (const auto x_code_index : after_code_index) {
      result_index.insert(x_code_index.second);
    }
  }
  return result_index;
}
