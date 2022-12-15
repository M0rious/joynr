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
#ifndef LOCALCAPABILITIESDIRECTORY_H
#define LOCALCAPABILITIESDIRECTORY_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/asio/steady_timer.hpp>

#include "joynr/BoostIoserviceForwardDecl.h"
#include "joynr/ILocalCapabilitiesCallback.h"
#include "joynr/JoynrClusterControllerExport.h"
#include "joynr/LcdPendingLookupsHandler.h"
#include "joynr/LocalCapabilitiesDirectoryStore.h"
#include "joynr/Logger.h"
#include "joynr/MessagingSettings.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Semaphore.h"
#include "joynr/system/DiscoveryAbstractProvider.h"
#include "joynr/system/ProviderReregistrationControllerProvider.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/types/DiscoveryError.h"
#include "joynr/types/DiscoveryScope.h"
#include "joynr/types/GlobalDiscoveryEntry.h"

namespace joynr
{
class IAccessController;
class IGlobalCapabilitiesDirectoryClient;
class ClusterControllerSettings;
class IMessageRouter;

namespace capabilities
{
class CachingStorage;
class Storage;
} // namespace capabilities

namespace exceptions
{
class ProviderRuntimeException;
}

namespace types
{
class DiscoveryEntry;
class DiscoveryEntryWithMetaInfo;
class DiscoveryQos;
} // namespace types

/**
 * The local capabilities directory is the "first point of call" for accessing
 * any information related to the capabilities, i.e. in finding the channel id
 * for a given interface and domain, or to return the interface address for a
 * given channel id.
 * This class is responsible for looking up its local cache first, and depending
 * on whether the data is compatible with the users QoS (e.g. dataFreshness) the
 * cached value will be returned.  Otherwise, a request will be made via the
 * Global Capabilities Directory Client which will make the remote call to the
 * backend to retrieve the data.
 */
class JOYNRCLUSTERCONTROLLER_EXPORT LocalCapabilitiesDirectory
        : public joynr::system::DiscoveryAbstractProvider,
          public joynr::system::ProviderReregistrationControllerProvider,
          public std::enable_shared_from_this<LocalCapabilitiesDirectory>
{
public:
    // TODO: change shared_ptr to unique_ptr once JoynrClusterControllerRuntime is refactored
    LocalCapabilitiesDirectory(
            ClusterControllerSettings& messagingSettings,
            std::shared_ptr<IGlobalCapabilitiesDirectoryClient> globalCapabilitiesDirectoryClient,
            std::shared_ptr<LocalCapabilitiesDirectoryStore> localCapabilitiesDirectoryStore,
            const std::string& localAddress,
            std::weak_ptr<IMessageRouter> messageRouter,
            boost::asio::io_service& ioService,
            const std::string clusterControllerId,
            std::vector<std::string> knownGbids,
            std::int64_t defaultExpiryIntervalMs,
            const std::chrono::milliseconds reAddInterval = std::chrono::milliseconds(7 * 24 * 60 *
                                                                                      60 * 1000));

    ~LocalCapabilitiesDirectory() override;

    void init();

    void shutdown();

    /*
     * Call back methods which will update the local capabilities cache and call the
     * original callback with the results, this indirection was needed because we
     * need to convert a CapabilitiesInformation object into a DiscoveryEntry object.
     */
    std::vector<types::DiscoveryEntryWithMetaInfo> registerReceivedCapabilities(
            const std::vector<types::GlobalDiscoveryEntry>&& capabilityEntries);

    // inherited method from joynr::system::DiscoveryProvider
    void add(const joynr::types::DiscoveryEntry& discoveryEntry,
             std::function<void()> onSuccess,
             std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;
    // inherited method from joynr::system::DiscoveryProvider
    void add(const joynr::types::DiscoveryEntry& discoveryEntry,
             const bool& awaitGlobalRegistration,
             std::function<void()> onSuccess,
             std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;
    // inherited method from joynr::system::DiscoveryProvider
    void add(const joynr::types::DiscoveryEntry& discoveryEntry,
             const bool& awaitGlobalRegistration,
             const std::vector<std::string>& gbids,
             std::function<void()> onSuccess,
             std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError)
            override;
    // inherited method from joynr::system::DiscoveryProvider
    void addToAll(const joynr::types::DiscoveryEntry& discoveryEntry,
                  const bool& awaitGlobalRegistration,
                  std::function<void()> onSuccess,
                  std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError)
            override;
    // inherited method from joynr::system::DiscoveryProvider
    void lookup(
            const std::vector<std::string>& domains,
            const std::string& interfaceName,
            const joynr::types::DiscoveryQos& discoveryQos,
            std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result)>
                    onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;
    // inherited method from joynr::system::DiscoveryProvider
    void lookup(
            const std::vector<std::string>& domains,
            const std::string& interfaceName,
            const joynr::types::DiscoveryQos& discoveryQos,
            const std::vector<std::string>& gbids,
            std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result)>
                    onSuccess,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError)
            override;
    // inherited method from joynr::system::DiscoveryProvider
    void lookup(
            const std::string& participantId,
            std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo& result)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;
    // inherited method from joynr::system::DiscoveryProvider
    void lookup(
            const std::string& participantId,
            const joynr::types::DiscoveryQos& discoveryQos,
            const std::vector<std::string>& gbids,
            std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo& result)> onSuccess,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError)
            override;
    // inherited method from joynr::system::DiscoveryProvider
    void remove(const std::string& participantId,
                std::function<void()> onSuccess,
                std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;

    /*
     * returns true if lookup calls with discovery scope LOCAL_THEN_GLOBAL are ongoing
     */
    bool hasPendingLookups();

    /*
     * Set AccessController so that registration of providers can be checked.
     */
    void setAccessController(std::weak_ptr<IAccessController> accessController);

    void triggerGlobalProviderReregistration(
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            final override;

    std::vector<types::DiscoveryEntry> getCachedGlobalDiscoveryEntries() const;

    /**
     * Remove stale providers of cluster controller from JDS, whose last seen date is
     * lower than given start up date of the cluster controller.
     * @param ccStartDateMs - start up date of the cluster controller in milliseconds
     */
    void removeStaleProvidersOfClusterController(const std::int64_t& clusterControllerStartDateMs);

private:
    DISALLOW_COPY_AND_ASSIGN(LocalCapabilitiesDirectory);
    ClusterControllerSettings& _clusterControllerSettings; // to retrieve info about persistency

    void capabilitiesReceived(const std::vector<types::GlobalDiscoveryEntry>& results,
                              std::vector<types::DiscoveryEntry>&& localEntries,
                              std::shared_ptr<ILocalCapabilitiesCallback> callback,
                              joynr::types::DiscoveryScope::Enum discoveryScope);
    void removeStaleProvidersOfClusterController(const std::int64_t& clusterControllerStartDateMs,
                                                 const std::string gbid);
    /*
     * Returns a list of capabilities matching the given domain and interfaceName and gbids.
     * This is an asynchronous request, must supply a callback.
     */
    void lookup(const std::vector<std::string>& domains,
                const std::string& interfaceName,
                const std::vector<std::string>& gbids,
                std::shared_ptr<ILocalCapabilitiesCallback> callback,
                const joynr::types::DiscoveryQos& discoveryQos);

    /*
     * Returns a capability entry for a given participant ID and gbids or
     * an empty list if it cannot be found.
     */
    virtual void lookup(const std::string& participantId,
                        const types::DiscoveryQos& discoveryQos,
                        const std::vector<std::string>& gbids,
                        std::shared_ptr<ILocalCapabilitiesCallback> callback);

    ADD_LOGGER(LocalCapabilitiesDirectory)
    std::shared_ptr<IGlobalCapabilitiesDirectoryClient> _globalCapabilitiesDirectoryClient;
    std::shared_ptr<LocalCapabilitiesDirectoryStore> _localCapabilitiesDirectoryStore;
    std::string _localAddress;
    std::mutex _pendingLookupsLock;

    std::weak_ptr<IMessageRouter> _messageRouter;

    LcdPendingLookupsHandler _lcdPendingLookupsHandler;

    std::weak_ptr<IAccessController> _accessController;

    boost::asio::steady_timer _checkExpiredDiscoveryEntriesTimer;

    void scheduleCleanupTimer();
    void checkExpiredDiscoveryEntries(const boost::system::error_code& errorCode);
    void remove(const types::DiscoveryEntry& discoveryEntry);
    boost::asio::steady_timer _freshnessUpdateTimer;
    boost::asio::steady_timer _reAddAllGlobalEntriesTimer;
    std::string _clusterControllerId;
    const std::vector<std::string> _knownGbids;
    std::unordered_set<std::string> _knownGbidsSet;
    const std::int64_t _defaultExpiryIntervalMs;
    const std::chrono::milliseconds _reAddInterval;

    void scheduleFreshnessUpdate();
    void scheduleReAddAllGlobalDiscoveryEntries();

    void sendAndRescheduleFreshnessUpdate(const boost::system::error_code& timerError);
    void triggerAndRescheduleReAdd(const boost::system::error_code& timerError);
    void informObserversOnAdd(const types::DiscoveryEntry& discoveryEntry);
    void informObserversOnRemove(const types::DiscoveryEntry& discoveryEntry);

    void addInternal(joynr::types::DiscoveryEntry entry,
                     bool awaitGlobalRegistration,
                     const std::vector<std::string>& gbids,
                     std::function<void()> onSuccess,
                     std::function<void(const joynr::types::DiscoveryError::Enum&)> onError);
    bool hasProviderPermission(const types::DiscoveryEntry& discoveryEntry);
};

class LocalCapabilitiesCallback : public ILocalCapabilitiesCallback
{
public:
    LocalCapabilitiesCallback(
            std::function<void(const std::vector<types::DiscoveryEntryWithMetaInfo>&)>&& onSuccess,
            std::function<void(const types::DiscoveryError::Enum&)>&& onError);
    void capabilitiesReceived(
            const std::vector<types::DiscoveryEntryWithMetaInfo>& capabilities) override;
    void onError(const types::DiscoveryError::Enum&) override;
    ~LocalCapabilitiesCallback() override = default;

private:
    DISALLOW_COPY_AND_ASSIGN(LocalCapabilitiesCallback);
    std::once_flag _onceFlag;
    std::function<void(const std::vector<types::DiscoveryEntryWithMetaInfo>&)> _onSuccess;
    std::function<void(const types::DiscoveryError::Enum&)> _onErrorCallback;
};

} // namespace joynr
#endif // LOCALCAPABILITIESDIRECTORY_H
