# Copyright (c) 2018-2019 NVIDIA CORPORATION.  All rights reserved.

@page ipc_mainsection IPC

## About This Module

Simple communication between processes is often a the base layer for more complex systems, e.g., running on different devices. The NVIDIA<sup>&reg;</sup> DriveWorks IPC module provides simple functionality enabling platform-agnostic inter-process communication.

Network sockets are used as the communication channel of the DriveWorks socket client/server API. A server is initialized with `dwSocketServer_initialize()` and will accept incoming connections from a client with `dwSocketServer_accept()`. Clients (initialized with `dwSocketClient_initialize()`) connect to a server with `dwSocketClient_connect()`. Both accepting (server) and connecting (client) result in a connection `::dwSocketConnectionHandle_t` in both processes, which can be used to transmit (`dwSocketConnection_write()`) and receive (`dwSocketConnection_read()`) data.

## Relevant Tutorials

- @ref ipc_usecase1

## APIs

- @ref ipc_group
