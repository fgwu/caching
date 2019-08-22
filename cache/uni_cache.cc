#include "rocksdb/uni_cache.h"
#include "rocksdb/cache.h"

#include "cache/lru_cache.h"

namespace rocksdb {

UniCacheFix::UniCacheFix(
			 size_t capacity, double kp_cache_ratio, int num_shard_bits, bool strict_capacity_limit,
			 std::shared_ptr<MemoryAllocator> /*memory_allocator*/) {
  if (kp_cache_ratio > 1 || kp_cache_ratio < 0) {
    kp_cache_ratio_ = 0;
  } else {
    kp_cache_ratio_ = kp_cache_ratio;
  }
  size_t kp_cache_capacity = capacity * kp_cache_ratio_;
  size_t kv_cache_capacity = capacity - kp_cache_capacity;
  kv_cache_ =
      NewLRUCache(kv_cache_capacity, num_shard_bits, strict_capacity_limit);
  kp_cache_ =
      NewLRUCache(kp_cache_capacity, num_shard_bits, strict_capacity_limit);
}

UniCacheFix::~UniCacheFix() {}

Status UniCacheFix::Insert(UniCacheEntryType type, const Slice &key,
                           void *value, size_t charge,
                           void (*deleter)(const Slice &key, void *value),
                           Cache::Handle **handle, Cache::Priority priority) {
  switch (type) {
  case kKV:
    return kv_cache_->Insert(key, value, charge, deleter, handle, priority);
  case kKP:
    return kp_cache_->Insert(key, value, charge, deleter, handle, priority);
  default:
    assert(0);
  }
}

Cache::Handle *UniCacheFix::Lookup(UniCacheEntryType type, const Slice &key,
                                   Statistics *stats) {
  switch (type) {
  case kKV:
    return kv_cache_->Lookup(key, stats);
  case kKP:
    return kp_cache_->Lookup(key, stats);
  default:
    assert(0);
  }
}

bool UniCacheFix::Ref(UniCacheEntryType type, Cache::Handle *handle) {
  switch (type) {
  case kKV:
    return kv_cache_->Ref(handle);
  case kKP:
    return kp_cache_->Ref(handle);
  default:
    assert(0);
  }
}

bool UniCacheFix::Release(UniCacheEntryType type, Cache::Handle *handle,
                          bool force_erase) {
  switch (type) {
  case kKV:
    return kv_cache_->Release(handle, force_erase);
  case kKP:
    return kp_cache_->Release(handle, force_erase);
  default:
    assert(0);
  }
}

void *UniCacheFix::Value(UniCacheEntryType type, Cache::Handle *handle) {
  switch (type) {
  case kKV:
    return kv_cache_->Value(handle);
  case kKP:
    return kp_cache_->Value(handle);
  default:
    assert(0);
  }
}

void UniCacheFix::Erase(UniCacheEntryType type, const Slice &key) {
  switch (type) {
  case kKV:
    return kv_cache_->Erase(key);
  case kKP:
    return kp_cache_->Erase(key);
  default:
    assert(0);
  }
}

void UniCacheFix::SetCapacity(size_t capacity) {
  size_t kp_cache_capacity = capacity * kp_cache_ratio_;
  size_t kv_cache_capacity = capacity - kp_cache_capacity;

  kv_cache_->SetCapacity(kv_cache_capacity);
  kp_cache_->SetCapacity(kp_cache_capacity);
}

void UniCacheFix::SetCapacity(UniCacheEntryType type, size_t capacity) {
  size_t old_capacity = GetCapacity();
  size_t kv_cache_capacity;
  size_t kp_cache_capacity;

  switch (type) {
  case kKV:
    kv_cache_capacity = capacity;
    kp_cache_capacity = old_capacity - kv_cache_capacity;
    break;
  case kKP:
    kp_cache_capacity = capacity;
    kv_cache_capacity = old_capacity - kp_cache_capacity;
    break;
  default:
    assert(0);
  }

  kv_cache_->SetCapacity(kv_cache_capacity);
  kp_cache_->SetCapacity(kp_cache_capacity);
}

void UniCacheFix::SetStrictCapacityLimit(bool strict_capacity_limit) {
  kv_cache_->SetStrictCapacityLimit(strict_capacity_limit);
  kp_cache_->SetStrictCapacityLimit(strict_capacity_limit);
}

bool UniCacheFix::HasStrictCapacityLimit() const {
  assert(kv_cache_->HasStrictCapacityLimit() ==
         kp_cache_->HasStrictCapacityLimit());
  return kv_cache_->HasStrictCapacityLimit();
}

size_t UniCacheFix::GetCapacity() const {
  return kv_cache_->GetCapacity() + kp_cache_->GetCapacity();
}

size_t UniCacheFix::GetCapacity(UniCacheEntryType type) const {
  switch (type) {
  case kKV:
    return kv_cache_->GetCapacity();
  case kKP:
    return kp_cache_->GetCapacity();
  default:
    assert(0);
  }
}

size_t UniCacheFix::GetUsage() const {
  return kv_cache_->GetUsage() + kp_cache_->GetUsage();
}

size_t UniCacheFix::GetUsage(UniCacheEntryType type) const {
  switch (type) {
  case kKV:
    return kv_cache_->GetUsage();
  case kKP:
    return kp_cache_->GetUsage();
  default:
    assert(0);
  }
}

size_t UniCacheFix::GetUsage(Cache::Handle *handle) const {
  return kv_cache_->GetUsage(handle) + kp_cache_->GetUsage(handle);
}

size_t UniCacheFix::GetUsage(UniCacheEntryType type,
                             Cache::Handle *handle) const {
  switch (type) {
  case kKV:
    return kv_cache_->GetUsage(handle);
  case kKP:
    return kp_cache_->GetUsage(handle);
  default:
    assert(0);
  }
}

size_t UniCacheFix::GetPinnedUsage() const {
  return kv_cache_->GetPinnedUsage() + kp_cache_->GetPinnedUsage();
}

void UniCacheFix::DisownData() {
  kv_cache_->DisownData();
  kp_cache_->DisownData();
}

void UniCacheFix::ApplyToAllCacheEntries(void (*callback)(void *, size_t),
                                         bool thread_safe) {
  kv_cache_->ApplyToAllCacheEntries(callback, thread_safe);
  kp_cache_->ApplyToAllCacheEntries(callback, thread_safe);
}

void UniCacheFix::EraseUnRefEntries() {
  kv_cache_->EraseUnRefEntries();
  kp_cache_->EraseUnRefEntries();
}

std::shared_ptr<UniCache>
NewUniCacheFix(size_t capacity, double kp_cache_ratio, int num_shard_bits, bool strict_capacity_limit,
               double /*high_pri_pool_ratio*/,
               std::shared_ptr<MemoryAllocator> memory_allocator,
               bool /*use_adaptive_mutex*/) {
  if (num_shard_bits >= 20) {
    return nullptr; // the cache cannot be sharded into too many fine pieces
  }

  if (num_shard_bits < 0) {
    num_shard_bits = GetDefaultCacheShardBits(capacity);
  }
  return std::make_shared<UniCacheFix>(capacity, kp_cache_ratio,  num_shard_bits,
                                       strict_capacity_limit,
                                       std::move(memory_allocator));
}

} // namespace rocksdb
