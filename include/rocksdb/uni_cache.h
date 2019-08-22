#pragma once

#include "rocksdb/cache.h"

namespace rocksdb {

/* Universal Cache */

enum UniCacheEntryType { kKV = 0, kKP = 1, kKVR = 2, kKPR = 3 };

struct UniCacheKey {
  Slice key;
  UniCacheEntryType type;
};

// using level and index_in_level to locate a file.
// if the FileDescriptor agrees with packed_number_and_path_id,
// it means the file is still valid.
struct FilePointer {
  unsigned int level;
  unsigned int index_in_level;
  uint64_t packed_number_and_path_id;

  FilePointer() : level(0), index_in_level(0), packed_number_and_path_id(0) {}
};

struct FilePointerAndBlockHandle;

class UniCache {
public:
  virtual ~UniCache() {}

  virtual const char *Name() const = 0;

  virtual Status Insert(UniCacheEntryType type, const Slice &uni_key,
                        void *value, size_t charge,
                        void (*deleter)(const Slice &key, void *value),
                        Cache::Handle **handle = nullptr,
                        Cache::Priority priority = Cache::Priority::LOW) = 0;

  virtual Cache::Handle *Lookup(UniCacheEntryType type, const Slice &key,
                                Statistics *stats = nullptr) = 0;

  virtual bool Ref(UniCacheEntryType type, Cache::Handle *handle) = 0;

  virtual bool Release(UniCacheEntryType type, Cache::Handle *handle,
                       bool force_erase = false) = 0;

  virtual void *Value(UniCacheEntryType type, Cache::Handle *handle) = 0;

  virtual void Erase(UniCacheEntryType type, const Slice &key) = 0;

  virtual void SetCapacity(size_t capacity) = 0;

  virtual void SetCapacity(UniCacheEntryType type, size_t capacity) = 0;

  virtual void SetStrictCapacityLimit(bool strict_capacity_limit) = 0;

  virtual bool HasStrictCapacityLimit() const = 0;

  virtual size_t GetCapacity() const = 0;

  virtual size_t GetCapacity(UniCacheEntryType type) const = 0;

  virtual size_t GetUsage() const = 0;

  virtual size_t GetUsage(UniCacheEntryType type) const = 0;

  virtual size_t GetUsage(Cache::Handle *handle) const = 0;

  virtual size_t GetUsage(UniCacheEntryType type,
                          Cache::Handle *handle) const = 0;

  virtual size_t GetPinnedUsage() const = 0;

  virtual void DisownData() = 0;

  virtual void ApplyToAllCacheEntries(void (*callback)(void *, size_t),
                                      bool thread_safe) = 0;

  virtual void EraseUnRefEntries() = 0;
};

class UniCacheFix : public UniCache {
public:
  UniCacheFix(size_t capacity, double kp_cache_ratio, int num_shard_bits, bool strict_capacity_limit,
              std::shared_ptr<MemoryAllocator> memory_allocator = nullptr);
  virtual ~UniCacheFix();

  virtual const char *Name() const override { return "UniCacheFix"; }

  virtual Status
  Insert(UniCacheEntryType type, const Slice &uni_key, void *value,
         size_t charge, void (*deleter)(const Slice &key, void *value),
         Cache::Handle **handle = nullptr,
         Cache::Priority priority = Cache::Priority::LOW) override;

  virtual Cache::Handle *Lookup(UniCacheEntryType type, const Slice &key,
                                Statistics *stats = nullptr) override;

  virtual bool Ref(UniCacheEntryType type, Cache::Handle *handle) override;

  virtual bool Release(UniCacheEntryType type, Cache::Handle *handle,
                       bool force_erase = false) override;

  virtual void *Value(UniCacheEntryType type, Cache::Handle *handle) override;

  virtual void Erase(UniCacheEntryType type, const Slice &key) override;

  virtual void SetCapacity(size_t capacity) override;

  virtual void SetCapacity(UniCacheEntryType type, size_t capacity) override;

  virtual void SetStrictCapacityLimit(bool strict_capacity_limit) override;

  virtual bool HasStrictCapacityLimit() const override;

  virtual size_t GetCapacity() const override;

  virtual size_t GetCapacity(UniCacheEntryType type) const override;

  virtual size_t GetUsage() const override;

  virtual size_t GetUsage(UniCacheEntryType type) const override;

  virtual size_t GetUsage(Cache::Handle *handle) const override;

  virtual size_t GetUsage(UniCacheEntryType type,
                          Cache::Handle *handle) const override;

  virtual size_t GetPinnedUsage() const override;

  virtual void DisownData() override;

  virtual void ApplyToAllCacheEntries(void (*callback)(void *, size_t),
                                      bool thread_safe) override;

  virtual void EraseUnRefEntries() override;

private:
  std::shared_ptr<Cache> kv_cache_;
  std::shared_ptr<Cache> kp_cache_;
  double kp_cache_ratio_;
};

extern std::shared_ptr<UniCache>
NewUniCacheFix(size_t capacity, double kp_cache_ratio, int num_shard_bits = -1,
               bool strict_capacity_limit = false,
               double high_pri_pool_ratio = 0.0,
               std::shared_ptr<MemoryAllocator> memory_allocator = nullptr,
               bool use_adaptive_mutex = kDefaultToAdaptiveMutex);

} // namespace rocksdb
