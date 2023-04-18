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
#include "joynr/CapabilitiesRegistrar.h"

namespace joynr
{

class PublicationManager;

CapabilitiesRegistrar::CapabilitiesRegistrar(
        std::vector<std::shared_ptr<IDispatcher>> dispatcherList,
        std::shared_ptr<system::IDiscoveryAsync> discoveryProxy,
        std::shared_ptr<ParticipantIdStorage> participantIdStorage,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> dispatcherAddress,
        std::shared_ptr<IMessageRouter> messageRouter,
        std::int64_t defaultExpiryIntervalMs,
        std::weak_ptr<PublicationManager> publicationManager,
        const std::string& globalAddress)
        : _dispatcherList(std::move(dispatcherList)),
          _discoveryProxy(discoveryProxy),
          _participantIdStorage(std::move(participantIdStorage)),
          _dispatcherAddress(std::move(dispatcherAddress)),
          _messageRouter(std::move(messageRouter)),
          _defaultExpiryIntervalMs(defaultExpiryIntervalMs),
          _publicationManager(std::move(publicationManager)),
          _globalAddress(globalAddress)
{
}

void CapabilitiesRegistrar::removeAsync(
        const std::string& participantId,
        std::function<void()> onSuccess,
        std::function<void(const exceptions::JoynrRuntimeException&)> onError) noexcept
{
    auto onSuccessWrapper = [
        dispatcherList = this->_dispatcherList,
        messageRouter = util::as_weak_ptr(_messageRouter),
        participantId,
        onSuccess = std::move(onSuccess),
        onError
    ]()
    {
        auto onSuccessWrapper =
                [ dispatcherList, participantId, onSuccess = std::move(onSuccess) ]()
        {
            for (std::shared_ptr<IDispatcher> currentDispatcher : dispatcherList) {
                currentDispatcher->removeRequestCaller(participantId);
            }
            onSuccess();
        };

        if (auto ptr = messageRouter.lock()) {
            ptr->removeNextHop(participantId, std::move(onSuccessWrapper), std::move(onError));
        }
    };

    _discoveryProxy->removeAsync(participantId, std::move(onSuccessWrapper), std::move(onError));
}

void CapabilitiesRegistrar::addDispatcher(std::shared_ptr<IDispatcher> dispatcher)
{
    _dispatcherList.push_back(std::move(dispatcher));
}

void CapabilitiesRegistrar::removeDispatcher(std::shared_ptr<IDispatcher> dispatcher)
{
    util::removeAll(_dispatcherList, dispatcher);
}

} // namespace joynr
