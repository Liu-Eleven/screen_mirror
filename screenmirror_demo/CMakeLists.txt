cmake_minimum_required(VERSION 3.14)
project(screenmirror_demo VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Qt 5.15
# ---------------------------------------------------------------------------
find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Quick Qml)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ---------------------------------------------------------------------------
# H133 / Tina cross-compile paths (set via cmake -DH133_SYSROOT=...)
# ---------------------------------------------------------------------------
if(DEFINED H133_SYSROOT AND NOT H133_SYSROOT STREQUAL "")
    set(CMAKE_SYSROOT "${H133_SYSROOT}")
    list(APPEND CMAKE_PREFIX_PATH "${H133_SYSROOT}/usr")

    # BSP library roots
    set(H133_USR_LIB   "${H133_SYSROOT}/usr/lib")
    set(H133_ROOT_LIB  "${H133_SYSROOT}/root-h133-p2/usr/lib")

    set(CMAKE_EXE_LINKER_FLAGS
        "${CMAKE_EXE_LINKER_FLAGS} \
         --sysroot=${H133_SYSROOT} \
         -Wl,-rpath,${H133_ROOT_LIB} \
         -Wl,-rpath-link,${H133_ROOT_LIB}")
    message(STATUS "H133 sysroot: ${H133_SYSROOT}")
else()
    message(STATUS "H133_SYSROOT not set – building for host (demo mode)")
endif()

# ---------------------------------------------------------------------------
# libscreenmirror (from sibling directory screenmirror_lib)
# ---------------------------------------------------------------------------
set(SCREENMIRROR_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../screenmirror_lib"
    CACHE PATH "Path to screenmirror_lib root")

set(SCREENMIRROR_INCLUDE "${SCREENMIRROR_ROOT}/include")
set(SCREENMIRROR_BUILD   "${SCREENMIRROR_ROOT}/build")

# Build the library if not already built
if(NOT EXISTS "${SCREENMIRROR_BUILD}/libscreenmirror.a"
   AND NOT EXISTS "${SCREENMIRROR_BUILD}/libscreenmirror.so")
    message(STATUS "Building libscreenmirror ...")
    execute_process(
        COMMAND make all
        WORKING_DIRECTORY "${SCREENMIRROR_ROOT}"
        RESULT_VARIABLE _SM_RESULT
    )
    if(NOT _SM_RESULT EQUAL 0)
        message(WARNING "libscreenmirror build failed (exit ${_SM_RESULT}) – linking may fail")
    endif()
endif()

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(SOURCES
    src/main.cpp
    src/screenmirror_manager.cpp
    src/wifi_manager.cpp
)

set(RESOURCES
    resources/qml.qrc
)

add_executable(${PROJECT_NAME} ${SOURCES} ${RESOURCES})

# ---------------------------------------------------------------------------
# Include dirs
# ---------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    src
    "${SCREENMIRROR_INCLUDE}"
)

# ---------------------------------------------------------------------------
# Link libraries
# ---------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Quick
    Qt5::Qml
    "-L${SCREENMIRROR_BUILD}" screenmirror
    dl rt pthread
)

# H133 BSP libraries (only when cross-compiling)
if(DEFINED H133_SYSROOT AND NOT H133_SYSROOT STREQUAL "")
    target_link_directories(${PROJECT_NAME} PRIVATE
        "${H133_USR_LIB}"
        "${H133_ROOT_LIB}"
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
        asound wolfssl awrecorder tplayer wfd_player
        wpa_client wfd2 aw_dlna wfd_hdcp wfd_rtsp
    )
endif()

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
