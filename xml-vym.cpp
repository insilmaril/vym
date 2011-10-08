#include "xml-vym.h"

#include <QMessageBox>
#include <QColor>
#include <QTextStream>
#include <iostream>
#include <typeinfo>

#include "attributeitem.h"
#include "branchitem.h"
#include "misc.h"
#include "settings.h"
#include "linkablemapobj.h"
#include "mainwindow.h"
#include "version.h"
#include "xlinkitem.h"

extern Main *mainWindow;
extern Settings settings;
extern QString vymVersion;

bool parseVYMHandler::startDocument()
{
    errorProt = "";
    state = StateInit;
    laststate = StateInit;
    stateStack.clear();
    stateStack.append(StateInit);
    textdata="";
    isVymPart=false;
    useProgress=false;
    return true;
}

bool parseVYMHandler::startElement  ( const QString&, const QString&,
                    const QString& eName, const QXmlAttributes& atts ) 
{
    QColor col;
    /* Testing
    cout << "startElement <"<< qPrintable(eName)
	<<">  state="<<state 
	<<"  laststate="<<stateStack.last()
	<<"   loadMode="<<loadMode
    //	<<"       line="<<QXmlDefaultHandler::lineNumber()
	<<endl;
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

	    if (!atts.value( "author").isEmpty() )
		model->setAuthor(atts.value( "author" ) );
	    if (!atts.value( "comment").isEmpty() )
		model->setComment (atts.value( "comment" ) );
	    if (!atts.value( "branchCount").isEmpty() )
	    {
		branchesTotal=atts.value("branchCount").toInt();
		if (branchesTotal>10)
		{
		    useProgress=true;
		    mainWindow->setProgressMaximum (branchesTotal);
		}
	    } 
		
	    if (!atts.value( "backgroundColor").isEmpty() )
	    {
		col.setNamedColor(atts.value("backgroundColor"));
		model->getScene()->setBackgroundBrush(col);
	    }	    
	    if (!atts.value( "defaultFont").isEmpty() )
	    {
		QFont font (atts.value("defaultFont"));
		model->setMapDefaultFont (font);
	    }	    
	    if (!atts.value( "defaultFontSize").isEmpty() )
	    {
		model->setMapDefaultFontSize (atts.value ("defaultFontSize").toDouble() );
	    }	    
	    if (!atts.value( "selectionColor").isEmpty() )
	    {
		col.setNamedColor(atts.value("selectionColor"));
		model->setSelectionColor(col);
	    }	    
	    if (!atts.value( "linkColorHint").isEmpty() ) 
	    {
		if (atts.value("linkColorHint")=="HeadingColor")
		    model->setMapLinkColorHint(LinkableMapObj::HeadingColor);
		else
		    model->setMapLinkColorHint(LinkableMapObj::DefaultColor);
	    }
	    if (!atts.value( "linkStyle").isEmpty() ) 
		model->setMapLinkStyle(atts.value("linkStyle"));
	    if (!atts.value( "linkColor").isEmpty() ) 
	    {
		col.setNamedColor(atts.value("linkColor"));
		model->setMapDefLinkColor(col);
	    }	
	    if (!atts.value( "defXLinkColor").isEmpty() ) 
	    {
		col.setNamedColor(atts.value("defXLinkColor"));
		model->setMapDefXLinkColor(col);
	    }	
	    if (!atts.value( "defXLinkWidth").isEmpty() ) 
		model->setMapDefXLinkWidth(atts.value("defXLinkWidth").toInt ());
	    if (!atts.value( "mapZoomFactor").isEmpty() ) 
		model->setMapZoomFactor(atts.value("mapZoomFactor").toDouble());
	    if (!atts.value( "mapRotationAngle").isEmpty() ) 
		model->setMapRotationAngle(atts.value("mapRotationAngle").toDouble());
	}   
	// Check version
	if (!atts.value( "version").isEmpty() ) 
	{
	    if (!checkVersion(atts.value("version")))
		QMessageBox::warning( 0, "Warning: Version Problem" ,
		   "<h3>Map is newer than VYM</h3>"
		   "<p>The map you are just trying to load was "
		   "saved using vym " +atts.value("version")+". "
		   "The version of this vym is " + vymVersion + 
		   ". If you run into problems after pressing "
		   "the ok-button below, updating vym should help.");
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
		    lastBranch=model->createBranch(lastBranch);
		} else  
		{
		    // Import Replace 
		    if (insertPos<0) 
		    {
			insertPos=lastBranch->num();
			model->clearItem (lastBranch);
		    } else
		    {
			TreeItem *pi=lastBranch->parent();
			if (pi && pi->isBranchLikeType())
			    lastBranch=model->addNewBranch(insertPos,lastBranch);
			else
			    qDebug()<<"xml-vym:  pi is no branch?!";
		    }
		}
	    } else
		// add mapCenter without parent
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
	laststate=state;
	state=StateHeading;
	if (!atts.value( "textColor").isEmpty() ) 
	{
	    col.setNamedColor(atts.value("textColor"));
	    lastBranch->setHeadingColor(col );
	}	
    } else if ( eName == "note" && 
		(state == StateMapCenter ||state==StateBranch))
    {	// only for backward compatibility (<1.4.6). Use htmlnote now.
	state=StateNote;
	if (!readNoteAttr (atts) ) return false;
    } else if ( eName == "htmlnote" && state == StateMapCenter) 
    {
	laststate=state;
	state=StateHtmlNote;
	no.clear();
	if (!atts.value( "fonthint").isEmpty() ) 
	    no.setFontHint(atts.value ("fonthint") );
    } else if ( eName == "vymnote" && 
		(state == StateMapCenter ||state==StateBranch)) 
    {
	laststate=state;
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
	laststate=state;
	state=StateHtmlNote;
	no.clear();
	if (!atts.value( "fonthint").isEmpty() ) 
	    no.setFontHint(atts.value ("fonthint") );
    } else if ( eName == "frame" && (state == StateBranch||state==StateMapCenter)) 
    {
	laststate=state;
	state=StateFrame;
	if (!readFrameAttr(atts)) return false;
    } else if ( eName == "xlink" && state == StateBranch ) 
    {
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
	textdata="<"+eName;
	readHtmlAttr(atts);
	textdata+=">";
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
	textdata+="<"+eName;
	readHtmlAttr(atts);
	textdata+=">";
    } else
        return false;   // Error
    return true;
}

bool parseVYMHandler::endElement  ( const QString&, const QString&, const QString &eName)
{
    /* Testing
    cout << "endElement </" <<qPrintable(eName)
	<<">  state=" <<state 
    //	<<"  laststate=" <<laststate
    //	<<"  stateStack="<<stateStack.last() 
    //	<<"  selString="<<model->getSelectString().toStdString()
	<<endl;
    */
    switch ( state ) 
    {
	case StateMap:
	    break;
        case StateMapCenter: 
	    model->emitDataHasChanged (lastBranch);
	    lastBranch=(BranchItem*)(lastBranch->parent());
	//  lastBranch->setLastSelectedBranch (0);  // Reset last selected to first child branch
            break;
        case StateBranch: 
	    // Empty branches may not be scrolled 
	    // (happens if bookmarks are imported)
	    if (lastBranch->isScrolled() && lastBranch->branchCount()==0) 
		lastBranch->unScroll();
	    model->emitDataHasChanged (lastBranch);

	    lastBranch=(BranchItem*)(lastBranch->parent());
	    lastBranch->setLastSelectedBranch (0);  // Reset last selected to first child branch
            break;
	case StateHtmlNote: 
	    no.setNote (textdata);  // Richtext note, needed anyway for backward compatibility
	    lastBranch->setNoteObj (no);
	    break;  
	case StateVymNote:	    // Might be richtext or plaintext with 
				    // version >= 1.13.8
	    no.setNote (textdata);
	    if (!no.isRichText())
		no.setNoteMasked (textdata);
	    lastBranch->setNoteObj (no);
	    break;  
        case StateHtml: 
	    textdata+="</"+eName+">";
	    if (eName=="html")
	    {
		//state=StateHtmlNote;  
		textdata.replace ("<br></br>","<br />");
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
    //cout << "characters \""<<ch.toStdString()<<"\"  state="<<state <<"  laststate="<<laststate<<endl;

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
	    textdata=ch;
	    break;
        case StateHtmlNote: 
	    textdata=ch;
	    break;
        case StateHtml:
	    textdata+=ch_org;
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
    // Format of links was changed several times:
    //
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

		if (!a.value( "color").isEmpty() ) 
		{
		    QColor col;
		    col.setNamedColor(a.value("color"));
		    li->setColor (col);
		}

		if (!a.value( "width").isEmpty() ) 
		{
		    bool okx;
		    li->setWidth(a.value ("width").toInt (&okx, 10));
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

    // Beginning in 1.13.2 xLinks are no longer parts of branches, but
    // a separate list after all the mapCenters within <vymmap> ... </vymmap>

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

		if (!a.value( "color").isEmpty() ) 
		{
		    QColor col;
		    col.setNamedColor(a.value("color"));
		    li->setColor (col);
		}

		if (!a.value( "width").isEmpty() ) 
		{
		    bool okx;
		    li->setWidth(a.value ("width").toInt (&okx, 10));
		}
		model->createLink (li,true);	// create MO by default
	    }
	}           
    }	
    return true;    
}

bool parseVYMHandler::readHtmlAttr (const QXmlAttributes& a)
{
    for (int i=1; i<=a.count(); i++)
	textdata+=" "+a.localName(i-1)+"=\""+a.value(i-1)+"\"";
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
