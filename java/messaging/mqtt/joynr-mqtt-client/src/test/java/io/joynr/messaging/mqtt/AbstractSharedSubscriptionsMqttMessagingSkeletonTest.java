/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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
package io.joynr.messaging.mqtt;

import static io.joynr.messaging.mqtt.MqttMessagingSkeletonTestUtil.createTestMessage;
import static io.joynr.messaging.mqtt.MqttMessagingSkeletonTestUtil.failIfCalledAction;
import static io.joynr.messaging.mqtt.MqttMessagingSkeletonTestUtil.feedMqttSkeletonWithRequests;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.startsWith;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.lang.reflect.Field;
import java.util.List;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;

import io.joynr.messaging.routing.MessageProcessedHandler;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.statusmetrics.JoynrStatusMetricsReceiver;
import io.joynr.util.ObjectMapper;
import joynr.Message;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.RoutingTypesUtil;

public abstract class AbstractSharedSubscriptionsMqttMessagingSkeletonTest {

    protected int maxMqttMessagesInQueue = 20;
    protected boolean backpressureEnabled = false;
    protected int backpressureIncomingMqttRequestsUpperThreshold = 80;
    protected int backpressureIncomingMqttRequestsLowerThreshold = 20;
    protected String ownGbid = "testOwnGbid";
    @Mock
    protected MqttClientFactory mqttClientFactory;
    @Mock
    protected JoynrMqttClient mqttClient;
    @Mock
    protected JoynrMqttClient mqttReplyClient;
    protected final String ownTopic = "testOwnTopic";
    protected final String replyToTopic = "testReplyToTopic";
    @Mock
    protected MessageRouter messageRouter;
    @Mock
    protected MessageProcessedHandler messageProcessedHandler;
    @Mock
    protected RoutingTable routingTable;
    @Mock
    protected MqttTopicPrefixProvider mqttTopicPrefixProvider;
    @Mock
    protected JoynrStatusMetricsReceiver mockJoynrStatusMetrics;
    protected SharedSubscriptionsMqttMessagingSkeleton subject;

    protected boolean separateReplyConnection;

    @Before
    public void setup() throws Exception {
        Field objectMapperField = RoutingTypesUtil.class.getDeclaredField("objectMapper");
        objectMapperField.setAccessible(true);
        objectMapperField.set(RoutingTypesUtil.class, new ObjectMapper());
    }

    protected void initMocks(boolean separateReplyConnection) {
        this.separateReplyConnection = separateReplyConnection;
        when(mqttClientFactory.createReceiver(ownGbid)).thenReturn(mqttClient);
        when(mqttClientFactory.createSender(ownGbid)).thenReturn(mqttClient);
        if (separateReplyConnection) {
            when(mqttClientFactory.createReplyReceiver(ownGbid)).thenReturn(mqttReplyClient);
        } else {
            when(mqttClientFactory.createReplyReceiver(ownGbid)).thenReturn(mqttClient);
        }
    }

    protected abstract void createSkeleton(String channelId);

    protected abstract void initAndSubscribe();

    private void createAndInitSkeleton(String channelId) {
        createSkeleton(channelId);
        initAndSubscribe();
    }

