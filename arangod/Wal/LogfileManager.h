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

#ifndef ARANGOD_WAL_LOGFILE_MANAGER_H
#define ARANGOD_WAL_LOGFILE_MANAGER_H 1

#include "Basics/Common.h"

#include "Basics/Mutex.h"
#include "Basics/ReadWriteLock.h"
#include "VocBase/voc-types.h"
#include "Wal/Logfile.h"
#include "Wal/Marker.h"
#include "Wal/Slots.h"

struct TRI_server_t;

namespace arangodb {
namespace options {
class ProgramOptions;
}

namespace wal {

class AllocatorThread;
class CollectorThread;
struct RecoverState;
class RemoverThread;
class Slot;
class SynchronizerThread;

struct LogfileRange {
  LogfileRange(Logfile::IdType id, std::string const& filename,
               std::string const& state, TRI_voc_tick_t tickMin,
               TRI_voc_tick_t tickMax)
      : id(id),
        filename(filename),
        state(state),
        tickMin(tickMin),
        tickMax(tickMax) {}

  Logfile::IdType id;
  std::string filename;
  std::string state;
  TRI_voc_tick_t tickMin;
  TRI_voc_tick_t tickMax;
};

typedef std::vector<LogfileRange> LogfileRanges;

struct LogfileManagerState {
  TRI_voc_tick_t lastAssignedTick;
  TRI_voc_tick_t lastCommittedTick;
  TRI_voc_tick_t lastCommittedDataTick;
  uint64_t numEvents;
  std::string timeString;
};

struct LogfileBarrier {
  LogfileBarrier() = delete;

  LogfileBarrier(TRI_voc_tick_t id, double expires, TRI_voc_tick_t minTick)
      : id(id), expires(expires), minTick(minTick) {}

  TRI_voc_tick_t const id;
  double expires;
  TRI_voc_tick_t minTick;
};

class LogfileManager {
  friend class AllocatorThread;
  friend class CollectorThread;

  LogfileManager(LogfileManager const&) = delete;
  LogfileManager& operator=(LogfileManager const&) = delete;

 public:
  LogfileManager(TRI_server_t*, std::string*);

  /// @brief destroy the logfile manager
  ~LogfileManager();

  /// @brief get the logfile manager instance
  static LogfileManager* instance();

  /// @brief initialize the logfile manager instance
  static void initialize(std::string*, TRI_server_t*);

 public:
  static void collectOptions(std::shared_ptr<options::ProgramOptions> options);
  bool prepare();
  bool open();
  bool start();
  void stop();

 public:
  /// @brief get the logfile directory
  inline std::string directory() const { return _directory; }

  /// @brief get the logfile size
  inline uint32_t filesize() const { return _filesize; }

  /// @brief set the logfile size
  inline void filesize(uint32_t value) { _filesize = value; }

  /// @brief get the sync interval
  inline uint64_t syncInterval() const { return _syncInterval / 1000; }

  /// @brief set the sync interval
  inline void syncInterval(uint64_t value) { _syncInterval = value * 1000; }

  /// @brief get the number of reserve logfiles
  inline uint32_t reserveLogfiles() const { return _reserveLogfiles; }

  /// @brief set the number of reserve logfiles
  inline void reserveLogfiles(uint32_t value) { _reserveLogfiles = value; }

  /// @brief get the number of historic logfiles to keep
  inline uint32_t historicLogfiles() const { return _historicLogfiles; }

  /// @brief set the number of historic logfiles
  inline void historicLogfiles(uint32_t value) { _historicLogfiles = value; }

  /// @brief whether or not there was a SHUTDOWN file with a tick value
  /// at server start
  inline bool hasFoundLastTick() const { return _hasFoundLastTick; }

  /// @brief whether or not we are in the recovery phase
  inline bool isInRecovery() const { return _inRecovery; }

  /// @brief whether or not we are in the shutdown phase
  inline bool isInShutdown() const { return (_shutdown != 0); }

  /// @brief return the slots manager
  Slots* slots() { return _slots; }

  /// @brief whether or not oversize entries are allowed
  inline bool allowOversizeEntries() const { return _allowOversizeEntries; }

  /// @brief sets the "allowOversizeEntries" value
  inline void allowOversizeEntries(bool value) {
    _allowOversizeEntries = value;
  }

