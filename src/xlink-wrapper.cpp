#include "xlink-wrapper.h"

#include "attributeitem.h"
#include "misc.h"
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

QString XLinkWrapper::getColor()
{
    return setResult(xlinkInt->getPen().color().name());
}

int XLinkWrapper::getWidth()
{
    return setResult(xlinkInt->getPen().width());
}

QString XLinkWrapper::getPenStyle()
{
    return setResult(penStyleToString(xlinkInt->getPen().style()));
}

QString XLinkWrapper::getStyleBegin()
{
    return setResult(xlinkInt->getStyleBeginString());
}

QString XLinkWrapper::getStyleEnd()
{
    return setResult(xlinkInt->getStyleEndString());
}

void XLinkWrapper::setColor(const QString &color)
{
    QColor col(color);
    if (!col.isValid())
        scriptEngine->throwError(
                QJSValue::GenericError,
                QString("Could not set color to %1").arg(color));
    else
        model()->setXLinkColor(color, xlinkInt);
}

void XLinkWrapper::setStyle(const QString &style)
{
    model()->setXLinkStyle(style, xlinkInt);
}

void XLinkWrapper::setStyleBegin(const QString &style)
{
    model()->setXLinkStyleBegin(style, xlinkInt);
}

void XLinkWrapper::setStyleEnd(const QString &style)
{
    model()->setXLinkStyleEnd(style, xlinkInt);
}

void XLinkWrapper::setWidth(int w)
{
    model()->setXLinkWidth(w, xlinkInt);
}

