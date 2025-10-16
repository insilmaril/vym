cmake_minimum_required(VERSION 3.16)

project(vym LANGUAGES CXX)

# See also https://doc.qt.io/qt-6.5/cmake-get-started.html

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    message(STATUS "using UNIX in cmake")	# FIXME-0 still required?
endif()

set(QtComponents
    LinguistTools
    Network
    PrintSupport
    Qml
    Svg
    SvgWidgets
    Widgets
    Xml
)

set(QtLibraries
    Qt6::Network
    Qt6::PrintSupport
    Qt6::Qml
    Qt6::Svg
    Qt6::SvgWidgets
    Qt6::Widgets
    Qt6::Xml
)

# Switch on Debug mode  on command line with
# cmake -DCMAKE_BUILD_TYPE=Debug .
#
# or uncomment here:
# set(CMAKE_BUILD_TYPE Debug)

# No longer needed with qt_standard_project_setup() below...
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# On Linux DBUS is used for IPC with vym
# On openSUSE install it using:  zypper install dbus-1-devel

if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    find_package(DBus1)

    if(DBus1_FOUND)
        message(STATUS "Deteced Linux and DBUS1, adding DBUS interfaces")

        LIST(APPEND QtComponents DBus)
        LIST(APPEND QtLibraries Qt6::DBus)
    endif()
endif()

if(WIN32)
    set(OPENSSL_ROOT_DIR "C:/OpenSSL-Win64")
    find_package(OpenSSL)

    if( OPENSSL_FOUND )
        include_directories(${OPENSSL_INCLUDE_DIRS})
        link_directories(${OPENSSL_LIBRARIES})
        message(STATUS "Using OpenSSL version: ${OPENSSL_VERSION}")
    else()
        message(STATUS "No openSSL found?!")
        # Error; with REQUIRED, pkg_search_module() will throw an error by it's own
    endif()

    #target_link_libraries(${YOUR_TARGET_HERE} ${OPENSSL_LIBRARIES})
    #target_link_libraries(project_name /path/of/libssl.so /path/of/libcrypto.so)

    set(CMAKE_INSTALL_DATAROOTDIR ".")
 endif()


# https://doc.qt.io/qt-6/qt-standard-project-setup.html
find_package(Qt6 REQUIRED COMPONENTS Core)
qt_standard_project_setup()

include(GNUInstallDirs)

find_package(Qt6 COMPONENTS ${QtComponents} REQUIRED)

list(APPEND CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_SOURCE_DIR}/forms")

get_target_property(QtLibraryType Qt6::Widgets TYPE)

if(QtLibraryType STREQUAL STATIC_LIBRARY)
    message(STATUS "Static Qt linkage")
    list(APPEND QtLibraries Qt6::QSvgPlugin)
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}
)

# Source files
file(GLOB VymSources RELATIVE ${CMAKE_SOURCE_DIR} "src/*.cpp")

if(WIN32)
    list(APPEND VymSources
        src/mkdtemp.cpp
        vym.rc
    )
    ADD_COMPILE_DEFINITIONS(_USE_MATH_DEFINES)
endif()

# Add DBUS interfaces to sources, if DBUS is available on platform
if(DBus1_FOUND)
    list(APPEND VymSources src/dbus/adaptormodel.cpp src/dbus/adaptorvym.cpp)
    include_directories(${CMAKE_SOURCE_DIR}/src/dbus ${CMAKE_SOURCE_DIR}/src)
    ADD_COMPILE_DEFINITIONS(VYM_DBUS)
endif()

# Translations  (release)

    # To update the translation files based on source code changes
    # first call manually
    # lupdate-qt5 src/ -ts lang/*.ts
    #
    # Qt6 will have a dedicated macro:
    # https://doc.qt.io/qt-6/qtlinguist-cmake-qt-add-lrelease.html

#file(GLOB ts_files ABSOLUTE ${CMAKE_BINARY_DIR} "lang/*.ts")
file(GLOB ts_files RELATIVE ${CMAKE_SOURCE_DIR} "lang/*.ts")
message(STATUS "Creating ${CMAKE_BINARY_DIR}/translations")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/translations")
set_source_files_properties(${ts_files} PROPERTIES OUTPUT_LOCATION ${CMAKE_BINARY_DIR}/translations)
#set_source_files_properties(${ts_files} PROPERTIES OUTPUT_LOCATION ${CMAKE_SOURCE_DIR}/translations)

