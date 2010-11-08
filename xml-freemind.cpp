#include "xml-freemind.h"

#include <QMessageBox>
#include <QColor>
#include <QTextStream>
#include <iostream>

#include "branchitem.h"
#include "misc.h"
#include "settings.h"
#include "linkablemapobj.h"
#include "version.h"

//static FloatObj *lastFloat;

extern Settings settings;
extern QString vymVersion;

extern QString flagsPath;

bool parseFreemindHandler::startDocument()
{
    errorProt = "";
    state = StateInit;
    laststate = StateInit;
    stateStack.clear();
    stateStack.append(StateInit);
    isVymPart=false;
    return true;
}


QString parseFreemindHandler::parseHREF(QString href)
{
    QString type=href.section(":",0,0);
    QString path=href.section(":",1,1);
    if (!tmpDir.endsWith("/"))
	return tmpDir + "/" + path;
    else    
	return tmpDir + path;
}

bool parseFreemindHandler::startElement  ( const QString&, const QString&,
                    const QString& eName, const QXmlAttributes& atts ) 
{
    QColor col;
    /* Testing
    cout << "startElement <"<< qPrintable(eName)
	<<">  state="<<state 
	<<"  laststate="<<stateStack.last()
	<<"   loadMode="<<loadMode
    //	<<"       line="<<qPrintable (QXmlDefaultHandler::lineNumber())
	<<endl;
    */	
    stateStack.append (state);	
    if ( state == StateInit && (eName == "map")  ) 
    {
        state = StateMap;
	if (!atts.value( "version").isEmpty() ) 
	{
	    QString v="0.8.0";
	    if (!checkVersion(atts.value("version"),v))
		QMessageBox::warning( 0, "Warning: Version Problem" ,
		   "<h3>Freemind map is newer than version " +v +" </h3>"
		   "<p>The map you are just trying to load was "
		   "saved using freemind " +atts.value("version")+". "
		   "The version of this vym can parse freemind " + v +"."); 
	}
	//FIXME-3 TreeItem *ti=model->first();	//  this will be NULL !!!
	TreeItem *ti=NULL;

	if (ti->getType()!=TreeItem::MapCenter)
	    qWarning ("parseFreeMindHandler::startElement  no mapCenter!!");

	//cout <<"model="<<model<<"   first="<<model->first()<<endl;

	lastBranchItem=model->createBranch(lastBranchItem);

	//FIXME-3 lastBranch->move2RelPos (200,0);
	lastBranchItem->setHeading ("  ");
	//FIXME-3 lastBranch->move2RelPos (-200,0);
	lastBranchItem->setHeading ("  ");
	lastBranchItem=(BranchItem*)lastBranchItem->parent();

    } else if ( eName == "node" &&  (state == StateMap || state == StateNode )) 
    {
	if (!atts.value( "POSITION").isEmpty() )
	{
	    if (atts.value ("POSITION")=="left")
	    {
		model->select ("bo:1");
		lastBranchItem=model->getSelectedBranch();
		if (lastBranchItem)
		{   
		    lastBranchItem=model->createBranch(lastBranchItem);
		    readNodeAttr (atts);
		}   
	    } else if (atts.value ("POSITION")=="right")
	    {
		model->select ("bo:0");
		lastBranchItem=model->getSelectedBranch();
		if (lastBranchItem)
		{   
		    lastBranchItem=model->createBranch(lastBranchItem);
		    readNodeAttr (atts);
		}   
	    }
	} else
	{
	    if (state!=StateMap)
	    {
		lastBranchItem=model->createBranch(lastBranchItem);
	    }
	    readNodeAttr (atts);
	}
	state=StateNode;
    } else if ( eName == "font" && state == StateNode) 
    {
	state=StateFont;
    } else if ( eName == "edge" && state == StateNode) 
    {
	state=StateEdge;
    } else if ( eName == "hook" && state == StateNode) 
    {
	state=StateHook;
    } else if ( eName == "icon" && state == StateNode) 
    {
	state=StateIcon;
	if (!atts.value("BUILTIN").isEmpty() )
	{
	    QString f=atts.value("BUILTIN");
	    QString v;
	    if (f=="help")
		v="questionmark";
	    else if (f=="messagebox_warning")
		v="freemind-warning"; 
	    else if (f=="idea")
		v="lamp"; 
	    else if (f=="button_ok")
		v="hook-green"; 
	    else if (f=="button_cancel")
		v="cross-red"; 
	    else if (f.contains("full-"))
		v=f.replace("full-","freemind-priority-"); 
	    else if (f=="back")
		v="freemind-back"; 
	    else if (f=="forward")
		v="freemind-forward"; 
	    else if (f=="attach")
		v="freemind-attach"; 
	    else if (f=="ksmiletris")
		v="smiley-good"; // 
	    else if (f=="clanbomber")
		v="freemind-clanbomber"; 
	    else if (f=="desktop_new")
		v="freemind-desktopnew"; 
	    else if (f=="flag")
		v="freemind-flag"; 
	    else if (f=="gohome")
		v="freemind-gohome"; 
	    else if (f=="kaddressbook")
		v="freemind-kaddressbook"; 
	    else if (f=="knotify")
		v="freemind-knotify"; 
	    else if (f=="korn")
		v="freemind-korn";
	    else if (f=="Mail")
		v="freemind-mail"; 
	    else if (f=="password")
		v="freemind-password"; 
	    else if (f=="pencil")
		v="freemind-pencil";
	    else if (f=="stop")
		v="freemind-stop"; 
	    else if (f=="wizard")
		v="freemind-wizard";
	    else if (f=="xmag")
		v="freemind-xmag";
	    else if (f=="bell")
		v="freemind-bell";
	    else if (f=="bookmark")
		v="freemind-bookmark"; 
	    else if (f=="penguin")
		v="freemind-penguin"; 
	    else if (f=="licq")
		v="freemind-licq"; 

	    //FIXME-3 lastBranch->activateStandardFlag( v);
	}
    } else if ( eName == "arrowlink" && state == StateNode) 
    {
	state=StateArrowLink;
    } else if ( eName == "cloud" && state == StateNode) 
    {
	state=StateCloud;
    } else if ( eName == "text" && state == StateHook) 
    {
	state=StateText;
    } else 
        return false;   // Error
    return true;
}

