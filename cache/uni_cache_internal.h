#pragma once

#include "rocksdb/cache.h"
#include "rocksdb/uni_cache.h"
#include "table/format.h"

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

struct FilePointerAndBlockHandle {
  // copiable data structure. We will drop the TableReader pointer inside it
  // as when the cached BlockPointer is found again, the TableReader pointer
  // may no longer be valid.
  FilePointer file_pointer;
  BlockHandle block_handle;

  FilePointerAndBlockHandle() : block_handle(0, 0) {}
};

enum UniCacheAdaptArcState {
  kBothMiss = 0,
  kFrequencyRealHit = 1,
  kRecencyRealHit = 2,
  kFrequencyGhostHit = 3,
  kRecencyGhostHit = 4,
};

struct UniCacheAdaptHandle {
  Cache::Handle *handle;
  UniCacheAdaptArcState state;
  UniCacheAdaptHandle() : handle(nullptr), state(kBothMiss) {}
  UniCacheAdaptHandle(Cache::Handle *h, UniCacheAdaptArcState s)
      : handle(h), state(s) {}
};

struct ValueLogAndLevel {
  std::string get_context_replay_log;
  int level;
  ~ValueLogAndLevel() { get_context_replay_log.~basic_string(); }

  ValueLogAndLevel(ValueLogAndLevel &&other) { *this = std::move(other); }

  ValueLogAndLevel &operator=(ValueLogAndLevel &&other) {
    get_context_replay_log = std::move(other.get_context_replay_log);
    level = other.level;
    return *this;
  }
};

struct DataEntry {
  UniCacheEntryType data_type;
  union {
    ValueLogAndLevel kv_entry;
    FilePointerAndBlockHandle kp_entry;
  };

  DataEntry() {}

  DataEntry(DataEntry &&other) { *this = std::move(other); }

  DataEntry &operator=(DataEntry &&other) {
    data_type = other.data_type;
    switch (data_type) {
    case kKV:
      kv_entry = std::move(other.kv_entry);
      break;
    case kKP:
      kp_entry = std::move(other.kp_entry);
      break;
    default:
      assert(0);
    }
    return *this;
  }

  ~DataEntry() {
    if (data_type == kKV) {
      kv_entry.~ValueLogAndLevel();
    }
  }
};

class UniCacheFix : public UniCache {
public:
  UniCacheFix(size_t capacity, double kp_cache_ratio, int num_shard_bits,
              bool strict_capacity_limit,
              std::shared_ptr<MemoryAllocator> memory_allocator = nullptr);
  virtual ~UniCacheFix();

  virtual const char *Name() const override { return "UniCacheFix"; }

  virtual Status Insert(UniCacheEntryType type, const Slice &uni_key,
                        void *value, size_t charge, int level,
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

  virtual size_t GetCapacity() const override;

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

  virtual bool AdaptiveSupported() override { return false; }

private:
  std::shared_ptr<Cache> kv_cache_;
  std::shared_ptr<Cache> kp_cache_;
  double kp_cache_ratio_;
};

class UniCacheAdapt : public UniCache {
public:
  UniCacheAdapt(size_t capacity, int num_shard_bits, bool strict_capacity_limit,
                std::shared_ptr<MemoryAllocator> memory_allocator = nullptr);

  virtual const char *Name() const override { return "UniCacheAdapt"; }

  virtual Status Insert(const Slice &uni_key, DataEntry *data_entry,
                        const UniCacheAdaptArcState &state);

  virtual UniCacheAdaptHandle Lookup(const Slice &key,
                                     Statistics *stats = nullptr);

  virtual bool Release(const UniCacheAdaptHandle &handle,
                       bool force_erase = false);

  virtual void *Value(const UniCacheAdaptHandle &arc_handle);

  virtual void Erase(const Slice &key, const UniCacheAdaptArcState &state);

  virtual size_t GetCapacity() const override;

  virtual bool AdaptiveSupported() override { return true; }

  Cache *frequency_real_cache() { return frequency_real_cache_.get(); }

  Cache *recency_real_cache() { return recency_real_cache_.get(); }

private:
  // the size is adjusted after each ghost hit.
  void AdjustSize() { assert(0); /*TODO(fwu)*/ }

  std::shared_ptr<Cache> frequency_real_cache_;
  std::shared_ptr<Cache> recency_real_cache_;

  std::shared_ptr<Cache> frequency_ghost_cache_;
  std::shared_ptr<Cache> recency_ghost_cache_;

  size_t total_capacity_;
  size_t target_recency_cache_capacity_;
};

} // namespace rocksdb
