#include <algorithm>   // for max
#include <cstddef>     // for size_t
#include <functional>  // for hash
#include <memory>      // for unique_ptr
#include <new>         // for operator new
#include <utility>     // for pair, swap

template <typename KeyValue, typename Hasher, typename GetKey>
class HashTable {
  enum Status : char { EMPTY, FULL, TOMBSTONE };
  struct Slot {
    KeyValue kv = KeyValue();
    Status status = EMPTY;
    Slot(const KeyValue& kv, Status status) : kv(kv), status(status) {}
    Slot() {}
  };

  std::size_t capacity_;
  std::unique_ptr<Slot[]> data_;
  Hasher hasher_;
  std::size_t population_ = 0;
  std::size_t tombstones_ = 0;

  friend void swap(HashTable& x, HashTable& y) {
    using std::swap;
    swap(x.capacity_, y.capacity_);
    swap(x.data_, y.data_);

    auto temp = x.hasher_;
    x.hasher_.~Hasher();
    new (&x.hasher_) Hasher(y.hasher_);
    y.hasher_.~Hasher();
    new (&y.hasher_) Hasher(temp);

    swap(x.population_, y.population_);
    swap(x.tombstones_, y.tombstones_);
  }

  template <typename Parent, typename InnerKeyValue>
  class IteratorBase {
    Parent* parent_;
    std::size_t offset_;

    friend class HashTable;
    IteratorBase(Parent* parent, std::size_t offset) : parent_(parent), offset_(offset) {}

   public:
    IteratorBase& operator++() {
      offset_ += 1;
      while (offset_ < parent_->capacity_ && parent_->data_[offset_].status != FULL) {
        ++offset_;
      }
      return *this;
    }

    InnerKeyValue& operator*() { return parent_->data_[offset_].kv; }

    template <typename T, typename U>
    bool operator==(IteratorBase<T, U> that) {
      return offset_ == that.offset_ && parent_ == that.parent_;
    }

    template <typename T, typename U>
    bool operator!=(IteratorBase<T, U> that) {
      return not operator==(that);
    }

    InnerKeyValue* operator->() { return &parent_->data_[offset_].kv; }

    operator IteratorBase<const HashTable, const KeyValue>() {
      return IteratorBase<const HashTable, const KeyValue>(parent_, offset_);
    }
  };

 public:
  using iterator = IteratorBase<HashTable, KeyValue>;
  using const_iterator = IteratorBase<const HashTable, const KeyValue>;

  iterator begin() {
    iterator result(this, -1ull);
    ++result;
    return result;
  }

  const_iterator begin() const {
    const_iterator result(this, -1ull);
    ++result;
    return result;
  }

  iterator end() { return iterator(this, capacity_); }
  const_iterator end() const { return const_iterator(this, capacity_); }

  std::size_t size() const { return population_; }

  explicit HashTable(std::size_t capacity = 16, const Hasher& f = Hasher{})
      : capacity_(std::max(static_cast<std::size_t>(16), capacity)),
        data_(new Slot[capacity_]()),
        hasher_(f) {}

  // Invariants:
  // tombstones_ < 1/8 capacity_
  // 1/4 capacity_ <= population_ < 7/8 capacity_
  // population_ + tombstones_ <= 7/8 capacity_

  iterator insert(const KeyValue& kv) {
    if (population_ + tombstones_ > (7.0 / 8.0) * capacity_) Resize(2 * capacity_);
    // tombstones_ = 0 and 3/8 < population_ < 7/16, so at least 1/8 Erases or 7/16
    // Inserts until next resize
    for (std::size_t i = hasher_(GetKey::Go(kv)) & (capacity_ - 1); true;
         i = (i + 1) & (capacity_ - 1)) {
      auto fullness = data_[i].status;
      if (fullness == FULL && not (GetKey::Go(kv) == GetKey::Go(data_[i].kv))) continue;
      switch (fullness) {
        case TOMBSTONE:
          --tombstones_;
          // fallthrough
        case EMPTY:
          ++population_;
          // fallthrough
        default:  // FULL
          data_[i].~Slot();
          new (&data_[i]) Slot(kv, FULL);
          return iterator{this, i};
      }
    }
  }

 private:
  template <typename Key>
  std::size_t FindIndex(const Key& key) const {
    for (std::size_t i = hasher_(key) & (capacity_ - 1); true;
         i = (i + 1) & (capacity_ - 1)) {
      auto fullness = data_[i].status;
      if (EMPTY == fullness) return capacity_;
      if (FULL == fullness && GetKey::Go(data_[i].kv) == key) return i;
    }
  }

 public:
  template <typename Key>
  iterator find(const Key& key) {
    return iterator{this, FindIndex(key)};
  }

  template <typename Key>
  const_iterator find(const Key& key) const {
    return const_iterator{this, FindIndex(key)};
  }

  template <typename Key>
  size_t erase(const Key& key) {
    // TODO: this leads to a lot of resizing on erases.
    if (tombstones_ > (1.0 / 8.0) * capacity_) {
      if (population_ <= (3.0 / 8.0) * capacity_) {
        Resize(capacity_ / 2);
        // tombstones_ = 0, 1/2 <= population_ <= 6/8, so at least 1/8 Erases or 1/8
        // Inserts until next resize
      } else if (population_ >= (6.0 / 8.0) * capacity_) {
        Resize(2 * capacity_);
        // tombstones_ = 0, 3/8 <= population_ < 7/16, so at least 1/8 Erases or 7/16
        // Inserts until next resize
      } else {
        Resize(capacity_);
        // tombstones_ = 0, 3/8 < population_ < 6/8, so at least 1/8 Erases or 1/8 Inserts
        // until next resize
      }
    } else if (population_ <= (1.0 / 4.0) * capacity_) {
      Resize(capacity_ / 2);
      // tombstones_ = 0, population_ = 1/2, so at least 1/8 Erases or 1/4 Inserts until
      // next resize
    }
    auto i = FindIndex(key);
    if (i == capacity_) return 0;
    data_[i].kv.~KeyValue();
    new (&data_[i]) Slot(KeyValue{}, TOMBSTONE);
    --population_;
    ++tombstones_;
    return 1;
  }

 private:
  void Resize(std::size_t new_capacity) {
    auto result = HashTable(new_capacity, hasher_);
    for (std::size_t i = 0; i < capacity_; ++i) {
      if (FULL == data_[i].status) {
        result.insert(data_[i].kv);
      }
    }
    swap(*this, result);
  }

 public:
  struct KeyFromPair {
    static auto Go(const KeyValue& kv) -> const decltype(kv.first)& { return kv.first; }
  };

  struct KeyFromKey {
    static const KeyValue& Go(const KeyValue& kv) { return kv; }
  };
};

template <typename Key, typename Value, typename Hasher = std::hash<Key>>
using HashMap =
    HashTable<std::pair<const Key, Value>, Hasher,
              typename HashTable<std::pair<const Key, Value>, Hasher, void>::KeyFromPair>;

template <typename Key, typename Hasher = std::hash<Key>>
using HashSet =
    HashTable<const Key, Hasher, typename HashTable<const Key, Hasher, void>::KeyFromKey>;
