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
 * @brief base class for sync config
 * @file SyncConfig.h
 * @author: yujiechen
 * @date 2021-05-25
 */
#pragma once
#include "bcos-framework/consensus/ConsensusNode.h"
#include <bcos-crypto/interfaces/crypto/KeyInterface.h>
#include <bcos-framework/protocol/ProtocolTypeDef.h>
#include <bcos-utilities/Log.h>
#include <mutex>
namespace bcos::sync
{
class SyncConfig
{
public:
    using Ptr = std::shared_ptr<SyncConfig>;
    SyncConfig(const SyncConfig&) = delete;
    SyncConfig(SyncConfig&&) = delete;
    SyncConfig& operator=(const SyncConfig&) = delete;
    SyncConfig& operator=(SyncConfig&&) = delete;
    explicit SyncConfig(bcos::crypto::NodeIDPtr _nodeId) : m_nodeId(std::move(_nodeId)) {}
    virtual ~SyncConfig() = default;

    bcos::crypto::NodeIDPtr nodeID() { return m_nodeId; }
    // Note: copy here to ensure thread-safe
    virtual bcos::consensus::ConsensusNodeList consensusNodeList()
    {
        ReadGuard lock(x_consensusNodeList);
        return m_consensusNodeList;
    }
    // Note: only when the consensusNodeList or observerNodeList changed will call this interface
    // for perf consideration
    virtual void setConsensusNodeList(bcos::consensus::ConsensusNodeList _consensusNodeList)
    {
        {
            WriteGuard lock(x_consensusNodeList);
            m_consensusNodeList = std::move(_consensusNodeList);
        }
        updateNodeList();
    }

    // Note: only when the consensusNodeList or observerNodeList changed will call this interface
    // for perf consideration
    virtual void setObserverList(bcos::consensus::ConsensusNodeList _observerNodeList)
    {
        {
            WriteGuard lock(x_observerNodeList);
            m_observerNodeList = std::move(_observerNodeList);
        }
        updateNodeList();
    }

    virtual bcos::consensus::ConsensusNodeList observerNodeList()
    {
        ReadGuard lock(x_observerNodeList);
        return m_observerNodeList;
    }

    // Note: copy here to remove multithreading issues
    virtual bcos::crypto::NodeIDSet connectedNodeList()
    {
        ReadGuard lock(x_connectedNodeList);
        return m_connectedNodeList;
    }

    virtual bcos::crypto::NodeIDSet connectedNodeSet()
    {
        ReadGuard lock(x_connectedNodeList);
        return m_connectedNodeList;
    }

    virtual void setConnectedNodeList(bcos::crypto::NodeIDSet _connectedNodeList)
    {
        WriteGuard lock(x_connectedNodeList);
        m_connectedNodeList = std::move(_connectedNodeList);
        BCOS_LOG(INFO) << LOG_DESC("SyncConfig: setConnectedNodeList")
                       << LOG_KV("size", m_connectedNodeList.size());
    }

    virtual bool existsInGroup()
    {
        ReadGuard lock(x_nodeList);
        return m_nodeList.contains(m_nodeId);
    }


    virtual bool existsInGroup(const bcos::crypto::NodeIDPtr& _nodeId)
    {
        ReadGuard lock(x_nodeList);
        return m_nodeList.contains(_nodeId);
    }

    virtual bool connected(const bcos::crypto::NodeIDPtr& _nodeId)
    {
        ReadGuard lock(x_connectedNodeList);
        return m_connectedNodeList.contains(_nodeId);
    }

    bcos::crypto::NodeIDSet groupNodeList()
    {
        ReadGuard lock(x_nodeList);
        return m_nodeList;
    }

    bcos::crypto::NodeIDSet connectedGroupNodeList()
    {
        std::scoped_lock lock(x_nodeList, x_connectedNodeList);
        auto nodeList =
            m_nodeList | RANGES::views::filter([this](const bcos::crypto::NodeIDPtr& _nodeId) {
                return m_connectedNodeList.contains(_nodeId);
            });
        return {nodeList.begin(), nodeList.end()};
    }

    virtual void notifyConnectedNodes(bcos::crypto::NodeIDSet const& _connectedNodes,
        const std::function<void(Error::Ptr)>& _onRecvResponse)
    {
        setConnectedNodeList(_connectedNodes);
        if (!_onRecvResponse)
        {
            return;
        }
        _onRecvResponse(nullptr);
    }

private:
    void updateNodeList()
    {
        auto nodeList = consensusNodeList() + observerNodeList();
        WriteGuard lock(x_nodeList);
        m_nodeList.clear();
        for (const auto& node : nodeList)
        {
            m_nodeList.insert(node.nodeID);
        }
    }

protected:
    bcos::crypto::NodeIDPtr m_nodeId;

    bcos::consensus::ConsensusNodeList m_consensusNodeList;
    mutable SharedMutex x_consensusNodeList;

    bcos::consensus::ConsensusNodeList m_observerNodeList;
    SharedMutex x_observerNodeList;

    bcos::crypto::NodeIDSet m_nodeList;
    mutable SharedMutex x_nodeList;

    bcos::crypto::NodeIDSet m_connectedNodeList;
    mutable SharedMutex x_connectedNodeList;
};
}  // namespace bcos::sync
