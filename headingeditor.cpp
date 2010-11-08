#include "headingeditor.h"
#include "settings.h"


extern Settings settings;

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////


HeadingEditor::HeadingEditor()
{
    //qDebug()<<"Constr HeadingEditor";
    // Load Settings
    resize (settings.value ( "/satellite/headingeditor/geometry/size", QSize(450,600)).toSize());
    move   (settings.value ( "/satellite/headingeditor/geometry/pos", QPoint (250,50)).toPoint());
    /*
    setCentralWidget( e );
    statusBar()->message( tr("Ready","Statusbar message"), statusbarTime);
    setCaption(vymName +" - " +tr ("Heading Editor","Window caption"));

    // Toolbars
    setupFileActions();
    setupEditActions();
    setupFormatActions();
    setupSettingsActions();
    
    // Various states
    blockChangedSignal=false;
    setInactive();

    
    setShowWithMain (settings.value ( "/satellite/noteeditor/showWithMain",true).toBool());

    varFont.fromString( settings.value
	("/satellite/noteeditor/fonts/varFont",
	"Nimbus Sans l,10,-1,5,48,0,0,0,0,0").toString() 
    );
    fixedFont.fromString (settings.value(
	"/satellite/noteeditor/fonts/fixedFont",
	"Courier,10-1,5,48,0,0,0,1,0").toString() 
    );
    QString s=settings.value ("/satellite/noteeditor/fonts/fonthintDefault","variable").toString();
    if (s == "fixed")
    {	
	actionSettingsFonthintDefault->setOn (true);
	e->setCurrentFont (fixedFont);
    } else  
    {
	actionSettingsFonthintDefault->setOn (false);
	e->setCurrentFont (varFont);
    }	
    filenameHint="";

    // Restore position of toolbars
    restoreState (settings.value("/satellite/noteeditor/state",0).toByteArray());

    // Save settings in vymrc
    settings.setValue("/mainwindow/printerName",printer->printerName());
    */
}


HeadingEditor::~HeadingEditor()
{
    //qDebug() <<"Destr HeadingEditor";

    // Save Settings
    settings.setValue( "/satellite/headingeditor/geometry/size", size() );
    settings.setValue( "/satellite/headingeditor/geometry/pos", pos() );
    settings.setValue ("/satellite/headingeditor/state",saveState(0));
    /*
    settings.setValue( "/satellite/noteeditor/showWithMain",showwithmain);

    QString s;
    if (actionSettingsFonthintDefault->isOn() )
	s="fixed";
    else    
	s="variable";
    settings.setValue( "/satellite/noteeditor/fonts/fonthintDefault",s );
    settings.setValue("/satellite/noteeditor/fonts/varFont", varFont.toString() );
    settings.setValue("/satellite/noteeditor/fonts/fixedFont", fixedFont.toString() );
*/

}



