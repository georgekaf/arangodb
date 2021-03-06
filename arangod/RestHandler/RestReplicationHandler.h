////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2016 ArangoDB GmbH, Cologne, Germany
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
/// @author Jan Steemann
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGOD_REST_HANDLER_REST_REPLICATION_HANDLER_H
#define ARANGOD_REST_HANDLER_REST_REPLICATION_HANDLER_H 1

#include "Basics/Common.h"

#include "RestHandler/RestVocbaseBaseHandler.h"
#include "VocBase/replication-common.h"

namespace arangodb {
class ClusterInfo;
class CollectionNameResolver;
class LogicalCollection;
namespace transaction {
class Methods;
}
;

////////////////////////////////////////////////////////////////////////////////
/// @brief replication request handler
////////////////////////////////////////////////////////////////////////////////

class RestReplicationHandler : public RestVocbaseBaseHandler {
 public:
  RestReplicationHandler(GeneralRequest*, GeneralResponse*);
  ~RestReplicationHandler();

 public:
  RestStatus execute() override;
  char const* name() const override final { return "RestReplicationHandler"; }

 public:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief comparator to sort collections
  /// sort order is by collection type first (vertices before edges, this is
  /// because edges depend on vertices being there), then name
  //////////////////////////////////////////////////////////////////////////////

  static bool sortCollections(arangodb::LogicalCollection const*,
                              arangodb::LogicalCollection const*);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief filter a collection based on collection attributes
  //////////////////////////////////////////////////////////////////////////////

  static bool filterCollection(arangodb::LogicalCollection*, void*);

 private:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief creates an error if called on a coordinator server
  //////////////////////////////////////////////////////////////////////////////

  bool isCoordinatorError();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief insert the applier action into an action list
  //////////////////////////////////////////////////////////////////////////////

  void insertClient(TRI_voc_tick_t);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief determine chunk size from request
  //////////////////////////////////////////////////////////////////////////////

  uint64_t determineChunkSize() const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return the state of the replication logger
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandLoggerState();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return the available logfile range
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandLoggerTickRanges();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return the first tick available in a logfile
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandLoggerFirstTick();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle a follow command for the replication log
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandLoggerFollow();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle the command to determine the transactions that were open
  /// at a certain point in time
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandDetermineOpenTransactions();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief turn the server into a slave of another
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandMakeSlave();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle a batch command
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandBatch();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief add or remove a WAL logfile barrier
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandBarrier();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief forward a command in the coordinator case
  //////////////////////////////////////////////////////////////////////////////

  void handleTrampolineCoordinator();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return the inventory (current replication and collection state)
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandInventory();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief returns the cluster inventory, only on coordinator
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandClusterInventory();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief extract the collection id from VelocyPack TODO: MOVE
  //////////////////////////////////////////////////////////////////////////////

  TRI_voc_cid_t getCid(VPackSlice const&) const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief creates a collection, based on the VelocyPack provided TODO: MOVE
  //////////////////////////////////////////////////////////////////////////////

  int createCollection(VPackSlice, arangodb::LogicalCollection**, bool);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle a restore command for a specific collection
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandRestoreCollection();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle a restore command for a specific collection
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandRestoreIndexes();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief restores the structure of a collection TODO MOVE
  //////////////////////////////////////////////////////////////////////////////

  int processRestoreCollection(VPackSlice const&, bool, bool, bool,
                               std::string&);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief restores the structure of a collection, coordinator case
  //////////////////////////////////////////////////////////////////////////////

  int processRestoreCollectionCoordinator(VPackSlice const&, bool, bool, bool,
                                          uint64_t, std::string&, uint64_t);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief restores the indexes of a collection TODO MOVE
  //////////////////////////////////////////////////////////////////////////////

  int processRestoreIndexes(VPackSlice const&, bool, std::string&);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief restores the indexes of a collection, coordinator case
  //////////////////////////////////////////////////////////////////////////////

  int processRestoreIndexesCoordinator(VPackSlice const&, bool, std::string&);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief apply a single marker from the collection dump
  //////////////////////////////////////////////////////////////////////////////

  int applyCollectionDumpMarker(transaction::Methods&,
                                CollectionNameResolver const&,
                                std::string const&,
                                TRI_replication_operation_e,
                                VPackSlice const&,
                                VPackSlice const&, std::string&);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief restores the data of a collection TODO MOVE
  //////////////////////////////////////////////////////////////////////////////

  int processRestoreDataBatch(transaction::Methods&,
                              std::string const&, bool, bool,
                              std::string&);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief restores the data of a collection TODO MOVE
  //////////////////////////////////////////////////////////////////////////////

  int processRestoreData(std::string const&, bool,
                         bool, std::string&);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle a restore command for a specific collection
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandRestoreData();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief produce list of keys for a specific collection
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandCreateKeys();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief returns a key range
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandGetKeys();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief returns date for a key range
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandFetchKeys();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief remove a list of keys for a specific collection
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandRemoveKeys();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle a dump command for a specific collection
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandDump();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle a sync command
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandSync();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle a server-id command
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandServerId();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return the configuration of the the replication applier
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandApplierGetConfig();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief configure the replication applier
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandApplierSetConfig();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief start the replication applier
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandApplierStart();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief stop the replication applier
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandApplierStop();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return the state of the replication applier
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandApplierGetState();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief delete the replication applier state
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandApplierDeleteState();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief add a follower of a shard to the list of followers
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandAddFollower();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief remove a follower of a shard from the list of followers
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandRemoveFollower();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief hold a read lock on a collection to stop writes temporarily
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandHoldReadLockCollection();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief check if we are holding a read lock on a collection
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandCheckHoldReadLockCollection();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief cancel holding a read lock on a collection
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandCancelHoldReadLockCollection();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief get an ID for a hold read lock job
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandGetIdForReadLockCollection();

 private:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief minimum chunk size
  //////////////////////////////////////////////////////////////////////////////

  static uint64_t const defaultChunkSize;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief maximum chunk size
  //////////////////////////////////////////////////////////////////////////////

  static uint64_t const maxChunkSize;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief condition locker to wake up holdReadLockCollection jobs
  //////////////////////////////////////////////////////////////////////////////

  static arangodb::basics::ConditionVariable _condVar;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief global set of ids of holdReadLockCollection jobs, an
  /// id mapping to false here indicates that a request to get the
  /// read lock has been started, the bool is changed to true once
  /// this read lock is acquired. To cancel the read lock, remove
  /// the entry here (under the protection of the mutex of
  /// condVar) and send a broadcast to the condition variable, 
  /// the job with that id is terminated. If it timeouts, then
  /// the read lock is released automatically and the entry here
  /// is deleted.
  //////////////////////////////////////////////////////////////////////////////

  static std::unordered_map<std::string, bool> _holdReadLockJobs;

};
}

#endif
