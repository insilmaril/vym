TARGET	    = vym
TEMPLATE    = app
LANGUAGE    = C++

CONFIG	+= qt warn_on x86_64 

QMAKE_MAC_SDK = macosx10.7

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
    HEADERS += adaptormodel.h adaptorvym.h 
    SOURCES += adaptormodel.cpp adaptorvym.cpp 
}

win32 {
    HEADERS += mkdtemp.h
    SOURCES += mkdtemp.cpp
    RC_FILE = vym.rc
    # Manifest embedding was suggested by Qt docs somewhere...
    win32: CONFIG += embed_manifest_exe

    # Without this, M_PI, and M_PI_2 won`t be defined.
    win32:DEFINES *= _USE_MATH_DEFINES
}

TRANSLATIONS += lang/vym_de_DE.ts
TRANSLATIONS += lang/vym_en.ts
TRANSLATIONS += lang/vym_es.ts
TRANSLATIONS += lang/vym_fr.ts
TRANSLATIONS += lang/vym_ia.ts
TRANSLATIONS += lang/vym_it.ts
TRANSLATIONS += lang/vym_ja.ts
TRANSLATIONS += lang/vym_pt_BR.ts
TRANSLATIONS += lang/vym_ru.ts
TRANSLATIONS += lang/vym_sv.ts
TRANSLATIONS += lang/vym_zh_CN.ts
TRANSLATIONS += lang/vym_zh_TW.ts
TRANSLATIONS += lang/vym_cs_CZ.ts

ICON =icons/vym.icns


HEADERS	+= \
    aboutdialog.h \
    taskfiltermodel.h \
    animpoint.h \
    arrowobj.h \
    attribute.h \
    attributeitem.h \
#   attributedelegate.h\
#   attributedialog.h \
#   attributewidget.h \
    branchitem.h \
    branchobj.h \
    branchpropeditor.h\
    bugagent.h \
    command.h \
    dockeditor.h \
    downloadagent.h \
    editxlinkdialog.h \
    exportoofiledialog.h \
    exporthtmldialog.h\
    exports.h \
    extrainfodialog.h \
    file.h \
    findwidget.h \
    findresultwidget.h \
    findresultitem.h \
    findresultmodel.h \
    flag.h \
    flagobj.h \
    flagrowobj.h \
    flagrow.h \
    floatimageobj.h \
    floatobj.h \
    frameobj.h \
    geometry.h \
    heading.h \
    headingeditor.h \
    headingobj.h \
    highlighter.h \
    historywindow.h \
    imageitem.h \
    imageobj.h \
    imports.h \
    lineeditdialog.h \
    linkablemapobj.h \
    macros.h \
    mainwindow.h \
    mapeditor.h \
    mapitem.h \
    mapobj.h \
    misc.h \
    mysortfilterproxymodel.h \
    noteeditor.h \
    options.h \
    ornamentedobj.h \
    parser.h \
    scripteditor.h\
    scripting.h \
    scriptoutput.h \
    settings.h \
    shortcuts.h\
    showtextdialog.h\
    slidecontrolwidget.h\
    slideeditor.h\
    slideitem.h\
    slidemodel.h\
    task.h\
    taskeditor.h\
    taskmodel.h\
    treedelegate.h \
    treeeditor.h \
    treeitem.h \
    treemodel.h \
    texteditor.h \
    version.h \
    vymmodel.h \
    vymnote.h \
    vymtext.h \
    vymview.h \
    winter.h \
    warningdialog.h \
    xlink.h \
    xlinkitem.h \
    xlinkobj.h \
    xml-base.h \
    xml-vym.h \
    xml-freemind.h \
    xmlobj.h\
    xsltproc.h \ 
    vymprocess.h \

SOURCES	+= \
    aboutdialog.cpp \
    taskfiltermodel.cpp \
    animpoint.cpp \
    arrowobj.cpp \
    attribute.cpp \
    attributeitem.cpp \
#   attributedelegate.cpp \
#   attributedialog.cpp \
#   attributewidget.cpp \
    branchitem.cpp \
    branchobj.cpp \
    branchpropeditor.cpp \
    bugagent.cpp \
    command.cpp \
    dockeditor.cpp \
    downloadagent.cpp \
    editxlinkdialog.cpp \
    exportoofiledialog.cpp \
    exports.cpp \
    exporthtmldialog.cpp \
    extrainfodialog.cpp \
    file.cpp \
    findwidget.cpp \
    findresultwidget.cpp \
    findresultitem.cpp \
    findresultmodel.cpp \
    flag.cpp \
    flagobj.cpp \
    flagrow.cpp \
    flagrowobj.cpp \
    floatimageobj.cpp \
    floatobj.cpp \
    frameobj.cpp \
    geometry.cpp \
    heading.cpp \
    headingeditor.cpp \
    headingobj.cpp \
    highlighter.cpp \
    historywindow.cpp \
    imageitem.cpp \
    imageobj.cpp \
    imports.cpp \
    lineeditdialog.cpp \
    linkablemapobj.cpp \
    macros.cpp \
    main.cpp \
    mainwindow.cpp \
    mapeditor.cpp \
    mapitem.cpp \
    mapobj.cpp \
    misc.cpp \
    mysortfilterproxymodel.cpp \
    noteeditor.cpp \
    options.cpp \
    ornamentedobj.cpp \
    parser.cpp \
    scripteditor.cpp \
    scriptoutput.cpp \
    scripting.cpp \
    settings.cpp \
    shortcuts.cpp\
    showtextdialog.cpp \
    slidecontrolwidget.cpp \
    slideeditor.cpp \
    slideitem.cpp \
    slidemodel.cpp \
    task.cpp \
    taskeditor.cpp \
    taskmodel.cpp \
    texteditor.cpp \
    treedelegate.cpp \
    treeeditor.cpp \
    treeitem.cpp \
    treemodel.cpp \
    version.cpp \
    vymmodel.cpp \
    vymnote.cpp \
    vymtext.cpp \
    vymview.cpp \
    warningdialog.cpp \
    winter.cpp \
    xlink.cpp \
    xlinkitem.cpp \
    xlinkobj.cpp \
    xml-base.cpp \
    xml-vym.cpp \
    xml-freemind.cpp \
    xmlobj.cpp \
    xsltproc.cpp \
    vymprocess.cpp

FORMS = \
    attributewidget.ui \
    branchpropeditor.ui \
    exporthtmldialog.ui \
    extrainfodialog.ui \
    editxlinkdialog.ui \
    historywindow.ui \
    lineeditdialog.ui \
    scripteditor.ui \
    showtextdialog.ui \
    warningdialog.ui 

isEmpty( PREFIX ) {
    PREFIX = /usr/local
    count( INSTALLDIR, 1 ) {
	PREFIX = $${INSTALLDIR}
	message( "Please use PREFIX instead of INSTALLDIR" )
    }
}
isEmpty( BINDIR ) {
    BINDIR = $${PREFIX}/bin
}
isEmpty( DATADIR ) {
    DATADIR = $${PREFIX}
}
isEmpty( DOCDIR ) {
    DOCDIR = $${DATADIR}/doc/packages/vym
}

message( "Installation directory" )
message( $$PREFIX )


target.path = $${BINDIR}
INSTALLS += target

support.files = styles/ scripts/ icons/ flags/ lang/ macros/ exports/ demos/
support.path = $${DATADIR}/vym
INSTALLS += support 

doc.files = doc/vym.pdf
doc.path = $${DOCDIR}
INSTALLS += doc
DEFINES += VYM_DOCDIR=\\\"$${DOCDIR}\\\"
