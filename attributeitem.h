#ifndef ATTRIBUTEITEM_H
#define ATTRIBUTEITEM_H

#include <QStringList>
#include <QVariant>

#include "branchitem.h"

/*! \brief A key and a value 
    The data itself is stored in Attribute Definitions (AttributeDef). 
    A list of these tables AttributeTable is maintained for every MapEditor.
*/
class AttributeItem:public BranchItem {
public:
enum Type {
    Undefined,	//!< Undefined type
    IntList,	//!< Integer
    FreeInt,	//!< Integer
    StringList, //!< List of strings
    FreeString,	//!< String
    UniqueString//!< String which is unique in a map, e.g. for IDs
};

    AttributeItem(const QList<QVariant> &data, TreeItem *parent = 0);
    virtual ~AttributeItem();
    void set (const QString &k, const QString &v, const Type &t);
    void get (QString &k, QString &v, Type &t);
    void setKey (const QString &k);
    QString getKey ();
    void setValue (const QString &v);
    QVariant getValue ();
    void setType (const Type &t);
    AttributeItem::Type getAttributeType ();
    QString getTypeString ();
    void setInternal (bool b);
    bool isInternal();
    QString getDataXML();
protected:
    void createHeading();
    bool internal;	    //!< Internal attributes cannot be edited by user
    QString key;
    QVariant value;
    Type attrType;
};

#endif

