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

#include "TestRunnable.h"

#include <thread>

namespace joynr
{

TestRunnable::TestRunnable() : Runnable()
{
}

void TestRunnable::shutdown()
{
    JOYNR_LOG_TRACE(logger(), "shutdown called...");
}

void TestRunnable::run()
{
    JOYNR_LOG_TRACE(logger(), "run: entering...");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    JOYNR_LOG_TRACE(logger(), "run: leaving...");
}

} // namespace joynr
