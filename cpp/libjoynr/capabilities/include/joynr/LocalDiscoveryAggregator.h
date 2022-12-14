/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */
#ifndef LOCALDISCOVERYAGGREGATOR_H
#define LOCALDISCOVERYAGGREGATOR_H

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/optional.hpp>

#include "joynr/Future.h"
#include "joynr/JoynrExport.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/system/IDiscovery.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"
#include "joynr/types/DiscoveryError.h"

namespace joynr
{

class MessagingQos;

namespace types
{
class DiscoveryEntry;
class DiscoveryQos;
} // namespace types

namespace exceptions
{
class JoynrRuntimeException;
}

/**
 * @brief The LocalDiscoveryAggregator class is a wrapper for discovery proxies. It holds a list
 * of provisioned discovery entries (for example for the discovery and routing provider). If a
 * lookup is performed by using a participant ID, these entries are checked and returned first
 * before the request is forwarded to the wrapped discovery provider.
 */
class JOYNR_EXPORT LocalDiscoveryAggregator : public joynr::system::IDiscoveryAsync
{
public:
    LocalDiscoveryAggregator(std::map<std::string, joynr::types::DiscoveryEntryWithMetaInfo>
                                     provisionedDiscoveryEntries);

    void setDiscoveryProxy(std::shared_ptr<IDiscoveryAsync> discoveryProxy);

    // inherited from joynr::system::IDiscoveryAsync
    std::shared_ptr<joynr::Future<void>> addAsync(
            const joynr::types::DiscoveryEntry& discoveryEntry,
            std::function<void()> onSuccess = nullptr,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError = nullptr,
            boost::optional<joynr::MessagingQos> messagingQos = boost::none) noexcept override;

    // inherited from joynr::system::IDiscoveryAsync
    std::shared_ptr<joynr::Future<void>> addAsync(
            const joynr::types::DiscoveryEntry& discoveryEntry,
            const bool& awaitGlobalRegistration,
            const std::vector<std::string>& gbids,
            std::function<void()> onSuccess = nullptr,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)>
                    onApplicationError = nullptr,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError = nullptr,
            boost::optional<joynr::MessagingQos> messagingQos = boost::none) noexcept override;

    // inherited from joynr::system::IDiscoveryAsync
    std::shared_ptr<joynr::Future<void>> addAsync(
            const joynr::types::DiscoveryEntry& discoveryEntry,
            const bool& awaitGlobalRegistration,
            std::function<void()> onSuccess = nullptr,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError = nullptr,
            boost::optional<joynr::MessagingQos> messagingQos = boost::none) noexcept override;

    // inherited from joynr::system::IDiscoveryAsync
    std::shared_ptr<joynr::Future<void>> addToAllAsync(
            const joynr::types::DiscoveryEntry& discoveryEntry,
            const bool& awaitGlobalRegistration,
            std::function<void()> onSuccess = nullptr,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)>
                    onApplicationError = nullptr,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError = nullptr,
            boost::optional<joynr::MessagingQos> messagingQos = boost::none) noexcept override;

    // inherited from joynr::system::IDiscoveryAsync
    std::shared_ptr<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>
    lookupAsync(
            const std::vector<std::string>& domains,
            const std::string& interfaceName,
            const joynr::types::DiscoveryQos& discoveryQos,
            std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result)>
                    onSuccess = nullptr,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError = nullptr,
            boost::optional<joynr::MessagingQos> messagingQos = boost::none) noexcept override;

    // inherited from joynr::system::IDiscoveryAsync
    std::shared_ptr<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>
    lookupAsync(
            const std::vector<std::string>& domains,
            const std::string& interfaceName,
            const joynr::types::DiscoveryQos& discoveryQos,
            const std::vector<std::string>& gbids,
            std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result)>
                    onSuccess = nullptr,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)>
                    onApplicationError = nullptr,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError = nullptr,
            boost::optional<joynr::MessagingQos> messagingQos = boost::none) noexcept override;

    // inherited from joynr::system::IDiscoveryAsync
    std::shared_ptr<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>> lookupAsync(
            const std::string& participantId,
            std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo& result)> onSuccess =
                    nullptr,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError = nullptr,
            boost::optional<joynr::MessagingQos> messagingQos = boost::none) noexcept override;

    // inherited from joynr::system::IDiscoveryAsync
    std::shared_ptr<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>> lookupAsync(
            const std::string& participantId,
            const joynr::types::DiscoveryQos& discoveryQos,
            const std::vector<std::string>& gbids,
            std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo& result)> onSuccess =
                    nullptr,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)>
                    onApplicationError = nullptr,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError = nullptr,
            boost::optional<joynr::MessagingQos> messagingQos = boost::none) noexcept override;

    // inherited from joynr::system::IDiscoveryAsync
    std::shared_ptr<joynr::Future<void>> removeAsync(
            const std::string& participantId,
            std::function<void()> onSuccess = nullptr,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError = nullptr,
            boost::optional<joynr::MessagingQos> messagingQos = boost::none) noexcept override;

private:
    DISALLOW_COPY_AND_ASSIGN(LocalDiscoveryAggregator);

    std::shared_ptr<joynr::Future<types::DiscoveryEntryWithMetaInfo>> findProvisionedEntry(
            const std::string& participantId,
            std::function<void(const types::DiscoveryEntryWithMetaInfo&)> onSuccess) noexcept;

    std::shared_ptr<joynr::Future<std::vector<types::DiscoveryEntryWithMetaInfo>>>
    findProvisionedEntry(const std::vector<std::string>& domains,
                         const std::string& interfaceName,
                         std::function<void(const std::vector<types::DiscoveryEntryWithMetaInfo>&)>
                                 onSuccess) noexcept;

    std::shared_ptr<joynr::system::IDiscoveryAsync> _discoveryProxy;

    using ProvisionedDiscoveryEntriesConteriner = boost::multi_index_container<
            joynr::types::DiscoveryEntryWithMetaInfo,
            boost::multi_index::indexed_by<
                    boost::multi_index::hashed_unique<BOOST_MULTI_INDEX_CONST_MEM_FUN(
                            types::DiscoveryEntry,
                            const std::string&,
                            getParticipantId)>,
                    boost::multi_index::hashed_non_unique<boost::multi_index::composite_key<
                            joynr::types::DiscoveryEntryWithMetaInfo,
                            BOOST_MULTI_INDEX_CONST_MEM_FUN(types::DiscoveryEntry,
                                                            const std::string&,
                                                            getDomain),
                            BOOST_MULTI_INDEX_CONST_MEM_FUN(types::DiscoveryEntry,
                                                            const std::string&,
                                                            getInterfaceName)>>>>;

    ProvisionedDiscoveryEntriesConteriner _provisionedDiscoveryEntries;
};
} // namespace joynr
#endif // LOCALDISCOVERYAGGREGATOR_H
