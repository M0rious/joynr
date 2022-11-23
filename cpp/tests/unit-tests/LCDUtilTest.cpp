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
#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

#include <boost/optional.hpp>

#include "joynr/LCDUtil.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"
#include "joynr/types/GlobalDiscoveryEntry.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/types/Version.h"

using namespace ::testing;
using namespace joynr;

TEST(LCDUtilTest, test_validateGbids_validGbid)
{
    std::string validGbid = "validGbid";
    std::unordered_set<std::string> validGbids;
    std::vector<std::string> testGbids;
    validGbids.insert(validGbid);
    testGbids.push_back(validGbid);
    ASSERT_EQ(ValidateGBIDsEnum::OK, LCDUtil::validateGbids(testGbids, validGbids));
}

TEST(LCDUtilTest, test_validateGbids_emptyGbid)
{
    std::string validGbid = "validGbid";
    std::string emptyGbid = "";
    std::unordered_set<std::string> validGbids;
    std::vector<std::string> testGbids;
    validGbids.insert(validGbid);
    testGbids.push_back(emptyGbid);
    ASSERT_EQ(ValidateGBIDsEnum::INVALID, LCDUtil::validateGbids(testGbids, validGbids));
}

TEST(LCDUtilTest, test_validateGbids_duplicateGbids)
{
    std::string validGbid = "validGbid";
    std::unordered_set<std::string> validGbids;
    std::vector<std::string> testGbids;
    validGbids.insert(validGbid);
    testGbids.push_back(validGbid);
    testGbids.push_back(validGbid);
    ASSERT_EQ(ValidateGBIDsEnum::INVALID, LCDUtil::validateGbids(testGbids, validGbids));
}

TEST(LCDUtilTest, test_validateGbids_unknownGbid)
{
    std::string validGbid = "validGbid";
    std::string unknownGbid = "unknownGbid";
    std::unordered_set<std::string> validGbids;
    std::vector<std::string> testGbids;
    validGbids.insert(validGbid);
    testGbids.push_back(unknownGbid);
    ASSERT_EQ(ValidateGBIDsEnum::UNKNOWN, LCDUtil::validateGbids(testGbids, validGbids));
}

TEST(LCDUtilTest, test_filterDuplicates)
{
    types::Version providerVersion(42, 42);
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    types::DiscoveryEntryWithMetaInfo entry1(providerVersion,
                                             "domain1",
                                             "interface1",
                                             "participantId1",
                                             providerQos,
                                             10000,
                                             10000,
                                             "testkey",
                                             false);
    types::DiscoveryEntryWithMetaInfo entry2(providerVersion,
                                             "domain2",
                                             "interface2",
                                             "participantId2",
                                             providerQos,
                                             10000,
                                             10000,
                                             "testkey",
                                             false);
    providerQos.setScope(types::ProviderScope::GLOBAL);
    types::DiscoveryEntryWithMetaInfo entry1_1(providerVersion,
                                               "domain1",
                                               "interface1",
                                               "participantId1",
                                               providerQos,
                                               10000,
                                               10000,
                                               "testkey",
                                               true);
    types::DiscoveryEntryWithMetaInfo entry3(providerVersion,
                                             "domain3",
                                             "interface3",
                                             "participantId3",
                                             providerQos,
                                             10000,
                                             10000,
                                             "testkey",
                                             true);
    std::vector<types::DiscoveryEntryWithMetaInfo> localCapabilitiesWithMetaInfo;
    localCapabilitiesWithMetaInfo.push_back(entry1);
    localCapabilitiesWithMetaInfo.push_back(entry2);

    std::vector<types::DiscoveryEntryWithMetaInfo> globalCapabilitiesWithMetaInfo;
    globalCapabilitiesWithMetaInfo.push_back(entry1_1);
    globalCapabilitiesWithMetaInfo.push_back(entry3);

    std::vector<types::DiscoveryEntryWithMetaInfo> result = LCDUtil::filterDuplicates(
            std::move(localCapabilitiesWithMetaInfo), std::move(globalCapabilitiesWithMetaInfo));
    ASSERT_EQ(3, result.size());

    // TODO: Global entry should be preferred(according to the comment in the method), but the local
    // one passes
    //  through the filter
    ASSERT_TRUE(std::find(result.begin(), result.end(), entry1) != result.end());
    ASSERT_TRUE(std::find(result.begin(), result.end(), entry2) != result.end());
    ASSERT_TRUE(std::find(result.begin(), result.end(), entry3) != result.end());
}

