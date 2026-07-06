cmake_minimum_required(VERSION 3.10)
project(libscreenmirror VERSION 2.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g -Wall -Wextra -Wno-unused-parameter -fPIC -pthread")

# ── Feature options ────────────────────────────────────────────────────────
option(H133_BOARD     "Enable H133 BSP hardware support" OFF)
option(ENABLE_WIFI    "Enable WiFi management (requires libwifimg)"        OFF)
option(ENABLE_BT      "Enable Bluetooth management (requires libbtmanager)" OFF)
option(ENABLE_MIRACAST "Enable Miracast protocol (requires libwfd2)"       OFF)
option(ENABLE_DLNA    "Enable DLNA protocol (requires libawdlna)"          OFF)
option(ENABLE_AIRPLAY "Enable AirPlay protocol (loaded at runtime)"        OFF)
option(ENABLE_WIRED   "Enable USB wired cast (requires libwirelesscast)"   OFF)

# H133_SYSROOT can be passed via cmake -DH133_SYSROOT=/path/to/sysroot
set(H133_SYSROOT "" CACHE PATH "Path to H133 cross-compile sysroot")

# If sysroot is provided, force-enable H133_BOARD
if(H133_SYSROOT)
    set(H133_BOARD ON)
    set(CMAKE_SYSROOT "${H133_SYSROOT}")
    set(BSP_LIB_DIR   "${H133_SYSROOT}/usr/lib")
    set(BSP_INC_DIR   "${H133_SYSROOT}/usr/include")
    include_directories(${BSP_INC_DIR})
    link_directories(${BSP_LIB_DIR})
endif()

# ── Include directories ────────────────────────────────────────────────────
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/bsp
)

# ── Source files ───────────────────────────────────────────────────────────
set(CORE_SOURCES
    src/core/mirror_engine.c
    src/core/event_system.c
    src/core/state_machine.c
)

set(CONNECTIVITY_SOURCES
    src/connectivity/wifi_core.c
    src/connectivity/bt_core.c
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
    ${CONNECTIVITY_SOURCES}
    ${PROTOCOL_SOURCES}
    ${NETWORK_SOURCES}
    ${PLATFORM_SOURCES}
)

# ── Compile definitions ────────────────────────────────────────────────────
set(SCREENMIRROR_DEFINES)

if(H133_BOARD)
    list(APPEND SCREENMIRROR_DEFINES H133_BOARD)
endif()

if(ENABLE_WIFI)
    list(APPEND SCREENMIRROR_DEFINES ENABLE_WIFI LV_USE_LINUX_WIFI2_MODE)
endif()

if(ENABLE_BT)
    list(APPEND SCREENMIRROR_DEFINES ENABLE_BT LV_USE_LINUX_BT4_MODE)
endif()

if(ENABLE_MIRACAST)
    list(APPEND SCREENMIRROR_DEFINES ENABLE_MIRACAST)
endif()

if(ENABLE_DLNA)
    list(APPEND SCREENMIRROR_DEFINES ENABLE_DLNA)
endif()

if(ENABLE_AIRPLAY)
    list(APPEND SCREENMIRROR_DEFINES ENABLE_AIRPLAY)
endif()

if(ENABLE_WIRED)
    list(APPEND SCREENMIRROR_DEFINES ENABLE_WIRED)
endif()

# ── BSP libraries (H133 only) ──────────────────────────────────────────────
set(BSP_LINK_LIBS pthread dl)

if(H133_BOARD)
    if(ENABLE_WIFI)
        list(APPEND BSP_LINK_LIBS wifimg)
    endif()
    if(ENABLE_BT)
        list(APPEND BSP_LINK_LIBS btmanager)
    endif()
    if(ENABLE_MIRACAST)
        list(APPEND BSP_LINK_LIBS wfd2 wpa_client)
    endif()
    if(ENABLE_DLNA)
        list(APPEND BSP_LINK_LIBS awdlna)
    endif()
    # libthirdparty_mirror.so is dlopen'd at runtime — no link-time dep needed
endif()

# ── Static library ─────────────────────────────────────────────────────────
add_library(screenmirror_static STATIC ${ALL_SOURCES})
set_target_properties(screenmirror_static PROPERTIES
    OUTPUT_NAME screenmirror
    VERSION     ${PROJECT_VERSION}
)
target_include_directories(screenmirror_static PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/bsp
)
target_compile_definitions(screenmirror_static PRIVATE ${SCREENMIRROR_DEFINES})
target_link_libraries(screenmirror_static PRIVATE ${BSP_LINK_LIBS})

# ── Shared library ─────────────────────────────────────────────────────────
add_library(screenmirror_shared SHARED ${ALL_SOURCES})
set_target_properties(screenmirror_shared PROPERTIES
    OUTPUT_NAME screenmirror
    VERSION     ${PROJECT_VERSION}
    SOVERSION   2
)
target_include_directories(screenmirror_shared PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/bsp
)
target_compile_definitions(screenmirror_shared PRIVATE ${SCREENMIRROR_DEFINES})
target_link_libraries(screenmirror_shared PRIVATE ${BSP_LINK_LIBS})

# ── Installation ───────────────────────────────────────────────────────────
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
    ${CMAKE_SOURCE_DIR}/include/bsp
    DESTINATION include
)

# ── Test / validation executable ───────────────────────────────────────────
if(BUILD_TESTS)
    add_executable(screenmirror_test examples/basic_example.c)
    target_link_libraries(screenmirror_test PRIVATE screenmirror_static ${BSP_LINK_LIBS})
    target_include_directories(screenmirror_test PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    target_compile_definitions(screenmirror_test PRIVATE ${SCREENMIRROR_DEFINES})
endif()

# ── Summary ────────────────────────────────────────────────────────────────
message(STATUS "libscreenmirror v${PROJECT_VERSION}")
message(STATUS "  H133_BOARD   : ${H133_BOARD}")
message(STATUS "  ENABLE_WIFI  : ${ENABLE_WIFI}")
message(STATUS "  ENABLE_BT    : ${ENABLE_BT}")
message(STATUS "  ENABLE_MIRACAST: ${ENABLE_MIRACAST}")
message(STATUS "  ENABLE_DLNA  : ${ENABLE_DLNA}")
message(STATUS "  ENABLE_AIRPLAY: ${ENABLE_AIRPLAY}")
message(STATUS "  ENABLE_WIRED : ${ENABLE_WIRED}")
message(STATUS "  H133_SYSROOT : ${H133_SYSROOT}")
