#include "rocksdb/cache.h"

#include "cache/lru_cache.h"
#include "cache/uni_cache.h"

namespace rocksdb {

UniCache::UniCache(size_t capacity, int num_shard_bits,
                   bool strict_capacity_limit,
                   std::shared_ptr<MemoryAllocator> /*memory_allocator*/) {
  cache_ = NewLRUCache(capacity, num_shard_bits, strict_capacity_limit);
}

UniCache::~UniCache() {}

Status UniCache::Insert(const Slice &key, void *value, size_t charge,
                        void (*deleter)(const Slice &key, void *value),
                        Cache::Handle **handle, Priority priority) {
  return cache_->Insert(key, value, charge, deleter, handle, priority);
}

Cache::Handle *UniCache::Lookup(const Slice &key, Statistics *stats) {
  return cache_->Lookup(key, stats);
}

bool UniCache::Ref(Cache::Handle *handle) { return cache_->Ref(handle); }

bool UniCache::Release(Cache::Handle *handle, bool force_erase) {
  return cache_->Release(handle, force_erase);
}

void *UniCache::Value(Cache::Handle *handle) { return cache_->Value(handle); }

void UniCache::Erase(const Slice &key) { return cache_->Erase(key); }

uint64_t UniCache::NewId() { return cache_->NewId(); }

void UniCache::SetCapacity(size_t capacity) {
  return cache_->SetCapacity(capacity);
}

void UniCache::SetStrictCapacityLimit(bool strict_capacity_limit) {
  return cache_->SetStrictCapacityLimit(strict_capacity_limit);
}

bool UniCache::HasStrictCapacityLimit() const {
  return cache_->HasStrictCapacityLimit();
}

size_t UniCache::GetCapacity() const { return cache_->GetCapacity(); }

size_t UniCache::GetUsage() const { return cache_->GetUsage(); }

size_t UniCache::GetUsage(Cache::Handle *handle) const {
  return cache_->GetUsage(handle);
}

size_t UniCache::GetPinnedUsage() const { return cache_->GetPinnedUsage(); }

void UniCache::DisownData() { cache_->DisownData(); }

void UniCache::ApplyToAllCacheEntries(void (*callback)(void *, size_t),
                                      bool thread_safe) {
  cache_->ApplyToAllCacheEntries(callback, thread_safe);
}

void UniCache::EraseUnRefEntries() { cache_->EraseUnRefEntries(); }

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
