#include <iostream>
#include <QDebug>

#include "attribute.h"

using namespace std;

extern bool debug;

Attribute::Attribute()
{
    table=NULL;
    definition=NULL;
}

void Attribute::setKey (const QString &k, const AttributeType &t)
{
    if (!table)
    {
	qWarning (QString("Attribute::setKey (%1)  No table defined!\n").arg(k).toUtf8());
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
    qWarning (QString("Attribute::setKey (%1)  attribute already defined!\n").arg(k).toUtf8());
}

QString Attribute::getKey ()
{
    if (!table)
    {
	qWarning ("Attribute::getKey ()  No table defined!");
	return QString();   
    }
    if (!definition)
    {
	qWarning ("Attribute::getKey ()  No attribute defined!");
	return QString ();  
    }	
    return definition->getKey();
}

void Attribute::setValue(const QString &v)
{
    if (!table)
    {
	qWarning ()<<QString ("Attribute::setValue (%1)  No table defined!").arg(v);
	return;	
    }
    if (!definition)
    {
	qWarning ()<<QString ("Attribute::setValue (%1)  No attribute defined!").arg(v);
	return;	
    }	
    definition->setValue (v);
}

QVariant Attribute::getValue()
{
    if (!table)
    {
	qWarning ("Attribute::getValue  No table defined!");
	return QString();   
    }
    if (!definition)
    {
	qWarning()<<"Attribute::getValue  No attribute defined!";
	return QString();   
    }	
    QVariant v= definition->getValue();
    return v;
}

void Attribute::setType (const AttributeType &t)
{
    if (!table)
    {
	qWarning ()<<"Attribute::setType  No table defined!";
	return;
    }
    if (!definition)
    {
	qWarning()<<"Attribute::setType  No attribute defined!";
	return; 
    }	
    definition->setType (t);
}

AttributeType Attribute::getType()
{
    if (!table)
    {
	qWarning ()<<"Attribute::getType  No table defined!";
	return Undefined;   
    }
    if (!definition)
    {
	qWarning ()<<"Attribute::getType  No attribute defined!";
	return Undefined;   
    }	
    return definition->getType();
}

QString Attribute::getTypeString()
{
    if (!table)
    {
	qWarning ()<<"Attribute::getTypeString  No table defined!";
	return "Undefined"; 
    }
    if (!definition)
    {
	qWarning ()<<"Attribute::getTypeString  No attribute defined!";
	return "Undefined"; 
    }	
    return definition->getTypeString();
}

void Attribute::setTable (AttributeTable *at)
{
    if (at)
	table=at;
     else
	qWarning ()<<"Attribute::setTable  table==NULL";
    
}

AttributeTable* Attribute::getTable()
{
    return table;
}

QString Attribute::getDataXML()
{
    QString a=beginElement ("attribute");
    a+=attribut ("key",getKey());
    a+=attribut ("value",getValue().toString() );
    a+=attribut ("type",getTypeString () );
    return a;
}


///////////////////////////////////////////////////////////////
AttributeDef::AttributeDef()
{
}

AttributeDef::~AttributeDef()
{
}

void AttributeDef::setType (const AttributeType &t)
{
    type=t;
}

AttributeType AttributeDef::getType ()
{
    return type;
}

QString AttributeDef::getTypeString ()
{
    if (type==StringList)
	return "StringList";
    else if (type==FreeString)
	return "FreeString";
    else if (type==UniqueString)
	return "UniqueString";
    return "Undefined";
}

void AttributeDef::setKey (const QString &k)
{
    key=k;
}

void AttributeDef::setValue (const QString &v)
{
}

void AttributeDef::setValue (const QVariant &v)
{
    if (type==Undefined)
	qWarning ()<<"AttributeDef::setValue  No type defined!";
    else if (type==StringList)
	value=v;
    else if (type==UniqueString)
	value=v;
    else
	qWarning ()<<"AttributeDef::setValue Unknown type???";
	
}

QVariant AttributeDef::getValue ()
{
    return QVariant ();
}

QString AttributeDef::getKey ()
{
    return key;
}

///////////////////////////////////////////////////////////////
AttributeTable::AttributeTable()
{
    typeList
	<< "Undefined"
	<< "IntList"
	<< "FreeInt"
	<< "StringList"
	<< "FreeString"
	<< "UniqueString";
}

AttributeTable::~AttributeTable()
{
    clear();
}

void AttributeTable::clear ()
{
    attdefs.clear();
}

AttributeDef* AttributeTable::addKey (const QString &k, const AttributeType &t)
{
    for (int i=0; i<attdefs.count();++i)
    {
	if (attdefs.at(i)->getKey()==k )
	{
	    qWarning (QString ("AttributeTable::addKey (%1) already in table\n").arg(k).toUtf8());
	    return NULL;
	}
    }
    AttributeDef *ad=new AttributeDef;
    ad->setKey (k);
    ad->setType (t);
    attdefs.append (ad);
    return ad;
}

void AttributeTable::removeKey (const QString &k)
{
    for (int i=0; i<attdefs.count();++i)
    {
	if (attdefs.at(i)->getKey()==k )
	{
	    
	    delete (attdefs.at(i));
	    attdefs.removeAt (i);
	    return ;
	}
    }
    qWarning (QString ("AttributeTable::removeKey (%1) key not in table\n").arg(k).toUtf8());
}

AttributeDef* AttributeTable::getDef(const QString &key)
{
    for (int i=0; i<attdefs.count();++i)
	if (attdefs.at(i)->getKey()==key ) return attdefs.at(i);
    qWarning (QString ("AttributeTable::getDef (%1) key not in table\n").arg(key).toUtf8());
    return NULL;    
}

int AttributeTable::countKeys()
{
    return attdefs.count();
}

QStringList AttributeTable::getKeys ()
{
    QStringList kl;
    for (int i=0; i<attdefs.count();i++)
	kl.append (attdefs.at(i)->getKey());
    return kl;
}

QStringList AttributeTable::getTypes ()
{
    return typeList;
}

QString AttributeTable::getDataXML()
{
    return valueElement ("attributeList","key","value");
}
