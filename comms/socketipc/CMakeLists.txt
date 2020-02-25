# Copyright (c) 2016-2019 NVIDIA Corporation. All rights reserved.
set(CODEOWNERS @muddin)

project(dw_comms_socketipc C CXX)

#-------------------------------------------------------------------------------
# Project files
#-------------------------------------------------------------------------------
set(PUBLIC_HEADERS
    SocketClientServer.h
)

set(SOURCES
    impl/SocketClientServer.hpp
    impl/SocketClientServer.cpp

    impl/SocketClientServer_interface.cpp

    SocketClientServer_private.h
)

set(PUBLIC_C_LIBRARIES
)

set(PUBLIC_CXX_LIBRARIES
    dw_core
)

set(PRIVATE_LIBRARIES
)

set(PUBLIC_DOCS
    docs/mainsection.md
    docs/usecase1.md
)

#-------------------------------------------------------------------------------
# Enable auto formatting
#-------------------------------------------------------------------------------
sdk_enable_auto_formatting("${CMAKE_CURRENT_SOURCE_DIR}")

#-------------------------------------------------------------------------------
# Final target
#-------------------------------------------------------------------------------
sdk_add_module_library("${PROJECT_NAME}"
                       "${PUBLIC_HEADERS}"
                       "${SOURCES}"
                       "${PUBLIC_C_LIBRARIES}"
                       "${PUBLIC_CXX_LIBRARIES}"
                       "${PRIVATE_LIBRARIES}")
