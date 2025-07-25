#pragma once
#include "../ledger/LedgerTypeDef.h"
#include "../protocol/Protocol.h"
#include "../storage/Entry.h"
#include "../storage2/Storage.h"
#include "../transaction-executor/StateKey.h"
#include "bcos-task/Task.h"
#include "bcos-tool/Exceptions.h"
#include <boost/throw_exception.hpp>
#include <array>
#include <bitset>
#include <magic_enum.hpp>
namespace bcos::ledger
{
DERIVE_BCOS_EXCEPTION(NoSuchFeatureError);
class Features
{
public:
    // Use for storage key, do not change the enum name!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // At most 256 flag
    enum class Flag
    {
        bugfix_revert,  // https://github.com/FISCO-BCOS/FISCO-BCOS/issues/3629
        bugfix_statestorage_hash,
        bugfix_evm_create2_delegatecall_staticcall_codecopy,
        bugfix_event_log_order,
        bugfix_call_noaddr_return,
        bugfix_precompiled_codehash,
        bugfix_dmc_revert,
        bugfix_keypage_system_entry_hash,
        bugfix_internal_create_redundant_storage,  // to perf internal create code and abi storage
        bugfix_internal_create_permission_denied,
        bugfix_sharding_call_in_child_executive,
        bugfix_empty_abi_reset,  // support empty abi reset of same code
        bugfix_eip55_addr,
        bugfix_eoa_as_contract,
        bugfix_eoa_match_failed,
        bugfix_evm_exception_gas_used,
        bugfix_dmc_deploy_gas_used,
        bugfix_staticcall_noaddr_return,
        bugfix_support_transfer_receive_fallback,
        bugfix_set_row_with_dirty_flag,
        bugfix_rpbft_vrf_blocknumber_input,
        bugfix_delete_account_code,
        bugfix_policy1_empty_code_address,
        bugfix_precompiled_gasused,
        bugfix_nonce_not_increase_when_revert,
        bugfix_set_contract_nonce_when_create,
        bugfix_precompiled_gascalc,
        bugfix_method_auth_sender,
        bugfix_precompiled_evm_status,
        feature_dmc2serial,
        feature_sharding,
        feature_rpbft,
        feature_paillier,
        feature_balance,
        feature_balance_precompiled,
        feature_balance_policy1,
        feature_paillier_add_raw,
        feature_evm_cancun,
        feature_evm_timestamp,
        feature_evm_address,
        feature_rpbft_term_weight,
        feature_raw_address,
        feature_rpbft_vrf_type_secp256k1,
        feature_balance_policy2,  // 转账白名单 Transfer whitelist
    };

private:
    std::bitset<magic_enum::enum_count<Flag>()> m_flags;

public:
    static Flag string2Flag(std::string_view str)
    {
        auto value = magic_enum::enum_cast<Flag>(str);
        if (!value)
        {
            BOOST_THROW_EXCEPTION(NoSuchFeatureError{});
        }
        return *value;
    }

    static bool contains(std::string_view flag)
    {
        return magic_enum::enum_cast<Flag>(flag).has_value();
    }

    void validate(std::string_view flag) const
    {
        auto value = magic_enum::enum_cast<Flag>(flag);
        if (!value)
        {
            BOOST_THROW_EXCEPTION(NoSuchFeatureError{});
        }

        validate(*value);
    }

    void validate(Flag flag) const
    {
        if (flag == Flag::feature_balance_precompiled && !get(Flag::feature_balance))
        {
            BOOST_THROW_EXCEPTION(bcos::tool::InvalidSetFeature{}
                                  << errinfo_comment("must set feature_balance first"));
        }
        if (flag == Flag::feature_balance_policy1 && !get(Flag::feature_balance_precompiled))
        {
            BOOST_THROW_EXCEPTION(bcos::tool::InvalidSetFeature{}
                                  << errinfo_comment("must set feature_balance_precompiled first"));
        }
    }

    bool get(Flag flag) const
    {
        auto index = magic_enum::enum_index(flag);
        if (!index)
        {
            BOOST_THROW_EXCEPTION(NoSuchFeatureError{});
        }

        return m_flags[*index];
    }
    bool get(std::string_view flag) const { return get(string2Flag(flag)); }

    // DO NOT use now, there is some action after set feature in systemPrecompiled
    static auto getFeatureDependencies(Flag flag)
    {
        /// NOTE：请不要在此处添加旧版本feature依赖！否则会出现数据不一致!
        /// Do NOT add old version feature dependencies here! Otherwise, data inconsistency will
        /// occur!
        // const auto mainSwitchDependence = std::unordered_map<Flag, std::set<Flag>>(
        //     {{Flag::feature_ethereum_compatible, {
        //                                              Flag::feature_balance,
        //                                              Flag::feature_balance_precompiled,
        //                                              Flag::feature_calculate_gasPrice,
        //                                              Flag::feature_evm_timestamp,
        //                                              Flag::feature_evm_address,
        //                                              Flag::feature_evm_cancun,
        //                                          }}});
        // if (mainSwitchDependence.contains(flag))
        // {
        //     return mainSwitchDependence.at(flag);
        // }
        return std::set<Flag>();
    }
    void enableDependencyFeatures(Flag flag)
    {
        for (const auto& dependence : getFeatureDependencies(flag))
        {
            set(dependence);
        }
    }

