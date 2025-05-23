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
 * @file ServiceV2.h
 * @author: yujiechen
 * @date 2022-5-24
 */
#pragma once
#include "Service.h"
#include "router/RouterTableInterface.h"

namespace bcos::gateway
{
class ServiceV2 : public Service
{
public:
    using Ptr = std::shared_ptr<ServiceV2>;
    ServiceV2(P2PInfo const& _p2pInfo, RouterTableFactory::Ptr _routerTableFactory);
    ServiceV2() = delete;
    ServiceV2(const ServiceV2&) = delete;
    ServiceV2(ServiceV2&&) = delete;
    ServiceV2& operator=(const ServiceV2&) = delete;
    ServiceV2& operator=(ServiceV2&&) = delete;
    ~ServiceV2() override = default;

    void start() override;
    void stop() override;

    void asyncSendMessageByNodeID(P2pID nodeID, std::shared_ptr<P2PMessage> message,
        CallbackFuncWithSession callback, Options options = Options()) override;

    void onMessage(NetworkException _error, SessionFace::Ptr session, Message::Ptr message,
        std::weak_ptr<P2PSession> p2pSessionWeakPtr) override;
    void sendRespMessageBySession(
        bytesConstRef _payload, P2PMessage::Ptr _p2pMessage, P2PSession::Ptr _p2pSession) override;
    void asyncBroadcastMessage(std::shared_ptr<P2PMessage> message, Options options) override;
    bool isReachable(P2pID const& _nodeID) const override;

    // handlers called when the node is unreachable
    void registerUnreachableHandler(std::function<void(std::string)> _handler) override;

    task::Task<Message::Ptr> sendMessageByNodeID(P2pID nodeID, P2PMessage& message,
        ::ranges::any_view<bytesConstRef> payloads, Options options = Options()) override;

    std::string getShortP2pID(std::string const& rawP2pID) const override;
    std::string getRawP2pID(std::string const& shortP2pID) const override;

    // Note: since the message of the old node maybe forwarded through new node, we should try to
    // reset the p2pNodeID in both cases >=v3 and <v3
    void resetP2pID(P2PMessage& message, bcos::protocol::ProtocolVersion const& version) override;

protected:
    // called when the nodes become unreachable
    void onP2PNodesUnreachable(std::set<std::string> const& _p2pNodeIDs);
    // router related
    virtual void onReceivePeersRouterTable(
        NetworkException _error, std::shared_ptr<P2PSession> _session, P2PMessage::Ptr _message);
    virtual void joinRouterTable(
        std::shared_ptr<P2PSession> _session, RouterTableInterface::Ptr _routerTable);
    virtual void onReceiveRouterTableRequest(
        NetworkException _error, std::shared_ptr<P2PSession> _session, P2PMessage::Ptr _message);
    virtual void broadcastRouterSeq();
    virtual void onReceiveRouterSeq(
        NetworkException _error, std::shared_ptr<P2PSession> _session, P2PMessage::Ptr _message);

    virtual void onNewSession(P2PSession::Ptr _session);
    virtual void onEraseSession(P2PSession::Ptr _session);
    bool tryToUpdateSeq(std::string const& _p2pNodeID, uint32_t _seq);
    bool eraseSeq(std::string const& _p2pNodeID);

    virtual void asyncSendMessageByNodeIDWithMsgForward(std::shared_ptr<P2PMessage> _message,
        CallbackFuncWithSession _callback, Options options = Options());

    virtual void asyncBroadcastMessageWithoutForward(
        std::shared_ptr<P2PMessage> message, Options options);

    void updateP2pInfo(P2PInfo const& p2pInfo);
    void tryToUpdateRawP2pInfo(P2PInfo const& p2pInfo);
    void tryToUpdateP2pInfo(P2PInfo const& p2pInfo);

private:
    // for message forward
    // Note: must use ptr here, for the timer uses enable_shared_from_this
    std::shared_ptr<bcos::Timer> m_routerTimer;
    std::atomic<uint32_t> m_statusSeq{1};

    RouterTableFactory::Ptr m_routerTableFactory;
    RouterTableInterface::Ptr m_routerTable;

    std::map<std::string, uint32_t> m_node2Seq;
    mutable SharedMutex x_node2Seq;

    // rawP2pID->p2pID
    std::map<std::string, std::string> m_rawP2pIDInfo;
    mutable SharedMutex x_rawP2pIDInfo;
    // p2pID->rawP2pID
    std::map<std::string, std::string> m_p2pIDInfo;
    mutable SharedMutex x_p2pIDInfo;

    const int c_unreachableDistance = 10;

    // called when the given node unreachable
    std::vector<std::function<void(std::string)>> m_unreachableHandlers;
    mutable SharedMutex x_unreachableHandlers;
};
}  // namespace bcos::gateway
