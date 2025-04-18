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
 * @brief implementation for PBFTEngine
 * @file PBFTEngine.h
 * @author: yujiechen
 * @date 2021-04-20
 */
#pragma once
#include "PBFTLogSync.h"
#include "bcos-framework/ledger/LedgerInterface.h"
#include "bcos-pbft/core/ConsensusEngine.h"
#include <bcos-utilities/Error.h>
#include <bcos-utilities/Timer.h>
#include <oneapi/tbb/concurrent_queue.h>
#include <utility>

namespace bcos
{
namespace ledger
{
class LedgerConfig;
}
namespace consensus
{
class PBFTBaseMessageInterface;
class PBFTMessageInterface;
class ViewChangeMsgInterface;
class NewViewMsgInterface;
class PBFTConfig;
class PBFTCacheProcessor;
class PBFTProposalInterface;

enum CheckResult
{
    VALID = 0,
    INVALID = 1,
};

class PBFTEngine : public ConsensusEngine, public std::enable_shared_from_this<PBFTEngine>
{
public:
    using Ptr = std::shared_ptr<PBFTEngine>;
    using SendResponseCallback = std::function<void(bytesConstRef)>;
    explicit PBFTEngine(std::shared_ptr<PBFTConfig> _config);
    ~PBFTEngine() override { stop(); }

    void start() override;
    void stop() override;

    virtual void asyncSubmitProposal(bool _containSysTxs, const protocol::Block& proposal,
        bcos::protocol::BlockNumber _proposalIndex, bcos::crypto::HashType const& _proposalHash,
        std::function<void(Error::Ptr)> _onProposalSubmitted);

    std::shared_ptr<PBFTConfig> pbftConfig() { return m_config; }

    // Receive PBFT message package from frontService
    virtual void onReceivePBFTMessage(bcos::Error::Ptr _error, std::string const& _id,
        bcos::crypto::NodeIDPtr _nodeID, bytesConstRef _data);

    virtual void initState(PBFTProposalList const& _proposals, bcos::crypto::NodeIDPtr _fromNode)
    {
        m_cacheProcessor->initState(_proposals, std::move(_fromNode));
    }

    virtual void asyncNotifyNewBlock(
        bcos::ledger::LedgerConfig::Ptr _ledgerConfig, std::function<void(Error::Ptr)> _onRecv);

    virtual std::shared_ptr<PBFTCacheProcessor> cacheProcessor() { return m_cacheProcessor; }
    virtual bool isTimeout() { return m_config->timeout(); }
    void registerCommittedProposalNotifier(
        std::function<void(bcos::protocol::BlockNumber, std::function<void(Error::Ptr)>)>
            _committedProposalNotifier)
    {
        m_cacheProcessor->registerCommittedProposalNotifier(std::move(_committedProposalNotifier));
    }

    virtual void restart();

    virtual void clearExceptionProposalState(bcos::protocol::BlockNumber _number);

    void clearAllCache();
    void recoverState();

    void fetchAndUpdateLedgerConfig();
    bool shouldRotateSealers(protocol::BlockNumber _number) const;
    void setLedger(ledger::LedgerInterface::Ptr ledger);

protected:
    virtual void initSendResponseHandler();
    virtual void tryToResendCheckPoint();
    virtual void onReceivePBFTMessage(bcos::Error::Ptr _error, bcos::crypto::NodeIDPtr _nodeID,
        bytesConstRef _data, SendResponseCallback _sendResponse);

    virtual void onRecvProposal(bool _containSysTxs, const protocol::Block& proposal,
        bcos::protocol::BlockNumber _proposalIndex, bcos::crypto::HashType const& _proposalHash);

    // PBFT main processing function
    void executeWorker() override;

    // General entry for message processing
    virtual void handleMsg(std::shared_ptr<PBFTBaseMessageInterface> _msg);

    // Process Pre-prepare type message packets
    virtual bool handlePrePrepareMsg(std::shared_ptr<PBFTMessageInterface> _prePrepareMsg,
        bool _needVerifyProposal, bool _generatedFromNewView = false,
        bool _needCheckSignature = true);
    // When handlePrePrepareMsg return false, then reset sealed txs
    virtual void resetSealedTxs(
        std::shared_ptr<PBFTMessageInterface> const& _prePrepareMsg, const protocol::Block& block);

    // To check pre-prepare msg valid
    virtual CheckResult checkPrePrepareMsg(std::shared_ptr<PBFTMessageInterface> _prePrepareMsg);
    // To check pbft msg sign valid
    virtual CheckResult checkSignature(std::shared_ptr<PBFTBaseMessageInterface> _req);
    virtual bool checkProposalSignature(
        IndexType _generatedFrom, PBFTProposalInterface::Ptr _proposal);

    virtual CheckResult checkPBFTMsgState(std::shared_ptr<PBFTMessageInterface> _pbftReq) const;
    virtual bool checkRotateTransactionValid(PBFTMessageInterface::Ptr const& _proposal,
        ConsensusNode const& _leaderInfo, bool needCheckSign);

    // When pre-prepare proposal seems ok, then broadcast prepare msg
    virtual void broadcastPrepareMsg(std::shared_ptr<PBFTMessageInterface> const& _prePrepareMsg);

