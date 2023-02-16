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
package io.joynr.messaging.mqtt;

import java.util.Map;

import com.hivemq.client.mqtt.mqtt5.message.publish.Mqtt5Publish;

import io.joynr.messaging.FailureAction;

public interface IMqttMessagingSkeleton {

    public void transmit(Mqtt5Publish mqtt5Publish,
                         Map<String, String> prefixedCustomHeaders,
                         FailureAction failureAction);
}
