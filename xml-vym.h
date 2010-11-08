#ifndef XML_H
#define XML_H

#include "xml-base.h"

class BranchItem;

/*! \brief Parsing VYM maps from XML documents */

class parseVYMHandler  : public parseBaseHandler
{
public:
    bool startDocument();
    bool startElement ( const QString&, const QString&,
                        const QString& eName, const QXmlAttributes& atts ); 
    bool   endElement ( const QString&, const QString&, const QString& ); 
    bool characters   ( const QString&);
    QString errorString();
    bool readBranchAttr (const QXmlAttributes&);
    bool readFrameAttr (const QXmlAttributes&);
    bool readOOAttr (const QXmlAttributes&);
    bool readNoteAttr (const QXmlAttributes&);
    bool readImageAttr (const QXmlAttributes&);
    bool readXLinkAttr (const QXmlAttributes&);
    bool readLinkNewAttr (const QXmlAttributes&);
    bool readHtmlAttr (const QXmlAttributes&);
    bool readSettingAttr (const QXmlAttributes&);

private:
    enum State 
    { 
	StateInit, 
	StateMap, 
	StateMapSelect, 
	StateMapSetting,
	StateMapCenter, 
	StateBranch, 
	StateBranchXLink,	    // Obsolete
	StateVymNote,
	StateHtmlNote,		    // Obsolete >= 1.13.6
	StateHtml, 
	StateFrame,
	StateStandardFlag,
	StateNote,		    // Obsolete >= 1.4.6
	StateImage,
	StateHeading,
	StateLink,
	StateAttribute
     };

     int branchesCounter;
     int branchesTotal;

    State state;	     
    State laststate;
    QList <State> stateStack;
    QString textdata;
    NoteObj no;

    BranchItem* lastBranch;
    ImageItem* lastImage;
    MapItem* lastMI;

    bool useProgress;
}; 
#endif
