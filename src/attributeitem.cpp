#include "attributeitem.h"

#include <QDebug>

extern bool debug;

AttributeItem::AttributeItem(TreeItem *parent)
    : BranchItem(parent)
{
    //qDebug() << "Constr. AttrItem (parent)";
    TreeItem::setType(Attribute);
    internal = false;
    attrType = Undefined;
}

AttributeItem::AttributeItem(const QString &k, const QString &v, TreeItem *parent)
    : BranchItem(parent)
{
    //qDebug() << "Constr. AttrItem (k, v, parent)";
    TreeItem::setType(Attribute);
    internal = false;

    set(k, v);
}

AttributeItem::~AttributeItem() {
    //qDebug() << "Destr. AttrItem";
}

void AttributeItem::copy(AttributeItem *other)
{
    key = other->key;
    value = other->value;
    attrType = other->attrType;
}

void AttributeItem::set(const QString &k, const QString &v)
{
    key = k;
    value = QVariant(v);
    attrType = String;
    createHeading();
}

void AttributeItem::get(QString &k, QString &v, Type &t) // FIXME-3  Better use return tuple
    // https://stackoverflow.com/questions/321068/returning-multiple-values-from-a-c-function
{
    k = key;
    v = value.toString();
    t = attrType;
}

void AttributeItem::setKey(const QString &k) // FIXME-3 Check if key aready exists in branch?
{
    key = k;
    createHeading();
}

QString AttributeItem::getKey()
{
    return key;
}

void AttributeItem::setValue(const QString &v)
{
    value = v;
    attrType = String;
    createHeading();
}

void AttributeItem::setValue(const qlonglong &n)
{
    value = n;
    attrType = Integer;
    createHeading();
}

void AttributeItem::setValue(const QDateTime &dt)
{
    value = dt;
    attrType = DateTime;
    createHeading();
}

QVariant AttributeItem::getValue()
{
    return value;
}

void AttributeItem::setAttributeType(const Type &t)
{
    attrType = t;
}

AttributeItem::Type AttributeItem::getAttributeType()
{
    return attrType;
}

QString AttributeItem::getAttributeTypeString()
{
    switch (attrType) {
        case Integer:
            return "Integer";
        case String:
            return "String";
        case DateTime:
            return "DateTime";
        case Undefined:
            return "Undefined";
        }
    }

    void AttributeItem::setInternal(bool b) { internal = b; }

    bool AttributeItem::isInternal() { return internal; }

QString AttributeItem::getDataXML()
{
    QString a;
    a = attribut("key", getKey());
    a += attribut("value", getValue().toString());
    a += attribut("type", getAttributeTypeString());
    return singleElement("attribute", a);
}

void AttributeItem::createHeading() // FIXME-3 Visible in TreeEditor, should not go to MapEditor
{
    setHeadingPlainText(
        QString("[Attr] %1: %2").arg(key).arg(value.toString()));
}
