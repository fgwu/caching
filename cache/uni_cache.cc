#include "rocksdb/cache.h"

#include "cache/lru_cache.h"
#include "cache/uni_cache_internal.h"

namespace rocksdb {

static void DeleteDataEntry(const Slice & /*key*/, void *value) {
  DataEntry *typed_value = reinterpret_cast<DataEntry *>(value);
  delete typed_value;
}

static void DeleteGhostEntry(const Slice & /*key*/, void *value) {
  assert(!value);
  // not any value stored. nothing needs to be deleted.
}

UniCacheFix::UniCacheFix(
    size_t capacity, double kp_cache_ratio, int num_shard_bits,
    bool strict_capacity_limit,
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
                           void *value, size_t charge, int /*level*/,
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

std::shared_ptr<LRUCache>
NewPureLRUCache(size_t capacity, int num_shard_bits, bool strict_capacity_limit,
                double high_pri_pool_ratio,
                std::shared_ptr<MemoryAllocator> memory_allocator,
                bool use_adaptive_mutex);

UniCacheAdapt::UniCacheAdapt(
    size_t capacity, int num_shard_bits, bool strict_capacity_limit,
    double recency_init_ratio, bool adaptive_size,
    std::shared_ptr<MemoryAllocator> /*memory_allocator*/)
    : total_capacity_(capacity),
      target_recency_cache_capacity_(recency_init_ratio * capacity),
      adaptive_size_(adaptive_size) {
  size_t recency_real_cache_capacity = target_recency_cache_capacity_;
  size_t frequency_real_cache_capacity =
      total_capacity_ - recency_real_cache_capacity;

  size_t recency_ghost_cache_capacity = frequency_real_cache_capacity;
  size_t frequency_ghost_cache_capacity = recency_real_cache_capacity;

  frequency_real_cache_ = NewPureLRUCache(
      frequency_real_cache_capacity, num_shard_bits, strict_capacity_limit);
  recency_real_cache_ = NewPureLRUCache(recency_real_cache_capacity,
                                        num_shard_bits, strict_capacity_limit);

  // TODO(fwu): reduce the ghost cache memory usage
  frequency_ghost_cache_ = NewPureLRUCache(
      frequency_ghost_cache_capacity, num_shard_bits, strict_capacity_limit);
  recency_ghost_cache_ = NewPureLRUCache(recency_ghost_cache_capacity,
                                         num_shard_bits, strict_capacity_limit);
}

Status UniCacheAdapt::HandleRecencyRealHit(const Slice &key, void *ptr,
                                           size_t charge) {
  std::shared_ptr<autovector<LRUHandle *>> evicted_handles;
  Status s = frequency_real_cache_->Insert(
      key, ptr, charge, &DeleteDataEntry, nullptr /*handle*/,
      Cache::Priority::LOW, &evicted_handles);

  if (!s.ok()) {
    return s;
  }

  // TODO(fwu): shrink the footage of the ghost cache by only store the hash.
  for (LRUHandle *evicted_entry : *evicted_handles) {
    frequency_ghost_cache_->Insert(
        evicted_entry->key(), nullptr,
        evicted_entry
            ->charge /* using virtual charge for same reason as above*/,
        &DeleteGhostEntry, nullptr /*handle*/, Cache::Priority::LOW,
        nullptr /*evicted_handles*/);
    evicted_entry->Free();
  }

  recency_real_cache_->Erase(key);
  return s;
}

Status UniCacheAdapt::HandleRecencyGhostHit(const Slice &key, void *ptr,
                                            size_t charge) {
  std::shared_ptr<autovector<LRUHandle *>> evicted_handles;
  Status s = frequency_real_cache_->Insert(
      key, ptr, charge, &DeleteDataEntry, nullptr /*handle*/,
      Cache::Priority::LOW, &evicted_handles);

  if (!s.ok()) {
    return s;
  }

  // TODO(fwu): shrink the footage of the ghost cache by only store the hash.
  for (LRUHandle *evicted_entry : *evicted_handles) {
    frequency_ghost_cache_->Insert(
        evicted_entry->key(), nullptr,
        evicted_entry
            ->charge /* using virtual charge for same reason as above*/,
        &DeleteGhostEntry, nullptr /*handle*/, Cache::Priority::LOW,
        nullptr /*evicted_handles*/);
    evicted_entry->Free();
  }

  recency_ghost_cache_->Erase(key);
  return s;
}
Status UniCacheAdapt::HandleFrequencyGhostHit(const Slice &key, void *ptr,
                                              size_t charge) {
  std::shared_ptr<autovector<LRUHandle *>> evicted_handles;
  Status s = frequency_real_cache_->Insert(
      key, ptr, charge, &DeleteDataEntry, nullptr /*handle*/,
      Cache::Priority::LOW, &evicted_handles);

  if (!s.ok()) {
    return s;
  }

  // TODO(fwu): shrink the footage of the ghost cache by only store the hash.
  for (LRUHandle *evicted_entry : *evicted_handles) {
    frequency_ghost_cache_->Insert(
        evicted_entry->key(), nullptr,
        evicted_entry
            ->charge /* using virtual charge for same reason as above*/,
        &DeleteGhostEntry, nullptr /*handle*/, Cache::Priority::LOW,
        nullptr /*evicted_handles*/);
    evicted_entry->Free();
  }

  frequency_ghost_cache_->Erase(key);
  return s;
}
Status UniCacheAdapt::HandleBothMiss(const Slice &key, void *ptr,
                                     size_t charge) {
  std::shared_ptr<autovector<LRUHandle *>> evicted_handles;
  Status s = recency_real_cache_->Insert(
      key, ptr, charge, &DeleteDataEntry, nullptr /*handle*/,
      Cache::Priority::LOW, &evicted_handles);
  if (!s.ok()) {
    return s;
  }

  for (LRUHandle *evicted_entry : *evicted_handles) {
    // insert the itme evicted from the real cache to the ghost cache
    // note that the charge is still the origitnal size, although we
    // discard the value.
    // This is to make the following equation holds:
    // real + ghost = physical cache capacity (virtually)
    // TODO(fwu): However the space taken by ghost cache is not counted.
    recency_ghost_cache_->Insert(
        evicted_entry->key(), nullptr, evicted_entry->charge /*virtual charge*/,
        &DeleteGhostEntry, nullptr /*handle*/, Cache::Priority::LOW,
        nullptr /*evicted_handles*/);
    evicted_entry->Free();
  }
  return s;
}

Status UniCacheAdapt::Insert(const Slice &key, DataEntry *data_entry,
                             const UniCacheAdaptArcState &state) {
  if (state == kFrequencyRealHit) {
    // Case I.1 The item was already moved to MRU of Freq Real during
    // Lookup(), so nothing else has to be done.
    // the item was just hit in FrequencyRealCache, we need not
    // move it ot any other cache.
    return Status::OK();
  }

  Status s;
  size_t charge = 0;

  // data_entry: calculate size charge
  assert(data_entry);
  switch (data_entry->data_type) {
  case kKV:
    assert(!data_entry->kv_entry()->get_context_replay_log.empty());
    charge = key.size() + sizeof(DataEntry) +
             data_entry->kv_entry()->get_context_replay_log.size();
    break;
  case kKP:
    assert(!data_entry->kp_entry()->block_handle.IsNull());
    charge = key.size() + sizeof(DataEntry);
    break;
  default:
    assert(0);
  }

  void *ptr = new DataEntry(std::move(*data_entry));

  // sanity check on the UniCache components
  assert(frequency_real_cache_->GetCapacity() > 0);
  assert(recency_real_cache_->GetCapacity() > 0);

  switch (state) {
  case kFrequencyRealHit: // Case I.B Already handled above
    assert(0);
  case kRecencyRealHit: // Case I.A
    return HandleRecencyRealHit(key, ptr, charge);
  case kRecencyGhostHit: // Case II
    return HandleRecencyGhostHit(key, ptr, charge);
  case kFrequencyGhostHit: // Case II*
    return HandleFrequencyGhostHit(key, ptr, charge);
  case kBothMiss: // Case IV
    return HandleBothMiss(key, ptr, charge);
  default:
    assert(0);
  }
  return s;
}

UniCacheAdaptHandle UniCacheAdapt::Lookup(const Slice &key, Statistics *stats) {
  Cache::Handle *handle = nullptr;

  if ((handle = frequency_real_cache_->Lookup(key, stats)) != nullptr) {
    // frequency real hit, no size adjustment needed.
    return UniCacheAdaptHandle(handle, kFrequencyRealHit);
  }

  if ((handle = recency_real_cache_->Lookup(key, stats)) != nullptr) {
    // recency real hit, should move to MRU of frequency real cache
    // however, promotion is possible. Let the caller to erase from recency real
    // cache and insert to frequency real cache later.
    return UniCacheAdaptHandle(handle, kRecencyRealHit);
  }

  if ((handle = frequency_ghost_cache_->Lookup(key, stats)) != nullptr) {
    // frequency ghost hit, no size adjustment needed.
    // the caller should insert the value to MRU of freq real cache
    frequency_ghost_cache_->Release(handle);
    return UniCacheAdaptHandle(nullptr, kFrequencyGhostHit);
  }

  if ((handle = recency_ghost_cache_->Lookup(key, stats)) != nullptr) {
    // recency ghost hit, no size adjustment needed.
    // the caller should insert the value to MRU of freq real cache
    recency_ghost_cache_->Release(handle);
    return UniCacheAdaptHandle(nullptr, kRecencyGhostHit);
  }

  // the caller should insert the value to MRU of recency real cache
  // no need to adjust sizes.
  return UniCacheAdaptHandle(nullptr, kBothMiss);
}

bool UniCacheAdapt::Release(const UniCacheAdaptHandle &arc_handle,
                            bool force_erase) {
  switch (arc_handle.state) {
  case kFrequencyRealHit:
    return frequency_real_cache_->Release(arc_handle.handle, force_erase);
  case kRecencyRealHit:
    return recency_real_cache_->Release(arc_handle.handle, force_erase);
  case kFrequencyGhostHit:
  case kRecencyGhostHit:
    // we do not have to release any resource, as ghost cache entry
    // has released at Lookup() already.
    return true;
  case kBothMiss:
    assert(0);
  default:
    assert(0);
  }
}

void *UniCacheAdapt::Value(const UniCacheAdaptHandle &arc_handle) {
  switch (arc_handle.state) {
  case kFrequencyRealHit:
    return frequency_real_cache_->Value(arc_handle.handle);
  case kRecencyRealHit:
    return recency_real_cache_->Value(arc_handle.handle);
  case kFrequencyGhostHit:
  case kRecencyGhostHit:
  case kBothMiss:
    assert(0);
  default:
    assert(0);
  }
  return nullptr; // cannot reach here.
}

void UniCacheAdapt::Erase(const Slice &key,
                          const UniCacheAdaptArcState &state) {
  switch (state) {
  case kFrequencyRealHit:
    return frequency_real_cache_->Erase(key);
  case kRecencyRealHit:
    return recency_real_cache_->Erase(key);
  case kFrequencyGhostHit:
  case kRecencyGhostHit:
  case kBothMiss:
    assert(0);
  default:
    assert(0);
  }
}

size_t UniCacheAdapt::GetCapacity() const {
  return frequency_real_cache_->GetCapacity() +
         recency_real_cache_->GetCapacity();
}

std::shared_ptr<UniCache>
NewUniCacheFix(size_t capacity, double kp_cache_ratio, int num_shard_bits,
               bool strict_capacity_limit, double /*high_pri_pool_ratio*/,
               std::shared_ptr<MemoryAllocator> memory_allocator,
               bool /*use_adaptive_mutex*/) {
  if (num_shard_bits >= 20) {
    return nullptr; // the cache cannot be sharded into too many fine pieces
  }

  if (num_shard_bits < 0) {
    num_shard_bits = GetDefaultCacheShardBits(capacity);
  }
  return std::make_shared<UniCacheFix>(capacity, kp_cache_ratio, num_shard_bits,
                                       strict_capacity_limit,
                                       std::move(memory_allocator));
}

std::shared_ptr<UniCache>
NewUniCacheAdapt(size_t capacity, int num_shard_bits,
                 bool strict_capacity_limit, double recency_init_ratio,
                 bool adaptive_size, double /*high_pri_pool_ratio*/,
                 std::shared_ptr<MemoryAllocator> memory_allocator,
                 bool /*use_adaptive_mutex*/) {
  if (num_shard_bits >= 20) {
    return nullptr; // the cache cannot be sharded into too many fine pieces
  }

  if (num_shard_bits < 0) {
    num_shard_bits = GetDefaultCacheShardBits(capacity);
  }
  return std::make_shared<UniCacheAdapt>(
      capacity, num_shard_bits, strict_capacity_limit, recency_init_ratio,
      adaptive_size, std::move(memory_allocator));
}

} // namespace rocksdb
