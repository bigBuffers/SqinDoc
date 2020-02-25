/////////////////////////////////////////////////////////////////////////////////////////
// This code contains NVIDIA Confidential Information and is disclosed
// under the Mutual Non-Disclosure Agreement.
//
// Notice
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
//
// NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. No third party distribution is allowed unless
// expressly authorized by NVIDIA.  Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2014-2019 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//
/////////////////////////////////////////////////////////////////////////////////////////

#include <dw/comms/socketipc/impl/SocketClientServer.hpp>
#include <dw/core/Context.hpp>
#include <dw/core/TimeSource.hpp>
#include <dw/core/Utils.hpp>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <thread>
#include <netinet/in.h>

using namespace dw::core;
using namespace dw::ipc;

// The socket macros include a lot of old-style casts disable for this cpp file
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

SocketServer::SocketServer(uint16_t port, size_t connectionPoolSize, Context* ctx, bool reusePort)
    : Object(ctx), m_connectionPool(connectionPoolSize, ctx)
{
    LOGSTREAM_OTA_FUNCTIONNAME;

    // create server socket
    m_serverSocket.reset(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP), [](int32_t s) {
        if (s >= 0)
            ::close(s);
    });

    if (m_serverSocket.get() == -1)
    {
        throw dw::core::Exception(DW_CANNOT_CREATE_OBJECT, "SocketServer: can't create socket");
    }
    SocketConnection::setNonBlocking(m_serverSocket.get());

    if (reusePort)
    {
        // SO_REUSEPORT allows one to bind multiple connections with
        // the exact tuple matching in the connection list.
        auto const trueValue = char{1};
        if (::setsockopt(m_serverSocket.get(), SOL_SOCKET, SO_REUSEPORT, static_cast<const char*>(&trueValue),
                         sizeof(int32_t)) == -1)
        {
            LOGSTREAM_ERROR(this) << "SocketServer: can't set socket reuse options "
                                  << ::strerror(errno) << Logger::State::endl;
            throw dw::core::Exception(DW_CANNOT_CREATE_OBJECT, "SocketServer: can't set socket reuse options");
        }
    }

    // initialize address/port structure
    struct sockaddr_in addr = {};
    addr.sin_family         = AF_INET;
    addr.sin_port           = htons(port);
    addr.sin_addr.s_addr    = INADDR_ANY;

    // assign a port number to the socket
    if (::bind(m_serverSocket.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
    {
        char const* err_str = ::strerror(errno);
        LOGSTREAM_ERROR(this) << "SocketServer: can't bind socket to port " << port << "-" << err_str << Logger::State::endl;
        throw dw::core::Exception(DW_CANNOT_CREATE_OBJECT, "SocketServer: can't bind socket to port ", static_cast<uint32_t>(port), "-", err_str);
    }

    // make it a "listening socket"
    if (::listen(m_serverSocket.get(), 20) != 0)
    {
        char const* err_str = ::strerror(errno);
        LOGSTREAM_ERROR(this) << "SocketServer: can't listen on socket-" << err_str << Logger::State::endl;
        throw dw::core::Exception(DW_CANNOT_CREATE_OBJECT, "SocketServer: can't listen on socket-", err_str);
    }

    LOGSTREAM_VERBOSE(this) << "SocketServer: listening on " << port << Logger::State::endl;
}

void SocketServer::reset()
{
}

SocketServer::AcceptStatus SocketServer::accept(dwTime_t timeout_us)
{
    auto ret = AcceptStatus{DW_TIME_OUT, nullptr};

    // get a free connection slot
    auto* connectionSlot = m_connectionPool.getFreeSlot();

    if (!connectionSlot)
    {
        ret = AcceptStatus{DW_BUFFER_FULL, nullptr};
        return ret;
    }

    // prepare timeout
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(m_serverSocket.get(), &fdset);

    timeval tv{};
    tv.tv_sec  = static_cast<decltype(timeval::tv_sec)>(timeout_us / ONE_SECOND_US);  // microseconds to seconds
    tv.tv_usec = static_cast<decltype(timeval::tv_usec)>(timeout_us % ONE_SECOND_US); // remaining microseconds

    // timeout if no client socket tries to connect
    if (::select(m_serverSocket.get() + 1, &fdset, nullptr, nullptr, &tv) > 0)
    {
        // accept a connection
        sockaddr_in client_addr;
        auto addrlen = sizeof(client_addr);

        auto connectionSocket = UniqueSocketDescriptorHandle(
            ::accept(m_serverSocket.get(), reinterpret_cast<sockaddr*>(&client_addr),
                     reinterpret_cast<socklen_t*>(&addrlen)),
            [](int32_t s) {
                if (s >= 0)
                    ::close(s);
            });

        if (connectionSocket.get() == -1)
            throw dw::core::Exception(DW_CANNOT_CREATE_OBJECT, "SocketServer: error accepting connection");

        LOGSTREAM_VERBOSE(this) << "SocketServer: accepted " << inet_ntoa(client_addr.sin_addr) << ":"
                                << ntohs(client_addr.sin_port) << Logger::State::endl;

        ret.status     = DW_SUCCESS;
        ret.connection = std::make_shared<SocketConnection>(std::move(connectionSocket), getContext());

        // observe connection in pool
        (*connectionSlot) = ret.connection;
    }

    return ret;
}

dwStatus SocketServer::broadcast(const uint8_t* buffer, size_t buffer_size, dwTime_t timeout_us)
{
    return m_connectionPool.broadcast(buffer, buffer_size, timeout_us);
}

SocketClient::SocketClient(size_t connectionPoolSize, Context* ctx)
    : Object(ctx), m_connectionPool(connectionPoolSize, ctx)
{
}

void SocketClient::reset()
{
}

bool SocketClient::handleConnectInProgress(ConnectStatus& ret,
                                           UniqueSocketDescriptorHandle& connectionSocket,
                                           dwTime_t timeout_us)
{
    // prepare timeout
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(connectionSocket.get(), &fdset);

    timeval tv{};
    tv.tv_sec =
        static_cast<decltype(timeval::tv_sec)>(timeout_us / ONE_SECOND_US); // microseconds to seconds
    tv.tv_usec =
        static_cast<decltype(timeval::tv_usec)>(timeout_us % ONE_SECOND_US); // remaining microseconds

    if (::select(connectionSocket.get() + 1, NULL, &fdset, NULL, &tv) > 0)
    {
        int32_t optval;
        socklen_t optlen = sizeof(decltype(optval));
        auto optret      = ::getsockopt(connectionSocket.get(), SOL_SOCKET, SO_ERROR,
                                   static_cast<void*>(&optval), &optlen);
        if (optret || optval)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            ret.status = DW_TIME_OUT;
            return true;
        }

        ret.status = DW_SUCCESS;
        ret.connection =
            std::make_shared<SocketConnection>(std::move(connectionSocket), getContext());
        return false;
    }
    else
    {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        ret.status = DW_TIME_OUT;
        return true;
    }
}

