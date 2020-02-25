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
// Copyright (c) 2016-2019 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//
/////////////////////////////////////////////////////////////////////////////////////////

#include "../SocketClientServer.h"
#include "../SocketClientServer_private.h"

#include "SocketClientServer.hpp"

#include <dw/core/Context.hpp>
#include <dw/core/Exception.hpp>
#include <dw/core/Profiling.hpp>

using namespace dw;
using namespace dw::ipc;

using SocketConnectionSharedPtr = std::shared_ptr<dw::ipc::SocketConnection>;

DECLARE_HANDLE_TYPE(dw::ipc::SocketServer, dwSocketServerObject)
DECLARE_HANDLE_TYPE(dw::ipc::SocketClient, dwSocketClientObject)
DECLARE_HANDLE_TYPE(SocketConnectionSharedPtr, dwSocketConnectionObject)

dwStatus dwSocketServer_initialize(dwSocketServerHandle_t* server, uint16_t port, size_t connection_pool_size,
                                   dwContextHandle_t context)
{
    FUNCTION_RANGE;
    return dw::core::Exception::guard([&] {
        makeUniqueCHandle(server,
                          makeUnique<SocketServer>(port, connection_pool_size, CHandle::cast(context)));
    });
}

dwStatus dwSocketServer_initializeNew(dwSocketServerHandle_t* server, const dwSocketServerParams* params,
                                      dwContextHandle_t context)
{
    FUNCTION_RANGE;
    return dw::core::Exception::guard([&] {
        makeUniqueCHandle(server,
                          makeUnique<SocketServer>(params->port,
                                                   params->connectionPoolSize,
                                                   CHandle::cast(context),
                                                   params->reusePort));
    });
}

dwStatus dwSocketServer_accept(dwSocketConnectionHandle_t* connection, dwTime_t timeout_us,
                               dwSocketServerHandle_t server)
{
    FUNCTION_RANGE;
    return dw::core::Exception::guardWithReturn([&] {
        THROW_IF_PARAM_NULL(server);

        auto ret = dw::CHandle::cast(server)->accept(timeout_us);
        if (ret.status == DW_SUCCESS)
            makeUniqueCHandle(connection, makeUnique<SocketConnectionSharedPtr>(ret.connection));

        return ret.status;
    });
}

dwStatus dwSocketServer_broadcast(const uint8_t* buffer, size_t buffer_size, dwSocketServerHandle_t server)
{
    FUNCTION_RANGE;
    return dw::core::Exception::guardWithReturn([&] {
        THROW_IF_PARAM_NULL(buffer);
        THROW_IF_PARAM_NULL(server);

        return dw::CHandle::cast(server)->broadcast(buffer, buffer_size);
    });
}

dwStatus dwSocketServer_release(dwSocketServerHandle_t server)
{
    FUNCTION_RANGE;
    return dw::core::Exception::guard([&] { deleteUniqueCHandle(server); });
}

dwStatus dwSocketClient_initialize(dwSocketClientHandle_t* client, size_t connection_pool_size,
                                   dwContextHandle_t context)
{
    FUNCTION_RANGE;
    return dw::core::Exception::guard([&] {
        makeUniqueCHandle(client, makeUnique<SocketClient>(connection_pool_size, CHandle::cast(context)));
    });
}

dwStatus dwSocketClient_connect(dwSocketConnectionHandle_t* connection, const char* server_ip, uint16_t port,
                                dwTime_t timeout_us, dwSocketClientHandle_t client)
{
    FUNCTION_RANGE;
    return dw::core::Exception::guardWithReturn([&] {
        THROW_IF_PARAM_NULL(server_ip);
        THROW_IF_PARAM_NULL(client);

        auto ret = dw::CHandle::cast(client)->connect(server_ip, port, timeout_us);
        if (ret.status == DW_SUCCESS)
            makeUniqueCHandle(connection, makeUnique<SocketConnectionSharedPtr>(ret.connection));

        return ret.status;
    });
}

dwStatus dwSocketClient_broadcast(const uint8_t* buffer, size_t buffer_size, dwSocketClientHandle_t client)
{
    FUNCTION_RANGE;
    return dw::core::Exception::guardWithReturn([&] {
        THROW_IF_PARAM_NULL(buffer);
        THROW_IF_PARAM_NULL(client);

        return dw::CHandle::cast(client)->broadcast(buffer, buffer_size);
    });
}

dwStatus dwSocketClient_release(dwSocketClientHandle_t client)
{
    FUNCTION_RANGE;
    return dw::core::Exception::guard([&] { deleteUniqueCHandle(client); });
}

dwStatus dwSocketConnection_write(const void* buffer, size_t* buffer_size, dwTime_t timeout_us,
                                  dwSocketConnectionHandle_t connection)
{
    FUNCTION_RANGE;
    return dw::core::Exception::guardWithReturn([&] {
        THROW_IF_PARAM_NULL(buffer);
        THROW_IF_PARAM_NULL(buffer_size);
        THROW_IF_PARAM_NULL(connection);

        auto const ret = dw::CHandle::cast(connection)->get()->send(buffer, *buffer_size, timeout_us);
        *buffer_size   = ret.transmissionSize;
        return ret.status;
    });
}

dwStatus dwSocketConnection_peek(uint8_t* buffer, size_t* buffer_size,
                                 dwTime_t timeout_us,
                                 dwSocketConnectionHandle_t connection)
{
    FUNCTION_RANGE;
    return dw::core::Exception::guardWithReturn([&] {
        THROW_IF_PARAM_NULL(buffer);
        THROW_IF_PARAM_NULL(buffer_size);
        THROW_IF_PARAM_NULL(connection);

        auto const ret = dw::CHandle::cast(connection)->get()->peek(buffer, *buffer_size, timeout_us);
        if (ret.status == DW_SUCCESS)
            *buffer_size = ret.transmissionSize;
        else
            *buffer_size = 0;
        return ret.status;
    });
}

dwStatus dwSocketConnection_read(void* buffer, size_t* buffer_size, dwTime_t timeout_us, dwSocketConnectionHandle_t connection)
{
    FUNCTION_RANGE;
    return dw::core::Exception::guardWithReturn([&] {
        THROW_IF_PARAM_NULL(buffer);
        THROW_IF_PARAM_NULL(buffer_size);
        THROW_IF_PARAM_NULL(connection);

        auto const ret = dw::CHandle::cast(connection)->get()->recv(buffer, *buffer_size, timeout_us);
        *buffer_size   = ret.transmissionSize;
        return ret.status;
    });
}

dwStatus dwSocketConnection_release(dwSocketConnectionHandle_t connection)
{
    FUNCTION_RANGE;
    return dw::core::Exception::guard([&] { deleteUniqueCHandle(connection); });
}

dwStatus dwSocketConnection_getSockaddr(dwSocketAddrIn* addr, dwSocketConnectionHandle_t connection)
{
    FUNCTION_RANGE;
    return dw::core::Exception::guardWithReturn([&] {
        THROW_IF_PARAM_NULL(addr);

        return dw::CHandle::cast(connection)->get()->getSockaddr(addr);
    });
}

dwStatus dwSocketConnection_getFd(int32_t* fd, const dwSocketConnectionHandle_t connection)
{
    FUNCTION_RANGE;
    return dw::core::Exception::guardWithReturn([&] {
        THROW_IF_PARAM_NULL(fd);
        *fd = dw::CHandle::cast(connection)->get()->get_sockfd();
        return DW_SUCCESS;
    });
}