    void set(Flag flag)
    {
        auto index = magic_enum::enum_index(flag);
        if (!index)
        {
            BOOST_THROW_EXCEPTION(NoSuchFeatureError{});
        }

        m_flags[*index] = true;
        // enableDependencyFeatures(flag);
    }

    void set(std::string_view flag) { set(string2Flag(flag)); }

    void setToShardingDefault(protocol::BlockVersion version)
    {
        if (version >= protocol::BlockVersion::V3_3_VERSION &&
            version <= protocol::BlockVersion::V3_4_VERSION)
        {
            set(Flag::feature_sharding);
        }
    }

    void setUpgradeFeatures(protocol::BlockVersion fromVersion, protocol::BlockVersion toVersion)
    {
        struct UpgradeFeatures
        {
            protocol::BlockVersion to;
            std::vector<Flag> flags;
        };
        const static auto upgradeRoadmap =
            std::to_array<UpgradeFeatures>({{.to = protocol::BlockVersion::V3_2_3_VERSION,
                                                .flags =
                                                    {
                                                        Flag::bugfix_revert,
                                                    }},
                {.to = protocol::BlockVersion::V3_2_4_VERSION,
                    .flags =
                        {
                            Flag::bugfix_statestorage_hash,
                            Flag::bugfix_evm_create2_delegatecall_staticcall_codecopy,
                        }},
                {.to = protocol::BlockVersion::V3_2_7_VERSION,
                    .flags =
                        {
                            Flag::bugfix_event_log_order,
                            Flag::bugfix_call_noaddr_return,
                            Flag::bugfix_precompiled_codehash,
                            Flag::bugfix_dmc_revert,
                        }},
                {.to = protocol::BlockVersion::V3_5_VERSION,
                    .flags =
                        {
                            Flag::bugfix_revert,
                            Flag::bugfix_statestorage_hash,
                        }},
                {.to = protocol::BlockVersion::V3_6_VERSION,
                    .flags =
                        {
                            Flag::bugfix_statestorage_hash,
                            Flag::bugfix_evm_create2_delegatecall_staticcall_codecopy,
                            Flag::bugfix_event_log_order,
                            Flag::bugfix_call_noaddr_return,
                            Flag::bugfix_precompiled_codehash,
                            Flag::bugfix_dmc_revert,
                        }},
                {.to = protocol::BlockVersion::V3_6_1_VERSION,
                    .flags =
                        {
                            Flag::bugfix_keypage_system_entry_hash,
                            Flag::bugfix_internal_create_redundant_storage,
                        }},
                {.to = protocol::BlockVersion::V3_7_0_VERSION,
                    .flags =
                        {
                            Flag::bugfix_empty_abi_reset,
                            Flag::bugfix_eip55_addr,
                            Flag::bugfix_sharding_call_in_child_executive,
                            Flag::bugfix_internal_create_permission_denied,
                        }},
                {.to = protocol::BlockVersion::V3_8_0_VERSION,
                    .flags =
                        {
                            Flag::bugfix_eoa_as_contract,
                            Flag::bugfix_dmc_deploy_gas_used,
                            Flag::bugfix_evm_exception_gas_used,
                            Flag::bugfix_set_row_with_dirty_flag,
                        }},
                {.to = protocol::BlockVersion::V3_9_0_VERSION,
                    .flags =
                        {
                            Flag::bugfix_staticcall_noaddr_return,
                            Flag::bugfix_support_transfer_receive_fallback,
                            Flag::bugfix_eoa_match_failed,
                        }},
                {.to = protocol::BlockVersion::V3_12_0_VERSION,
                    .flags =
                        {
                            Flag::bugfix_rpbft_vrf_blocknumber_input,
                        }},
                {.to = protocol::BlockVersion::V3_13_0_VERSION,
                    .flags =
                        {
                            Flag::bugfix_delete_account_code,
                            Flag::bugfix_policy1_empty_code_address,
                            Flag::bugfix_precompiled_gasused,
                        }},
                {.to = protocol::BlockVersion::V3_14_0_VERSION,
                    .flags =
                        {
                            Flag::bugfix_nonce_not_increase_when_revert,
                            Flag::bugfix_set_contract_nonce_when_create,
                        }},
                {.to = protocol::BlockVersion::V3_15_1_VERSION,
                    .flags = {Flag::bugfix_precompiled_gascalc}},
                {.to = protocol::BlockVersion::V3_15_2_VERSION,
                    .flags = {
                        Flag::bugfix_method_auth_sender,
                        Flag::bugfix_precompiled_evm_status,
                    }}});
        for (const auto& upgradeFeatures : upgradeRoadmap)
        {
            if (((toVersion < protocol::BlockVersion::V3_2_7_VERSION) &&
                    (toVersion >= upgradeFeatures.to)) ||
                (fromVersion < upgradeFeatures.to && toVersion >= upgradeFeatures.to))
            {
                for (auto flag : upgradeFeatures.flags)
                {
                    set(flag);
                }
            }
        }
    }