void SocketClient::handleConnect(ConnectStatus& ret,
                                 UniqueSocketDescriptorHandle& connectionSocket,
                                 const struct addrinfo* serv_addr,
                                 dwTime_t then_us,
                                 dwTime_t timeout_us)
{
    do
    {
        std::this_thread::yield();

        if (::connect(connectionSocket.get(), serv_addr->ai_addr,
                      serv_addr->ai_addrlen) < 0)
        {
            if (errno == EINPROGRESS)
            {
                if (handleConnectInProgress(ret, connectionSocket, timeout_us))
                    continue;
                else
                    break;
            }
            else if (errno == ECONNREFUSED)
            {
                // TCP requires the socket to be closed and re-opened
                connectionSocket = UniqueSocketDescriptorHandle(
                    ::socket(serv_addr->ai_family, serv_addr->ai_socktype, serv_addr->ai_protocol),
                    [](int32_t s) {
                        if (s >= 0)
                            ::close(s);
                    });
                if (connectionSocket.get() != -1)
                    SocketConnection::setNonBlocking(
                        connectionSocket.get()); // connecting sockets need to be non-blocking
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                ret.status = DW_TIME_OUT;
                continue;
            }
        }
        else
        {
            ret.status     = DW_SUCCESS;
            ret.connection = std::make_shared<SocketConnection>(std::move(connectionSocket), getContext());
            break;
        }
    } while (getContext()->getTimeSource()->getCurrentTime() <= then_us);
}

