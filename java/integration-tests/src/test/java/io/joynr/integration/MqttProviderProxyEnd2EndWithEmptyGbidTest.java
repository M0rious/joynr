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
package io.joynr.integration;

import static io.joynr.util.JoynrUtil.createUuidString;
import static org.junit.Assert.assertEquals;

import java.util.Properties;

import org.junit.After;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestName;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.arbitration.ArbitrationConstants;
import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.capabilities.ParticipantIdKeyUtil;
import io.joynr.integration.AbstractProviderProxyEnd2EndTest.TestProvider;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import joynr.test.JoynrTestLoggingRule;
import joynr.tests.testProvider;
import joynr.tests.testProxy;
import joynr.types.ProviderQos;

public class MqttProviderProxyEnd2EndWithEmptyGbidTest extends JoynrEnd2EndTest {

    private static final Logger logger = LoggerFactory.getLogger(MqttProviderProxyEnd2EndWithEmptyGbidTest.class);
    @Rule
    public JoynrTestLoggingRule joynrTestRule = new JoynrTestLoggingRule(logger);

    @Rule
    public TestName name = new TestName();

    // This timeout must be shared by all integration test environments and
    // cannot be too short.
    private static final int CONST_DEFAULT_TEST_TIMEOUT = 60000;

    private static final String PROVIDER_PARTICIPANTID = "providerParticipantId";

    private static Properties baseTestConfig;
    private static Properties mqttConfig;
    private static Properties mqttConfigWithEmptyGbid;
    private static int mqttBrokerPort = 1883;

    private JoynrRuntime providerRuntime;
    private JoynrRuntime consumerRuntime;

    private TestProvider provider;
    private ProviderQos testProviderQos = new ProviderQos();
    private String domain;

    // The timeouts should not be too small because some test environments are slow
    protected MessagingQos messagingQos = new MessagingQos(40000);
    protected DiscoveryQos discoveryQos;

    public MqttProviderProxyEnd2EndWithEmptyGbidTest() {
        discoveryQos = new DiscoveryQos(30000, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);
        discoveryQos.setRetryIntervalMs(5000);
    }

    @BeforeClass
    public static void setupBaseConfig() {
        baseTestConfig = new Properties();
        baseTestConfig.put(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS, "10");
        baseTestConfig.put(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_MINIMUM_RETRY_INTERVAL_MS, "200");
    }

    @BeforeClass
    public static void setupMqttConfig() {
        mqttConfig = new Properties();
        mqttConfig.put(MqttModule.PROPERTY_MQTT_BROKER_URIS, "tcp://localhost:" + mqttBrokerPort);
        mqttConfigWithEmptyGbid = new Properties();
        mqttConfigWithEmptyGbid.put(MqttModule.PROPERTY_MQTT_BROKER_URIS, "tcp://localhost:" + mqttBrokerPort);
        mqttConfigWithEmptyGbid.put(ConfigurableMessagingSettings.PROPERTY_GBIDS, "");
    }

    private JoynrRuntime getRuntime(Properties joynrConfig, Module... modules) {
        joynrConfig.putAll(mqttConfig);
        joynrConfig.putAll(baseTestConfig);
        Module runtimeModule = Modules.override(new CCInProcessRuntimeModule()).with(modules);
        Module modulesWithRuntime = Modules.override(runtimeModule).with(new HivemqMqttClientModule());

        return new JoynrInjectorFactory(joynrConfig, modulesWithRuntime).getInjector().getInstance(JoynrRuntime.class);
    }

    private JoynrRuntime getRuntimeWithEmptyGbid(Properties joynrConfig, Module... modules) {
        joynrConfig.putAll(mqttConfigWithEmptyGbid);
        joynrConfig.putAll(baseTestConfig);
        Module runtimeModule = Modules.override(new CCInProcessRuntimeModule()).with(modules);
        Module modulesWithRuntime = Modules.override(runtimeModule).with(new HivemqMqttClientModule());

        return new JoynrInjectorFactory(joynrConfig, modulesWithRuntime).getInjector().getInstance(JoynrRuntime.class);
    }

    @Before
    public void baseSetup() throws Exception {
        // prints the tests name in the log so we know what we are testing
        String methodName = name.getMethodName();
        logger.info("{} setup beginning...", methodName);

        domain = "MqttProviderProxyEnd2EndWithEmptyGbidTest." + name.getMethodName() + System.currentTimeMillis();

        // use channelNames = test name
        String channelIdProvider = "JavaTest-" + methodName + createUuidString() + "-end2endTestProvider";
        String channelIdConsumer = "JavaTest-" + methodName + createUuidString() + "-end2endConsumer";

        Properties joynrConfigProvider = new Properties();
        joynrConfigProvider.put(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL,
                                "localdomain." + createUuidString());
        joynrConfigProvider.put(MessagingPropertyKeys.CHANNELID, channelIdProvider);
        joynrConfigProvider.put(MessagingPropertyKeys.RECEIVERID, createUuidString());
        joynrConfigProvider.put(ParticipantIdKeyUtil.getProviderParticipantIdKey(domain, testProvider.class),
                                PROVIDER_PARTICIPANTID);

        providerRuntime = getRuntime(joynrConfigProvider, getSubscriptionPublisherFactoryModule());

        Properties joynrConfigConsumer = new Properties();
        joynrConfigConsumer.put(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL,
                                "localdomain." + createUuidString());
        joynrConfigConsumer.put(MessagingPropertyKeys.CHANNELID, channelIdConsumer);
        joynrConfigConsumer.put(MessagingPropertyKeys.RECEIVERID, createUuidString());

        consumerRuntime = getRuntimeWithEmptyGbid(joynrConfigConsumer, getSubscriptionPublisherFactoryModule());

        provider = new TestProvider();
        testProviderQos.setPriority(System.currentTimeMillis());

        providerRuntime.getProviderRegistrar(domain, provider)
                       .withProviderQos(testProviderQos)
                       .awaitGlobalRegistration()
                       .register()
                       .get(CONST_DEFAULT_TEST_TIMEOUT);

        logger.info("Setup finished");

    }

    @After
    public void tearDown() throws InterruptedException {
        if (consumerRuntime != null) {
            consumerRuntime.shutdown(true);
        }

        if (providerRuntime != null) {
            providerRuntime.unregisterProvider(domain, provider);
            // wait grace period for the unregister (remove) message to get
            // sent to global discovery
            Thread.sleep(1000);
            providerRuntime.shutdown(true);
        }
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void createProxyViaDomainInterfaceAndCallMethod() {
        int result;
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        result = proxy.addNumbers(6, 3, 2);
        assertEquals(11, result);

    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void createProxyViaParticipantIdAndCallMethod() {
        int result;
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.FixedChannel);
        discoveryQos.addCustomParameter(ArbitrationConstants.FIXEDPARTICIPANT_KEYWORD, PROVIDER_PARTICIPANTID);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        result = proxy.addNumbers(6, 3, 2);
        assertEquals(11, result);

    }

}
