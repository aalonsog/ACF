#pragma once

// Performs key inference on an adaptive cuckoo filter

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "debug-table.hpp"

using namespace std;

template <typename T>
struct KeyCode {
  T key = T();
  uint64_t code = -1ul;
};

// Rainbow is a table structure to organize input keys by their fingerprint values, then
// their index values (location in the ACF).
//
// To use, the caller needs to know the identities of the keys, the hash function
// producing fingerprints (here as four 16-bit values in on uint64_t), and the two hash
// functions producing indexes into the ACF.
template<typename T, int W>
struct Rainbow {
  static_assert(W <= 32, "W <= 32");
  uint64_t mask;
  // fingerprint -> index_into_filter -> index_into_key_vector
  // TODO: should last one be a vector?
  // TODO: key should be unsigned __int128?
  unordered_map<uint64_t, unordered_map<uint64_t, unordered_set<uint64_t>>> table;
  // index_into_filter -> (index_into_key_vector, fingerprint)
  unordered_map<uint64_t, unordered_map<uint64_t, uint64_t>> filter_all_options;
  const vector<KeyCode<T>>& keys;
  MS64 hash_maker;
  MS64 xor_maker;

  template <typename U>
  Rainbow(uint64_t capacity, const vector<KeyCode<T>>& keys, MS64 hash_maker,
          MS64 xor_maker, U fingerprinter)
      : mask(capacity / 4 - 1), keys(keys), hash_maker(hash_maker), xor_maker(xor_maker) {
    static_assert(W <= 16, "W <= 16");
    constexpr uint64_t kFingerprintMask = (W == 16) ? -1ul : ((1ul << ((4 * W) % 64)) - 1);
    if (mask & (mask + 1)) throw mask;
    size_t n = keys.size();
    for (uint64_t i = 0; i < n; ++i) {
      const auto fingerprints = fingerprinter(keys[i].code) & kFingerprintMask;
      for (uint64_t index :
           {hash_maker(keys[i].code) & mask,
            (hash_maker(keys[i].code) ^ xor_maker(keys[i].code)) & mask}) {
        table[fingerprints][index].insert(i);
        filter_all_options[index][i] = fingerprints;
      }
    }
  }

  unordered_set<uint64_t> Extract() {
    unordered_set<uint64_t> blocklist;
    unordered_map<uint64_t, uint64_t> blocklist_support;
    for (auto& bucket : filter_all_options) {
      if (bucket.second.size() > 4) {
        for (int i = 0; i < 4; ++i) {
          // fingerprint -> index_into_key_vector
          unordered_map<uint64_t, unordered_set<uint64_t>> collisions;
          for (auto& x : bucket.second) {
            collisions[((1 << W) - 1) & ((x.second >> (W * i)))].insert(x.first);
          }
          for (auto& s : collisions) {
            if (s.second.size() > 4) {
              blocklist_support[bucket.first] = bucket.second.size();
              blocklist.insert(s.second.begin(), s.second.end());
              // for (auto& t : s.second) cerr << "blocklist " << t << endl;
            }
          }
        }
      }
    }
    unordered_set<uint64_t> result;
    for (auto& bucket : table) {
      for (auto& index_map : bucket.second) {
        if (index_map.second.size() > 1) {
          // for (auto& x : index_map.second) {
          //   // cerr << "unrecoverable A " << x << endl;
          // }
          continue;
        }
        auto ki = *index_map.second.begin();
        bool unique = true;
        for (uint64_t index :
             {hash_maker(keys[ki].code) & mask,
              (hash_maker(keys[ki].code) ^ xor_maker(keys[ki].code)) & mask}) {
          if (bucket.second.find(index)->second.size() > 1) {
            // for (auto& x : bucket.second.find(index)->second) {
            //   // cerr << "unrecoverable B " << x << endl;
            // }
            unique = false;
            break;
          }
        }
        if (unique) result.insert(ki);
      }
    }
    // cout << "extracted size " << result.size() << "\t";
    size_t erased = 0;
    for (auto& x : blocklist) erased += result.erase(x);
    // cout << "blocklist.size() " << blocklist.size() << "\t";
    // cout << "erased " << erased << "\t";
    // cout << "blocklist support " << blocklist_support.size() << "\t";
    // size_t heaviest_bucket = 0;
    // for (auto & i : blocklist_support) {
    //   heaviest_bucket = max(heaviest_bucket, i.second);
    // }
    // cout << "heaviest support " << heaviest_bucket << "\t";
    return result;
  }
};