void SocketClient::attemptConnect(ConnectStatus& ret,
                                  struct addrinfo* gai_results,
                                  dwTime_t then_us,
                                  dwTime_t attempt_timeout_us)
{
    THROW_IF_PARAM_NULL(gai_results);

    for (struct addrinfo* rp = gai_results; rp != NULL; rp = rp->ai_next)
    {
        // create connection socket
        auto connectionSocket = UniqueSocketDescriptorHandle(
            ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol), [](int32_t s) {
                if (s >= 0)
                    ::close(s);
            });
        if (connectionSocket.get() == -1)
        {
            LOGSTREAM_ERROR(this) << "SocketClient: can't create socket - " << ::strerror(errno)
                                  << Logger::State::endl;
            throw dw::core::Exception(DW_CANNOT_CREATE_OBJECT, "SocketClient: can't create socket");
        }
        SocketConnection::setNonBlocking(
            connectionSocket.get()); // connecting sockets need to be non-blocking
        handleConnect(ret, connectionSocket, rp, then_us, attempt_timeout_us);
        if (ret.status == DW_SUCCESS)
        {
            SocketConnection::unsetNonBlocking(
                ret.connection
                    ->get_sockfd()); // no need to be non-blocking anymore after connection is established
            break;
        }
    }
}

SocketClient::ConnectStatus SocketClient::connect(const char* host, uint16_t port, dwTime_t timeout_us)
{
    THROW_IF_PARAM_NULL(host);

    ConnectStatus ret;

    // get a free connection slot
    auto* connectionSlot = m_connectionPool.getFreeSlot();

    if (!connectionSlot)
    {
        ret.status = DW_BUFFER_FULL;
        return ret;
    }

    struct addrinfo gai_hints = {};
    gai_hints.ai_family       = AF_INET; // Allow IPv4 only
    gai_hints.ai_socktype     = SOCK_STREAM;
    gai_hints.ai_protocol     = IPPROTO_TCP;

    FixedString<6> portStr("");
    portStr += port;
    dwTime_t attempt_timeout_us = timeout_us / MAX_CONN_ATTEMPT;
    for (uint16_t attempt = 0; attempt < MAX_CONN_ATTEMPT && ret.status != DW_SUCCESS; attempt++)
    {
        // try to connect with timeout
        auto const then_us = getContext()->getTimeSource()->getCurrentTime() + attempt_timeout_us;
        struct addrinfo* gai_results{};
        int32_t gai_errno = ::getaddrinfo(host, portStr.c_str(), &gai_hints, &gai_results);
        if (gai_errno != 0 || gai_results == NULL)
        {
            LOGSTREAM_ERROR(this) << "SocketClient: can't resolve " << host << ":" << portStr.c_str()
                                  << ". Err- " << ::gai_strerror(gai_errno) << Logger::State::endl;
            if (attempt < MAX_CONN_ATTEMPT - 1)
            {
                std::this_thread::sleep_for(std::chrono::microseconds(attempt_timeout_us));
                continue;
            }
            throw dw::core::Exception(DW_CANNOT_CREATE_OBJECT, "SocketClient: can't resolve ", host, ":",
                                      portStr.c_str(), ". Err- ", ::gai_strerror(gai_errno));
        }
        attemptConnect(ret, gai_results, then_us, attempt_timeout_us);
        ::freeaddrinfo(gai_results);
    }
    if (ret.status == DW_SUCCESS)
    {
        // observe connection in pool
        (*connectionSlot) = ret.connection;
        LOGSTREAM_VERBOSE(this) << "SocketClient: connected " << host << ":" << port
                                << Logger::State::endl;
    }
    return ret;
}

dwStatus SocketClient::broadcast(const uint8_t* buffer, size_t buffer_size, dwTime_t timeout_us)
{
    return m_connectionPool.broadcast(buffer, buffer_size, timeout_us);
}

