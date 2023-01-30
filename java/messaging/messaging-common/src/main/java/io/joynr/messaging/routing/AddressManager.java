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
package io.joynr.messaging.routing;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Optional;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;

import io.joynr.exceptions.JoynrRuntimeException;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.Message.MessageType;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;

public class AddressManager {
    private static final Logger logger = LoggerFactory.getLogger(AddressManager.class);
    public static final String multicastAddressCalculatorParticipantId = "joynr.internal.multicastAddressCalculatorParticipantId";

    private final MulticastReceiverRegistry multicastReceiversRegistry;

    private RoutingTable routingTable;
    private MulticastAddressCalculator multicastAddressCalculator;

    @Inject
    public AddressManager(RoutingTable routingTable,
                          Optional<MulticastAddressCalculator> multicastAddressCalculator,
                          MulticastReceiverRegistry multicastReceiverRegistry) {
        logger.trace("Initialised with routingTable: {}, multicastAddressCalculator: {}, multicastReceiverRegistry: {}",
                     routingTable,
                     multicastAddressCalculator.orElse(null),
                     multicastReceiverRegistry);
        this.routingTable = routingTable;
        this.multicastReceiversRegistry = multicastReceiverRegistry;
        this.multicastAddressCalculator = multicastAddressCalculator.orElse(null);

    }

    /**
     * Get the participantIds to which the passed in message should be sent to grouped by their address.
     * <ul>
     * <li> For multicast messages, the returned map can have multiple entries with multiple participantIds.
     * <li> For non multicast messages, the returned map always has a single entry: a dummy address mapped to the
     * participantId of the recipient.
     * </ul>
     *
     * @param message the message for which we want to find the participantIds to send it to.
     * @return map of participantIds to send the message to grouped by their addresses. Will not be null, because if
     * a participantId can't be determined, the returned map will be empty.
     */
    public Map<Address, Set<String>> getParticipantIdMap(ImmutableMessage message) {
        HashMap<Address, Set<String>> result = new HashMap<>();
        if (MessageType.VALUE_MESSAGE_TYPE_MULTICAST.equals(message.getType())) {

            Set<String> localReceivers = getLocalMulticastReceiversFromRegistry(message);
            for (String r : localReceivers) {
                Address address = routingTable.get(r);
                if (address == null) {
                    logger.error("No address found for multicast receiver {} for {}", r, message.getTrackingInfo());
                    continue;
                }
                Set<String> receiverSet = result.get(address);
                if (receiverSet == null) {
                    receiverSet = new HashSet<String>();
                    result.put(address, receiverSet);
                }
                receiverSet.add(r);
            }

            if (!message.isReceivedFromGlobal() && multicastAddressCalculator != null) {
                if (multicastAddressCalculator.createsGlobalTransportAddresses()) {
                    // only global providers should multicast to the "outside world"
                    if (isProviderGloballyVisible(message.getSender())) {
                        addReceiversFromAddressCalculator(message, result);
                    }
                } else {
                    // in case the address calculator does not provide an address
                    // to the "outside world" it is safe to forward the message
                    // regardless of the provider being globally visible or not
                    addReceiversFromAddressCalculator(message, result);
                }
            }
        } else {
            String toParticipantId = message.getRecipient();
            if (toParticipantId != null) {
                result.put(new Address(), Set.of(toParticipantId));
            }
        }
        logger.trace("Found the following recipients for {}: {}", message, result);
        return result;
    }

    /**
     * Get the address to which the passed in message should be sent to.
     * This can be an address contained in the {@link RoutingTable}, or a
     * multicast address calculated from the header content of the message.
     * <p>
     * If the message has multiple recipients (this should only happen for multicast messages), this methods expects
     * that all recipients have the same address, see {@link #getParticipantIdMap(ImmutableMessage)}).
     *
     * @param message the message for which we want to find an address to send it to.
     * @return Optional of an address to send the message to. Will not be null, because if an address
     * can't be determined, the returned Optional will be empty.
     */
    public Optional<Address> getAddressForDelayableImmutableMessage(DelayableImmutableMessage message) {
        Map<String, String> customHeader = message.getMessage().getCustomHeaders();
        final String gbidVal = customHeader.get(Message.CUSTOM_HEADER_GBID_KEY);
        Address address = null;
        Set<String> recipients = message.getRecipients();
        for (String recipient : recipients) {
            // Return the first non null address. All recipients of a message have the same address, see getAddressesMap().
            if (address != null) {
                break;
            } else if (recipient.startsWith(multicastAddressCalculatorParticipantId)) {
                address = determineAddressFromMulticastAddressCalculator(message, recipient);
            } else if (gbidVal == null) {
                address = routingTable.get(recipient);
            } else {
                address = routingTable.get(recipient, gbidVal);
            }
        }
        return address == null ? Optional.empty() : Optional.of(address);
    }

    private Address determineAddressFromMulticastAddressCalculator(DelayableImmutableMessage message,
                                                                   String recipient) {
        Address address = null;
        Set<Address> addressSet = multicastAddressCalculator.calculate(message.getMessage());
        if (addressSet.size() <= 1) {
            for (Address calculatedAddress : addressSet) {
                address = calculatedAddress;
            }
        } else {
            // This case can only happen if we have multiple backends, which can only happen in case of MQTT
            for (Address calculatedAddress : addressSet) {
                MqttAddress mqttAddress = (MqttAddress) calculatedAddress;
                String brokerUri = mqttAddress.getBrokerUri();
                if (recipient.equals(multicastAddressCalculatorParticipantId + "_" + brokerUri)) {
                    address = calculatedAddress;
                    break;
                }
            }
        }
        return address;
    }

    private boolean isProviderGloballyVisible(String participantId) {
        boolean isGloballyVisible = false;

        try {
            isGloballyVisible = routingTable.getIsGloballyVisible(participantId);
        } catch (JoynrRuntimeException e) {
            // This should never happen
            logger.error("No routing entry found for Multicast Provider {}. The message will not be published globally.",
                         participantId);
        }
        return isGloballyVisible;
    }

    private void addReceiversFromAddressCalculator(ImmutableMessage message, HashMap<Address, Set<String>> result) {
        Set<Address> calculatedAddresses = multicastAddressCalculator.calculate(message);
        if (calculatedAddresses.size() == 1) {
            result.put(calculatedAddresses.iterator().next(), Set.of(multicastAddressCalculatorParticipantId));
        } else {
            // This case can only happen if we have multiple backends, which can only happen in case of MQTT
            for (Address address : calculatedAddresses) {
                MqttAddress mqttAddress = (MqttAddress) address;
                result.put(address, Set.of(multicastAddressCalculatorParticipantId + "_" + mqttAddress.getBrokerUri()));
            }
        }
    }

    private Set<String> getLocalMulticastReceiversFromRegistry(ImmutableMessage message) {
        return multicastReceiversRegistry.getReceivers(message.getRecipient());
    }
}
