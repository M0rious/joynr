/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
#ifndef LCDUTIL_H
#define LCDUTIL_H

#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/functional/hash.hpp>
#include <boost/optional.hpp>

#include "joynr/InterfaceAddress.h"
#include "joynr/Logger.h"
#include "joynr/types/DiscoveryEntry.h"

namespace joynr
{

namespace types
{
class DiscoveryEntry;
class DiscoveryEntryWithMetaInfo;
class GlobalDiscoveryEntry;
} // namespace types

struct DiscoveryEntryHash {
    std::size_t operator()(const types::DiscoveryEntry& entry) const
    {
        std::size_t seed = 0;
        boost::hash_combine(seed, entry.getParticipantId());
        return seed;
    }
};

struct DiscoveryEntryKeyEq {
    bool operator()(const types::DiscoveryEntry& lhs, const types::DiscoveryEntry& rhs) const
    {
        // there is no need to check typeid because entries are of the same type.
        return joynr::util::compareValues(lhs.getParticipantId(), rhs.getParticipantId());
    }
};

struct ValidateGBIDsEnum {
    enum Enum : std::uint32_t { OK = 0, INVALID = 1, UNKNOWN = 2 };
};

class LCDUtil
{
public:
    static ValidateGBIDsEnum::Enum validateGbids(std::vector<std::string> gbids,
                                                 std::unordered_set<std::string> validGbids);

    static std::vector<types::DiscoveryEntry> filterDiscoveryEntriesByGbids(
            const std::unique_lock<std::recursive_mutex>& cacheLock,
            const std::vector<types::DiscoveryEntry>& entries,
            const std::unordered_set<std::string>& gbids,
            std::unordered_map<std::string, std::vector<std::string>>
                    globalParticipantIdsToGbidsMap);

    static std::vector<types::DiscoveryEntryWithMetaInfo> filterDuplicates(
            std::vector<types::DiscoveryEntryWithMetaInfo>&& globalCapabilitiesWithMetaInfo,
            std::vector<types::DiscoveryEntryWithMetaInfo>&& localCapabilitiesWithMetaInfo);
    static bool containsOnlyEmptyString(const std::vector<std::string> gbids);

    static void replaceGbidWithEmptyString(
            std::vector<joynr::types::GlobalDiscoveryEntry>& capabilities);

    static std::vector<types::DiscoveryEntry> optionalToVector(
            boost::optional<types::DiscoveryEntry> optionalEntry);

    static bool isGlobal(const types::DiscoveryEntry& discoveryEntry);

    static std::string joinToString(const std::vector<types::DiscoveryEntry>& discoveryEntries);

    static bool isEntryForGbid(const std::unique_lock<std::recursive_mutex>& cacheLock,
                               const types::DiscoveryEntry& entry,
                               const std::unordered_set<std::string> gbids,
                               std::unordered_map<std::string, std::vector<std::string>>
                                       globalParticipantIdsToGbidsMap);
    static types::GlobalDiscoveryEntry toGlobalDiscoveryEntry(
            const types::DiscoveryEntry& discoveryEntry,
            const std::string& localAddress);

    static std::vector<InterfaceAddress> getInterfaceAddresses(
            const std::vector<std::string>& domains,
            const std::string& interfaceName);

    static types::DiscoveryEntryWithMetaInfo convert(bool isLocal,
                                                     const types::DiscoveryEntry& entry);
    static std::vector<types::DiscoveryEntryWithMetaInfo> convert(
            bool isLocal,
            const std::vector<types::DiscoveryEntry>& entries);

private:
    ADD_LOGGER(LCDUtil)
};

} // namespace joynr

#endif // LCDUTIL_H