SocketConnection::SocketConnection(UniqueSocketDescriptorHandle connectionSocket, Context* ctx)
    : Object(ctx), m_connectionSocket(std::move(connectionSocket))
{
    // check for valid socket connection
    if (!m_connectionSocket)
    {
        LOGSTREAM_ERROR(this) << "SocketConnection: socket file descriptor invalid"
                              << Logger::State::endl;
        throw dw::core::Exception(DW_CANNOT_CREATE_OBJECT,
                                  "SocketConnection: socket file descriptor invalid");
    }

    int32_t optval;
    socklen_t optlen = sizeof(decltype(optval));
    auto optret =
        ::getsockopt(m_connectionSocket.get(), SOL_SOCKET, SO_ERROR, static_cast<void*>(&optval), &optlen);
    if (optret)
    {
        LOGSTREAM_ERROR(this) << "SocketConnection: socket state not retrievable"
                              << Logger::State::endl;
        throw dw::core::Exception(DW_CANNOT_CREATE_OBJECT, "SocketConnection: socket state not retrievable");
    }

    if (optval)
    {
        LOGSTREAM_ERROR(this) << "SocketConnection: socket in error state" << Logger::State::endl;
        throw dw::core::Exception(DW_CANNOT_CREATE_OBJECT, "SocketConnection: socket in error state");
    }
}

SocketConnection::~SocketConnection()
{
    if (m_skipCount > 0)
    {
        LOGSTREAM_ERROR(this) << "SocketConnection: Total " << m_skipCount
                              << " packet(s) skipped for the conn with sockfd-" << get_sockfd()
                              << "." << Logger::State::endl;
    }
}

void SocketConnection::reset()
{
}

dwStatus SocketConnection::getSockaddr(dwSocketAddrIn* addr)
{
    sockaddr_in addr_posix;
    socklen_t addr_size = sizeof(decltype(addr_posix));

    if (getpeername(get_sockfd(),
                    reinterpret_cast<struct sockaddr*>(&addr_posix),
                    &addr_size) == 0)
    {
        addr->ip   = addr_posix.sin_addr.s_addr;
        addr->port = addr_posix.sin_port;
        return DW_SUCCESS;
    }
    return DW_FAILURE;
}

void SocketConnection::setNonBlocking(int32_t sockfd)
{
    // Set non-blocking
    int32_t flags = ::fcntl(sockfd, F_GETFL, NULL);
    if (flags < 0)
        throw dw::core::Exception(DW_CANNOT_CREATE_OBJECT,
                                  "SocketConnection:",
                                  __func__,
                                  " can't get socket flags. Err- ",
                                  ::strerror(errno));
    if (::fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0)
        throw dw::core::Exception(DW_CANNOT_CREATE_OBJECT,
                                  "SocketConnection:",
                                  __func__,
                                  " can't set socket flags. Err- ",
                                  ::strerror(errno));
}

void SocketConnection::unsetNonBlocking(int32_t sockfd)
{
    // Clear the non-blocking flag
    int32_t flags = ::fcntl(sockfd, F_GETFL, NULL);
    if (flags < 0)
        throw dw::core::Exception(DW_CANNOT_CREATE_OBJECT,
                                  "SocketConnection:",
                                  __func__,
                                  " can't get socket flags. Err- ",
                                  ::strerror(errno));
    flags &= (~O_NONBLOCK);
    if (::fcntl(sockfd, F_SETFL, flags) < 0)
        throw dw::core::Exception(DW_CANNOT_CREATE_OBJECT,
                                  "SocketConnection:",
                                  __func__,
                                  " can't set socket flags. Err- ",
                                  ::strerror(errno));
}

