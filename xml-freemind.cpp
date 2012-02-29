#include "xml-freemind.h"

#include <QDebug>
#include <QMessageBox>

#include "branchitem.h"
#include "version.h"
#include "vymmodel.h"

extern Settings settings;
extern QString vymVersion;
extern QString flagsPath;

bool parseFreemindHandler::startDocument()
{
    errorProt = "";
    state = StateInit;
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
    qDebug() << "startElement <"<< qPrintable(eName)
	<<">  state="<<state 
	<<"  stateStack="<<stateStack.last()
	<<"   loadMode="<<loadMode
    //	<<"       line="<<qPrintable (QXmlDefaultHandler::lineNumber())
	;
    */	
    stateStack.append (state);	
    if ( state == StateInit && (eName == "map")  ) 
    {
        state = StateMap;
	if (!atts.value( "version").isEmpty() ) 
	{
	    QString v="0.9.0";
	    if (!checkVersion(atts.value("version"),v))
		QMessageBox::warning( 0, "Warning: Version Problem" ,
		   "<h3>Freemind map is newer than version " +v +" </h3>"
		   "<p>The map you are just trying to load was "
		   "saved using freemind " +atts.value("version")+". "
		   "Your version of vym can parse freemind " + v +"."); 
	}
	// Create mapcenter
	model->clear();
	mapCenter=model->createMapCenter();
	lastBranch=mapCenter;

	// Create two hidden branches, because Freemind has no relative 
	// positioning for mainbranches
	mainBranchLeft  = model->createBranch (lastBranch);
	mainBranchRight = model->createBranch (lastBranch);

	mainBranchLeft->setRelPos ( QPointF(-200,0));
	mainBranchLeft->setHeading ("  ");
	mainBranchRight->setRelPos ( QPointF(200,0));
	mainBranchRight->setHeading ("  ");
    } else if ( eName == "attribute_registry" &&  state == StateMap ) 
    {
        state = StateAttributeRegistry;
    } else if ( eName == "attribute_name" &&  state == StateAttributeRegistry) 
    {
        state = StateAttributeName;
    } else if ( eName == "attribute_value" &&  state == StateAttributeName) 
    {
        state = StateAttributeValue;
    } else if ( eName == "node" &&  state == StateMap ) 
    {
	state=StateNode;
	readNodeAttr (atts);
    } else if ( eName == "node" &&  state == StateNode ) 
    {
	lastBranch=model->createBranch(lastBranch);
	state=StateNode;
	readNodeAttr (atts);
    } else if ( eName == "font" && state == StateNode) //FIXME-3 not implemented
    {
	state=StateFont;
    } else if ( eName == "edge" && state == StateNode) //FIXME-3 xlink not implemented
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
	    else 
		qWarning()<<"parseFreemindHandler: Unknown icon found: "<<f;

	    lastBranch->activateStandardFlag (v);
	}
    } else if ( eName == "arrowlink" && state == StateNode) 
    {
	state=StateArrowLink;
    } else if ( eName == "cloud" && state == StateNode) 
    {
	state=StateCloud;
    } else if ( eName == "richcontent" && state == StateNode) 
    {
	state=StateRichContent;
	return readRichContentAttr (atts);
    } else if ( eName == "html" && state == StateRichContent) 
    {
	state=StateHtml;
	htmldata="<"+eName;
	readHtmlAttr(atts);
	htmldata+=">";
    } else if ( eName == "text" && state == StateHook) 
    {
	state=StateText;
    } else if ( state == StateHtml ) 
    {
	// accept all while in html mode,
	htmldata+="<"+eName;
	readHtmlAttr(atts);
	htmldata+=">";
    } else 
        return false;   // Error
    return true;
}

bool parseFreemindHandler::endElement  ( const QString &, const QString&, const QString &eName)
{
    /* Testing
    qDebug() << "endElement </" <<qPrintable(eName)
	<<">  state=" <<state 
	<<"  stateStack="<<stateStack.last() 
	;
    */
    switch ( state ) 
    {
	case StateMap:
	    // Freemind does not have the two "extra" mainbranches used here,
	    // so we have to update mapcenter
	    model->emitDataHasChanged (mapCenter);

	    // Remove helper branches, if not needed
	    if (mainBranchLeft->childCount()==0) model->deleteItem (mainBranchLeft);
	    if (mainBranchRight->childCount()==0) model->deleteItem (mainBranchRight);
	    break;
        case StateNode: 
	    model->emitDataHasChanged (lastBranch);
	    lastBranch=(BranchItem*)lastBranch->parent();
	    lastBranch->setLastSelectedBranch (0);  
            break;
	case StateRichContent:
	    if (!htmldata.isEmpty()) 
	    {
		if (htmlPurpose==Node)
		    lastBranch->setHeading (htmldata);
		else if (htmlPurpose==Note)
		    lastBranch->setNote (htmldata);
	    }	
	    break;
        case StateHtml: 
	    htmldata+="</"+eName+">";
	    if (eName=="html")
	    {
		//state=StateHtmlNote;  
		htmldata.replace ("<br></br>","<br />");
	    }	
	    break;
	default: 
	    break;
    }  
    state=stateStack.takeLast();    
    return true;
}

bool parseFreemindHandler::characters   ( const QString& ch)
{
    //qDebug() << "characters \""<<qPrintable(ch)<<"\"  state="<<state;

    QString ch_org=quotemeta (ch);
    QString ch_simplified=ch.simplified();
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
	    lastBranch->setNote (ch_simplified);
	    break; 
        case StateHtml:
	    htmldata+=ch_org;
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

bool parseFreemindHandler::readNodeAttr (const QXmlAttributes& a)  
{
    // Freemind has a different concept for mainbranches
    if (!a.value( "POSITION").isEmpty() )
    {
	if (a.value ("POSITION")=="left")
	    model->relinkBranch (lastBranch, mainBranchLeft);
	else if (a.value ("POSITION")=="right")
	    model->relinkBranch (lastBranch, mainBranchRight);
    } 

    if (a.value( "FOLDED")=="true" )
	lastBranch->toggleScroll();

    if (!a.value( "TEXT").isEmpty() )
    {
	lastBranch->setHeading (a.value ("TEXT"));
	//model->setHeading (a.value ("TEXT"), lastBranch);
    }

    if (!a.value( "COLOR").isEmpty() )
	lastBranch->setHeadingColor (QColor (a.value ("COLOR")));

    if (!a.value( "LINK").isEmpty() )	
	lastBranch->setURL (a.value ("LINK"));
    return true;    
}


bool parseFreemindHandler::readRichContentAttr (const QXmlAttributes& a)  
{
    if (a.value ("TYPE")=="NODE" )
	htmlPurpose=Node;
    else if (a.value ("TYPE")=="NOTE" )
	htmlPurpose=Note;
    else
    {
	htmlPurpose=Unknown;
	qWarning()<<"parseFreemindHandler: Unknown purpose of richContent found";
	return false;
    }	
    return true;
}

