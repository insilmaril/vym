#ifndef XML_FREEMIND_H
#define XML_FREEMIND_H

#include "xml-base.h"


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

private:
    QString     errorProt;
    enum State 
    { 
	StateInit, 
	StateMap, 
	StateNode, 
	StateCloud,
	StateEdge, 
	StateIcon, 
	StateFont,
	StateArrowLink,
	StateHook,
	StateText
     };
		 
    State state;	     
    State laststate;
    QList <State> stateStack;
    BranchItem *lastBranchItem;
}; 
#endif
