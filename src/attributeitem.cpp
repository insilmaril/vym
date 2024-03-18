#include "attributeitem.h"

#include <QApplication>
#include <QDebug>

extern bool debug;

AttributeItem::AttributeItem(TreeItem *parent)
    : BranchItem(parent)
{
    //qDebug() << "Constr. AttrItem (parent)";
    TreeItem::setType(Attribute);
    internal = false;
}

AttributeItem::AttributeItem(const QString &k, const QVariant &v, TreeItem *parent)
    : BranchItem(parent)
{
    //qDebug() << "Constr. AttrItem (k, v, parent)";
    TreeItem::setType(Attribute);
    internal = false;

    keyInt = k;
    setValue(v);
}

AttributeItem::~AttributeItem() {
    //qDebug() << "Destr. AttrItem";
}

void AttributeItem::copy(AttributeItem *other)
{
    keyInt = other->keyInt;
    valueInt = other->valueInt;
}

/* void AttributeItem::set(const QString &k, const QString &v) // FIXME-0 remove
{
    keyInt = k;
    valueInt = QVariant(v);
    updateHeading();
}
*/

void AttributeItem::setKey(const QString &k)
{
    keyInt = k;
    updateHeading();
}

QString AttributeItem::key()
{
    return keyInt;
}

void AttributeItem::setValue(const QVariant &v)
{
    valueInt = v;
    updateHeading();
}

QVariant AttributeItem::value()
{
    return valueInt;
}

void AttributeItem::updateHeading()
{
    setHeadingPlainText(
        QString("[Attr] %1: %2").arg(keyInt).arg(valueInt.toString()));
}

void AttributeItem::setInternal(bool b) { internal = b; }

bool AttributeItem::isInternal() { return internal; }

QString AttributeItem::getDataXML()
{
    QString a;
    a = attribute("key", keyInt);
    a += attribute("value", valueInt.toString());
    a += attribute("type", valueInt.typeName());
    return singleElement("attribute", a);
}
