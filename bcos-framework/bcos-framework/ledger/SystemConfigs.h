/**
 *  Copyright (C) 2024 FISCO BCOS.
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
 * @file SystemConfigs.h
 * @author: kyonGuo
 * @date 2024/5/22
 */

#pragma once
#include "../protocol/Protocol.h"
#include "../storage/Entry.h"
#include "../storage/LegacyStorageMethods.h"
#include "../storage2/Storage.h"
#include "../transaction-executor/StateKey.h"
#include "bcos-framework/ledger/LedgerTypeDef.h"
#include "bcos-task/Task.h"
#include "bcos-tool/Exceptions.h"
#include "bcos-utilities/Exceptions.h"
#include "bcos-utilities/Ranges.h"
#include <boost/throw_exception.hpp>
#include <magic_enum.hpp>
#include <utility>

namespace bcos::ledger
{
struct NoSuchSystemConfig : public bcos::error::Exception
{
};
/// IMPORTANT!!
/// DO NOT change the name of enum. It is used to get the name of the config.
enum class SystemConfig
{
    tx_gas_limit,
    tx_gas_price,
    tx_count_limit,
    consensus_leader_period,
    auth_check_status,
    compatibility_version,
    feature_rpbft,
    feature_rpbft_epoch_block_num,
    feature_rpbft_epoch_sealer_num,
    feature_balance_precompiled,
    web3_chain_id,
    balance_transfer,
};
class SystemConfigs
{
public:
    using ConfigPair = std::pair<std::string, protocol::BlockNumber>;
    SystemConfigs() { m_sysConfigs.reserve(magic_enum::enum_count<SystemConfig>()); }

    static SystemConfig fromString(std::string_view str)
    {
        auto const value = magic_enum::enum_cast<SystemConfig>(str);
        if (!value)
        {
            BOOST_THROW_EXCEPTION(NoSuchSystemConfig{});
        }
        return *value;
    }

    std::optional<ConfigPair> get(SystemConfig config) const
    {
        if (const auto it = m_sysConfigs.find(config); it != m_sysConfigs.end())
        {
            return it->second;
        }
        else
        {
            return std::nullopt;
        }
    }

    ConfigPair getOrDefault(SystemConfig config, std::string defaultValue) const
    {
        return get(config).value_or(ConfigPair{std::move(defaultValue), 0});
    }

    std::optional<ConfigPair> get(std::string_view config) const { return get(fromString(config)); }

    void set(SystemConfig config, std::string value, protocol::BlockNumber number)
    {
        m_sysConfigs[config] = {std::move(value), number};
    }
    void set(std::string_view config, std::string value, protocol::BlockNumber number)
    {
        set(fromString(config), std::move(value), number);
    }

    auto systemConfigs() const
    {
        return RANGES::views::iota(0LU, m_sysConfigs.size()) |
               RANGES::views::transform([this](size_t index) {
                   auto flag = magic_enum::enum_value<SystemConfig>(index);
                   return std::make_tuple(flag, magic_enum::enum_name(flag), m_sysConfigs.at(flag));
               });
    }

    // return all config names
    static auto supportConfigs()
    {
        return RANGES::views::iota(0LU, magic_enum::enum_count<SystemConfig>()) |
               RANGES::views::transform([](size_t index) {
                   auto flag = magic_enum::enum_value<SystemConfig>(index);
                   return magic_enum::enum_name(flag);
               });
    }

private:
    std::unordered_map<SystemConfig, std::optional<ConfigPair>> m_sysConfigs;
};

}  // namespace bcos::ledger
