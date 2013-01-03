#include "xml-vym.h"

#include <QMessageBox>
#include <QColor>
#include <QTextStream>
#include <typeinfo>

#include "attributeitem.h"
#include "branchitem.h"
#include "misc.h"
#include "settings.h"
#include "linkablemapobj.h"
#include "mainwindow.h"
#include "slideitem.h"
#include "task.h"
#include "taskmodel.h"
#include "version.h"
#include "xlinkitem.h"

extern Main *mainWindow;
extern Settings settings;
extern TaskModel *taskModel;
extern QString vymVersion;

parseVYMHandler::parseVYMHandler()
{
    // Default is to load everything
    contentFilter = 0x0000; //FIXME-4 use filters for all content types below
}

void parseVYMHandler::setContentFilter (const int &c)
{
    contentFilter=c;
}

bool parseVYMHandler::startDocument()
{
    errorProt = "";
    state = StateInit;
    stateStack.clear();
    stateStack.append(StateInit);
    htmldata="";
    isVymPart=false;
    useProgress=false;
    return true;
}

bool parseVYMHandler::startElement  ( const QString&, const QString&,
                    const QString& eName, const QXmlAttributes& atts ) 
{
    QColor col;
    /* Testing
    qDebug()<< "startElement <"<< eName
	<<">  state="<<state 
	<<"  laststate="<<stateStack.last()
	<<"   loadMode="<<loadMode
    	//<<"       line="<<QXmlDefaultHandler::lineNumber();
	<<"contentFilter="<<contentFilter;
    */	
    stateStack.append (state);	
    if ( state == StateInit && (eName == "vymmap")  ) 
    {
        state = StateMap;
	branchesTotal=0;	
	branchesCounter=0;

	if (loadMode==NewMap )
	{
	    // Create mapCenter
	    model->clear();
	    lastBranch=NULL;

	    readMapAttr (atts);
	}   
	// Check version
	if (!atts.value( "version").isEmpty() ) 
	{
	    if (!checkVersion(atts.value("version")))
		QMessageBox::warning( 0, QObject::tr("Warning: Version Problem") , 
		   QObject::tr("<h3>Map is newer than VYM</h3>"
		   "<p>The map you are just trying to load was "
		   "saved using vym %1. "
		   "The version of this vym is %2. " 
		   "If you run into problems after pressing "
		   "the ok-button below, updating vym should help.</p>").arg(atts.value("version")).arg(vymVersion));
	    else       
		model->setVersion(atts.value( "version" ));

	}

    } else if ( eName == "select" && state == StateMap ) 
    {
	state=StateMapSelect;
    } else if ( eName == "setting" && state == StateMap ) 
    {
	state=StateMapSetting;
	if (loadMode==NewMap)
	    readSettingAttr (atts);
    } else if ( eName == "slide" && state == StateMap )
    {
	state=StateMapSlide;
	if (!  contentFilter && SlideContent)  
	{   
	    // Ignore slides during paste
	    lastSlide=model->addSlide();
	    if (insertPos>=0)
	    model->relinkSlide (lastSlide, insertPos);
	    
	    readSlideAttr(atts);
	}
    } else if ( eName == "mapcenter" && state == StateMap ) 
    {
	state=StateMapCenter;
	if (loadMode==NewMap)
	{   
	    // Really use the found mapcenter as MCO in a new map
	    lastBranch=model->createMapCenter(); 
	} else
	{
	    // Treat the found mapcenter as a branch 
	    // in an existing map
	    BranchItem *bi=model->getSelectedBranch();	
	    if (bi)
	    {
		lastBranch=bi;
		if (loadMode==ImportAdd)
		{
		    // Import Add
		    if (insertPos<0) 
			lastBranch=model->createBranch(lastBranch);
		    else
		    {
			lastBranch=model->addNewBranch(lastBranch, insertPos);
			insertPos++;
		    }
		} else  
		{
		    // Import Replace 
		    if (insertPos<0)
		    {
			insertPos=lastBranch->num() +1;
			model->clearItem (lastBranch);
		    } else
		    {
			BranchItem *pi=bi->parentBranch();
			lastBranch=model->addNewBranch(pi, insertPos);
			insertPos++;
		    }
		}
	    } else
		// if nothing selected, add mapCenter without parent
		lastBranch=model->createMapCenter(); 
	}	
	readBranchAttr (atts);
    } else if ( 
	(eName == "standardflag" ||eName == "standardFlag") && 
	(state == StateMapCenter || state==StateBranch)) 
    {
	state=StateStandardFlag;
    } else if ( eName == "heading" && (state == StateMapCenter||state==StateBranch)) 
    {
	state=StateHeading;
	if (!atts.value( "textColor").isEmpty() ) 
	{
	    col.setNamedColor(atts.value("textColor"));
	    lastBranch->setHeadingColor(col );
	}	
    } else if ( eName == "task" && (state == StateMapCenter||state==StateBranch)) 
    {
	state=StateTask;
	lastTask=taskModel->createTask (lastBranch);
	if (!readTaskAttr(atts)) return false;
    } else if ( eName == "note" && 
		(state == StateMapCenter ||state==StateBranch))
    {	// only for backward compatibility (<1.4.6). Use htmlnote now.
	state=StateNote;
	if (!readNoteAttr (atts) ) return false;
    } else if ( eName == "htmlnote" && state == StateMapCenter) 
    {
	state=StateHtmlNote;
	no.clear();
	if (!atts.value( "fonthint").isEmpty() ) 
	    no.setFontHint(atts.value ("fonthint") );
    } else if ( eName == "vymnote" && 
		(state == StateMapCenter ||state==StateBranch)) 
    {
	state=StateVymNote;
	no.clear();
	if (!atts.value( "fonthint").isEmpty() ) 
	    no.setFontHint(atts.value ("fonthint") );
    } else if ( eName == "floatimage" && 
		(state == StateMapCenter ||state==StateBranch)) 
    {
	state=StateImage;
	lastImage=model->createImage(lastBranch);
	if (!readImageAttr(atts)) return false;
    } else if ( (eName == "branch"||eName=="floatimage") && state == StateMap) 
    {
	// This is used in vymparts, which have no mapcenter or for undo
	isVymPart=true;
	TreeItem *ti=model->getSelectedItem();
	if (!ti)
	{
	    // If a vym part is _loaded_ (not imported), 
	    // selection==lmo==NULL
	    // Treat it like ImportAdd then...
	    loadMode=ImportAdd;
	    // we really have no MCO at this time
	    lastBranch=model->createMapCenter();
	    model->select (lastBranch);
	    model->setHeading("Import");
	    ti=lastBranch;
	}   
	if (ti && ti->isBranchLikeType() )
	{
	    lastBranch=(BranchItem*)ti;
	    if (eName=="branch")
	    {
		state=StateBranch;
		if (loadMode==ImportAdd)
		{
		    lastBranch=model->createBranch(lastBranch);
		    if (insertPos>=0)
			model->relinkBranch (lastBranch,(BranchItem*)ti,insertPos);
		} else
		    model->clearItem (lastBranch);
		readBranchAttr (atts);
	    } else if (eName=="floatimage")
	    {
		state=StateImage;
		lastImage=model->createImage (lastBranch);
		if (!readImageAttr(atts)) return false;
	    } else return false;
	} else return false;
    } else if ( eName == "branch" && state == StateMapCenter) 
    {
	state=StateBranch;
	lastBranch=model->createBranch(lastBranch);
	readBranchAttr (atts);
    } else if ( eName == "htmlnote" && state == StateBranch) 
    {
	state=StateHtmlNote;
	no.clear();
	if (!atts.value( "fonthint").isEmpty() ) 
	    no.setFontHint(atts.value ("fonthint") );
    } else if ( eName == "frame" && (state == StateBranch||state==StateMapCenter)) 
    {
	state=StateFrame;
	if (!readFrameAttr(atts)) return false;
    } else if ( eName == "xlink" && state == StateBranch ) 
    {
	// Obsolete after 1.13.2
	state=StateBranchXLink;
	if (!readXLinkAttr (atts)) return false;
    } else if ( eName == "xlink" && state == StateMap) 
    {
	state=StateLink;
	if (!readLinkNewAttr (atts)) return false;
    } else if ( eName == "branch" && state == StateBranch ) 
    {
	lastBranch=model->createBranch(lastBranch);
	readBranchAttr (atts);
    } else if ( eName == "html" && 
	(state == StateHtmlNote || state == StateVymNote) ) 
    {
	state=StateHtml;
	htmldata="<"+eName;
	readHtmlAttr(atts);
	htmldata+=">";
    } else if ( eName == "attribute" && 
	(state == StateBranch || state == StateMapCenter ) ) 
    {
	state=StateAttribute;
	QList<QVariant> cData;
	cData << "new attribute" << "undef";
	AttributeItem *ai=new AttributeItem (cData);
	if (ai)
	{
	    if (!atts.value("type").isEmpty())
		ai->setKey(atts.value("type"));
	    if (!atts.value("key").isEmpty())
		ai->setKey(atts.value("key"));
	    if (!atts.value("value").isEmpty())
		ai->setKey(atts.value("value"));
	} 
	    
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

bool parseVYMHandler::endElement  ( const QString&, const QString&, const QString &eName)
{
    /* Testing
    qDebug()<< "endElement </" <<eName
	<<">  state=" <<state 
    //	<<"  stateStack="<<stateStack.last() 
    //	<<"  selString="<<model->getSelectString().toStdString();
    */
    switch ( state ) 
    {
	case StateMap:
	    break;
        case StateMapCenter: 
	    model->emitDataChanged (lastBranch);
	    lastBranch=(BranchItem*)(lastBranch->parent());
            break;
        case StateBranch: 
	    // Empty branches may not be scrolled 
	    // (happens if bookmarks are imported)
	    if (lastBranch->isScrolled() && lastBranch->branchCount()==0) 
		lastBranch->unScroll();
	    model->emitDataChanged (lastBranch);

	    lastBranch=(BranchItem*)(lastBranch->parent());
	    lastBranch->setLastSelectedBranch (0);  
            break;
	case StateHtmlNote: 
	    no.setNote (htmldata);  // Richtext note, needed anyway for backward compatibility
	    lastBranch->setNoteObj (no);
	    break;  
	case StateMapSlide: 
	    lastSlide=NULL;
	    break;  
	case StateVymNote:	    // Might be richtext or plaintext with 
				    // version >= 1.13.8
	    no.setNote (htmldata);
	    if (!no.isRichText())
		no.setNoteMasked (htmldata);
	    lastBranch->setNoteObj (no);
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

bool parseVYMHandler::characters   ( const QString& ch)
{
    //qDebug()<< "xml-vym: characters \""<<ch<<"\"  state="<<state;

    QString ch_org=quotemeta (ch);
    QString ch_simplified=ch.simplified();
    if ( ch_simplified.isEmpty() ) return true;

    switch ( state ) 
    {
        case StateInit: break;
        case StateMap: break; 
	case StateMapSelect:
	    model->select(ch_simplified);
	    break;
	case StateMapSetting:break;
        case StateMapCenter: break;
        case StateNote:	    // only in vym <1.4.6
	    lastBranch->setNote(ch_simplified);
	    break;
        case StateBranch: break;
        case StateStandardFlag: 
            lastBranch->activateStandardFlag(ch_simplified); 
            break;
        case StateImage: break;
        case StateVymNote: 
	    htmldata=ch;
	    break;
        case StateHtmlNote: 
	    htmldata=ch;
	    break;
        case StateHtml:
	    htmldata+=ch_org;
	    break;
        case StateHeading: 
            lastBranch->setHeading(ch);
            break;
        default: 
	    return false;
    }
    return true;
}

QString parseVYMHandler::errorString() 
{
    return "the document is not in the VYM file format";
}

bool parseVYMHandler::readMapAttr (const QXmlAttributes& a)	
{
    QColor col;
    if (!a.value( "author").isEmpty() )  
	model->setAuthor(a.value( "author" ) );
    if (!a.value( "title").isEmpty() )
	model->setTitle (a.value( "title" ) );
    if (!a.value( "comment").isEmpty() )
	model->setComment (a.value( "comment" ) );
    if (!a.value( "branchCount").isEmpty() )
    {
	branchesTotal=a.value("branchCount").toInt();
	if (branchesTotal>10)
	{
	    useProgress=true;
	    mainWindow->setProgressMaximum (branchesTotal);
	}
    } 
	
    if (!a.value( "backgroundColor").isEmpty() )
    {
	col.setNamedColor(a.value("backgroundColor"));
	model->getScene()->setBackgroundBrush(col);
    }	    
    if (!a.value( "defaultFont").isEmpty() )
    {
	QFont font;
	font.fromString(a.value("defaultFont"));
	model->setMapDefaultFont (font);
    }	    
    if (!a.value( "selectionColor").isEmpty() )
    {
	col.setNamedColor(a.value("selectionColor"));
	model->setSelectionColor(col);
    }	    
    if (!a.value( "linkColorHint").isEmpty() ) 
    {
	if (a.value("linkColorHint")=="HeadingColor")
	    model->setMapLinkColorHint(LinkableMapObj::HeadingColor);
	else
	    model->setMapLinkColorHint(LinkableMapObj::DefaultColor);
    }
    if (!a.value( "linkStyle").isEmpty() ) 
	model->setMapLinkStyle(a.value("linkStyle"));
    if (!a.value( "linkColor").isEmpty() ) 
    {
	col.setNamedColor(a.value("linkColor"));
	model->setMapDefLinkColor(col);
    }	

    QPen pen (model->getMapDefXLinkPen() );
    if (!a.value( "defXLinkColor").isEmpty() ) 
    {
	col.setNamedColor(a.value("defXLinkColor"));
	pen.setColor(col);
    }	
    if (!a.value( "defXLinkWidth").isEmpty() ) 
	pen.setWidth(a.value("defXLinkWidth").toInt ());
    if (!a.value( "defXLinkPenStyle").isEmpty() ) 
    {	
	bool ok;
	Qt::PenStyle ps=penStyle (a.value("defXLinkPenStyle"),ok );
	if (!ok) return false;
	pen.setStyle (ps);
    }
    model->setMapDefXLinkPen (pen);

    if (!a.value( "mapZoomFactor").isEmpty() ) 
	model->setMapZoomFactor(a.value("mapZoomFactor").toDouble());
    if (!a.value( "mapRotationAngle").isEmpty() ) 
	model->setMapRotationAngle(a.value("mapRotationAngle").toDouble());
    return true;
}

bool parseVYMHandler::readBranchAttr (const QXmlAttributes& a)	
{
    branchesCounter++;
    if (useProgress) 
	mainWindow->addProgressValue ((float)branchesCounter/branchesTotal);	

    lastMI=lastBranch;

    if (!readOOAttr(a)) return false;

    if (!a.value( "scrolled").isEmpty() )
	lastBranch->toggleScroll(); 
	// (interesting for import of KDE bookmarks)

    if (!a.value( "incImgV").isEmpty() ) 
    {	
	if (a.value("incImgV")=="true")
	    lastBranch->setIncludeImagesVer(true);
	else	
	    lastBranch->setIncludeImagesVer(false);
    }	
    if (!a.value( "incImgH").isEmpty() ) 
    {	
	if (a.value("incImgH")=="true")
	    lastBranch->setIncludeImagesHor(true);
	else	
	    lastBranch->setIncludeImagesHor(false);
    }	
    return true;    
}

bool parseVYMHandler::readFrameAttr (const QXmlAttributes& a)	// FIXME-4 does not work if there is no lmo for treeitem
{
    if (lastMI)
    {
	OrnamentedObj* oo=(OrnamentedObj*)(lastMI->getLMO()); 
	if (oo)
	{
	    bool ok;
	    int x;
	    {
		if (!a.value( "frameType").isEmpty() ) 
		    oo->setFrameType (a.value("frameType"));
		if (!a.value( "penColor").isEmpty() ) 
		    oo->setFramePenColor (a.value("penColor"));
		if (!a.value( "brushColor").isEmpty() ) 
		{
		    oo->setFrameBrushColor (a.value("brushColor"));
		    lastMI->setBackgroundColor (a.value("brushColor"));
		}
		if (!a.value( "padding").isEmpty() ) 
		{
		    x=a.value("padding").toInt(&ok);
		    if (ok) oo->setFramePadding(x);
		}   
		if (!a.value( "borderWidth").isEmpty() ) 
		{
		    x=a.value("borderWidth").toInt(&ok);
		    if (ok) oo->setFrameBorderWidth(x);
		}   
		if (!a.value( "includeChildren").isEmpty() ) 
		{
		    if (a.value("includeChildren")=="true")
			oo->setFrameIncludeChildren(true);
		    else	
			oo->setFrameIncludeChildren(false);
		}   
	    }	    
	    return true;
	}
    }
    return false;
}

bool parseVYMHandler::readOOAttr (const QXmlAttributes& a)
{
    if (lastMI)
    {
	bool okx,oky;
	float x,y;
	if (!a.value( "relPosX").isEmpty() ) 
	{
	    if (!a.value( "relPosY").isEmpty() ) 
	    {
		x=a.value("relPosX").toFloat (&okx);
		y=a.value("relPosY").toFloat (&oky);
		if (okx && oky  )
		    lastMI->setRelPos (QPointF(x,y));
		else
		    return false;   // Couldn't read relPos
	    }           
	}           
	if (!a.value( "absPosX").isEmpty() ) 
	{
	    if (!a.value( "absPosY").isEmpty() ) 
	    {
		x=a.value("absPosX").toFloat (&okx);
		y=a.value("absPosY").toFloat (&oky);
		if (okx && oky  )
		    lastMI->setAbsPos (QPointF(x,y));
		else
		    return false;   // Couldn't read absPos
	    }           
	}           
	if (!a.value( "url").isEmpty() ) 
	    lastMI->setURL (a.value ("url"));
	if (!a.value( "vymLink").isEmpty() ) 
	    lastMI->setVymLink (a.value ("vymLink"));
	if (!a.value( "hideInExport").isEmpty() ) 
	    if (a.value("hideInExport")=="true")
		lastMI->setHideInExport(true);

	if (!a.value( "hideLink").isEmpty()) 
	{
	    if (a.value ("hideLink") =="true")
		lastMI->setHideLinkUnselected(true);
	    else    
		lastMI->setHideLinkUnselected(false);
	}   

	if (!a.value( "localTarget").isEmpty() )
	    if (a.value ("localTarget")=="true")
		lastMI->toggleTarget();
	if (!a.value( "rotation").isEmpty() ) 
	{
	    x=a.value("rotation").toFloat (&okx);
	    if (okx )
		lastMI->setRotation (x);
	    else	
		return false;   // Couldn't read rotation
	}           

        if (!a.value( "uuid").isEmpty() ) 
            lastMI->setUuid (a.value( "uuid") );
    }
    return true;    
}

bool parseVYMHandler::readNoteAttr (const QXmlAttributes& a)
{   // only for backward compatibility (<1.4.6). Use htmlnote now.
    no.clear();
    QString fn;
    if (!a.value( "href").isEmpty() ) 
    {
	// Load note
	fn=parseHREF(a.value ("href") );
	QFile file (fn);
	QString s;			// Reading a note

	if ( !file.open( QIODevice::ReadOnly) )
	{
	    qWarning ()<<"parseVYMHandler::readNoteAttr:  Couldn't load "+fn;
	    return false;
	}   
	QTextStream stream( &file );
	QString lines;
	while ( !stream.atEnd() ) {
	    lines += stream.readLine()+"\n"; 
	}
	file.close();

	lines ="<html><head><meta name=\"qrichtext\" content=\"1\" /></head><body>"+lines + "</p></body></html>";
	no.setNote (lines);
    }	    
    if (!a.value( "fonthint").isEmpty() ) 
	no.setFontHint(a.value ("fonthint") );
    lastBranch->setNoteObj(no);
    return true;
}

bool parseVYMHandler::readImageAttr (const QXmlAttributes& a)
{
    lastMI=lastImage;
    
    if (!readOOAttr(a)) return false;  

    if (!a.value( "href").isEmpty() )
    {
	// Load Image
	if (!lastImage->load (parseHREF(a.value ("href") ) ))
	{
	    QMessageBox::warning( 0, "Warning: " ,
		"Couldn't load image\n"+parseHREF(a.value ("href") ));
	    lastImage=NULL;
	    return true;
	}
	
    }	
    if (!a.value( "zPlane").isEmpty() ) 
	lastImage->setZValue (a.value("zPlane").toInt ());
    float x,y;
    bool okx,oky;
    if (!a.value( "relPosX").isEmpty() ) 
    {
	if (!a.value( "relPosY").isEmpty() ) 
	{
	    // read relPos
	    x=a.value("relPosX").toFloat (&okx);
	    y=a.value("relPosY").toFloat (&oky);
	    if (okx && oky) 
		lastImage->setRelPos (QPointF (x,y) );
	    else
		// Couldn't read relPos
		return false;  
	}           
    }	
    
    // Scale image	
    x=y=1;
    if (!a.value( "scaleX").isEmpty() ) 
    {
	x=a.value("scaleX").toFloat (&okx);
	if (!okx ) return false;  
    }	
    
    if (!a.value( "scaleY").isEmpty() ) 
    {
	y=a.value("scaleY").toFloat (&oky);
	if (!oky ) return false;  
    }	
    if (x!=1 || y!=1)
	lastImage->setScale (x,y);
    
    if (!readOOAttr(a)) return false;

    if (!a.value ("originalName").isEmpty() )
    {
	lastImage->setOriginalFilename (a.value("originalName"));
    }
    return true;
}

bool parseVYMHandler::readXLinkAttr (const QXmlAttributes& a) 
{
    // Obsolete, see also readLinkAttr

    if (!a.value( "beginID").isEmpty() ) 
    { 
	if (!a.value( "endID").isEmpty() ) 
	{
	    TreeItem *beginBI=model->findBySelectString (a.value( "beginID"));
	    TreeItem   *endBI=model->findBySelectString (a.value( "endID"));
	    if (beginBI && endBI && beginBI->isBranchLikeType() && endBI->isBranchLikeType() )
	    {
		Link *li=new Link (model);
		li->setBeginBranch ( (BranchItem*)beginBI );
		li->setEndBranch ( (BranchItem*)endBI);
		QPen pen=li->getPen();

		if (!a.value( "color").isEmpty() ) 
		{
		    QColor col;
		    col.setNamedColor(a.value("color"));
		    pen.setColor (col);
		}

		if (!a.value( "width").isEmpty() ) 
		{
		    bool okx;
		    pen.setWidth(a.value ("width").toInt (&okx, 10));
		}
		model->createLink (li,true);	// create MO by default
	    }
	}           
    }	
    return true;    
}

bool parseVYMHandler::readLinkNewAttr (const QXmlAttributes& a)	
{
    // object ID is used starting in version 1.8.76
    // (before there was beginBranch and endBranch)
    //
    // Starting in 1.13.2 xlinks are no longer subitems of branches,
    // but listed at the end of the data in a map. This make handling 
    // of links much safer and easier

    if (!a.value( "beginID").isEmpty() ) 
    { 
	if (!a.value( "endID").isEmpty() ) 
	{
	    TreeItem *beginBI=model->findBySelectString (a.value( "beginID"));
	    TreeItem   *endBI=model->findBySelectString (a.value( "endID"));
	    if (beginBI && endBI && beginBI->isBranchLikeType() && endBI->isBranchLikeType() )
	    {
		Link *li=new Link (model);
		li->setBeginBranch ( (BranchItem*)beginBI );
		li->setEndBranch ( (BranchItem*)endBI);

		bool okx;
		QPen pen=li->getPen();
		if (!a.value( "type").isEmpty() ) 
		{
		    li->setLinkType (a.value( "type") );
		}
		if (!a.value( "color").isEmpty() ) 
		{
		    QColor col;
		    col.setNamedColor(a.value("color"));
		    pen.setColor (col);
		}

		if (!a.value( "width").isEmpty() ) 
		{
		    pen.setWidth(a.value ("width").toInt (&okx, 10));
		}
		if (!a.value( "penstyle").isEmpty() ) 
		{
		    pen.setStyle( penStyle (a.value ("penstyle"), okx));
		}
		li->setPen (pen);

		model->createLink (li,true);	// create MO by default

		XLinkObj *xlo=(XLinkObj*)(li->getMO() );
		if (xlo && !a.value( "c0").isEmpty() )
		{
		    QPointF p=point(a.value("c0"),okx );
		    if (okx) xlo->setC0 (p);
		}
		if (xlo && !a.value( "c1").isEmpty() )
		{
		    QPointF p=point(a.value("c1"),okx );
		    if (okx) xlo->setC1 (p);
		}
	    }
	}           
    }	
    return true;    
}

bool parseVYMHandler::readSettingAttr (const QXmlAttributes& a)
{
    if (!a.value( "key").isEmpty() ) 
    {
	if (!a.value( "value").isEmpty() ) 
	    settings.setLocalValue(model->getDestPath(), a.value ("key"), a.value ("value"));
	else
	    return false;
	
    } else
	return false;
    
    return true;
}

bool parseVYMHandler::readSlideAttr (const QXmlAttributes& a)
{
    QStringList scriptlines;	// FIXME-3 needed for switching to inScript
                                // Most attributes are obsolete with inScript
    if (!lastSlide) return false;
    {
	if (!a.value( "name").isEmpty() ) 
	    lastSlide->setName (a.value( "name" ) );
	if (!a.value( "zoom").isEmpty() ) 
	{
	    bool ok;
	    qreal z=a.value ("zoom").toDouble(&ok);
	    if (!ok) return false;
	    scriptlines.append( QString("setMapZoom(%1)").arg(z) );
	}
	if (!a.value( "rotation").isEmpty() ) 
	{
	    bool ok;
	    qreal z=a.value ("rotation").toDouble(&ok);
	    if (!ok) return false;
	    scriptlines.append( QString("setMapRotation(%1)").arg(z) );
	}
	if (!a.value( "duration").isEmpty() ) 
	{
	    bool ok;
	    int d=a.value ("duration").toInt(&ok);
	    if (!ok) return false;
	    scriptlines.append( QString("setMapAnimDuration(%1)").arg(d) );
	}
	if (!a.value( "curve").isEmpty() ) 
	{
	    bool ok;
	    int i=a.value ("curve").toInt(&ok);
	    if (!ok ) return false;
	    if (i<0 || i>QEasingCurve::OutInBounce) return false;
	    scriptlines.append( QString("setMapAnimCurve(%1)").arg(i) );
	}
	if (!a.value( "mapitem").isEmpty() ) 
	{
	    TreeItem *ti=model->findBySelectString ( a.value( "mapitem") );
	    if (!ti) return false;
	    scriptlines.append( QString("centerOnID(\"%1\")").arg(ti->getUuid().toString() ) );
	}
	if (!a.value( "inScript").isEmpty() ) 
	{
	    lastSlide->setInScript( unquotemeta( a.value( "inScript") ) );
	} else
	    lastSlide->setInScript( unquotemeta( scriptlines.join(";\n") ) );

	if (!a.value( "outScript").isEmpty() ) 
	{
	    lastSlide->setOutScript( unquotemeta( a.value( "outScript") ) );
	}
    }
    return true;
}

bool parseVYMHandler::readTaskAttr (const QXmlAttributes& a)
{
    if (!lastTask) return false;
    {
	if (!a.value( "status").isEmpty() ) 
	    lastTask->setStatus (a.value( "status" ) );
	if (!a.value( "awake").isEmpty() ) 
	    lastTask->setAwake (a.value( "awake" ) );
	if (!a.value( "date_creation").isEmpty() ) 
	    lastTask->setDateCreation ( a.value( "date_creation" ) );
	if (!a.value( "date_modified").isEmpty() ) 
	    lastTask->setDateModified( a.value( "date_modified" ) );
	if (!a.value( "date_sleep").isEmpty() ) 
	    lastTask->setDateSleep( a.value( "date_sleep" ) );
    }
    return true;
}

