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
#ifndef UDSSERVER_H
#define UDSSERVER_H

#include <atomic>
#include <future>
#include <list>
#include <memory>
#include <mutex>

#include <boost/asio.hpp>

#include "joynr/BoostIoserviceForwardDecl.h"
#include "joynr/IUdsSender.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/UdsSettings.h"
#include "joynr/system/RoutingTypes/UdsClientAddress.h"

namespace joynr
{
namespace exceptions
{
class JoynrRuntimeException;
} // namespace exceptions

class UdsFrameBufferV1;

template <typename FRAME>
class UdsSendQueue;

class UdsServerUtil
{
public:
    static std::string getUserNameByUid(uid_t uid);

private:
    ADD_LOGGER(UdsServerUtil)
};

class UdsServer
{
public:
    using Connected = std::function<void(const system::RoutingTypes::UdsClientAddress&,
                                         std::unique_ptr<IUdsSender>)>;
    using Disconnected = std::function<void(const system::RoutingTypes::UdsClientAddress&)>;
    using Received = std::function<void(const system::RoutingTypes::UdsClientAddress&,
                                        smrf::ByteVector&&,
                                        const std::string&)>;

    explicit UdsServer(const UdsSettings& settings);
    ~UdsServer();

    // Server cannot be copied since it has internal threads
    DISALLOW_COPY_AND_ASSIGN(UdsServer);

    // atomic_bool _started cannot be moved
    DISALLOW_MOVE_AND_ASSIGN(UdsServer);

    /**
     * @brief Sets callback for sucuessful connection of a client. Connection is successful if
     * initial message has been received.
     * @param callback Callback
     */
    void setConnectCallback(const Connected& callback);

    /**
     * @brief Sets callback for disconnection of the client
     * @param callback Callback
     */
    void setDisconnectCallback(const Disconnected& callback);

    /**
     * @brief Sets callback for message reception
     * @param callback Callback
     */
    void setReceiveCallback(const Received& callback);

    /** Opens an UNIX domain socket asynchronously and starts the IO thread pool. */
    void start();

private:
    using uds = boost::asio::local::stream_protocol;
    // Default config basically does nothing, everything is just eaten
    struct ConnectionConfig {
        std::size_t _maxSendQueueSize = 0;
        Connected _connectedCallback = [](const system::RoutingTypes::UdsClientAddress&,
                                          std::shared_ptr<IUdsSender>) {};
        Disconnected _disconnectedCallback = [](const system::RoutingTypes::UdsClientAddress&) {};
        Received _receivedCallback = [](const system::RoutingTypes::UdsClientAddress&,
                                        smrf::ByteVector&&,
                                        const std::string&) {};
    };

    // Connection to remote client
    class Connection : public std::enable_shared_from_this<Connection>
    {
        friend class UdsSender;

    public:
        Connection(std::shared_ptr<boost::asio::io_service>& ioContext,
                   const ConnectionConfig& config,
                   std::uint64_t connectionIndex) noexcept;

        /** Notifies sender if the connection got lost (error occured) */
        ~Connection() = default;

        // remote clients are shared
        DISALLOW_COPY_AND_ASSIGN(Connection);
        DISALLOW_MOVE_AND_ASSIGN(Connection);

        uds::socket& getSocket();

        void send(const smrf::ByteArrayView& msg, const IUdsSender::SendFailed& callback);

        void shutdown();

        void doReadInitHeader() noexcept;

    private:
        std::string getUserName();
        // I/O context functions
        void doReadInitBody() noexcept;
        void doReadHeader() noexcept;
        void doReadBody() noexcept;
        void doWrite() noexcept;
        bool doCheck(const boost::system::error_code& ec) noexcept;
        void doClose(const std::string& errorMessage, const std::exception& error) noexcept;
        void doClose(const std::string& errorMessage) noexcept;
        void doClose() noexcept;

        std::shared_ptr<boost::asio::io_service> _ioContext;
        uds::socket _socket;
        Connected _connectedCallback;
        Disconnected _disconnectedCallback;
        Received _receivedCallback;

        std::atomic_bool _isClosed;

        system::RoutingTypes::UdsClientAddress _address; // Only accessed by read-strand

        std::string _username; // Appended to each received message for ACL

        // PIMPL to keep includes clean
        std::unique_ptr<UdsSendQueue<UdsFrameBufferV1>> _sendQueue;
        std::unique_ptr<UdsFrameBufferV1> _readBuffer;

        std::uint64_t _connectionIndex;

        ADD_LOGGER(Connection)
    };

    // Lifetime of connection user is decoupled from server
    class UdsSender : public IUdsSender
    {
    public:
        UdsSender(std::weak_ptr<Connection> connection);
        virtual ~UdsSender();
        void send(const smrf::ByteArrayView& msg, const IUdsSender::SendFailed& callback) override;

    private:
        std::weak_ptr<Connection> _connection;
        ADD_LOGGER(UdsSender)
    };

    void run();

    // I/O context functions
    void doAcceptClient() noexcept;

    // One thread handles server socket and all client sockets. Therefore no strands are required.
    static constexpr int _threadsPerServer = 1;
    // Context is shared with the connection (but lifetime depends on DecoupledUser and UdsServer)
    std::shared_ptr<boost::asio::io_service> _ioContext;
    ConnectionConfig _remoteConfig;
    std::chrono::milliseconds _openSleepTime;
    uds::endpoint _endpoint;
    uds::acceptor _acceptor;
    std::future<void> _worker;
    std::atomic_bool _started;
    std::mutex _acceptorMutex;
    std::unordered_map<std::uint64_t, std::weak_ptr<Connection>> _connectionMap;
    std::atomic<uint64_t> _connectionIndex;
    std::shared_ptr<boost::asio::io_service::work> _workGuard;
    ADD_LOGGER(UdsServer)
};

} // namespace joynr

#endif // UDSSERVER_H
