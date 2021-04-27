include(GNUInstallDirs)

add_library(ps1-memcard-protocol STATIC)
# ALIAS target to export a namespaced target even when building in-tree
add_library(PS1MemoryCardProtocol::ps1-memcard-protocol ALIAS ps1-memcard-protocol)
# add source files
add_subdirectory(src)
# library public header files location
target_include_directories(
    ps1-memcard-protocol PUBLIC
    # different include directory path depending on if using the local or installed version of library
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
# pass in version of ps1-memcard-protocol as preprocessor definitions
target_compile_definitions(
    ps1-memcard-protocol PRIVATE
    -DPROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    -DPROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    -DPROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    -DPROJECT_VERSION_STRING=${PS1_MEMCARD_PROTOCOL_ESCAPED_VERSION_STRING}
)
# set up version and soversion for the main library object
set_target_properties(
    ps1-memcard-protocol PROPERTIES
    VERSION ${PS1_MEMCARD_PROTOCOL_VERSION_STRING}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    INTERFACE_PS1MemoryCardProtocol_MAJOR_VERSION ${PROJECT_VERSION_MAJOR}
    INTERFACE_PS1MemoryCardProtocol_MINOR_VERSION ${PROJECT_VERSION_MINOR}
)
set_property(
    TARGET ps1-memcard-protocol
    APPEND PROPERTY COMPATIBLE_INTERFACE_STRING "${PS1MemoryCardProtocol_MAJOR_VERSION}.${PS1MemoryCardProtocol_MINOR_VERSION}"
)
# inherit common ps1-memcard-protocol compiler options
target_link_libraries(
    ps1-memcard-protocol
        PRIVATE
            $<BUILD_INTERFACE:ps1-memcard-protocol-compiler-options>
)

# install if we're not being built as a sub-ps1-memcard-protocol
if (NOT PS1_MEMCARD_PROTOCOL_SUBPROJECT)
    # library
    install(
        TARGETS ps1-memcard-protocol
        EXPORT PS1MemoryCardProtocolTargets
        # when a static library
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        # when a shared library on UNIX
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        # when a DLL on Windows
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    # public headers
    install(
        DIRECTORY "include/" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    # CMake export
    install(
        EXPORT PS1MemoryCardProtocolTargets
        FILE PS1MemoryCardProtocolTargets.cmake
        NAMESPACE PS1MemoryCardProtocol::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/PS1MemoryCardProtocol"
    )
    # CMake package
    include(CMakePackageConfigHelpers)
    configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/PS1MemoryCardProtocolConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/PS1MemoryCardProtocol"
    )
    # generate the version file for the config file
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/PS1MemoryCardProtocolConfigVersion.cmake"
        VERSION "${version}"
        COMPATIBILITY SameMinorVersion
    )
    install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/PS1MemoryCardProtocolConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/PS1MemoryCardProtocolConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/PS1MemoryCardProtocol"
    )
endif()
