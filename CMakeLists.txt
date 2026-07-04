cmake_minimum_required(VERSION 3.16)

project(vym LANGUAGES CXX)

# See also https://doc.qt.io/qt-6.5/cmake-get-started.html

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    message(STATUS "vym: using UNIX in cmake")	# FIXME-0 still required?
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

# Should no longer needed with qt_standard_project_setup() below...
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# On Linux DBUS is used for IPC with vym
# On openSUSE install it using:  zypper install dbus-1-devel

if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    find_package(DBus1)

    if(DBus1_FOUND)
        message(STATUS "vym: Deteced Linux and DBUS1, adding DBUS interfaces")

        LIST(APPEND QtComponents DBus)
        LIST(APPEND QtLibraries Qt6::DBus)
    endif()
endif()


# https://doc.qt.io/qt-6/qt-standard-project-setup.html
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 COMPONENTS ${QtComponents} REQUIRED)

find_package (Git)
if (GIT_FOUND)
      message("git found: ${GIT_EXECUTABLE} in version ${GIT_VERSION_STRING}")

    execute_process(
        COMMAND ${GIT_EXECUTABLE} -C ${CMAKE_SOURCE_DIR} branch --show-current
        RESULT_VARIABLE RESULT
        OUTPUT_VARIABLE GIT_BRANCH)

    execute_process(
        COMMAND ${GIT_EXECUTABLE} -C ${CMAKE_SOURCE_DIR} rev-parse --short HEAD
        RESULT_VARIABLE RESULT
        OUTPUT_VARIABLE GIT_COMMIT_HASH)

    string (STRIP ${GIT_BRANCH} GIT_BRANCH)
    string (STRIP ${GIT_COMMIT_HASH} GIT_COMMIT_HASH)

    message(STATUS "vym: git branch: ${OUTPUT} - commit: ${GIT_COMMIT_HASH}")

    add_compile_definitions(
        GIT_BRANCH="${GIT_BRANCH}"
        GIT_COMMIT_HASH="${GIT_COMMIT_HASH}")

    configure_file(${CMAKE_SOURCE_DIR}/src/git.h.in ${CMAKE_SOURCE_DIR}/src/git.h)
else    
    add_compile_definitions(
        GIT_BRANCH="n.a."
        GIT_COMMIT_HASH="n.a.")
endif (GIT_FOUND)

# Translations
#
# To update the translation files based on source code changes
# add the targest update_translations and release_translations
#
# See also (for Qt 6):
# https://doc.qt.io/qt-6/qtlinguist-cmake-qt-add-lrelease.html
# https://doc.qt.io/qt-6/qtlinguist-cmake-qt-add-translations.html

file(GLOB ts_files RELATIVE ${CMAKE_SOURCE_DIR} "lang/*.ts")

message(STATUS "vym: Creating ${CMAKE_BINARY_DIR}/translations")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/translations")

set_source_files_properties(${ts_files} PROPERTIES OUTPUT_LOCATION ${CMAKE_BINARY_DIR}/translations)


#qt_add_lrelease(target TS_FILES ${ts_files} QM_FILES_OUTPUT_VARIABLE ${CMAKE_BINARY_DIR}/translations) 

message(STATUS "vym: Found Qt version: ${Qt6_VERSION}")

if(Qt6_VERSION VERSION_GREATER_EQUAL 6.7)
    qt_standard_project_setup( I18N_TRANSLATED_LANGUAGES
        el cs_CZ de en es fr hr_HR ia it ja pt_BR ru sv zh_CN zh_TW
    ) # FIXME add missing translations

    qt_add_translations( vym
        TS_FILE_DIR ${CMAKE_SOURCE_DIR}/lang    # TS_OUTPUT_DIRECTORY in Qt 6.9
        RESOURCE_PREFIX ${CMAKE_BINARY_DIR}/translations
        #QM_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/translations  # Only in Qt 6.9
    )
    # qm files will be created
    # - for APPLE in # ${CMAKE_BINARY_DIR}/translations
else()
    # Fallback without translations for the time being
    qt_standard_project_setup()
endif()

include(GNUInstallDirs)

list(APPEND CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_SOURCE_DIR}/forms")

get_target_property(QtLibraryType Qt6::Widgets TYPE)

if(QtLibraryType STREQUAL STATIC_LIBRARY)
    message(STATUS "vym: Static Qt linkage")
    list(APPEND QtLibraries Qt6::QSvgPlugin)
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}
)

# Source files
file(GLOB VymSources RELATIVE ${CMAKE_SOURCE_DIR} "src/*.cpp")

