#pragma once

#include "rocksdb/cache.h"

namespace rocksdb {

/* Universal Cache */

class UniCache {
public:
  virtual ~UniCache() {}

  virtual const char *Name() const = 0;

  // virtual Status Insert(UniCacheEntryType type, const Slice &uni_key,
  //                       void *value, size_t charge, int level,
  //                       void (*deleter)(const Slice &key, void *value),
  //                       Cache::Handle **handle = nullptr,
  //                       Cache::Priority priority = Cache::Priority::LOW) = 0;

  // virtual Cache::Handle *Lookup(UniCacheEntryType type, const Slice &key,
  //                               Statistics *stats = nullptr) = 0;

  // virtual bool Ref(UniCacheEntryType type, Cache::Handle *handle) = 0;

  // virtual bool Release(UniCacheEntryType type, Cache::Handle *handle,
  //                      bool force_erase = false) = 0;

  // virtual void *Value(UniCacheEntryType type, Cache::Handle *handle) = 0;

  // virtual void Erase(UniCacheEntryType type, const Slice &key) = 0;

  // virtual void SetCapacity(size_t capacity) = 0;

  // virtual void SetCapacity(UniCacheEntryType type, size_t capacity) = 0;

  // virtual void SetStrictCapacityLimit(bool strict_capacity_limit) = 0;

  // virtual bool HasStrictCapacityLimit() const = 0;

  virtual size_t GetCapacity() const = 0;

  // virtual size_t GetCapacity(UniCacheEntryType type) const = 0;

  // virtual size_t GetUsage() const = 0;

  // virtual size_t GetUsage(UniCacheEntryType type) const = 0;

  // virtual size_t GetUsage(Cache::Handle *handle) const = 0;

  // virtual size_t GetUsage(UniCacheEntryType type,
  //                         Cache::Handle *handle) const = 0;

  // virtual size_t GetPinnedUsage() const = 0;

  // virtual void DisownData() = 0;

  // virtual void ApplyToAllCacheEntries(void (*callback)(void *, size_t),
  //                                     bool thread_safe) = 0;

  // virtual void EraseUnRefEntries() = 0;

  virtual bool AdaptiveSupported() = 0;
};

extern std::shared_ptr<UniCache>
NewUniCacheFix(size_t capacity, double kp_cache_ratio, int num_shard_bits = -1,
               bool strict_capacity_limit = false,
               double high_pri_pool_ratio = 0.0,
               std::shared_ptr<MemoryAllocator> memory_allocator = nullptr,
               bool use_adaptive_mutex = kDefaultToAdaptiveMutex);

extern std::shared_ptr<UniCache>
NewUniCacheAdapt(size_t capacity, int num_shard_bits = -1,
                 bool strict_capacity_limit = false,
                 double recency_init_ratio = 0.5, bool adaptive_size = true,
                 double high_pri_pool_ratio = 0.0,
                 std::shared_ptr<MemoryAllocator> memory_allocator = nullptr,
                 bool use_adaptive_mutex = kDefaultToAdaptiveMutex);

} // namespace rocksdb
