#include "rocksdb/cache.h"

#include "cache/lru_cache.h"
#include "cache/uni_cache.h"

namespace rocksdb {

const double kKVCacheRatio = 1;

UniCache::UniCache(size_t capacity, int num_shard_bits,
                   bool strict_capacity_limit,
                   std::shared_ptr<MemoryAllocator> /*memory_allocator*/) {
  size_t kv_cache_capacity = capacity * kKVCacheRatio;
  size_t kp_cache_capacity = capacity - kv_cache_capacity;
  kv_cache_ =
      NewLRUCache(kv_cache_capacity, num_shard_bits, strict_capacity_limit);
  kp_cache_ =
      NewLRUCache(kp_cache_capacity, num_shard_bits, strict_capacity_limit);
}

UniCache::~UniCache() {}

Status UniCache::Insert(const Slice &key, void *value, size_t charge,
                        void (*deleter)(const Slice &key, void *value),
                        Cache::Handle **handle, Priority priority) {
  return kv_cache_->Insert(key, value, charge, deleter, handle, priority);
}

Status UniCache::Insert(UniCacheEntryType type, const Slice &key, void *value,
                        size_t charge,
                        void (*deleter)(const Slice &key, void *value),
                        Cache::Handle **handle, Priority priority) {
  switch (type) {
  case kKV:
    return kv_cache_->Insert(key, value, charge, deleter, handle, priority);
  case kKP:
    return kp_cache_->Insert(key, value, charge, deleter, handle, priority);
  default:
    assert(0);
  }
}

Cache::Handle *UniCache::Lookup(const Slice &key, Statistics *stats) {
  return kv_cache_->Lookup(key, stats);
}

Cache::Handle *UniCache::Lookup(UniCacheEntryType type, const Slice &key,
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

bool UniCache::Ref(Cache::Handle *handle) { return kv_cache_->Ref(handle); }

bool UniCache::Ref(UniCacheEntryType type, Cache::Handle *handle) {
  switch (type) {
  case kKV:
    return kv_cache_->Ref(handle);
  case kKP:
    return kp_cache_->Ref(handle);
  default:
    assert(0);
  }
}

bool UniCache::Release(Cache::Handle *handle, bool force_erase) {
  return kv_cache_->Release(handle, force_erase);
}

bool UniCache::Release(UniCacheEntryType type, Cache::Handle *handle,
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

void *UniCache::Value(Cache::Handle *handle) {
  return kv_cache_->Value(handle);
}

void *UniCache::Value(UniCacheEntryType type, Cache::Handle *handle) {
  switch (type) {
  case kKV:
    return kv_cache_->Value(handle);
  case kKP:
    return kp_cache_->Value(handle);
  default:
    assert(0);
  }
}

void UniCache::Erase(const Slice &key) { return kv_cache_->Erase(key); }

void UniCache::Erase(UniCacheEntryType type, const Slice &key) {
  switch (type) {
  case kKV:
    return kv_cache_->Erase(key);
  case kKP:
    return kp_cache_->Erase(key);
  default:
    assert(0);
  }
}

uint64_t UniCache::NewId() { return kv_cache_->NewId(); }

void UniCache::SetCapacity(size_t capacity) {
  size_t kv_cache_capacity = capacity * kKVCacheRatio;
  size_t kp_cache_capacity = capacity - kv_cache_capacity;

  kv_cache_->SetCapacity(kv_cache_capacity);
  kp_cache_->SetCapacity(kp_cache_capacity);
}

void UniCache::SetStrictCapacityLimit(bool strict_capacity_limit) {
  kv_cache_->SetStrictCapacityLimit(strict_capacity_limit);
  kp_cache_->SetStrictCapacityLimit(strict_capacity_limit);
}

bool UniCache::HasStrictCapacityLimit() const {
  assert(kv_cache_->HasStrictCapacityLimit() ==
         kp_cache_->HasStrictCapacityLimit());
  return kv_cache_->HasStrictCapacityLimit();
}

size_t UniCache::GetCapacity() const {
  return kv_cache_->GetCapacity() + kp_cache_->GetCapacity();
}

size_t UniCache::GetUsage() const {
  return kv_cache_->GetUsage() + kp_cache_->GetUsage();
}

size_t UniCache::GetUsage(Cache::Handle *handle) const {
  return kv_cache_->GetUsage(handle) + kp_cache_->GetUsage(handle);
}

size_t UniCache::GetPinnedUsage() const {
  return kv_cache_->GetPinnedUsage() + kp_cache_->GetPinnedUsage();
}

void UniCache::DisownData() {
  kv_cache_->DisownData();
  kp_cache_->DisownData();
}

void UniCache::ApplyToAllCacheEntries(void (*callback)(void *, size_t),
                                      bool thread_safe) {
  kv_cache_->ApplyToAllCacheEntries(callback, thread_safe);
  kp_cache_->ApplyToAllCacheEntries(callback, thread_safe);
}

void UniCache::EraseUnRefEntries() {
  kv_cache_->EraseUnRefEntries();
  kp_cache_->EraseUnRefEntries();
}

std::shared_ptr<Cache>
NewUniCache(size_t capacity, int num_shard_bits, bool strict_capacity_limit,
            double /*high_pri_pool_ratio*/,
            std::shared_ptr<MemoryAllocator> memory_allocator,
            bool /*use_adaptive_mutex*/) {
  if (num_shard_bits >= 20) {
    return nullptr; // the cache cannot be sharded into too many fine pieces
  }

  if (num_shard_bits < 0) {
    num_shard_bits = GetDefaultCacheShardBits(capacity);
  }
  return std::make_shared<UniCache>(capacity, num_shard_bits,
                                    strict_capacity_limit,
                                    std::move(memory_allocator));
}

} // namespace rocksdb
