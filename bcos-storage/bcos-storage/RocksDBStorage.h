/*
 *  Copyright (C) 2021 FISCO BCOS.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @brief the implement of storage
 * @file Storage.cpp
 * @author: xingqiangbai
 * @date: 2021-04-16
 * @file Storage.cpp
 * @author: ancelmo
 * @date: 2021-08-27
 */
#pragma once

#include <bcos-framework/storage/StorageInterface.h>
#include <bcos-framework/security/StorageEncryptInterface.h>
#include <rocksdb/db.h>
#include <tbb/parallel_for.h>

namespace rocksdb
{
class WriteBatch;
}

namespace bcos::storage
{
class RocksDBStorage : public TransactionalStorageInterface
{
public:
    using Ptr = std::shared_ptr<RocksDBStorage>;
    explicit RocksDBStorage(std::unique_ptr<rocksdb::DB, std::function<void(rocksdb::DB*)>>&& db,
        const bcos::security::StorageEncryptInterface::Ptr dataEncryption);

    ~RocksDBStorage() {}

    void asyncGetPrimaryKeys(std::string_view _table,
        const std::optional<Condition const>& _condition,
        std::function<void(Error::UniquePtr, std::vector<std::string>)> _callback) override;

    // due to the blocking of m_db->Get, this interface is actually a synchronous interface
    void asyncGetRow(std::string_view table, std::string_view _key,
        std::function<void(Error::UniquePtr, std::optional<Entry>)> _callback) override;

    // due to the blocking of m_db->MultiGet, this interface is actually a synchronous interface
    void asyncGetRows(std::string_view table,
        RANGES::any_view<std::string_view,
            RANGES::category::input | RANGES::category::random_access | RANGES::category::sized>
            keys,
        std::function<void(Error::UniquePtr, std::vector<std::optional<Entry>>)> _callback)
        override;

    void asyncSetRow(std::string_view table, std::string_view key, Entry entry,
        std::function<void(Error::UniquePtr)> callback) override;

    void asyncPrepare(const bcos::protocol::TwoPCParams& params,
        const TraverseStorageInterface& storage,
        std::function<void(Error::Ptr, uint64_t, const std::string&)> callback) override;

    void asyncCommit(const bcos::protocol::TwoPCParams& params,
        std::function<void(Error::Ptr, uint64_t)> callback) override;

    void asyncRollback(const bcos::protocol::TwoPCParams& params,
        std::function<void(Error::Ptr)> callback) override;
    Error::Ptr setRows(std::string_view tableName,
        RANGES::any_view<std::string_view,
            RANGES::category::random_access | RANGES::category::sized>
            keys,
        RANGES::any_view<std::string_view,
            RANGES::category::random_access | RANGES::category::sized>
            values) noexcept override;
    virtual Error::Ptr deleteRows(
        std::string_view, const std::variant<const gsl::span<std::string_view const>,
                              const gsl::span<std::string const>>&) noexcept override;

    rocksdb::DB& rocksDB() { return *m_db; }

    void stop() override;

private:
    Error::Ptr checkStatus(rocksdb::Status const& status);
    std::shared_ptr<rocksdb::WriteBatch> m_writeBatch = nullptr;
    std::mutex m_writeBatchMutex;
    std::unique_ptr<rocksdb::DB, std::function<void(rocksdb::DB*)>> m_db;

    // Security Storage
    bcos::security::StorageEncryptInterface::Ptr m_dataEncryption{nullptr};
    bool enableRocksDBMemoryStatistics = false;
};
}  // namespace bcos::storage
