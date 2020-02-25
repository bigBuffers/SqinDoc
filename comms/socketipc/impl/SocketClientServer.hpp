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

#ifndef DW_IPC_SOCKETCLIENTSERVER_HPP_
#define DW_IPC_SOCKETCLIENTSERVER_HPP_

// C++ interface
#include <dw/core/Object.hpp>
#include <dw/core/Types.hpp>
#include <dw/core/UniqueHandle.hpp>
#include <dw/core/container/VectorFixed.hpp>

#include <dw/comms/socketipc/SocketClientServer.h>

#include <memory>

struct addrinfo;

namespace dw
{
namespace ipc
{

using UniqueSocketDescriptorHandle = UniqueHandle<int32_t, std::function<void(int32_t)>, -1>;
constexpr dwTime_t ONE_SECOND_US   = 1000 * 1000;

class SocketConnection : public Object
{
public:
    SocketConnection(UniqueSocketDescriptorHandle connectionSocket, Context* ctx);
    ~SocketConnection() override;

    // Counter for packet skip
    uint64_t m_skipCount = 0;

    // Message Transfer
    struct MessageStatus
    {
        dwStatus status;
        size_t transmissionSize;
    };
    // sends content of the buffer
    MessageStatus send(const void* buffer, size_t buffer_size, dwTime_t timeout = DW_TIMEOUT_INFINITE);

    // reads into buffer (returns number of read bytes if successful)
    MessageStatus recv(void* buffer, size_t buffer_size, dwTime_t timeout = DW_TIMEOUT_INFINITE);
    // DW_TIMEOUT_INFINITE timeout shall be totally blocking
    MessageStatus peek(uint8_t* buffer, size_t buffer_size, dwTime_t timeout_us);
    static void setNonBlocking(int32_t sockfd);
    static void unsetNonBlocking(int32_t sockfd);
    void setSendTimeout(dwTime_t timeout_us);
    void setRecvTimeout(dwTime_t timeout_us);
    int32_t get_sockfd()
    {
        return m_connectionSocket.get();
    }
    dwStatus getSockaddr(dwSocketAddrIn* addr);

private:
    void reset() override;

    ssize_t peekBlock(uint8_t* buffer, size_t buffer_size);
    ssize_t peekBlockWithTimeout(uint8_t* buffer, size_t buffer_size, dwTime_t timeout_us);
    ssize_t peekNonBlock(uint8_t* buffer, size_t buffer_size);
    ssize_t processRecvWithTimeout(void* buffer, size_t buffer_size, dwTime_t timeout_us);
    inline timeval timeoutToTimeval(dwTime_t timeout_us)
    {
        timeval tv = {
            .tv_sec  = static_cast<decltype(timeval::tv_sec)>(timeout_us / ONE_SECOND_US),
            .tv_usec = static_cast<decltype(timeval::tv_usec)>(timeout_us % ONE_SECOND_US)};

#ifdef VIBRANTE_V5Q
        // Some of the QNX socket timeouts are restricted to 32 seconds or under (eg., SO_SNDTIMEO)
        if (tv.tv_sec > 31)
        {
            tv.tv_sec = 31;
        }
#endif

        return tv;
    }

    UniqueSocketDescriptorHandle m_connectionSocket;
    dwTime_t m_sendTimeoutUs = 0;
    dwTime_t m_recvTimeoutUs = 0;
};

class SocketConnectionPool : public Object
{
public:
    SocketConnectionPool(size_t poolSize, Context* ctx);

    ~SocketConnectionPool() override;

    // acquires a free pool slot, or nullptr if no slot is free
    std::weak_ptr<SocketConnection>* getFreeSlot();

    // broadcasts content of the buffer to all valid connections
    dwStatus broadcast(const uint8_t* buffer, size_t buffer_size, dwTime_t timeout_us = DW_TIMEOUT_INFINITE);

private:
    friend class SocketServer;
    friend class SocketClient;

    constexpr static size_t SOCK_SELECT_AT_BROADCAST_TIMEOUT = 100;
    // Log interval in (2^n)-1 format
    constexpr static size_t REPEATED_ERR_LOG_INTERVAL_MINUS1 = 511;

    VectorFixed<std::weak_ptr<SocketConnection>> m_connectionPool;
    // Counter for packet drop
    uint64_t m_dropCount = 0;

    void reset() override;

    bool isBroadcastPossible(fd_set& wfdset);
};

class SocketServer : public Object
{
public:
    SocketServer(uint16_t port, size_t connectionPoolSize, Context* ctx, bool reusePort = true);

    void reset() override;

    // accept an incoming connection until the timeout is reached
    struct AcceptStatus
    {
        dwStatus status;
        std::shared_ptr<SocketConnection> connection;
    };
    AcceptStatus accept(dwTime_t timeout_us);

    // broadcasts content of the buffer to all valid connections
    dwStatus broadcast(const uint8_t* buffer, size_t buffer_size, dwTime_t timeout_us = DW_TIMEOUT_INFINITE);

protected:
    UniqueSocketDescriptorHandle m_serverSocket;

private:
    SocketConnectionPool m_connectionPool;
};

class SocketClient : public Object
{
public:
    SocketClient(size_t connectionPoolSize, Context* ctx);

    void reset() override;

    // connect a connection to a server until the timeout is reached
    struct ConnectStatus
    {
        dwStatus status{DW_FAILURE};
        std::shared_ptr<SocketConnection> connection = nullptr;
    };
    ConnectStatus connect(const char* server_ip, uint16_t port, dwTime_t timeout_us);

    // broadcasts content of the buffer to all valid connections
    dwStatus broadcast(const uint8_t* buffer, size_t buffer_size, dwTime_t timeout_us = DW_TIMEOUT_INFINITE);

private:
    constexpr static size_t MAX_CONN_ATTEMPT = 3;
    SocketConnectionPool m_connectionPool;

    void attemptConnect(ConnectStatus& ret,
                        struct addrinfo* gai_results,
                        dwTime_t then_us,
                        dwTime_t attempt_timeout_us);
    void handleConnect(ConnectStatus& ret,
                       UniqueSocketDescriptorHandle& connectionSocket,
                       const struct addrinfo* serv_addr,
                       dwTime_t then_us,
                       dwTime_t timeout_us);
    bool handleConnectInProgress(ConnectStatus& ret,
                                 UniqueSocketDescriptorHandle& connectionSocket,
                                 dwTime_t timeout_us); // returns requirement to continue trying
};

} // namespace ipc
} // namespace dw

#endif // DW_IPC_SOCKETCLIENTSERVER_HPP_
