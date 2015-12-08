/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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
#ifndef DBUSMESSAGINGSTUBFACTORY_H
#define DBUSMESSAGINGSTUBFACTORY_H

#include <QMutex>

#include "joynr/IMiddlewareMessagingStubFactory.h"

#include <memory>
#include <map>
#include <string>

namespace joynr
{

class DbusMessagingStubFactory : public IMiddlewareMessagingStubFactory
{

public:
    DbusMessagingStubFactory();
    std::shared_ptr<IMessaging> create(const joynr::system::RoutingTypes::QtAddress& destAddress);
    bool canCreate(const joynr::system::RoutingTypes::QtAddress& destAddress);

private:
    std::map<std::string, std::shared_ptr<IMessaging>> stubMap;
    QMutex mutex;
};

} // namespace joynr
#endif // DBUSMESSAGINGSTUBFACTORY_H