    void setGenesisFeatures(protocol::BlockVersion toVersion)
    {
        setToShardingDefault(toVersion);
        if (toVersion == protocol::BlockVersion::V3_3_VERSION ||
            toVersion == protocol::BlockVersion::V3_4_VERSION)
        {
            return;
        }

        if (toVersion == protocol::BlockVersion::V3_5_VERSION)
        {
            setUpgradeFeatures(protocol::BlockVersion::V3_4_VERSION, toVersion);
        }
        else
        {
            setUpgradeFeatures(protocol::BlockVersion::MIN_VERSION, toVersion);
        }
    }

    auto flags() const
    {
        return ::ranges::views::iota(0LU, m_flags.size()) |
               ::ranges::views::transform([this](size_t index) {
                   auto flag = magic_enum::enum_value<Flag>(index);
                   return std::make_tuple(flag, magic_enum::enum_name(flag), m_flags[index]);
               });
    }

    static auto featureKeys()
    {
        return ::ranges::views::iota(0LU, magic_enum::enum_count<Flag>()) |
               ::ranges::views::transform([](size_t index) {
                   auto flag = magic_enum::enum_value<Flag>(index);
                   return magic_enum::enum_name(flag);
               });
    }

    task::Task<void> readFromStorage(
        storage2::ReadableStorage<executor_v1::StateKeyView> auto& storage, long blockNumber)
    {
        for (auto key : bcos::ledger::Features::featureKeys())
        {
            auto entry = co_await storage2::readOne(
                storage, executor_v1::StateKeyView(ledger::SYS_CONFIG, key));
            if (entry)
            {
                auto [value, enableNumber] = entry->template getObject<ledger::SystemConfigEntry>();
                if (blockNumber >= enableNumber)
                {
                    set(key);
                }
            }
        }
    }

    task::Task<void> writeToStorage(
        storage2::WritableStorage<executor_v1::StateKey, executor_v1::StateValue> auto& storage,
        long blockNumber, bool ignoreDuplicate = true) const
    {
        for (auto [flag, name, value] : flags())
        {
            if (value &&
                !(ignoreDuplicate && co_await storage2::existsOne(storage,
                                         executor_v1::StateKeyView(ledger::SYS_CONFIG, name))))
            {
                storage::Entry entry;
                entry.setObject(
                    SystemConfigEntry{boost::lexical_cast<std::string>((int)value), blockNumber});
                co_await storage2::writeOne(
                    storage, executor_v1::StateKey(ledger::SYS_CONFIG, name), std::move(entry));
            }
        }
    }
};

inline task::Task<void> readFromStorage(Features& features,
    storage2::ReadableStorage<executor_v1::StateKey> auto& storage, long blockNumber)
{
    decltype(auto) keys = bcos::ledger::Features::featureKeys();
    auto entries = co_await storage2::readSome(std::forward<decltype(storage)>(storage),
        keys | ::ranges::views::transform([](std::string_view key) {
            return executor_v1::StateKeyView(ledger::SYS_CONFIG, key);
        }));
    for (auto&& [key, entry] : ::ranges::views::zip(keys, entries))
    {
        if (entry)
        {
            auto [value, enableNumber] = entry->template getObject<ledger::SystemConfigEntry>();
            if (blockNumber >= enableNumber)
            {
                features.set(key);
            }
        }
    }
}

inline task::Task<void> writeToStorage(Features const& features,
    storage2::WritableStorage<executor_v1::StateKey, executor_v1::StateValue> auto& storage,
    long blockNumber)
{
    decltype(auto) flags =
        features.flags() | ::ranges::views::filter([](auto&& tuple) { return std::get<2>(tuple); });
    co_await storage2::writeSome(std::forward<decltype(storage)>(storage),
        ::ranges::views::transform(flags, [&](auto&& tuple) {
            storage::Entry entry;
            entry.setObject(SystemConfigEntry{
                boost::lexical_cast<std::string>((int)std::get<2>(tuple)), blockNumber});
            return std::make_tuple(
                executor_v1::StateKey(ledger::SYS_CONFIG, std::get<1>(tuple)), std::move(entry));
        }));
}

inline std::ostream& operator<<(std::ostream& stream, Features::Flag flag)
{
    stream << magic_enum::enum_name(flag);
    return stream;
}

}  // namespace bcos::ledger
