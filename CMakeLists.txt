cmake_minimum_required(VERSION 3.15)
project(pulsenet_udp LANGUAGES CXX)

# Determine standalone build
if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(PULSENET_UDP_STANDALONE_BUILD ON)
else()
    set(PULSENET_UDP_STANDALONE_BUILD OFF)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

# Source files based on platform
if (WIN32)
    set(PULSENET_UDP_SRC
        src/udp_addr_win.cpp
        src/win_socket_factory_impl.cpp
        src/win_socket_impl.cpp
    )
else()
    set(PULSENET_UDP_SRC
        src/udp_addr_unix.cpp
        src/unix_socket_factory_impl.cpp
        src/unix_socket_impl.cpp
    )
endif()

add_library(pulsenet_udp STATIC
    ${PULSENET_UDP_SRC}
    include/pulse/net/udp/error_code.h
    include/pulse/net/udp/socket_factory.h
    include/pulse/net/udp/udp_addr.h
    include/pulse/net/udp/udp.h
)

add_library(pulsenet::udp ALIAS pulsenet_udp)

target_include_directories(pulsenet_udp PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_definitions(pulsenet_udp PRIVATE -D_HAS_STD_BYTE=0) # Example: fix Windows std::byte issues

# Install rules
include(CMakePackageConfigHelpers)

install(TARGETS pulsenet_udp
        EXPORT pulsenet_udpTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT pulsenet_udpTargets
        FILE pulsenet_udpTargets.cmake
        NAMESPACE pulsenet::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pulsenet_udp)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/pulsenet_udpConfigVersion.cmake"
    VERSION 1.0.0
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/pulsenet_udpConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/pulsenet_udpConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pulsenet_udp
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/pulsenet_udpConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/pulsenet_udpConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pulsenet_udp
)

if (PULSENET_UDP_STANDALONE_BUILD)
    add_executable(pulsenet_udp_test tests/IntegrationTest.cpp)
    target_link_libraries(pulsenet_udp_test PRIVATE pulsenet_udp)

    install(TARGETS pulsenet_udp_test
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    
    add_executable(pulsenet_udp_ccu_test tests/CCUTest.cpp)
    target_link_libraries(pulsenet_udp_ccu_test PRIVATE pulsenet_udp)

    install(TARGETS pulsenet_udp_ccu_test
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
