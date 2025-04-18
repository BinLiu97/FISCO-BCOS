/**
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
 * @file Common.h
 * @author: ancelmo
 * @date 2021-04-20
 */

#pragma once
// if windows, manual include tup/Tars.h first
#include <bcos-framework/consensus/ConsensusNode.h>
#ifdef _WIN32
#include <tup/Tars.h>
#endif
#include "bcos-tars-protocol/tars/GatewayInfo.h"
#include "bcos-tars-protocol/tars/TransactionReceipt.h"
#include <bcos-crypto/interfaces/crypto/Hash.h>
#include <bcos-framework/protocol/LogEntry.h>
#if !ONLY_CPP_SDK
#include "bcos-tars-protocol/tars/GroupInfo.h"
#include "bcos-tars-protocol/tars/LedgerConfig.h"
#include "bcos-tars-protocol/tars/TwoPCParams.h"
#include <bcos-crypto/interfaces/crypto/KeyFactory.h>
#include <bcos-framework/gateway/GatewayTypeDef.h>
#include <bcos-framework/ledger/LedgerConfig.h>
#include <bcos-framework/multigroup/ChainNodeInfoFactory.h>
#include <bcos-framework/multigroup/GroupInfoFactory.h>
#endif
#include <bcos-framework/protocol/ProtocolInfo.h>
#include <bcos-utilities/Common.h>
#include <cstdint>
#include <functional>
#include <memory>

namespace bcostars
{
namespace protocol
{
const static bcos::crypto::HashType emptyHash;

template <class Container>
class BufferWriter
{
protected:
    using ByteType = typename Container::value_type;
    using SizeType = typename Container::size_type;

    mutable Container _buffer;
    ByteType* _buf;
    SizeType _len;
    SizeType _buf_len;
    std::function<ByteType*(BufferWriter&, size_t)> _reserve;

private:
    BufferWriter(const BufferWriter&);
    BufferWriter& operator=(const BufferWriter& buf);

public:
    BufferWriter() : _buf(NULL), _len(0), _buf_len(0), _reserve({})
    {
#ifndef GEN_PYTHON_MASK
        _reserve = [](BufferWriter& os, size_t len) {
            os._buffer.resize(len);
            return os._buffer.data();
        };
#endif
    }

    ~BufferWriter() {}

    void reset() { _len = 0; }

    void writeBuf(const ByteType* buf, size_t len)
    {
        TarsReserveBuf(*this, _len + len);
        memcpy(_buf + _len, buf, len);
        _len += len;
    }