  /// @brief whether or not write-throttling can be enabled
  inline bool canBeThrottled() const { return (_throttleWhenPending > 0); }

  /// @brief maximum wait time when write-throttled (in milliseconds)
  inline uint64_t maxThrottleWait() const { return _maxThrottleWait; }

  /// @brief maximum wait time when write-throttled (in milliseconds)
  inline void maxThrottleWait(uint64_t value) { _maxThrottleWait = value; }

  /// @brief whether or not write-throttling is currently enabled
  inline bool isThrottled() { return (_writeThrottled != 0); }

  /// @brief activate write-throttling
  void activateWriteThrottling() { _writeThrottled = 1; }

  /// @brief deactivate write-throttling
  void deactivateWriteThrottling() { _writeThrottled = 0; }

  /// @brief allow or disallow writes to the WAL
  inline void allowWrites(bool value) { _allowWrites = value; }

  /// @brief get the value of --wal.throttle-when-pending
  inline uint64_t throttleWhenPending() const { return _throttleWhenPending; }

  /// @brief set the value of --wal.throttle-when-pending
  inline void throttleWhenPending(uint64_t value) {
    _throttleWhenPending = value;

    if (_throttleWhenPending == 0) {
      deactivateWriteThrottling();
    }
  }

  /// @brief registers a transaction
  int registerTransaction(TRI_voc_tid_t);

  /// @brief unregisters a transaction
  void unregisterTransaction(TRI_voc_tid_t, bool);

  /// @brief return the set of failed transactions
  std::unordered_set<TRI_voc_tid_t> getFailedTransactions();

  /// @brief return the set of dropped collections
  /// this is used during recovery and not used afterwards
  std::unordered_set<TRI_voc_cid_t> getDroppedCollections();

  /// @brief return the set of dropped databases
  /// this is used during recovery and not used afterwards
  std::unordered_set<TRI_voc_tick_t> getDroppedDatabases();

  /// @brief unregister a list of failed transactions
  void unregisterFailedTransactions(std::unordered_set<TRI_voc_tid_t> const&);

  /// @brief whether or not it is currently allowed to create an additional
  /// logfile
  bool logfileCreationAllowed(uint32_t);

  /// @brief whether or not there are reserve logfiles
  bool hasReserveLogfiles();

  /// @brief signal that a sync operation is required
  void signalSync(bool);

  /// @brief reserve space in a logfile
  SlotInfo allocate(uint32_t);
  
  /// @brief reserve space in a logfile
  SlotInfo allocate(TRI_voc_tick_t, TRI_voc_cid_t, uint32_t);

  /// @brief finalize a log entry
  void finalize(SlotInfo&, bool);
  
  /// @brief write data into the logfile, using database id and collection id
  /// this is a convenience function that combines allocate, memcpy and finalize
  SlotInfoCopy allocateAndWrite(TRI_voc_tick_t, TRI_voc_cid_t, void*, uint32_t, bool);

  /// @brief write data into the logfile
  /// this is a convenience function that combines allocate, memcpy and finalize
  SlotInfoCopy allocateAndWrite(void*, uint32_t, bool);

  /// @brief write data into the logfile
  /// this is a convenience function that combines allocate, memcpy and finalize
  SlotInfoCopy allocateAndWrite(Marker const&, bool);

  /// @brief wait for the collector queue to get cleared for the given
  /// collection
  int waitForCollectorQueue(TRI_voc_cid_t, double);

  /// @brief finalize and seal the currently open logfile
  /// this is useful to ensure that any open writes up to this point have made
  /// it into a logfile
  int flush(bool, bool, bool);
  
  /// wait until all changes to the current logfile are synced
  bool waitForSync(double);

  /// @brief re-inserts a logfile back into the inventory only
  void relinkLogfile(Logfile*);

  /// @brief remove a logfile from the inventory only
  bool unlinkLogfile(Logfile*);

  /// @brief remove a logfile from the inventory only
  Logfile* unlinkLogfile(Logfile::IdType);

  /// @brief removes logfiles that are allowed to be removed
  bool removeLogfiles();

  /// @brief sets the status of a logfile to open
  void setLogfileOpen(Logfile*);

  /// @brief sets the status of a logfile to seal-requested
  void setLogfileSealRequested(Logfile*);

  /// @brief sets the status of a logfile to sealed
  void setLogfileSealed(Logfile*);

