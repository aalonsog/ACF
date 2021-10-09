// Performs key inference on an adaptive cuckoo filter

#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

// Three strong typedefs to avoid accidentally confusing one integer for another

struct Fingerprints {
  uint64_t fingerprints;
};

bool operator==(const Fingerprints& x, const Fingerprints& y) {
  return x.fingerprints == y.fingerprints;
}

template <>
struct hash<Fingerprints> {
  size_t operator()(Fingerprints x) const {
    hash<uint64_t> h;
    return h(x.fingerprints);
  }
};

struct Index {
  uint64_t index;
};

bool operator==(const Index& x, const Index& y) {
  return x.index == y.index;
}

template <>
struct hash<Index> {
  size_t operator()(Index x) const {
    hash<uint64_t> h;
    return h(x.index);
  }
};

struct KeyIndex {
  uint64_t key_index;
};

bool operator==(const KeyIndex& x, const KeyIndex& y) {
  return x.key_index == y.key_index;
}

template <>
struct hash<KeyIndex> {
  size_t operator()(KeyIndex x) const {
    hash<uint64_t> h;
    return h(x.key_index);
  }
};

// Rainbow is a table structure to organize input keys by theirfingerprint values, then
// their index values (location in the ACF).
//
// To use, the caller needs to know the identities of the keys, the hash function
// producing fingerprints (here as four 16-bit values in on uint64_t), and the two hash
// functions producing indexes into the ACF.
template<typename T>
struct Rainbow {
  unordered_map<Fingerprints, unordered_map<Index, unordered_set<KeyIndex>>> table;
  const vector<T>& keys;
  function<size_t(const T&)> indexers[2];

  Rainbow(const vector<T>& keys, function<size_t(const T&)> indexers[2],
          function<uint64_t(const T&)> fingerprinter)
      : keys(keys), indexers{indexers[0], indexers[1]} {
    size_t n = keys.size();
    for(uint64_t i = 0; i < n; ++i) {
      for (int j : {0, 1}) {
        table[Fingerprints{fingerprinter(keys[i])}][Index{indexers[j](keys[i])}].insert(
            KeyIndex{i});
      }
    }
  }

  unordered_set<KeyIndex> Extract() {
    unordered_set<KeyIndex> result;
    for (auto& bucket : table) {
      for (auto& index_map : bucket.second) {
        if (index_map.second.size() > 1) continue;
        auto ki = *index_map.second.begin();
        bool unique = true;
        for (int j : {0, 1}) {
          auto alternate_key = indexers[j](keys[ki.key_index]);
          if (bucket.second.find(Index{alternate_key})->second.size() > 1) {
            unique = false;
            break;
          }
        }
        if (unique) result.insert(ki);
      }
    }
    return result;
  }
};

// read strings from stdin, put them in a Rainbow, then print all keys that can be
// recovered
int main() {
  vector<string> v;
  string s;
  while (cin >> s) v.push_back(s);
  auto mf = [](const string& x) { return hash<string>()(x); };
  auto mi1 = [](const string& x) { return hash<string>()(x + "1"); };
  auto mi2 = [](const string& x) { return hash<string>()(x + "2"); };
  function<size_t(const string&)> indexers[2] = {mi1, mi2};
  Rainbow<string> r(v, indexers, mf);
  auto e = r.Extract();
  for (auto& w : e) {
    cout << v[w.key_index] << endl;
  }
}
