/**
 *  Copyright (C) 2021 bcos-sync.
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
 * @brief fixture for the BlockSync
 * @file SyncFixture.h
 * @author: yujiechen
 * @date 2021-06-08
 */
#pragma once
#include "../faker/FakeConsensus.h"
#include "bcos-sync/BlockSync.h"
#include "bcos-sync/BlockSyncFactory.h"
#include <bcos-framework/consensus/ConsensusNode.h>
#include <bcos-framework/testutils/faker/FakeFrontService.h>
#include <bcos-framework/testutils/faker/FakeLedger.h>
#include <bcos-framework/testutils/faker/FakeScheduler.h>
#include <bcos-framework/testutils/faker/FakeTxPool.h>
#include <bcos-protocol/TransactionSubmitResultFactoryImpl.h>
#include <bcos-tool/NodeTimeMaintenance.h>

using namespace bcos;
using namespace bcos::sync;
using namespace bcos::crypto;
using namespace bcos::protocol;
using namespace bcos::scheduler;
using namespace bcos::tool;

namespace bcos
{
namespace test
{
class FakeBlockSync : public BlockSync
{
public:
    using Ptr = std::shared_ptr<FakeBlockSync>;
    FakeBlockSync(BlockSyncConfig::Ptr _config, unsigned _idleWaitMs = 200)
      : BlockSync(_config, _idleWaitMs)
    {
        m_running = true;
        enableAsMaster(true);
    }
    ~FakeBlockSync() override {}

    void executeWorker() override { BlockSync::executeWorker(); }
    void maintainPeersConnection() override { BlockSync::maintainPeersConnection(); }
    SyncPeerStatus::Ptr syncStatus() { return m_syncStatus; }
};

class FakeTxPoolForSync : public FakeTxPool
{
public:
    FakeTxPoolForSync() = default;
    void asyncNotifyBlockResult(BlockNumber, TransactionSubmitResultsPtr,
        std::function<void(Error::Ptr)> _callback) override
    {
        if (_callback)
        {
            _callback(nullptr);
        }
    }
};

class FakeBlockSyncFactory : public BlockSyncFactory
{
public:
    using Ptr = std::shared_ptr<FakeBlockSyncFactory>;
    FakeBlockSyncFactory(PublicPtr _nodeId, BlockFactory::Ptr _blockFactory,
        LedgerInterface::Ptr _ledger, FrontServiceInterface::Ptr _frontService,
        SchedulerInterface::Ptr _dispatcher, ConsensusInterface::Ptr _consensus,
        NodeTimeMaintenance::Ptr _nodeTimeMaintenance)
      : BlockSyncFactory(_nodeId, _blockFactory,
            std::make_shared<bcos::protocol::TransactionSubmitResultFactoryImpl>(), _ledger,
            std::make_shared<FakeTxPoolForSync>(), _frontService, _dispatcher, _consensus,
            _nodeTimeMaintenance)
    {}

    BlockSync::Ptr createBlockSync() override
    {
        auto sync = BlockSyncFactory::createBlockSync();
        return std::make_shared<FakeBlockSync>(sync->config());
    }
};

class SyncFixture
{
public:
    using Ptr = std::shared_ptr<SyncFixture>;
    SyncFixture(CryptoSuite::Ptr _cryptoSuite, FakeGateWay::Ptr _fakeGateWay,
        size_t _blockNumber = 0, std::vector<bytes> _sealerList = std::vector<bytes>())
      : m_cryptoSuite(_cryptoSuite), m_gateWay(_fakeGateWay)
    {
        m_keyPair = _cryptoSuite->signatureImpl()->generateKeyPair();
        m_blockFactory = createBlockFactory(_cryptoSuite);
        m_ledger = std::make_shared<FakeLedger>(m_blockFactory, _blockNumber, 10, 0, _sealerList);
        m_ledger->setSystemConfig(SYSTEM_KEY_TX_COUNT_LIMIT, std::to_string(1000));
        m_ledger->setSystemConfig(SYSTEM_KEY_CONSENSUS_LEADER_PERIOD, std::to_string(1));
        m_ledger->setSystemConfig(SYSTEM_KEY_AUTH_CHECK_STATUS, std::to_string(0));
        m_ledger->setSystemConfig(SYSTEM_KEY_COMPATIBILITY_VERSION, protocol::DEFAULT_VERSION_STR);
        // m_ledger->ledgerConfig()->setConsensusTimeout(_consensusTimeout * 20);
        m_frontService = std::make_shared<FakeFrontService>(m_keyPair->publicKey());
        m_consensus = std::make_shared<FakeConsensus>();
        m_nodeTimeMaintenance = std::make_shared<NodeTimeMaintenance>();

        // create FakeScheduler
        m_scheduler = std::make_shared<FakeScheduler>(m_ledger, m_blockFactory);
        auto blockSyncFactory =
            std::make_shared<FakeBlockSyncFactory>(m_keyPair->publicKey(), m_blockFactory, m_ledger,
                m_frontService, m_scheduler, m_consensus, m_nodeTimeMaintenance);
        m_sync = std::dynamic_pointer_cast<FakeBlockSync>(blockSyncFactory->createBlockSync());
        if (_fakeGateWay)
        {
            _fakeGateWay->addSync(m_keyPair->publicKey(), m_sync);
        }
        m_frontService->setGateWay(_fakeGateWay);
    }