TEST(LCDUtilTest, test_containsOnlyEmptyString)
{
    std::string validGbid = "validGbid";
    std::string emptyString = "";
    std::vector<std::string> validGbids;
    std::vector<std::string> testGbids;
    validGbids.push_back(validGbid);
    testGbids.push_back(emptyString);
    ASSERT_TRUE(LCDUtil::containsOnlyEmptyString(testGbids));
    ASSERT_FALSE(LCDUtil::containsOnlyEmptyString(validGbids));
    testGbids.push_back(validGbid);
    ASSERT_FALSE(LCDUtil::containsOnlyEmptyString(testGbids));
}

TEST(LCDUtilTest, test_optionalToVector)
{
    types::Version providerVersion(42, 42);
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    types::DiscoveryEntry entry(providerVersion,
                                "domain1",
                                "interface1",
                                "participantId1",
                                providerQos,
                                10000,
                                10000,
                                "testkey");

    boost::optional<types::DiscoveryEntry> emptyOptional;
    boost::optional<types::DiscoveryEntry> filledOptional(entry);

    auto emptyVector = LCDUtil::optionalToVector(emptyOptional);
    ASSERT_EQ(0, emptyVector.size());
    auto filledVector = LCDUtil::optionalToVector(filledOptional);
    ASSERT_EQ(1, filledVector.size());
    ASSERT_EQ(entry, filledVector.at(0));
}

TEST(LCDUtilTest, test_isGlobal)
{
    types::Version providerVersion(42, 42);
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    types::DiscoveryEntry entry(providerVersion,
                                "domain1",
                                "interface1",
                                "participantId1",
                                providerQos,
                                10000,
                                10000,
                                "testkey");
    ASSERT_FALSE(LCDUtil::isGlobal(entry));

    providerQos.setScope(types::ProviderScope::GLOBAL);
    entry.setQos(providerQos);
    ASSERT_TRUE(LCDUtil::isGlobal(entry));
}

TEST(LCDUtilTest, test_isEntryForGbid)
{
    std::recursive_mutex lock;
    std::unique_lock<std::recursive_mutex> cacheLock(lock);
    types::Version providerVersion(42, 42);
    types::ProviderQos providerQos;
    std::string participantId = "participantId1";
    providerQos.setScope(types::ProviderScope::LOCAL);
    types::DiscoveryEntry entry(providerVersion,
                                "domain1",
                                "interface1",
                                participantId,
                                providerQos,
                                10000,
                                10000,
                                "testkey");
    std::string gbid = "gbid1";
    std::string wrongGbid = "wrongGbid";
    std::unordered_set<std::string> testGbids;
    std::vector<std::string> testGbidsVector;
    testGbids.insert(gbid);
    testGbidsVector.push_back(gbid);
    std::unordered_set<std::string> wrongGbids;
    wrongGbids.insert(wrongGbid);
    std::unordered_map<std::string, std::vector<std::string>> participantIdsToGbidsMap;
    participantIdsToGbidsMap[participantId] = testGbidsVector;

    ASSERT_TRUE(LCDUtil::isEntryForGbid(cacheLock, entry, testGbids, participantIdsToGbidsMap));
    ASSERT_FALSE(LCDUtil::isEntryForGbid(cacheLock, entry, wrongGbids, participantIdsToGbidsMap));
}

