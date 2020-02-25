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

/**
 * @file
 * <b>NVIDIA DriveWorks API: Inter Process Communication</b>
 *
 * @b Description: This file defines methods for inter-process communication.
 */

/**
 * @defgroup ipc_group IPC Interface
 *
 * @brief Provides functionality for inter-process communication.
 *
 * @{
 */

#ifndef DW_IPC_SOCKETCLIENTSERVER_H_
#define DW_IPC_SOCKETCLIENTSERVER_H_

#include <dw/core/Config.h>
#include <dw/core/Context.h>
#include <dw/core/Exports.h>
#include <dw/core/Status.h>
#include <dw/core/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Handle representing the a network socket server.
typedef struct dwSocketServerObject* dwSocketServerHandle_t;

/// Handle representing the a network socket client.
typedef struct dwSocketClientObject* dwSocketClientHandle_t;

/// Handle representing the a bi-directional client-server network socket connection.
typedef struct dwSocketConnectionObject* dwSocketConnectionHandle_t;

/**
 * @brief A structure contains data for POSIX sockaddr_in
 */
typedef struct
{
    uint16_t port;
    uint32_t ip;
} dwSocketAddrIn;

/**
 * @brief Creates and initializes a socket server accepting incoming client connections.
 *
 * @param[out] server A pointer to the server handle will be returned here.
 * @param[in] port The network port the server is listening on.
 * @param[in] connection_pool_size The maximal number of concurrently acceptable connections.
 * @param[in] context Specifies the handle to the context under which the socket server is created.
 *
 * @return DW_CANNOT_CREATE_OBJECT - if socket initialization failed. <br>
 *         DW_INVALID_HANDLE - if provided context handle is invalid,i.e. null or of wrong type  <br>
 *         DW_SUCCESS - if server is created successfully.
 */
DW_API_PUBLIC
dwStatus dwSocketServer_initialize(dwSocketServerHandle_t* server, uint16_t port, size_t connection_pool_size,
                                   dwContextHandle_t context);

/**
 * @brief Accepts an incoming connection at a socket server.
 *
 * @param[out] connection A pointer to the socket connection handle will be returned here.
 * @param[in] timeout_us Timeout to block this call.
 * @param[in] server A handle to the socket server.
 *
 * @return DW_BUFFER_FULL - if the server's connection pool is depleted. <br>
 *         DW_TIME_OUT - if no connection could be accepted before the timeout. <br>
 *         DW_INVALID_ARGUMENT - if the server handle is invalid. <br>
 *         DW_SUCCESS - if a socket connection was established.
 */
DW_API_PUBLIC
dwStatus dwSocketServer_accept(dwSocketConnectionHandle_t* connection, dwTime_t timeout_us,
                               dwSocketServerHandle_t server);

/**
 * @brief Broadcasts a message to all connected sockets of the pool.
 *
 * @param[in] buffer A pointer to the data to be send.
 * @param[in] buffer_size The number of bytes to send.
 * @param[in] server A handle to the socket server.
 *
 * @return DW_INVALID_HANDLE - if buffer or socket handle is invalid,i.e. null or of wrong type  <br>
 *         DW_FAILURE - if a single message was not send successfully to all connections. <br>
 *         DW_SUCCESS - if all messages to all connections were send successfully.
 */
DW_API_PUBLIC
dwStatus dwSocketServer_broadcast(const uint8_t* buffer, size_t buffer_size, dwSocketServerHandle_t server);

/**
 * @brief Terminate a socket server.
 *
 * @param[in] server A handle to the socket server.
 *
 * @return DW_FAILURE - if server was not terminated cleanly <br>
 *         DW_SUCCESS - if server was terminated cleanly.
 */
DW_API_PUBLIC
dwStatus dwSocketServer_release(dwSocketServerHandle_t server);

/**
 * @brief Creates and initializes a socket client.
 *
 * @param[out] client A pointer to the client handle will be returned here.
 * @param[in] connection_pool_size The maximal number of concurrently connected connections.
 * @param[in] context Specifies the handle to the context under which the socket client is created.
 *
 * @return DW_INVALID_HANDLE - if provided context handle is invalid, i.e. null or of wrong type <br>
 *         DW_SUCCESS - when client was created successfully.
 */
DW_API_PUBLIC
dwStatus dwSocketClient_initialize(dwSocketClientHandle_t* client, size_t connection_pool_size,
                                   dwContextHandle_t context);

/**
 * @brief Connects a socket connection to a listening socket server.
 *
 * @param[out] connection A pointer to the socket connection handle will be returned here.
 * @param[in] host A pointer to string representation of the the server's IP address or hostname.
 * @param[in] port The network port the server is listening on.
 * @param[in] timeout_us Timeout to block this call.
 * @param[in] client A handle to the socket client.
 *
 * @return DW_BUFFER_FULL - if the clients's connection pool is depleted. <br>
 *         DW_CANNOT_CREATE_OBJECT - if the socket can't be created or the server's IP/hostname is invalid <br>
 *         DW_TIME_OUT - if no connection could be accepted before the timeout. <br>
 *         DW_INVALID_ARGUMENT - if the client handle is invalid. <br>
 *         DW_SUCCESS - if a socket connection was established.
 */