inline void SocketConnection::setSendTimeout(dwTime_t timeout_us)
{
    if (timeout_us == m_sendTimeoutUs)
    {
        return;
    }
    timeval tv = timeoutToTimeval(timeout_us);
    errno      = 0;
    if (::setsockopt(m_connectionSocket.get(), SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0)
    {
        throw dw::core::Exception(DW_FAILURE, "SocketConnection: can't set send timeout - ", ::strerror(errno));
    }
    m_sendTimeoutUs = timeout_us;
}

inline void SocketConnection::setRecvTimeout(dwTime_t timeout_us)
{
    if (timeout_us == m_recvTimeoutUs)
    {
        return;
    }
    timeval tv = timeoutToTimeval(timeout_us);
    errno      = 0;
    if (::setsockopt(m_connectionSocket.get(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        throw dw::core::Exception(DW_FAILURE, "SocketConnection: can't set recv timeout - ", ::strerror(errno));
    }
    m_recvTimeoutUs = timeout_us;
}

SocketConnection::MessageStatus SocketConnection::send(const void* buffer, size_t buffer_size, dwTime_t timeout_us)
{
    THROW_IF_PARAM_NULL(buffer);

    if (buffer_size == 0)
    {
        return {DW_SUCCESS, 0};
    }

    if (!m_connectionSocket)
    {
        return {DW_END_OF_STREAM, 0};
    }

    ssize_t ret = 0;
    // Set timeout for send operation
    setSendTimeout(timeout_us);

    // send to the socket with a timeo-out now
    ret = ::send(m_connectionSocket.get(), buffer, buffer_size, MSG_NOSIGNAL);

    if (ret < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return {DW_TIME_OUT, 0};
        }
        else
        {
            return {DW_FAILURE, 0};
        }
    }
    else if (ret == 0)
    {
        m_connectionSocket.reset();
        return {DW_END_OF_STREAM, 0};
    }

    return {DW_SUCCESS, static_cast<size_t>(ret)};
}

ssize_t SocketConnection::processRecvWithTimeout(void* buffer, size_t buffer_size, dwTime_t timeout_us)
{
    ssize_t ret = 0;
    // Set timeout for receive operation
    setRecvTimeout(timeout_us);
    // read from the socket with a timeo-out now
    auto waitStartTS = std::chrono::high_resolution_clock::now();
    do
    {
        ret = ::recv(m_connectionSocket.get(), buffer, buffer_size, MSG_WAITALL);

        if (ret < 0)
        {
            // In some systems (eg., QNX) EAGAIN/EWOULDBLOCK comes ignoring timeout, try again
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                if (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - waitStartTS).count() > timeout_us)
                    break;
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
            else
                break;
        }
    } while (ret < 0);
    return ret;
}
SocketConnection::MessageStatus SocketConnection::recv(void* buffer, size_t buffer_size, dwTime_t timeout_us)
{
    THROW_IF_PARAM_NULL(buffer);

    if (!m_connectionSocket)
    {
        return {DW_END_OF_STREAM, 0};
    }

    if (buffer_size == 0)
    {
        return {DW_SUCCESS, 0};
    }

    ssize_t ret = 0;
    // we want non-blocking operation
    if (timeout_us == 0)
    {
        ret = ::recv(m_connectionSocket.get(), buffer, buffer_size, MSG_DONTWAIT);
    }
    else
    {
        ret = processRecvWithTimeout(buffer, buffer_size, timeout_us);
    }
    if (ret < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return {DW_TIME_OUT, 0};
        }
        else
        {
            return {DW_FAILURE, 0};
        }
    }
    else if (ret == 0)
    {
        m_connectionSocket.reset();
        return {DW_END_OF_STREAM, 0};
    }

    return {DW_SUCCESS, static_cast<size_t>(ret)};
}

SocketConnection::MessageStatus SocketConnection::peek(uint8_t* buffer, size_t buffer_size, dwTime_t timeout_us)
{
    THROW_IF_PARAM_NULL(buffer);

    if (!m_connectionSocket)
        return {DW_END_OF_STREAM, 0};

    // '::recv' returns 0 when buffer size is 0, we cannot distinguish between this case and end-of-stream.
    if (buffer_size == 0 || timeout_us < 0)
        return {DW_INVALID_ARGUMENT, 0};

    ssize_t ret{};
    if (timeout_us == 0)
        ret = peekNonBlock(buffer, buffer_size);
    else if (timeout_us == DW_TIMEOUT_INFINITE)
        ret = peekBlock(buffer, buffer_size);
    else
        ret = peekBlockWithTimeout(buffer, buffer_size, timeout_us);

    if (ret == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return {DW_TIME_OUT, 0};
        }
        else
        {
            return {DW_FAILURE, 0};
        }
    }

    if (ret == 0)
    {
        m_connectionSocket.reset();
        return {DW_END_OF_STREAM, 0};
    }

    return {DW_SUCCESS, static_cast<size_t>(ret)};
}

ssize_t SocketConnection::peekBlock(uint8_t* buffer, size_t buffer_size)
{
    auto ret = ssize_t{};

    // Clear up the receive timeout, if any
    setRecvTimeout(0);
    do
    {
        ret = ::recv(m_connectionSocket.get(), static_cast<void*>(buffer), buffer_size, MSG_PEEK);

        if (ret == -1)
        {
            // Non-blocking sockets may time out even with MSG_WAITALL set, try again
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
            else
                break;
        }
    } while (ret == -1);

    return ret;
}

