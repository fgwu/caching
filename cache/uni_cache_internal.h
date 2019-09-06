#pragma once

#include "cache/lru_cache.h"
#include "rocksdb/cache.h"
#include "rocksdb/uni_cache.h"
#include "table/format.h"

namespace rocksdb {

/* Universal Cache */

enum UniCacheEntryType { kNotSet = 0, kKV = 1, kKP = 2, kKVR = 3, kKPR = 4 };

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

struct ValueLogAndLevel {
  std::string get_context_replay_log;
  unsigned int level;
};

struct DataEntry {
  UniCacheEntryType data_type;
  void *entry;

  DataEntry() : data_type(kNotSet), entry(nullptr) {}

  ~DataEntry() {
    switch (data_type) {
    case kKV:
      if (entry) {
        delete (reinterpret_cast<ValueLogAndLevel *>(entry));
      }
      entry = nullptr;
      break;
    case kKP:
      if (entry) {
        delete (reinterpret_cast<FilePointerAndBlockHandle *>(entry));
      }
      entry = nullptr;
      break;
    default:
      assert(0);
    }
  }

  ValueLogAndLevel *kv_entry() {
    assert(data_type == kKV);
    return reinterpret_cast<ValueLogAndLevel *>(entry);
  }

  const ValueLogAndLevel *kv_entry() const {
    assert(data_type == kKV);
    return reinterpret_cast<ValueLogAndLevel *>(entry);
  }

  FilePointerAndBlockHandle *kp_entry() {
    assert(data_type == kKV);
    return reinterpret_cast<FilePointerAndBlockHandle *>(entry);
  }

  const FilePointerAndBlockHandle *kp_entry() const {
    assert(data_type == kKV);
    return reinterpret_cast<FilePointerAndBlockHandle *>(entry);
  }

  int level() const {
    assert(entry);
    switch (data_type) {
    case kKV:
      return reinterpret_cast<ValueLogAndLevel *>(entry)->level;
    case kKP:
      return reinterpret_cast<FilePointerAndBlockHandle *>(entry)->file_pointer.level;
    default:
      assert(0);
    }
  }
  
  void InitNew(UniCacheEntryType _data_type) {
    data_type = _data_type;
    switch (data_type) {
    case kKV:
      entry = new ValueLogAndLevel();
      break;
    case kKP:
      entry = new FilePointerAndBlockHandle();
      break;
    default:
      assert(0);
    }
  }

  DataEntry(DataEntry &&other) { *this = std::move(other); }

  DataEntry &operator=(DataEntry &&other) {
    data_type = other.data_type;
    entry = other.entry;
    other.entry = nullptr;
    return *this;
  }
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

const size_t kAdaptBaseUnitSize = 1 << 8;
const size_t kAdaptBaseUnitSizeSqure = 1 << 16;

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
  inline void AdjustSize() {
    
  }

  static inline size_t AdjustAmount(int level, size_t charge) {
    // E.g. level = 3. The get will have travel L0 .. L3
    // L0 suppose has 8 SSTs, then
    // Saved I/O = L0 + L1 + L2 + L3 = 8 + 1 + 1 + 1.
    // E.g. level = 0 we estimate half of the L0 will be traveled
    // suppose L0 has 8 SSTs, then
    // Saved I/O = 8/2 = 4;
    int estimated_saved_io = level ? (level + 8) : 4;

    // kAdaptBaseUnitSize * estimiated_saved_io / (charge / kAdaptBaseUnitSize)
    return kAdaptBaseUnitSizeSqure * estimiated_saved_io / charge;
  }

  std::shared_ptr<LRUCache> frequency_real_cache_;
  std::shared_ptr<LRUCache> recency_real_cache_;

  std::shared_ptr<LRUCache> frequency_ghost_cache_;
  std::shared_ptr<LRUCache> recency_ghost_cache_;

  size_t total_capacity_;
  size_t target_recency_cache_capacity_;
};

} // namespace rocksdb
