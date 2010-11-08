#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#include <QStringList>
#include <QVariant>

#include "xmlobj.h"

class AttributeTable;	//FIXME-3 remove from CVS
class AttributeDef;	//FIXME-3 remove from CVS

enum AttributeType {
    Undefined,	//!< Undefined type
    IntList,	//!< Free integer
    FreeInt,	//!< Free integer
    StringList, //!< List of strings, one can be attribute value
    FreeString,	//!< Any string can be attribute value, not unique
    UniqueString//!< UniqueString, e.g. for IDs
};

/*! \brief A key and a value 
    The data itself is stored in Attribute Definitions (AttributeDef). 
    A list of these tables AttributeTable is maintained for every MapEditor.
*/
class Attribute:public XMLObj {
public:
    Attribute();
    void setKey (const QString &k, const AttributeType &t);
    QString getKey ();
    void setValue (const QString &v);
    QVariant getValue ();
    void setType (const AttributeType &t);
    AttributeType getType ();
    QString getTypeString ();
    void setTable (AttributeTable *at);
    AttributeTable* getTable();
    QString getDataXML();
protected:
    AttributeTable *table;
    AttributeDef *definition;
    QString freeString;	    //!< String value for type FreeString
};


/*! \brief 
    Attribute definition, defines possible values and type of attribute.
*/
class AttributeDef {
public:
    AttributeDef();
    ~AttributeDef();
    void setType (const AttributeType &t);
    AttributeType getType();
    QString getTypeString ();
    void setKey (const QString &k);
    QString getKey ();
    void setValue (const QString &v);
    void setValue (const QVariant &v);
    QVariant getValue ();
private:
    QString key;
    AttributeType type;

    QVariant value;		//!< value (except FreeString, FreeInt ...
};

/*! \brief A table containing a list of keys and each of these keys has
   a list of default values. The keys and the values for each key are
   unique.
*/

class AttributeTable:public XMLObj{
public:
    AttributeTable();
    ~AttributeTable();
    void clear();
    AttributeDef* addKey (const QString &k, const AttributeType &t);	//!< Adds a key to the table
    void removeKey (const QString &k);	//!< Removes key and its default values
    AttributeDef* getDef(const QString &k); //!< Get defintion of attribute
    int countKeys();			//!< Return number of keys
    QStringList getKeys ();
    QStringList getTypes();
    QString getDataXML();

protected:
    QList <AttributeDef*> attdefs;
    QStringList typeList;
};



#endif