    const Container& getByteBuffer() const
    {
        _buffer.resize(_len);
        return _buffer;
    }
    Container& getByteBuffer()
    {
        _buffer.resize(_len);
        return _buffer;
    }
    const ByteType* getBuffer() const { return _buf; }
    size_t getLength() const { return _len; }
    void swap(std::vector<ByteType>& v)
    {
        _buffer.resize(_len);
        v.swap(_buffer);
        _buf = NULL;
        _buf_len = 0;
        _len = 0;
    }
    void swap(BufferWriter& buf)
    {
        buf._buffer.swap(_buffer);
        std::swap(_buf, buf._buf);
        std::swap(_buf_len, buf._buf_len);
        std::swap(_len, buf._len);
    }
};

using BufferWriterByteVector = BufferWriter<std::vector<bcos::byte>>;
using BufferWriterStdByteVector = BufferWriter<std::vector<std::byte>>;
using BufferWriterString = BufferWriter<std::string>;
}  // namespace protocol

#if !ONLY_CPP_SDK

inline bcos::group::ChainNodeInfo::Ptr toBcosChainNodeInfo(
    bcos::group::ChainNodeInfoFactory::Ptr _factory, bcostars::ChainNodeInfo const& _tarsNodeInfo)
{
    auto nodeInfo = _factory->createNodeInfo();
    nodeInfo->setNodeName(_tarsNodeInfo.nodeName);
    nodeInfo->setNodeCryptoType((bcos::group::NodeCryptoType)_tarsNodeInfo.nodeCryptoType);
    nodeInfo->setNodeID(_tarsNodeInfo.nodeID);
    nodeInfo->setIniConfig(_tarsNodeInfo.iniConfig);
    nodeInfo->setMicroService(_tarsNodeInfo.microService);
    nodeInfo->setNodeType((bcos::protocol::NodeType)_tarsNodeInfo.nodeType);
    nodeInfo->setSmCryptoType(_tarsNodeInfo.nodeCryptoType == bcos::group::NodeCryptoType::SM_NODE);
    for (auto const& it : _tarsNodeInfo.serviceInfo)
    {
        nodeInfo->appendServiceInfo((bcos::protocol::ServiceType)it.first, it.second);
    }
    // recover the nodeProtocolVersion
    auto& protocolInfo = _tarsNodeInfo.protocolInfo;
    auto bcosProtocolInfo = std::make_shared<bcos::protocol::ProtocolInfo>(
        (bcos::protocol::ProtocolModuleID)protocolInfo.moduleID,
        (bcos::protocol::ProtocolVersion)protocolInfo.minVersion,
        (bcos::protocol::ProtocolVersion)protocolInfo.maxVersion);
    nodeInfo->setNodeProtocol(std::move(*bcosProtocolInfo));
    // recover system version(data version)
    nodeInfo->setCompatibilityVersion(_tarsNodeInfo.compatibilityVersion);
    return nodeInfo;
}

inline bcos::group::GroupInfo::Ptr toBcosGroupInfo(
    bcos::group::ChainNodeInfoFactory::Ptr _nodeFactory,
    bcos::group::GroupInfoFactory::Ptr _groupFactory, bcostars::GroupInfo const& _tarsGroupInfo)
{
    auto groupInfo = _groupFactory->createGroupInfo();
    groupInfo->setChainID(_tarsGroupInfo.chainID);
    groupInfo->setGroupID(_tarsGroupInfo.groupID);
    groupInfo->setGenesisConfig(_tarsGroupInfo.genesisConfig);
    groupInfo->setIniConfig(_tarsGroupInfo.iniConfig);
    groupInfo->setWasm(_tarsGroupInfo.isWasm != 0);
    bool isFirst = true;
    for (auto const& tarsNodeInfo : _tarsGroupInfo.nodeList)
    {
        auto nodeInfo = toBcosChainNodeInfo(_nodeFactory, tarsNodeInfo);
        if (isFirst)
        {
            groupInfo->setSmCryptoType(nodeInfo->smCryptoType());
            isFirst = false;
        }
        groupInfo->appendNodeInfo(std::move(nodeInfo));
    }
    return groupInfo;
}

inline bcostars::ChainNodeInfo toTarsChainNodeInfo(bcos::group::ChainNodeInfo::Ptr _nodeInfo)
{
    bcostars::ChainNodeInfo tarsNodeInfo;
    if (!_nodeInfo)
    {
        return tarsNodeInfo;
    }
    tarsNodeInfo.nodeName = _nodeInfo->nodeName();
    tarsNodeInfo.nodeCryptoType = _nodeInfo->nodeCryptoType();
    tarsNodeInfo.nodeType = _nodeInfo->nodeType();
    auto const& info = _nodeInfo->serviceInfo();
    for (auto const& it : info)
    {
        tarsNodeInfo.serviceInfo[(int32_t)it.first] = it.second;
    }
    tarsNodeInfo.nodeID = _nodeInfo->nodeID();
    tarsNodeInfo.microService = _nodeInfo->microService();
    tarsNodeInfo.iniConfig = _nodeInfo->iniConfig();
    // set the nodeProtocolVersion
    auto const& protocol = _nodeInfo->nodeProtocol();
    tarsNodeInfo.protocolInfo.moduleID = protocol->protocolModuleID();
    tarsNodeInfo.protocolInfo.minVersion = protocol->minVersion();
    tarsNodeInfo.protocolInfo.maxVersion = protocol->maxVersion();
    // write the compatibilityVersion
    tarsNodeInfo.compatibilityVersion = _nodeInfo->compatibilityVersion();
    return tarsNodeInfo;
}

inline bcostars::GroupInfo toTarsGroupInfo(bcos::group::GroupInfo::Ptr _groupInfo)
{
    bcostars::GroupInfo tarsGroupInfo;
    if (!_groupInfo)
    {
        return tarsGroupInfo;
    }
    tarsGroupInfo.chainID = _groupInfo->chainID();
    tarsGroupInfo.groupID = _groupInfo->groupID();
    tarsGroupInfo.genesisConfig = _groupInfo->genesisConfig();
    tarsGroupInfo.iniConfig = _groupInfo->iniConfig();
    tarsGroupInfo.isWasm = _groupInfo->wasm() ? 1 : 0;
    // set nodeList
    std::vector<bcostars::ChainNodeInfo> tarsNodeList;
    auto bcosNodeList = _groupInfo->nodeInfos();
    for (auto const& it : bcosNodeList)
    {
        auto const& nodeInfo = it.second;
        tarsNodeList.emplace_back(toTarsChainNodeInfo(nodeInfo));
    }
    tarsGroupInfo.nodeList = std::move(tarsNodeList);
    return tarsGroupInfo;
}

inline bcos::consensus::ConsensusNodeList toConsensusNodeList(
    bcos::crypto::KeyFactory::Ptr _keyFactory, bcos::consensus::Type type,
    const vector<bcostars::ConsensusNode>& _tarsConsensusNodeList)
{
    bcos::consensus::ConsensusNodeList consensusNodeList;
    for (auto const& node : _tarsConsensusNodeList)
    {
        auto nodeID = _keyFactory->createKey(
            bcos::bytesConstRef((bcos::byte*)node.nodeID.data(), node.nodeID.size()));
        consensusNodeList.push_back(bcos::consensus::ConsensusNode{.nodeID = nodeID,
            .type = type,
            .voteWeight = static_cast<uint64_t>(node.voteWeight),
            .termWeight = static_cast<uint64_t>(node.termWeight),
            .enableNumber = node.enableNumber});
    }
    return consensusNodeList;
}

inline bcos::ledger::LedgerConfig::Ptr toLedgerConfig(
    bcostars::LedgerConfig const& _ledgerConfig, bcos::crypto::KeyFactory::Ptr _keyFactory)
{
    auto ledgerConfig = std::make_shared<bcos::ledger::LedgerConfig>();
    auto consensusNodeList = toConsensusNodeList(
        _keyFactory, bcos::consensus::Type::consensus_sealer, _ledgerConfig.consensusNodeList);
    ledgerConfig->setConsensusNodeList(consensusNodeList);

    auto observerNodeList = toConsensusNodeList(
        _keyFactory, bcos::consensus::Type::consensus_observer, _ledgerConfig.observerNodeList);
    ledgerConfig->setObserverNodeList(observerNodeList);

    auto hash = bcos::crypto::HashType();
    if (_ledgerConfig.hash.size() >= bcos::crypto::HashType::SIZE)
    {
        hash = bcos::crypto::HashType(
            (const bcos::byte*)_ledgerConfig.hash.data(), bcos::crypto::HashType::SIZE);
    }
    ledgerConfig->setHash(hash);
    ledgerConfig->setBlockNumber(_ledgerConfig.blockNumber);
    ledgerConfig->setBlockTxCountLimit(_ledgerConfig.blockTxCountLimit);
    ledgerConfig->setLeaderSwitchPeriod(_ledgerConfig.leaderSwitchPeriod);
    ledgerConfig->setSealerId(_ledgerConfig.sealerId);
    ledgerConfig->setGasLimit(std::make_tuple(_ledgerConfig.gasLimit, _ledgerConfig.blockNumber));
    ledgerConfig->setCompatibilityVersion(_ledgerConfig.compatibilityVersion);
    return ledgerConfig;
}

inline vector<bcostars::ConsensusNode> toTarsConsensusNodeList(
    bcos::consensus::ConsensusNodeList const& _nodeList)
{
    // set consensusNodeList
    vector<bcostars::ConsensusNode> tarsConsensusNodeList;
    for (auto node : _nodeList)
    {
        bcostars::ConsensusNode consensusNode;
        consensusNode.nodeID.assign(node.nodeID->data().begin(), node.nodeID->data().end());
        consensusNode.voteWeight = node.voteWeight;
        consensusNode.termWeight = node.termWeight;
        tarsConsensusNodeList.emplace_back(consensusNode);
    }
    return tarsConsensusNodeList;
}
inline bcostars::LedgerConfig toTarsLedgerConfig(bcos::ledger::LedgerConfig::Ptr _ledgerConfig)
{
    bcostars::LedgerConfig ledgerConfig;
    if (!_ledgerConfig)
    {
        return ledgerConfig;
    }
    auto hash = _ledgerConfig->hash().asBytes();
    ledgerConfig.hash.assign(hash.begin(), hash.end());
    ledgerConfig.blockNumber = _ledgerConfig->blockNumber();
    ledgerConfig.blockTxCountLimit = _ledgerConfig->blockTxCountLimit();
    ledgerConfig.leaderSwitchPeriod = _ledgerConfig->leaderSwitchPeriod();
    ledgerConfig.sealerId = _ledgerConfig->sealerId();
    ledgerConfig.gasLimit = std::get<0>(_ledgerConfig->gasLimit());
    ledgerConfig.compatibilityVersion = _ledgerConfig->compatibilityVersion();

    // set consensusNodeList
    ledgerConfig.consensusNodeList = toTarsConsensusNodeList(_ledgerConfig->consensusNodeList());
    // set observerNodeList
    ledgerConfig.observerNodeList = toTarsConsensusNodeList(_ledgerConfig->observerNodeList());
    return ledgerConfig;
}

inline bcostars::P2PInfo toTarsP2PInfo(bcos::gateway::P2PInfo const& _p2pInfo)
{
    bcostars::P2PInfo tarsP2PInfo;
    tarsP2PInfo.p2pID = _p2pInfo.p2pID;
    tarsP2PInfo.agencyName = _p2pInfo.agencyName;
    tarsP2PInfo.nodeName = _p2pInfo.nodeName;
    tarsP2PInfo.host = _p2pInfo.nodeIPEndpoint.address();
    tarsP2PInfo.port = _p2pInfo.nodeIPEndpoint.port();
    return tarsP2PInfo;
}

inline bcostars::GroupNodeInfo toTarsNodeIDInfo(std::string const& _groupID,
    std::set<std::string> const& _nodeIDList, std::set<std::uint32_t> const& _nodeTypeList)
{
    GroupNodeInfo groupNodeIDInfo;
    groupNodeIDInfo.groupID = _groupID;
    groupNodeIDInfo.nodeIDList = std::vector<std::string>(_nodeIDList.begin(), _nodeIDList.end());
    groupNodeIDInfo.nodeTypeList = std::vector<int32_t>(_nodeTypeList.begin(), _nodeTypeList.end());
    return groupNodeIDInfo;
}
inline bcostars::GatewayInfo toTarsGatewayInfo(bcos::gateway::GatewayInfo::Ptr _bcosGatewayInfo)
{
    bcostars::GatewayInfo tarsGatewayInfo;
    if (!_bcosGatewayInfo)
    {
        return tarsGatewayInfo;
    }
    tarsGatewayInfo.p2pInfo = toTarsP2PInfo(_bcosGatewayInfo->p2pInfo());
    auto nodeIDList = _bcosGatewayInfo->nodeIDInfo();
    std::vector<GroupNodeInfo> tarsNodeIDInfos;
    for (auto const& it : nodeIDList)
    {
        auto gourpID = it.first;
        auto nodeInfos = it.second;
        std::set<std::string> nodeIDs;
        std::set<uint32_t> nodeTypes;
        for (auto const& nodeInfo : nodeInfos)
        {
            auto nodeID = nodeInfo.first;
            auto nodeType = nodeInfo.second;
            nodeIDs.insert(nodeID);
            nodeTypes.insert(nodeType);
        }
        tarsNodeIDInfos.emplace_back(toTarsNodeIDInfo(it.first, nodeIDs, nodeTypes));
    }
    tarsGatewayInfo.nodeIDInfo = tarsNodeIDInfos;
    return tarsGatewayInfo;
}

// Note: use struct here maybe Inconvenient to override
inline bcos::gateway::P2PInfo toBcosP2PNodeInfo(bcostars::P2PInfo const& _tarsP2pInfo)
{
    bcos::gateway::P2PInfo p2pInfo;
    p2pInfo.p2pID = _tarsP2pInfo.p2pID;
    p2pInfo.agencyName = _tarsP2pInfo.agencyName;
    p2pInfo.nodeName = _tarsP2pInfo.nodeName;
    p2pInfo.nodeIPEndpoint = bcos::gateway::NodeIPEndpoint(_tarsP2pInfo.host, _tarsP2pInfo.port);
    return p2pInfo;
}

inline bcos::gateway::GatewayInfo::Ptr fromTarsGatewayInfo(bcostars::GatewayInfo _tarsGatewayInfo)
{
    auto bcosGatewayInfo = std::make_shared<bcos::gateway::GatewayInfo>();
    auto p2pInfo = toBcosP2PNodeInfo(_tarsGatewayInfo.p2pInfo);
    std::map<std::string, std::map<std::string, uint32_t>> nodeIDInfos;
    for (auto const& it : _tarsGatewayInfo.nodeIDInfo)
    {
        auto const& nodeIDListInfo = it.nodeIDList;
        auto const& nodeTypeInfo = it.nodeTypeList;
        for (size_t i = 0; i < nodeIDListInfo.size(); ++i)
        {
            auto nodeID = nodeIDListInfo[i];
            auto nodeType = nodeTypeInfo[i];
            nodeIDInfos[it.groupID].insert(std::pair<std::string, uint32_t>(nodeID, nodeType));
        }
    }
    bcosGatewayInfo->setP2PInfo(std::move(p2pInfo));
    bcosGatewayInfo->setNodeIDInfo(std::move(nodeIDInfos));
    return bcosGatewayInfo;
}

inline bcos::protocol::TwoPCParams toBcosTwoPCParams(bcostars::TwoPCParams const& _param)
{
    bcos::protocol::TwoPCParams bcosTwoPCParams;
    bcosTwoPCParams.number = _param.blockNumber;
    bcosTwoPCParams.primaryKey = _param.primaryKey;
    bcosTwoPCParams.timestamp = _param.timePoint;
    return bcosTwoPCParams;
}

inline bcostars::TwoPCParams toTarsTwoPCParams(bcos::protocol::TwoPCParams _param)
{
    bcostars::TwoPCParams tarsTwoPCParams;
    tarsTwoPCParams.blockNumber = _param.number;
    tarsTwoPCParams.primaryKey = _param.primaryKey;
    tarsTwoPCParams.timePoint = _param.timestamp;
    return tarsTwoPCParams;
}

#endif

inline bcostars::LogEntry toTarsLogEntry(bcos::protocol::LogEntry const& _logEntry)
{
    bcostars::LogEntry logEntry;
    logEntry.address.assign(_logEntry.address().begin(), _logEntry.address().end());
    for (auto& topicIt : _logEntry.topics())
    {
        logEntry.topic.push_back(std::vector<char>(topicIt.begin(), topicIt.end()));
    }
    logEntry.data.assign(_logEntry.data().begin(), _logEntry.data().end());
    return logEntry;
}

inline bcos::protocol::LogEntry toBcosLogEntry(bcostars::LogEntry const& _logEntry)
{
    std::vector<bcos::h256> topics;
    for (auto& topicIt : _logEntry.topic)
    {
        topics.emplace_back((const bcos::byte*)topicIt.data(), topicIt.size());
    }
    return bcos::protocol::LogEntry(bcos::bytes(_logEntry.address.begin(), _logEntry.address.end()),
        topics, bcos::bytes(_logEntry.data.begin(), _logEntry.data.end()));
}

inline bcos::protocol::LogEntry takeToBcosLogEntry(bcostars::LogEntry&& _logEntry)
{
    std::vector<bcos::h256> topics;
    for (auto&& topicIt : _logEntry.topic)
    {
        bcos::h256 topic;
        RANGES::move(topicIt.begin(), topicIt.end(), topic.mutableData().data());
        topics.push_back(std::move(topic));
    }
    bcos::bytes address;
    RANGES::move(std::move(_logEntry.address), std::back_inserter(address));
    bcos::bytes data;
    RANGES::move(std::move(_logEntry.data), std::back_inserter(data));
    return {std::move(address), std::move(topics), std::move(data)};
}
}  // namespace bcostars