template <int W, typename H, typename U, int Alpha = 0>
unordered_set<uint64_t> RainbowExtract(const DebugTable<U, W, H>& dt,
                                       vector<KeyCode<U>>& keys) {
  static_assert(W <= 32, "W <= 32");
  uint64_t mask = dt.Capacity() / 4 - 1;
  // fingerprint -> index_into_filter -> index_into_key_vector
  // TODO: should last one be a vector?
  // TODO: key should be unsigned __int128?
  unordered_map<uint64_t, unordered_map<uint64_t, unordered_set<uint64_t>>> table;
  // index_into_filter -> (index_into_key_vector, fingerprint)
  unordered_map<uint64_t, unordered_map<uint64_t, uint64_t>> filter_all_options;

  MS64 hash_maker = dt.hash_maker_;
  MS64 xor_maker = dt.xor_maker_;
  auto fingerprinter = dt.print_makers_;

  static_assert(W <= 16, "W <= 16");
  constexpr uint64_t kFingerprintMask = (W == 16) ? -1ul : ((1ul << ((4 * W) % 64)) - 1);
  if (mask & (mask + 1)) throw mask;
  size_t n = keys.size();
  for (uint64_t i = 0; i < n; ++i) keys[i].code = dt.h(keys[i].code);
  for (uint64_t i = 0; i < n; ++i) {
    const auto fingerprints = fingerprinter(keys[i].code) & kFingerprintMask;
    for (uint64_t index : {hash_maker(keys[i].code) & mask,
                           (hash_maker(keys[i].code) ^ xor_maker(keys[i].code)) & mask}) {
      table[fingerprints][index].insert(i);
      filter_all_options[index][i] = fingerprints;
    }
  }

  unordered_set<uint64_t> blocklist;
  unordered_map<uint64_t, uint64_t> blocklist_support;
  for (auto& bucket : filter_all_options) {
    if (bucket.second.size() > 4) {
      for (int i = 0; i < 4; ++i) {
        // fingerprint -> index_into_key_vector
        unordered_map<uint64_t, unordered_set<uint64_t>> collisions;
        for (auto& x : bucket.second) {
          collisions[((1 << W) - 1) & ((x.second >> (W * i)))].insert(x.first);
        }
        for (auto& s : collisions) {
          if (s.second.size() > 4) {
            blocklist_support[bucket.first] = bucket.second.size();
            blocklist.insert(s.second.begin(), s.second.end());
            // for (auto& t : s.second) cerr << "blocklist " << t << endl;
          }
        }
      }
    }
  }
  unordered_set<uint64_t> result;
  for (auto& bucket : table) {
    for (auto& index_map : bucket.second) {
      if (index_map.second.size() > 1) {
        // for (auto& x : index_map.second) {
        //   // cerr << "unrecoverable A " << x << endl;
        // }
        continue;
      }
      auto ki = *index_map.second.begin();
      bool unique = true;
      for (uint64_t index :
           {hash_maker(keys[ki].code) & mask,
            (hash_maker(keys[ki].code) ^ xor_maker(keys[ki].code)) & mask}) {
        if (bucket.second.find(index)->second.size() > 1) {
          // for (auto& x : bucket.second.find(index)->second) {
          //   // cerr << "unrecoverable B " << x << endl;
          // }
          unique = false;
          break;
        }
      }
      if (unique) result.insert(ki);
    }
  }
  // cout << "extracted size " << result.size() << "\t";
  size_t erased = 0;
  for (auto& x : blocklist) erased += result.erase(x);
  // cout << "blocklist.size() " << blocklist.size() << "\t";
  // cout << "erased " << erased << "\t";
  // cout << "blocklist support " << blocklist_support.size() << "\t";
  // size_t heaviest_bucket = 0;
  // for (auto & i : blocklist_support) {
  //   heaviest_bucket = max(heaviest_bucket, i.second);
  // }
  // cout << "heaviest support " << heaviest_bucket << "\t";
  return result;
}