    // Process the Prepare type message packet
    virtual bool handlePrepareMsg(PBFTMessageInterface::Ptr const& _prepareMsg);
    // To check 'Prepare' or 'Commit' type proposal
    virtual CheckResult checkPBFTMsg(PBFTMessageInterface::Ptr const& _prepareMsg);

    virtual bool handleCommitMsg(PBFTMessageInterface::Ptr const& _commitMsg);

    virtual void onTimeout();
    virtual ViewChangeMsgInterface::Ptr generateViewChange();
    virtual void broadcastViewChangeReq();
    virtual void sendViewChange(bcos::crypto::NodeIDPtr _dstNode);

    virtual bool handleViewChangeMsg(std::shared_ptr<ViewChangeMsgInterface> _viewChangeMsg);
    virtual bool isValidViewChangeMsg(bcos::crypto::NodeIDPtr _fromNode,
        std::shared_ptr<ViewChangeMsgInterface> _viewChangeMsg, bool _shouldCheckSig = true);

    virtual bool handleNewViewMsg(std::shared_ptr<NewViewMsgInterface> _newViewMsg);
    virtual void reHandlePrePrepareProposals(std::shared_ptr<NewViewMsgInterface> _newViewMsg);
    virtual bool isValidNewViewMsg(std::shared_ptr<NewViewMsgInterface> _newViewMsg);
    virtual void reachNewView(ViewType _view);

    // handle the checkpoint message
    virtual bool handleCheckPointMsg(std::shared_ptr<PBFTMessageInterface> _checkPointMsg);

    // function called after reaching a consensus
    virtual void finalizeConsensus(
        std::shared_ptr<bcos::ledger::LedgerConfig> _ledgerConfig, bool _syncedBlock = false);


    virtual void onProposalApplied(int64_t _errorCode, PBFTProposalInterface::Ptr _proposal,
        PBFTProposalInterface::Ptr _executedProposal);
    virtual void onProposalApplySuccess(
        PBFTProposalInterface::Ptr _proposal, PBFTProposalInterface::Ptr _executedProposal);
    virtual void onProposalApplyFailed(int64_t _errorCode, PBFTProposalInterface::Ptr _proposal);
    virtual void onLoadAndVerifyProposalFinish(
        bool _verifyResult, Error::Ptr const& _error, PBFTProposalInterface::Ptr const& _proposal);
    virtual void triggerTimeout(bool _incTimeout = true);

    void handleRecoverResponse(PBFTMessageInterface::Ptr _recoverResponse);
    void handleRecoverRequest(PBFTMessageInterface::Ptr _request);
    void sendRecoverResponse(bcos::crypto::NodeIDPtr _dstNode);
    bool isSyncingHigher();
    /**
     * @brief Receive proposal requests from other nodes and reply to corresponding proposals
     *
     * @param _pbftMsg the proposal request
     * @param _sendResponse callback used to send the requested-proposals back to the node
     */
    virtual void onReceiveCommittedProposalRequest(
        PBFTBaseMessageInterface::Ptr _pbftMsg, SendResponseCallback _sendResponse);

    /**
     * @brief Receive precommit requests from other nodes and reply to the corresponding precommit
     * data
     *
     * @param _pbftMessage the precommit request
     * @param _sendResponse callback used to send the requested-proposals back to the node
     */
    virtual void onReceivePrecommitRequest(
        std::shared_ptr<PBFTBaseMessageInterface> _pbftMessage, SendResponseCallback _sendResponse);
    void sendCommittedProposalResponse(
        PBFTProposalList const& _proposalList, SendResponseCallback _sendResponse);

    virtual void onStableCheckPointCommitFailed(
        Error::Ptr&& _error, PBFTProposalInterface::Ptr _stableProposal);

private:
    // utility functions
    void waitSignal()
    {
        boost::unique_lock<boost::mutex> lock(x_signalled);
        m_signalled.wait_for(lock, boost::chrono::milliseconds(1));
    }
    void switchToRPBFT(const ledger::LedgerConfig::Ptr& _ledgerConfig);

protected:
    // PBFT configuration class
    // mainly maintains the node information, consensus configuration information
    // such as consensus node list, consensus weight, etc.
    std::shared_ptr<PBFTConfig> m_config;

    // PBFT message cache queue
    tbb::concurrent_queue<std::shared_ptr<PBFTBaseMessageInterface>> m_msgQueue;
    std::shared_ptr<PBFTCacheProcessor> m_cacheProcessor;
    // for log syncing
    PBFTLogSync::Ptr m_logSync;

    std::function<void(std::string const&, int, bcos::crypto::NodeIDPtr, bytesConstRef)>
        m_sendResponseHandler;

    boost::condition_variable m_signalled;
    boost::mutex x_signalled;
    mutable RecursiveMutex m_mutex;

    const unsigned c_PopWaitSeconds = 5;
    const std::set<PacketType> c_consensusPacket = {PrePreparePacket, PreparePacket, CommitPacket};

    std::atomic_bool m_stopped = {false};

    // the timer used to resend checkPointProposal
    std::shared_ptr<bcos::Timer> m_timer;

    ledger::LedgerInterface::Ptr m_ledger;
    ledger::LedgerConfig::Ptr m_ledgerConfig;
};
}  // namespace consensus
}  // namespace bcos