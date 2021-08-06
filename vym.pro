TARGET	    = vym
TEMPLATE    = app
LANGUAGE    = C++

CONFIG	+= qt warn_on x86_64 

QMAKE_MAC_SDK = macosx10.10

QT += network 
QT += xml 
QT += script 
QT += svg 
QT += printsupport
QT += widgets

#  include(tmp/modeltest/modeltest.pri)

RESOURCES = vym.qrc

unix:!macx:isEmpty(NO_DBUS) {
    message("Compiling with DBUS")
    DEFINES += VYM_DBUS
    QT      += dbus 
    HEADERS += src/adaptormodel.h src/adaptorvym.h 
    SOURCES += src/adaptormodel.cpp src/adaptorvym.cpp 
}

win32 {
    message("Compiling with win32")
    HEADERS += src/mkdtemp.h
    SOURCES += src/mkdtemp.cpp
    RC_FILE = vym.rc
    # Manifest embedding was suggested by Qt docs somewhere...
    win32: CONFIG += embed_manifest_exe

    # Without this, M_PI, and M_PI_2 won`t be defined.
    win32:DEFINES *= _USE_MATH_DEFINES

    QT_QPA_PLATFORM_PLUGIN_PATH=%QTDIR%\plugins\platforms\
}
macx {
    QMAK_MAC_SDK = macosx10.10
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.10
}

TRANSLATIONS += lang/vym.de_DE.ts
TRANSLATIONS += lang/vym.el.ts
TRANSLATIONS += lang/vym.en.ts
TRANSLATIONS += lang/vym.es.ts
TRANSLATIONS += lang/vym.fr.ts
TRANSLATIONS += lang/vym.hr_HR.ts
TRANSLATIONS += lang/vym.ia.ts
TRANSLATIONS += lang/vym.it.ts
TRANSLATIONS += lang/vym.ja.ts
TRANSLATIONS += lang/vym.pt_BR.ts
TRANSLATIONS += lang/vym.ru.ts
TRANSLATIONS += lang/vym.sv.ts
TRANSLATIONS += lang/vym.zh_CN.ts
TRANSLATIONS += lang/vym.zh_TW.ts
TRANSLATIONS += lang/vym.cs_CZ.ts

ICON =icons/vym.icns


HEADERS	+= \
    src/aboutdialog.h \
    src/animpoint.h \
    src/arrowobj.h \
    src/attribute.h \
    src/attributeitem.h \
#    src/attributedelegate.h\
#    src/attributedialog.h \
#    src/attributewidget.h \
    src/branchitem.h \
    src/branchobj.h \
    src/branchpropeditor.h\
    src/codeeditor.h \
    src/command.h \
    src/container.h \
    src/confluence-agent.h \
    src/credentials.h \
    src/dockeditor.h \
    src/download-agent.h \
    src/editxlinkdialog.h \
    src/exportoofiledialog.h \
    src/export-html-dialog.h\
    src/export-confluence-dialog.h\
    src/export-ao.h \
    src/export-ascii.h \
    src/export-base.h \
    src/export-csv.h \
    src/export-confluence.h \
    src/export-firefox.h \
    src/export-html.h \
    src/export-impress.h \
    src/export-latex.h \
    src/export-markdown.h \
    src/export-orgmode.h \
    src/export-taskjuggler.h \
    src/extrainfodialog.h \
    src/file.h \
    src/findwidget.h \
    src/findresultwidget.h \
    src/findresultitem.h \
    src/findresultmodel.h \
    src/flag.h \
    src/flagobj.h \
    src/flagrowobj.h \
    src/flagrow.h \
    src/flagrowmaster.h \
    src/floatimageobj.h \
    src/floatobj.h \
    src/frameobj.h \
    src/geometry.h \
    src/heading.h \
    src/headingeditor.h \
    src/headingobj.h \
    src/highlighter.h \
    src/historywindow.h \
    src/imageitem.h \
    src/imageobj.h \
    src/imports.h \
    src/jira-agent.h \
    src/lineeditdialog.h \
    src/linkablemapobj.h \
    src/lockedfiledialog.h \
    src/macros.h \
    src/mainwindow.h \
    src/mapeditor.h \
    src/mapitem.h \
    src/mapobj.h \
    src/misc.h \
    src/mysortfilterproxymodel.h \
    src/noteeditor.h \
    src/options.h \
    src/ornamentedobj.h \
    src/scripteditor.h\
    src/scripting.h \
    src/scriptoutput.h \
    src/settings.h \
    src/shortcuts.h\
    src/showtextdialog.h\
    src/slidecontrolwidget.h\
    src/slideeditor.h\
    src/slideitem.h\
    src/slidemodel.h\
    src/task.h\
    src/taskeditor.h\
    src/taskfiltermodel.h \
    src/taskmodel.h\
    src/treedelegate.h \
    src/treeeditor.h \
    src/treeitem.h \
    src/treemodel.h \
    src/texteditor.h \
    src/userdialog.h \
    src/version.h \
    src/vymlock.h \
    src/vymmodel.h \
    src/vymmodelwrapper.h \
    src/vymnote.h \
    src/vymprocess.h \
    src/vymtext.h \
    src/vymview.h \
    src/winter.h \
    src/warningdialog.h \
    src/xlink.h \
    src/xlinkitem.h \
    src/xlinkobj.h \
    src/xml-base.h \
    src/xml-vym.h \
    src/xml-freemind.h \
    src/xmlobj.h\
    src/xsltproc.h \
    src/zip-settings-dialog.h