DW_API_PUBLIC
dwStatus dwSocketClient_connect(dwSocketConnectionHandle_t* connection, const char* host, uint16_t port,
                                dwTime_t timeout_us, dwSocketClientHandle_t client);

/**
 * @brief Broadcasts a message to all connected sockets of the pool.
 *
 * @param[in] buffer A pointer to the data to be send.
 * @param[in] buffer_size The number of bytes to send.
 * @param[in] client A handle to the socket client.
 *
 * @return DW_INVALID_HANDLE - if buffer or socket handle is invalid, i.e. null or of wrong type  <br>
 *         DW_FAILURE - if a single message was not send successfully to all connections. <br>
 *         DW_SUCCESS - if all messages to all connections were send successfully.
 */
DW_API_PUBLIC
dwStatus dwSocketClient_broadcast(const uint8_t* buffer, size_t buffer_size, dwSocketClientHandle_t client);

/**
 * @brief Terminate a socket client.
 *
 * @param[in] client A handle to the socket client.
 *
 * @return DW_FAILURE - if client was not terminated cleanly <br>
 *         DW_SUCCESS - if client was terminated cleanly.
 */
DW_API_PUBLIC
dwStatus dwSocketClient_release(dwSocketClientHandle_t client);

/**
 * Send a message of a given length through the socket connection with a timeout.
 * While sending has not timedout, the buffer will be retried to be sent as long as possible.
 *
 * @param[in] buffer A pointer to the data to be send.
 * @param[in,out] buffer_size A pointer to the number of bytes to send, and the actual number of bytes sent.
 * @param[in] timeout_us Amount of time to try to send the data
 * @param[in] connection A handle to the network socket connection.
 *
 * @return DW_INVALID_HANDLE - if buffer or socket handle is invalid,i.e. null or of wrong type <br>
 *         DW_END_OF_STREAM - if the connection ended. <br>
 *         DW_FAILURE - if data could not be transmitted. <br>
 *         DW_TIME_OUT - if no data could be transmitted within the requested timeout time. <br>
 *         DW_SUCCESS - if data could be send.
 *
 * @note Even if the method times out, at least partially the data could have been sent.
 */
DW_API_PUBLIC
dwStatus dwSocketConnection_write(const void* buffer, size_t* buffer_size, dwTime_t timeout_us,
                                  dwSocketConnectionHandle_t connection);

/**
 * @brief Peek at a message of a given length from the network connection (blocking within timeout period).
 *
 * @param[in] buffer A pointer to the memory location data is written to.
 * @param[in,out] buffer_size A pointer to the number of bytes to receive, and the actual number of bytes
 *                            received on success.
 * @param[in] timeout_us Timeout to block this call.
 *                       Specify 0 for non-blocking behavior.
 *                       Specify DW_TIMEOUT_INFINITE for infinitely blocking behavior until data is available.
 * @param[in] connection A handle to the network socket connection.
 *
 * @return DW_INVALID_HANDLE - if buffer or socket handle is invalid,i.e. null or of wrong type  <br>
 *         DW_END_OF_STREAM - if the connection ended. <br>
 *         DW_TIME_OUT - if no data is available to be received before the timeout. <br>
 *         DW_FAILURE - if data could not be received or timeout could not be set. <br>
 *         DW_SUCCESS - if data could be received.
 */
DW_API_PUBLIC
dwStatus dwSocketConnection_peek(uint8_t* buffer, size_t* buffer_size,
                                 dwTime_t timeout_us,
                                 dwSocketConnectionHandle_t connection);

/**
 * Receive a message of a given length from the network connection. The method blocks for the provided
 * amount of time to receive the data.
 *
 * @param[in] buffer A pointer to the memory location data is written to.
 * @param[in,out] buffer_size A pointer to the number of bytes to receive, and the actual number of bytes received.
 * @param[in] timeout_us Time to wait to receive the content. Can be 0 for non-blocking and DW_TIMEOUT_INFINITE for blocking mode
 * @param[in] connection A handle to the network socket connection.
 *
 * @return DW_INVALID_HANDLE - if buffer or socket handle is invalid,i.e. null or of wrong type  <br>
 *         DW_END_OF_STREAM - if the connection ended. <br>
 *         DW_TIME_OUT - if timed out before any data could be received <br>
 *         DW_FAILURE - if data could not be received. <br>
 *         DW_SUCCESS - if data could be received.
 */
DW_API_PUBLIC
dwStatus dwSocketConnection_read(void* buffer, size_t* buffer_size, dwTime_t timeout_us, dwSocketConnectionHandle_t connection);

/**
 * @brief Terminate a socket connection.
 *
 * @param[in] connection A handle to the socket connection.
 *
 * @return DW_FAILURE - if connection was not terminated cleanly <br>
 *         DW_SUCCESS - if connection was terminated cleanly.
 */
DW_API_PUBLIC
dwStatus dwSocketConnection_release(dwSocketConnectionHandle_t connection);

#ifdef __cplusplus
}
#endif

/** @} */

#endif // DW_IPC_SOCKETCLIENTSERVER_H_