#QT5_create_translation(qm_files ${CMAKE_SOURCE_DIR} ${ts_files})
#QT5_create_translation(qm_files ${CMAKE_BINARY_DIR} ${ts_files})
#QT5_create_translation(qm_files ${CMAKE_SOURCE_DIR}/src)

QT6_add_translation(qm_files ${ts_files})
add_compile_definitions(CMAKE_SOURCE_DIR)

message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message(STATUS "ts_files: ${ts_files}")
message(STATUS "qm_files: ${qm_files}")

# Collection of files for later installation
file(GLOB vymDemos RELATIVE ${CMAKE_SOURCE_DIR} "demos/*.*")
file(GLOB vymMacros RELATIVE ${CMAKE_SOURCE_DIR} "macros/*.vys")
file(GLOB vymScripts RELATIVE ${CMAKE_SOURCE_DIR} "demos/scripts/*.*")
message(STATUS "  vymDemos: ${vymDemos}")
message(STATUS " vymMacros: ${vymMacros}")
message(STATUS "vymScripts: ${vymScripts}")

# On Unix-like systems, place app data under a dedicated subdir
# so runtime looks in .../share/vym for resources.
set(VYM_SHARE_SUBDIR "vym")
ADD_COMPILE_DEFINITIONS(VYMBASEDIR="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/${VYM_SHARE_SUBDIR}")

add_executable(vym ${VymSources} vym.qrc ${qm_files})
target_link_libraries(vym ${QtLibraries})

if(WIN32)
    set_target_properties(vym PROPERTIES WIN32_EXECUTABLE TRUE)
    install(TARGETS vym DESTINATION "${CMAKE_INSTALL_PREFIX}")
else()
    install(TARGETS vym DESTINATION bin)
endif()

if(APPLE)
    message(STATUS "Detected APPLE")

    set(MACOSX_BUNDLE_EXECUTABLE_NAME ${PROJECT_NAME})

    # Prepare Info.plist
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.insilmaril.vym"
        MACOSX_BUNDLE_ICON_FILE "vym.icns"
        MACOSX_BUNDLE TRUE
    )

    set(vymIconMac ${CMAKE_CURRENT_SOURCE_DIR}/icons/vym.icns)
    target_sources(vym PRIVATE ${vymIconMac})
    set_source_files_properties(${vymIconMac} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")

    target_sources(vym PRIVATE ${vymDemos})
    set_source_files_properties(${vymDemos} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources/demos"
    )
    target_sources(vym PRIVATE ${vymScripts})
    set_source_files_properties(${vymScripts} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources/demos/scripts"
    )

    target_sources(vym PRIVATE ${vymMacros})
    set_source_files_properties(${vymMacros} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources/macros"
    )
    #add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    #    COMMAND codesign -f -s - "${CMAKE_CURRENT_SOURC_DIR}/${PROJECT_NAME}.app"
    #    COMMENT "Signing the .app bundle"
    #)

endif()

add_custom_target(make-translations-directory ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/translations)
add_dependencies(vym make-translations-directory)

install(DIRECTORY demos DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${VYM_SHARE_SUBDIR})
install(DIRECTORY doc DESTINATION ${CMAKE_INSTALL_DOCDIR} FILES_MATCHING PATTERN "*.pdf")
install(FILES doc/vym.1.gz DESTINATION ${CMAKE_INSTALL_MANDIR})
install(FILES README.md LICENSE.txt DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(DIRECTORY exports flags icons macros ${CMAKE_BINARY_DIR}/translations scripts styles DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${VYM_SHARE_SUBDIR})

if(UNIX)
    install(FILES icons/vym.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/apps)
    # Disabling PR #109 for now:
    # file(CREATE_LINK ../apps/vym.png application-x-vym.png SYMBOLIC)
    # install(FILES ${CMAKE_BINARY_DIR}/application-x-vym.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/mimetypes)
    install(FILES config/vym.xml DESTINATION ${CMAKE_INSTALL_PREFIX}/share/mime/packages)
    install(FILES config/vym.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
endif()
