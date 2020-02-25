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

#ifndef DW_IPC_SOCKETCLIENTSERVER_PRIVATE_H_
#define DW_IPC_SOCKETCLIENTSERVER_PRIVATE_H_

#include "SocketClientServer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    /// The network port the server is listening on.
    uint16_t port;
    /// The maximal number of concurrently acceptable connections.
    size_t connectionPoolSize;
    /// Whether to reuse the socket port with other open connections.
    bool reusePort;
} dwSocketServerParams;

/**
 * @brief Gets the address of the peer this socket connects to.
 *
 * @param[out] addr A pointer to the sockaddr object which will be populated with the result.
 * @param[in] connection A handle to the network socket connection.
 *
 * @return DW_FAILURE - if finding the peer address failed. <br>
 *         DW_INVALID_ARGUMENT - if provided addr is invalid,i.e. null or of wrong type  <br>
 *         DW_INVALID_HANDLE - if provided connection handle is invalid,i.e. null or of wrong type  <br>
 *         DW_SUCCESS - if addr is populated successfully.
 */
DW_API_PUBLIC
dwStatus dwSocketConnection_getSockaddr(dwSocketAddrIn* addr, dwSocketConnectionHandle_t connection);

/**
 * @brief Gets the file descriptor of the socket connection. Operations should not be performed on the
 *        file descriptor directly. It is only supposed to used with epoll or select
 *
 * @param[out] fd the file descriptor
 * @param[in] connection A handle to the network socket connection.
 *
 * @return DW_INVALID_ARGUMENT - if provided addr is invalid,i.e. null or of wrong type  <br>
 *         DW_INVALID_HANDLE - if provided connection handle is invalid,i.e. null or of wrong type  <br>
 *         DW_SUCCESS - if addr is populated successfully.
 */
DW_API_PUBLIC
dwStatus dwSocketConnection_getFd(int32_t* fd, const dwSocketConnectionHandle_t connection);

/**
 * @brief Creates and initializes a socket server accepting incoming client connections.
 *
 * @param[out] server A pointer to the server handle will be returned here.
 * @param[in] params The parameters to initialize with.
 * @param[in] context Specifies the handle to the context under which the socket server is created.
 *
 * @return DW_CANNOT_CREATE_OBJECT - if socket initialization failed. <br>
 *         DW_INVALID_HANDLE - if provided context handle is invalid,i.e. null or of wrong type  <br>
 *         DW_SUCCESS - if server is created successfully.
 */
DW_API_PUBLIC
dwStatus dwSocketServer_initializeNew(dwSocketServerHandle_t* server, const dwSocketServerParams* params,
                                      dwContextHandle_t context);

#ifdef __cplusplus
}
#endif

#endif // DW_IPC_SOCKETCLIENTSERVER_PRIVATE_H_
