# Packaging.cmake - CPack configuration for Sn Compiler
#
# This module configures CPack for creating distributable packages
# across Linux (DEB, RPM, TGZ), Windows (ZIP, NSIS), and macOS (TGZ, DMG).
#
# Usage: include(Packaging) at the end of CMakeLists.txt

include_guard(GLOBAL)

# Package metadata
set(CPACK_PACKAGE_NAME "sindarin")
set(CPACK_PACKAGE_VENDOR "Sindarin Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Sindarin - A statically-typed procedural language")
set(CPACK_PACKAGE_DESCRIPTION "Sindarin is a statically-typed procedural programming language that compiles to C. It features modern syntax, type inference, and seamless C interoperability.")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/your-org/sindarin")
set(CPACK_PACKAGE_CONTACT "maintainer@example.com")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

# Resource directory for packaging assets
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

# Installation components
set(CPACK_COMPONENTS_ALL compiler runtime headers configs)

set(CPACK_COMPONENT_COMPILER_DISPLAY_NAME "Sindarin Compiler")
set(CPACK_COMPONENT_COMPILER_DESCRIPTION "The sn compiler executable")
set(CPACK_COMPONENT_COMPILER_REQUIRED ON)

set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "Runtime Libraries")
set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "Pre-compiled runtime object files for linking")
set(CPACK_COMPONENT_RUNTIME_REQUIRED ON)
set(CPACK_COMPONENT_RUNTIME_DEPENDS compiler)

set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "Development Headers")
set(CPACK_COMPONENT_HEADERS_DESCRIPTION "Header files for Sindarin runtime")
set(CPACK_COMPONENT_HEADERS_DEPENDS runtime)

set(CPACK_COMPONENT_CONFIGS_DISPLAY_NAME "Configuration Files")
set(CPACK_COMPONENT_CONFIGS_DESCRIPTION "Backend configuration files")
set(CPACK_COMPONENT_CONFIGS_DEPENDS compiler)

# Output directory
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")

# Platform-specific generator configuration
if(WIN32)
    # Windows: ZIP and NSIS installer
    set(CPACK_GENERATOR "ZIP;NSIS")

    # NSIS-specific settings
    set(CPACK_NSIS_DISPLAY_NAME "Sindarin Compiler")
    set(CPACK_NSIS_PACKAGE_NAME "Sindarin")
    set(CPACK_NSIS_HELP_LINK "https://github.com/your-org/sindarin")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/your-org/sindarin")
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)

    # Installation paths
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")

    # File extension
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-windows-x64")

elseif(APPLE)
    # macOS: TGZ and DMG
    set(CPACK_GENERATOR "TGZ;DragNDrop")

    # DMG-specific settings
    set(CPACK_DMG_VOLUME_NAME "Sindarin ${CPACK_PACKAGE_VERSION}")
    set(CPACK_DMG_FORMAT "UDZO")

    # Bundle settings (for future .app support)
    set(CPACK_BUNDLE_NAME "Sindarin")

    # File extension
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-macos-x64")

else()
    # Linux: TGZ, DEB, and RPM
    set(CPACK_GENERATOR "TGZ;DEB;RPM")

    # File extension
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-linux-x64")

    # DEB-specific settings
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Sindarin Maintainers <maintainer@example.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.17), zlib1g (>= 1:1.2.0)")
    set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "gcc | clang")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/your-org/sindarin")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

    # RPM-specific settings
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_GROUP "Development/Languages")
    set(CPACK_RPM_PACKAGE_URL "https://github.com/your-org/sindarin")
    set(CPACK_RPM_PACKAGE_REQUIRES "glibc >= 2.17, zlib >= 1.2")
    set(CPACK_RPM_PACKAGE_SUGGESTS "gcc, clang")
    set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
endif()

# Source package settings (for tarball releases)
set(CPACK_SOURCE_GENERATOR "TGZ;TXZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-source")
set(CPACK_SOURCE_IGNORE_FILES
    "/\\.git/"
    "/build/"
    "/bin/"
    "/log/"
    "/\\.cache/"
    "/\\.vscode/"
    "/\\.idea/"
    "\\.o$"
    "\\.exe$"
    "CMakeCache\\.txt"
    "CMakeFiles"
)

# Install rules for packaging
# Note: These should be called from the main CMakeLists.txt after targets are defined

function(sn_configure_install)
    # Install compiler executable
    install(TARGETS sn
        RUNTIME DESTINATION bin
        COMPONENT compiler
    )

    # Install runtime objects
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/bin/lib/"
        DESTINATION lib/sindarin
        COMPONENT runtime
        FILES_MATCHING PATTERN "*.o"
    )

    # Install headers
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/bin/include/"
        DESTINATION include/sindarin
        COMPONENT headers
    )

    # Install config files
    install(FILES
        "${CMAKE_SOURCE_DIR}/etc/sn-gcc.cfg"
        "${CMAKE_SOURCE_DIR}/etc/sn-clang.cfg"
        "${CMAKE_SOURCE_DIR}/etc/sn-tcc.cfg"
        DESTINATION etc/sindarin
        COMPONENT configs
    )

    # Install documentation
    if(EXISTS "${CMAKE_SOURCE_DIR}/README.md")
        install(FILES "${CMAKE_SOURCE_DIR}/README.md"
            DESTINATION share/doc/sindarin
            COMPONENT compiler
        )
    endif()

    if(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE")
        install(FILES "${CMAKE_SOURCE_DIR}/LICENSE"
            DESTINATION share/doc/sindarin
            COMPONENT compiler
        )
    endif()
endfunction()

# Include CPack at the end
include(CPack)