    private void triggerAndVerifySharedSubscriptionsTopicUnsubscribeAndSubscribeCycle(String expectedChannelId,
                                                                                      int expectedTotalUnsubscribeCallCount,
                                                                                      int expectedTotalSubscribeCallCount) throws Exception {
        initMocks(false);
        final String expectedSharedSubscriptionsTopicPrefix = "$share/" + expectedChannelId + "/";

        final int mqttRequestsToHitUpperThreshold = (maxMqttMessagesInQueue
                * backpressureIncomingMqttRequestsUpperThreshold) / 100;
        final int mqttRequestsToHitLowerThreshold = (maxMqttMessagesInQueue
                * backpressureIncomingMqttRequestsLowerThreshold) / 100;

        // fill up with requests and verify unsubscribe
        List<String> messageIds = feedMqttSkeletonWithRequests(subject, mqttRequestsToHitUpperThreshold + 1);
        verify(mqttClient,
               times(expectedTotalUnsubscribeCallCount)).unsubscribe(startsWith(expectedSharedSubscriptionsTopicPrefix));

        // finish processing of as many requests needed to drop below lower threshold
        final int numOfRequestsToProcess = mqttRequestsToHitUpperThreshold - mqttRequestsToHitLowerThreshold + 2;
        for (int i = 0; i < numOfRequestsToProcess; i++) {
            subject.messageProcessed(messageIds.get(i));
        }

        // verify that now subscribe is triggered again
        verify(mqttClient,
               times(expectedTotalSubscribeCallCount)).subscribe(startsWith(expectedSharedSubscriptionsTopicPrefix));

        // cleanup: process the rest of the messages in order to have 0 pending requests
        for (int i = numOfRequestsToProcess; i < messageIds.size(); i++) {
            subject.messageProcessed(messageIds.get(i));
        }
    }

    @Test
    public void testSubscribesToSharedSubscription() {
        initMocks(false);
        createSkeleton("channelId");
        verify(mqttClient, times(0)).subscribe(any(String.class));
        initAndSubscribe();
        verify(mqttClient).subscribe(eq("$share/channelId/" + ownTopic + "/#"));
        verify(mqttClient).subscribe(eq(replyToTopic + "/#"));
    }

    @Test
    public void subscribeToSharedTopicSubscribesToSharedTopic() {
        initMocks(false);
        createSkeleton("channelId");
        verify(mqttClient, times(0)).subscribe(any(String.class));
        subject.subscribeToSharedTopic();
        verify(mqttClient).subscribe(eq("$share/channelId/" + ownTopic + "/#"));
    }

    @Test
    public void subscribeToSharedTopicDoesNotSubscribeToReplyToTopic() {
        initMocks(false);
        createSkeleton("channelId");
        verify(mqttClient, times(0)).subscribe(any(String.class));
        subject.subscribeToSharedTopic();
        verify(mqttClient, times(0)).subscribe(startsWith(replyToTopic));
    }

    @Test
    public void subscribeToReplyToTopicSubscribesToReplyToTopic() {
        initMocks(false);
        createSkeleton("channelId");
        verify(mqttClient, times(0)).subscribe(any(String.class));
        subject.subscribeToReplyTopic();
        verify(mqttClient).subscribe(eq(replyToTopic + "/#"));
    }

    @Test
    public void subscribeToReplyToTopicSubscribesToReplyToTopic_separateReplyClient() {
        initMocks(true);
        createSkeleton("channelId");
        verify(mqttClient, times(0)).subscribe(any(String.class));
        subject.subscribeToReplyTopic();
        verify(mqttClient, times(0)).subscribe(any(String.class));
        verify(mqttReplyClient).subscribe(replyToTopic + "/#");
    }

    @Test
    public void subscribeToReplyToTopicDoesNotSubscribeToSharedTopic() {
        initMocks(false);
        createSkeleton("channelId");
        verify(mqttClient, times(0)).subscribe(any(String.class));
        subject.subscribeToReplyTopic();
        verify(mqttClient, times(0)).subscribe(startsWith("$share/"));
    }

    @Test
    public void testChannelIdStrippedOfNonAlphaChars() {
        initMocks(false);
        createAndInitSkeleton("channel@123_bling$$");
        verify(mqttClient).subscribe(startsWith("$share/channelbling/"));
    }

    @Test(expected = IllegalArgumentException.class)
    public void testIllegalChannelId() {
        initMocks(false);
        createAndInitSkeleton("@123_$$-!");
    }

    @Test(expected = IllegalArgumentException.class)
    public void testInvalidBackpressureParametersNoMaxMqttMessagesInQueue() {
        initMocks(false);
        backpressureEnabled = true;
        maxMqttMessagesInQueue = 0;

        createAndInitSkeleton("channelId");
    }

