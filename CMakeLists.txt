project( vym )
cmake_minimum_required( VERSION 2.6 )

if( NOT CMAKE_BUILD_TYPE )
	set( CMAKE_BUILD_TYPE Debug )
endif( NOT CMAKE_BUILD_TYPE )

if( CMAKE_COMPILER_IS_GNUCXX )
	add_definitions ( -Wall )
endif( CMAKE_COMPILER_IS_GNUCXX )

if( UNIX AND NOT APPLE )
    set( WITH_DBUS_DEFAULT ON )
else( UNIX AND NOT APPLE )
    set( WITH_DBUS_DEFAULT OFF )
endif( UNIX AND NOT APPLE )

option( WITH_DBUS "Build vym with dbus support" ${WITH_DBUS_DEFAULT} )

find_package ( Qt4 REQUIRED )
include ( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )

if( WIN32 )
	add_definitions( -DUNICODE -D_USE_MATH_DEFINES )
	if( MSVC )
		set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:wchar_t-" )
	endif( MSVC )
endif( WIN32 )

find_package( cppcheck )
if(CPPCHECK_FOUND)
	set(_cppcheck_args)
	list(APPEND _cppcheck_args ${CPPCHECK_INCLUDEPATH_ARG} ${CMAKE_SOURCE_DIR})
# 	list(APPEND _cppcheck_args ${CPPCHECK_INCLUDEPATH_ARG} ${QT_QTCORE_INCLUDE_DIR})
# 	list(APPEND _cppcheck_args ${CPPCHECK_INCLUDEPATH_ARG} ${QT_QTDBUS_INCLUDE_DIR})
# 	list(APPEND _cppcheck_args ${CPPCHECK_INCLUDEPATH_ARG} ${QT_QTGUI_INCLUDE_DIR})
# 	list(APPEND _cppcheck_args ${CPPCHECK_INCLUDEPATH_ARG} ${QT_QTNETWORK_INCLUDE_DIR})
# 	list(APPEND _cppcheck_args ${CPPCHECK_INCLUDEPATH_ARG} ${QT_QTXML_INCLUDE_DIR})
# 	list(APPEND _cppcheck_args ${CPPCHECK_INCLUDEPATH_ARG} ${QT_QTSVG_INCLUDE_DIR})

	list(APPEND _cppcheck_args "enable" "all")
	
	add_custom_target(
		cppcheck
		COMMAND 
		"${CPPCHECK_EXECUTABLE}"
		${CPPCHECK_TEMPLATE_ARG}
		${CPPCHECK_QUIET_ARG}
		"--force"
#		"--std=c++11"
		${_cppcheck_args}
		${CMAKE_SOURCE_DIR}
	)

	message(STATUS "cppcheck target added.")
endif(CPPCHECK_FOUND)

