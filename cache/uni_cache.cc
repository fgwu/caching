#include "rocksdb/cache.h"

#include "cache/lru_cache.h"
#include "cache/uni_cache_internal.h"

#include <algorithm>

namespace rocksdb {

size_t UniCacheAdapt::adapt_base_unit_size_square_ = 1 << 16;

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

void UniCacheAdapt::AdjustCapacity(UniCacheAdaptArcState state) {
  size_t usage_frequency_real;
  size_t usage_recency_real;
  size_t usage_recency_ghost;

  std::shared_ptr<autovector<LRUHandle *>> recency_real_victims;
  std::shared_ptr<autovector<LRUHandle *>> frequency_real_victims;

  // Shrink size of the *real caches* to the desired state.
  // whether to shrink recency_real or frequency_real depends on
  // the target_recency_real_cache_size
  if (recency_real_cache_->GetUsage() > target_recency_cache_capacity_ ||
      state == kRecencyRealHit) {
    // evict from recency_real

    // a) fix frequency_real. The only limit is the total cache size.
    frequency_real_cache_->SetCapacity(total_capacity_,
                                       &frequency_real_victims);
    usage_frequency_real = frequency_real_cache_->GetUsage();

    // b) handle recency_real, based on the freq real size.
    recency_real_cache_->SetCapacity(total_capacity_ - usage_frequency_real,
                                     &recency_real_victims);
    usage_recency_real = recency_real_cache_->GetUsage();
  } else {
    // recency_too_large == false, evict from freq_real

    // a) fix recency_real. The only limit is the total_capacity
    recency_real_cache_->SetCapacity(total_capacity_, &recency_real_victims);
    usage_recency_real = recency_real_cache_->GetUsage();

    // b) handle frequency_real, based on the recency_real size
    frequency_real_cache_->SetCapacity(total_capacity_ - usage_recency_real,
                                       &frequency_real_victims);
    usage_frequency_real = frequency_real_cache_->GetUsage();
  }

  // 4. Handle the *ghost cache* size. Always handle the recency_ghost first
  // first insert the overflow victims, and then set the capaicty

  // c) handle recency_ghost, based on the recency_real size, and the overflow
  // evicted victim list
  for (LRUHandle *recency_real_victim : *recency_real_victims) {
    recency_ghost_cache_->Insert(
        recency_real_victim->key(), nullptr,
        recency_real_victim->charge /*virtual charge*/, &DeleteGhostEntry,
        nullptr /*handle*/, Cache::Priority::LOW, nullptr /*evicted_handles*/);
    recency_real_victim->Free();
  }
  recency_ghost_cache_->SetCapacity(total_capacity_ - usage_recency_real);
  usage_recency_ghost = recency_ghost_cache_->GetUsage();

  // d) handle the frequency_ghost cache, base on the usage of all others
  for (LRUHandle *frequency_real_victim : *frequency_real_victims) {
    frequency_ghost_cache_->Insert(
        frequency_real_victim->key(), nullptr,
        frequency_real_victim->charge /*virtual charge*/, &DeleteGhostEntry,
        nullptr /*handle*/, Cache::Priority::LOW, nullptr /*evicted_handles*/);
    frequency_real_victim->Free();
  }
  frequency_ghost_cache_->SetCapacity(total_capacity_ * 2 -
                                      usage_frequency_real -
                                      usage_recency_real - usage_recency_ghost);

  // now every cache has been settled down. Adjust the capacity to the usage
  frequency_real_cache_->SetCapacity(usage_frequency_real);
  recency_real_cache_->SetCapacity(usage_recency_real);
  recency_ghost_cache_->SetCapacity(usage_recency_ghost);

  assert(frequency_real_cache_->GetCapacity() +
             recency_real_cache_->GetCapacity() +
             frequency_ghost_cache_->GetCapacity() +
             recency_ghost_cache_->GetCapacity() ==
         total_capacity_ * 2);
}

Status UniCacheAdapt::Insert(const Slice &key, DataEntry *data_entry,
                             UniCacheAdaptArcState prev_lookup_state) {
  if (prev_lookup_state == kFrequencyRealHit) {
    // Case I.1 The item was already moved to MRU of Freq Real during
    // Lookup(), so nothing else has to be done.
    // the item was just hit in FrequencyRealCache, we need not
    // move it ot any other cache.
    return Status::OK();
  }

  Status s;
  size_t charge = 0;
  int level = data_entry->level();

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

  // 1. Enlarge Capacity first. The size will be adjust adaptively (shrink)
  // later in AdjustSize()
  size_t large_capacity = total_capacity_ * 3;
  frequency_real_cache_->SetCapacity(large_capacity);
  recency_real_cache_->SetCapacity(large_capacity);
  frequency_ghost_cache_->SetCapacity(large_capacity);
  recency_ghost_cache_->SetCapacity(large_capacity);

  // 2. Perform insertion and deletion as necessary
  std::shared_ptr<autovector<LRUHandle *>> victims;
  switch (prev_lookup_state) {
  case kFrequencyRealHit: // Case I.B Already handled above
    assert(0);
  case kRecencyRealHit: // Case I.A
  {
    recency_real_cache_->Erase(key);
    s = frequency_real_cache_->Insert(key, ptr, charge, &DeleteDataEntry,
                                      nullptr /*handle*/, Cache::Priority::LOW,
                                      &victims);
    assert(s.ok());
    // Cache size big enough, Shouldn't cause any eviction
    assert(victims->size() == 0);
    break;
  }
  case kRecencyGhostHit: // Case II
  {
    AdjustTargetRecencyRealCacheSize(true /*increase_flag*/, level, charge);
    recency_ghost_cache_->Erase(key);
    s = frequency_real_cache_->Insert(key, ptr, charge, &DeleteDataEntry,
                                      nullptr /*handle*/, Cache::Priority::LOW,
                                      &victims);
    assert(s.ok());
    // Cache size big enough, Shouldn't cause any eviction
    assert(victims->size() == 0);
    break;
  }
  case kFrequencyGhostHit: // Case III
  {
    AdjustTargetRecencyRealCacheSize(false /*increase_flag*/, level, charge);
    frequency_ghost_cache_->Erase(key);
    s = frequency_real_cache_->Insert(key, ptr, charge, &DeleteDataEntry,
                                      nullptr /*handle*/, Cache::Priority::LOW,
                                      &victims);
    assert(s.ok());
    // Cache size big enough, Shouldn't cause any eviction
    assert(victims->size() == 0);
    break;
  }
  case kBothMiss: // Case IV
  {
    s = recency_real_cache_->Insert(key, ptr, charge, &DeleteDataEntry,
                                    nullptr /*handle*/, Cache::Priority::LOW,
                                    &victims);
    assert(s.ok());
    // Cache size big enough, Shouldn't cause any eviction
    assert(victims->size() == 0);
    break;
  }
  default:
    assert(0);
  }

  AdjustCapacity(prev_lookup_state);

  return s;
}

// used during flush, where a new KV is replacing the old KV.
// the newly create DataEntry will be inserted to the old cache.
Status UniCacheAdapt::InsertInPlace(const Slice &key, DataEntry *data_entry,
                                    UniCacheAdaptArcState prev_lookup_state) {
  if (prev_lookup_state != kFrequencyRealHit &&
      prev_lookup_state != kRecencyRealHit) {
    assert(0);
  }
  assert(data_entry->data_type == kKV);

  Status s;
  size_t charge = key.size() + sizeof(DataEntry) +
                  data_entry->kv_entry()->get_context_replay_log.size();

  void *ptr = new DataEntry(std::move(*data_entry));

  if (prev_lookup_state == kFrequencyRealHit) {
    std::shared_ptr<autovector<LRUHandle *>> frequency_real_victims;

    s = frequency_real_cache_->Insert(key, ptr, charge, &DeleteDataEntry,
                                      nullptr /*handle*/, Cache::Priority::LOW,
                                      &frequency_real_victims);
    for (LRUHandle *frequency_real_victim : *frequency_real_victims) {
      frequency_ghost_cache_->Insert(
          frequency_real_victim->key(), nullptr,
          frequency_real_victim->charge /*virtual charge*/, &DeleteGhostEntry,
          nullptr /*handle*/, Cache::Priority::LOW,
          nullptr /*evicted_handles*/);
      frequency_real_victim->Free();
    }
  } else if (prev_lookup_state == kRecencyRealHit) {
    std::shared_ptr<autovector<LRUHandle *>> recency_real_victims;

    s = recency_real_cache_->Insert(key, ptr, charge, &DeleteDataEntry,
                                    nullptr /*handle*/, Cache::Priority::LOW,
                                    &recency_real_victims);
    for (LRUHandle *recency_real_victim : *recency_real_victims) {
      recency_ghost_cache_->Insert(
          recency_real_victim->key(), nullptr,
          recency_real_victim->charge /*virtual charge*/, &DeleteGhostEntry,
          nullptr /*handle*/, Cache::Priority::LOW,
          nullptr /*evicted_handles*/);
      recency_real_victim->Free();
    }
  } else {
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
  case kBothMiss: // nothing ot release
    return true;
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
                          UniCacheAdaptArcState prev_lookup_state) {
  switch (prev_lookup_state) {
  case kFrequencyRealHit:
    frequency_real_cache_->Erase(key);
    return;
  case kRecencyRealHit:
    recency_real_cache_->Erase(key);
    return;
  case kFrequencyGhostHit:
    frequency_ghost_cache_->Erase(key);
    return;
  case kRecencyGhostHit:
    recency_ghost_cache_->Erase(key);
    return;
  case kBothMiss:
    assert(0);
  case kAllErase:
    frequency_real_cache_->Erase(key);
    recency_real_cache_->Erase(key);
    frequency_ghost_cache_->Erase(key);
    recency_ghost_cache_->Erase(key);
    return;
  default:
    assert(0);
  }
}

size_t UniCacheAdapt::GetCapacity() const { return total_capacity_; }

size_t UniCacheAdapt::GetUsage() const {
  return frequency_real_cache_->GetUsage() + recency_real_cache_->GetUsage();
}

std::shared_ptr<UniCache>
NewUniCacheFix(size_t capacity, double kp_cache_ratio, int num_shard_bits,
               bool strict_capacity_limit, double /*high_pri_pool_ratio*/,
               std::shared_ptr<MemoryAllocator> memory_allocator,
               bool /*use_adaptive_mutex*/) {
  assert(num_shard_bits == 0); // TODO(fwu): only use one shard for ease of impl
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
  assert(num_shard_bits == 0); // TODO(fwu): only use one shard for ease of impl
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
