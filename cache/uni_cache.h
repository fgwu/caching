#pragma once

#include "rocksdb/cache.h"

namespace rocksdb {

/* Universal Cache */
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

  virtual Handle *Lookup(const Slice &key,
                         Statistics *stats = nullptr) override;

  virtual bool Ref(Handle *handle) override;

  virtual bool Release(Handle *handle, bool force_erase = false) override;

  virtual void *Value(Handle *handle) override;

  virtual void Erase(const Slice &key) override;

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
  std::shared_ptr<Cache> cache_;
};

} // namespace rocksdb
