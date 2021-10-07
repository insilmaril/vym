#ifndef ATTRIBUTEITEM_H
#define ATTRIBUTEITEM_H

#include <QStringList>
#include <QVariant>

#include "branchitem.h"

/*! \brief A key and a value
    The data itself is stored in Attribute Definitions (AttributeDef).
    A list of these tables AttributeTable is maintained for every MapEditor.
*/
class AttributeItem : public BranchItem {
  public:
    enum Type {
        Undefined,   //!< Undefined type
        Integer,     //!< Integer
        DateTime,    //!< DateTime
        String       //!< String
    };

    AttributeItem(TreeItem *parent = nullptr);
    AttributeItem(const QString &k, const QString &v, TreeItem *parent = nullptr);
    virtual ~AttributeItem();
    void copy(AttributeItem *other);
    void set(const QString &k, const QString &v);
    void get(QString &k, QString &v, Type &t);
    void setKey(const QString &k);
    QString getKey();
    void setValue(const QString &v);
    void setValue(const qlonglong &n);
    void setValue(const QDateTime &dt);
    QVariant getValue();
    QDateTime getValueDateTime();
    using BranchItem::setType;
    virtual void setAttributeType(const Type &t);
    AttributeItem::Type getAttributeType();
    QString getAttributeTypeString();
    void setInternal(bool b);
    bool isInternal();
    QString getDataXML();

  protected:
    void createHeading();
    bool internal; //!< Internal attributes cannot be edited by user
    QString key;
    QVariant value;
    Type attrType;
};

#endif
