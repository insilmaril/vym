cmake_minimum_required(VERSION 3.12)

project(ViewYourMind)

# Build optimized with debug info
set(CMAKE_BUILD_TYPE RelWithDebInfo)

set(QtComponents
    LinguistTools
    Network
    PrintSupport
    Script
    Svg
    Widgets
    Xml
)

set(QtLibraries
    Qt5::Network
    Qt5::PrintSupport
    Qt5::Script
    Qt5::Svg
    Qt5::Widgets
    Qt5::Xml
)

# On Linux DBUS is used for IPC with vym
# On openSUSE install it using:  zypper install dbus-1-devel

if (UNIX)
    find_package(DBus1)

    if(DBus1_FOUND)
        LIST(APPEND QtComponents DBus)
        LIST(APPEND QtLibraries Qt5::DBus)
    endif()
endif()

if(WIN32)
    set(OPENSSL_ROOT_DIR "C:/OpenSSL-Win64")
    find_package(OpenSSL REQUIRED)

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

include(GNUInstallDirs)

find_package(Qt5 COMPONENTS ${QtComponents} REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
list(APPEND CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_SOURCE_DIR}/forms")

get_target_property(QtLibraryType Qt5::Widgets TYPE)

if(QtLibraryType STREQUAL STATIC_LIBRARY)
    message(STATUS "Static Qt linkage")
    list(APPEND QtLibraries Qt5::QSvgPlugin)
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



if(DBus1_FOUND)
    list(APPEND VymSources src/dbus/adaptormodel.cpp src/dbus/adaptorvym.cpp)
    include_directories(${CMAKE_SOURCE_DIR}/src/dbus ${CMAKE_SOURCE_DIR}/src)
    ADD_COMPILE_DEFINITIONS(VYM_DBUS)
endif()

# Translations

file(GLOB_RECURSE VymTranslationSources RELATIVE ${CMAKE_SOURCE_DIR} "lang/*.ts")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/lang")
set_source_files_properties(${VymTranslationSources} PROPERTIES OUTPUT_LOCATION lang)
QT5_ADD_TRANSLATION(VymTranslations ${VymTranslationSources})

ADD_COMPILE_DEFINITIONS(VYMBASEDIR="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}")

add_executable(vym ${VymSources} vym.qrc ${VymTranslations})
target_link_libraries(vym ${QtLibraries})

if(WIN32)
    set_target_properties(vym PROPERTIES WIN32_EXECUTABLE TRUE)
    install(TARGETS vym DESTINATION "${CMAKE_INSTALL_PREFIX}")
else()
    install(TARGETS vym DESTINATION bin)
endif()

install(DIRECTORY demos DESTINATION ${CMAKE_INSTALL_DATAROOTDIR})
install(DIRECTORY doc DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(FILES doc/vym.1.gz DESTINATION ${CMAKE_INSTALL_MANDIR})
install(FILES README.md LICENSE.txt DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(DIRECTORY exports flags icons macros lang scripts styles DESTINATION ${CMAKE_INSTALL_DATAROOTDIR})

if(UNIX)
    install(FILES icons/vym.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/apps)
    install(FILES config/vym.xml DESTINATION ${CMAKE_INSTALL_PREFIX}/share/mime/packages)
    install(FILES config/vym.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
endif()
