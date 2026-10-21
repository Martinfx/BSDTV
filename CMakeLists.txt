cmake_minimum_required(VERSION 3.21)

project(BSDTV VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Network Qml Quick LinguistTools)
qt_standard_project_setup(REQUIRES 6.8)

include(GNUInstallDirs)
set(BSDTV_DATA_DIR "${CMAKE_INSTALL_FULL_DATADIR}/bsdtv")

# Everything except main() lives in a library so the tests can link it too.
qt_add_library(bsdtvcore STATIC
    src/appmodel.cpp src/appmodel.h
    src/apprunner.cpp src/apprunner.h
    src/apps.cpp src/apps.h
    src/backend.cpp src/backend.h
    src/cec.cpp src/cec.h
    src/config.cpp src/config.h
    src/ctl.cpp src/ctl.h
    src/jellyfin.cpp src/jellyfin.h
    src/remoteserver.cpp src/remoteserver.h
    src/systemtools.cpp src/systemtools.h
)
target_include_directories(bsdtvcore PUBLIC src)
target_compile_definitions(bsdtvcore PUBLIC
    BSDTV_VERSION="${PROJECT_VERSION}"
    BSDTV_DATA_DIR="${BSDTV_DATA_DIR}"
    # Lets a build run straight from the source tree (themes, etc/*.json).
    BSDTV_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(bsdtvcore PUBLIC Qt6::Core Qt6::Gui Qt6::Network Qt6::Qml Qt6::Quick)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(bsdtvcore PRIVATE -Wall -Wextra)
endif()

file(GLOB BSDTV_QML_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" CONFIGURE_DEPENDS qml/*.qml)
qt_add_qml_module(bsdtvcore
    URI BSDTV
    VERSION 1.0
    QML_FILES ${BSDTV_QML_FILES}
    RESOURCES web/remote.html
    NO_PLUGIN_OPTIONAL
)

# The translations cover the QML files, which belong to the library.
qt_add_translations(bsdtvcore
    TS_FILES i18n/bsdtv_cs.ts
    RESOURCE_PREFIX /i18n
    LUPDATE_OPTIONS -no-obsolete
)

qt_add_executable(bsdtv src/main.cpp)
target_link_libraries(bsdtv PRIVATE bsdtvcore bsdtvcoreplugin)

install(TARGETS bsdtv RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(PROGRAMS scripts/bsdtv-ctl scripts/bsdtv-cec system/bsdtv-session system/bsdtv-drm-browser
        DESTINATION ${CMAKE_INSTALL_BINDIR})
set(BSDTV_DATA_DEST "${CMAKE_INSTALL_DATADIR}/bsdtv")
install(DIRECTORY themes/ DESTINATION ${BSDTV_DATA_DEST}/themes)
install(FILES etc/config.json etc/apps.json etc/adblock-lists.txt etc/adblock-allow.txt
        DESTINATION ${BSDTV_DATA_DEST}/etc)
install(DIRECTORY system/ DESTINATION ${BSDTV_DATA_DEST}/system)
install(PROGRAMS tools/adblock-update.sh DESTINATION ${CMAKE_INSTALL_SBINDIR} RENAME bsdtv-adblock-update)

option(BSDTV_BUILD_TESTS "Build the tests" ON)
if(BSDTV_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
