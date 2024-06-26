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
 * @file SealerFactory.cpp
 * @author: yujiechen
 * @date: 2021-05-20
 */
#include "SealerFactory.h"

#include "Sealer.h"
#include "bcos-tool/NodeTimeMaintenance.h"
#include <utility>

using namespace bcos;
using namespace bcos::sealer;

SealerFactory::SealerFactory(bcos::tool::NodeConfig::Ptr _nodeConfig,
    bcos::protocol::BlockFactory::Ptr _blockFactory, bcos::txpool::TxPoolInterface::Ptr _txpool,
    bcos::tool::NodeTimeMaintenance::Ptr _nodeTimeMaintenance,
    bcos::crypto::KeyPairInterface::Ptr _key)
  : m_groupId(_nodeConfig->groupId()),
    m_chainId(_nodeConfig->chainId()),
    m_blockFactory(std::move(_blockFactory)),
    m_txpool(std::move(_txpool)),
    m_minSealTime(_nodeConfig->minSealTime()),
    m_nodeTimeMaintenance(std::move(_nodeTimeMaintenance)),
    m_keyPair(std::move(_key))
{}

Sealer::Ptr SealerFactory::createSealer()
{
    auto sealerConfig =
        std::make_shared<SealerConfig>(m_blockFactory, m_txpool, m_nodeTimeMaintenance);
    sealerConfig->setMinSealTime(m_minSealTime);
    sealerConfig->setKeyPair(m_keyPair);
    sealerConfig->setGroupId(m_groupId);
    sealerConfig->setChainId(m_chainId);
    return std::make_shared<Sealer>(sealerConfig);
}

VRFBasedSealer::Ptr SealerFactory::createVRFBasedSealer()
{
    auto sealerConfig =
        std::make_shared<SealerConfig>(m_blockFactory, m_txpool, m_nodeTimeMaintenance);
    sealerConfig->setMinSealTime(m_minSealTime);
    sealerConfig->setKeyPair(m_keyPair);
    sealerConfig->setGroupId(m_groupId);
    sealerConfig->setChainId(m_chainId);
    return std::make_shared<VRFBasedSealer>(sealerConfig);
}