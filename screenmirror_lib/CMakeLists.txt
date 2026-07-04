cmake_minimum_required(VERSION 3.10)
project(libscreenmirror VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g -Wall -Wextra -fPIC -pthread")

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(CORE_SOURCES
    src/core/mirror_engine.c
    src/core/event_system.c
    src/core/state_machine.c
)

set(PROTOCOL_SOURCES
    src/protocols/miracast/miracast.c
    src/protocols/airplay/airplay.c
    src/protocols/dlna/dlna.c
    src/protocols/wired/wired.c
    src/protocols/wireless/wireless.c
)

set(NETWORK_SOURCES
    src/network/discovery.c
    src/network/transport.c
)

set(PLATFORM_SOURCES
    src/platform/cedarx_platform.c
)

set(ALL_SOURCES
    ${CORE_SOURCES}
    ${PROTOCOL_SOURCES}
    ${NETWORK_SOURCES}
    ${PLATFORM_SOURCES}
)

# Static library
add_library(screenmirror_static STATIC ${ALL_SOURCES})
set_target_properties(screenmirror_static PROPERTIES
    OUTPUT_NAME screenmirror
    VERSION ${PROJECT_VERSION}
)
target_include_directories(screenmirror_static PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Shared library
add_library(screenmirror_shared SHARED ${ALL_SOURCES})
set_target_properties(screenmirror_shared PROPERTIES
    OUTPUT_NAME screenmirror
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)
target_include_directories(screenmirror_shared PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(screenmirror_shared PRIVATE pthread)

# Installation
install(TARGETS screenmirror_static screenmirror_shared
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES
    ${CMAKE_SOURCE_DIR}/include/screenmirror.h
    DESTINATION include
)

install(FILES
    ${CMAKE_SOURCE_DIR}/include/screenmirror_internal.h
    DESTINATION include
)

install(DIRECTORY
    ${CMAKE_SOURCE_DIR}/include/protocols
    ${CMAKE_SOURCE_DIR}/include/network
    ${CMAKE_SOURCE_DIR}/include/platform
    DESTINATION include
)

# Test executable (optional)
if(BUILD_TESTS)
    add_executable(screenmirror_test examples/basic_example.c)
    target_link_libraries(screenmirror_test PRIVATE screenmirror_static pthread)
    target_include_directories(screenmirror_test PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
endif()
