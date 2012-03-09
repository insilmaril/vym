#include "headingeditor.h"
#include "settings.h"


extern Settings settings;
extern QString vymName;

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////


HeadingEditor::HeadingEditor()	
{
    // Load Settings
    restoreState (settings.value("/satellite/headingeditor/state",0).toByteArray());
    resize (settings.value ( "/satellite/headingeditor/geometry/size", QSize(450,600)).toSize());
    move   (settings.value ( "/satellite/headingeditor/geometry/pos", QPoint (250,50)).toPoint());

    
    setShowWithMain (settings.value ( "/satellite/headingeditor/showWithMain",true).toBool());
    setWindowTitle (vymName +" - " +tr ("Heading Editor","Window caption"));
    varFont.fromString( settings.value
	("/satellite/headingeditor/fonts/varFont",
	"DejaVu Sans Mono [unknown],14,-1,0,50,0,0,0,0,0").toString() 
    );
    fixedFont.fromString (settings.value(
	"/satellite/headingeditor/fonts/fixedFont",
	"Courier,10-1,5,48,0,0,0,1,0").toString() 
    );
    QString s=settings.value ("/satellite/headingeditor/fonts/fonthintDefault","variable").toString();
    if (s == "fixed")
    {	
	actionSettingsFonthintDefault->setChecked (true);
	e->setCurrentFont (fixedFont);
    } else  
    {
	actionSettingsFonthintDefault->setChecked (false);
	e->setCurrentFont (varFont);
    }	
}


HeadingEditor::~HeadingEditor()
{
    // Save Settings
    settings.setValue( "/satellite/headingeditor/geometry/size", size() );
    settings.setValue( "/satellite/headingeditor/geometry/pos", pos() );
    settings.setValue ("/satellite/headingeditor/state",saveState(0));
    
    settings.setValue( "/satellite/headingeditor/showWithMain",showwithmain);

    QString s;
    if (actionSettingsFonthintDefault->isChecked() )
	s="fixed";
    else    
	s="variable";
    settings.setValue( "/satellite/headingeditor/fonts/fonthintDefault",s );
    settings.setValue("/satellite/headingeditor/fonts/varFont", varFont.toString() );
    settings.setValue("/satellite/headingeditor/fonts/fixedFont", fixedFont.toString() );
}



