#include "xlink-wrapper.h"

#include "attributeitem.h"

#include "vymmodel.h"

#include <QJSEngine>

extern QJSEngine *scriptEngine;

XLinkWrapper::XLinkWrapper(Link *xl)
{
    //qDebug() << "Constr XLinkWrapper (ii)";
    xlinkInt = xl;
}

XLinkWrapper::~XLinkWrapper()
{
    //qDebug() << "Destr XLinkWrapper";
}

VymModel* XLinkWrapper::model() {return xlinkInt->getModel();}

Link* XLinkWrapper::xlink() {return xlinkInt;}

QString XLinkWrapper::headingText()
{
    return QString("Foo.");
    //return setResult(attributeItemInt->headingPlain());
}