    @Test(expected = IllegalArgumentException.class)
    public void testInvalidBackpressureParametersInvalidUpperThreshold() {
        initMocks(false);
        backpressureEnabled = true;
        backpressureIncomingMqttRequestsUpperThreshold = 101;

        createAndInitSkeleton("channelId");
    }

    @Test(expected = IllegalArgumentException.class)
    public void testInvalidBackpressureParametersInvalidLowerThreshold() {
        initMocks(false);
        backpressureEnabled = true;
        backpressureIncomingMqttRequestsLowerThreshold = -1;

        createAndInitSkeleton("channelId");
    }

    @Test(expected = IllegalArgumentException.class)
    public void testInvalidBackpressureParametersLowerThresholdAboveUpper() {
        initMocks(false);
        backpressureEnabled = true;
        backpressureIncomingMqttRequestsUpperThreshold = 70;
        backpressureIncomingMqttRequestsLowerThreshold = 75;

        createAndInitSkeleton("channelId");
    }

    @Test
    public void testBackpressureTriggersUnsubscribeWhenUpperThresholdHit() throws Exception {
        initMocks(false);
        backpressureEnabled = true;
        createAndInitSkeleton("channelIdBackpressure");

        doReturn(true).when(routingTable).put(anyString(), any(Address.class), anyBoolean(), anyLong());
        final int mqttRequestsToHitUpperThreshold = (maxMqttMessagesInQueue
                * backpressureIncomingMqttRequestsUpperThreshold) / 100;
        feedMqttSkeletonWithRequests(subject, mqttRequestsToHitUpperThreshold - 1);

        // just below threshold, still no unsubscribe call expected
        verify(mqttClient, times(0)).unsubscribe(any(String.class));

        // messages that are not of request type should not trigger unsubscribe as well
        subject.transmit(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_REPLY).getSerializedMessage(),
                         createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_REPLY).getPrefixedCustomHeaders(),
                         failIfCalledAction);
        subject.transmit(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST).getSerializedMessage(),
                         createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST).getPrefixedCustomHeaders(),
                         failIfCalledAction);
        verify(mqttClient, times(0)).unsubscribe(any(String.class));

        // a further request should hit the threshold value and trigger an unsubscribe
        subject.transmit(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST).getSerializedMessage(),
                         createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST).getPrefixedCustomHeaders(),
                         failIfCalledAction);
        verify(mqttClient).unsubscribe(startsWith("$share/channelIdBackpressure/"));
    }

    @Test
    public void testBackpressureTriggersResubscribeWhenDroppedBelowLowerThreshold() throws Exception {
        initMocks(false);
        backpressureEnabled = true;
        final String channelId = "channelIdBackpressureOneCycle";
        createAndInitSkeleton(channelId);
        doReturn(true).when(routingTable).put(anyString(), any(Address.class), anyBoolean(), anyLong());

        final int expectedTotalUnsubscribeCallCount = 1;
        final int expectedTotalSubscribeCallCount = 2; // one call is from the init method of the skeleton
        triggerAndVerifySharedSubscriptionsTopicUnsubscribeAndSubscribeCycle(channelId,
                                                                             expectedTotalUnsubscribeCallCount,
                                                                             expectedTotalSubscribeCallCount);
    }

    @Test
    public void testBackpressureTriggersUnsubscribeAndSubscribeRepeatedly() throws Exception {
        initMocks(false);
        backpressureEnabled = true;
        final String channelId = "channelIdBackpressureMultipleCycles";
        createAndInitSkeleton(channelId);

        doReturn(true).when(routingTable).put(anyString(), any(Address.class), anyBoolean(), anyLong());

        final int numCycles = 10;
        for (int i = 1; i <= numCycles; i++) {
            final int expectedTotalUnsubscribeCallCount = i;
            final int expectedTotalSubscribeCallCount = i + 1; // one call is from the init method of the skeleton
            triggerAndVerifySharedSubscriptionsTopicUnsubscribeAndSubscribeCycle(channelId,
                                                                                 expectedTotalUnsubscribeCallCount,
                                                                                 expectedTotalSubscribeCallCount);
        }
    }

}
