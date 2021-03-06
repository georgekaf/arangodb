////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2017 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Daniel H. Larkin
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGODB_CACHE_TRANSACTIONAL_CACHE_H
#define ARANGODB_CACHE_TRANSACTIONAL_CACHE_H

#include "Basics/Common.h"
#include "Cache/Cache.h"
#include "Cache/CachedValue.h"
#include "Cache/FrequencyBuffer.h"
#include "Cache/Manager.h"
#include "Cache/ManagerTasks.h"
#include "Cache/Metadata.h"
#include "Cache/State.h"
#include "Cache/TransactionalBucket.h"

#include <stdint.h>
#include <atomic>
#include <chrono>
#include <list>

namespace arangodb {
namespace cache {

////////////////////////////////////////////////////////////////////////////////
/// @brief A transactional, LRU-ish cache.
///
/// To create a cache, see Manager class. Once created, the class has a simple
/// API mostly following that of the base Cache class. For any non-pure-virtual
/// functions, see Cache.h for documentation. The only additional functions
/// exposed on the API of the transactional cache are those dealing with the
/// blacklisting of keys.
///
/// To operate correctly, whenever a key is about to be written to the backing
/// store, it must be blacklisted in any corresponding transactional caches.
/// This will prevent the cache from serving stale or potentially incorrect
/// values and allow for clients to fall through to the backing transactional
/// store.
////////////////////////////////////////////////////////////////////////////////
class TransactionalCache final : public Cache {
 public:
  TransactionalCache(Cache::ConstructionGuard guard, Manager* manager,
                     Manager::MetadataItr metadata, bool allowGrowth,
                     bool enableWindowedStats);
  ~TransactionalCache();

  TransactionalCache() = delete;
  TransactionalCache(TransactionalCache const&) = delete;
  TransactionalCache& operator=(TransactionalCache const&) = delete;

 public:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief Looks up the given key.
  ///
  /// May report a false negative if it fails to acquire a lock in a timely
  /// fashion. Should not block for long.
  //////////////////////////////////////////////////////////////////////////////
  Cache::Finding find(void const* key, uint32_t keySize);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief Attempts to insert the given value.
  ///
  /// Returns true if inserted, false otherwise. Will not insert if the key is
  /// (or its corresponding hash) is blacklisted. Will not insert value if this
  /// would cause the total usage to exceed the limits. May also not insert
  /// value if it fails to acquire a lock in a timely fashion. Should not block
  /// for long.
  //////////////////////////////////////////////////////////////////////////////
  bool insert(CachedValue* value);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief Attempts to remove the given key.
  ///
  /// Returns true if the key guaranteed not to be in the cache, false if the
  /// key may remain in the cache. May leave the key in the cache if it fails to
  /// acquire a lock in a timely fashion. Makes more attempts to acquire a lock
  /// before quitting, so may block for longer than find or insert. Client may
  /// re-try.
  //////////////////////////////////////////////////////////////////////////////
  bool remove(void const* key, uint32_t keySize);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief Attempts to blacklist the given key.
  ///
  /// Returns true if the key was blacklisted and is guaranteed not to be in the
  /// cache, false otherwise. May not blacklist the key if it fails to
  /// acquire a lock in a timely fashion. Makes more attempts to acquire a lock
  /// before quitting, so may block for longer than find or insert. Client
  /// should re-try.
  //////////////////////////////////////////////////////////////////////////////
  bool blacklist(void const* key, uint32_t keySize);

 private:
  // main table info
  TransactionalBucket* _table;
  uint32_t _logSize;
  uint64_t _tableSize;
  uint32_t _maskShift;
  uint32_t _bucketMask;

  // auxiliary table info
  TransactionalBucket* _auxiliaryTable;
  uint32_t _auxiliaryLogSize;
  uint64_t _auxiliaryTableSize;
  uint32_t _auxiliaryMaskShift;
  uint32_t _auxiliaryBucketMask;

  // friend class manager and tasks
  friend class FreeMemoryTask;
  friend class Manager;
  friend class MigrateTask;

 private:
  static uint64_t allocationSize(bool enableWindowedStats);
  static std::shared_ptr<Cache> create(Manager* manager,
                                       Manager::MetadataItr metadata,
                                       bool allowGrowth,
                                       bool enableWindowedStats);
  // management
  bool freeMemory();
  bool migrate();
  void clearTables();

  // helpers
  std::pair<bool, TransactionalBucket*> getBucket(uint32_t hash,
                                                  int64_t maxTries,
                                                  bool singleOperation = true);
  void clearTable(TransactionalBucket* table, uint64_t tableSize);
  uint32_t getIndex(uint32_t hash, bool useAuxiliary) const;
};

};  // end namespace cache
};  // end namespace arangodb

#endif
