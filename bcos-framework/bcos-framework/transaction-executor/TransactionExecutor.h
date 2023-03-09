#pragma once

#include "../protocol/BlockHeader.h"
#include "../protocol/Transaction.h"
#include "../protocol/TransactionReceipt.h"
#include "../protocol/TransactionReceiptFactory.h"
#include "../storage/Entry.h"
#include "../storage2/StringPool.h"
#include <bcos-concepts/ByteBuffer.h>
#include <bcos-task/Trait.h>
#include <boost/container/small_vector.hpp>
#include <compare>
#include <type_traits>

namespace bcos::transaction_executor
{
using TableNamePool = storage2::string_pool::FixedStringPool;
using TableNameID = storage2::string_pool::StringID;

class SmallKey : public boost::container::small_vector<char, 32>
{
public:
    using boost::container::small_vector<char, 32>::small_vector;

    explicit SmallKey(concepts::bytebuffer::ByteBuffer auto const& buffer)
      : boost::container::small_vector<char, 32>::small_vector::small_vector(
            RANGES::begin(buffer), RANGES::end(buffer))
    {}
    SmallKey& operator=(concepts::bytebuffer::ByteBuffer auto const& buffer)
    {
        this->assign(RANGES::begin(buffer), RANGES::end(buffer));
        return *this;
    }
    bool operator==(concepts::bytebuffer::ByteBuffer auto const& buffer)
    {
        auto view = concepts::bytebuffer::toView(buffer);
        return toStringView() == view;
    }
    auto operator<=>(concepts::bytebuffer::ByteBuffer auto const& buffer) const
    {
        auto view = concepts::bytebuffer::toView(buffer);
        return toStringView() <=> view;
    }

    std::string_view toStringView() const& { return {this->data(), this->size()}; }
    std::string_view operator()() const& { return toStringView(); }
};

using StateKey = std::tuple<TableNameID, SmallKey>;
using StateValue = storage::Entry;

template <class StorageType>
concept StateStorage = requires()
{
    requires storage2::ReadableStorage<StorageType>;
    requires storage2::WriteableStorage<StorageType>;
};

template <class TransactionExecutorType, class Storage, class ReceiptFactory>
concept TransactionExecutor = requires(TransactionExecutorType executor,
    const protocol::BlockHeader& blockHeader, Storage& storage, ReceiptFactory& receiptFactory)
{
    requires StateStorage<Storage>;
    requires protocol::IsTransactionReceiptFactory<ReceiptFactory>;

    // TransactionExecutorType::TransactionExecutorType(storage, receiptFactory);
    requires std::same_as<task::AwaitableReturnType<decltype(executor.execute(
                              blockHeader, std::declval<protocol::Transaction>(), 0))>,
        protocol::ReceiptFactoryReturnType<ReceiptFactory>>;
};
}  // namespace bcos::transaction_executor

template <bcos::concepts::bytebuffer::ByteBuffer Buffer>
inline std::weak_ordering operator<=>(const bcos::transaction_executor::StateKey& stateKey,
    std::tuple<bcos::transaction_executor::TableNameID, Buffer> const& rhs)
{
    auto const& [lhsTable, lhsKey] = stateKey;
    auto lhsKeyView = lhsKey.toStringView();

    auto const& [rhsTable, rhsKey] = rhs;
    auto rhsKeyView = bcos::concepts::bytebuffer::toView(rhsKey);
    auto tableComp = lhsTable <=> rhsTable;
    if (tableComp != std::weak_ordering::equivalent)
    {
        return lhsKeyView <=> rhsKeyView;
    }
    return tableComp;
}

template <bcos::concepts::bytebuffer::ByteBuffer Buffer>
inline bool operator==(const bcos::transaction_executor::StateKey& stateKey,
    std::tuple<bcos::transaction_executor::TableNameID, Buffer> const& rhs)
{
    auto const& [lhsTable, lhsKey] = stateKey;
    auto lhsKeyView = lhsKey.toStringView();

    auto const& [rhsTable, rhsKey] = rhs;
    auto rhsKeyView = bcos::concepts::bytebuffer::toView(rhsKey);

    if (lhsTable == rhsTable)
    {
        return lhsKeyView == rhsKeyView;
    }
    return false;
}

template <>
struct std::less<bcos::transaction_executor::StateKey>
{
    bool operator()(const bcos::transaction_executor::StateKey& lhs,
        const bcos::transaction_executor::StateKey& rhs) const
    {
        return lhs < rhs;
    }

    template <bcos::concepts::bytebuffer::ByteBuffer Buffer>
    bool operator()(const bcos::transaction_executor::StateKey& lhs,
        std::tuple<bcos::transaction_executor::TableNameID, Buffer> const& rhs) const
    {
        return lhs < rhs;
    }

    template <bcos::concepts::bytebuffer::ByteBuffer Buffer>
    bool operator()(std::tuple<bcos::transaction_executor::TableNameID, Buffer> const& lhs,
        const bcos::transaction_executor::StateKey& rhs) const
    {
        return lhs < rhs;
    }
};

template <>
struct std::hash<bcos::transaction_executor::StateKey>
{
    size_t operator()(const bcos::transaction_executor::StateKey& stateKey) const
    {
        auto const& [table, key] = stateKey;
        size_t hash = 0;
        boost::hash_combine(hash, std::hash<bcos::transaction_executor::TableNameID>{}(table));
        boost::hash_combine(hash, std::hash<std::string_view>{}(key.toStringView()));
        return hash;
    }

    template <bcos::concepts::bytebuffer::ByteBuffer Buffer>
    size_t operator()(
        const std::tuple<bcos::transaction_executor::TableNameID, Buffer>& stateKey) const
    {
        auto const& [table, key] = stateKey;
        size_t hash = 0;
        boost::hash_combine(hash, std::hash<bcos::transaction_executor::TableNameID>{}(table));
        boost::hash_combine(
            hash, std::hash<std::string_view>{}(bcos::concepts::bytebuffer::toView(key)));
        return hash;
    }
};

template <>
struct boost::hash<bcos::transaction_executor::StateKey>
{
    size_t operator()(const bcos::transaction_executor::StateKey& stateKey) const
    {
        return std::hash<bcos::transaction_executor::StateKey>{}(stateKey);
    }
};

inline std::ostream& operator<<(
    std::ostream& stream, bcos::transaction_executor::SmallKey const& smallKey)
{
    stream << smallKey.toStringView();
    return stream;
}