if(WIN32) # FIXME still needed?
    list(APPEND VymSources
        #src/mkdtemp.cpp
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


add_compile_definitions(CMAKE_SOURCE_DIR)

message(STATUS "vym: ts_files: ${ts_files}")
message(STATUS "vym: qm_files: ${qm_files}")


# On Unix-like systems, place app data under a dedicated subdir
# so runtime looks in .../share/vym for resources.
set(VYM_SHARE_SUBDIR "vym")
ADD_COMPILE_DEFINITIONS(VYMBASEDIR="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/${VYM_SHARE_SUBDIR}")


# FIXME qm_files needed?
#add_executable(vym ${VymSources} vym.qrc ${qm_files})
qt_add_executable(vym MACOSX_BUNDLE ${VymSources} vym.qrc ${qm_files} ) # FIXME Icon missing?
                                                                # FIXME Move add_executable to end?
target_link_libraries(vym PRIVATE ${QtLibraries})

#FIXME shoud translations/ really be in  CMAKE_BINARY_DIR? Or better e.g. in Contents?
#
add_custom_target(make-translations-directory ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/translations)
add_dependencies(vym make-translations-directory)

if(WIN32)
    set_target_properties(vym PROPERTIES WIN32_EXECUTABLE TRUE)
    install(TARGETS vym DESTINATION "${CMAKE_INSTALL_PREFIX}")
else()
    install(TARGETS vym DESTINATION bin)
endif()

if(APPLE)
    message(STATUS "vym: Detected APPLE, preparing bundle:")

    set(MACOSX_BUNDLE_EXECUTABLE_NAME ${PROJECT_NAME})

    # Prepare Info.plist
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION "2.9.5xx" #"${PROJECT_VERSION}"   # FIXME Version missing^
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.insilmaril.vym"
        MACOSX_BUNDLE_ICON_FILE "icons/vym.icns"
        MACOSX_BUNDLE TRUE
    )

    # Root paths from sources to be copied to vym.app bundle (adjust if needed)
    set(DIRLIST demos doc exports flags icons macros styles)

    # Copy above directories from sources to bundle
    set(RESOURCE_DIR "${CMAKE_BINARY_DIR}/vym.app/Contents/Resources")
    foreach(CURRENT_DIR ${DIRLIST})
        set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/${CURRENT_DIR}")
        set(TARGET_DIR "${RESOURCE_DIR}/${CURRENT_DIR}")

        # --- 1. Add all subdirectories of SOURCE to the include path ---

        # Get all subdirectories recursively
        file(GLOB_RECURSE SOURCE_SUBDIRS LIST_DIRECTORIES true RELATIVE ${SOURCE_DIR} "${SOURCE_DIR}/*")

        set(INCLUDE_DIRS "")
        foreach(path ${SOURCE_SUBDIRS})
            if(IS_DIRECTORY "${SOURCE_DIR}/${path}")
                list(APPEND INCLUDE_DIRS "${SOURCE_DIR}/${path}")
            endif()
        endforeach()

        # Add SOURCE itself too
        list(APPEND INCLUDE_DIRS "${SOURCE_DIR}")

        # Add them to your target
        target_include_directories(vym PRIVATE ${INCLUDE_DIRS})

        # message(STATUS "CURRENT_DIR=${CURRENT_DIR}  includeDirs=${INCLUDE_DIRS}")

        # --- 2. Copy SOURCE_DIR -> TARGET_DIR at build time ---

        add_custom_command(
            TARGET vym POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${SOURCE_DIR}"
                    "${TARGET_DIR}"
            COMMENT "Copying SOURCE directory ${SOURCE_DIR} to TARGET directory ${TARGET_DIR}..."
        )
    endforeach()

    # Finally copy generated translations to bundle
    set(SOURCE_DIR "${CMAKE_BINARY_DIR}/translations")
    set(TARGET_DIR "${RESOURCE_DIR}/translations")
    add_custom_command(
        TARGET vym POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${SOURCE_DIR}" "${TARGET_DIR}"
        # COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --blue "vym: Copying ${SOURCE_DIR} to ${TARGET_DIR}..."
        COMMENT "Copying translations directory to TARGET directory ${TARGET_DIR}..."
    )
endif()

message(STATUS "vym: CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}")
message(STATUS "vym: CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}")
message(STATUS "vym: CMAKE_INSTALL_DATAROOTDIR=${CMAKE_INSTALL_DATAROOTDIR}")


install(DIRECTORY demos DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${VYM_SHARE_SUBDIR})
install(DIRECTORY doc DESTINATION ${CMAKE_INSTALL_DOCDIR} FILES_MATCHING PATTERN "*.pdf")
install(FILES doc/vym.1.gz DESTINATION ${CMAKE_INSTALL_MANDIR})
install(FILES README.md LICENSE.txt DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(DIRECTORY exports flags icons macros ${CMAKE_BINARY_DIR}/translations scripts styles DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${VYM_SHARE_SUBDIR})

if(UNIX)
    install(FILES icons/vym.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/apps)
    # Disabling PR #109 for now:    # FIXME
    # file(CREATE_LINK ../apps/vym.png application-x-vym.png SYMBOLIC)
    # install(FILES ${CMAKE_BINARY_DIR}/application-x-vym.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/mimetypes)
    install(FILES config/vym.xml DESTINATION ${CMAKE_INSTALL_PREFIX}/share/mime/packages)
    install(FILES config/vym.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
endif()