include_directories (
	${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
	${QT_QTCORE_INCLUDE_DIR} ${QT_QTGUI_INCLUDE_DIR} ${QT_QTNETWORK_INCLUDE_DIR} ${QT_QTXML_INCLUDE_DIR} ${QT_QTSVG_INCLUDE_DIR}
	)

if( WITH_DBUS )
    include_directories( ${QT_QTDBUS_INCLUDE_DIR} )
endif( WITH_DBUS )

set ( vym_HDRS
      aboutdialog.h
      taskfiltermodel.h
      animpoint.h
      attribute.h
      attributeitem.h
      #   attributedelegate.h
      #   attributedialog.h
      #   attributewidget.h
      branchitem.h
      branchobj.h
      branchpropeditor.h
      bugagent.h
      command.h
      dockeditor.h
      downloadagent.h
      editxlinkdialog.h
      exportoofiledialog.h
      exporthtmldialog.h
      exports.h
      extrainfodialog.h
      file.h
      findwidget.h
      findresultwidget.h
      findresultitem.h
      findresultmodel.h
      flag.h
      flagobj.h
      flagrowobj.h
      flagrow.h
      floatimageobj.h
      floatobj.h
      frameobj.h
      geometry.h
      headingeditor.h
      headingobj.h
      highlighter.h
      historywindow.h
      imageitem.h
      imageobj.h
      imports.h
      lineeditdialog.h
      linkablemapobj.h
      macros.h
      mainwindow.h
      mapeditor.h
      mapitem.h
      mapobj.h
      misc.h
      mysortfilterproxymodel.h
      noteeditor.h
      noteobj.h
      options.h
      ornamentedobj.h
      parser.h
      process.h
      scripteditor.h
      settings.h
      shortcuts.h
      showtextdialog.h
      slidecontrolwidget.h
      slideeditor.h
      slideitem.h
      slidemodel.h
      task.h
      taskeditor.h
      taskmodel.h
      treedelegate.h
      treeeditor.h
      treeitem.h
      treemodel.h
      texteditor.h
      version.h
      vymmodel.h
      vymview.h
      winter.h
      warningdialog.h
      xlink.h
      xlinkitem.h
      xlinkobj.h
      xml-base.h
      xml-vym.h
      xml-freemind.h
      xmlobj.h
      xsltproc.h
	)

set ( vym_SRCS
      aboutdialog.cpp
      taskfiltermodel.cpp
      animpoint.cpp
      attribute.cpp
      attributeitem.cpp
      #   attributedelegate.cpp
      #   attributedialog.cpp
      #   attributewidget.cpp
      branchitem.cpp
      branchobj.cpp
      branchpropeditor.cpp
      bugagent.cpp
      command.cpp
      dockeditor.cpp
      downloadagent.cpp
      editxlinkdialog.cpp
      exportoofiledialog.cpp
      exports.cpp
      exporthtmldialog.cpp
      extrainfodialog.cpp
      file.cpp
      findwidget.cpp
      findresultwidget.cpp
      findresultitem.cpp
      findresultmodel.cpp
      flag.cpp
      flagobj.cpp
      flagrow.cpp
      flagrowobj.cpp
      floatimageobj.cpp
      floatobj.cpp
      frameobj.cpp
      geometry.cpp
      headingeditor.cpp
      headingobj.cpp
      highlighter.cpp
      historywindow.cpp
      imageitem.cpp
      imageobj.cpp
      imports.cpp
      lineeditdialog.cpp
      linkablemapobj.cpp
      macros.cpp
      main.cpp
      mainwindow.cpp
      mapeditor.cpp
      mapitem.cpp
      mapobj.cpp
      misc.cpp
      mysortfilterproxymodel.cpp
      noteeditor.cpp
      noteobj.cpp
      options.cpp
      ornamentedobj.cpp
      parser.cpp
      process.cpp
      scripteditor.cpp
      settings.cpp
      shortcuts.cpp
      showtextdialog.cpp
      slidecontrolwidget.cpp
      slideeditor.cpp
      slideitem.cpp
      slidemodel.cpp
      task.cpp
      taskeditor.cpp
      taskmodel.cpp
      texteditor.cpp
      treedelegate.cpp
      treeeditor.cpp
      treeitem.cpp
      treemodel.cpp
      version.cpp
      vymmodel.cpp
      vymview.cpp
      winter.cpp
      warningdialog.cpp
      xlink.cpp
      xlinkitem.cpp
      xlinkobj.cpp
      xml-base.cpp
      xml-vym.cpp
      xml-freemind.cpp
      xmlobj.cpp
      xsltproc.cpp
	)

set ( vym_UIS
      attributewidget.ui
      branchpropeditor.ui
      exporthtmldialog.ui
      extrainfodialog.ui
      editxlinkdialog.ui
      historywindow.ui
      lineeditdialog.ui
      scripteditor.ui
      showtextdialog.ui
      warningdialog.ui
	)
QT4_WRAP_UI(UIS ${vym_UIS})

set ( vym_TRS
	lang/vym_de_DE.ts
	lang/vym_en.ts
	lang/vym_es.ts
	lang/vym_fr.ts
	lang/vym_it.ts
	lang/vym_pt_BR.ts
	lang/vym_ru.ts
	lang/vym_sv.ts
	lang/vym_zh_CN.ts
	lang/vym_zh_TW.ts
	lang/vym_cs_CZ.ts
	)
QT4_ADD_TRANSLATION(TRS ${vym_TRS})

set ( vym_MOCS
      aboutdialog.h
      branchpropeditor.h
      bugagent.h
      dockeditor.h
      downloadagent.h
      editxlinkdialog.h
      exportoofiledialog.h
      exporthtmldialog.h
      extrainfodialog.h
      findwidget.h
      findresultwidget.h
      findresultmodel.h
      headingeditor.h
      highlighter.h
      historywindow.h
      lineeditdialog.h
      mainwindow.h
      mapeditor.h
      mysortfilterproxymodel.h
      noteeditor.h
      process.h
      scripteditor.h
      showtextdialog.h
      slidecontrolwidget.h
      slideeditor.h
      slidemodel.h
      taskeditor.h
      taskmodel.h
      treedelegate.h
      treeeditor.h
      treemodel.h
      texteditor.h
      vymmodel.h
      vymview.h
      winter.h
      warningdialog.h
	)
if( WITH_DBUS )
    add_definitions(-DVYM_DBUS)
    list(APPEND vym_HDRS adaptormodel.h adaptorvym.h)
    list(APPEND vym_SRCS adaptormodel.cpp adaptorvym.cpp)
    list(APPEND vym_MOCS adaptormodel.h adaptorvym.h)
endif( WITH_DBUS )
QT4_WRAP_CPP(MOCS ${vym_MOCS})



if( WIN32 )
	set( vym_SRCS ${vym_SRCS} mkdtemp.cpp vym.rc )
endif( WIN32 )

add_executable ( vym WIN32 ${vym_SRCS} ${UIS} ${RSCS} ${TRS} ${MOCS} )
target_link_libraries ( vym  ${QT_QTMAIN_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${QT_QTXML_LIBRARY} ${QT_QTSVG_LIBRARY} )
if( WITH_DBUS )
    target_link_libraries( vym ${QT_QTDBUS_LIBRARY} )
endif( WITH_DBUS )

install( TARGETS vym RUNTIME DESTINATION bin )

set( _installfolders demos exports flags icons macros scripts styles )
install( DIRECTORY ${_installfolders} DESTINATION share/vym )
install( FILES ${TRS} DESTINATION share/vym/lang )