    FakeFrontService::Ptr frontService() { return m_frontService; }
    FakeScheduler::Ptr scheduler() { return m_scheduler; }
    FakeConsensus::Ptr consensus() { return m_consensus; }
    FakeLedger::Ptr ledger() { return m_ledger; }

    FakeGateWay::Ptr gateWay() { return m_gateWay; }
    PublicPtr nodeID() { return m_keyPair->publicKey(); }

    FakeBlockSync::Ptr sync() { return m_sync; }

    void appendObserver(NodeIDPtr _nodeId)
    {
        auto node = ConsensusNode{_nodeId, consensus::Type::consensus_observer, 0, 0, 0};
        m_ledger->ledgerConfig()->mutableObserverList().emplace_back(node);
        m_sync->config()->setObserverList(m_ledger->ledgerConfig()->observerNodeList());
    }

    void setConsensus(std::vector<NodeIDPtr> _nodeIdList)
    {
        m_ledger->ledgerConfig()->mutableConsensusList().clear();
        for (auto const& node : _nodeIdList)
        {
            m_ledger->ledgerConfig()->mutableConsensusList().emplace_back(
                ConsensusNode{node, consensus::Type::consensus_sealer, 1, 0, 0});
        }
        m_sync->config()->setConsensusNodeList(m_ledger->ledgerConfig()->consensusNodeList());
        bcos::crypto::NodeIDSet nodeIdSet;
        for (const auto& node : m_ledger->ledgerConfig()->consensusNodeList())
        {
            nodeIdSet.insert(node.nodeID);
        }
        for (const auto& node : m_ledger->ledgerConfig()->observerNodeList())
        {
            nodeIdSet.insert(node.nodeID);
        }
        m_sync->config()->setConnectedNodeList(nodeIdSet);
        m_frontService->setNodeIDList(m_sync->config()->connectedNodeList());

        std::cout << "consensusNodeList size: "
                  << m_ledger->ledgerConfig()->consensusNodeList().size() << std::endl;
    }

    void setObservers(std::vector<NodeIDPtr> _nodeIdList)
    {
        m_ledger->ledgerConfig()->mutableObserverList().clear();
        for (auto const& node : _nodeIdList)
        {
            m_ledger->ledgerConfig()->mutableObserverList().emplace_back(
                ConsensusNode{node, consensus::Type::consensus_observer, 0, 0, 0});
        }
        m_sync->config()->setObserverList(m_ledger->ledgerConfig()->observerNodeList());
        bcos::crypto::NodeIDSet nodeIdSet;
        for (const auto& node : m_ledger->ledgerConfig()->consensusNodeList())
        {
            nodeIdSet.insert(node.nodeID);
        }
        for (const auto& node : m_ledger->ledgerConfig()->observerNodeList())
        {
            nodeIdSet.insert(node.nodeID);
        }
        m_sync->config()->setConnectedNodeList(nodeIdSet);
        m_frontService->setNodeIDList(m_sync->config()->connectedNodeList());
    }

    void init() { m_sync->init(); }

    BlockSyncConfig::Ptr syncConfig() { return m_sync->config(); }

private:
    CryptoSuite::Ptr m_cryptoSuite;
    KeyPairInterface::Ptr m_keyPair;
    BlockFactory::Ptr m_blockFactory;
    FakeGateWay::Ptr m_gateWay;

    FakeFrontService::Ptr m_frontService;
    FakeConsensus::Ptr m_consensus;
    FakeLedger::Ptr m_ledger;

    FakeScheduler::Ptr m_scheduler;
    FakeBlockSync::Ptr m_sync;
    NodeTimeMaintenance::Ptr m_nodeTimeMaintenance;
};
}  // namespace test
}  // namespace bcos
