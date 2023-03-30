/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;

import java.util.Properties;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.name.Names;

import io.joynr.common.JoynrPropertiesModule;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.inprocess.InProcessMessagingSkeleton;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.BinderAddress;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;
import joynr.system.RoutingTypes.WebSocketProtocol;

@RunWith(MockitoJUnitRunner.class)
public class CcRoutingTableAddressValidatorTest {

    private CcRoutingTableAddressValidator validator;

    private final String brokerUri = "ownBrokerUri";
    private final String globalTopic = "ownTopic";
    private final String replyToTopic = "ownTopic";
    private final MqttAddress globalAddress = new MqttAddress(brokerUri, globalTopic);
    private final MqttAddress replyToAddress = new MqttAddress(brokerUri, replyToTopic);

    @Before
    public void setup() {
        // This method is required to avoid nullptrexceptions
        Injector injector = Guice.createInjector(new AbstractModule() {
            @Override
            protected void configure() {
                bind(Address.class).annotatedWith(Names.named(MessagingPropertyKeys.GLOBAL_ADDRESS))
                                   .toInstance(globalAddress);
                bind(Address.class).annotatedWith(Names.named(MessagingPropertyKeys.REPLY_TO_ADDRESS))
                                   .toInstance(replyToAddress);
            }
        }, new JoynrPropertiesModule(new Properties()));
        validator = injector.getInstance(CcRoutingTableAddressValidator.class);
    }

    private void initValidator(boolean setGlobalAddress, boolean setReplyToAddress) {
        Injector injector = Guice.createInjector(new AbstractModule() {
            @Override
            protected void configure() {
                if (setGlobalAddress) {
                    bind(Address.class).annotatedWith(Names.named(MessagingPropertyKeys.GLOBAL_ADDRESS))
                                       .toInstance(globalAddress);
                }
                if (setReplyToAddress) {
                    bind(Address.class).annotatedWith(Names.named(MessagingPropertyKeys.REPLY_TO_ADDRESS))
                                       .toInstance(replyToAddress);
                }
            }
        }, new JoynrPropertiesModule(new Properties()));
        validator = injector.getInstance(CcRoutingTableAddressValidator.class);
    }

    @Test
    public void multipleOwnAddresses() {
        initValidator(true, true);

        final MqttAddress testGlobalAddress = new MqttAddress(brokerUri, globalTopic);
        final MqttAddress testReplyToAddress = new MqttAddress(brokerUri, replyToTopic);

        assertFalse(validator.isValidForRoutingTable(testGlobalAddress));
        assertFalse(validator.isValidForRoutingTable(testReplyToAddress));
    }

    private void otherAddressesOfOwnAddressTypeAreValid() {
        Address otherMqttAddress = new MqttAddress(brokerUri, "otherTopic");
        assertTrue(validator.isValidForRoutingTable(otherMqttAddress));

        otherMqttAddress = new MqttAddress("otherBroker", globalTopic);
        assertTrue(validator.isValidForRoutingTable(otherMqttAddress));

        otherMqttAddress = new MqttAddress("otherBroker", "otherTopic");
        assertTrue(validator.isValidForRoutingTable(otherMqttAddress));
    }

    @Test
    public void otherAddressesOfGlobalAddressTypeAreValid() {
        initValidator(true, false);
        assertFalse(validator.isValidForRoutingTable(globalAddress));

        otherAddressesOfOwnAddressTypeAreValid();
    }

    @Test
    public void otherAddressesOfReplyToAddressTypeAreValid() {
        initValidator(false, true);
        assertFalse(validator.isValidForRoutingTable(replyToAddress));

        otherAddressesOfOwnAddressTypeAreValid();
    }

    @Test
    public void webSocketAddressTypeIsNotValid() {
        assertFalse(validator.isValidForRoutingTable(new WebSocketAddress()));
    }

    @Test
    public void otherAddressTypesAreValid() {
        initValidator(true, false);
        assertFalse(validator.isValidForRoutingTable(globalAddress));

        Address otherAddress = new WebSocketClientAddress();
        assertTrue(validator.isValidForRoutingTable(otherAddress));

        otherAddress = new InProcessAddress();
        assertTrue(validator.isValidForRoutingTable(otherAddress));
    }

    @Test
    public void allowUpdateOfInProcessAddress() {
        // inProcessAddress can only be replaced with InProcessAddress
        final boolean isGloballyVisible = true;
        final long expiryDateMs = Long.MAX_VALUE;
        final InProcessAddress oldAddress = new InProcessAddress(mock(InProcessMessagingSkeleton.class));
        final RoutingEntry oldEntry = new RoutingEntry(oldAddress, isGloballyVisible, expiryDateMs, false);

        final MqttAddress mqttAddress = new MqttAddress("brokerUri", "topic");
        final RoutingEntry newEntry = new RoutingEntry(mqttAddress, isGloballyVisible, expiryDateMs, false);
        assertFalse(validator.allowUpdate(oldEntry, newEntry));

        final WebSocketAddress websocketAddress = new WebSocketAddress(WebSocketProtocol.WS, "host", 4242, "path");
        newEntry.setAddress(websocketAddress);
        assertFalse(validator.allowUpdate(oldEntry, newEntry));

        final WebSocketClientAddress websocketClientAddress = new WebSocketClientAddress("webSocketId");
        newEntry.setAddress(websocketClientAddress);
        assertFalse(validator.allowUpdate(oldEntry, newEntry));

        final InProcessAddress inProcessAddress = new InProcessAddress();
        newEntry.setAddress(inProcessAddress);
        assertTrue(validator.allowUpdate(oldEntry, newEntry));
    }

