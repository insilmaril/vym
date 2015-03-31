#include "attributeitem.h"


extern bool debug;

AttributeItem::AttributeItem(const QList<QVariant> &data, TreeItem *parent):BranchItem (data,parent)
{
    TreeItem::setType (Attribute);
    internal=false;
}

AttributeItem::~AttributeItem()
{
}

void AttributeItem::set (const QString &k, const QString &v, const Type &)
{
    key=k;
    value=QVariant (v);
    createHeading();
}

void AttributeItem::get (QString &k, QString &v, Type &t)
{
    k=key;
    v=value.toString();
    t=attrType;
}

void AttributeItem::setKey (const QString &k)
{
/*
    if (!table)
    {
	qWarning (QString("AttributeItem::setKey (%1)  No table defined!\n").arg(k).ascii());
	return;	
    }
    
    if (!definition)
    {
	definition=table->getDef(k);
	if (!definition)
	{
	    table->addKey (k,t);
	    return; 
	}
    }	
    qWarning (QString("AttributeItem::setKey (%1)  attribute already defined!\n").arg(k).ascii());
    */
    key=k;
    createHeading();
}

QString AttributeItem::getKey ()
{
/*
    if (!table)
    {
	qWarning ("AttributeItem::getKey ()  No table defined!");
	return QString();   
    }
    if (!definition)
    {
	qWarning ("AttributeItem::getKey ()  No attribute defined!");
	return QString ();  
    }	
    return definition->getKey();
    */
    return key;
}

void AttributeItem::setValue(const QString &v)
{
/*
    if (!table)
    {
	qWarning (QString ("AttributeItem::setValue (%1)  No table defined!").arg(v));
	return;	
    }
    if (!definition)
    {
	qWarning (QString ("AttributeItem::setValue (%1)  No attribute defined!").arg(v));
	return;	
    }	
    definition->setValue (v);
*/
    value=v;
    createHeading();
}

QVariant AttributeItem::getValue()
{
/*
    if (!table)
    {
	qWarning ("AttributeItem::getValue  No table defined!");
	return QString();   
    }
    if (!definition)
    {
	qWarning ("AttributeItem::getValue  No attribute defined!");
	return QString();   
    }	
    QVariant v= definition->getValue();
    return v;
    */
    return value;
}

void AttributeItem::setType (const Type &t)
{
/*
    if (!table)
    {
	qWarning ("AttributeItem::setType  No table defined!");
	return;
    }
    if (!definition)
    {
	qWarning ("Attribute::setType  No attribute defined!");
	return; 
    }	
    definition->setType (t);
*/
    attrType=t;
}

AttributeItem::Type AttributeItem::getAttributeType()
{
/*
    if (!table)
    {
	qWarning ("AttributeItem::getType  No table defined!");
	return Undefined;   
    }
    if (!definition)
    {
	qWarning ("AttributeItem::getType  No attribute defined!");
	return Undefined;   
    }	
    return definition->getType();
*/
    return attrType;
}

QString AttributeItem::getTypeString()
{
/*
    if (!table)
    {
	qWarning ("AttributeItem::getTypeString  No table defined!");
	return "Undefined"; 
    }
    if (!definition)
    {
	qWarning ("Attribute::getTypeString  No AttributeItem defined!");
	return "Undefined"; 
    }	
    return definition->getTypeString();
*/  
    switch (attrType)
    {
	case IntList: return "IntList";
	case FreeInt: return "FreeInt";
	case StringList:return "StringList";
	case FreeString:return "FreeString";
	case UniqueString: return "UniqueString";
	default: return "Undefined";
    }
}

void AttributeItem::setInternal(bool b)
{
    internal=b;
}

bool AttributeItem::isInternal()
{
    return internal;
}

QString AttributeItem::getDataXML()
{
    QString a;
    a=attribut ("key",getKey());
    a+=attribut ("value",getValue().toString() );
    a+=attribut ("type",getTypeString () );
    return singleElement ("attribute",a);
}

void AttributeItem::createHeading()
{
    setHeadingText (QString ("K: %1 | V: %2").arg(key).arg(value.toString()));
}

