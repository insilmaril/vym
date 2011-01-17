#include "noteeditor.h"

#include "noteobj.h"
#include "settings.h"

extern bool debug;
extern Settings settings;


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////


NoteEditor::NoteEditor()
{

    //qDebug()<<"Constr. NoteEditor";
    menuBar()->show();
    // Load Settings
    //restoreState (settings.value("/satellite/noteeditor/state",0).toByteArray());
    resize (settings.value ( "/satellite/noteeditor/geometry/size", QSize(450,600)).toSize());
    move   (settings.value ( "/satellite/noteeditor/geometry/pos", QPoint (250,50)).toPoint());
    
    setShowWithMain (settings.value ( "/satellite/noteeditor/showWithMain",true).toBool());
    return;

    /*
    setCentralWidget( e );
    statusBar()->message( tr("Ready","Statusbar message"), statusbarTime);
    setCaption(vymName +" - " +tr ("Note Editor","Window caption"));

    // Toolbars
    setupFileActions();
    setupEditActions();
    setupFormatActions();
    setupSettingsActions();
    
    // Various states
    blockChangedSignal=false;
    setInactive();
    */


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
	actionSettingsFonthintDefault->setChecked (true);
	e->setCurrentFont (fixedFont);
    } else  
    {
	actionSettingsFonthintDefault->setChecked (false);
	e->setCurrentFont (varFont);
    }	

}


NoteEditor::~NoteEditor()
{
    //qDebug ()<<"Destr. NoteEditor";
    
    // Save Settings
    settings.setValue( "/satellite/noteeditor/geometry/size", size() );
    settings.setValue( "/satellite/noteeditor/geometry/pos", pos() );
    //settings.setValue ("/satellite/noteeditor/state",saveState(0));
    
    settings.setValue( "/satellite/noteeditor/showWithMain",showwithmain);

    QString s;
    if (actionSettingsFonthintDefault->isChecked() )
	s="fixed";
    else    
	s="variable";
    settings.setValue( "/satellite/noteeditor/fonts/fonthintDefault",s );
    settings.setValue("/satellite/noteeditor/fonts/varFont", varFont.toString() );
    settings.setValue("/satellite/noteeditor/fonts/fixedFont", fixedFont.toString() );
}


NoteObj NoteEditor::getNoteObj()
{
    NoteObj note (getText() );
    note.setFontHint (getFontHint() );
    note.setFilenameHint (getFilenameHint () );
    return note;
}

void NoteEditor::setNote (const NoteObj &note)
{
    setText (note.getNote() );
    setFilenameHint (note.getFilenameHint() );
    setFontHint (note.getFontHint() );
}