ssize_t SocketConnection::peekBlockWithTimeout(uint8_t* buffer, size_t buffer_size, dwTime_t timeout_us)
{

    // Set timeout for peek operation.
    setRecvTimeout(timeout_us);
    return ::recv(m_connectionSocket.get(), static_cast<void*>(buffer), buffer_size, MSG_PEEK);
}

ssize_t SocketConnection::peekNonBlock(uint8_t* buffer, size_t buffer_size)
{
    return ::recv(m_connectionSocket.get(), static_cast<void*>(buffer), buffer_size, MSG_PEEK | MSG_DONTWAIT);
}

SocketConnectionPool::SocketConnectionPool(size_t poolSize, Context* ctx)
    : Object(ctx), m_connectionPool(poolSize)
{

    for (size_t i = 0; i < poolSize; ++i)
        m_connectionPool.push_back(std::weak_ptr<SocketConnection>{});
}

void SocketConnectionPool::reset()
{
}

std::weak_ptr<SocketConnection>* SocketConnectionPool::getFreeSlot()
{
    for (size_t i = 0; i < m_connectionPool.size(); ++i)
    {
        auto& connectionPtr = m_connectionPool[i];

        // free slot
        if (connectionPtr.use_count() == 0)
            return &connectionPtr;
    }

    // no free slot available
    return nullptr;
}

bool SocketConnectionPool::isBroadcastPossible(fd_set& wfdset)
{
    bool connFound = false;
    int32_t sockfd = 0, max_sockfd = 0;

    timeval tv{};
    tv.tv_sec  = 0;
    tv.tv_usec = SOCK_SELECT_AT_BROADCAST_TIMEOUT;

    FD_ZERO(&wfdset);
    for (size_t i = 0; i < m_connectionPool.size(); ++i)
    {
        // get used slots
        auto connection = m_connectionPool[i].lock();

        if (!connection)
            continue;
        connFound = true;
        sockfd    = connection->get_sockfd();
        if (sockfd > max_sockfd)
            max_sockfd = sockfd;
        FD_SET(sockfd, &wfdset);
    }
    if (!connFound)
        return false;

    if (::select(max_sockfd + 1, nullptr, &wfdset, nullptr, &tv) > 0)
        return true;

    m_dropCount++;
    if ((m_dropCount & REPEATED_ERR_LOG_INTERVAL_MINUS1) == 1)
    {
        LOGSTREAM_ERROR(this) << "SocketConnectionPool: dropped " << m_dropCount
                              << " packet(s) so far, as receiver(s) delayed - "
                              << ::strerror(errno) << Logger::State::endl;
    }
    return false;
}

dwStatus SocketConnectionPool::broadcast(const uint8_t* buffer, size_t buffer_size, dwTime_t timeout_us)
{
    THROW_IF_PARAM_NULL(buffer);

    auto ret = DW_SUCCESS;
    fd_set wfdset;

    if (!isBroadcastPossible(wfdset))
        return ret;

    for (size_t i = 0; i < m_connectionPool.size(); ++i)
    {
        auto connection = m_connectionPool[i].lock();

        if (!connection)
            continue;
        if (!FD_ISSET(connection->get_sockfd(), &wfdset))
        {
            connection->m_skipCount++;
            if ((connection->m_skipCount & REPEATED_ERR_LOG_INTERVAL_MINUS1) == 1)
            {
                LOGSTREAM_ERROR(this) << "SocketConnection: skipped " << connection->m_skipCount
                                      << " packet(s) transmit so far for the conn with sockfd-"
                                      << connection->get_sockfd() << ", as the receiver delayed."
                                      << Logger::State::endl;
            }
            continue;
        }
        auto messageStatus = connection->send(buffer, buffer_size, timeout_us);

        // a single failure is a failure of the broadcast
        if (ret == DW_FAILURE)
            continue;

        if ((messageStatus.status != DW_SUCCESS) || (messageStatus.transmissionSize != buffer_size))
            ret = DW_FAILURE;
    }

    return ret;
}

SocketConnectionPool::~SocketConnectionPool()
{
    if (m_dropCount > 0)
        LOGSTREAM_ERROR(this) << "SocketConnectionPool: Total "
                              << m_dropCount << " packet(s) dropped."
                              << Logger::State::endl;
}

#pragma GCC diagnostic pop