TEST(LCDUtilTest, filterDiscoveryEntriesByGbids)
{
    std::recursive_mutex lock;
    std::unique_lock<std::recursive_mutex> cacheLock(lock);
    types::Version providerVersion(42, 42);
    types::ProviderQos providerQos;
    std::string participantId = "participantId1";
    providerQos.setScope(types::ProviderScope::LOCAL);
    types::DiscoveryEntry entry(providerVersion,
                                "domain1",
                                "interface1",
                                participantId,
                                providerQos,
                                10000,
                                10000,
                                "testkey");
    types::DiscoveryEntry otherEntry(providerVersion,
                                     "domain1",
                                     "interface1",
                                     "otherParticipantId",
                                     providerQos,
                                     10000,
                                     10000,
                                     "testkey");
    std::vector<types::DiscoveryEntry> discoveryEntryVector;
    discoveryEntryVector.push_back(entry);
    discoveryEntryVector.push_back(otherEntry);
    std::string gbid = "gbid1";
    std::string wrongGbid = "wrongGbid";
    std::unordered_set<std::string> testGbids;
    std::vector<std::string> testGbidsVector;
    testGbids.insert(gbid);
    testGbidsVector.push_back(gbid);
    std::unordered_set<std::string> wrongGbids;
    wrongGbids.insert(wrongGbid);
    std::unordered_map<std::string, std::vector<std::string>> participantIdsToGbidsMap;
    participantIdsToGbidsMap[participantId] = testGbidsVector;

    auto resultVector = LCDUtil::filterDiscoveryEntriesByGbids(
            cacheLock, discoveryEntryVector, testGbids, participantIdsToGbidsMap);
    ASSERT_EQ(1, resultVector.size());
    ASSERT_EQ(entry, resultVector.at(0));
}

TEST(LCDUtilTest, test_joinToString)
{
    types::Version providerVersion(42, 42);
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    types::DiscoveryEntry entry(providerVersion,
                                "domain1",
                                "interface1",
                                "participantId",
                                providerQos,
                                10000,
                                10000,
                                "testkey");
    types::DiscoveryEntry otherEntry(providerVersion,
                                     "domain2",
                                     "interface2",
                                     "otherParticipantId",
                                     providerQos,
                                     1000,
                                     1000,
                                     "testkey");
    std::vector<types::DiscoveryEntry> discoveryEntryVector;
    discoveryEntryVector.push_back(entry);
    discoveryEntryVector.push_back(otherEntry);
    std::string result = LCDUtil::joinToString(discoveryEntryVector);
    ASSERT_TRUE(result.length() > 0);
    const std::string expected = entry.toString() + ", " + otherEntry.toString() + ", ";
    ASSERT_EQ(expected, result);
}

TEST(LCDUtilTest, test_replaceGbidWithEmptyString)
{
    system::RoutingTypes::MqttAddress mqttAddress("brokerUri", "topic");

    types::Version providerVersion(42, 42);
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    types::GlobalDiscoveryEntry firstEntry(providerVersion,
                                           "domain1",
                                           "interface1",
                                           "participantId1",
                                           providerQos,
                                           10000,
                                           10000,
                                           "testkey",
                                           joynr::serializer::serializeToJson(mqttAddress));
    types::GlobalDiscoveryEntry otherEntry(providerVersion,
                                           "domain2",
                                           "interface2",
                                           "participantId2",
                                           providerQos,
                                           1000,
                                           1000,
                                           "testkey",
                                           joynr::serializer::serializeToJson(mqttAddress));

    std::vector<types::GlobalDiscoveryEntry> discoveryEntryVector;
    discoveryEntryVector.push_back(firstEntry);
    discoveryEntryVector.push_back(otherEntry);

    LCDUtil::replaceGbidWithEmptyString(discoveryEntryVector);

    for (auto entry : discoveryEntryVector) {
        const std::string& serializedAddress = entry.getAddress();
        std::shared_ptr<system::RoutingTypes::Address> address;
        try {
            joynr::serializer::deserializeFromJson(address, serializedAddress);
        } catch (const std::invalid_argument& e) {
            FAIL() << "Error when deserializing address: " << e.what();
        }
        if (auto castAddress = dynamic_cast<system::RoutingTypes::MqttAddress*>(address.get())) {
            ASSERT_EQ("", castAddress->getBrokerUri());
        } else {
            FAIL() << "Deserialized address is not a MQTT address";
        }
    }
}

TEST(LCDUtilTest, test_toGlobalDiscoveryEntry)
{
    types::ProviderQos providerQos{};
    types::DiscoveryEntry entry{joynr::types::Version{1, 1},
                                "domain",
                                "interfaceName",
                                "participantId",
                                providerQos,
                                12,
                                10,
                                "publicKeyId"};

    const std::string localAddress = "myLocalAddress";

    auto globalEntry = LCDUtil::toGlobalDiscoveryEntry(entry, localAddress);

    ASSERT_EQ(entry.getDomain(), globalEntry.getDomain());
    ASSERT_EQ(entry.getParticipantId(), globalEntry.getParticipantId());
    ASSERT_EQ(entry.getQos(), globalEntry.getQos());
    ASSERT_EQ(entry.getLastSeenDateMs(), globalEntry.getLastSeenDateMs());
    ASSERT_EQ(entry.getExpiryDateMs(), globalEntry.getExpiryDateMs());
    ASSERT_EQ(entry.getPublicKeyId(), globalEntry.getPublicKeyId());
    ASSERT_EQ(localAddress, globalEntry.getAddress());

    ASSERT_EQ(entry, static_cast<types::DiscoveryEntry>(globalEntry));
}

