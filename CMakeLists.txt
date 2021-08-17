CMAKE_MINIMUM_REQUIRED(VERSION 3.12)

PROJECT(ViewYourMind)

SET(QtComponents
    LinguistTools
    Network
    PrintSupport
    Script
    Svg
    Widgets
    Xml
)

SET(QtLibraries
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
    FIND_PACKAGE(DBus1)

    if(DBus1_FOUND)
        LIST(APPEND QtComponents DBus)
        LIST(APPEND QtLibraries Qt5::DBus)
    endif()
endif()

FIND_PACKAGE(Qt5 COMPONENTS ${QtComponents} REQUIRED)
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_AUTOUIC ON)
LIST(APPEND CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_SOURCE_DIR}/forms")

GET_TARGET_PROPERTY(QtLibraryType Qt5::Widgets TYPE)

IF(QtLibraryType STREQUAL STATIC_LIBRARY)
    MESSAGE(STATUS "Static Qt linkage")
    LIST(APPEND QtLibraries Qt5::QSvgPlugin)
ENDIF()

INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}
)

SET(VymSources
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
    src/confluence-userdialog.cpp
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

IF(WIN32)
    LIST(APPEND VymSources
        src/mkdtemp.cpp
        vym.rc
    )
    ADD_COMPILE_DEFINITIONS(_USE_MATH_DEFINES)
ENDIF()

IF(DBus1_FOUND)
    LIST(APPEND VymSources
        src/adaptormodel.cpp
        src/adaptorvym.cpp
    )
    ADD_COMPILE_DEFINITIONS(VYM_DBUS)
ENDIF()

SET(VymTranslationSources
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

FILE(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/lang")
SET_SOURCE_FILES_PROPERTIES(${VymTranslationSources} PROPERTIES OUTPUT_LOCATION lang)
QT5_ADD_TRANSLATION(VymTranslations ${VymTranslationSources})

ADD_COMPILE_DEFINITIONS(VYMBASEDIR="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}")

ADD_EXECUTABLE(vym ${VymSources} vym.qrc ${VymTranslations})
TARGET_LINK_LIBRARIES(vym ${QtLibraries})

IF(WIN32)
    SET_TARGET_PROPERTIES(vym PROPERTIES WIN32_EXECUTABLE TRUE)
    INSTALL(TARGETS vym DESTINATION "${CMAKE_INSTALL_PREFIX}")
ELSE()
    INSTALL(TARGETS vym DESTINATION bin)
ENDIF()

INSTALL(DIRECTORY demos TYPE DATA)
INSTALL(DIRECTORY doc TYPE DOC FILES_MATCHING PATTERN "*.pdf")
INSTALL(FILES doc/vym.1.gz TYPE MAN)
INSTALL(FILES README.md LICENSE.txt TYPE DOC)
INSTALL(DIRECTORY exports flags icons macros lang scripts styles TYPE DATA)

if(UNIX)
    INSTALL(FILES icons/vym.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/apps)
endif()
