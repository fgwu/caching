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
  UniCache(size_t capacity, int num_shard_bits, bool strict_capacity_limit,
           std::shared_ptr<MemoryAllocator> memory_allocator = nullptr);
  virtual ~UniCache();

  virtual const char *Name() const { return "UniCache"; }

  virtual Status Insert(UniCacheEntryType type, const Slice &uni_key,
                        void *value, size_t charge,
                        void (*deleter)(const Slice &key, void *value),
                        Cache::Handle **handle = nullptr,
                        Cache::Priority priority = Cache::Priority::LOW);

  virtual Cache::Handle *Lookup(UniCacheEntryType type, const Slice &key,
                                Statistics *stats = nullptr);

  virtual bool Ref(UniCacheEntryType type, Cache::Handle *handle);

  virtual bool Release(UniCacheEntryType type, Cache::Handle *handle,
                       bool force_erase = false);

  virtual void *Value(UniCacheEntryType type, Cache::Handle *handle);

  virtual void Erase(UniCacheEntryType type, const Slice &key);

  virtual void SetCapacity(size_t capacity);

  virtual void SetCapacity(UniCacheEntryType type, size_t capacity);

  virtual void SetStrictCapacityLimit(bool strict_capacity_limit);

  virtual bool HasStrictCapacityLimit() const;

  virtual size_t GetCapacity() const;

  virtual size_t GetCapacity(UniCacheEntryType type) const;

  virtual size_t GetUsage() const;

  virtual size_t GetUsage(UniCacheEntryType type) const;

  virtual size_t GetUsage(Cache::Handle *handle) const;

  virtual size_t GetUsage(UniCacheEntryType type, Cache::Handle *handle) const;

  virtual size_t GetPinnedUsage() const;

  virtual void DisownData();

  virtual void ApplyToAllCacheEntries(void (*callback)(void *, size_t),
                                      bool thread_safe);

  virtual void EraseUnRefEntries();

private:
  std::shared_ptr<Cache> kv_cache_;
  std::shared_ptr<Cache> kp_cache_;
};

} // namespace rocksdb