  /// @brief sets the status of a logfile to sealed
  void setLogfileSealed(Logfile::IdType);

  /// @brief return the status of a logfile
  Logfile::StatusType getLogfileStatus(Logfile::IdType);

  /// @brief return the file descriptor of a logfile
  int getLogfileDescriptor(Logfile::IdType);

  /// @brief get the current open region of a logfile
  /// this uses the slots lock
  void getActiveLogfileRegion(Logfile*, char const*&, char const*&);

  /// @brief garbage collect expires logfile barriers
  void collectLogfileBarriers();

  /// @brief returns a list of all logfile barrier ids
  std::vector<TRI_voc_tick_t> getLogfileBarriers();

  /// @brief remove a specific logfile barrier
  bool removeLogfileBarrier(TRI_voc_tick_t);

  /// @brief adds a barrier that prevents removal of logfiles
  TRI_voc_tick_t addLogfileBarrier(TRI_voc_tick_t, double);

  /// @brief extend the lifetime of a logfile barrier
  bool extendLogfileBarrier(TRI_voc_tick_t, double, TRI_voc_tick_t);

  /// @brief get minimum tick value from all logfile barriers
  TRI_voc_tick_t getMinBarrierTick();

  /// @brief get logfiles for a tick range
  std::vector<Logfile*> getLogfilesForTickRange(TRI_voc_tick_t, TRI_voc_tick_t,
                                                bool& minTickIncluded);

  /// @brief return logfiles for a tick range
  void returnLogfiles(std::vector<Logfile*> const&);

  /// @brief get a logfile by id
  Logfile* getLogfile(Logfile::IdType);

  /// @brief get a logfile and its status by id
  Logfile* getLogfile(Logfile::IdType, Logfile::StatusType&);

  /// @brief get a logfile for writing. this may return nullptr
  int getWriteableLogfile(uint32_t, Logfile::StatusType&, Logfile*&);

  /// @brief get a logfile to collect. this may return nullptr
  Logfile* getCollectableLogfile();

  /// @brief get a logfile to remove. this may return nullptr
  /// if it returns a logfile, the logfile is removed from the list of available
  /// logfiles
  Logfile* getRemovableLogfile();

  /// @brief increase the number of collect operations for a logfile
  void increaseCollectQueueSize(Logfile*);

  /// @brief decrease the number of collect operations for a logfile
  void decreaseCollectQueueSize(Logfile*);

  /// @brief mark a file as being requested for collection
  void setCollectionRequested(Logfile*);

  /// @brief mark a file as being done with collection
  void setCollectionDone(Logfile*);

  /// @brief force the status of a specific logfile
  void forceStatus(Logfile*, Logfile::StatusType);

  /// @brief return the current state
  LogfileManagerState state();

  /// @brief return the current available logfile ranges
  LogfileRanges ranges();

  /// @brief get information about running transactions
  std::tuple<size_t, Logfile::IdType, Logfile::IdType> runningTransactions();

 private:
  /// @brief remove a logfile in the file system
  void removeLogfile(Logfile*);

  /// @brief wait for the collector thread to collect a specific logfile
  int waitForCollector(Logfile::IdType, double);

  /// @brief run the recovery procedure
  /// this is called after the logfiles have been scanned completely and
  /// recovery state has been build. additionally, all databases have been
  /// opened already so we can use collections
  int runRecovery();

  /// @brief closes all logfiles
  void closeLogfiles();

  /// @brief reads the shutdown information
  int readShutdownInfo();

  /// @brief writes the shutdown information
  int writeShutdownInfo(bool);

  /// @brief start the synchronizer thread
  int startSynchronizerThread();

  /// @brief stop the synchronizer thread
  void stopSynchronizerThread();

  /// @brief start the allocator thread
  int startAllocatorThread();

  /// @brief stop the allocator thread
  void stopAllocatorThread();

  /// @brief start the collector thread
  int startCollectorThread();

  /// @brief stop the collector thread
  void stopCollectorThread();

  /// @brief start the remover thread
  int startRemoverThread();

  /// @brief stop the remover thread
  void stopRemoverThread();

  /// @brief check which logfiles are present in the log directory
  int inventory();

  /// @brief inspect all found WAL logfiles
  /// this searches for the max tick in the logfiles and builds up the initial
  /// transaction state
  int inspectLogfiles();