    @Test
    public void allowUpdateOfWebSocketClientAddress() {
        // precedence: InProcessAddress > WebSocketClientAddress > MqttAddress > WebSocketAddress
        final boolean isGloballyVisible = true;
        final long expiryDateMs = Long.MAX_VALUE;
        final WebSocketClientAddress oldAddress = new WebSocketClientAddress("testWebSocketId");
        final RoutingEntry oldEntry = new RoutingEntry(oldAddress, isGloballyVisible, expiryDateMs, false);

        final MqttAddress mqttAddress = new MqttAddress("brokerUri", "topic");
        final RoutingEntry newEntry = new RoutingEntry(mqttAddress, isGloballyVisible, expiryDateMs, false);
        assertFalse(validator.allowUpdate(oldEntry, newEntry));

        final WebSocketAddress websocketAddress = new WebSocketAddress(WebSocketProtocol.WS, "host", 4242, "path");
        newEntry.setAddress(websocketAddress);
        assertFalse(validator.allowUpdate(oldEntry, newEntry));

        final WebSocketClientAddress websocketClientAddress = new WebSocketClientAddress("webSocketId");
        newEntry.setAddress(websocketClientAddress);
        assertTrue(validator.allowUpdate(oldEntry, newEntry));

        final InProcessAddress inProcessAddress = new InProcessAddress();
        newEntry.setAddress(inProcessAddress);
        assertTrue(validator.allowUpdate(oldEntry, newEntry));
    }

    @Test
    public void allowUpdateOfWebSocketAddress() {
        // precedence: InProcessAddress > WebSocketClientAddress > MqttAddress > WebSocketAddress
        final boolean isGloballyVisible = true;
        final long expiryDateMs = Long.MAX_VALUE;
        final WebSocketAddress oldAddress = new WebSocketAddress(WebSocketProtocol.WSS, "testHost", 23, "testPath");
        final RoutingEntry oldEntry = new RoutingEntry(oldAddress, isGloballyVisible, expiryDateMs, false);

        final MqttAddress mqttAddress = new MqttAddress("brokerUri", "topic");
        final RoutingEntry newEntry = new RoutingEntry(mqttAddress, isGloballyVisible, expiryDateMs, false);
        assertTrue(validator.allowUpdate(oldEntry, newEntry));

        final WebSocketAddress websocketAddress = new WebSocketAddress(WebSocketProtocol.WS, "host", 4242, "path");
        newEntry.setAddress(websocketAddress);
        assertTrue(validator.allowUpdate(oldEntry, newEntry));

        final WebSocketClientAddress websocketClientAddress = new WebSocketClientAddress("webSocketId");
        newEntry.setAddress(websocketClientAddress);
        assertTrue(validator.allowUpdate(oldEntry, newEntry));

        final InProcessAddress inProcessAddress = new InProcessAddress();
        newEntry.setAddress(inProcessAddress);
        assertTrue(validator.allowUpdate(oldEntry, newEntry));
    }

    @Test
    public void allowUpdateOfMqttAddress() {
        // precedence: InProcessAddress > WebSocketClientAddress > MqttAddress > WebSocketAddress
        final boolean isGloballyVisible = true;
        final long expiryDateMs = Long.MAX_VALUE;
        final MqttAddress oldAddress = new MqttAddress("testBrokerUri", "testTopic");
        final RoutingEntry oldEntry = new RoutingEntry(oldAddress, isGloballyVisible, expiryDateMs, false);

        final MqttAddress mqttAddress = new MqttAddress("brokerUri", "topic");
        final RoutingEntry newEntry = new RoutingEntry(mqttAddress, isGloballyVisible, expiryDateMs, false);
        assertTrue(validator.allowUpdate(oldEntry, newEntry));

        final WebSocketAddress websocketAddress = new WebSocketAddress(WebSocketProtocol.WS, "host", 4242, "path");
        newEntry.setAddress(websocketAddress);
        assertFalse(validator.allowUpdate(oldEntry, newEntry));

        final WebSocketClientAddress websocketClientAddress = new WebSocketClientAddress("webSocketId");
        newEntry.setAddress(websocketClientAddress);
        assertTrue(validator.allowUpdate(oldEntry, newEntry));

        final InProcessAddress inProcessAddress = new InProcessAddress();
        newEntry.setAddress(inProcessAddress);
        assertTrue(validator.allowUpdate(oldEntry, newEntry));
    }

    @Test
    public void allowUpdateOfBinderAddress() {
        // precedence: InProcessAddress > WebSocketClientAddress/UdsClientAddress/BinderAddress > MqttAddress > WebSocketAddress/UdsAddress
        final boolean isGloballyVisible = true;
        final long expiryDateMs = Long.MAX_VALUE;
        final BinderAddress oldAddress = new BinderAddress("test.package.name", 0);
        final RoutingEntry oldEntry = new RoutingEntry(oldAddress, isGloballyVisible, expiryDateMs, false);

        final MqttAddress mqttAddress = new MqttAddress("brokerUri", "topic");
        final RoutingEntry newEntry = new RoutingEntry(mqttAddress, isGloballyVisible, expiryDateMs, false);
        assertFalse(validator.allowUpdate(oldEntry, newEntry));

        final BinderAddress binderAddress = new BinderAddress("package.name", 1);
        newEntry.setAddress(binderAddress);
        assertTrue(validator.allowUpdate(oldEntry, newEntry));

        final WebSocketClientAddress websocketClientAddress = new WebSocketClientAddress("webSocketId");
        newEntry.setAddress(websocketClientAddress);
        assertTrue(validator.allowUpdate(oldEntry, newEntry));

        final InProcessAddress inProcessAddress = new InProcessAddress();
        newEntry.setAddress(inProcessAddress);
        assertTrue(validator.allowUpdate(oldEntry, newEntry));
    }

}