bool parseFreemindHandler::endElement  ( const QString&, const QString&, const QString&)
{
    /* Testing
    cout << "endElement </" <<qPrintable(eName)
	<<">  state=" <<state 
	<<"  laststate=" <<laststate
	<<"  stateStack="<<stateStack.last() 
	<<endl;
    */
    switch ( state ) 
    {
        case StateNode: 
	    lastBranchItem=(BranchItem*)lastBranchItem->parent();
            break;
	default: 
	    break;
    }  
    state=stateStack.takeLast();    
    return true;
}

bool parseFreemindHandler::characters   ( const QString& ch)
{
    //cout << "characters \""<<qPrintable(ch)<<"\"  state="<<state <<"  laststate="<<laststate<<endl;

    QString ch_org=quotemeta (ch);
    QString ch_simplified=ch.simplifyWhiteSpace();
    if ( ch_simplified.isEmpty() ) return true;

    switch ( state ) 
    {
        case StateInit: break;
        case StateMap: break; 
        case StateNode: break; 
        case StateCloud: break; 
        case StateEdge: break; 
        case StateIcon: break; 
        case StateArrowLink: break; 
        case StateFont: break; 
        case StateHook: break; 
        case StateText: 
	    lastBranchItem->setNote (ch_simplified);
	    break; 
        default: 
	    return false;
    }
    return true;
}

QString parseFreemindHandler::errorString() 
{
    return "the document is not in the Freemind file format";
}

bool parseFreemindHandler::readNodeAttr (const QXmlAttributes& a)   //FIXME-3
{
    //lastBranchItem=(BranchItem*)(lastBranch->getTreeItem() );

    if (a.value( "FOLDED")=="true" )
	lastBranchItem->toggleScroll();
/*
    if (!a.value( "TEXT").isEmpty() )
	lastBranch->setHeading (a.value ("TEXT"));

    if (!a.value( "COLOR").isEmpty() )
	lastBranch->setColor (QColor (a.value ("COLOR")));

    if (!a.value( "LINK").isEmpty() )
	lastBranch->setURL (a.value ("LINK"));
*/
    return true;    
}


