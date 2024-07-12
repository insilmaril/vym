#ifndef ATTRIBUTEITEM_H
#define ATTRIBUTEITEM_H

#include <QStringList>
#include <QVariant>

#include "branchitem.h"

class AttributeWrapper;

/*! \brief A key and a value
    The data itself is stored in Attribute Definitions (AttributeDef).
    A list of these tables AttributeTable is maintained for every MapEditor.
*/
class AttributeItem : public BranchItem {
  public:
    AttributeItem(TreeItem *parent = nullptr);
    AttributeItem(const QString &k, const QVariant &v, TreeItem *parent = nullptr);
    virtual ~AttributeItem();
    void copy(AttributeItem *other);
    void init();
    AttributeWrapper* attributeWrapper();
    void setKey(const QString &k);
    QString key();
    void setValue(const QVariant &v);
    QVariant value();
    void updateHeading();
    using BranchItem::setType;
    void setInternal(bool b);
    bool isInternal();
    QString getDataXML();

  protected:
    bool internal; //!< Internal attributes cannot be edited by user
    QString keyInt;
    QVariant valueInt;
    AttributeWrapper *attributeWrapperInt;
};

#endif
