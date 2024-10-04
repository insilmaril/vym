#include "scripting-xlink-wrapper.h"

#include "attributeitem.h"
#include "misc.h"
#include "vymmodel.h"
#include "mainwindow.h"
#include "xlink.h"
//#include "xlinkitem.h"

extern Main *mainWindow;

XLinkWrapper::XLinkWrapper(XLink *xl)
{
    //qDebug() << "Constr XLinkWrapper (ii)";
    xlinkInt = xl;
}

XLinkWrapper::~XLinkWrapper()
{
    //qDebug() << "Destr XLinkWrapper";
}

VymModel* XLinkWrapper::model() {return xlinkInt->getModel();}

XLink* XLinkWrapper::xlink() {return xlinkInt;}

QString XLinkWrapper::getColor()
{
    QString r = xlinkInt->getPen().color().name();
    mainWindow->setScriptResult(r);
    return r;
}

int XLinkWrapper::getWidth()
{
    int r = xlinkInt->getPen().width();
    mainWindow->setScriptResult(r);
    return r;
}

QString XLinkWrapper::getPenStyle()
{
    QString r = penStyleToString(xlinkInt->getPen().style());
    mainWindow->setScriptResult(r);
    return r;
}

QString XLinkWrapper::getStyleBegin()
{
    QString r =  xlinkInt->getStyleBeginString();
    mainWindow->setScriptResult(r);
    return r;
}

QString XLinkWrapper::getStyleEnd()
{
    QString r =  xlinkInt->getStyleEndString();
    mainWindow->setScriptResult(r);
    return r;
}

void XLinkWrapper::setColor(const QString &color)
{
    QColor col(color);
    if (!col.isValid())
        mainWindow->abortScript(
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

