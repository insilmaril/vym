TARGET	    = vym
TEMPLATE    = app
LANGUAGE    = C++

CONFIG	+= qt warn_on debug x86 ppc qdbus

QT += qt3support network xml

# Only needed with Qt < 4.6
# include (/data/qtanimationframework-2.3-opensource/src/qtanimationframework.pri)

TRANSLATIONS += lang/vym_de.ts
TRANSLATIONS += lang/vym_en.ts
TRANSLATIONS += lang/vym_es.ts
TRANSLATIONS += lang/vym_fr.ts
TRANSLATIONS += lang/vym_it.ts
TRANSLATIONS += lang/vym_pt_BR.ts
TRANSLATIONS += lang/vym_ru.ts
TRANSLATIONS += lang/vym_sv.ts
TRANSLATIONS += lang/vym_zh_CN.ts
TRANSLATIONS += lang/vym_zh_TW.ts
TRANSLATIONS += lang/vym_cs_CZ.ts

ICON =icons/vym.icns


HEADERS	+= \
    aboutdialog.h \
    adaptormodel.h \
    animpoint.h \
    attribute.h \
    attributeitem.h \
#   attributedelegate.h\
#   attributedialog.h \
#   attributewidget.h \
    branchitem.h \
    branchobj.h \
    branchpropwindow.h\
    bugagent.h \
    editxlinkdialog.h \
    exportoofiledialog.h \
    exporthtmldialog.h\
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
    headingeditor.h \
    headingobj.h \
    highlighter.h \
    historywindow.h \
    imageitem.h \
    imageobj.h \
    imports.h \
    linkablemapobj.h \
    mainwindow.h \
    mapeditor.h \
    mapitem.h \
    mapobj.h \
    misc.h \
    mysortfilterproxymodel.h \
    noteeditor.h \
    noteobj.h \
    options.h \
    ornamentedobj.h \
    parser.h \
    process.h \
    settings.h \
    shortcuts.h\
    showtextdialog.h\
    simplescripteditor.h\
    treedelegate.h \
    treeeditor.h \
    treeitem.h \
    treemodel.h \
    texteditor.h \
    version.h \
    vymmodel.h \
    vymview.h \
    warningdialog.h \
    xlink.h \
    xlinkitem.h \
    xlinkobj.h \
    xml-base.h \
    xml-vym.h \
    xml-freemind.h \
    xmlobj.h\
    xsltproc.h 

SOURCES	+= \
    aboutdialog.cpp \
    adaptormodel.cpp \
    animpoint.cpp \
    attribute.cpp \
    attributeitem.cpp \
#   attributedelegate.cpp \
#   attributedialog.cpp \
#   attributewidget.cpp \
    branchitem.cpp \
    branchobj.cpp \
    branchpropwindow.cpp \
    bugagent.cpp \
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
    headingeditor.cpp \
    headingobj.cpp \
    highlighter.cpp \
    historywindow.cpp \
    imageitem.cpp \
    imageobj.cpp \
    imports.cpp \
    linkablemapobj.cpp \
    main.cpp \
    mainwindow.cpp \
    mapeditor.cpp \
    mapitem.cpp \
    mapobj.cpp \
    misc.cpp \
    mysortfilterproxymodel.cpp \
    noteeditor.cpp \
    noteobj.cpp \
    options.cpp \
    ornamentedobj.cpp \
    parser.cpp \
    process.cpp \
    settings.cpp \
    shortcuts.cpp\
    showtextdialog.cpp \
    simplescripteditor.cpp \
    texteditor.cpp \
    treedelegate.cpp \
    treeeditor.cpp \
    treeitem.cpp \
    treemodel.cpp \
    version.cpp \
    vymmodel.cpp \
    vymview.cpp \
    warningdialog.cpp \
    xlink.cpp \
    xlinkitem.cpp \
    xlinkobj.cpp \
    xml-base.cpp \
    xml-vym.cpp \
    xml-freemind.cpp \
    xmlobj.cpp \
    xsltproc.cpp 

FORMS = \
    attributewidget.ui \
    branchpropwindow.ui \
    exporthtmldialog.ui \
    extrainfodialog.ui \
    editxlinkdialog.ui \
    historywindow.ui \
    simplescripteditor.ui \
    showtextdialog.ui \
    warningdialog.ui

win32 {
    HEADERS += mkdtemp.h
    SOURCES += mkdtemp.cpp
    RC_FILE = vym.rc
    # Manifest embedding was suggested by Qt docs somewhere...
    win32: CONFIG += embed_manifest_exe

    # Without this, M_PI, and M_PI_2 won`t be defined.
    win32:DEFINES *= _USE_MATH_DEFINES
}

#The following lines were inserted by qt3to4

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
    DATADIR = $${PREFIX}/share
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

#include(test/modeltest/modeltest.pri)