SOURCES	+= \
    src/aboutdialog.cpp \
    src/animpoint.cpp \
    src/arrowobj.cpp \
    src/attribute.cpp \
    src/attributeitem.cpp \
#    src/attributedelegate.cpp \
#    src/attributedialog.cpp \
#    src/attributewidget.cpp \
    src/branchitem.cpp \
    src/branchobj.cpp \
    src/branchpropeditor.cpp \
    src/codeeditor.cpp \
    src/command.cpp \
    src/confluence-agent.cpp \
    src/container.cpp \
    src/credentials.cpp \
    src/dockeditor.cpp \
    src/download-agent.cpp \
    src/editxlinkdialog.cpp \
    src/export-html-dialog.cpp \
    src/export-confluence-dialog.cpp \
    src/exportoofiledialog.cpp \
    src/export-ao.cpp \
    src/export-ascii.cpp \
    src/export-base.cpp \
    src/export-confluence.cpp \
    src/export-csv.cpp \
    src/export-firefox.cpp \
    src/export-html.cpp \
    src/export-impress.cpp \
    src/export-latex.cpp \
    src/export-markdown.cpp \
    src/export-orgmode.cpp \
    src/export-taskjuggler.cpp \
    src/extrainfodialog.cpp \
    src/file.cpp \
    src/findwidget.cpp \
    src/findresultwidget.cpp \
    src/findresultitem.cpp \
    src/findresultmodel.cpp \
    src/flag.cpp \
    src/flagobj.cpp \
    src/flagrow.cpp \
    src/flagrowmaster.cpp \
    src/flagrowobj.cpp \
    src/floatimageobj.cpp \
    src/floatobj.cpp \
    src/frameobj.cpp \
    src/geometry.cpp \
    src/heading.cpp \
    src/headingeditor.cpp \
    src/headingobj.cpp \
    src/highlighter.cpp \
    src/historywindow.cpp \
    src/imageitem.cpp \
    src/imageobj.cpp \
    src/imports.cpp \
    src/jira-agent.cpp \
    src/lineeditdialog.cpp \
    src/linkablemapobj.cpp \
    src/lockedfiledialog.cpp \
    src/macros.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/mapeditor.cpp \
    src/mapitem.cpp \
    src/mapobj.cpp \
    src/misc.cpp \
    src/mysortfilterproxymodel.cpp \
    src/noteeditor.cpp \
    src/options.cpp \
    src/ornamentedobj.cpp \
    src/scripteditor.cpp \
    src/scripting.cpp \
    src/scriptoutput.cpp \
    src/settings.cpp \
    src/shortcuts.cpp\
    src/showtextdialog.cpp \
    src/slidecontrolwidget.cpp \
    src/slideeditor.cpp \
    src/slideitem.cpp \
    src/slidemodel.cpp \
    src/task.cpp \
    src/taskeditor.cpp \
    src/taskfiltermodel.cpp \
    src/taskmodel.cpp \
    src/texteditor.cpp \
    src/treedelegate.cpp \
    src/treeeditor.cpp \
    src/treeitem.cpp \
    src/treemodel.cpp \
    src/userdialog.cpp \
    src/version.cpp \
    src/vymlock.cpp \
    src/vymmodel.cpp \
    src/vymmodelwrapper.cpp \
    src/vymnote.cpp \
    src/vymprocess.cpp \
    src/vymtext.cpp \
    src/vymview.cpp \
    src/warningdialog.cpp \
    src/winter.cpp \
    src//xlink.cpp \
    src/xlinkitem.cpp \
    src/xlinkobj.cpp \
    src/xml-base.cpp \
    src/xml-vym.cpp \
    src/xml-freemind.cpp \
    src/xmlobj.cpp \
    src/xsltproc.cpp \
    src/zip-settings-dialog.cpp

FORMS = \
    forms/attributewidget.ui \
    forms/branchpropeditor.ui \
    forms/credentials.ui \
    forms/export-html-dialog.ui \
    forms/export-confluence-dialog.ui \
    forms/extrainfodialog.ui \
    forms/editxlinkdialog.ui \
    forms/historywindow.ui \
    forms/lineeditdialog.ui \
    forms/lockedfiledialog.ui \
    forms/scripteditor.ui \
    forms/showtextdialog.ui \
    forms/userdialog.ui \
    forms/warningdialog.ui \
    forms/zip-settings-dialog.ui

isEmpty( PREFIX ) {
    PREFIX = /usr/share
    count( INSTALLDIR, 1 ) {
	PREFIX = $${INSTALLDIR}
    }
}
isEmpty( BINDIR ) {
    BINDIR = /usr/bin
}
isEmpty( DATADIR ) {
    DATADIR = $${PREFIX}/vym
}

target.path = $${BINDIR}
INSTALLS += target

DEFINES += "VYMBASEDIR='\"$${DATADIR}\"'"

message( "PREFIX Dir: $$PREFIX" )
message( "DATADIR: $$DATADIR" )
message( "DEFINES: $$DEFINES" )

language.files = lang/*.qm
language.path = $${DATADIR}/lang
INSTALLS += language

support.files = demos \
    exports/  \
    flags/ \
    icons/ \
    macros/ \
    scripts/ \
    styles/ 

support.path = $${DATADIR}
INSTALLS += support 

# doc.files = doc/vym.pdf
# doc.files += LICENSE.txt
# doc.path = $${DOCDIR}
# INSTALLS += doc
# DEFINES += VYM_DOCDIR=\\\"$${DOCDIR}\\\"
