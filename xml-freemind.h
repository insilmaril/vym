#ifndef XML_FREEMIND_H
#define XML_FREEMIND_H

#include "xml-base.h"

class BranchItem;
class VymModel;

/*! \brief Parsing Freemind maps from XML documents */

class parseFreemindHandler  : public parseBaseHandler
{
public:
    bool startDocument();
    QString parseHREF(QString);
    bool startElement ( const QString&, const QString&,
                        const QString& eName, const QXmlAttributes& atts ); 
    bool   endElement ( const QString&, const QString&, const QString& ); 
    bool characters   ( const QString&);
    QString errorString();
    bool readNodeAttr (const QXmlAttributes&);
    bool readRichContentAttr (const QXmlAttributes&);

private:
    QString     errorProt;
    enum State 
    { 
	StateInit, 
	StateAttributeRegistry,
	StateAttributeName,
	StateAttributeValue,
	StateMap, 
	StateNode, 
	StateProperties, 
	StateMapStyles, 
	StateCloud,
	StateEdge, 
	StateIcon, 
	StateFont,
	StateArrowLink,
	StateHook,
	StateText,
	StateRichContent,
	StateHtml,
        StateUnknown
     };
    enum HtmlPurpose {Node,Note,Unknown};
    HtmlPurpose htmlPurpose;
		 
    State state;	     
    QList <State> stateStack;
    BranchItem *lastBranch;
    BranchItem *mapCenter;
    BranchItem *mainBranchLeft;
    BranchItem *mainBranchRight;
}; 
#endif
