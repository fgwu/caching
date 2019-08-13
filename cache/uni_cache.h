#pragma once

#include "rocksdb/cache.h"

namespace rocksdb {

/* Universal Cache */

enum UniCacheEntryType { kKV = 0, kKP = 1, kKVR = 2, kKPR = 3 };

struct UniCacheKey {
  Slice key;
  UniCacheEntryType type;
};

struct FdAndBlockHandle {
  // copiable data structure. We will drop the TableReader pointer inside it
  // as when the cached BlockPointer is found again, the TableReader pointer
  // may no longer be valid.
  FileDescriptor fd;
  BlockHandler block_handle;
};

class UniCache : public Cache {
public:
  UniCache(size_t capacity, int num_shard_bits, bool strict_capacity_limit,
           std::shared_ptr<MemoryAllocator> memory_allocator = nullptr);
  ~UniCache();

  virtual const char *Name() const override { return "UniCache"; }

  virtual Status Insert(const Slice &key, void *value, size_t charge,
                        void (*deleter)(const Slice &key, void *value),
                        Handle **handle = nullptr,
                        Priority priority = Priority::LOW) override;

  virtual Status Insert(UniCacheEntryType type, const Slice &uni_key,
                        void *value, size_t charge,
                        void (*deleter)(const Slice &key, void *value),
                        Handle **handle = nullptr,
                        Priority priority = Priority::LOW);

  virtual Handle *Lookup(const Slice &key,
                         Statistics *stats = nullptr) override;

  virtual Handle *Lookup(UniCacheEntryType type, const Slice &key,
                         Statistics *stats = nullptr);

  virtual bool Ref(Handle *handle) override;

  virtual bool Ref(UniCacheEntryType type, Handle *handle);

  virtual bool Release(Handle *handle, bool force_erase = false) override;

  virtual bool Release(UniCacheEntryType type, Handle *handle,
                       bool force_erase = false);

  virtual void *Value(Handle *handle) override;

  virtual void *Value(UniCacheEntryType type, Handle *handle);

  virtual void Erase(const Slice &key) override;

  virtual void Erase(UniCacheEntryType type, const Slice &key);

  virtual uint64_t NewId() override;

  virtual void SetCapacity(size_t capacity) override;

  virtual void SetStrictCapacityLimit(bool strict_capacity_limit) override;

  virtual bool HasStrictCapacityLimit() const override;

  virtual size_t GetCapacity() const override;

  virtual size_t GetUsage() const override;

  virtual size_t GetUsage(Handle *handle) const override;

  virtual size_t GetPinnedUsage() const override;

  virtual void DisownData() override;

  virtual void ApplyToAllCacheEntries(void (*callback)(void *, size_t),
                                      bool thread_safe) override;

  virtual void EraseUnRefEntries() override;

private:
  std::shared_ptr<Cache> kv_cache_;
  std::shared_ptr<Cache> kp_cache_;
};

} // namespace rocksdb
