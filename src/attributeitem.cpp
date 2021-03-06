#include "attributeitem.h"

extern bool debug;

AttributeItem::AttributeItem(const QList<QVariant> &data, TreeItem *parent)
    : BranchItem(data, parent)
{
    TreeItem::setType(Attribute);
    internal = false;
}

AttributeItem::~AttributeItem() {}

void AttributeItem::set(const QString &k, const QString &v, const Type &)
{
    key = k;
    value = QVariant(v);
    createHeading();
}

void AttributeItem::get(QString &k, QString &v, Type &t)
{
    k = key;
    v = value.toString();
    t = attrType;
}

void AttributeItem::setKey(const QString &k) // FIXME-2 Check if key aready exists here
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
    createHeading();
}

QVariant AttributeItem::getValue()
{
    return value;
}

void AttributeItem::setType(const Type &t)
{
    attrType = t;
}

AttributeItem::Type AttributeItem::getAttributeType()
{
    return attrType;
}

QString AttributeItem::getTypeString()
{
    switch (attrType) {
    case IntList:
        return "IntList";
    case FreeInt:
        return "FreeInt";
    case StringList:
        return "StringList";
    case FreeString:
        return "FreeString";
    case UniqueString:
        return "UniqueString";
    default:
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
    a += attribut("type", getTypeString());
    return singleElement("attribute", a);
}

void AttributeItem::createHeading() // FIXME-3 Visible in TreeEditor, should not go to MapEditor
{
    setHeadingPlainText(
        QString("[Attr] %1: %2").arg(key).arg(value.toString()));
}
