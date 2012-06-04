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
}


HeadingEditor::~HeadingEditor()
{
    // Save Settings
    settings.setValue( "/satellite/headingeditor/geometry/size", size() );
    settings.setValue( "/satellite/headingeditor/geometry/pos", pos() );
    settings.setValue ("/satellite/headingeditor/state",saveState(0));
    
    settings.setValue( "/satellite/headingeditor/showWithMain",showwithmain);
}

