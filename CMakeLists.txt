cmake_minimum_required(VERSION 3.12)

project(ViewYourMind)

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
 endif()

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

set(VymSources
    src/aboutdialog.cpp
    src/animpoint.cpp
    src/arrowobj.cpp
    src/attribute.cpp
    src/attributeitem.cpp
#    src/attributedelegate.cpp
#    src/attributedialog.cpp
#    src/attributewidget.cpp
    src/branchitem.cpp
    src/branchobj.cpp
    src/branchpropeditor.cpp
    src/codeeditor.cpp
    src/command.cpp
    src/confluence-agent.cpp
    src/confluence-userdialog.cpp
    src/confluence-user.cpp
    src/credentials.cpp
    src/dockeditor.cpp
    src/download-agent.cpp
    src/editxlinkdialog.cpp
    src/export-html-dialog.cpp
    src/export-confluence-dialog.cpp
    src/exportoofiledialog.cpp
    src/export-ao.cpp
    src/export-ascii.cpp
    src/export-base.cpp
    src/export-confluence.cpp
    src/export-csv.cpp
    src/export-firefox.cpp
    src/export-html.cpp
    src/export-impress.cpp
    src/export-latex.cpp
    src/export-markdown.cpp
    src/export-orgmode.cpp
    src/export-taskjuggler.cpp
    src/extrainfodialog.cpp
    src/file.cpp
    src/findwidget.cpp
    src/findresultwidget.cpp
    src/findresultitem.cpp
    src/findresultmodel.cpp
    src/flag.cpp
    src/flagobj.cpp
    src/flagrow.cpp
    src/flagrowmaster.cpp
    src/flagrowobj.cpp
    src/floatimageobj.cpp
    src/floatobj.cpp
    src/frameobj.cpp
    src/geometry.cpp
    src/heading.cpp
    src/headingeditor.cpp
    src/headingobj.cpp
    src/highlighter.cpp
    src/historywindow.cpp
    src/imageitem.cpp
    src/imageobj.cpp
    src/imports.cpp
    src/jira-agent.cpp
    src/jira-settings-dialog.cpp
    src/lineeditdialog.cpp
    src/linkablemapobj.cpp
    src/lockedfiledialog.cpp
    src/macros.cpp
    src/main.cpp
    src/mainwindow.cpp
    src/mapeditor.cpp
    src/mapitem.cpp
    src/mapobj.cpp
    src/misc.cpp
    src/mysortfilterproxymodel.cpp
    src/noteeditor.cpp
    src/options.cpp
    src/ornamentedobj.cpp
    src/scripteditor.cpp
    src/scripting.cpp
    src/scriptoutput.cpp
    src/settings.cpp
    src/shortcuts.cpp
    src/showtextdialog.cpp
    src/slidecontrolwidget.cpp
    src/slideeditor.cpp
    src/slideitem.cpp
    src/slidemodel.cpp
    src/task.cpp
    src/taskeditor.cpp
    src/taskfiltermodel.cpp
    src/taskmodel.cpp
    src/texteditor.cpp
    src/treedelegate.cpp
    src/treeeditor.cpp
    src/treeitem.cpp
    src/treemodel.cpp
    src/version.cpp
    src/vymlock.cpp
    src/vymmodel.cpp
    src/vymmodelwrapper.cpp
    src/vymnote.cpp
    src/vymprocess.cpp
    src/vymtext.cpp
    src/vymview.cpp
    src/warningdialog.cpp
    src/winter.cpp
    src//xlink.cpp
    src/xlinkitem.cpp
    src/xlinkobj.cpp
    src/xml-base.cpp
    src/xml-vym.cpp
    src/xml-freemind.cpp
    src/xmlobj.cpp
    src/xsltproc.cpp
    src/zip-settings-dialog.cpp
)

if(WIN32)
    list(APPEND VymSources
        src/mkdtemp.cpp
        vym.rc
    )
    ADD_COMPILE_DEFINITIONS(_USE_MATH_DEFINES)
endif()

if(DBus1_FOUND)
    list(APPEND VymSources
        src/adaptormodel.cpp
        src/adaptorvym.cpp
    )
    ADD_COMPILE_DEFINITIONS(VYM_DBUS)
endif()

set(VymTranslationSources
    lang/vym.de_DE.ts
    lang/vym.el.ts
    lang/vym.en.ts
    lang/vym.es.ts
    lang/vym.fr.ts
    lang/vym.hr_HR.ts
    lang/vym.ia.ts
    lang/vym.it.ts
    lang/vym.ja.ts
    lang/vym.pt_BR.ts
    lang/vym.ru.ts
    lang/vym.sv.ts
    lang/vym.zh_CN.ts
    lang/vym.zh_TW.ts
    lang/vym.cs_CZ.ts
)

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

install(DIRECTORY demos TYPE DATA)
install(DIRECTORY doc TYPE DOC FILES_MATCHING PATTERN "*.pdf")
install(FILES doc/vym.1.gz TYPE MAN)
install(FILES README.md LICENSE.txt TYPE DOC)
install(DIRECTORY exports flags icons macros lang scripts styles TYPE DATA)

if(UNIX)
    install(FILES icons/vym.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/apps)
endif()