  /// @brief allocate a new reserve logfile
  int createReserveLogfile(uint32_t);

  /// @brief get an id for the next logfile
  Logfile::IdType nextId();

  /// @brief ensure the wal logfiles directory is actually there
  int ensureDirectory();

  /// @brief return the absolute name of the shutdown file
  std::string shutdownFilename() const;

  /// @brief return an absolute filename for a logfile id
  std::string logfileName(Logfile::IdType) const;

  /// @brief return the current time as a string
  static std::string getTimeString();

 private:
  /// @brief pointer to the server
  TRI_server_t* _server;

  /// @brief the arangod config variable containing the database path
  std::string* _databasePath;

  /// @brief state during recovery
  RecoverState* _recoverState;

  /// @brief maximum number of parallel open logfiles


  /// @brief maximum wait time for write-throttling

  //YYY #warning JAN this should be non-static, but the singleton cannot be created before 'start'
  static bool _allowOversizeEntries;
  static std::string _directory;
  static uint32_t _historicLogfiles;
  static bool _ignoreLogfileErrors;
  static bool _ignoreRecoveryErrors;
  static uint32_t _filesize;
  static uint32_t _maxOpenLogfiles;
  static uint32_t _reserveLogfiles;
  static uint32_t _numberOfSlots;
  static uint64_t _syncInterval;
  static uint64_t _throttleWhenPending;
  static uint64_t _maxThrottleWait;

  /// @brief whether or not writes to the WAL are allowed
  bool _allowWrites;

  /// @brief this is true if there was a SHUTDOWN file with a last tick at
  /// server start
  bool _hasFoundLastTick;

  /// @brief whether or not the recovery procedure is running
  bool _inRecovery;

  /// @brief whether or not the logfile manager was properly initialized and
  /// started
  bool _startCalled;

  /// @brief a lock protecting the _logfiles map and the logfiles' statuses
  basics::ReadWriteLock _logfilesLock;

  /// @brief the logfiles
  std::map<Logfile::IdType, Logfile*> _logfiles;

  /// @brief the slots manager
  Slots* _slots;

  /// @brief the synchronizer thread
  SynchronizerThread* _synchronizerThread;

  /// @brief the allocator thread
  AllocatorThread* _allocatorThread;

  /// @brief the collector thread
  CollectorThread* _collectorThread;

  /// @brief the logfile remover thread
  RemoverThread* _removerThread;

  /// @brief last opened logfile id. note: writing to this variable is protected
  /// by the _idLock
  std::atomic<Logfile::IdType> _lastOpenedId;

  /// @brief last fully collected logfile id. note: writing to this variable is
  /// protected by the_idLock
  std::atomic<Logfile::IdType> _lastCollectedId;

  /// @brief last fully sealed logfile id. note: writing to this variable is
  /// protected by the _idLock
  std::atomic<Logfile::IdType> _lastSealedId;

  /// @brief a lock protecting the shutdown file
  Mutex _shutdownFileLock;

  /// @brief a lock protecting _transactions and _failedTransactions
  basics::ReadWriteLock _transactionsLock;

  /// @brief currently ongoing transactions
  std::unordered_map<TRI_voc_tid_t, std::pair<Logfile::IdType, Logfile::IdType>>
      _transactions;

  /// @brief set of failed transactions
  std::unordered_set<TRI_voc_tid_t> _failedTransactions;

  /// @brief set of dropped collections
  /// this is populated during recovery and not used afterwards
  std::unordered_set<TRI_voc_cid_t> _droppedCollections;

  /// @brief set of dropped databases
  /// this is populated during recovery and not used afterwards
  std::unordered_set<TRI_voc_tick_t> _droppedDatabases;

  /// @brief a lock protecting the updates of _lastCollectedId, _lastSealedId,
  /// and _lastOpenedId
  Mutex _idLock;

  /// @brief whether or not write-throttling is currently enabled
  int _writeThrottled;

  /// @brief whether or not we have been shut down already
  volatile sig_atomic_t _shutdown;

  /// @brief a lock protecting _barriers
  basics::ReadWriteLock _barriersLock;

  /// @brief barriers that prevent WAL logfiles from being collected
  std::unordered_map<TRI_voc_tick_t, LogfileBarrier*> _barriers;
};
}
}

#endif
