#ifndef XML_BASE
#define XML_BASE


//#include <QString>
#include <QXmlAttributes>

#include "file.h"

class VymModel;

/*! \brief Base class for parsing maps from XML documents */

class parseBaseHandler  : public QXmlDefaultHandler
{
public:
    parseBaseHandler();
    ~parseBaseHandler();
    QString errorProtocol();
    QString parseHREF(QString);
    virtual bool startElement ( const QString&, const QString&,
                        const QString& eName, const QXmlAttributes& atts )=0; 
    virtual bool   endElement ( const QString&, const QString&, const QString& )=0; 
    virtual bool characters   ( const QString&)=0;

    virtual QString errorString()=0;
    bool fatalError( const QXmlParseException&);
    void setModel (VymModel *);
    void setTmpDir (QString);
    void setInputFile (QString);
    void setLoadMode (const LoadMode &,int p=-1);

protected:
    QString     errorProt;

    LoadMode loadMode;
    int insertPos;

    bool isVymPart;
    int branchDepth; 
    VymModel *model;
    QString tmpDir; 
    QString inputFile;
}; 
#endif