TEST(LCDUtilTest, test_getInterfaceAddresses)
{
    const std::string interfaceName = "myInterfaceName";
    const std::vector<std::string> domains = {"localDomain", "cachedDomain", "remoteDomain"};

    auto interfaceAddresses = LCDUtil::getInterfaceAddresses(domains, interfaceName);

    ASSERT_TRUE(interfaceAddresses.size() == domains.size());

    ASSERT_TRUE(std::equal(interfaceAddresses.begin(),
                           interfaceAddresses.end(),
                           domains.begin(),
                           [interfaceName](auto l, auto r) {
                               return ((l.getDomain() == r) && (l.getInterface() == interfaceName));
                           }));
}

void test_convertDiscoveryEntry(const bool isLocal)
{
    types::ProviderQos providerQos{};
    providerQos.setScope(types::ProviderScope::LOCAL);

    types::DiscoveryEntry entry{joynr::types::Version{1, 1},
                                "domain",
                                "interfaceName",
                                "participantId",
                                providerQos,
                                12,
                                10,
                                "publicKeyId"};

    auto result = LCDUtil::convert(isLocal, entry);

    ASSERT_EQ(entry.getDomain(), result.getDomain());
    ASSERT_EQ(entry.getParticipantId(), result.getParticipantId());
    ASSERT_EQ(entry.getQos(), result.getQos());
    ASSERT_EQ(entry.getLastSeenDateMs(), result.getLastSeenDateMs());
    ASSERT_EQ(entry.getExpiryDateMs(), result.getExpiryDateMs());
    ASSERT_EQ(entry.getPublicKeyId(), result.getPublicKeyId());
    ASSERT_EQ(isLocal, result.getIsLocal());

    ASSERT_EQ(entry, static_cast<types::DiscoveryEntry>(result));
}

TEST(LCDUtilTest, test_convertDiscoveryEntry_local)
{
    const bool isLocal = true;
    test_convertDiscoveryEntry(isLocal);
}

TEST(LCDUtilTest, test_convertDiscoveryEntry_nonlocal)
{
    const bool isLocal = false;
    test_convertDiscoveryEntry(isLocal);
}

void test_convertDiscoveryEntryVector(const bool isLocal)
{
    types::ProviderQos providerQos;

    providerQos.setScope(types::ProviderScope::LOCAL);
    types::DiscoveryEntry firstEntry{joynr::types::Version{1, 1},
                                     "domain",
                                     "interfaceName",
                                     "participantId",
                                     providerQos,
                                     12,
                                     10,
                                     "publicKeyId"};

    providerQos.setScope(types::ProviderScope::GLOBAL);
    types::DiscoveryEntry otherEntry{joynr::types::Version{2, 2},
                                     "domain1",
                                     "interfaceName1",
                                     "participantId1",
                                     providerQos,
                                     1212,
                                     1010,
                                     "publicKeyId1"};

    std::vector<types::DiscoveryEntry> entries;
    entries.push_back(firstEntry);
    entries.push_back(otherEntry);

    auto convertedEntries = LCDUtil::convert(isLocal, entries);

    ASSERT_TRUE(
            std::equal(entries.begin(), entries.end(), convertedEntries.begin(),
                       [](auto l, auto r) { return l == static_cast<types::DiscoveryEntry>(r); }));

    for (auto entry : convertedEntries)
        ASSERT_EQ(isLocal, entry.getIsLocal());
}

TEST(LCDUtilTest, test_convertDiscoveryEntryVector_local)
{
    const bool isLocal = true;
    test_convertDiscoveryEntryVector(isLocal);
}

TEST(LCDUtilTest, test_convertDiscoveryEntryVector_nonlocal)
{
    const bool isLocal = false;
    test_convertDiscoveryEntryVector(isLocal);
}
