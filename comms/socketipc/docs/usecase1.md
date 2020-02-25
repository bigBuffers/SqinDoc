# Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.

@page ipc_usecase1 IPC Workflow

The following code snippet shows the general structure of a server process receiving data

```{.cpp}
    dwSocketServer_initialize(...);

    dwSocketServer_accept(&connection, ...);
    while(true)
    {
        // receive data from client
        dwSocketConnection_read(&buffer, &buffer_size, timeout_us, connection);
    }
```

The following code snippet shows the general structure of a corresponding client process sending data

```{.cpp}
    dwSocketClient_initialize(...);

    dwSocketClient_connect(&connection, ...);
    while(true)
    {
        // send data to server
        dwSocketConnection_write(&buffer, &buffer_size, timeout_us, connection);
    }
```
This workflow is demonstrated in the following sample: @ref dwx_ipc_socketclientserver_sample
