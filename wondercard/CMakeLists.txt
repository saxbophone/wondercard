include(GNUInstallDirs)

add_library(wondercard STATIC)
# ALIAS target to export a namespaced target even when building in-tree
add_library(WonderCard::wondercard ALIAS wondercard)
# add source files
add_subdirectory(src)
# library public header files location
target_include_directories(
    wondercard PUBLIC
    # different include directory path depending on if using the local or installed version of library
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
# pass in version of wondercard as preprocessor definitions
target_compile_definitions(
    wondercard PRIVATE
    -DPROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    -DPROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    -DPROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    -DPROJECT_VERSION_STRING=${WONDERCARD_ESCAPED_VERSION_STRING}
)
# set up version and soversion for the main library object
set_target_properties(
    wondercard PROPERTIES
    VERSION ${WONDERCARD_VERSION_STRING}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    INTERFACE_WonderCard_MAJOR_VERSION ${PROJECT_VERSION_MAJOR}
    INTERFACE_WonderCard_MINOR_VERSION ${PROJECT_VERSION_MINOR}
)
set_property(
    TARGET wondercard
    APPEND PROPERTY COMPATIBLE_INTERFACE_STRING "${WonderCard_MAJOR_VERSION}.${WonderCard_MINOR_VERSION}"
)
# inherit common wondercard compiler options
target_link_libraries(
    wondercard
        PRIVATE
            $<BUILD_INTERFACE:wondercard-compiler-options>
)

# install if we're not being built as a sub-wondercard
if (NOT WONDERCARD_SUBPROJECT)
    # library
    install(
        TARGETS wondercard
        EXPORT WonderCardTargets
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
        EXPORT WonderCardTargets
        FILE WonderCardTargets.cmake
        NAMESPACE WonderCard::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/WonderCard"
    )
    # CMake package
    include(CMakePackageConfigHelpers)
    configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/WonderCardConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/WonderCard"
    )
    # generate the version file for the config file
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/WonderCardConfigVersion.cmake"
        VERSION "${version}"
        COMPATIBILITY SameMinorVersion
    )
    install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/WonderCardConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/WonderCardConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/WonderCard"
    )
endif()
